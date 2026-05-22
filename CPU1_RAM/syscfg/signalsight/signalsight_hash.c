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
 

#include "signalsight_hash.h"
#include "driverlib.h"
#include "device.h"
#include <stdio.h>
#include <stdbool.h>

//
//Hash Table Definitions
//
#if(SSHASH_HASH_TABLE_SELECTED == HASH_TABLE_myHashTable0)
    //
    //Hash Table Variables Externs
    //
    extern float sopwm_m;
    extern float sopwm_theta;
    extern float sopwm_angle1;
    extern float sopwm_angle2;
    extern float sopwm_angle3;
    extern float sopwm_angle4;
    extern float sopwm_angle5;
    extern float sopwm_angle6;
    extern float sopwm_angle7;

    //
    //Hash Table Indices - indices are inclusive
    //
    int16_t SSHASH_uint16_t_startIndex = -1;
    int16_t SSHASH_uint32_t_startIndex = -1;
    int16_t SSHASH_int16_t_startIndex = -1;
    int16_t SSHASH_int32_t_startIndex = -1;
    int16_t SSHASH_bool_startIndex = -1;
    int16_t SSHASH_float_startIndex = 0;

    //
    // Hash Table arrays
    //
    const void* SSHASH_hash_table[SSHASH_NUMBER_OF_VARIABLES] = {
    /*float variable*/                         &sopwm_m,
    /*float variable*/                         &sopwm_theta,
    /*float variable*/                         &sopwm_angle1,
    /*float variable*/                         &sopwm_angle2,
    /*float variable*/                         &sopwm_angle3,
    /*float variable*/                         &sopwm_angle4,
    /*float variable*/                         &sopwm_angle5,
    /*float variable*/                         &sopwm_angle6,
    /*float variable*/                         &sopwm_angle7,
    };
#endif //HASH_TABLE_myHashTable0

