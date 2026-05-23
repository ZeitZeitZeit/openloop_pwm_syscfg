/*
 * Copyright (c) 2020 Texas Instruments Incorporated - http://www.ti.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef BOARD_H
#define BOARD_H

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

//
// Included Files
//

#include "driverlib.h"
#include "device.h"

//*****************************************************************************
//
// PinMux Configurations
//
//*****************************************************************************

//
// EPWM1 -> myEPWM1 Pinmux
//
//
// EPWM1_A - GPIO Settings
//
#define GPIO_PIN_EPWM1_A 0
#define myEPWM1_EPWMA_GPIO 0
#define myEPWM1_EPWMA_PIN_CONFIG GPIO_0_EPWM1_A
//
// EPWM1_B - GPIO Settings
//
#define GPIO_PIN_EPWM1_B 1
#define myEPWM1_EPWMB_GPIO 1
#define myEPWM1_EPWMB_PIN_CONFIG GPIO_1_EPWM1_B

//
// EPWM3 -> myEPWM4 Pinmux
//
//
// EPWM3_A - GPIO Settings
//
#define GPIO_PIN_EPWM3_A 4
#define myEPWM4_EPWMA_GPIO 4
#define myEPWM4_EPWMA_PIN_CONFIG GPIO_4_EPWM3_A
//
// EPWM3_B - GPIO Settings
//
#define GPIO_PIN_EPWM3_B 5
#define myEPWM4_EPWMB_GPIO 5
#define myEPWM4_EPWMB_PIN_CONFIG GPIO_5_EPWM3_B

//
// EPWM2 -> myEPWM7 Pinmux
//
//
// EPWM2_A - GPIO Settings
//
#define GPIO_PIN_EPWM2_A 2
#define myEPWM7_EPWMA_GPIO 2
#define myEPWM7_EPWMA_PIN_CONFIG GPIO_2_EPWM2_A
//
// EPWM2_B - GPIO Settings
//
#define GPIO_PIN_EPWM2_B 3
#define myEPWM7_EPWMB_GPIO 3
#define myEPWM7_EPWMB_PIN_CONFIG GPIO_3_EPWM2_B

//
// SCIA -> myMCUSignalSight0_transferLayer_SCI Pinmux
//
//
// SCIA_RX - GPIO Settings
//
#define GPIO_PIN_SCIA_RX 28
#define myMCUSignalSight0_transferLayer_SCI_SCIRX_GPIO 28
#define myMCUSignalSight0_transferLayer_SCI_SCIRX_PIN_CONFIG GPIO_28_SCIA_RX
//
// SCIA_TX - GPIO Settings
//
#define GPIO_PIN_SCIA_TX 29
#define myMCUSignalSight0_transferLayer_SCI_SCITX_GPIO 29
#define myMCUSignalSight0_transferLayer_SCI_SCITX_PIN_CONFIG GPIO_29_SCIA_TX

//*****************************************************************************
//
// DMA Configurations
//
//*****************************************************************************
#define myDMA0_SRCADDRESS 0 
#define myDMA0_DESTADDRESS 16491 
#define myDMA0_BASE DMA_CH1_BASE 
#define myDMA0_BURSTSIZE 2U
#define myDMA0_TRANSFERSIZE 50U
#define myDMA0_SRC_WRAPSIZE 100U
#define myDMA0_DEST_WRAPSIZE 65535U
void myDMA0_init();
#define myDMA1_SRCADDRESS 0 
#define myDMA1_DESTADDRESS 17259 
#define myDMA1_BASE DMA_CH2_BASE 
#define myDMA1_BURSTSIZE 2U
#define myDMA1_TRANSFERSIZE 50U
#define myDMA1_SRC_WRAPSIZE 100U
#define myDMA1_DEST_WRAPSIZE 65535U
void myDMA1_init();
#define myDMA2_SRCADDRESS 0 
#define myDMA2_DESTADDRESS 18027 
#define myDMA2_BASE DMA_CH3_BASE 
#define myDMA2_BURSTSIZE 2U
#define myDMA2_TRANSFERSIZE 50U
#define myDMA2_SRC_WRAPSIZE 100U
#define myDMA2_DEST_WRAPSIZE 65535U
void myDMA2_init();

//*****************************************************************************
//
// EPWM Configurations
//
//*****************************************************************************
#define myEPWM1_BASE EPWM1_BASE
#define myEPWM1_TBPRD 1999
#define myEPWM1_COUNTER_MODE EPWM_COUNTER_MODE_UP
#define myEPWM1_TBPHS 0
#define myEPWM1_CMPA 1000
#define myEPWM1_CMPB 500
#define myEPWM1_CMPC 0
#define myEPWM1_CMPD 0
#define myEPWM1_DBRED 240
#define myEPWM1_DBFED 240
#define myEPWM1_TZA_ACTION EPWM_TZ_ACTION_HIGH_Z
#define myEPWM1_TZB_ACTION EPWM_TZ_ACTION_HIGH_Z
#define myEPWM1_INTERRUPT_SOURCE EPWM_INT_TBCTR_ZERO
#define myEPWM4_BASE EPWM3_BASE
#define myEPWM4_TBPRD 1999
#define myEPWM4_COUNTER_MODE EPWM_COUNTER_MODE_UP
#define myEPWM4_TBPHS 667
#define myEPWM4_CMPA 1000
#define myEPWM4_CMPB 500
#define myEPWM4_CMPC 0
#define myEPWM4_CMPD 0
#define myEPWM4_DBRED 240
#define myEPWM4_DBFED 240
#define myEPWM4_TZA_ACTION EPWM_TZ_ACTION_HIGH_Z
#define myEPWM4_TZB_ACTION EPWM_TZ_ACTION_HIGH_Z
#define myEPWM4_INTERRUPT_SOURCE EPWM_INT_TBCTR_ZERO
#define myEPWM7_BASE EPWM2_BASE
#define myEPWM7_TBPRD 1999
#define myEPWM7_COUNTER_MODE EPWM_COUNTER_MODE_UP
#define myEPWM7_TBPHS 1333
#define myEPWM7_CMPA 1000
#define myEPWM7_CMPB 500
#define myEPWM7_CMPC 0
#define myEPWM7_CMPD 0
#define myEPWM7_DBRED 240
#define myEPWM7_DBFED 240
#define myEPWM7_TZA_ACTION EPWM_TZ_ACTION_HIGH_Z
#define myEPWM7_TZB_ACTION EPWM_TZ_ACTION_HIGH_Z
#define myEPWM7_INTERRUPT_SOURCE EPWM_INT_TBCTR_ZERO

//*****************************************************************************
//
// INPUTXBAR Configurations
//
//*****************************************************************************
#define myINPUTXBARINPUT0_SOURCE 56
#define myINPUTXBARINPUT0_INPUT XBAR_INPUT5
void myINPUTXBARINPUT0_init();
#define myINPUTXBARINPUT1_SOURCE 56
#define myINPUTXBARINPUT1_INPUT XBAR_INPUT6
void myINPUTXBARINPUT1_init();

//*****************************************************************************
//
// INTERRUPT Configurations
//
//*****************************************************************************

// Interrupt Settings for INT_myMCUSignalSight0_transferLayer_SCI_RX
// ISR need to be defined for the registered interrupts
#define INT_myMCUSignalSight0_transferLayer_SCI_RX INT_SCIA_RX
#define INT_myMCUSignalSight0_transferLayer_SCI_RX_INTERRUPT_ACK_GROUP INTERRUPT_ACK_GROUP9
extern __interrupt void INT_myMCUSignalSight0_transferLayer_SCI_RX_ISR(void);

// Interrupt Settings for INT_myMCUSignalSight0_transferLayer_SCI_TX
// ISR need to be defined for the registered interrupts
#define INT_myMCUSignalSight0_transferLayer_SCI_TX INT_SCIA_TX
#define INT_myMCUSignalSight0_transferLayer_SCI_TX_INTERRUPT_ACK_GROUP INTERRUPT_ACK_GROUP9
extern __interrupt void INT_myMCUSignalSight0_transferLayer_SCI_TX_ISR(void);

//*****************************************************************************
//
// SCI Configurations
//
//*****************************************************************************
#define myMCUSignalSight0_transferLayer_SCI_BASE SCIA_BASE
#define myMCUSignalSight0_transferLayer_SCI_BAUDRATE 115200
#define myMCUSignalSight0_transferLayer_SCI_CONFIG_WLEN SCI_CONFIG_WLEN_8
#define myMCUSignalSight0_transferLayer_SCI_CONFIG_STOP SCI_CONFIG_STOP_ONE
#define myMCUSignalSight0_transferLayer_SCI_CONFIG_PAR SCI_CONFIG_PAR_NONE
#define myMCUSignalSight0_transferLayer_SCI_FIFO_TX_LVL SCI_FIFO_TX0
#define myMCUSignalSight0_transferLayer_SCI_FIFO_RX_LVL SCI_FIFO_RX16
void myMCUSignalSight0_transferLayer_SCI_init();

//*****************************************************************************
//
// SYNC Scheme Configurations
//
//*****************************************************************************

//*****************************************************************************
//
// Board Configurations
//
//*****************************************************************************
void	Board_init();
void	DMA_init();
void	EPWM_init();
void	INPUTXBAR_init();
void	INTERRUPT_init();
void	SCI_init();
void	SYNC_init();
void	PinMux_init();

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif  // end of BOARD_H definition
