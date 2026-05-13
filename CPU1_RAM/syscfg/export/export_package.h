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

#ifndef INCLUDE_EXPORTPACKAGE_H_
#define INCLUDE_EXPORTPACKAGE_H_

#include <stdbool.h>
#include <stdint.h>
#include <export/export.h>
#include <transfer_utils.h>


//
// Generic Function Prototypes
//
extern int EXPORTPACKAGE_sendFloatArrayAsByteArray(char *cmd, uint16_t cmdLength, float* data, uint32_t size);
extern int EXPORTPACKAGE_sendUInt32AsByteArray(char *cmd, uint16_t cmdLength, uint32_t val);
extern int EXPORTPACKAGE_sendInt32AsByteArray(char *cmd, uint16_t cmdLength, int32_t val);
extern int EXPORTPACKAGE_sendUInt16AsByteArray(char *cmd, uint16_t cmdLength, uint16_t val);
extern int EXPORTPACKAGE_sendInt16AsByteArray(char *cmd, uint16_t cmdLength, int16_t val);
extern int EXPORTPACKAGE_sendFloatAsByteArray(char *cmd, uint16_t cmdLength, float val);
extern int EXPORTPACKAGE_sendBoolAsByteArray(char *cmd, uint16_t cmdLength, bool val);

extern void EXPORTPACKAGE_takeActionOnMessage(uint16_t cmdByte, uint16_t* msgData, bool error);

#endif /* INCLUDE_EXPORTPACKAGE_H_ */
