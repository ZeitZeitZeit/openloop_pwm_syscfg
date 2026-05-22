//#############################################################################
//
// FILE:   main.c
//
// TITLE:  Open-Loop 3-Phase PWM Application using ePWM with SysConfig
//
//! \addtogroup driver_example_list
//! <h1>ePWM Space Vector Modulation</h1>
//!
//! This application generates 3-phase PWM outputs using a selectable
//! modulation method. A fake electrical angle is incremented in the
//! EPWM1 ISR to produce rotating voltage vectors in open-loop mode.
//!
//! The modulation module (e.g. SVM) is kept separate so it can be
//! replaced with another method (e.g. SINPWM, DPWM) without
//! modifying this file beyond the #include and function calls.
//!
//!  - ePWM1 drives Phase A (sync source, generates ISR)
//!  - ePWM2 drives Phase B (synced to ePWM1)
//!  - ePWM3 drives Phase C (synced to ePWM1)
//!
//! \b External \b Connections \n
//! - GPIO0 EPWM1A (Phase A high-side)
//! - GPIO1 EPWM1B (Phase A low-side)
//! - GPIO2 EPWM2A (Phase B high-side)
//! - GPIO3 EPWM2B (Phase B low-side)
//! - GPIO4 EPWM3A (Phase C high-side)
//! - GPIO5 EPWM3B (Phase C low-side)
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
//   SOPWM_TBPRD : must match TBPRD configured in SysConfig for ePWM1/4/7
//                 100 MHz / 50 kHz = 2000 counts
//   SOPWM_N     : pulse number — 7, 9, 11, 13 or 15
//   SOPWM_M_INIT: starting modulation index (main loop updates each fundamental)
// ---------------------------------------------------------------------------
#define SOPWM_TBPRD    2000U
#define SOPWM_N        7U
#define SOPWM_M_INIT   0.50f

// Modulation index command — write from debugger or closed-loop controller
float sopwm_m_cmd = SOPWM_M_INIT;

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
    // Initialize Signal Sight tool state
    //
    SIGNALSIGHT_init();

    // Static DMA channel config (trigger, burst, transfer, DST) is generated
    // by SysConfig into Board_init() above — myDMA0/1/2 map to CH1/CH2/CH3.
    // Point each channel's SRC to the initial active schedule buffer (index 0).
    DMA_configAddresses(myDMA0_BASE,
                        (const void *)myDMA0_DESTADDRESS,
                        (const void *)&sopwm_sched[0][0][0]);
    DMA_configAddresses(myDMA1_BASE,
                        (const void *)myDMA1_DESTADDRESS,
                        (const void *)&sopwm_sched[1][0][0]);
    DMA_configAddresses(myDMA2_BASE,
                        (const void *)myDMA2_DESTADDRESS,
                        (const void *)&sopwm_sched[2][0][0]);

    // Start all three DMA channels — they will auto-trigger on ePWM ZERO.
    DMA_startChannel(myDMA0_BASE);
    DMA_startChannel(myDMA1_BASE);
    DMA_startChannel(myDMA2_BASE);

    //
    // Pre-build BOTH double-buffers so DMA has valid data from cycle 0.
    // (BuildSchedule alone only fills the inactive buffer; the active buffer
    // would start uninitialized for the first 50 carrier cycles.)
    //
    SOPWM_InitSchedule(SOPWM_M_INIT, SOPWM_N, SOPWM_TBPRD);

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
    // Rebuild the SOPWM schedule once per fundamental cycle (every 1 ms at
    // 1 kHz fundamental).  SOPWM_schedISR() sets sopwm_fund_tick at the
    // carrier-cycle rollover so this runs at exactly the right rate.
    //
    while(1)
    {
        if (sopwm_fund_tick)
        {
            sopwm_fund_tick = 0U;

            // --- Update m here from closed-loop controller if needed ---
            // sopwm_m_cmd = compute_m(Ualpha, Ubeta, Udc);

            SOPWM_BuildSchedule(sopwm_m_cmd, SOPWM_N, SOPWM_TBPRD);
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
    // DMA loads CMPA/CMPB into ePWM1/4/7 shadow registers automatically —
    // no manual EPWM_setCounterCompareValue() calls needed here.
    //
    SOPWM_schedISR();

    // After a fundamental rollover, re-point DMA SRC to the newly activated
    // buffer so the next 50-cycle run reads the freshly built schedule.
    if (sopwm_fund_tick)
    {
        DMA_configAddresses(myDMA0_BASE,
                            (const void *)myDMA0_DESTADDRESS,
                            (const void *)&sopwm_sched[0][sopwm_sched_active][0]);
        DMA_configAddresses(myDMA1_BASE,
                            (const void *)myDMA1_DESTADDRESS,
                            (const void *)&sopwm_sched[1][sopwm_sched_active][0]);
        DMA_configAddresses(myDMA2_BASE,
                            (const void *)myDMA2_DESTADDRESS,
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
