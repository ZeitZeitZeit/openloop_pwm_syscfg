#!/usr/bin/env python3
"""
check_events_per_cycle.py
=========================
Reads all SOPWM LUT angles from sopwm.c, expands every row to the full
4·M switching angles using quarter-wave symmetry, then checks how many
switching events fall in each sawtooth carrier cycle.

Rule: no single carrier cycle should contain more than 2 events.

Parameters:
  F_FUND  =  1 kHz  (fundamental)
  F_CARR  = 20 kHz  (sawtooth carrier)
  → N_CARR = 20 cycles per fundamental period
  → Each cycle spans 2π/20 = π/10 radians of fundamental angle
"""

import re
import sys
from pathlib import Path

import numpy as np

PI = np.pi

# ── Config ────────────────────────────────────────────────────────────────────
SOPWM_C   = Path(__file__).parent / "pwm" / "sopwm" / "sopwm.c"
F_FUND    = 1_000
F_CARR    = 50_000
N_CARR    = F_CARR // F_FUND          # 50 cycles per fundamental period
CYCLE_SPAN = 2 * PI / N_CARR          # radians covered by one carrier cycle
MAX_EVENTS = 2                        # tolerance limit

# LUT names → M (number of base angles per quarter-wave)
LUT_INFO = {
    "SOPWM_LUT_N7":  3,
    "SOPWM_LUT_N9":  4,
    "SOPWM_LUT_N11": 5,
    "SOPWM_LUT_N13": 6,
    "SOPWM_LUT_N15": 7,
}


# ── Parse sopwm.c ─────────────────────────────────────────────────────────────

def parse_luts(path: Path) -> dict:
    """
    Return dict: lut_name → np.ndarray shape (100, M) in degrees.
    Skips rows that are all-zero (placeholder entries).
    """
    text = path.read_text(encoding="utf-8")
    luts = {}

    for lut_name, n_cols in LUT_INFO.items():
        # Find the block that starts with this LUT name
        pattern = re.compile(
            rf"{re.escape(lut_name)}\s*\[.*?\]\s*=\s*\{{(.*?)\}};",
            re.DOTALL,
        )
        m = pattern.search(text)
        if not m:
            print(f"  WARNING: {lut_name} not found in {path.name}")
            continue

        block = m.group(1)
        # Extract every row: { val, val, ... val }
        row_pattern = re.compile(r"\{([^}]+)\}")
        rows = []
        for row_m in row_pattern.finditer(block):
            vals = [float(v.replace("f", "")) for v in row_m.group(1).split(",")]
            if len(vals) == n_cols and any(v != 0.0 for v in vals):
                rows.append(vals)

        if rows:
            luts[lut_name] = np.array(rows)   # degrees

    return luts


# ── Quarter-wave expansion ────────────────────────────────────────────────────

def expand(base_deg: np.ndarray) -> np.ndarray:
    """
    base_deg: 1-D array of M angles in degrees, all in (0, 90).
    Returns 4·M switching angles in radians covering [0, 2π].
    """
    base = np.deg2rad(base_deg)
    rev  = base[::-1]
    return np.concatenate([base, PI - rev, PI + base, 2*PI - rev])


# ── Event-per-cycle audit ─────────────────────────────────────────────────────

def events_per_cycle(angles_rad: np.ndarray) -> np.ndarray:
    """
    Returns an int array of length N_CARR giving the number of switching
    events that fall in each carrier cycle.
    """
    counts = np.zeros(N_CARR, dtype=int)
    for ang in angles_rad:
        idx = int(ang / CYCLE_SPAN)
        if 0 <= idx < N_CARR:
            counts[idx] += 1
    return counts


# ── Main ──────────────────────────────────────────────────────────────────────

def main():
    if not SOPWM_C.exists():
        print(f"ERROR: {SOPWM_C} not found.")
        sys.exit(1)

    luts = parse_luts(SOPWM_C)
    if not luts:
        print("ERROR: No LUT data parsed.")
        sys.exit(1)

    sep = "─" * 70
    print(sep)
    print(f"  SOPWM carrier-event check")
    print(f"  F_fund={F_FUND} Hz  |  F_carrier={F_CARR} Hz  |  "
          f"N_cycles={N_CARR}  |  max_events/cycle={MAX_EVENTS}")
    print(sep)

    total_rows    = 0
    total_violate = 0

    for lut_name, data in luts.items():
        M         = LUT_INFO[lut_name]
        n_rows    = len(data)
        violations = []

        for row_idx, base_deg in enumerate(data):
            m_val   = (row_idx + 1) * 0.01
            all_ang = expand(base_deg)
            counts  = events_per_cycle(all_ang)
            worst   = counts.max()

            if worst > MAX_EVENTS:
                cycles_over = np.where(counts > MAX_EVENTS)[0].tolist()
                violations.append((m_val, worst, counts, cycles_over))

        total_rows    += n_rows
        total_violate += len(violations)
        status = "✓  PASS" if not violations else f"✗  {len(violations)} VIOLATIONS"
        print(f"\n  {lut_name}  (M={M}, {n_rows} m-values)  →  {status}")

        if violations:
            print(f"  {'m':>6}  {'max_events':>10}  {'cycles_over_limit'}")
            print("  " + "─" * 50)
            for m_val, worst, counts, cycles_over in violations:
                cycle_str = ", ".join(
                    f"C{c+1}({counts[c]})" for c in cycles_over
                )
                print(f"  {m_val:6.2f}  {worst:>10}  {cycle_str}")
        else:
            # Show worst-case max even for passing LUTs
            all_maxes = []
            for base_deg in data:
                all_ang   = expand(base_deg)
                all_maxes.append(events_per_cycle(all_ang).max())
            print(f"  Worst events in any single cycle across all m: {max(all_maxes)}")

    print(f"\n{sep}")
    if total_violate == 0:
        print(f"  OVERALL: ALL PASS  ({total_rows} LUT rows checked)")
    else:
        print(f"  OVERALL: {total_violate} VIOLATION ROWS found across all LUTs")
    print(sep)

    # ── Worst-case detail for N15 ─────────────────────────────────────────────
    if "SOPWM_LUT_N15" not in luts:
        return

    print(f"\n{'─'*70}")
    print("  SOPWM_LUT_N15 — worst-case cycles across ALL m values")
    print(f"  Carrier span per cycle: {np.degrees(CYCLE_SPAN):.3f}° = {CYCLE_SPAN:.6f} rad")
    print(f"{'─'*70}")

    data15   = luts["SOPWM_LUT_N15"]
    M        = LUT_INFO["SOPWM_LUT_N15"]

    # Build per-m counts matrix  shape (100, N_CARR)
    counts_matrix = np.zeros((len(data15), N_CARR), dtype=int)
    for row_idx, base_deg in enumerate(data15):
        counts_matrix[row_idx] = events_per_cycle(expand(base_deg))

    # Worst cycle (highest events) per m row
    max_per_row  = counts_matrix.max(axis=1)
    global_worst = max_per_row.max()
    worst_m_idx  = max_per_row.argmax()
    worst_m      = (worst_m_idx + 1) * 0.01

    print(f"\n  Global worst: m = {worst_m:.2f}  →  {global_worst} events in one cycle")

    # Distribution: how many rows hit each event count
    print(f"\n  Distribution of max-events-per-row across 100 m values:")
    print(f"  {'max events':>12}  {'rows':>6}  {'m values'}")
    print("  " + "─" * 55)
    for ev in range(1, global_worst + 1):
        rows_at = np.where(max_per_row == ev)[0]
        if len(rows_at) == 0:
            continue
        m_list = ", ".join(f"{(r+1)*0.01:.2f}" for r in rows_at[:8])
        suffix = f"  … (+{len(rows_at)-8} more)" if len(rows_at) > 8 else ""
        print(f"  {ev:>12}  {len(rows_at):>6}  {m_list}{suffix}")

    # Worst row detail: show which cycles are overloaded
    print(f"\n  Detail for worst row  m = {worst_m:.2f}:")
    ang_deg = data15[worst_m_idx]
    all_ang = expand(ang_deg)
    counts  = events_per_cycle(all_ang)
    print(f"  Base angles (deg): {', '.join(f'{a:.4f}' for a in ang_deg)}")
    print(f"  All {len(all_ang)} angles (deg): {', '.join(f'{np.degrees(a):.3f}' for a in all_ang)}")
    print(f"\n  Cycle  Events  Angles in cycle (deg)")
    print("  " + "─" * 60)
    for c_idx in range(N_CARR):
        if counts[c_idx] == 0:
            continue
        c_start = np.degrees(c_idx * CYCLE_SPAN)
        c_end   = np.degrees((c_idx + 1) * CYCLE_SPAN)
        in_cycle = [np.degrees(a) for a in all_ang
                    if c_idx * CYCLE_SPAN <= a < (c_idx + 1) * CYCLE_SPAN]
        flag = "  ← VIOLATION" if counts[c_idx] > MAX_EVENTS else ""
        print(f"  C{c_idx+1:02d}  ({c_start:6.2f}°–{c_end:6.2f}°)  "
              f"{counts[c_idx]} events  "
              f"{', '.join(f'{a:.3f}°' for a in in_cycle)}{flag}")

    print(f"\n{'─'*70}")


if __name__ == "__main__":
    main()
