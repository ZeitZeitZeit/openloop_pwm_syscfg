#!/usr/bin/env python3
"""
sopwm_sim.py — Firmware-matched SOPWM schedule + 3-phase PWM simulation.

Mirrors pwm/sopwm/sopwm.c:
  SOPWM_SetFundamentalHz, SOPWM_GetAngles, sopwm_bin_lut,
  sopwm_force_halfwave_toggle, sopwm_frame_closure, sopwm_copy_rotated_phase.

Inputs (same names as main.c watch variables):
  N_cmd, m_cmd, f_fund_cmd

Output:
  Phase A/B/C leg PWM waveforms over 2 fundamental cycles (+ optional plot/CSV).

Usage:
  python tools/sopwm_sim.py --N 15 --m 0.5 --f 500
  python tools/sopwm_sim.py --N 13 --m 0.5 --f 1000 --csv out.csv
  python tools/sopwm_sim.py --N 15 --m 0.5 --f 800 --no-plot
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parent.parent
SOPWM_C = ROOT / "pwm" / "sopwm" / "sopwm.c"

# sopwm.h
F_CARR_HZ = 50_000.0
F_FUND_MIN_HZ = 400.0
F_FUND_MAX_HZ = 1000.0
N_CARR_MAX = 125
TBPRD = 2000
CMP_OFF = 0xFFFF
M_COUNT = 100
VALID_N = (7, 9, 11, 13, 15)
N_COLS = {7: 3, 9: 4, 11: 5, 13: 6, 15: 7}


@dataclass
class TimingInfo:
    f_fund_hz: float
    n_carr: int
    span_deg: float
    mid_slot_a: int
    phase_b_offset: int
    phase_c_offset: int
    lut_n_base: int
    dropped_angles: int


@dataclass
class SimResult:
    timing: TimingInfo
    schedules: dict[str, list[dict]]  # A, B, C slot tables
    t_ms: np.ndarray
    pwm_a: np.ndarray
    pwm_b: np.ndarray
    pwm_c: np.ndarray


# ---------------------------------------------------------------------------
# LUT load (from sopwm.c)
# ---------------------------------------------------------------------------

def _load_lut_tables() -> dict[int, list[list[float]]]:
    text = SOPWM_C.read_text(encoding="utf-8")
    out: dict[int, list[list[float]]] = {}
    for n in VALID_N:
        name = f"SOPWM_LUT_N{n}"
        m = re.search(rf"{name}\[.*?\]\s*=\s*\{{(.*?)\}};", text, re.DOTALL)
        if not m:
            raise FileNotFoundError(f"{name} not found in {SOPWM_C}")
        cols = N_COLS[n]
        rows: list[list[float]] = []
        for rm in re.finditer(r"\{([^}]+)\}", m.group(1)):
            vals = [float(v.replace("f", "")) for v in rm.group(1).split(",")]
            if len(vals) == cols:
                rows.append(vals)
        if len(rows) != M_COUNT:
            raise ValueError(f"{name}: expected {M_COUNT} rows, got {len(rows)}")
        out[n] = rows
    return out


_LUT = _load_lut_tables()


# ---------------------------------------------------------------------------
# Firmware-equivalent helpers
# ---------------------------------------------------------------------------

def get_angles(m: float, n: int) -> list[float]:
    """SOPWM_GetAngles() in sopwm.h."""
    if m < 0.01 or m > 1.0 or n not in VALID_N:
        return []
    idx = int(m * 100.0 - 1.0 + 0.5)
    if idx >= M_COUNT:
        idx = M_COUNT - 1
    return list(_LUT[n][idx])


def set_fundamental_hz(f_fund: float) -> TimingInfo:
    """SOPWM_SetFundamentalHz() — returns timing; does not mutate globals."""
    if f_fund < F_FUND_MIN_HZ:
        f_fund = F_FUND_MIN_HZ
    elif f_fund > F_FUND_MAX_HZ:
        f_fund = F_FUND_MAX_HZ

    n_min = int(F_CARR_HZ / F_FUND_MAX_HZ + 0.5)
    n_max = int(F_CARR_HZ / F_FUND_MIN_HZ + 0.5)
    n_new = int(F_CARR_HZ / f_fund + 0.5)
    n_new = max(n_min, min(n_max, n_new))

    span = 360.0 / n_new
    mid_slot = int(180.0 / span)
    if mid_slot >= n_new:
        mid_slot = n_new - 1

    off_b = ((2 * n_new) + 1) // 3
    off_c = (n_new + 1) // 3

    return TimingInfo(
        f_fund_hz=f_fund,
        n_carr=n_new,
        span_deg=span,
        mid_slot_a=mid_slot,
        phase_b_offset=off_b,
        phase_c_offset=off_c,
        lut_n_base=0,
        dropped_angles=0,
    )


def _expand_quarter_wave(base: list[float]) -> list[float]:
    """SOPWM_BuildSchedule quarter-wave expansion."""
    rev = base[::-1]
    return (
        list(base)
        + [180.0 - x for x in rev]
        + [180.0 + x for x in base]
        + [360.0 - x for x in rev]
    )


def _bin_lut(
    global_deg: list[float],
    n_carr: int,
    span: float,
    tbprd: int,
) -> tuple[list[dict], int]:
    """sopwm_bin_lut()."""
    tbl = [{"cmpa": CMP_OFF, "cmpb": CMP_OFF} for _ in range(n_carr)]
    dropped = 0

    for ang in global_deg:
        slot = int(ang / span)
        if slot >= n_carr:
            slot = n_carr - 1
        local = ang - slot * span
        counts = int(round((local / span) * tbprd))
        if counts >= tbprd:
            counts = tbprd - 1

        s = tbl[slot]
        if s["cmpa"] == CMP_OFF:
            s["cmpa"] = counts
        elif s["cmpb"] == CMP_OFF:
            if counts < s["cmpa"]:
                s["cmpa"], s["cmpb"] = counts, s["cmpa"]
            else:
                s["cmpb"] = counts
        else:
            dropped += 1

    return tbl, dropped


def _insert_counts(slot: dict, counts: int) -> None:
    """sopwm_insert_counts()."""
    if slot["cmpa"] == CMP_OFF:
        slot["cmpa"] = counts
    elif slot["cmpb"] == CMP_OFF:
        if counts < slot["cmpa"]:
            slot["cmpb"] = slot["cmpa"]
            slot["cmpa"] = counts
        else:
            slot["cmpb"] = counts


def _force_halfwave_toggle(tbl: list[dict], span: float, n_carr: int, tbprd: int) -> None:
    """sopwm_force_halfwave_toggle()."""
    mid_deg = 180.0
    mid_slot = int(mid_deg / span)
    if mid_slot >= n_carr:
        mid_slot = n_carr - 1

    local = mid_deg - mid_slot * span
    mid_counts = int(round((local / span) * tbprd))
    if mid_counts >= tbprd:
        mid_counts = tbprd - 1

    s = tbl[mid_slot]
    if mid_counts == 0:
        if s["cmpa"] == CMP_OFF:
            s["cmpa"] = 0
        elif s["cmpa"] != 0 and s["cmpb"] == CMP_OFF:
            s["cmpb"] = s["cmpa"]
            s["cmpa"] = 0
        elif s["cmpb"] == CMP_OFF:
            s["cmpb"] = 0
    else:
        _insert_counts(s, mid_counts)


def _frame_closure(tbl: list[dict], span: float, n_carr: int, tbprd: int) -> None:
    """sopwm_frame_closure()."""
    _force_halfwave_toggle(tbl, span, n_carr, tbprd)

    n_toggles = sum(
        (tbl[i]["cmpa"] != CMP_OFF) + (tbl[i]["cmpb"] != CMP_OFF)
        for i in range(n_carr)
    )

    if n_toggles & 1:
        # Close the odd toggle at the PERIOD END (~360 deg), never at slot 0
        # counter 0, so slot 0 stays event-free for the AQ re-anchor SW force.
        k = n_carr
        while k > 0:
            k -= 1
            if (tbl[k]["cmpb"] == CMP_OFF and tbl[k]["cmpa"] != CMP_OFF
                    and tbl[k]["cmpa"] < tbprd - 1):
                tbl[k]["cmpb"] = tbprd - 1
                break
            if tbl[k]["cmpa"] == CMP_OFF:
                tbl[k]["cmpa"] = tbprd - 1
                break


def _copy_rotated_phase(src: list[dict], offset: int, n_carr: int) -> list[dict]:
    """sopwm_copy_rotated_phase()."""
    return [dict(src[(k + offset) % n_carr]) for k in range(n_carr)]


def build_schedule(m: float, n_cmd: int, f_fund: float, tbprd: int = TBPRD) -> tuple[TimingInfo, dict[str, list[dict]]]:
    """SOPWM_SetFundamentalHz + SOPWM_BuildSchedule for phases A/B/C."""
    timing = set_fundamental_hz(f_fund)
    n_carr = timing.n_carr
    span = timing.span_deg

    base = get_angles(m, n_cmd)
    if not base:
        raise ValueError(f"Invalid m={m} or N={n_cmd}")

    timing.lut_n_base = len(base)
    all_angles = _expand_quarter_wave(base)

    tbl_a, dropped = _bin_lut(all_angles, n_carr, span, tbprd)
    timing.dropped_angles = dropped
    _frame_closure(tbl_a, span, n_carr, tbprd)

    tbl_b = _copy_rotated_phase(tbl_a, timing.phase_b_offset, n_carr)
    tbl_c = _copy_rotated_phase(tbl_a, timing.phase_c_offset, n_carr)

    return timing, {"A": tbl_a, "B": tbl_b, "C": tbl_c}


# ---------------------------------------------------------------------------
# PWM from schedule (EPWM1A toggle-on-CMP, up-count)
# ---------------------------------------------------------------------------

def schedule_to_pwm(
    tbl: list[dict],
    timing: TimingInfo,
    n_fund_cycles: int,
    tbprd: int = TBPRD,
    invert: bool = False,
) -> np.ndarray:
    """
    Leg PWM from one phase schedule over n_fund_cycles fundamentals.

    Toggle at each compare event (EPWM UP-count CMP match); initial level LOW.
    """
    n_carr = timing.n_carr
    span = timing.span_deg
    steps_per_fund = n_carr * tbprd
    steps = steps_per_fund * n_fund_cycles
    toggles = np.zeros(steps, dtype=np.uint8)

    for fund in range(n_fund_cycles):
        base_step = fund * steps_per_fund
        for cy in range(n_carr):
            base_deg = cy * span
            for key in ("cmpa", "cmpb"):
                c = tbl[cy][key]
                if c == CMP_OFF:
                    continue
                ang_deg = base_deg + (c / tbprd) * span
                idx = base_step + int(round((ang_deg / 360.0) * steps_per_fund))
                if 0 <= idx < steps:
                    toggles[idx] ^= 1

    state = np.bitwise_xor.accumulate(toggles).astype(np.float64)
    if invert:
        state = 1.0 - state
    return state


def run_sim(
    n_cmd: int,
    m_cmd: float,
    f_fund_cmd: float,
    n_fund_cycles: int = 2,
    invert_phase_c: bool = True,
    tbprd: int = TBPRD,
) -> SimResult:
    timing, schedules = build_schedule(m_cmd, n_cmd, f_fund_cmd, tbprd)

    pwm_a = schedule_to_pwm(schedules["A"], timing, n_fund_cycles, tbprd, False)
    pwm_b = schedule_to_pwm(schedules["B"], timing, n_fund_cycles, tbprd, False)
    pwm_c = schedule_to_pwm(schedules["C"], timing, n_fund_cycles, tbprd, invert_phase_c)

    steps = len(pwm_a)
    dt_ms = 1000.0 / (timing.f_fund_hz * timing.n_carr * tbprd)
    t_ms = np.arange(steps) * dt_ms

    return SimResult(
        timing=timing,
        schedules=schedules,
        t_ms=t_ms,
        pwm_a=pwm_a,
        pwm_b=pwm_b,
        pwm_c=pwm_c,
    )


def print_summary(res: SimResult, n_cmd: int, m_cmd: float) -> None:
    t = res.timing
    print(f"SOPWM sim: N_cmd={n_cmd}  m_cmd={m_cmd:.2f}  f_fund_cmd={t.f_fund_hz:.1f} Hz")
    print(f"  n_carr={t.n_carr}  span={t.span_deg:.4f} deg/slot  T_fund={1000/t.f_fund_hz:.3f} ms")
    print(f"  mid_slot_a={t.mid_slot_a}  off_b={t.phase_b_offset}  off_c={t.phase_c_offset}")
    print(f"  lut_n_base={t.lut_n_base}  dropped_angles={t.dropped_angles}")
    print(f"  output: {len(res.t_ms)} samples = {len(res.t_ms) // (res.timing.n_carr * TBPRD)} fundamental cycles")


def plot_result(res: SimResult, n_cmd: int, m_cmd: float, save: str | None = None) -> None:
    import matplotlib.pyplot as plt

    t = res.timing
    fig, axes = plt.subplots(3, 1, figsize=(14, 7), sharex=True)
    fig.suptitle(
        f"SOPWM simulation — N={n_cmd}  m={m_cmd:.2f}  f={t.f_fund_hz:.0f} Hz  "
        f"(n_carr={t.n_carr}, 2 cycles)",
        fontsize=11,
        fontweight="bold",
    )

    for ax, y, label, color in zip(
        axes,
        (res.pwm_a, res.pwm_b, res.pwm_c),
        ("Phase A", "Phase B", "Phase C (RED invert)"),
        ("#E63946", "#2A9D8F", "#457B9D"),
    ):
        ax.plot(res.t_ms, y, color=color, linewidth=0.8)
        ax.set_ylabel(label)
        ax.set_ylim(-0.15, 1.15)
        ax.grid(True, alpha=0.3)
        for k in range(3):
            ax.axvline(k * 1000.0 / t.f_fund_hz, color="gray", ls=":", alpha=0.5)

    axes[-1].set_xlabel("Time (ms)")
    axes[0].set_xlim(0, 2 * 1000.0 / t.f_fund_hz)
    plt.tight_layout()
    if save:
        fig.savefig(save, dpi=150)
        print(f"Saved plot: {save}")
    else:
        plt.show()


def write_csv(path: Path, res: SimResult) -> None:
    data = np.column_stack([res.t_ms, res.pwm_a, res.pwm_b, res.pwm_c])
    np.savetxt(
        path,
        data,
        delimiter=",",
        header="t_ms,pwm_a,pwm_b,pwm_c",
        comments="",
    )
    print(f"Saved CSV: {path}")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Simulate SOPWM phases A/B/C (firmware-matched logic)."
    )
    parser.add_argument("--N", type=int, default=15, dest="n_cmd", choices=VALID_N)
    parser.add_argument("--m", type=float, default=0.5, dest="m_cmd")
    parser.add_argument("--f", type=float, default=500.0, dest="f_fund_cmd")
    parser.add_argument(
        "--cycles", type=int, default=2, help="Fundamental cycles to simulate"
    )
    parser.add_argument(
        "--no-invert-c",
        action="store_true",
        help="Do not invert phase C (skip EPWM2 RED Active Low)",
    )
    parser.add_argument("--csv", type=str, default="", help="Write CSV path")
    parser.add_argument("--save", type=str, default="", help="Save PNG instead of show")
    parser.add_argument("--no-plot", action="store_true")
    args = parser.parse_args()

    try:
        res = run_sim(
            args.n_cmd,
            args.m_cmd,
            args.f_fund_cmd,
            n_fund_cycles=args.cycles,
            invert_phase_c=not args.no_invert_c,
        )
    except ValueError as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1

    print_summary(res, args.n_cmd, args.m_cmd)

    if args.csv:
        write_csv(Path(args.csv), res)

    if not args.no_plot:
        try:
            plot_result(
                res,
                args.n_cmd,
                args.m_cmd,
                save=args.save or None,
            )
        except ImportError:
            print("matplotlib not installed — use --csv or pip install matplotlib")

    return 0


if __name__ == "__main__":
    sys.exit(main())
