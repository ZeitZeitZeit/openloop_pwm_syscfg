//#############################################################################
//
// FILE:   main.c
//
// TITLE:  Open-Loop 3-Phase PWM Application using ePWM with SysConfig
//
//! \addtogroup driver_example_list
//! <h1>ePWM Space Vector Modulation</h1>
//!
//! This application generates 3-phase PWM outputs using Selective Optimal
//! PWM (SOPWM).  A DMA-driven schedule table loads compare values into
//! three ePWM modules each carrier cycle; the main loop rebuilds the
//! schedule once per fundamental period.
//!
//!  - ePWM1 master: ISR + EPWM1SOCA (DMA trigger), optional GPIO0/1 heartbeat
//!  - ePWM6 drives Phase A (DMA CH0 → 0x456B)
//!  - ePWM5 drives Phase B (DMA CH1 → 0x446B)
//!  - ePWM3 drives Phase C (DMA CH2 → 0x426B)
//!
//! \b External \b Connections \n
//! - GPIO10 EPWM6A (Phase A high-side)
//! - GPIO11 EPWM6B (Phase A low-side)
//! - GPIO8  EPWM5A (Phase B high-side)
//! - GPIO9  EPWM5B (Phase B low-side)
//! - GPIO4  EPWM3A (Phase C high-side)
//! - GPIO5  EPWM3B (Phase C low-side)
//! - GPIO0/1  EPWM1A/B (master timer only — not SOPWM schedule)
//! - GPIO26 ISR timing toggle (optional)
//!
//! \b Watch \b Variables \n
//! - sopwm_m
//! - sopwm_theta, sopwm_dutyA, sopwm_dutyB, sopwm_dutyC
//
//#############################################################################

//
// Included Files
//
#include "driverlib.h"
#include "device.h"
#include "board.h"
#include <signalsight/signalsight.h>

//
// Modulation method — swap this header to use a different PWM strategy
//
#include "pwm/sopwm/sopwm.h"

// ---------------------------------------------------------------------------
// SOPWM schedule parameters
//   SOPWM_TBPRD : must match TBPRD configured in SysConfig for ePWM1/6/5/3
//                 100 MHz / 50 kHz = 2000 counts
//   sopwm_N_cmd : pulse number — 7, 9, 11, 13, or 15 (writable from debugger)
//   SOPWM_M_INIT: starting modulation index (main loop updates each fundamental)
// ---------------------------------------------------------------------------
#define SOPWM_TBPRD    2000U
#define SOPWM_N_INIT   7U
#define SOPWM_M_INIT   0.50f

// Runtime pulse number — write 7, 9, 11, 13, or 15 from CCS debugger to change live
volatile uint16_t sopwm_N_cmd = SOPWM_N_INIT;

// Modulation index command — write from debugger or closed-loop controller
float sopwm_m_cmd = SOPWM_M_INIT;

// Debug snapshots — verify DMA is writing the correct schedule values.
// At m=0.50, N=7, span=3.6 deg/slot, TBPRD=2000:
//   Phase A: slot 23 fires at 85.21 deg  → cmpa ≈ 1339  (read at cycle_idx==24)
//   Phase B: slot  1 fires at  6.45 deg  → cmpa ≈ 1583  (read at cycle_idx==2)
//   Phase C: slot 35 fires at 126.45 deg → cmpa ≈  250  (read at cycle_idx==36)
volatile uint16_t dbg_cmpa_snapshot  = 0;  // PhA slot[23] CMPA
volatile uint16_t dbg_cmpb_snapshot  = 0;  // PhA slot[23] CMPB
volatile uint16_t dbg_phB_cmpa_slot0 = 0;  // PhB slot[ 1] CMPA, expect ~1583
volatile uint16_t dbg_phC_cmpa_slot17= 0;  // PhC slot[35] CMPA, expect ~250
volatile uint32_t dbg_isr_count      = 0;

//
// Function Prototypes
//
__interrupt void epwm1ISR(void);

//
// Main
//
void main(void)
{
    //
    // Initialize device clock and peripherals
    //
    Device_init();

    //
    // Disable pin locks and enable internal pull-ups
    //
    Device_initGPIO();

    //
    // Initialize PIE and clear PIE registers. Disables CPU interrupts.
    //
    Interrupt_initModule();

    //
    // Initialize the PIE vector table with pointers to the shell Interrupt
    // Service Routines (ISR).
    //
    Interrupt_initVectorTable();

    //
    // Assign the interrupt service routine to ePWM1 interrupt
    //
    Interrupt_register(INT_EPWM1, &epwm1ISR);

    //
    // Disable sync (Freeze clock to PWM as well)
    //
    SysCtl_disablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);

    //
    // Configure GPIOs and ePWM modules via SysConfig
    //
    Board_init();

    //
    // Output phases (EPWM6/5/3): 120°/240° offset lives in the SOPWM schedule,
    // not hardware phase shift.  EPWM1 alone provides ISR + SOC-A for DMA.
    //
    EPWM_disablePhaseShiftLoad(myEPWM6_BASE);
    EPWM_disablePhaseShiftLoad(myEPWM5_BASE);
    EPWM_disablePhaseShiftLoad(myEPWM3_BASE);
    EPWM_disableInterrupt(myEPWM6_BASE);
    EPWM_disableADCTrigger(myEPWM6_BASE, EPWM_SOC_A);
    EPWM_disableInterrupt(myEPWM5_BASE);
    EPWM_disableInterrupt(myEPWM3_BASE);
    EPWM_disableADCTrigger(myEPWM5_BASE, EPWM_SOC_A);
    EPWM_disableADCTrigger(myEPWM3_BASE, EPWM_SOC_A);

    //
    // Initialize Signal Sight tool state
    //
    SIGNALSIGHT_init();

    //
    // Pre-build BOTH double-buffers BEFORE starting DMA so data is valid
    // from the very first SOC-A trigger.
    //
    SOPWM_InitSchedule(SOPWM_M_INIT, sopwm_N_cmd, SOPWM_TBPRD);

    // Set DMA source addresses to the populated schedule table.
    DMA_configAddresses(myDMA0_BASE,
                        (const void *)(myEPWM6_BASE + EPWM_O_CMPA + 1U),
                        (const void *)&sopwm_sched[0][0][0]);
    DMA_configAddresses(myDMA1_BASE,
                        (const void *)(myEPWM5_BASE + EPWM_O_CMPA + 1U),
                        (const void *)&sopwm_sched[1][0][0]);
    DMA_configAddresses(myDMA2_BASE,
                        (const void *)(myEPWM3_BASE + EPWM_O_CMPA + 1U),
                        (const void *)&sopwm_sched[2][0][0]);

    // Start all three DMA channels — they will auto-trigger on ePWM ZERO.
    DMA_startChannel(myDMA0_BASE);
    DMA_startChannel(myDMA1_BASE);
    DMA_startChannel(myDMA2_BASE);

    //
    // Enable sync and clock to PWM
    //
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);

    //
    // Enable ePWM1 interrupt
    //
    Interrupt_enable(INT_EPWM1);

    //
    // Enable Global Interrupt (INTM) and real-time interrupt (DBGM)
    //
    EINT;
    ERTM;

    //
    // IDLE loop.
    // Rebuild the SOPWM schedule once per fundamental cycle (every 2 ms at
    // 500 Hz fundamental).  SOPWM_schedISR() sets sopwm_fund_tick at the
    // carrier-cycle rollover so this runs at exactly the right rate.
    //
    while(1)
    {
        if (sopwm_fund_tick)
        {
            sopwm_fund_tick = 0U;

            // --- Update m here from closed-loop controller if needed ---
            // sopwm_m_cmd = compute_m(Ualpha, Ubeta, Udc);

            SOPWM_BuildSchedule(sopwm_m_cmd, sopwm_N_cmd, SOPWM_TBPRD);
            SOPWM_CommitSchedule();

            SIGNALSIGHT_capturePlotData();
        }

        SIGNALSIGHT_sendPlotData();
    }
}

//
// epwm1ISR - ePWM 1 ISR
//
// Runs every PWM cycle. Generates a fake rotating angle, computes
// duty cycles via the modulation module, and writes CMPA values to
// all three ePWM modules.
//
__interrupt void epwm1ISR(void)
{
    //
    // Toggle GPIO26 HIGH for ISR timing measurement
    //
    GPIO_writePin(26, 1);

    //
    // Advance carrier-cycle index; set sopwm_fund_tick at fundamental boundary;
    // swap double-buffer if SOPWM_CommitSchedule() was called by main loop.
    // DMA loads CMPA/CMPB into ePWM6/5/3 shadow registers automatically —
    // no manual EPWM_setCounterCompareValue() calls needed here.
    //
    SOPWM_schedISR();
    dbg_isr_count++;

    // Snapshot CMPA one cycle after each target slot fires (shadow loads at ZERO).
    if (sopwm_cycle_idx == 24U)  // PhA slot[23]: expect cmpa≈1339
    {
        dbg_cmpa_snapshot  = EPWM_getCounterCompareValue(myEPWM6_BASE, EPWM_COUNTER_COMPARE_A);
        dbg_cmpb_snapshot  = EPWM_getCounterCompareValue(myEPWM6_BASE, EPWM_COUNTER_COMPARE_B);
    }
    if (sopwm_cycle_idx == 2U)   // PhB slot[ 1]: expect cmpa≈1583
    {
        dbg_phB_cmpa_slot0  = EPWM_getCounterCompareValue(myEPWM5_BASE, EPWM_COUNTER_COMPARE_A);
    }
    if (sopwm_cycle_idx == 36U)  // PhC slot[35]: expect cmpa≈250
    {
        dbg_phC_cmpa_slot17 = EPWM_getCounterCompareValue(myEPWM3_BASE, EPWM_COUNTER_COMPARE_A);
    }

    // After a fundamental rollover, re-point DMA SRC to the newly activated
    // buffer so the next 100-cycle run reads the freshly built schedule.
    if (sopwm_fund_tick)
    {
        // Re-point DMA source to the newly active buffer.
        // All 3 channels trigger from EPWM1SOCA (same event as this ISR),
        // so the next trigger won't fire until the next ZERO (20µs away).
        // DMA_configAddresses updates both srcBeg and srcAddr.
        DMA_configAddresses(myDMA0_BASE,
                            (const void *)(myEPWM6_BASE + EPWM_O_CMPA + 1U),
                            (const void *)&sopwm_sched[0][sopwm_sched_active][0]);
        DMA_configAddresses(myDMA1_BASE,
                            (const void *)(myEPWM5_BASE + EPWM_O_CMPA + 1U),
                            (const void *)&sopwm_sched[1][sopwm_sched_active][0]);
        DMA_configAddresses(myDMA2_BASE,
                            (const void *)(myEPWM3_BASE + EPWM_O_CMPA + 1U),
                            (const void *)&sopwm_sched[2][sopwm_sched_active][0]);
    }

    //
    // Clear INT flag for this timer
    //
    EPWM_clearEventTriggerInterruptFlag(myEPWM1_BASE);

    //
    // Acknowledge interrupt group
    //
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP3);

    //
    // Toggle GPIO26 LOW
    //
    GPIO_writePin(26, 0);
}
