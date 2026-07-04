#!/usr/bin/env python3
"""Analyze phase-A half-wave symmetry vs firmware build."""

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SOPWM_C = ROOT / "pwm" / "sopwm" / "sopwm.c"
OFF = 0xFFFF
TBPRD = 2000


def parse_lut(n):
    name = f"SOPWM_LUT_N{n}"
    text = SOPWM_C.read_text(encoding="utf-8")
    m = re.search(rf"{name}\[.*?\]\s*=\s*\{{(.*?)\}};", text, re.DOTALL)
    rows = []
    cols = {7: 3, 9: 4, 11: 5, 13: 6, 15: 7}[n]
    for rm in re.finditer(r"\{([^}]+)\}", m.group(1)):
        vals = [float(v.replace("f", "")) for v in rm.group(1).split(",")]
        if len(vals) == cols:
            rows.append(vals)
    return rows


def expand(base):
    b = list(base)
    rev = b[::-1]
    return b + [180.0 - x for x in rev] + [180.0 + x for x in b] + [360.0 - x for x in rev]


def bin_lut(angs, n_carr, span, tbprd):
    tbl = [{"cmpa": OFF, "cmpb": OFF} for _ in range(n_carr)]
    dropped = []
    per_angle = []
    for a in angs:
        slot = int(a / span)
        if slot >= n_carr:
            slot = n_carr - 1
        local = a - slot * span
        counts = int(round((local / span) * tbprd))
        if counts >= tbprd:
            counts = tbprd - 1
        s = tbl[slot]
        kept = True
        if s["cmpa"] == OFF:
            s["cmpa"] = counts
        elif s["cmpb"] == OFF:
            if counts < s["cmpa"]:
                s["cmpa"], s["cmpb"] = counts, s["cmpa"]
            else:
                s["cmpb"] = counts
        else:
            kept = False
            dropped.append((a, slot, counts))
        per_angle.append((a, slot, counts, kept))
    return tbl, dropped, per_angle


def frame_closure(tbl, mid_slot, n_carr, tbprd):
    tbl = [dict(x) for x in tbl]
    if mid_slot >= n_carr:
        mid_slot = n_carr - 1
    s = tbl[mid_slot]
    mid_act = None
    if s["cmpa"] == OFF:
        s["cmpa"] = 0
        mid_act = "cmpa=0"
    elif s["cmpb"] == OFF:
        s["cmpb"] = 0
        mid_act = "cmpb=0"
    else:
        mid_act = "full"

    nt = sum((tbl[i]["cmpa"] != OFF) + (tbl[i]["cmpb"] != OFF) for i in range(n_carr))
    odd_fix = None
    if nt & 1:
        if tbl[0]["cmpa"] == OFF:
            tbl[0]["cmpa"] = 0
            odd_fix = "slot0 cmpa=0"
        elif tbl[0]["cmpa"] != 0 and tbl[0]["cmpb"] == OFF:
            tbl[0]["cmpb"] = tbl[0]["cmpa"]
            tbl[0]["cmpa"] = 0
            odd_fix = "slot0 reorder"
        elif tbl[n_carr - 1]["cmpb"] == OFF:
            tbl[n_carr - 1]["cmpb"] = tbprd - 1
            odd_fix = f"slot{n_carr-1} cmpb={tbprd-1}"
    return tbl, nt, mid_act, odd_fix


def pwm_state(tbl, n_carr, span, tbprd=TBPRD):
    steps = n_carr * tbprd
    toggles = [0] * steps
    for cy in range(n_carr):
        base_deg = cy * span
        for key in ("cmpa", "cmpb"):
            c = tbl[cy][key]
            if c == OFF:
                continue
            ang = base_deg + (c / tbprd) * span
            idx = int(round(ang / 360.0 * steps)) % steps
            toggles[idx] ^= 1
    state = []
    cur = 0
    for t in toggles:
        cur ^= t
        state.append(cur)
    return state


def halfwave_metric(state):
    half = len(state) // 2
    inv = sum(state[i] == (1 - state[i + half]) for i in range(half))
    same = half - inv
    return inv / half, same / half


def slot_mirror_symmetry(tbl, n):
    """Check if slot i mirrors slot (n-1-i) for compare pairs."""
    mism = []
    for i in range(n // 2):
        j = n - 1 - i
        if tbl[i] != tbl[j]:
            mism.append((i, tbl[i], j, tbl[j]))
    return mism


def analyze(n_carr, n_pulse, m_idx):
    span = 360.0 / n_carr
    mid = n_carr // 2
    base = parse_lut(n_pulse)[m_idx]
    angs = expand(base)
    raw, dropped, per_angle = bin_lut(angs, n_carr, span, TBPRD)
    closed, nt, mid_act, odd_fix = frame_closure(raw, mid, n_carr, TBPRD)
    state = pwm_state(closed, n_carr, span)
    hw_inv, hw_same = halfwave_metric(state)
    mirror = slot_mirror_symmetry(closed, n_carr)

    print(
        f"n={n_carr:3d} N={n_pulse:2d} m={(m_idx+1)/100:.2f}  "
        f"dropped={len(dropped):2d} toggles={nt:2d} mid={mid}({mid_act}) odd_fix={odd_fix}"
    )
    print(f"  half-wave invert match={hw_inv:.4f}  same={hw_same:.4f}  slot-mirror mism={len(mirror)}")
    if dropped:
        print(f"  first drops: {dropped[:4]}")
    if mirror[:3]:
        for row in mirror[:3]:
            print(f"    slot {row[0]} {row[1]} vs slot {row[2]} {row[3]}")
    # first/second half slot toggle counts
    h = n_carr // 2
    t1 = sum((closed[i]['cmpa']!=OFF)+(closed[i]['cmpb']!=OFF) for i in range(h))
    t2 = sum((closed[i]['cmpa']!=OFF)+(closed[i]['cmpb']!=OFF) for i in range(h, n_carr))
    print(f"  toggles 1st half={t1} 2nd half={t2}  slot0={closed[0]} mid={closed[mid]} last={closed[-1]}")
    return hw_inv, len(dropped), len(mirror)


if __name__ == "__main__":
    print("=== N=15 m=0.50 across n_carr ===")
    for n in (50, 63, 80, 100, 125):
        analyze(n, 15, 49)

    print("\n=== worst N=15 m scan at n=100 ===")
    worst = []
    lut = parse_lut(15)
    for mi in range(100):
        span = 3.6
        mid = 50
        angs = expand(lut[mi])
        raw, dropped, _ = bin_lut(angs, 100, span, TBPRD)
        closed, _, _, _ = frame_closure(raw, mid, 100, TBPRD)
        hw, _, mir = halfwave_metric(pwm_state(closed, 100, span)), len(dropped), len(slot_mirror_symmetry(closed, 100))
        worst.append(((mi + 1) / 100, hw[0], dropped, mir))
    worst.sort(key=lambda x: x[1])
    print("  lowest half-wave:", worst[:5])
    bad = [w for w in worst if w[1] < 0.99 or w[2] > 0]
    print(f"  m rows with hw<0.99 or drops: {len(bad)}")

    print("\n=== TBPRD mismatch test (period reg=1999, scale=2000) ===")
    # already using tbprd=2000 clamp 1999 - ok
