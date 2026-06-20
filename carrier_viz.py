#!/usr/bin/env python3
"""
carrier_viz.py
==============
Visualise the relationship between:
  - 1 kHz fundamental period  (one full SOPWM output cycle)
  - 50 kHz sawtooth carrier   (50 carrier cycles per fundamental)
  - 12 SOPWM switching events derived from M=3 LUT angles (N=7, m=0.50)
"""

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import matplotlib.patches as mpatches

PI = np.pi

# ── Parameters ────────────────────────────────────────────────────────────────
F_FUND   = 1_000
F_CARR   = 50_000
N_CARR   = F_CARR // F_FUND       # 50
T_FUND   = 1 / F_FUND             # 1 ms
T_CARR   = 1 / F_CARR             # 0.02 ms

N_PTS    = 100_000
t        = np.linspace(0, T_FUND, N_PTS, endpoint=False)

# ── Carrier: sawtooth 0→1 per carrier cycle ──────────────────────────────────
carrier_phase = (t % T_CARR) / T_CARR
carrier       = carrier_phase                        # sawtooth 0→1

# ── M=3 base angles from SOPWM_LUT_N7, m=0.50 (degrees → radians) ─────────────
M        = 3
N        = 2 * M + 1                       # pulse number = 7
BASE_DEG = np.array([
    66.4466338389, 76.5217501494, 85.2117684069
])
base_rad = np.deg2rad(BASE_DEG)          # α1, α2, α3  ∈ (0, π/2)

# ── Quarter-wave expansion → 4·M switching angles in [0, 2π] ─────────────────
rev = base_rad[::-1]
all_angles_rad = np.concatenate([
    base_rad,            # Q1: α1, α2, α3
    PI   - rev,          # Q2: π−α3, π−α2, π−α1
    PI   + base_rad,     # Q3: π+α1, π+α2, π+α3
    2*PI - rev,          # Q4: 2π−α3, 2π−α2, 2π−α1
])
Q_IDX = [i // M for i in range(4 * M)]               # quarter index 0-3

# ── Convert each switching angle to a time instant ───────────────────────────
t_switch = all_angles_rad / (2 * PI) * T_FUND         # seconds

# ── Carrier (sawtooth) value at each switching instant ───────────────────────
cp_sw        = (t_switch % T_CARR) / T_CARR
carrier_at_t = cp_sw                                  # sawtooth 0→1

# ── PWM generation (half-wave + quarter-wave) ─────────────────────────────────
toggles = np.zeros(N_PTS, dtype=np.uint8)
for ang in all_angles_rad:
    saw = (2 * PI / T_FUND) * t                       # monotone 0→2π
    idx = int(np.searchsorted(saw, ang))
    if 0 < idx < N_PTS:
        toggles[idx] ^= 1
idx_pi = int(np.searchsorted((2*PI/T_FUND)*t, PI))    # half-wave toggle at π
if 0 < idx_pi < N_PTS:
    toggles[idx_pi] ^= 1
pwm = np.bitwise_xor.accumulate(toggles)

# ── Colors per quarter ────────────────────────────────────────────────────────
COLORS  = ['#E63946', '#2A9D8F', '#457B9D', '#E9C46A']
Q_NAMES = ['Q1 [0,π/2]', 'Q2 [π/2,π]', 'Q3 [π,3π/2]', 'Q4 [3π/2,2π]']

t_quarters = [i * T_FUND / 4 for i in range(5)]
t_carriers = [i * T_CARR       for i in range(N_CARR + 1)]

# ── Plot ──────────────────────────────────────────────────────────────────────
fig, (ax_carr, ax_pwm) = plt.subplots(
    2, 1, figsize=(16, 8), sharex=True,
    gridspec_kw={'height_ratios': [2, 1]}
)
fig.suptitle(
    f'SOPWM  N={N}, M={M}, m=0.50  |  base angles: '
    f'{', '.join(f"{a:.3f}°" for a in BASE_DEG)}\n'
    f'Fundamental {F_FUND} Hz  ·  Sawtooth carrier {F_CARR} Hz  ·  '
    f'{4*M} switching events (quarter-wave + half-wave)',
    fontsize=11, fontweight='bold'
)

# ─── Panel 1: carrier + switching dots ───────────────────────────────────────
ax_carr.plot(t * 1e3, carrier, color='#555', lw=1.2, zorder=3,
             label=f'Sawtooth carrier ({F_CARR} Hz)', alpha=0.85)

# shade alternating carrier cycles
for i in range(N_CARR):
    x0, x1 = t_carriers[i]*1e3, t_carriers[i+1]*1e3
    ax_carr.axvspan(x0, x1, alpha=0.05,
                    color='#E63946' if i % 2 == 0 else '#457B9D')
    ax_carr.text((x0+x1)/2, 1.07, f'C{i+1}',
                 ha='center', fontsize=7.5, color='gray')

# quarter spans (background)
for qi in range(4):
    x0, x1 = t_quarters[qi]*1e3, t_quarters[qi+1]*1e3
    ax_carr.axvspan(x0, x1, alpha=0.06, color=COLORS[qi], zorder=1)

# quarter boundary lines + labels
Q_ANG_LABELS = ['0', 'π/2', 'π', '3π/2', '2π']
for tq, ql in zip(t_quarters, Q_ANG_LABELS):
    ax_carr.axvline(tq*1e3, color='#7B2D8B', lw=1.5, ls='--', alpha=0.7, zorder=4)
    ax_carr.text(tq*1e3 + 0.005, -0.17, ql,
                 ha='center', fontsize=9, color='#7B2D8B', fontweight='bold')

# carrier cycle ticks
for tc in t_carriers:
    ax_carr.axvline(tc*1e3, color='gray', lw=0.5, ls=':', alpha=0.35)

# switching dots + vertical drop lines
for k, (ts, cv, qi) in enumerate(zip(t_switch, carrier_at_t, Q_IDX)):
    col = COLORS[qi]
    # vertical dotted line from x-axis to dot
    ax_carr.axvline(ts*1e3, color=col, lw=0.8, ls=':', alpha=0.6, zorder=2)
    # dot on the carrier waveform
    ax_carr.plot(ts*1e3, cv, 'o', color=col, markersize=6,
                 markeredgecolor='white', markeredgewidth=1.0, zorder=6)

# quarter legend patches
q_patches = [mpatches.Patch(facecolor=COLORS[i], alpha=0.5, label=Q_NAMES[i])
             for i in range(4)]
ax_carr.legend(handles=[ax_carr.get_lines()[0]] + q_patches,
               fontsize=8.5, loc='upper left', framealpha=0.9)

ax_carr.set_ylabel('Carrier amplitude', fontsize=10)
ax_carr.set_ylim(-0.28, 1.22)
ax_carr.set_yticks([0, 0.5, 1.0])
ax_carr.set_title(
    f'Sawtooth carrier (50 kHz) with {4*M} switching-event dots  '
    '(colored by quarter,  purple dashed = quarter boundaries)',
    fontsize=10
)

# ─── Panel 2: PWM output ─────────────────────────────────────────────────────
ax_pwm.step(t*1e3, pwm, where='post', color='#1D3557', lw=1.5, label='PWM output')
ax_pwm.fill_between(t*1e3, 0, pwm, step='post', alpha=0.18, color='#1D3557')

for qi in range(4):
    x0, x1 = t_quarters[qi]*1e3, t_quarters[qi+1]*1e3
    ax_pwm.axvspan(x0, x1, alpha=0.07, color=COLORS[qi])

for tq in t_quarters:
    ax_pwm.axvline(tq*1e3, color='#7B2D8B', lw=1.5, ls='--', alpha=0.7)

# toggle markers on PWM
for ts, qi in zip(t_switch, Q_IDX):
    ax_pwm.axvline(ts*1e3, color=COLORS[qi], lw=0.8, ls=':', alpha=0.65)

ax_pwm.axvline((T_FUND/2)*1e3, color='#7B2D8B', lw=2.0, ls='-', alpha=0.8,
               label='π  half-wave toggle')

ax_pwm.set_ylabel('PWM state', fontsize=10)
ax_pwm.set_xlabel('Time  (ms)', fontsize=10)
ax_pwm.set_yticks([0, 1])
ax_pwm.set_yticklabels(['LOW', 'HIGH'])
ax_pwm.set_ylim(-0.25, 1.45)
ax_pwm.legend(loc='upper right', fontsize=9)
ax_pwm.set_title(
    f'Resulting PWM output  (N={N}, {4*M} toggle events per cycle, quarter-wave + half-wave symmetry)',
    fontsize=10
)

ax_carr.xaxis.set_major_locator(ticker.MultipleLocator(T_CARR * 1e3))
ax_carr.xaxis.set_major_formatter(ticker.FormatStrFormatter('%.2f'))
plt.xlim(0, T_FUND * 1e3)
plt.tight_layout()
plt.show()


# ── Parameters ────────────────────────────────────────────────────────────────
F_FUND   = 1_000          # Hz  — fundamental
F_CARR   = 10_000         # Hz  — carrier (switching frequency)
N_CARR   = F_CARR // F_FUND   # = 10 carrier cycles per fundamental period
T_FUND   = 1 / F_FUND    # s
T_CARR   = 1 / F_CARR    # s

N_PTS    = 100_000        # resolution
t        = np.linspace(0, T_FUND, N_PTS, endpoint=False)

# ── Carrier: up-down triangle, normalised 0 → 1 → 0 per carrier cycle ────────
#  phase within each carrier cycle ∈ [0, 1)
carrier_phase = (t % T_CARR) / T_CARR          # 0..1 sawtooth per cycle
carrier = 1 - np.abs(2 * carrier_phase - 1)    # triangle: 0→1→0

# ── Fundamental: simple square wave (LOW first half, HIGH second half) ────────
fundamental = (t >= T_FUND / 2).astype(float)

# ── Quarter-cycle boundaries (useful reference lines) ─────────────────────────
t_quarters = [i * T_FUND / 4 for i in range(5)]   # 0, T/4, T/2, 3T/4, T
t_carriers = [i * T_CARR       for i in range(N_CARR + 1)]  # each carrier edge

# ── Plot ──────────────────────────────────────────────────────────────────────
fig, (ax_fund, ax_carr) = plt.subplots(
    2, 1, figsize=(16, 6), sharex=True,
    gridspec_kw={'height_ratios': [1, 2]}
)
fig.suptitle(
    f'Fundamental  {F_FUND} Hz  vs  Carrier  {F_CARR} Hz  '
    f'({N_CARR} carrier cycles per fundamental period)',
    fontsize=12, fontweight='bold'
)

# ─── Top: fundamental square wave ─────────────────────────────────────────────
ax_fund.step(t * 1e3, fundamental, where='post', color='#1D3557', lw=2.0,
             label=f'Fundamental  ({F_FUND} Hz)')
ax_fund.fill_between(t * 1e3, 0, fundamental, step='post',
                     alpha=0.15, color='#1D3557')

for tq in t_quarters:
    ax_fund.axvline(tq * 1e3, color='gray', lw=0.8, ls='--', alpha=0.5)

ax_fund.set_ylabel('State', fontsize=10)
ax_fund.set_yticks([0, 1])
ax_fund.set_yticklabels(['LOW', 'HIGH'])
ax_fund.set_ylim(-0.3, 1.5)
ax_fund.legend(loc='upper right', fontsize=9)
ax_fund.set_title('Fundamental period  (1 ms)', fontsize=10)

# ─── Bottom: carrier triangles ────────────────────────────────────────────────
ax_carr.plot(t * 1e3, carrier, color='#E63946', lw=1.5,
             label=f'Up-down carrier  ({F_CARR} Hz)')

# shade alternating carrier cycles for clarity
for i in range(N_CARR):
    x0 = t_carriers[i]   * 1e3
    x1 = t_carriers[i+1] * 1e3
    col = '#E63946' if i % 2 == 0 else '#457B9D'
    ax_carr.axvspan(x0, x1, alpha=0.07, color=col)
    # cycle number label
    ax_carr.text((x0 + x1) / 2, 1.06, f'C{i+1}',
                 ha='center', fontsize=8, color=col, fontweight='bold')

# quarter-cycle boundaries on carrier plot
Q_LABELS = ['0', 'π/2', 'π', '3π/2', '2π']
for tq, ql in zip(t_quarters, Q_LABELS):
    ax_carr.axvline(tq * 1e3, color='#7B2D8B', lw=1.4, ls='--', alpha=0.7)
    ax_carr.text(tq * 1e3 + 0.003, -0.14, ql,
                 ha='center', fontsize=9, color='#7B2D8B', fontweight='bold')

# carrier boundary ticks
for tc in t_carriers:
    ax_carr.axvline(tc * 1e3, color='gray', lw=0.6, ls=':', alpha=0.4)

ax_carr.set_ylabel('Carrier amplitude', fontsize=10)
ax_carr.set_xlabel('Time  (ms)', fontsize=10)
ax_carr.set_ylim(-0.25, 1.20)
ax_carr.set_yticks([0, 0.5, 1.0])
ax_carr.legend(loc='upper right', fontsize=9)
ax_carr.set_title(
    f'{N_CARR} carrier cycles  (each {T_CARR*1e6:.0f} µs) — '
    f'2 consecutive cycles = 1 SOPWM switching interval',
    fontsize=10
)

# x-axis: label every carrier period
ax_carr.xaxis.set_major_locator(ticker.MultipleLocator(T_CARR * 1e3))
ax_carr.xaxis.set_major_formatter(ticker.FormatStrFormatter('%.2f'))

plt.xlim(0, T_FUND * 1e3)
plt.tight_layout()
plt.show()
