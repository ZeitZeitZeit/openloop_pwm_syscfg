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
 
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <export/export.h>
#include <export/export_package.h>

void __attribute__((weak)) EXPORTPACKAGE_takeActionOnMessage(uint16_t cmdByte, uint16_t* msgData, bool error) {};

//
// Defines
//
#define EP_START_BYTE 115
#define EP_END_BYTE 101

void EXPORT_receivedData(uint16_t* receivedData, uint16_t receivedDataLength){


    uint16_t cmdByte = receivedData[1];
    //
    // Check received data
    //
    if(receivedData[0] != EP_START_BYTE  || receivedData[15] != EP_END_BYTE){ //verify proper start and end byte
        //Error event
        EXPORTPACKAGE_takeActionOnMessage(cmdByte, &receivedData[2], true);
    }
    else{
        EXPORTPACKAGE_takeActionOnMessage(cmdByte, &receivedData[2], false);
    }
}

int EXPORTPACKAGE_sendUInt32AsByteArray(char *cmd, uint16_t cmdLength, uint32_t val)
{
    uint16_t bytes[UINT32_BYTE_COUNT] = {0};
    uint16_t byteIndex = 0;
    // Send start byte
    EXPORT_transmitCharBlocking(EP_START_BYTE);
    // Key IDs
    uint16_t cmdIndex;
    for(cmdIndex = 0; cmdIndex < cmdLength; cmdIndex++){
        EXPORT_transmitCharBlocking(cmd[cmdIndex]);
    }
    // Send data payload
    u32toBytes(val, bytes);
    for (byteIndex = 0; byteIndex < UINT32_BYTE_COUNT; byteIndex++){
        EXPORT_transmitCharBlocking(bytes[byteIndex]);
    }
    // Send end byte
    EXPORT_transmitCharBlocking(EP_END_BYTE);
    return 0;
}

int EXPORTPACKAGE_sendInt32AsByteArray(char *cmd, uint16_t cmdLength, int32_t val)
{
    uint16_t bytes[INT32_BYTE_COUNT] = {0};
    uint16_t byteIndex = 0;
    // Send start byte
    EXPORT_transmitCharBlocking(EP_START_BYTE);
    // Key IDs
    uint16_t cmdIndex;
    for(cmdIndex = 0; cmdIndex < cmdLength; cmdIndex++){
        EXPORT_transmitCharBlocking(cmd[cmdIndex]);
    }
    // Send data payload
    i32toBytes(val, bytes);
    for (byteIndex = 0; byteIndex < INT32_BYTE_COUNT; byteIndex++){
        EXPORT_transmitCharBlocking(bytes[byteIndex]);
    }
    // Send end byte
    EXPORT_transmitCharBlocking(EP_END_BYTE);
    return 0;
}

int EXPORTPACKAGE_sendUInt16AsByteArray(char *cmd, uint16_t cmdLength, uint16_t val)
{
    uint16_t bytes[UINT16_BYTE_COUNT] = {0};
    uint16_t byteIndex = 0;
    // Send start byte
    EXPORT_transmitCharBlocking(EP_START_BYTE);
    // Key IDs
    uint16_t cmdIndex;
    for(cmdIndex = 0; cmdIndex < cmdLength; cmdIndex++){
        EXPORT_transmitCharBlocking(cmd[cmdIndex]);
    }
    // Send data payload
    u16toBytes(val, bytes);
    for (byteIndex = 0; byteIndex < INT16_BYTE_COUNT; byteIndex++){
        EXPORT_transmitCharBlocking(bytes[byteIndex]);
    }
    // Send end byte
    EXPORT_transmitCharBlocking(EP_END_BYTE);
    return 0;
}

int EXPORTPACKAGE_sendInt16AsByteArray(char *cmd, uint16_t cmdLength, int16_t val)
{
    uint16_t bytes[INT16_BYTE_COUNT] = {0};
    uint16_t byteIndex = 0;

    // Send start byte
    EXPORT_transmitCharBlocking(EP_START_BYTE);
    // Key IDs
    uint16_t cmdIndex;
    for(cmdIndex = 0; cmdIndex < cmdLength; cmdIndex++){
        EXPORT_transmitCharBlocking(cmd[cmdIndex]);
    }
    // Send data payload
    i16toBytes(val, bytes);
    for (byteIndex = 0; byteIndex < INT16_BYTE_COUNT; byteIndex++){
        EXPORT_transmitCharBlocking(bytes[byteIndex]);
    }
    // Send end byte
    EXPORT_transmitCharBlocking(EP_END_BYTE);
    return 0;
}

int EXPORTPACKAGE_sendFloatAsByteArray(char *cmd, uint16_t cmdLength, float val)
{
    uint16_t bytes[FLOAT32_BYTE_COUNT] = {0};
    uint16_t byteIndex = 0;
    // Send start byte
    EXPORT_transmitCharBlocking(EP_START_BYTE);
    // Key IDs
    uint16_t cmdIndex;
    for(cmdIndex = 0; cmdIndex < cmdLength; cmdIndex++){
        EXPORT_transmitCharBlocking(cmd[cmdIndex]);
    }
    // Send data payload
    f32toBytes(val, bytes);
    for (byteIndex = 0; byteIndex < FLOAT32_BYTE_COUNT; byteIndex++){
        EXPORT_transmitCharBlocking(bytes[byteIndex]);
    }
    // Send end byte
    EXPORT_transmitCharBlocking(EP_END_BYTE);
    return 0;
}

int EXPORTPACKAGE_sendBoolAsByteArray(char *cmd, uint16_t cmdLength, bool val)
{
    #define BOOL_BYTE_COUNT_TEMP 2
    uint16_t bytes[BOOL_BYTE_COUNT_TEMP] = {0};
    uint16_t byteIndex = 0;
    // Send start byte
    EXPORT_transmitCharBlocking(EP_START_BYTE);
    // Key IDs
    uint16_t cmdIndex;
    for(cmdIndex = 0; cmdIndex < cmdLength; cmdIndex++){
        EXPORT_transmitCharBlocking(cmd[cmdIndex]);
    }
    // Send data payload
    u16toBytes(val, bytes);
    for (byteIndex = 0; byteIndex < BOOL_BYTE_COUNT_TEMP; byteIndex++){
        EXPORT_transmitCharBlocking(bytes[byteIndex]);
    }
    // Send end byte
    EXPORT_transmitCharBlocking(EP_END_BYTE);
    return 0;
}

int EXPORTPACKAGE_sendFloatArrayAsByteArray(char *cmd, uint16_t cmdLength, float* data, uint32_t size)
{
    uint32_t datai;
    uint16_t bytes[FLOAT32_BYTE_COUNT] = {0};
    uint16_t byteIndex = 0;
    // Send start byte
    EXPORT_transmitCharBlocking(EP_START_BYTE);
    EXPORT_transmitCharBlocking(cmd[0]);
    // Send data payload
    for (datai = 0; datai < size; datai++) {
        f32toBytes(data[datai], bytes);
        for (byteIndex = 0; byteIndex < FLOAT32_BYTE_COUNT; byteIndex++){
            EXPORT_transmitCharBlocking(bytes[byteIndex]);
        }
    }
    // Send end byte
    EXPORT_transmitCharBlocking(EP_END_BYTE);
    return 0;
}

