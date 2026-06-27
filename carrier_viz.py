#!/usr/bin/env python3
"""
carrier_viz.py
==============
Visualise SOPWM using the same slot-based schedule logic as pwm/sopwm/sopwm.c:
  - bin LUT angles into carrier slots
  - half-wave symmetry toggle at 180°
  - period closure at 360°=0° when toggle count is odd

Shows two consecutive fundamental periods to confirm frame-to-frame repeat.
"""

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import matplotlib.patches as mpatches

PI = np.pi
CMP_OFF = 0xFFFF

# ── Parameters (match firmware defaults) ─────────────────────────────────────
F_FUND = 1_000
F_CARR = 50_000
N_CARR = F_CARR // F_FUND          # 50 slots → 1 kHz fundamental
TBPRD  = 2000                      # SOPWM_TBPRD in main.c
T_FUND = 1 / F_FUND
T_CARR = 1 / F_CARR

N_PULSE = 7                        # sopwm_N_cmd
M_MOD   = 0.50
N_BASE  = 3                        # N_PULSE // 2 rounded up for N=7

# SOPWM_LUT_N7[m=0.50] from sopwm.c
BASE_DEG = np.array([66.4466338389, 76.5217501494, 85.2117684069])

N_PERIODS = 2                      # plot two frames to check repeat
N_PTS     = 200_000


def expand_quarter_wave(base_deg):
    """Mirror firmware SOPWM_BuildSchedule quarter-wave expansion."""
    rev = base_deg[::-1]
    return np.concatenate([
        base_deg,                  # Q1
        180.0 - rev,               # Q2
        180.0 + base_deg,          # Q3
        360.0 - rev,               # Q4
    ])


def build_phase_schedule(global_deg, n_carr, tbprd, phase_offset_deg=0.0):
    """
    Mirror sopwm_build_phase(): slot table with cmpa/cmpb compare counts.
    Returns list of dicts with keys cmpa, cmpb (CMP_OFF when inactive).
    """
    span = 360.0 / n_carr
    tbl = [{'cmpa': CMP_OFF, 'cmpb': CMP_OFF} for _ in range(n_carr)]

    for ang in global_deg:
        slot = int(ang / span)
        if slot >= n_carr:
            slot = n_carr - 1
        local = ang - slot * span
        counts = int(local / span * tbprd + 0.5)
        if counts >= tbprd:
            counts = tbprd - 1

        s = tbl[slot]
        if s['cmpa'] == CMP_OFF:
            s['cmpa'] = counts
        elif s['cmpb'] == CMP_OFF:
            if counts < s['cmpa']:
                s['cmpb'], s['cmpa'] = s['cmpa'], counts
            else:
                s['cmpb'] = counts

    mid_deg = phase_offset_deg + 180.0
    if mid_deg >= 360.0:
        mid_deg -= 360.0
    mid_slot = int(mid_deg / span)
    if mid_slot >= n_carr:
        mid_slot = n_carr - 1
    s = tbl[mid_slot]
    if s['cmpa'] == CMP_OFF:
        s['cmpa'] = 0
    elif s['cmpb'] == CMP_OFF:
        s['cmpb'] = 0

    n_toggles = sum(
        1 for s in tbl for k in ('cmpa', 'cmpb') if s[k] != CMP_OFF
    )
    closure_added = (n_toggles & 1) != 0
    if closure_added:
        s0 = tbl[0]
        if s0['cmpa'] == CMP_OFF:
            s0['cmpa'] = 0
        elif s0['cmpa'] != 0 and s0['cmpb'] == CMP_OFF:
            s0['cmpb'], s0['cmpa'] = s0['cmpa'], 0
        elif tbl[n_carr - 1]['cmpb'] == CMP_OFF:
            tbl[n_carr - 1]['cmpb'] = tbprd - 1

    return tbl, closure_added


def schedule_toggle_times(tbl, n_carr, tbprd, t_fund, period_idx=0):
    """Convert slot CMP table to absolute toggle times (seconds)."""
    t_carr = t_fund / n_carr
    t0 = period_idx * t_fund
    times = []
    for slot, entry in enumerate(tbl):
        base_t = t0 + slot * t_carr
        for key in ('cmpa', 'cmpb'):
            c = entry[key]
            if c == CMP_OFF:
                continue
            times.append(base_t + (c / tbprd) * t_carr)

    return np.array(times)


def pwm_from_schedule(tbl, t, n_carr, tbprd, t_fund):
    """
    Toggle-based PWM from slot schedule.
    Counts toggles within each fundamental period (mod 2) so consecutive
    frames match when period closure makes the per-frame toggle count even.
    """
    period_times = schedule_toggle_times(tbl, n_carr, tbprd, t_fund, period_idx=0)
    pwm = np.zeros(len(t), dtype=np.uint8)

    for i, ti in enumerate(t):
        period_idx = int(ti // t_fund)
        tp = ti - period_idx * t_fund
        n = period_idx * len(period_times)
        for tt in period_times:
            if tt <= tp + 1e-12:
                n += 1
        pwm[i] = n & 1

    return pwm, None


def count_toggles(tbl):
    return sum(1 for s in tbl for k in ('cmpa', 'cmpb') if s[k] != CMP_OFF)


# ── Build schedule (phase A) ──────────────────────────────────────────────────
all_deg = expand_quarter_wave(BASE_DEG)
tbl, closure_added = build_phase_schedule(all_deg, N_CARR, TBPRD, phase_offset_deg=0.0)
n_toggles = count_toggles(tbl)

# ── Time base: two fundamental periods ────────────────────────────────────────
t = np.linspace(0, N_PERIODS * T_FUND, N_PTS, endpoint=False)
carrier_phase = (t % T_CARR) / T_CARR
carrier = carrier_phase

pwm, _ = pwm_from_schedule(tbl, t, N_CARR, TBPRD, T_FUND)

half = N_PTS // 2
period_match = np.all(pwm[:half] == pwm[half:2 * half])

# ── Plot helpers ──────────────────────────────────────────────────────────────
COLORS = ['#E63946', '#2A9D8F', '#457B9D', '#E9C46A']
Q_NAMES = ['Q1 [0,π/2]', 'Q2 [π/2,π]', 'Q3 [π,3π/2]', 'Q4 [3π/2,2π]']
Q_IDX = [i // N_BASE for i in range(4 * N_BASE)]

t_quarters = [i * T_FUND / 4 for i in range(5)]
t_carriers = [i * T_CARR for i in range(N_CARR + 1)]
toggle_times = schedule_toggle_times(tbl, N_CARR, TBPRD, T_FUND, period_idx=0)
all_angles_rad = np.deg2rad(all_deg)
t_switch = all_angles_rad / (2 * PI) * T_FUND
cp_sw = (t_switch % T_CARR) / T_CARR

fig, (ax_carr, ax_pwm) = plt.subplots(
    2, 1, figsize=(16, 9), sharex=True,
    gridspec_kw={'height_ratios': [2, 1]}
)
fig.suptitle(
    f'SOPWM  N={N_PULSE}, m={M_MOD:.2f}  |  '
    f'{n_toggles} toggles/frame'
    f'{" (+360° closure)" if closure_added else ""}  |  '
    f'2-period match: {period_match}\n'
    f'Fundamental {F_FUND} Hz  ·  Carrier {F_CARR} Hz  ·  '
    f'{N_CARR} slots  ·  TBPRD={TBPRD}',
    fontsize=11, fontweight='bold'
)

# ─── Panel 1: carrier + switching dots ───────────────────────────────────────
ax_carr.plot(t * 1e3, carrier, color='#555', lw=1.2, zorder=3,
             label=f'Sawtooth carrier ({F_CARR} Hz)', alpha=0.85)

for i in range(N_CARR):
    x0, x1 = t_carriers[i] * 1e3, t_carriers[i + 1] * 1e3
    ax_carr.axvspan(x0, x1, alpha=0.05,
                    color='#E63946' if i % 2 == 0 else '#457B9D')
    if i < N_CARR:
        ax_carr.text((x0 + x1) / 2, 1.07, f'S{i}',
                     ha='center', fontsize=6.5, color='gray')

for qi in range(4):
    x0, x1 = t_quarters[qi] * 1e3, t_quarters[qi + 1] * 1e3
    ax_carr.axvspan(x0, x1, alpha=0.06, color=COLORS[qi], zorder=1)

Q_ANG_LABELS = ['0', 'π/2', 'π', '3π/2', '2π']
for tq, ql in zip(t_quarters, Q_ANG_LABELS):
    ax_carr.axvline(tq * 1e3, color='#7B2D8B', lw=1.5, ls='--', alpha=0.7, zorder=4)
    ax_carr.text(tq * 1e3 + 0.005, -0.17, ql,
                 ha='center', fontsize=9, color='#7B2D8B', fontweight='bold')

for tc in t_carriers:
    ax_carr.axvline(tc * 1e3, color='gray', lw=0.5, ls=':', alpha=0.35)

ax_carr.axvline(T_FUND * 1e3, color='black', lw=1.2, ls='-', alpha=0.5,
                label='frame boundary (360°=0°)')

for ts, cv, qi in zip(t_switch, cp_sw, Q_IDX):
    col = COLORS[qi]
    ax_carr.axvline(ts * 1e3, color=col, lw=0.8, ls=':', alpha=0.6, zorder=2)
    ax_carr.plot(ts * 1e3, cv, 'o', color=col, markersize=5,
                 markeredgecolor='white', markeredgewidth=0.8, zorder=6)

q_patches = [mpatches.Patch(facecolor=COLORS[i], alpha=0.5, label=Q_NAMES[i])
             for i in range(4)]
ax_carr.legend(handles=[ax_carr.get_lines()[0]] + q_patches,
               fontsize=8.5, loc='upper left', framealpha=0.9)
ax_carr.set_ylabel('Carrier amplitude', fontsize=10)
ax_carr.set_ylim(-0.28, 1.22)
ax_carr.set_yticks([0, 0.5, 1.0])
ax_carr.set_title(
    'Slot-mapped schedule (same algorithm as sopwm_build_phase)',
    fontsize=10
)

# ─── Panel 2: PWM — two fundamental periods ──────────────────────────────────
ax_pwm.step(t * 1e3, pwm, where='post', color='#1D3557', lw=1.2, label='PWM output')
ax_pwm.fill_between(t * 1e3, 0, pwm, step='post', alpha=0.15, color='#1D3557')

ax_pwm.axvline(T_FUND * 1e3, color='black', lw=1.2, ls='-', alpha=0.6,
                label='360°=0° closure (slot 0)')
ax_pwm.axvline(T_FUND / 2 * 1e3, color='#7B2D8B', lw=2.0, ls='-',
                alpha=0.8, label='180° half-wave toggle')

for ts, qi in zip(t_switch, Q_IDX):
    ax_pwm.axvline(ts * 1e3, color=COLORS[qi], lw=0.8, ls=':', alpha=0.5)

ax_pwm.set_ylabel('PWM state', fontsize=10)
ax_pwm.set_xlabel('Time  (ms)', fontsize=10)
ax_pwm.set_yticks([0, 1])
ax_pwm.set_yticklabels(['LOW', 'HIGH'])
ax_pwm.set_ylim(-0.25, 1.45)
ax_pwm.legend(loc='upper right', fontsize=9)
ax_pwm.set_title(
    f'Two consecutive {F_FUND} Hz frames — '
    f'{"identical" if period_match else "MISMATCH (check closure)"}',
    fontsize=10
)

ax_carr.xaxis.set_major_locator(ticker.MultipleLocator(T_CARR * 1e3))
ax_carr.xaxis.set_major_formatter(ticker.FormatStrFormatter('%.2f'))
plt.xlim(0, N_PERIODS * T_FUND * 1e3)
plt.tight_layout()
plt.show()
