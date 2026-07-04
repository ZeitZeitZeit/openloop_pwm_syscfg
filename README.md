# epwm_svm — 3-Phase SOPWM on TMS320F280049C

DMA-driven **Selective Optimal PWM (SOPWM)** generating 3-phase outputs on a
TI C2000 F280049C LaunchPad. A schedule table is loaded into three ePWM modules
each carrier cycle by DMA; the main loop rebuilds the schedule once per
fundamental period. Outputs are half-wave / quarter-wave symmetric and phase
shifted 120° from each other.

## Hardware

| Phase | ePWM  | High side | Low side | Notes |
|-------|-------|-----------|----------|-------|
| A     | EPWM1 | GPIO0     | GPIO1    | Sync source, generates ISR, DMA CH0 |
| B     | EPWM4 | GPIO6     | GPIO7    | Synced to EPWM1, DMA CH1 |
| C     | EPWM2 | GPIO2     | GPIO3    | Synced to EPWM1, DMA CH2 |
| —     | —     | GPIO26    | —        | ISR timing toggle (optional) |

- **Board:** TMS320F280049C LaunchPad (`targetConfigs/`)
- **Debug probe:** XDS110 (on-board)
- **Carrier:** 50 kHz fixed (`TBPRD = 2000`, SysConfig period = 1999)

## Runtime parameters (CCS watch / write)

| Variable            | Default | Range / values            | Meaning |
|---------------------|---------|---------------------------|---------|
| `sopwm_f_fund_cmd`  | 500 Hz  | 400–1000 Hz               | Fundamental frequency |
| `sopwm_N_cmd`       | 15      | 7, 9, 11, 13, 15          | Pulse number |
| `sopwm_m_cmd`       | 0.50    | 0.01–1.00                 | Modulation index |

Change **one** parameter at a time in the debugger and wait **one full
fundamental period** before judging the scope. N/f changes rebuild both DMA
buffers with the time base frozen; `m` uses the double-buffer path.

## Build / run on a new laptop

The build folder (`CPU1_RAM/`) is git-ignored and regenerates on a clean build,
so no absolute paths carry over. All C2000Ware include/lib paths resolve through
the CCS product variable `COM_TI_C2000WARE_INSTALL_DIR`.

### Required tools (versions this project targets)

| Component                 | Version                | Source in repo |
|---------------------------|------------------------|----------------|
| Code Composer Studio (Theia) | 70.5.0 (CCS 20.x)   | `.ccsproject` |
| TI C2000 CGT compiler     | 25.11.0.LTS            | `.cproject` (`OPT_CODEGEN_VERSION`) |
| C2000Ware                 | 26.00.00.00            | `.cproject`, `CPU1_RAM/subdir_vars.mk` |
| SysConfig                 | 1.27.0 (bundled)       | `epwm_sync_svm.syscfg` header |
| Device support            | TMS320F280049C         | `targetConfigs/` |

### Steps

1. **Install CCS Theia** (same major version). It bundles SysConfig and the
   C2000 CGT. If the bundled compiler is not `25.11.0.LTS`, install that CGT via
   the CCS App Center / Products, or retarget in Project Properties → General.
2. **Install C2000Ware 26.00.00.00** (default `C:\ti\c2000\C2000Ware_26_00_00_00`).
   If installed elsewhere, register it in
   *Window → Preferences → Code Composer Studio → Products* so
   `COM_TI_C2000WARE_INSTALL_DIR` resolves.
3. **Import**: *File → Import → CCS Projects* → select this repo folder
   (leave it in place, do not copy into the workspace).
4. **Clean build** (*Project → Clean*). SysConfig regenerates `board.c`,
   `c2000ware_libraries`, `signalsight`, and links `driverlib.lib`.
5. **Debug**: connect the LaunchPad over USB (XDS110) and launch the
   `CPU1_RAM` debug configuration.

### Notes

- `.project` / `.cproject` reference `C:/TDM-GCC-64/bin` as `CLB_SIM_COMPILER`.
  This is only for CLB simulation, which this project does not use
  (`GENERATE_DIAGRAM = 0`). A "path variable undefined" warning is harmless.
- `GUI_SUPPORT = 1` runs the generated `syscfg/gui_setup.bat` post-build; it is
  self-generated, nothing to install.
- The `.ccsproject` `origin` line is template metadata, not used by the build.
- SignalSight files come from C2000Ware; no separate install to build/flash.

## Source layout

| Path | Purpose |
|------|---------|
| `main.c` | Startup, DMA setup, ISR, per-fundamental schedule rebuild, param-change handling |
| `pwm/sopwm/sopwm.c` / `.h` | LUTs, schedule build, quarter-wave expansion, half-wave closure, DMA reconfig |
| `pwm/svm/` | Space Vector Modulation (alternate method) |
| `epwm_sync_svm.syscfg` | ePWM1/4/2, dead-band, DMA, GPIO configuration |
| `device/` | F28004x driverlib and device support |
| `targetConfigs/` | Debug target / probe configuration |
| `tools/sopwm_sim.py` | Firmware-matched SOPWM simulator (`--N --m --f`) |

## Documentation

- `sopwm_flow.md` — schedule build flow (diagrams)
- `SOPWM_DEBUG_NOTES.md` — known issues, fixes, oscilloscope verification
- `log.md` — change log

## Simulation

```
python tools/sopwm_sim.py --N 15 --m 0.5 --f 500
```

Requires `numpy` (and `matplotlib` for plots). Mirrors the firmware schedule
build so you can compare against scope captures before flashing.
