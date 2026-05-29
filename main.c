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
//! - mySvm.sector
//! - mySvm.T_a, mySvm.T_b, mySvm.T_c
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
#include "pwm/svm/svm.h"

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
    // The DAC output buffer was enabled in SysConfig, but the internal analog
    // reference (VREFHI) must be explicitly connected via ASysCtl on F280049C.
    // Without this, the DAC output rail stays at 0 V regardless of shadow value.
    // DACA uses VREFHIA, DACB uses VREFHIB.
    //
    EALLOW;
    ASysCtl_setAnalogReferenceInternal(ASYSCTL_VREFHIA | ASYSCTL_VREFHIB);
    EDIS;
    DEVICE_DELAY_US(5000);   // allow internal reference to settle

    //
    // Initialize Signal Sight tool state
    //
    SIGNALSIGHT_init();

    //
    // Initialize modulation module (open-loop SVM)
    // Udc=24V, Ts=0.0001s, magnitude=10, 200 samples/cycle
    //
    SVM_openLoopInit(24.0f, 0.0001f, 10.0f, 200);

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
    // IDLE loop. Modulation math runs entirely inside the ISR.
    //
    while(1)
    {
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
    // Run the modulation module — returns three duty cycles
    //
    float dutyA, dutyB, dutyC;
    SVM_openLoopRun(&dutyA, &dutyB, &dutyC);

    //
    // Decimate SignalSight capture to reduce UART data rate
    // Capture every 10th ISR = ~1kHz effective sample rate
    //
    {
        static int captureDiv = 0;
        if(++captureDiv >= 10)
        {
            SIGNALSIGHT_capturePlotData();
            captureDiv = 0;
        }
    }

    //
    // =============================================
    // UPDATE HARDWARE REGISTERS
    // =============================================
    // Convert duty cycle (0.0 to 0.96) into timer compare ticks
    //
    EPWM_setCounterCompareValue(myEPWM1_BASE, EPWM_COUNTER_COMPARE_A,
                                (uint16_t)(dutyA * myEPWM1_TBPRD));
    EPWM_setCounterCompareValue(myEPWM2_BASE, EPWM_COUNTER_COMPARE_A,
                                (uint16_t)(dutyB * myEPWM2_TBPRD));
    EPWM_setCounterCompareValue(myEPWM3_BASE, EPWM_COUNTER_COMPARE_A,
                                (uint16_t)(dutyC * myEPWM3_TBPRD));

    //
    // Output Ta and Tb on DACA/DACB for oscilloscope viewing.
    // DAC is 12-bit (0–4095). Duty cycle [0.0, 1.0] → [0, 4095].
    // DACA = Ta, DACB = Tb. Tc = 1 - Ta - Tb (view mathematically).
    //
    DAC_setShadowValue(myDAC0_BASE, (uint16_t)(dutyA * 4095.0f));
    DAC_setShadowValue(myDAC1_BASE, (uint16_t)(dutyB * 4095.0f));

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
