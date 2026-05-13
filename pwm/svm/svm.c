//#############################################################################
//
// FILE:   svm.c
//
// TITLE:  Space Vector Modulation (SVM) Module — Open-Loop Wrapper
//
// Contains the SVM instance, pre-computed lookup tables, SignalSight
// plot variables, and the open-loop init/run wrapper functions.
//
//#############################################################################

#include "svm.h"
#include <math.h>

//
// Defines
//
#define MATH_TWO_PI         6.283185307f
#define SVM_MAX_SAMPLES     200

//
// Module-level state
//
static SVM_DATA mySvm;
static int   dataIndex = 0;
static int   sampleResolution = SVM_MAX_SAMPLES;
static float alphaSnapshot[SVM_MAX_SAMPLES] = {0};
static float betaSnapshot[SVM_MAX_SAMPLES]  = {0};

//
// SignalSight plot variables (extern-ed by signalsight_hash.c)
//
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
// SVM_openLoopInit
//
// Pre-computes one full electrical cycle of alpha/beta values into
// lookup tables and initializes the SVM data structure.
//
void SVM_openLoopInit(float Udc, float Ts, float magnitude, int sampleRes)
{
    int i;
    float angle;

    if(sampleRes > SVM_MAX_SAMPLES)
    {
        sampleRes = SVM_MAX_SAMPLES;
    }
    sampleResolution = sampleRes;
    dataIndex = 0;

    SVM_INIT(&mySvm);
    mySvm.Udc = Udc;
    mySvm.T_s = Ts;

    for(i = 0; i < sampleResolution; i++)
    {
        angle = ((float)i) * MATH_TWO_PI / ((float)sampleResolution);
        alphaSnapshot[i] = magnitude * cosf(angle);
        betaSnapshot[i]  = magnitude * sinf(angle);
    }
}

//
// SVM_openLoopRun
//
// Called once per PWM cycle from the ISR. Steps through the lookup
// table, runs the SVM algorithm, updates the SignalSight plot
// variables, and returns the three duty cycles via pointers.
//
void SVM_openLoopRun(float *dutyA, float *dutyB, float *dutyC)
{
    //
    // Read pre-computed alpha/beta from lookup table
    //
    svm_Ualpha = alphaSnapshot[dataIndex];
    svm_Ubeta  = betaSnapshot[dataIndex];

    mySvm.Us_alpha = svm_Ualpha;
    mySvm.Us_beta  = svm_Ubeta;

    if(dataIndex >= sampleResolution - 1)
    {
        dataIndex = 0;
    }
    else
    {
        dataIndex++;
    }

    //
    // Run the SVM math
    //
    SVM_EXEC(&mySvm);

    //
    // Update plot variables
    //
    svm_Ta = mySvm.T_a;
    svm_Tb = mySvm.T_b;
    svm_Tc = mySvm.T_c;
    svm_d1 = mySvm.d_1;
    svm_d2 = mySvm.d_2;
    svm_d0 = mySvm.d_0;
    svm_Sector = mySvm.sector;

    //
    // Return duty cycles to caller
    //
    *dutyA = mySvm.T_a;
    *dutyB = mySvm.T_b;
    *dutyC = mySvm.T_c;
}
