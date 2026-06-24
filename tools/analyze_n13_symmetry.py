#!/usr/bin/env python3
"""Analyze N=13 SOPWM symmetry vs firmware binning logic."""

import re
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parent.parent
SOPWM_C = ROOT / "pwm" / "sopwm" / "sopwm.c"

N_CARR = 50
SPAN = 360.0 / N_CARR
TBPRD = 2000
OFF = 0xFFFF


def parse_lut(name, cols):
    text = SOPWM_C.read_text(encoding="utf-8")
    m = re.search(rf"{name}\[.*?\]\s*=\s*\{{(.*?)\}};", text, re.DOTALL)
    rows = []
    for rm in re.finditer(r"\{([^}]+)\}", m.group(1)):
        vals = [float(v.replace("f", "")) for v in rm.group(1).split(",")]
        if len(vals) == cols:
            rows.append(vals)
    return np.array(rows)


def expand(base):
    b = np.array(base, dtype=float)
    rev = b[::-1]
    return np.concatenate([b, 180.0 - rev, 180.0 + b, 360.0 - rev])


def firmware_build(base, phase_offset=0.0, tbprd=TBPRD):
    angs = expand(base)
    if phase_offset:
        angs = np.where(angs + phase_offset >= 360.0, angs + phase_offset - 360.0, angs + phase_offset)

    tbl = [{"cmpa": OFF, "cmpb": OFF} for _ in range(N_CARR)]
    dropped = []

    for a in angs:
        slot = int(a / SPAN)
        if slot >= N_CARR:
            slot = N_CARR - 1
        local = a - slot * SPAN
        counts = int(round((local / SPAN) * tbprd))
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
            dropped.append((float(a), slot, counts))

    mid = phase_offset + 180.0
    if mid >= 360.0:
        mid -= 360.0
    mid_slot = int(mid / SPAN)
    if mid_slot >= N_CARR:
        mid_slot = N_CARR - 1
    mid_counts = 0  # exact 180 deg at slot leading edge
    s = tbl[mid_slot]
    if s["cmpa"] == OFF:
        s["cmpa"] = mid_counts
    elif s["cmpb"] == OFF:
        s["cmpb"] = mid_counts
    else:
        dropped.append((mid, "mid", mid_counts))

    return tbl, dropped


def pwm_from_tbl(tbl, tbprd=TBPRD):
    n = 36000
    toggles = np.zeros(n, dtype=np.uint8)
    for cy in range(N_CARR):
        base_deg = cy * SPAN
        for key in ("cmpa", "cmpb"):
            c = tbl[cy][key]
            if c == OFF:
                continue
            ang = base_deg + (c / tbprd) * SPAN
            idx = int(ang / 360.0 * n)
            if 0 <= idx < n:
                toggles[idx] ^= 1
    return np.bitwise_xor.accumulate(toggles)


def halfwave_ok(state):
    half = len(state) // 2
    return float(np.mean(state[:half] == (1 - state[half:])))


def report(name, cols, m_idx):
    base = parse_lut(name, cols)[m_idx]
    tbl, dropped = firmware_build(base, 0.0)
    state = pwm_from_tbl(tbl)
    m = (m_idx + 1) * 0.01
    print(f"=== {name}  m={m:.2f}  M={cols} ===")
    print(f"  base Q1 angles: {base}")
    print(f"  dropped (>2/slot): {len(dropped)}")
    if dropped:
        print(f"    first drops: {dropped[:6]}")
    print(f"  half-wave match: {halfwave_ok(state):.4f}")
    print(f"  slot 12 (86-93 deg): {tbl[12]}")
    print(f"  slot 25 (180 deg):   {tbl[25]}")
    print()


def scan_drops(name, cols):
    lut = parse_lut(name, cols)
    bad = []
    for i, base in enumerate(lut):
        _, dropped = firmware_build(base, 0.0)
        if dropped:
            bad.append(((i + 1) * 0.01, len(dropped)))
    print(f"{name}: {len(bad)}/100 m-rows drop angles")
    if bad:
        print("  worst:", sorted(bad, key=lambda x: -x[1])[:10])


if __name__ == "__main__":
    for mi in (49, 96, 99):
        report("SOPWM_LUT_N7", 3, mi)
    for mi in (49, 96, 99):
        report("SOPWM_LUT_N13", 6, mi)
    print("--- drop scan ---")
    scan_drops("SOPWM_LUT_N7", 3)
    scan_drops("SOPWM_LUT_N13", 6)
    scan_drops("SOPWM_LUT_N11", 5)
