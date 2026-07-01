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
//!  - ePWM1 drives Phase A (sync source, generates ISR, DMA CH0)
//!  - ePWM4 drives Phase B (synced to ePWM1, DMA CH1)
//!  - ePWM2 drives Phase C (synced to ePWM1, DMA CH2)
//!
//! \b External \b Connections \n
//! - GPIO0 EPWM1A (Phase A high-side)
//! - GPIO1 EPWM1B (Phase A low-side)
//! - GPIO6 EPWM4A (Phase B high-side)
//! - GPIO7 EPWM4B (Phase B low-side)
//! - GPIO2 EPWM2A (Phase C high-side)
//! - GPIO3 EPWM2B (Phase C low-side)
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
//   SOPWM_TBPRD : must match TBPRD configured in SysConfig for ePWM1/4/2
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

// Fundamental frequency command [400, 1000] Hz — write from debugger or host
float sopwm_f_fund_cmd = 1000.0f;
static float sopwm_f_fund_applied = -1.0f;
static uint16_t sopwm_N_applied = SOPWM_N_INIT;

// Debug snapshots — verify DMA is writing the correct schedule values.
// At m=0.50, N=7, span=7.2 deg/slot, TBPRD=2000 (sym A @25, B=rot+33, C=rot+17):
//   Phase A: slot 11 → cmpa ≈ 1670  (cycle_idx==12)
//   Phase B: slot 17 → cmpa = 0 (A slot-0 closure rotated)  (cycle_idx==18)
//   Phase C: mid slot 8 @ 57.6° → cmpa = 0  (cycle_idx==9)
volatile uint16_t dbg_cmpa_snapshot  = 0;  // PhA slot[11] CMPA
volatile uint16_t dbg_cmpb_snapshot  = 0;  // PhA slot[11] CMPB
volatile uint16_t dbg_phB_cmpa_slot0 = 0;  // PhB slot[17] CMPA, expect 0
volatile uint16_t dbg_phC_cmpa_slot17= 0;  // PhC slot[8] mid CMPA, expect 0
volatile uint32_t dbg_isr_count      = 0;

//
// Function Prototypes
//
__interrupt void epwm1ISR(void);

static void sopwm_repoint_dma(void)
{
    DMA_configAddresses(myDMA0_BASE,
                        (const void *)(myEPWM1_BASE + EPWM_O_CMPA + 1U),
                        (const void *)&sopwm_sched[0][sopwm_sched_active][0]);
    DMA_configAddresses(myDMA1_BASE,
                        (const void *)(myEPWM4_BASE + EPWM_O_CMPA + 1U),
                        (const void *)&sopwm_sched[1][sopwm_sched_active][0]);
    DMA_configAddresses(myDMA2_BASE,
                        (const void *)(myEPWM2_BASE + EPWM_O_CMPA + 1U),
                        (const void *)&sopwm_sched[2][sopwm_sched_active][0]);
}

static void sopwm_apply_timing_change(void)
{
    DMA_stopChannel(myDMA0_BASE);
    DMA_stopChannel(myDMA1_BASE);
    DMA_stopChannel(myDMA2_BASE);

    SOPWM_SetFundamentalHz(sopwm_f_fund_cmd);
    SOPWM_ReconfigDma(myDMA0_BASE, myDMA1_BASE, myDMA2_BASE);
    SOPWM_InitSchedule(sopwm_m_cmd, sopwm_N_cmd, SOPWM_TBPRD);
    sopwm_repoint_dma();

    DMA_startChannel(myDMA0_BASE);
    DMA_startChannel(myDMA1_BASE);
    DMA_startChannel(myDMA2_BASE);
}

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
    // Initialize Signal Sight tool state
    //
    SIGNALSIGHT_init();

    //
    // Pre-build BOTH double-buffers BEFORE starting DMA so data is valid
    // from the very first SOC-A trigger.
    //
    SOPWM_SetFundamentalHz(sopwm_f_fund_cmd);
    sopwm_f_fund_applied = sopwm_f_fund_cmd;
    sopwm_N_applied      = sopwm_N_cmd;
    SOPWM_InitSchedule(SOPWM_M_INIT, sopwm_N_cmd, SOPWM_TBPRD);

    // Set DMA source addresses to the populated schedule table.
    DMA_configAddresses(myDMA0_BASE,
                        (const void *)(myEPWM1_BASE + EPWM_O_CMPA + 1U),
                        (const void *)&sopwm_sched[0][0][0]);
    DMA_configAddresses(myDMA1_BASE,
                        (const void *)(myEPWM4_BASE + EPWM_O_CMPA + 1U),
                        (const void *)&sopwm_sched[1][0][0]);
    DMA_configAddresses(myDMA2_BASE,
                        (const void *)(myEPWM2_BASE + EPWM_O_CMPA + 1U),
                        (const void *)&sopwm_sched[2][0][0]);

    SOPWM_ReconfigDma(myDMA0_BASE, myDMA1_BASE, myDMA2_BASE);

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
    // Rebuild the SOPWM schedule once per fundamental cycle.
    // SOPWM_schedISR() sets sopwm_fund_tick at the carrier-cycle rollover.
    //
    while(1)
    {
        if (sopwm_fund_tick)
        {
            sopwm_fund_tick = 0U;

            if (sopwm_f_fund_cmd != sopwm_f_fund_applied) {
                sopwm_f_fund_applied = sopwm_f_fund_cmd;
                sopwm_N_applied      = sopwm_N_cmd;
                sopwm_apply_timing_change();
            } else if (sopwm_N_cmd != sopwm_N_applied) {
                sopwm_N_applied = sopwm_N_cmd;
                SOPWM_InitSchedule(sopwm_m_cmd, sopwm_N_cmd, SOPWM_TBPRD);
            }

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
    // DMA loads CMPA/CMPB into ePWM1/4/2 shadow registers automatically —
    // no manual EPWM_setCounterCompareValue() calls needed here.
    //
    SOPWM_schedISR();
    dbg_isr_count++;

    // Snapshot CMPA one cycle after each target slot fires (shadow loads at ZERO).
    if (sopwm_cycle_idx == 12U)  // PhA slot[11]: expect cmpa≈1670
    {
        dbg_cmpa_snapshot  = EPWM_getCounterCompareValue(myEPWM1_BASE, EPWM_COUNTER_COMPARE_A);
        dbg_cmpb_snapshot  = EPWM_getCounterCompareValue(myEPWM1_BASE, EPWM_COUNTER_COMPARE_B);
    }
    if (sopwm_cycle_idx == 18U)  // PhB slot[17]: expect cmpa=0 (A closure rotated)
    {
        dbg_phB_cmpa_slot0  = EPWM_getCounterCompareValue(myEPWM4_BASE, EPWM_COUNTER_COMPARE_A);
    }
    if (sopwm_cycle_idx == 9U)   // PhC slot[8] mid: expect cmpa=0
    {
        dbg_phC_cmpa_slot17 = EPWM_getCounterCompareValue(myEPWM2_BASE, EPWM_COUNTER_COMPARE_A);
    }

    // After a fundamental rollover, re-point DMA SRC to the newly activated
    // buffer so the next sopwm_n_carr-cycle run reads the freshly built schedule.
    if (sopwm_fund_tick)
    {
        sopwm_repoint_dma();
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
