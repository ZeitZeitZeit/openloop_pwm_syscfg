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

#include <export/export.h>
#include <board.h>
#include <export/export_package.h>
void __attribute__((weak)) EXPORT_receivedData(uint16_t* receivedData, uint16_t receivedDataLength) {};



//
// Global buffer to store the received data
//
volatile uint16_t exportRXData [16] = {0};


//*****************************************************************************
//
// EXPORT Transmit and Init
//
//*****************************************************************************
void EXPORT_transmitCharBlocking(char c)
{
    SCI_writeCharBlockingFIFO(myMCUSignalSight0_transferLayer_SCI_BASE, c);
}

void EXPORT_transmitStringLengthBlocking(char *str, uint16_t strLength)
{
    uint16_t i;

    for (i = 0; i < strLength; i++) {
            SCI_writeCharBlockingFIFO(myMCUSignalSight0_transferLayer_SCI_BASE, str[i]);
    }
}

void EXPORT_init()
{
    //
    // Initialize the HW module (The functions below are also called in Board_init)
    //
    // myMCUSignalSight0_transferLayer_SCI_init();


}

//*****************************************************************************
//
// Interrupts
//
//*****************************************************************************
//
// COMs link TX Interrupt (unused)
//
interrupt void INT_myMCUSignalSight0_transferLayer_SCI_TX_ISR(void)
{
    
    SCI_clearOverflowStatus(myMCUSignalSight0_transferLayer_SCI_BASE);
    SCI_clearInterruptStatus(myMCUSignalSight0_transferLayer_SCI_BASE, SCI_INT_TXFF);

    //
    // Issue PIE ack
    //
    Interrupt_clearACKGroup(INT_myMCUSignalSight0_transferLayer_SCI_TX_INTERRUPT_ACK_GROUP);
}

//
// COMs link RX Interrupt
//
interrupt void INT_myMCUSignalSight0_transferLayer_SCI_RX_ISR(void)
{
    uint16_t rxIndex = 0;
    //
    // Read data
    //
    for (rxIndex = 0; rxIndex < 16; rxIndex++){
        exportRXData[rxIndex] = SCI_readCharNonBlocking(myMCUSignalSight0_transferLayer_SCI_BASE);
    }

    if (EXPORT_receivedData) {
        EXPORT_receivedData((uint16_t *)exportRXData, 16);
    }

    SCI_clearOverflowStatus(myMCUSignalSight0_transferLayer_SCI_BASE);
    SCI_clearInterruptStatus(myMCUSignalSight0_transferLayer_SCI_BASE, SCI_INT_RXFF);

    //
    // Issue PIE ack
    //
    Interrupt_clearACKGroup(INT_myMCUSignalSight0_transferLayer_SCI_RX_INTERRUPT_ACK_GROUP);
}
