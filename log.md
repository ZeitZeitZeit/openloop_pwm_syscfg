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
