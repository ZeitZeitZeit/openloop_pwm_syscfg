/*
 * Project_Main.c
 * Cleaned up for Open-Loop Space Vector Modulation (SVM) Testing
 */

#include "F28x_Project.h" // Standard TI Device Headers (or driverlib.h)
#include "SVM.h"          // Your perfected SVM header
#include <math.h>         // Required for sinf() and cosf()

// --- Defines ---
#define PWM_TBPRD   5000  // Example Period (Adjust to your switching freq)
#define DEAD_TIME   50    // Example Deadband ticks
#define MATH_PI     3.14159265f

// --- Global Variables ---
SVM_DATA mySvm;

float electrical_angle = 0.0f;
float angle_step = 0.01f;        // Controls the fake motor speed
float voltage_magnitude = 10.0f; // Fake Voltage Amplitude

// --- Function Prototypes ---
extern void GPIO_Config(void);
extern void epwm_cfg(Uint16 period, Uint16 deadtime);
__interrupt void epwm_isr(void);


void main(void)
{
    // 1. Initialize System Control (Clocks, PLL, Watchdog)
    InitSysCtrl();

    // 2. Initialize standard GPIOs
    InitGpio();

    // 3. Clear Interrupts and Initialize PIE Vector Table
    DINT;
    InitPieCtrl();
    IER = 0x0000;
    IFR = 0x0000;
    InitPieVectTable();

    // Map our ISR function to the EPWM1 Interrupt vector
    EALLOW;
    PieVectTable.EPWM1_INT = &epwm_isr;
    EDIS;

    // 4. Disable PWM Clock Sync before configuring
    EALLOW;
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 0;
    EDIS;

    // 5. Run Peripheral Configurations
    gpio_cfg();                        // Use the stripped-down GPIO file
    epwm_cfg(PWM_TBPRD, DEAD_TIME);     // Use your legacy PWM file

    // 6. Initialize SVM Math
    SVM_INIT(&mySvm);
    mySvm.Udc = 24.0f; // Fake the battery voltage to avoid divide-by-zero!

    // 7. Re-enable PWM Clock Sync
    EALLOW;
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 1;
    EDIS;

    // 8. Enable EPWM1 Interrupt in PIE (Group 3, INT 1)
    PieCtrlRegs.PIEIER3.bit.INTx1 = 1;
    IER |= M_INT3;

    // 9. Enable Global Interrupts
    EINT;
    ERTM;

    // 10. Infinite Loop
    for(;;)
    {
        // The CPU chills here. All the SVM math happens in the epwm_isr!
    }
}

// --- The High-Speed PWM Interrupt ---
// This runs every single time the PWM timer hits zero
__interrupt void epwm_isr(void)
{
    // 1. Pull GPIO26 HIGH to measure CPU execution time
    GpioDataRegs.GPASET.bit.GPIO26 = 1;

    // ==========================================
    // FAKE MOTOR ANGLE GENERATOR
    // ==========================================
    electrical_angle += angle_step;
    if (electrical_angle > (2.0f * MATH_PI)) {
        electrical_angle -= (2.0f * MATH_PI); // Wrap around 360 degrees
    }

    mySvm.Us_alpha = voltage_magnitude * cosf(electrical_angle);
    mySvm.Us_beta  = voltage_magnitude * sinf(electrical_angle);


    // ==========================================
    // RUN THE SVM MATH
    // ==========================================
    SVM_EXEC(&mySvm);


    // ==========================================
    // UPDATE HARDWARE REGISTERS
    // ==========================================
    // Convert duty cycle (0.0 to 0.96) into actual timer ticks
    EPwm1Regs.CMPA.bit.CMPA = (uint16_t)(mySvm.T_a * PWM_TBPRD);
    EPwm2Regs.CMPA.bit.CMPA = (uint16_t)(mySvm.T_b * PWM_TBPRD);
    EPwm6Regs.CMPA.bit.CMPA = (uint16_t)(mySvm.T_c * PWM_TBPRD);

    // 2. Clear Interrupt Flags so it can fire again next time
    EPwm1Regs.ETCLR.bit.INT = 1;
    PieCtrlRegs.PIEACK.all |= PIEACK_GROUP3;

    // 3. Pull GPIO26 LOW
    GpioDataRegs.GPACLEAR.bit.GPIO26 = 1;
}

// End of file
