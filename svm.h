/*
*   SVM.h
*   Date: 5/5/2026
*   Author: VietNH
*/

#include <stdint.h>
#include <stdbool.h>

#ifndef _SVM_H_
#define _SVM_H_

#define MATH_ONE_HALF       0.5f
#define MATH_SQRT3          1.732050807568877f
#define US_MAX              250.0f
#define UDC_MAX             682.0f

typedef struct {
    float Udc;         //DC Voltage
    float Us_alpha;
    float Us_beta;
    float T_s;
    float Us_a;
    float Us_b;
    float Us_c;
    float d_0;
    float d_1;
    float d_2;
    float T_a;
    float T_b;
    float T_c;
    float temp_val1;
    float temp_val2;
    float inv_Udc;
    uint16_t sector;
}SVM_DATA;

//SVM initialize
static inline void SVM_INIT(SVM_DATA *x) {
    x->Udc = 0.0f;
    x->Us_alpha = 0.0f;
    x->Us_beta = 0.0f;
    x->T_s = 0.0f;
    x->Us_a = 0.0f;
    x->Us_b = 0.0f;
    x->Us_c = 0.0f;
    x->d_0 = 0.0f;
    x->d_1 = 0.0f;
    x->d_2 = 0.0f;
    x->T_a = 0.0f;
    x->T_b = 0.0f;
    x->T_c = 0.0f;
    x->temp_val1 = 0.0f;
    x->temp_val2 = 0.0f;
    x->sector = 0;
    x->inv_Udc = 0.0f;
}

//Modulation Index Calculation
static inline void SVM_EXEC(SVM_DATA *x) {

    x->temp_val1 = - MATH_ONE_HALF * x->Us_alpha;
    x->temp_val2 = (MATH_SQRT3 / 2) * x->Us_beta;

    //Inverse Clark Transform
    x->Us_a = x->Us_alpha;
    x->Us_b = x->temp_val1 + x->temp_val2;
    x->Us_c = x->temp_val1 - x->temp_val2;

    //Sector Identifier
    if (x->Us_a >= x->Us_b) {
        if (x->Us_b >= x-> Us_c) {
            x->sector = 1;
        }
        else if (x->Us_c >= x->Us_a) {
            x->sector = 5;
        }
        else {
            x->sector = 6;
        }
    }
    else {
        if (x->Us_b < x->Us_c) {
            x->sector = 4;
        }
        else if (x->Us_a > x->Us_c) {
            x->sector = 2;
        }
        else {
            x->sector = 3;
        }
    }
    x->inv_Udc = (US_MAX * 1.0 / UDC_MAX) * (1.0 / x->Udc);

    //Modulation Index Calculation

    switch (x->sector) {
        case 0:
        x->T_a = 0.0f;
        x->T_b = 0.0f;
        x->T_c = 0.0f;
        break;

        case 1:
        x->d_1 = ((MATH_ONE_HALF * 3) * x->Us_alpha - (MATH_SQRT3 / 2) * x->Us_beta) * x->inv_Udc;
        x->d_2 = MATH_SQRT3 * x->Us_beta * x->inv_Udc;
        x->d_0 = 1.0f - x->d_1 - x->d_2;
        x->T_a = x->d_0 / 2.0f + x->d_1 + x->d_2;
        x->T_b = x->d_0 / 2.0f + x->d_2;
        x->T_c = x->d_0 / 2.0f;
        break;

        case 2:
        x->d_1 = ((MATH_ONE_HALF * 3) * x->Us_alpha + (MATH_SQRT3 / 2) * x->Us_beta) * x->inv_Udc;
        x->d_2 = (-(MATH_ONE_HALF * 3) * x->Us_alpha + (MATH_SQRT3 / 2) * x->Us_beta) * x->inv_Udc;
        x->d_0 = 1.0f - x->d_1 - x->d_2;
        x->T_a = x->d_0 / 2.0f + x->d_1;
        x->T_b = x->d_0 / 2.0f + x->d_1 + x->d_2;
        x->T_c = x->d_0 / 2.0f;
        break;

        case 3:
        x->d_1 = MATH_SQRT3 * x->Us_beta * x->inv_Udc;
        x->d_2 = (- (MATH_ONE_HALF * 3) * x->Us_alpha - (MATH_SQRT3 / 2) * x->Us_beta) * x->inv_Udc ;
        x->d_0 = 1.0f - x->d_1 - x->d_2;
        x->T_a = x->d_0 / 2.0f;
        x->T_b = x->d_0 / 2.0f + x->d_1 + x->d_2;
        x->T_c = x->d_0 / 2.0f + x->d_2;
        break;

        case 4:
        x->d_1 = - MATH_SQRT3 * x->Us_beta * x->inv_Udc;
        x->d_2 = (- (MATH_ONE_HALF * 3) * x->Us_alpha + (MATH_SQRT3 / 2) * x->Us_beta) * x->inv_Udc;
        x->d_0 = 1.0f - x->d_1 - x->d_2;
        x->T_a = x->d_0 / 2.0f;
        x->T_b = x->d_0 / 2.0f + x->d_2;
        x->T_c = x->d_0 / 2.0f + x->d_2 + x->d_1;
        break;

        case 5:
        x->d_1 = (- (MATH_ONE_HALF * 3) * x->Us_alpha - (MATH_SQRT3 / 2) * x->Us_beta) * x->inv_Udc;
        x->d_2 = ((MATH_ONE_HALF * 3) * x->Us_alpha - (MATH_SQRT3 / 2) * x->Us_beta) * x->inv_Udc;
        x->d_0 = 1.0f - x->d_1 - x->d_2;
        x->T_a = x->d_0 / 2.0f + x->d_2;
        x->T_b = x->d_0 / 2.0f;
        x->T_c = x->d_0 / 2.0f + x->d_2 + x->d_1;
        break;

        case 6:
        x->d_1 = ((MATH_ONE_HALF * 3) * x->Us_alpha + (MATH_SQRT3 / 2) * x->Us_beta) * x->inv_Udc;
        x->d_2 = - MATH_SQRT3 * x->Us_beta * x->inv_Udc;
        x->d_0 = 1.0f - x->d_1 - x->d_2;
        x->T_a = x->d_0 / 2.0f + x->d_1 + x->d_2;
        x->T_b = x->d_0 / 2.0f;
        x->T_c = x->d_0 / 2.0f + x->d_2;
        break;
    }

/* Limit 0 < T_a T_b T_c < 1 */
if (x->T_a < 0.0f) x->T_a = 0.0f; else if (x->T_a > 0.96) x->T_a = 0.96f;
if (x->T_b < 0.0f) x->T_b = 0.0f; else if (x->T_b > 0.96) x->T_b = 0.96f;
if (x->T_c < 0.0f) x->T_c = 0.0f; else if (x->T_c > 0.96) x->T_c = 0.96f;

}

#endif /* _SVM_H_ */

