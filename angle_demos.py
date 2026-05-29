#!/usr/bin/env python3
"""
angle_demos.py
==============
SOPWM switching-angle demonstration.

Three base angles (α1 < α2 < α3, all < π/2) are chosen randomly.
From them, 12 switching angles spanning [0, 2π] are derived using
quarter-wave symmetry:

  Q1  α1, α2, α3               ∈ [0,    π/2]   ─→  x = [0.0,  2.5]
  Q2  α4=π-α3, α5=π-α2, α6=π-α1  ∈ [π/2,  π ]   ─→  x = [2.5,  5.0]
  Q3  α7=π+α1, α8=π+α2, α9=π+α3  ∈ [π,  3π/2]   ─→  x = [5.0,  7.5]
  Q4  α10=2π-α3, α11=2π-α2, α12=2π-α1 ∈ [3π/2, 2π] ─→ x = [7.5, 10.0]

A sawtooth carrier rises linearly from 0 → 2π across x = 0 → 10.
The PWM starts LOW and uses BOTH symmetries:
  • Quarter-wave: toggles at each of the 12 switching angles
  • Half-wave   : mandatory extra toggle at π (x = 5.0) so the
                  second half [π, 2π] is the bitwise complement of
                  the first half [0, π]
"""

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches

# ── 1.  Three random base angles in (0.05, π/2 − 0.05) ───────────────────────
#rng = np.random.default_rng()          # pass an int seed for repeatability
#alpha1, alpha2, alpha3 = np.sort(rng.uniform(0.05, np.pi / 2 - 0.05, 3))

alpha1 = 68.8837313449/180.0 * np.pi
alpha2 = 75.9211716794/180.0 * np.pi
alpha3 = 86.6752706499/180.0 * np.pi

print("─" * 55)
print("Input angles (random, all < π/2):")
print(f"  α1 = {alpha1:.6f} rad  ({np.degrees(alpha1):.4f}°)")
print(f"  α2 = {alpha2:.6f} rad  ({np.degrees(alpha2):.4f}°)")
print(f"  α3 = {alpha3:.6f} rad  ({np.degrees(alpha3):.4f}°)")
print("─" * 55)

# ── 2.  Derive the 12 SOPWM switching angles ──────────────────────────────────
PI = np.pi

alphas = np.array([
    # Q1 — first quarter  [0, π/2]
    alpha1,
    alpha2,
    alpha3,
    # Q2 — second quarter [π/2, π]
    PI - alpha3,
    PI - alpha2,
    PI - alpha1,
    # Q3 — third quarter  [π, 3π/2]
    PI + alpha1,
    PI + alpha2,
    PI + alpha3,
    # Q4 — fourth quarter [3π/2, 2π]
    2*PI - alpha3,
    2*PI - alpha2,
    2*PI - alpha1,
])
labels = [f'α{i+1}' for i in range(12)]

# ── 3.  Sawtooth carrier: x ∈ [0, 10] → value ∈ [0, 2π) ─────────────────────
N   = 100_000
x   = np.linspace(0, 10, N, endpoint=False)
saw = (2 * PI / 10) * x

# ── 4.  PWM generation — quarter-wave + half-wave symmetry ─────────────────
# Start LOW (np.zeros default). Toggle at each switching angle (quarter-wave)
# and once more at π (half-wave boundary), making the second half the
# bitwise complement of the first half.
toggles = np.zeros(N, dtype=np.uint8)

for ang in alphas:
    idx = int(np.searchsorted(saw, ang))
    if 0 < idx < N:
        toggles[idx] ^= 1

# Half-wave symmetry: force toggle at π (x = 5.0)
idx_pi = int(np.searchsorted(saw, PI))
if 0 < idx_pi < N:
    toggles[idx_pi] ^= 1

pwm = np.bitwise_xor.accumulate(toggles)   # running parity = PWM state

# ── 5.  Plot ──────────────────────────────────────────────────────────────────
COLORS  = ['#E63946', '#2A9D8F', '#457B9D', '#E9C46A']
Q_SPANS = [(0.0, 2.5), (2.5, 5.0), (5.0, 7.5), (7.5, 10.0)]
Q_NAMES = ['Q1  [0, π/2]', 'Q2  [π/2, π]', 'Q3  [π, 3π/2]', 'Q4  [3π/2, 2π]']

fig, (ax1, ax2) = plt.subplots(
    2, 1, figsize=(16, 9), sharex=True,
    gridspec_kw={'height_ratios': [2, 1]}
)
fig.suptitle(
    f'SOPWM  (quarter-wave + half-wave symmetry)  ─  '
    f'α1={np.degrees(alpha1):.2f}°,  α2={np.degrees(alpha2):.2f}°,  α3={np.degrees(alpha3):.2f}°',
    fontsize=12, fontweight='bold'
)

# ─── Subplot 1: sawtooth + threshold lines ────────────────────────────────────
ax1.plot(x, saw, color='#1D3557', lw=1.6, zorder=3, label='Sawtooth carrier')

for qi, ((x0, x1), col, qname) in enumerate(zip(Q_SPANS, COLORS, Q_NAMES)):
    ax1.axvspan(x0, x1, alpha=0.10, color=col, zorder=1)

    for j in range(3):
        ang = alphas[qi * 3 + j]
        lbl = labels[qi * 3 + j]
        xc  = ang * 10 / (2 * PI)

        # Horizontal dashed threshold line (only within its quarter span)
        ax1.hlines(ang, x0, x1, colors=col, lw=1.8, linestyles='--', zorder=4)

        # Vertical dotted crossing marker
        ax1.axvline(xc, color=col, lw=0.9, ls=':', alpha=0.75, zorder=2)

        # Filled dot at the exact intersection point
        ax1.plot(xc, ang, 'o', color=col, markersize=4, markeredgecolor='white',
                 markeredgewidth=0.8, zorder=6)

    # Quarter band label at the very top
    ax1.text((x0 + x1) / 2, 2*PI + 0.38, qname,
             ha='center', fontsize=9.5, color=col, fontweight='bold')

# Quarter dividers (gray dashed) + half-wave boundary at π (solid purple)
for xv in (2.5, 7.5):
    ax1.axvline(xv, color='gray', lw=0.9, ls='--', alpha=0.45, zorder=2)
ax1.axvline(5.0, color='#7B2D8B', lw=2.0, ls='-', alpha=0.75, zorder=5,
            label='π  (half-wave toggle)')
ax1.axhline(PI, color='#7B2D8B', lw=1.4, ls='-.', alpha=0.55, zorder=4)

ax1.set_ylabel('Carrier / angle value (rad)', fontsize=10)
ax1.set_yticks([0, PI/2, PI, 3*PI/2, 2*PI])
ax1.set_yticklabels(['0', 'π/2', 'π', '3π/2', '2π'])
ax1.set_ylim(-0.15, 2*PI + 0.95)

patches = [mpatches.Patch(facecolor=COLORS[i], alpha=0.55, label=Q_NAMES[i])
           for i in range(4)]
purple_line = mpatches.Patch(facecolor='#7B2D8B', alpha=0.7, label='π  half-wave toggle')
ax1.legend(
    handles=[ax1.get_lines()[0]] + patches + [purple_line],
    fontsize=8.5, loc='upper left', framealpha=0.9
)
ax1.set_title(
    'Sawtooth carrier vs. 12 SOPWM switching angles  '
    '(-- threshold level · ● dot = carrier crossing · purple = π half-wave boundary)',
    fontsize=10
)

# ─── Subplot 2: PWM output ────────────────────────────────────────────────────
ax2.step(x, pwm, where='post', color='#1D3557', lw=1.5, label='PWM output')
ax2.fill_between(x, 0, pwm, step='post', alpha=0.22, color='#1D3557')

for qi, ((x0, x1), col) in enumerate(zip(Q_SPANS, COLORS)):
    ax2.axvspan(x0, x1, alpha=0.10, color=col)
    ax2.axvline(x0, color='gray', lw=0.8, ls='--', alpha=0.40)
    for j in range(3):
        ang = alphas[qi * 3 + j]
        xc  = ang * 10 / (2 * PI)
        ax2.axvline(xc, color=col, lw=1.0, ls=':', alpha=0.80)

# Half-wave boundary marker
ax2.axvline(5.0, color='#7B2D8B', lw=2.0, ls='-', alpha=0.85, label='π  (half-wave toggle)')

ax2.set_ylabel('PWM state', fontsize=10)
ax2.set_xlabel('x  (one fundamental period  0 → 10)', fontsize=10)
ax2.set_yticks([0, 1])
ax2.set_yticklabels(['LOW', 'HIGH'])
ax2.set_ylim(-0.20, 1.45)
ax2.set_title(
    'PWM output — 12 switching-angle toggles (quarter-wave) + 1 half-wave toggle at π',
    fontsize=10
)
ax2.legend(loc='upper right', fontsize=9)

plt.xlim(0, 10)
plt.tight_layout()
plt.show()

# ── 6.  Console summary ───────────────────────────────────────────────────────
print("\n12 Switching Angles:")
print(f"{'Name':5}  {'rad':>10}  {'degrees':>10}  {'Quarter':8}  x-crossing")
print("─" * 58)
for i, (ang, lbl) in enumerate(zip(alphas, labels)):
    qi  = i // 3 + 1
    xc  = ang * 10 / (2 * PI)
    print(f"{lbl:5}  {ang:10.6f}  {np.degrees(ang):10.4f}°   Q{qi}       x = {xc:.4f}")
print("─" * 58)
