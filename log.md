# SVM Project Change Log

## 2026-05-13: Fixed svm_Ualpha / svm_Ubeta not showing sine/cosine waveforms

### Problem
The `svm_Ualpha` and `svm_Ubeta` variables were not displaying as proper sine/cosine waveforms in the MCU Signal Sight tool.

### Root Causes
1. **Missing radians conversion** — `electrical_angle` was normalized (0 to 1.0) but passed directly to `sinf()`/`cosf()` without scaling to radians (0 to 2*PI).

2. **Runtime trig in ISR** — The original code called `sinf()`/`cosf()` every ISR cycle at 10 kHz. This was replaced with a pre-computed lookup table approach (matching the pattern from `transfer_ex8_signalsight_basic_demo.c`), where one full electrical cycle (200 samples) is computed once at startup and indexed in the ISR.

3. **SignalSight data rate overflow** — At 115200 baud (~11.5 KB/s), capturing 5 float variables (20 bytes) every ISR cycle at 10 kHz (200 KB/s) far exceeded the UART bandwidth. This caused dropped/corrupted samples, distorting the waveform display. Fixed by adding decimation — capturing every 10th ISR cycle (~1 kHz effective rate).

4. **Linker memory overflow** — The two new 200-element float lookup tables (`alphaSnapshot`, `betaSnapshot`) caused the `.const` section to exceed RAMLS5 capacity. Fixed by spreading `.bss`, `.const`, and `.data` across RAMLS5, RAMLS6, and RAMLS7 in `28004x_generic_ram_lnk.cmd`.

### Changes Made
- **svm.c**: Replaced runtime `sinf()`/`cosf()` with pre-computed `alphaSnapshot[]`/`betaSnapshot[]` lookup tables. Added SignalSight capture decimation (every 10th ISR). Set `svm_Ualpha`/`svm_Ubeta` directly from lookup tables before `SVM_EXEC()`.
- **28004x_generic_ram_lnk.cmd**: Extended `.bss`, `.const`, `.data` placement to use `RAMLS5 | RAMLS6`, moved `.sysmem` to `RAMLS7`.

## 2026-05-13: Fixed SVM Sector 2 dwell time formulas (svm.h)

### Problem
`svm_Ta`, `svm_Tb`, `svm_Tc` waveforms showed a discontinuity ("break") at the top of the plot, at the sector 1→2 and 2→3 boundaries (around 60°–120°).

### Root Cause
The dwell time equations for **Sector 2** in `SVM_EXEC()` were incorrect.

For sector 2 (60°–120°), the reference vector is decomposed into active vectors V2(110) and V3(010):

**Wrong formulas (before):**
```
d_1 = (sqrt(3)/2 * Us_alpha - 1/2 * Us_beta) * inv_Udc
d_2 = Us_beta * inv_Udc
T_a = d_0/2 + d_2
T_b = d_0/2 + d_2 + d_1
T_c = d_0/2
```

**Correct formulas (after):**
```
d_1 = (3/2 * Us_alpha + sqrt(3)/2 * Us_beta) * inv_Udc
d_2 = (-3/2 * Us_alpha + sqrt(3)/2 * Us_beta) * inv_Udc
d_0 = 1 - d_1 - d_2
T_a = d_0/2 + d_1
T_b = d_0/2 + d_1 + d_2
T_c = d_0/2
```

### Derivation
The standard SVM dwell times for sector 2 are solved from:

```
Vs = d_1 * V2 + d_2 * V3
```

Where V2 = (2/3)Udc [cos(60°), sin(60°)] and V3 = (2/3)Udc [cos(120°), sin(120°)]:

```
Us_alpha = d_1 * (2/3)Udc * cos(60°) + d_2 * (2/3)Udc * cos(120°)
         = (Udc/3) * d_1 - (Udc/3) * d_2

Us_beta  = d_1 * (2/3)Udc * sin(60°) + d_2 * (2/3)Udc * sin(120°)
         = (sqrt(3)/3)Udc * d_1 + (sqrt(3)/3)Udc * d_2
```

Solving for d_1 and d_2:

```
d_1 = (3/2 * Us_alpha + sqrt(3)/2 * Us_beta) / Udc
d_2 = (-3/2 * Us_alpha + sqrt(3)/2 * Us_beta) / Udc
```

For T_a: Phase A is ON during V2 (110) but OFF during V3 (010), so T_a = d_0/2 + d_1.
For T_b: Phase B is ON during both V2 (110) and V3 (010), so T_b = d_0/2 + d_1 + d_2.
For T_c: Phase C is OFF during both active vectors, so T_c = d_0/2.

### Changes Made
- **svm.h**: Corrected `d_1`, `d_2`, and `T_a` formulas in sector 2 of `SVM_EXEC()`.

## 2026-05-26: SOPWM DMA System — Critical Fixes

### 1. ePWM Instance Mismatch (SysConfig)
**Problem:** SysConfig names "myEPWM4" and "myEPWM7" were mapped to physical EPWM3 (GPIO4/5) and EPWM2 (GPIO2/3). DMA triggers referenced EPWM4SOCA/EPWM7SOCA which never fired because those physical modules were unconfigured.

**Fix:** Changed SysConfig to use actual EPWM4 (GPIO6/7) and EPWM7 (GPIO12/13). DMA dest addresses (0x436B, 0x466B) and triggers now match the real hardware.

### 2. Phase Shift Removed
**Problem:** Hardware carrier phase shifts (667/1333 counts) displaced the ZERO event between modules. This shifted when DMA writes took effect, misaligning the schedule table with the carrier counter.

**Fix:** Set phase shift = 0 on all three ePWMs. The 120°/240° electrical offset is baked into the SOPWM schedule table via `SOPWM_PHASE_OFFSET_DEG[]` — it shifts *which* slots contain switching events, not the carrier timing.

### 3. DMA Transfer Steps
**Problem:** SysConfig generated `srcTransferStep=0, destTransferStep=-4` which caused DMA to re-read the same word and write to the wrong register after each burst.

**Fix:** Corrected to `srcTransferStep=1, destTransferStep=-2` (now generated correctly by SysConfig after reconfiguration). Per C2000 TRM: transferStep replaces burstStep after the last word of a burst.

### 4. DMA Trigger Race — All Channels from EPWM1SOCA
**Problem:** DMA CH2/CH3 triggered by EPWM4SOCA/EPWM7SOCA respectively. The ISR (on EPWM1 INT) called `DMA_configAddresses` at fundamental boundaries, but CH2/CH3 could fire their next burst before the ISR updated them, causing reads from stale buffer pointers (resulting in 0xFFFF values on the 2nd fundamental cycle).

**Fix:** Change all three DMA channels to trigger from **EPWM1SOCA**. This makes DMA and ISR deterministically sequenced — DMA burst completes, then ISR runs, then 20µs gap before next trigger. `DMA_startChannel` calls in ISR removed (not needed with `DMA_CFG_CONTINUOUS_ENABLE` and synchronized triggers).

### 5. SYNC Routing (SysConfig)
**Problem:** SYNC inputs for EPWM4/EPWM7 were set to `SYSCTL_SYNC_IN_SRC_EXTSYNCIN1` (not connected) instead of EPWM1SYNCOUT.

**Fix:** Not critical since TBCLKSYNC starts all counters from CTR=0 simultaneously with identical periods. Optionally fix to EPWM1SYNCOUT for robustness.

### 6. Half-Wave Symmetry Constraint Missing
**Problem:** SOPWM output was missing the forced toggle at the 180° midpoint. Without this, the waveform lacks true half-wave symmetry (f(t) = -f(t - T/2)), resulting in only 6 pulses instead of proper balanced modulation.

**Fix:** Added forced toggle at midpoint slot (slot 25 for 50-slot schedule) at tbprd/2 (≈1000 counts for tbprd=2000). This forces the output LOW at exactly 180°, ensuring:
- First half (0°-180°): LUT-defined switching
- Second half (180°-360°): Exact inverse (automatic via half-wave symmetry)
- Proper balanced 3-phase modulation across all phases

**Changes Made:**
- **sopwm.c** `sopwm_build_phase()`: Added forced CMPA toggle at midpoint after LUT angle binning.

### Pending SysConfig Changes
- [ ] DMA CH2 trigger: EPWM4SOCA → EPWM1SOCA
- [ ] DMA CH3 trigger: EPWM7SOCA → EPWM1SOCA
- [ ] Optionally disable SOC-A on EPWM4/EPWM7
- [ ] Optionally fix SYNC inputs to EPWM1SYNCOUT

## 2026-07-07: SOPWM Parameter-Change Bugs — N-Change Ignored & Phase-B Inversion

This entry documents two distinct, high-impact defects that appeared when changing
SOPWM parameters *on the fly* (i.e. after cold start, while the modulator is already
running). Both stem from the same architectural reality: the output stage uses a
**toggle-only Action Qualifier** whose final pin level is *history-dependent*, and the
waveform is streamed from a **DMA table whose read pointer and burst counters are not
implicitly reset** when a channel is stopped. A change to either `N` (pulse number) or
`f` (fundamental frequency) must therefore reproduce the *exact* cold-start conditions,
or the hardware latches into a wrong state.

### Background: why the output is history-dependent

The AQ for `EPWM_AQ_OUTPUT_A` is configured **TOGGLE-only**:

- `ZERO`  → `NO_CHANGE`
- `PERIOD` → `NO_CHANGE`
- `UP_CMPA` → `TOGGLE`
- `UP_CMPB` → `TOGGLE`

There is **no absolute level action** (no forced HIGH at ZERO / LOW at PERIOD). The pin
level at any instant is the running XOR of every toggle seen since power-up. This is
efficient (a single compare event flips the state, which is exactly what an SOPWM edge
table wants) but it means:

- The **start-of-cycle level** is whatever the previous cycle left behind.
- Any change that alters the **parity** (odd/even count) of toggles inside a fundamental
  period will *invert the entire next cycle*.
- Phase B is generated on **EPWM4 with RED active-low dead-band, which is inverting**
  (AQ LOW → pin HIGH). So a parity/anchor error on B is doubly visible.

Cold start is the known-good reference: all three AQ latches are LOW, all DMA burst /
transfer counters are 0, and every channel's read pointer sits at table slot 0. **The
correct design rule is: every parameter change must replicate cold start exactly.**

---

### Bug 1 — Changing `N_cmd` had no effect (output stuck at the default N=15)

#### Problem
Commanding a new pulse number (e.g. `N_cmd = 7`) while running did nothing: the scope
kept showing the previously-applied pattern (the N=15 default). Only a full reset /
re-flash would pick up a new `N`. Changing `f` *did* take effect — which was the clue.

#### Root Cause
The parameter-apply path only performed a **full DMA re-initialisation when the frequency
changed**. For an N-only change the code merely did `DMA_stopChannel()` →
`DMA_startChannel()`.

`DMA_stopChannel()` **preserves** the channel's burst-count and transfer-count registers
— it halts the channel but does *not* rewind it. On restart the DMA therefore resumed
**mid-table**, at whatever slot it happened to be stopped on, and continued streaming the
*old* table image. Because the new schedule is written assuming playback begins at slot 0,
the freshly-built N=7 table was never actually presented from its start; the old N=15
pattern kept cycling.

Only `SOPWM_ReconfigDma()` — which internally issues `DMA_triggerSoftReset()` — clears the
burst / transfer counters and forces the read pointer back to slot 0. That call lived
*inside* the `if (f changed)` branch, so N-only changes skipped it.

#### Fix
Call `SOPWM_ReconfigDma()` **unconditionally** on every parameter change, so the DMA is
always soft-reset and realigned to slot 0 regardless of whether `N`, `f`, or both changed.
`SOPWM_SetFundamentalHz()` (which only needs to recompute `n_carr`/rotation offsets when
`f` moves) stays inside the frequency branch.

```c
/* main.c : sopwm_apply_param_change() */
if (sopwm_f_fund_cmd != sopwm_f_fund_applied) {
    SOPWM_SetFundamentalHz(sopwm_f_fund_cmd);   /* f-only work */
    sopwm_f_fund_applied = sopwm_f_fund_cmd;
}
/* ALWAYS realign the DMA: soft-reset clears burst/transfer counters
   and rewinds every channel to table slot 0 (cold-start replica). */
SOPWM_ReconfigDma(myDMA0_BASE, myDMA1_BASE, myDMA2_BASE);
```

---

### Bug 2 — Phase-B inverted after `f = 500 → 1000` Hz

#### Problem
After changing the fundamental from 500 Hz to 1000 Hz, phase B came up **inverted** for
the remainder of the run. The initial hypothesis was that `n_carr` parity (odd vs even
number of carriers per fundamental) flipped the B start level.

#### Investigation — the odd/even hypothesis was disproved
A **firmware-matched Python simulator** (`tools/sopwm_sim.py`) was used. It loads the
angle LUTs directly out of `sopwm.c` and reproduces every build step bit-for-bit:
quarter-wave expansion → binary LUT → forced 180° half-wave toggle → frame closure →
slot-rotated B/C copies.

- `f = 500 Hz` → `n_carr = round(50000/500) = 100` (even)
- `f = 1000 Hz` → `n_carr = round(50000/1000) = 50` (even)

**Both are even**, so parity never actually changed. The simulator further showed the
computed **B start-level parity is LOW (0) at *both* frequencies** — i.e. B should *not*
have inverted from a frequency change at all. The real defect was elsewhere.

#### Root Cause
The original `sopwm_frame_closure()` (which appends a final toggle to guarantee an even
total toggle count so the waveform closes cleanly each fundamental) placed that closing
toggle at **slot 0 with `cmpa = 0`**. A compare event at counter 0 fires on the *exact
fundamental boundary*, which **races the per-cycle software re-anchor force** that we
issue at the top of each cycle to reset the AQ latch to LOW.

Depending on the precise ordering of (a) the SW force-LOW and (b) the CTR=0 / CMPA=0
hardware toggle, phase B could latch one extra flip and stay inverted. The simulator
confirmed the hazard was specific to A's slot-0 event (`cmpa = 0` at slot 0); the rotated
B/C copies inherited a boundary-adjacent toggle that collided with the anchor.

#### Fix
Two coordinated changes:

1. **Relocate the closure toggle to the period end** instead of slot 0. The new
   `sopwm_frame_closure()` scans backward and places the closing toggle at
   `TBPRD - 1` (end of the last carrier), never at slot 0 / `cmpa = 0`. This keeps the
   fundamental boundary event-free so nothing races the anchor.

```c
/* sopwm.c : sopwm_frame_closure() — place closing toggle at PERIOD END */
uint16_t k = sopwm_n_carr;
while (k-- > 0U) {
    if (cmpb[k] == OFF && cmpa[k] != OFF && cmpa[k] < (tbprd - 1U)) {
        cmpb[k] = tbprd - 1U;   /* second toggle at end of slot */
        break;
    }
    if (cmpa[k] == OFF) {
        cmpa[k] = tbprd - 1U;   /* single closing toggle at end of slot */
        break;
    }
}
```

2. **Frozen-window force-LOW cold-start replica.** After the counters are zeroed
   (`EPWM_setTimeBaseCounter(...,0U)`, `sopwm_cycle_idx = 0`) and **before** TBCLK is
   re-synced, each phase's AQ latch is explicitly driven LOW:

```c
/* main.c : after zeroing counters, before TBCLKSYNC enable */
EPWM_setActionQualifierSWAction(base, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_LOW);
EPWM_forceActionQualifierSWAction(base, EPWM_AQ_OUTPUT_A);   /* base = EPWM1, EPWM4, EPWM2 */
```

This guarantees all three latches start LOW (exactly like cold start); the inverting RED
dead-band on B then maps that LOW to the correct pin polarity. The waveform-level
`SOPWM_ConfigPhaseBPolarity()` hack is no longer needed and remains **commented out**.

#### Verification (simulator — hardware scope check still pending)
Re-running `tools/sopwm_sim.py` after the fix confirmed, for `N = 7` and `N = 15` at both
`f = 500` and `f = 1000`:

- `A_total` toggle count stays **even** (14 for N7, 30 for N15) → waveform closes cleanly.
- **Slot 0 is event-free** for all three phases (A/C had no slot-0 event; A's first event
  moved to slot 2 at f=500 / slot 1 at f=1000).
- `cmpa == 0` now occurs **only at the mid-slot** (50 for f=500, 25 for f=1000) — a
  mid-fundamental 180° toggle, which is *not* on a boundary and does not race the anchor.
- Phase-B start level is LOW at both frequencies.

---

### Changes Made (both bugs)
- **`main.c`** — `SOPWM_ReconfigDma()` now called **unconditionally** on every parameter
  change (moved out of the `f`-changed branch). Added the frozen-window force-LOW
  re-anchor for EPWM1/EPWM4/EPWM2 after zeroing counters and before `TBCLKSYNC`.
  `SOPWM_ConfigPhaseBPolarity()` left commented out.
- **`pwm/sopwm/sopwm.c`** — `sopwm_frame_closure()` rewritten to place the odd-parity
  closing toggle at **period end** (`TBPRD-1`), never at slot 0 / `cmpa = 0`.
- **`tools/sopwm_sim.py`** — `_frame_closure()` updated to mirror the new period-end
  placement so the firmware-matched simulator stays authoritative.

### Key Lessons
- **Cold start is the ground truth.** Every parameter change must replicate it exactly:
  all AQ latches LOW, all DMA counters 0, all read pointers at slot 0.
- **`DMA_stopChannel()` does not rewind the channel.** Burst/transfer counters survive a
  stop; only a soft reset (`DMA_triggerSoftReset`, inside `SOPWM_ReconfigDma`) realigns to
  slot 0.
- **Toggle-only AQ makes start-level parity fragile.** Never place a compare event on the
  fundamental boundary (`cmpa = 0` at slot 0) — it races the per-cycle SW re-anchor. Keep
  closure toggles at the period end.
- The `n_carr` odd/even theory was a red herring — both test frequencies yield even
  `n_carr`; a firmware-matched simulator disproved it and pinned the true cause.

### Pending Hardware Verification
- [ ] Scope-confirm N changes take effect live (e.g. N=15 → N=7 → N=11).
- [ ] Scope-confirm phase B stays upright across f = 500 → 1000 → 500 Hz.
- [ ] Confirm no glitch on the fundamental boundary after the force-LOW re-anchor.
