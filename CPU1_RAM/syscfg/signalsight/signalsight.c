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

#include <signalsight/signalsight.h>

//
// Defines
//
#define SS_NUMBER_OF_SIMULTANEOUS_STREAM_VARIABLES 4
#define SS_DEFAULT_STREAM_VAR_A 0
#define SS_DEFAULT_STREAM_VAR_B 1
#define SS_DEFAULT_STREAM_VAR_C 2
#define SS_DEFAULT_STREAM_VAR_D 3
#define SS_BUFFER_SIZE 50
#define SS_POLLING_VARIABLE_MAX 20
#define SS_PING_PONG_BUFFERING true
#define SS_DEFAULT_STREAM_VAR_A 0
#if SSHASH_NUMBER_OF_VARIABLES == 1
#define SS_DEFAULT_STREAM_VAR_B 0
#define SS_DEFAULT_STREAM_VAR_C 0
#define SS_DEFAULT_STREAM_VAR_D 0
#elif SSHASH_NUMBER_OF_VARIABLES == 2
#define SS_DEFAULT_STREAM_VAR_B 1
#define SS_DEFAULT_STREAM_VAR_C 1
#define SS_DEFAULT_STREAM_VAR_D 1
#elif SSHASH_NUMBER_OF_VARIABLES == 3
#define SS_DEFAULT_STREAM_VAR_B 1
#define SS_DEFAULT_STREAM_VAR_C 2
#define SS_DEFAULT_STREAM_VAR_D 2
#elif SSHASH_NUMBER_OF_VARIABLES >= 4
#define SS_DEFAULT_STREAM_VAR_B 1
#define SS_DEFAULT_STREAM_VAR_C 2
#define SS_DEFAULT_STREAM_VAR_D 3
#endif

//
// Enumerations
//
enum SS_STREAMING_STATES {
    STREAM_IDLE = 0,
    STREAM1 = 1,
    STREAM2 = 2,
    STREAM3 = 3,
    STREAM4 = 4,
};
enum SS_CMDS { 
    NO_UPDATE = 0, //default
    PACKET_ERROR = 39,
    INITCMD = 40,
    INIT_STREAMING_VARS = 44,
    START_STREAMING_A = 45,
    START_STREAMING_B = 46,
    START_STREAMING_C = 47,
    START_STREAMING_D = 48,
    STOP_STREAMING_A = 49,
    STOP_STREAMING_B = 50,
    STOP_STREAMING_C = 51,
    STOP_STREAMING_D = 52,
    STREAM_DATA_1_VAR = 53,
    STREAM_DATA_2_VAR = 54,
    STREAM_DATA_3_VAR = 55,
    STREAM_DATA_4_VAR = 56,
    CLEAR_STREAMING_PLOT = 78,
};


//
// Stream State Global Variables
//
enum SS_STREAMING_STATES SS_currentStreamState = STREAM_IDLE;
int16_t SS_currentPlotHashIndices[SS_NUMBER_OF_SIMULTANEOUS_STREAM_VARIABLES] = {SS_DEFAULT_STREAM_VAR_A, SS_DEFAULT_STREAM_VAR_B, SS_DEFAULT_STREAM_VAR_C, SS_DEFAULT_STREAM_VAR_D};
bool SS_streamToggleStates[SS_NUMBER_OF_SIMULTANEOUS_STREAM_VARIABLES] = {false, false, false, false};
enum SS_CMDS SS_nextTxCMD = NO_UPDATE;

//
// Stream Buffering Global Variables
//
float SS_streamA_buffer[SS_BUFFER_SIZE] = {0};
float SS_streamB_buffer[SS_BUFFER_SIZE] = {0};
float SS_streamC_buffer[SS_BUFFER_SIZE] = {0};
float SS_streamD_buffer[SS_BUFFER_SIZE] = {0};
volatile uint16_t SS_bufferElementIndex = 0;
bool SS_bufferIsFull = false;
uint16_t SS_streamDataSkips = 0;
bool SS_initiallyClearBuffer = true;
    
float SS_streamA_buffer2[SS_BUFFER_SIZE] = {0};
float SS_streamB_buffer2[SS_BUFFER_SIZE] = {0};
float SS_streamC_buffer2[SS_BUFFER_SIZE] = {0};
float SS_streamD_buffer2[SS_BUFFER_SIZE] = {0};
volatile uint16_t SS_bufferElementIndex2 = 0;
bool SS_bufferIsFull2 = false;
bool SS_writeBuffer1readBuffer2 = true;
uint16_t SS_streamDataSkips2 = 0;

void SIGNALSIGHT_resetStates()
{
    //
    //Initialize/clear buffer values
    //
    SS_bufferElementIndex = 0;
    SS_bufferIsFull = false;
    SS_bufferElementIndex2 = 0;
    SS_bufferIsFull2 = false;
    SS_writeBuffer1readBuffer2 = true;
}

//*****************************************************************************
//
// Signal Sight Initialization function
//
// Call this function once in your initialization code. Call after
// device initialization functions, interrupt initialization functions and  
// Board_init() but before enabling interrupts globally.
//
//*****************************************************************************
void SIGNALSIGHT_init()
{
    EXPORT_init();
    SIGNALSIGHT_resetStates();
}

void SIGNALSIGHT_sendAcknowledge(enum SS_CMDS command)
{
    enum SS_CMDS* cmdPtr = &command;
    EXPORTPACKAGE_sendFloatArrayAsByteArray((char*)cmdPtr, 1, NULL, 0);
    SS_nextTxCMD = NO_UPDATE;
}

void* SIGNALSIGHT_getVariableAddressAtHashIndex(int16_t hashIndex)
{
    const void* variableAddress = SSHASH_hash_table[hashIndex];
    return (float*)variableAddress;
}

int16_t SIGNALSIGHT_getHashIndexAtVariableAddress(void* variable)
{
    int16_t i;
    for(i = 0; i < SSHASH_NUMBER_OF_VARIABLES; i++){
        if(SSHASH_hash_table[i] == variable){
            return i;
        }
    }
    //
    // Error: Variable not found in Hash table
    //
    return -1; 
}

void SIGNALSIGHT_queueUpStreamData(uint16_t numChannelsON, bool buffer1)
{
    //
    // Read current stream toggle states into temp array in case state change happens during transmission
    //
    bool SS_streamToggleStatesTemp[4] = {false, false, false, false};
    SS_streamToggleStatesTemp[0] = SS_streamToggleStates[0];
    SS_streamToggleStatesTemp[1] = SS_streamToggleStates[1];
    SS_streamToggleStatesTemp[2] = SS_streamToggleStates[2];
    SS_streamToggleStatesTemp[3] = SS_streamToggleStates[3];
    uint16_t channelsToStream = 0;
    if(buffer1){
        //
        // Capture data and write to buffer 1
        //
        if(SS_streamToggleStatesTemp[0]){
            //
            // Channel A
            //
            float* dataPtr = SIGNALSIGHT_getVariableAddressAtHashIndex(SS_currentPlotHashIndices[0]);
            SS_streamA_buffer[SS_bufferElementIndex] = *dataPtr;
            channelsToStream++;
        }
        if(SS_streamToggleStatesTemp[1]){
            //
            // Channel B
            //
            float* dataPtr = SIGNALSIGHT_getVariableAddressAtHashIndex(SS_currentPlotHashIndices[1]);
            SS_streamB_buffer[SS_bufferElementIndex] = *dataPtr;
            channelsToStream++;
        }
        if(SS_streamToggleStatesTemp[2]){
            //
            // Channel C
            //
            float* dataPtr = SIGNALSIGHT_getVariableAddressAtHashIndex(SS_currentPlotHashIndices[2]);
            SS_streamC_buffer[SS_bufferElementIndex] = *dataPtr;
            channelsToStream++;
        }
        if(SS_streamToggleStatesTemp[3]){
            //
            // Channel D
            //
            float* dataPtr = SIGNALSIGHT_getVariableAddressAtHashIndex(SS_currentPlotHashIndices[3]);
            SS_streamD_buffer[SS_bufferElementIndex] = *dataPtr;
            channelsToStream++;
        }
        SS_bufferElementIndex++;
        if(SS_bufferElementIndex == SS_BUFFER_SIZE){
            SS_bufferIsFull = true;
            SS_bufferElementIndex = 0;
        }
    }
    else{
        //
        // Capture data and write to buffer 2
        //
        if(SS_streamToggleStatesTemp[0]){
            //
            // Channel A
            //
            float* dataPtr = SIGNALSIGHT_getVariableAddressAtHashIndex(SS_currentPlotHashIndices[0]);
            SS_streamA_buffer2[SS_bufferElementIndex2] = *dataPtr;
            channelsToStream++;
        }
        if(SS_streamToggleStatesTemp[1]){
            //
            // Channel B
            //
            float* dataPtr = SIGNALSIGHT_getVariableAddressAtHashIndex(SS_currentPlotHashIndices[1]);
            SS_streamB_buffer2[SS_bufferElementIndex2] = *dataPtr;
            channelsToStream++;
        }
        if(SS_streamToggleStatesTemp[2]){
            //
            // Channel C
            //
            float* dataPtr = SIGNALSIGHT_getVariableAddressAtHashIndex(SS_currentPlotHashIndices[2]);
            SS_streamC_buffer2[SS_bufferElementIndex2] = *dataPtr;
            channelsToStream++;
        }
        if(SS_streamToggleStatesTemp[3]){
            //
            // Channel D
            //
            float* dataPtr = SIGNALSIGHT_getVariableAddressAtHashIndex(SS_currentPlotHashIndices[3]);
            SS_streamD_buffer2[SS_bufferElementIndex2] = *dataPtr;
            channelsToStream++;
        }
        SS_bufferElementIndex2++;
        if(SS_bufferElementIndex2 == SS_BUFFER_SIZE){
            SS_bufferIsFull2 = true;
            SS_bufferElementIndex2 = 0;
        }
    }
}

//*****************************************************************************
//
// Signal Sight Capture Plot Data function
//
// Call this function where data should be sampled in your application code.
// A snapshot of the value of your plot variable will be captured and saved 
// for transmission in a future call of SIGNALSIGHT_sendPlotData().
// (example: in a CPU timer or ePWM ISR). 
//
//*****************************************************************************
void SIGNALSIGHT_capturePlotData(){  //write process
    //
    // Sample streaming data
    //
    if(SS_writeBuffer1readBuffer2){
        //
        // Write sampled data to buffer 1 (if not full)
        //
        if(SS_bufferIsFull){
            SS_streamDataSkips++;
            return;
        }
        else{
            if(SS_currentStreamState != STREAM_IDLE){
                if(SS_currentStreamState == STREAM1){
                    SIGNALSIGHT_queueUpStreamData(1, true);
                }
                else if(SS_currentStreamState == STREAM2){
                    SIGNALSIGHT_queueUpStreamData(2, true);
                }
                else if(SS_currentStreamState == STREAM3){
                    SIGNALSIGHT_queueUpStreamData(3, true);
                }
                else if(SS_currentStreamState == STREAM4){
                    SIGNALSIGHT_queueUpStreamData(4, true);
                }
            }
        }
    }
    else{
        //
        // Write sampled data to buffer 2 (if not full)
        //
        if(SS_bufferIsFull2){
            SS_streamDataSkips2++;
            return;
        }
        else{
            if(SS_currentStreamState != STREAM_IDLE){
                if(SS_currentStreamState == STREAM1){
                    SIGNALSIGHT_queueUpStreamData(1, false);
                }
                else if(SS_currentStreamState == STREAM2){
                    SIGNALSIGHT_queueUpStreamData(2, false);
                }
                else if(SS_currentStreamState == STREAM3){
                    SIGNALSIGHT_queueUpStreamData(3, false);
                }
                else if(SS_currentStreamState == STREAM4){
                    SIGNALSIGHT_queueUpStreamData(4, false);
                }
            }
        }
    }

}

//*****************************************************************************
//
// Signal Sight Send Data function
//
// Call this function in your application where you want to send data to the 
// GUI via a communication peripheral. It is recommended to place this function
// call in a low priority location of your application, (in the main loop for 
// example) since it could be blocked waiting.
//
//*****************************************************************************
void SIGNALSIGHT_sendPlotData(){  
    //
    // Check SS_nextTxCMD - Sending ack takes priority over sending data
    //
    if(SS_nextTxCMD != NO_UPDATE){
        SIGNALSIGHT_sendAcknowledge(SS_nextTxCMD);
    }
    else if(SS_currentStreamState != STREAM_IDLE){
        if(SS_writeBuffer1readBuffer2){
            //
            // Read from buffer 2
            //
            if(SS_bufferIsFull2){
                //
                // Read current stream toggle states into temp array in case state change happens during transmission
                //
                bool SS_streamToggleStatesTemp[4] = {false, false, false, false};
                SS_streamToggleStatesTemp[0] = SS_streamToggleStates[0];
                SS_streamToggleStatesTemp[1] = SS_streamToggleStates[1];
                SS_streamToggleStatesTemp[2] = SS_streamToggleStates[2];
                SS_streamToggleStatesTemp[3] = SS_streamToggleStates[3];
                //
                // Loop through all buffer elements
                //
                if(SS_initiallyClearBuffer){
                    SIGNALSIGHT_sendAcknowledge(CLEAR_STREAMING_PLOT);
                    SS_initiallyClearBuffer = false;
                }
                uint16_t bufferElementIndex;
                for(bufferElementIndex = 0; bufferElementIndex < SS_BUFFER_SIZE; bufferElementIndex++){
                    //
                    // Send clear plot data command before sending first packet from buffer.
                    // Read in data from buffers at bufferElementIndex.
                    //
                    uint16_t insertIndex = 0;
                    float* dataArray = 0;
                    if(SS_streamToggleStatesTemp[0]){
                        //
                        // Channel A on
                        //
                        *(dataArray + insertIndex) = SS_streamA_buffer2[bufferElementIndex];
                        insertIndex++;
                    }
                    if(SS_streamToggleStatesTemp[1]){
                        //
                        // Channel B on
                        //
                        *(dataArray + insertIndex) = SS_streamB_buffer2[bufferElementIndex];
                        insertIndex++;
                    }
                    if(SS_streamToggleStatesTemp[2]){
                        //
                        // Channel C on
                        //
                        *(dataArray + insertIndex) = SS_streamC_buffer2[bufferElementIndex];
                        insertIndex++;
                    }
                    if(SS_streamToggleStatesTemp[3]){
                        //
                        // Channel D on
                        //
                        *(dataArray + insertIndex) = SS_streamD_buffer2[bufferElementIndex];
                        insertIndex++;
                    }
                    //
                    // Package and transmit data from buffers
                    //
                    if(insertIndex == 1){
                        char cmd = (char)STREAM_DATA_1_VAR;
                        EXPORTPACKAGE_sendFloatArrayAsByteArray(&cmd, 1, dataArray, 1);
                    }
                    else if(insertIndex == 2){
                        char cmd = (char)STREAM_DATA_2_VAR;
                        EXPORTPACKAGE_sendFloatArrayAsByteArray(&cmd, 1, dataArray, 2);
                    }
                    else if(insertIndex == 3){
                        char cmd = (char)STREAM_DATA_3_VAR;
                        EXPORTPACKAGE_sendFloatArrayAsByteArray(&cmd, 1, dataArray, 3);
                    }
                    else if(insertIndex == 4){
                        char cmd = (char)STREAM_DATA_4_VAR;
                        EXPORTPACKAGE_sendFloatArrayAsByteArray(&cmd, 1, dataArray, 4);
                    }
                }
                SS_bufferIsFull2 = false;
            }
            else{
                if(!SS_bufferIsFull2 && SS_bufferIsFull){
                    SS_writeBuffer1readBuffer2 = false;
                }
                return;
            }
        }
        else{
            //
            // Read from buffer 1
            //
            if(SS_bufferIsFull){
                //
                // Read current stream toggle states into temp array in case state change happens during transmission
                //
                bool SS_streamToggleStatesTemp[4] = {false, false, false, false};
                SS_streamToggleStatesTemp[0] = SS_streamToggleStates[0];
                SS_streamToggleStatesTemp[1] = SS_streamToggleStates[1];
                SS_streamToggleStatesTemp[2] = SS_streamToggleStates[2];
                SS_streamToggleStatesTemp[3] = SS_streamToggleStates[3];
                //
                // Loop through all buffer elements
                //
                if(SS_initiallyClearBuffer){
                    SIGNALSIGHT_sendAcknowledge(CLEAR_STREAMING_PLOT);
                    SS_initiallyClearBuffer = false;
                }
                uint16_t bufferElementIndex;
                for(bufferElementIndex = 0; bufferElementIndex < SS_BUFFER_SIZE; bufferElementIndex++){
                    //
                    // Send clear plot data command before sending first packet from buffer.
                    // Read in data from buffers at bufferElementIndex.
                    //
                    uint16_t insertIndex = 0;
                    float* dataArray = 0;
                    if(SS_streamToggleStatesTemp[0]){
                        //
                        // Channel A on
                        //
                        *(dataArray + insertIndex) = SS_streamA_buffer[bufferElementIndex];
                        insertIndex++;
                    }
                    if(SS_streamToggleStatesTemp[1]){
                        //
                        // Channel B on
                        //
                        *(dataArray + insertIndex) = SS_streamB_buffer[bufferElementIndex];
                        insertIndex++;
                    }
                    if(SS_streamToggleStatesTemp[2]){
                        //
                        // Channel C on
                        //
                        *(dataArray + insertIndex) = SS_streamC_buffer[bufferElementIndex];
                        insertIndex++;
                    }
                    if(SS_streamToggleStatesTemp[3]){
                        //
                        // Channel D on
                        //
                        *(dataArray + insertIndex) = SS_streamD_buffer[bufferElementIndex];
                        insertIndex++;
                    }
                    //
                    //Package and transmit data from buffers
                    //
                    if(insertIndex == 1){
                        char cmd = (char)STREAM_DATA_1_VAR;
                        EXPORTPACKAGE_sendFloatArrayAsByteArray(&cmd, 1, dataArray, 1);
                    }
                    else if(insertIndex == 2){
                        char cmd = (char)STREAM_DATA_2_VAR;
                        EXPORTPACKAGE_sendFloatArrayAsByteArray(&cmd, 1, dataArray, 2);
                    }
                    else if(insertIndex == 3){
                        char cmd = (char)STREAM_DATA_3_VAR;
                        EXPORTPACKAGE_sendFloatArrayAsByteArray(&cmd, 1, dataArray, 3);
                    }
                    else if(insertIndex == 4){
                        char cmd = (char)STREAM_DATA_4_VAR;
                        EXPORTPACKAGE_sendFloatArrayAsByteArray(&cmd, 1, dataArray, 4);
                    }
                }
                SS_bufferIsFull = false;
            }
            else{
                if(!SS_bufferIsFull && SS_bufferIsFull2){
                    SS_writeBuffer1readBuffer2 = true;
                }
                return;
            }
        }
    }
}

void EXPORTPACKAGE_takeActionOnMessage(uint16_t cmdByte, uint16_t* msgData, bool error){
    if(error){
        SS_nextTxCMD = PACKET_ERROR;
    }
    else{
        bool streamVarAdd = false;
        bool streamVarSubtract = false;
        //
        // Set up ack of CMD
        //
        SS_nextTxCMD = (enum SS_CMDS)cmdByte;
        //
        // Get CMD byte and perform action
        //
        if(cmdByte == INITCMD){
            //
            // Reinitialize all states
            //
            SS_currentPlotHashIndices[0] = SS_DEFAULT_STREAM_VAR_A;
            SS_currentPlotHashIndices[1] = SS_DEFAULT_STREAM_VAR_B;
            SS_currentPlotHashIndices[2] = SS_DEFAULT_STREAM_VAR_C;
            SS_currentPlotHashIndices[3] = SS_DEFAULT_STREAM_VAR_D;
            SS_currentStreamState = STREAM_IDLE;
            int i;
            for(i = 0; i < SS_NUMBER_OF_SIMULTANEOUS_STREAM_VARIABLES; i++){
                SS_streamToggleStates[i] = false;
            }
            SIGNALSIGHT_resetStates();
        }
        else if(cmdByte == INIT_STREAMING_VARS){  //read in variables to stream, only meaningful in streaming mode
            SS_currentPlotHashIndices[0] = (int16_t)msgData[0];
            SS_currentPlotHashIndices[1] = (int16_t)msgData[1];
            SS_currentPlotHashIndices[2] = (int16_t)msgData[2];
            SS_currentPlotHashIndices[3] = (int16_t)msgData[3];
        }
        else if(cmdByte == START_STREAMING_A){
            SS_streamToggleStates[0] = true;
            streamVarAdd = true;
        }
        else if(cmdByte == START_STREAMING_B){
            SS_streamToggleStates[1] = true;
            streamVarAdd = true;
        }
        else if(cmdByte == START_STREAMING_C){
            SS_streamToggleStates[2] = true;
            streamVarAdd = true;
        }
        else if(cmdByte == START_STREAMING_D){
            SS_streamToggleStates[3] = true;
            streamVarAdd = true;
        }
        else if(cmdByte == STOP_STREAMING_A){
            SS_streamToggleStates[0] = false;
            streamVarSubtract = true;
        }
        else if(cmdByte == STOP_STREAMING_B){
            SS_streamToggleStates[1] = false;
            streamVarSubtract = true;
        }
        else if(cmdByte == STOP_STREAMING_C){
            SS_streamToggleStates[2] = false;
            streamVarSubtract = true;
        }
        else if(cmdByte == STOP_STREAMING_D){
            SS_streamToggleStates[3] = false;
            streamVarSubtract = true;
        }
        else{
            SS_nextTxCMD = PACKET_ERROR;
        }

        //
        // Update SS_currentStreamState 
        //
        if(streamVarAdd){
            switch(SS_currentStreamState)
            {
                case STREAM_IDLE: SS_currentStreamState = STREAM1;
                             break;
                case STREAM1: SS_currentStreamState = STREAM2;
                             break;
                case STREAM2: SS_currentStreamState = STREAM3;
                              break;
                case STREAM3: SS_currentStreamState = STREAM4;
                              break;
                case STREAM4: SS_currentStreamState = STREAM4; SS_nextTxCMD = PACKET_ERROR;
                              break;
                default: SS_currentStreamState = STREAM_IDLE;
            }
        }
        else if(streamVarSubtract){
            switch(SS_currentStreamState)
            {
                case STREAM_IDLE: SS_currentStreamState = STREAM_IDLE; SS_nextTxCMD = PACKET_ERROR;
                             break;
                case STREAM1: SS_currentStreamState = STREAM_IDLE; SS_initiallyClearBuffer = true;
                             break;
                case STREAM2: SS_currentStreamState = STREAM1;
                              break;
                case STREAM3: SS_currentStreamState = STREAM2;
                              break;
                case STREAM4: SS_currentStreamState = STREAM3;
                              break;
                default: SS_currentStreamState = STREAM_IDLE;
            }
        }
    }
}
