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

### Pending SysConfig Changes
- [ ] DMA CH2 trigger: EPWM4SOCA → EPWM1SOCA
- [ ] DMA CH3 trigger: EPWM7SOCA → EPWM1SOCA
- [ ] Optionally disable SOC-A on EPWM4/EPWM7
- [ ] Optionally fix SYNC inputs to EPWM1SYNCOUT
