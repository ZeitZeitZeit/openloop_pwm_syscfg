#!/usr/bin/env python3
"""
sopwm_demo.py — SOPWM quarter-wave + half-wave symmetry visualiser
====================================================================
User provides:
  • Number of base angles N  (3, 5, or 7)
  • N angles in radians, all < π/2, entered one per line

From the N base angles the full set of 4·N switching angles is built by
applying quarter-wave symmetry:

  Q1  α₁ … αN              ∈ [0,   π/2]
  Q2  π−αN … π−α₁          ∈ [π/2, π  ]
  Q3  π+α₁ … π+αN          ∈ [π,   3π/2]
  Q4  2π−αN … 2π−α₁        ∈ [3π/2, 2π]

AND half-wave symmetry:
  • The PWM starts LOW.
  • An extra toggle is inserted at π (x = 5.0), forcing the second
    half [π, 2π] to be the bitwise complement of the first half [0, π].

A sawtooth carrier rises from 0 → 2π over x = [0, 10].
"""

import sys
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches

PI = np.pi
VALID_N  = {3, 5, 7}
COLORS   = ['#E63946', '#2A9D8F', '#457B9D', '#E9C46A']
Q_NAMES  = ['Q1  [0, π/2]', 'Q2  [π/2, π]', 'Q3  [π, 3π/2]', 'Q4  [3π/2, 2π]']
Q_SPANS  = [(0.0, 2.5), (2.5, 5.0), (5.0, 7.5), (7.5, 10.0)]
X_TOTAL  = 10.0
N_POINTS = 100_000


# ─────────────────────────────────────────────────────────────────────────────
# Input helpers
# ─────────────────────────────────────────────────────────────────────────────

def ask_n() -> int:
    """Ask the user for the number of base angles (3, 5, or 7)."""
    while True:
        raw = input(f"Number of base angles {sorted(VALID_N)}: ").strip()
        if raw.isdigit() and int(raw) in VALID_N:
            return int(raw)
        print(f"  ✗  Please enter one of {sorted(VALID_N)}.")


def ask_angles(n: int) -> np.ndarray:
    """
    Ask for n angles in radians, enforce 0 < αᵢ < π/2 and strict increase.
    Returns a sorted numpy array.
    """
    print(f"Enter {n} angles in radians (each on its own line, 0 < α < π/2 ≈ {PI/2:.4f}):")
    angles = []
    while len(angles) < n:
        k = len(angles) + 1
        raw = input(f"  α{k}: ").strip()
        try:
            val = float(raw)
        except ValueError:
            print("  ✗  Not a number — try again.")
            continue
        if not (0 < val < PI / 2):
            print(f"  ✗  Must satisfy 0 < α < π/2 ({PI/2:.4f}) — try again.")
            continue
        if angles and val <= angles[-1]:
            print(f"  ✗  Must be > previous value ({angles[-1]:.6f}) — try again.")
            continue
        angles.append(val)
    return np.array(angles)


# ─────────────────────────────────────────────────────────────────────────────
# SOPWM core
# ─────────────────────────────────────────────────────────────────────────────

def quarter_wave_expand(base: np.ndarray) -> np.ndarray:
    """
    Given N sorted base angles in (0, π/2), return the full 4·N switching
    angles in [0, 2π] produced by quarter-wave symmetry.
    A half-wave toggle at π is handled separately in generate_pwm().
    """
    rev  = base[::-1]
    q1   = base          # α₁ … αN
    q2   = PI   - rev    # π−αN … π−α₁
    q3   = PI   + base   # π+α₁ … π+αN
    q4   = 2*PI - rev    # 2π−αN … 2π−α₁
    return np.concatenate([q1, q2, q3, q4])


def make_sawtooth(n_points: int = N_POINTS):
    """Return (x, saw) arrays: x ∈ [0, X_TOTAL), saw ∈ [0, 2π)."""
    x   = np.linspace(0, X_TOTAL, n_points, endpoint=False)
    saw = (2 * PI / X_TOTAL) * x
    return x, saw


def generate_pwm(thresholds: np.ndarray, saw: np.ndarray) -> np.ndarray:
    """
    Quarter-wave + half-wave symmetric PWM.

    PWM starts LOW. Toggles at every threshold in `thresholds` (quarter-wave)
    AND at the π midpoint of the carrier (half-wave), so the second half is
    the bitwise complement of the first half.
    """
    toggles = np.zeros(len(saw), dtype=np.uint8)   # start LOW

    for ang in thresholds:
        idx = int(np.searchsorted(saw, ang))
        if 0 < idx < len(saw):
            toggles[idx] ^= 1

    # Half-wave symmetry: extra toggle at π
    idx_pi = int(np.searchsorted(saw, PI))
    if 0 < idx_pi < len(saw):
        toggles[idx_pi] ^= 1

    return np.bitwise_xor.accumulate(toggles)


# ─────────────────────────────────────────────────────────────────────────────
# Console summary
# ─────────────────────────────────────────────────────────────────────────────

def print_summary(base: np.ndarray, all_angles: np.ndarray) -> None:
    n = len(base)
    print("\n" + "─" * 62)
    print(f"  Base angles  (N = {n})")
    for i, a in enumerate(base):
        print(f"    α{i+1:1d} = {a:.6f} rad  ({np.degrees(a):.4f}°)")

    print(f"\n  {4*n} switching angles (quarter-wave) + 1 half-wave toggle at π")
    print(f"  {'Label':6}  {'rad':>10}  {'degrees':>10}   Quarter")
    print("  " + "─" * 46)
    for i, ang in enumerate(all_angles):
        qi   = i // n + 1
        xc   = ang * X_TOTAL / (2 * PI)
        lbl  = f'α{i+1}'
        print(f"  {lbl:6}  {ang:10.6f}  {np.degrees(ang):10.4f}°   Q{qi}  (x={xc:.3f})")
    print("─" * 62 + "\n")


# ─────────────────────────────────────────────────────────────────────────────
# Plot
# ─────────────────────────────────────────────────────────────────────────────

def plot(base: np.ndarray, all_angles: np.ndarray, x: np.ndarray,
         saw: np.ndarray, pwm: np.ndarray) -> None:

    n          = len(base)
    total      = 4 * n
    base_str   = ',  '.join(f'α{i+1}={np.degrees(a):.2f}°' for i, a in enumerate(base))

    fig, (ax1, ax2) = plt.subplots(
        2, 1, figsize=(16, 9), sharex=True,
        gridspec_kw={'height_ratios': [2, 1]}
    )
    fig.suptitle(
        f'SOPWM  (N = {2*n+1} pulses | quarter-wave + half-wave symmetry)  '
        f'─  {base_str}',
        fontsize=11, fontweight='bold'
    )

    # ── Carrier + thresholds ─────────────────────────────────────────────────
    ax1.plot(x, saw, color='#1D3557', lw=1.6, zorder=3, label='Sawtooth carrier')

    for qi, ((x0, x1), col, qname) in enumerate(zip(Q_SPANS, COLORS, Q_NAMES)):
        ax1.axvspan(x0, x1, alpha=0.10, color=col, zorder=1)
        ax1.text((x0 + x1) / 2, 2*PI + 0.38, qname,
                 ha='center', fontsize=9, color=col, fontweight='bold')

        for j in range(n):
            ang = all_angles[qi * n + j]
            lbl = f'α{qi*n + j + 1}'
            xc  = ang * X_TOTAL / (2 * PI)

            # Horizontal dashed threshold line
            ax1.hlines(ang, x0, x1, colors=col, lw=1.6, linestyles='--', zorder=4)

            # Vertical dotted crossing marker
            ax1.axvline(xc, color=col, lw=0.8, ls=':', alpha=0.70, zorder=2)

            # Filled dot at the exact intersection point
            ax1.plot(xc, ang, 'o', color=col, markersize=5, markeredgecolor='white',
                     markeredgewidth=0.8, zorder=6)

    for xv in (2.5, 7.5):
        ax1.axvline(xv, color='gray', lw=0.9, ls='--', alpha=0.4, zorder=2)
    # Half-wave boundary
    ax1.axvline(5.0, color='#7B2D8B', lw=2.0, ls='-', alpha=0.75, zorder=5,
                label='π  (half-wave toggle)')
    ax1.axhline(PI, color='#7B2D8B', lw=1.4, ls='-.', alpha=0.55, zorder=4)

    ax1.set_ylabel('Carrier / angle  (rad)', fontsize=10)
    ax1.set_yticks([0, PI/2, PI, 3*PI/2, 2*PI])
    ax1.set_yticklabels(['0', 'π/2', 'π', '3π/2', '2π'])
    ax1.set_ylim(-0.15, 2*PI + 0.95)

    legend_patches = [mpatches.Patch(facecolor=COLORS[i], alpha=0.5, label=Q_NAMES[i])
                      for i in range(4)]
    purple_patch = mpatches.Patch(facecolor='#7B2D8B', alpha=0.7, label='π  half-wave toggle')
    ax1.legend(handles=[ax1.get_lines()[0]] + legend_patches + [purple_patch],
               fontsize=8.5, loc='upper left', framealpha=0.9)
    ax1.set_title(
        f'{total} switching-angle thresholds (quarter-wave)  '
        f'+ 1 half-wave toggle at π  (-- threshold level · ● dot = carrier crossing · purple = π)',
        fontsize=10
    )

    # \u2500\u2500 PWM output \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500
    ax2.step(x, pwm, where='post', color='#1D3557', lw=1.5, label='PWM output')
    ax2.fill_between(x, 0, pwm, step='post', alpha=0.20, color='#1D3557')

    for qi, ((x0, x1), col) in enumerate(zip(Q_SPANS, COLORS)):
        ax2.axvspan(x0, x1, alpha=0.10, color=col)
        ax2.axvline(x0, color='gray', lw=0.8, ls='--', alpha=0.35)
        for j in range(n):
            ang = all_angles[qi * n + j]
            ax2.axvline(ang * X_TOTAL / (2 * PI), color=col, lw=0.9, ls=':', alpha=0.75)

    # Half-wave boundary marker
    ax2.axvline(5.0, color='#7B2D8B', lw=2.0, ls='-', alpha=0.85,
                label='π  (half-wave toggle)')

    ax2.set_ylabel('PWM state', fontsize=10)
    ax2.set_xlabel('x  (one fundamental period  0 → 10)', fontsize=10)
    ax2.set_yticks([0, 1])
    ax2.set_yticklabels(['LOW', 'HIGH'])
    ax2.set_ylim(-0.20, 1.45)
    ax2.set_title(
        f'{total} switching-angle toggles (quarter-wave) + 1 half-wave toggle at π',
        fontsize=10
)
    ax2.legend(loc='upper right', fontsize=9)

    plt.xlim(0, X_TOTAL)
    plt.tight_layout()
    plt.show()


# ─────────────────────────────────────────────────────────────────────────────
# Entry point
# ─────────────────────────────────────────────────────────────────────────────

def main() -> None:
    print("─" * 62)
    print("  SOPWM Demo — quarter-wave + half-wave symmetry")
    print("─" * 62)

    n          = ask_n()
    base       = ask_angles(n)
    all_angles = quarter_wave_expand(base)

    x, saw = make_sawtooth()
    pwm    = generate_pwm(all_angles, saw)

    print_summary(base, all_angles)

    plot(base, all_angles, x, saw, pwm)


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        print("\nAborted.")
        sys.exit(0)
