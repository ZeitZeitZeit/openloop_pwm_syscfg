/*
 *   SOPWM.h - Selective Optimal Pulse Width Modulation
 *   Date: 5/6/2026
 *   Author: VietNH
 *
 *   Look-Up Tables for SOPWM angles (in degrees)
 *   Input:  m - modulation index (0.01 to 1.00, step 0.01)
 *           N - number of angles * 2 + 1 (7, 9, 11, 13, or 15)
 *   Output: switching angles (degrees)
 */

#ifndef _SOPWM_H_
#define _SOPWM_H_

#include <stdint.h>

/* Number of modulation index entries (m = 0.01 to 1.00) */
#define SOPWM_M_COUNT   100

/* LUT declarations (defined in sopwm.c) */
extern const float SOPWM_LUT_N7[SOPWM_M_COUNT][3];
extern const float SOPWM_LUT_N9[SOPWM_M_COUNT][4];
extern const float SOPWM_LUT_N11[SOPWM_M_COUNT][5];
extern const float SOPWM_LUT_N13[SOPWM_M_COUNT][6];
extern const float SOPWM_LUT_N15[SOPWM_M_COUNT][7];

/*
 * SOPWM_GetAngles - Look up switching angles from the LUT
 * Parameters:
 *   m       - modulation index (0.01 to 1.00)
 *   N       - pulse number (7, 9, 11, 13, or 15)
 *   angles  - output array to store the angles (must be large enough)
 * Returns:
 *   Number of angles written, or 0 on invalid input
 */
static inline uint16_t SOPWM_GetAngles(float m, uint16_t N, float *angles) 
{
    uint16_t idx;
    uint16_t num_angles;
    uint16_t i;

    /* Validate modulation index */
    if (m < 0.01f || m > 1.00f) return 0;

    /* Convert m to LUT index (0-based): m=0.01 -> index 0, m=1.00 -> index 99 */
    idx = (uint16_t)((m * 100.0f) - 1.0f + 0.5f);
    if (idx >= SOPWM_M_COUNT) idx = SOPWM_M_COUNT - 1;

    switch (N) {
        case 7:
            num_angles = 3;
            for (i = 0; i < num_angles; i++)
                angles[i] = SOPWM_LUT_N7[idx][i];
            break;
        case 9:
            num_angles = 4;
            for (i = 0; i < num_angles; i++)
                angles[i] = SOPWM_LUT_N9[idx][i];
            break;
        case 11:
            num_angles = 5;
            for (i = 0; i < num_angles; i++)
                angles[i] = SOPWM_LUT_N11[idx][i];
            break;
        case 13:
            num_angles = 6;
            for (i = 0; i < num_angles; i++)
                angles[i] = SOPWM_LUT_N13[idx][i];
            break;
        case 15:
            num_angles = 7;
            for (i = 0; i < num_angles; i++)
                angles[i] = SOPWM_LUT_N15[idx][i];
            break;
        default:
            return 0;
    }

    return num_angles;
}

#endif /* _SOPWM_H_ */
