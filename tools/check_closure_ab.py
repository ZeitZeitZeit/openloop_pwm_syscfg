#!/usr/bin/env python3
"""Verify frame closure for phases A/B/C: rotate closed A vs per-phase closure."""

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SOPWM_C = ROOT / "pwm" / "sopwm" / "sopwm.c"
OFF = 0xFFFF
TBPRD = 2000


def parse_lut_n15_m05():
    text = SOPWM_C.read_text(encoding="utf-8")
    m = re.search(r"SOPWM_LUT_N15\[.*?\]\s*=\s*\{(.*?)\};", text, re.DOTALL)
    rows = []
    for rm in re.finditer(r"\{([^}]+)\}", m.group(1)):
        vals = [float(v.replace("f", "")) for v in rm.group(1).split(",")]
        if len(vals) == 7:
            rows.append(vals)
    return rows[49]


def expand(base):
    b = list(base)
    rev = b[::-1]
    return b + [180.0 - x for x in rev] + [180.0 + x for x in b] + [360.0 - x for x in rev]


def bin_lut(angs, n_carr, tbprd):
    span = 360.0 / n_carr
    tbl = [{"cmpa": OFF, "cmpb": OFF} for _ in range(n_carr)]
    dropped = []
    for a in angs:
        slot = int(a / span)
        if slot >= n_carr:
            slot = n_carr - 1
        local = a - slot * span
        counts = int(round((local / span) * tbprd))
        if counts >= tbprd:
            counts = tbprd - 1
        s = tbl[slot]
        if s["cmpa"] == OFF:
            s["cmpa"] = counts
        elif s["cmpb"] == OFF:
            if counts < s["cmpa"]:
                s["cmpa"], s["cmpb"] = counts, s["cmpa"]
            else:
                s["cmpb"] = counts
        else:
            dropped.append(a)
    return tbl, dropped


def frame_closure(tbl, mid_slot, n_carr, tbprd):
    tbl = [dict(x) for x in tbl]
    if mid_slot >= n_carr:
        mid_slot = n_carr - 1
    s = tbl[mid_slot]
    if s["cmpa"] == OFF:
        s["cmpa"] = 0
    elif s["cmpb"] == OFF:
        s["cmpb"] = 0

    n_toggles = 0
    for i in range(n_carr):
        if tbl[i]["cmpa"] != OFF:
            n_toggles += 1
        if tbl[i]["cmpb"] != OFF:
            n_toggles += 1

    odd = n_toggles & 1
    slot0_fix = None
    if odd:
        s0 = tbl[0]
        if s0["cmpa"] == OFF:
            tbl[0]["cmpa"] = 0
            slot0_fix = "slot0 cmpa=0"
        elif s0["cmpa"] != 0 and s0["cmpb"] == OFF:
            tbl[0]["cmpb"] = tbl[0]["cmpa"]
            tbl[0]["cmpa"] = 0
            slot0_fix = "slot0 cmpa=0 cmpb=prev"
        elif tbl[n_carr - 1]["cmpb"] == OFF:
            tbl[n_carr - 1]["cmpb"] = tbprd - 1
            slot0_fix = f"slot{n_carr-1} cmpb={tbprd-1}"
    return tbl, n_toggles, odd, slot0_fix


def rotate(src, off, n):
    return [dict(src[(k + off) % n]) for k in range(n)]


def count_toggles(tbl, n):
    c = 0
    for i in range(n):
        if tbl[i]["cmpa"] != OFF:
            c += 1
        if tbl[i]["cmpb"] != OFF:
            c += 1
    return c


def phase_angle_at_slot(k, lag_deg, n):
    span = 360.0 / n
    return (k * span - lag_deg) % 360.0


def slot_nearest_angle(target, lag_deg, n):
    span = 360.0 / n
    best = 0
    best_err = 999.0
    for k in range(n):
        err = min(abs(phase_angle_at_slot(k, lag_deg, n) - target),
                  abs(phase_angle_at_slot(k, lag_deg, n) - target - 360.0))
        if err < best_err:
            best_err = err
            best = k
    return best


def pwm_state(tbl, n, tbprd=TBPRD):
    span = 360.0 / n
    steps = n * tbprd
    toggles = [0] * steps
    for cy in range(n):
        base_deg = cy * span
        for key in ("cmpa", "cmpb"):
            c = tbl[cy][key]
            if c == OFF:
                continue
            ang = base_deg + (c / tbprd) * span
            idx = int(ang / 360.0 * steps)
            if 0 <= idx < steps:
                toggles[idx] ^= 1
    state = [0] * steps
    cur = 0
    for i in range(steps):
        cur ^= toggles[i]
        state[i] = cur
    return state


def compare_tables(a, b, n):
    diffs = []
    for k in range(n):
        if a[k] != b[k]:
            diffs.append(k)
    return diffs


def analyze(n):
    off_b = ((2 * n) + 1) // 3
    off_c = (n + 1) // 3
    mid_a = n // 2
    base = parse_lut_n15_m05()
    angs = expand(base)

    a_raw, dropped = bin_lut(angs, n, TBPRD)
    a, nt, odd, fix = frame_closure(a_raw, mid_a, n, TBPRD)
    b_rot = rotate(a, off_b, n)
    c_rot = rotate(a, off_c, n)

    print(f"\n=== n={n}  N=15 m=0.5  off_b={off_b}  mid_a={mid_a} ===")
    print(f"  dropped={len(dropped)}  A toggles={nt} odd={odd}  fix={fix}")

    for name, lag, tbl in (("A", 0.0, a), ("B_rot", 120.0, b_rot), ("C_rot", 240.0, c_rot)):
        nt_ph = count_toggles(tbl, n)
        s180 = slot_nearest_angle(180.0, lag, n)
        s0 = slot_nearest_angle(0.0, lag, n)
        t180 = tbl[s180]
        t0 = tbl[s0]
        has0_180 = t180["cmpa"] == 0 or t180["cmpb"] == 0
        print(
            f"  {name:5s} toggles={nt_ph:2d} even={nt_ph%2==0}  "
            f"180@slot{s180}({phase_angle_at_slot(s180,lag,n):5.1f})={t180} cmp0={has0_180}  "
            f"0@slot{s0}({phase_angle_at_slot(s0,lag,n):5.1f})={t0}"
        )

    angs_b = [(x - 120.0) % 360.0 for x in angs]
    angs_c = [(x - 240.0) % 360.0 for x in angs]
    b_ind, _ = bin_lut(angs_b, n, TBPRD)
    c_ind, _ = bin_lut(angs_c, n, TBPRD)
    mid_b = slot_nearest_angle(180.0, 120.0, n)
    mid_c = slot_nearest_angle(180.0, 240.0, n)
    b_ind, _, _, _ = frame_closure(b_ind, mid_b, n, TBPRD)
    c_ind, _, _, _ = frame_closure(c_ind, mid_c, n, TBPRD)

    db = compare_tables(b_rot, b_ind, n)
    dc = compare_tables(c_rot, c_ind, n)
    print(f"  rotate vs independent B slots differ: {len(db)}/{n}")
    if db[:8]:
        for k in db[:8]:
            print(f"    slot {k:3d}: rot={b_rot[k]}  ind={b_ind[k]}")
    print(f"  rotate vs independent C slots differ: {len(dc)}/{n}")

    st_a = pwm_state(a, n)
    st_br = pwm_state(b_rot, n)
    st_bi = pwm_state(b_ind, n)
    half = len(st_a) // 2
    hw_a = sum(st_a[i] == (1 - st_a[i + half]) for i in range(half)) / half
    hw_br = sum(st_br[i] == (1 - st_br[i + half]) for i in range(half)) / half
    hw_bi = sum(st_bi[i] == (1 - st_bi[i + half]) for i in range(half)) / half
    print(f"  half-wave symmetry A={hw_a:.4f}  B_rot={hw_br:.4f}  B_ind={hw_bi:.4f}")


if __name__ == "__main__":
    for n in (100, 50, 63, 125):
        analyze(n)
