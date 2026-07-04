# SOPWM — Problems, Fixes, and Scope Notes

Summary of issues found while bringing up 3-phase DMA-driven SOPWM on TMS320F280049C  
(`epwm_svm` project). Written for debug handoff and oscilloscope verification.

---

## Architecture (reference)

1. **Phase A only** is built from LUT → quarter-wave expansion → `sopwm_bin_lut()` → `sopwm_frame_closure()`.
2. **Phase B/C** = slot-rotated copies of closed phase A (no separate binning):
   - `B[k] = A[(k + off_b) % n]`
   - `C[k] = A[(k + off_c) % n]`
3. **Runtime timing** (`SOPWM_SetFundamentalHz`):
   - `n_carr = round(50000 / f_fund)`, clamped 50–125
   - `off_b = (2*n + 1) / 3`, `off_c = (n + 1) / 3`
   - `sopwm_mid_slot_a` = carrier slot containing 180° (see half-wave fix below)
4. **Phase C polarity**: rotated schedule is inverted vs desired A+240° leg; **EPWM2 RED = Active Low** (SysConfig) corrects this on scope.

**Probe points (high-side leg):**

| Phase | ePWM | GPIO |
|-------|------|------|
| A | EPWM1 | GPIO0 |
| B | EPWM4 | GPIO6 |
| C | EPWM2 | GPIO2 |

Carrier: 50 kHz fixed (`TBPRD = 2000`, SysConfig period = 1999 → 2000 counts 0…1999).

---

## Problem 1 — Fundamental frequency hardcoded

**Symptom:** Timing tied to 1000 Hz / `N_CARR = 50`; could not sweep 400–1000 Hz with one variable.

**Fix:** `SOPWM_SetFundamentalHz(f_fund)`, runtime `sopwm_n_carr`, `SOPWM_ReconfigDma()`, `sopwm_f_fund_cmd` in `main.c`. DMA `transferSize` and wrap follow `sopwm_n_carr`.

**Watch:** `sopwm_f_fund_cmd`, `sopwm_n_carr`, `sopwm_f_fund_applied`.

---

## Problem 2 — Phase shift wrong when changing frequency or N

**Symptom:** B/C lag incorrect at 400/800/900 Hz or after changing `sopwm_N_cmd` (e.g. 7 → 15).

**Causes (multiple):**

1. DMA not reconfigured when `n_carr` changed (`srcWrapStep`, `transferSize`, wrap bursts).
2. Stale double-buffer after parameter change (see Problem 4).
3. Missing `sopwm_repoint_dma()` on N-only change.

**Fix:**

- `SOPWM_ReconfigDma()` on frequency change; stop/restart DMA in `sopwm_apply_timing_change()`.
- `sopwm_repoint_dma()` after `InitSchedule` on both frequency and N-only paths.
- Integer offsets `off_b`, `off_c` recomputed in `SOPWM_SetFundamentalHz()`.

**Scope check:** Measure **matching edge A → B** delay = **120°** of fundamental period; A → C = **240°**.  
At 500 Hz (2 ms period): **667 µs** and **1333 µs**. Do not compare A and B at the same absolute time in the trace.

---

## Problem 3 — Phase C inverted on scope

**Symptom:** Phase C looked inverted at 400/800 Hz; 1000 Hz sometimes looked OK by accident (even `n_carr`).

**Cause:** Slot rotation gives correct **timing** for C but **inverts** the leg waveform vs A+240°.

**Fix:** SysConfig `myEPWM2`: **RED Active Low** (`copyUse = false` on EPWM2 if inheriting wrong DB).  
`SOPWM_ConfigPhaseCPolarity()` exists but may be commented out if SysConfig already sets polarity.

**Note:** SysConfig “RED Active Low” = dead-band polarity **inverted** (not non-inverted).

---

## Problem 4 — Stale DMA buffer (A/B wrong after N or f change)

**Symptom:** N=15 @ 500 Hz — phase A/B wrong; phase C could still look plausible after RED fix.

**Cause:** Bug in `SOPWM_InitSchedule()`:

```c
saved_next = sopwm_sched_next;   // often 0 after runtime swap
build buffer 0
sopwm_sched_next = saved_next;   // still 0
build buffer 0 again            // buffer 1 never updated
```

If `sopwm_sched_active == 1`, DMA kept reading **stale buffer 1**.

**Fix:** Always build buffer 0 then buffer 1; reset `sopwm_sched_active = 0`, clear swap pending, reset `sopwm_cycle_idx`. Caller repoints DMA to buffer 0.

**Files:** `pwm/sopwm/sopwm.c` (`SOPWM_InitSchedule`), `main.c` (`sopwm_repoint_dma` on N change).

---

## Problem 5 — Closure toggle on B/C (misunderstanding)

**Question:** Is half-wave / odd-toggle closure applied to phase B?

**Answer:** Closure runs **once on phase A**; B/C inherit it via rotation. **Do not** call `sopwm_frame_closure()` again on B/C — that can double-apply and break toggle parity.

**Scope trap:** B’s 180° closure is **not** at carrier slot `n/2`:

| Phase | n=100 @ 500 Hz | ~180° closure time |
|-------|----------------|---------------------|
| A | slot 50 | 1.00 ms |
| B | slot 83 | 1.66 ms |
| C | slot 17 | 0.34 ms |

Closure toggles land at different **slot indices** but correct **electrical angles** after rotation.

**CCS watch (N=15, m=0.5, n=100):**

- `sopwm_sched[0][active][50].cmpa == 0`
- `sopwm_sched[1][active][83].cmpa == 0`
- `sopwm_sched[1][active][33].cmpa == 0` (A’s slot-0 odd fix rotated to B’s 0°)

---

## Problem 6 — Phase A not symmetric around π (half-wave toggle)

**Symptom (oscilloscope):** At Q2 / 180°, OFF interval before π and ON interval after π are **unequal** (one long, one short). Reported for N=13, N=15, various frequencies.

**Root cause:** Half-wave toggle was forced at **slot `n/2` leading edge with `cmpa = 0`**. That is exactly 180° **only when `n_carr` is even**.

| f_fund | n_carr | Old mid-slot angle | Asymmetry |
|--------|--------|--------------------|-----------|
| 1000 Hz | 50 | 180.0° | OK |
| 500 Hz | 100 | 180.0° | OK (math) |
| 800 Hz | 63 | **177.1°** | Visible |
| 400 Hz | 125 | **178.6°** | Visible |

Earlier related bug: using `tbprd/2` counts in the mid slot placed toggle at ~183.6° (n=50) instead of 180° — same scope signature.

**Fix:** `sopwm_force_halfwave_toggle()` in `sopwm_frame_closure()`:

- Slot = `floor(180° / span)`
- Counts = fractional position within slot so toggle is at **exactly 180°**
- For even n (e.g. 100): counts = 0 at slot 50 (unchanged)
- For odd n (e.g. 63): slot 31, counts ≈ 1000

**Scope check (GPIO0, phase A):**

1. m = 0.5, N = 15, wait ≥1 fundamental period after any change.
2. Zoom **170°–190°** (500 Hz: ~1.89–1.94 ms from fund start; π at **1.00 ms**).
3. Compare last **OFF** before π vs first **ON** after π — should be **equal** after fix.

**Important:** Over a full period, the **second half is inverted** vs the first (half-wave symmetry), not a copy. Do not expect identical HIGH/LOW pattern in 0–1 ms vs 1–2 ms.

---

## Problem 7 — N = 9 / N = 13 / even-M symmetry (LUT + expectations)

- LUTs `SOPWM_LUT_N9`, `SOPWM_LUT_N13` exist; same build path as N=7/11/15.
- **Even M** (N=9, 13): quarter-wave pair-sum structure differs from odd M (N=7, 11, 15); waveform shape near 90°/270° can look “busy” but is not necessarily a code bug.
- At **m = 0.50**, binning drops **0 angles** for N=13; half-wave + closure logic is the main scope concern, not missing LUT angles.

---

## Problem 8 — Runtime parameter transition breaks scope output

**Symptom:** Simulator correct at steady state; changing `sopwm_N_cmd`, `sopwm_f_fund_cmd`, or `sopwm_m_cmd` live in CCS breaks the waveform until reset.

**Root causes:**

1. **N-only change without stopping DMA** — `InitSchedule()` overwrote `sopwm_sched` while DMA was still reading it (memory race). f-only path stopped DMA; N-only path did not.

2. **Apply at wrong point in fundamental** — if the main loop handled `sopwm_fund_tick` late (SignalSight, etc.), `sopwm_cycle_idx` was already >> 0 while the schedule assumed slot 0. DMA slot index and software state desynced.

3. **Redundant BuildSchedule after InitSchedule** — same tick ran full init then `BuildSchedule` + `CommitSchedule`, causing an extra buffer swap on top of freshly init'd tables.

**Fix (main.c):**

- Unified `sopwm_apply_param_change()` for any **N or f** change: `__disable_interrupts()` → stop all DMA → `SetFundamentalHz` / `ReconfigDma` if f changed → `InitSchedule` → `repoint_dma` → start DMA.
- Handle `sopwm_fund_tick` only when `sopwm_cycle_idx <= 1` (fund period start).
- On apply tick: **skip** `BuildSchedule` / `CommitSchedule` (both buffers already valid).
- **m-only** changes still use the normal double-buffer path when N/f are stable.

**Usage:** After changing N or f in CCS, wait **one full fundamental period** at the new settings before judging the scope. Change one parameter at a time when debugging.

---

## DMA / double-buffer checklist

| Step | When |
|------|------|
| `SOPWM_SetFundamentalHz` | f_fund change |
| `SOPWM_ReconfigDma` | f_fund change |
| `SOPWM_InitSchedule` | startup, f or N change |
| `sopwm_repoint_dma` | after InitSchedule |
| Stop/start DMA | `sopwm_apply_param_change()` (N or f change) |
| `SOPWM_BuildSchedule` + `CommitSchedule` | each fundamental (main loop) |
| Buffer swap | `SOPWM_schedISR` at `sopwm_cycle_idx >= n_carr` |

**Debug variables:** `sopwm_sched_active`, `sopwm_sched_next`, `sopwm_swap_pending`, `sopwm_N_cmd`, `sopwm_N_applied`, `sopwm_lut_n_base`, `sopwm_build_count`.

---

## Simulation tools (repo)

| Script | Purpose |
|--------|---------|
| **`tools/sopwm_sim.py`** | **Main firmware-matched sim: `--N`, `--m`, `--f` → A/B/C PWM, 2 cycles, plot/CSV** |
| `tools/analyze_n13_symmetry.py` | N=13 binning / half-wave (legacy n=50) |
| `tools/check_closure_ab.py` | Phase A closure rotated to B/C |
| `tools/analyze_phase_a_symmetry.py` | Phase A half-wave vs n_carr |

Run: `python tools/sopwm_sim.py --N 15 --m 0.5 --f 500`

Update legacy scripts if they still assume `N_CARR = 50` only; firmware uses runtime `sopwm_n_carr`.

---

## Outstanding / verify on hardware

- [ ] Reflash with all fixes; confirm InitSchedule + exact-180° closure.
- [ ] Retest **400, 600, 700, 800 Hz** (odd n_carr) for π symmetry on GPIO0.
- [ ] Retest **500 Hz, N=15, m=0.5** — A/B delay 667 µs, C 1333 µs.
- [ ] Confirm phase C with RED Active Low on EPWM2 after timing changes.

---

## Key source files

- `pwm/sopwm/sopwm.c` — schedule build, closure, InitSchedule
- `pwm/sopwm/sopwm.h` — API, runtime vars
- `main.c` — `sopwm_f_fund_cmd`, `sopwm_N_cmd`, DMA repoint, timing change
- `epwm_sync_svm.syscfg` — EPWM1/4/2, dead-band, GPIO pins
