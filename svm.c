//#############################################################################
//
// FILE:   svm.c
//
// TITLE:  Open-Loop Space Vector Modulation (SVM) using ePWM with SysConfig
//
//! \addtogroup driver_example_list
//! <h1>ePWM Space Vector Modulation</h1>
//!
//! This example generates 3-phase PWM outputs using Space Vector Modulation.
//! A fake electrical angle is incremented in the EPWM1 ISR to produce
//! rotating voltage vectors in open-loop mode.
//!
//!  - ePWM1 drives Phase A (sync source, generates ISR)
//!  - ePWM2 drives Phase B (synced to ePWM1)
//!  - ePWM3 drives Phase C (synced to ePWM1)
//!
//! The SVM algorithm runs inside the EPWM1 ISR and updates CMPA for all
//! three modules every PWM cycle.
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
//! - electrical_angle
//
//#############################################################################

//
// Included Files
//
#include "driverlib.h"
#include "device.h"
#include "board.h"
#include <signalsight/signalsight.h>
#include "svm.h"
#include <math.h>

//
// Defines
//
#define MATH_PI             3.14159265f
#define MATH_TWO_PI         6.283185307f
#define SAMPLE_RESOLUTION   200          // 10kHz PWM / 50Hz = 200 samples per cycle

//
// Globals
//
SVM_DATA mySvm;

int dataIndex = 0;
float voltage_magnitude = 10.0f;        // Fake voltage amplitude

//
// Pre-computed lookup tables for one full electrical cycle
//
float alphaSnapshot[SAMPLE_RESOLUTION] = {0};
float betaSnapshot[SAMPLE_RESOLUTION]  = {0};

float svm_Ta = 0.0f;
float svm_Tb = 0.0f;
float svm_Tc = 0.0f;
float svm_d1 = 0.0f;
float svm_d2 = 0.0f;
float svm_d0 = 0.0f;

float svm_Ualpha = 0.0f;
float svm_Ubeta = 0.0f;
float svm_Sector = 0.0f;

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

    //new
    SIGNALSIGHT_init();
    //

    //
    // Initialize SVM data structure
    //
    SVM_INIT(&mySvm);
    mySvm.Udc = 24.0f;     // Set DC bus voltage
    mySvm.T_s = 0.0001;     // Switching time
    //
    // Pre-compute one full electrical cycle of alpha/beta reference
    //
    int i;
    float angle;
    for(i = 0; i < SAMPLE_RESOLUTION; i++)
    {
        angle = ((float)i) * MATH_TWO_PI / ((float)SAMPLE_RESOLUTION);
        alphaSnapshot[i] = voltage_magnitude * cosf(angle);
        betaSnapshot[i]  = voltage_magnitude * sinf(angle);
    }

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
    // IDLE loop. SVM math runs entirely inside the ISR.
    //
    while(1)
    {
        //new
        SIGNALSIGHT_sendPlotData();
        //
    }
}

//
// epwm1ISR - ePWM 1 ISR
//
// Runs every PWM cycle. Generates a fake rotating angle, computes SVM
// duty cycles, and writes CMPA values to all three ePWM modules.
//
__interrupt void epwm1ISR(void)
{
    //
    // Toggle GPIO26 HIGH for ISR timing measurement
    //
    GPIO_writePin(26, 1);

    //

    //
    // =============================================
    // FAKE MOTOR ANGLE GENERATOR (open-loop)
    // =============================================
    // Use pre-computed lookup table instead of real-time sinf/cosf
    //
    svm_Ualpha = alphaSnapshot[dataIndex];
    svm_Ubeta  = betaSnapshot[dataIndex];

    mySvm.Us_alpha = svm_Ualpha;
    mySvm.Us_beta  = svm_Ubeta;

    if(dataIndex == SAMPLE_RESOLUTION - 1)
    {
        dataIndex = 0;
    }
    else
    {
        dataIndex++;
    }

    //
    // =============================================
    // RUN THE SVM MATH
    // =============================================
    //
    SVM_EXEC(&mySvm);

    svm_Ta = mySvm.T_a;
    svm_Tb = mySvm.T_b;
    svm_Tc = mySvm.T_c;
    svm_d1 = mySvm.d_1;
    svm_d2 = mySvm.d_2;
    svm_d0 = mySvm.d_0;
    
    svm_Sector = mySvm.sector;
    
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
                                (uint16_t)(mySvm.T_a * myEPWM1_TBPRD));
    EPWM_setCounterCompareValue(myEPWM2_BASE, EPWM_COUNTER_COMPARE_A,
                                (uint16_t)(mySvm.T_b * myEPWM2_TBPRD));
    EPWM_setCounterCompareValue(myEPWM3_BASE, EPWM_COUNTER_COMPARE_A,
                                (uint16_t)(mySvm.T_c * myEPWM3_TBPRD));

    
    
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
