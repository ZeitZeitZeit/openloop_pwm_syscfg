/*
 *   enhanced_sopwm.h - Enhanced SOPWM Look-Up Table Declarations
 *   Source data: enhanced.xlsx
 */

#ifndef _ENHANCED_SOPWM_H_
#define _ENHANCED_SOPWM_H_

#include <stdint.h>

#define ENHANCED_SOPWM_M_COUNT   100

extern const float ENHANCED_SOPWM_LUT_N7[ENHANCED_SOPWM_M_COUNT][3];
extern const float ENHANCED_SOPWM_LUT_N9[ENHANCED_SOPWM_M_COUNT][4];
extern const float ENHANCED_SOPWM_LUT_N11[ENHANCED_SOPWM_M_COUNT][5];
extern const float ENHANCED_SOPWM_LUT_N13[ENHANCED_SOPWM_M_COUNT][6];
extern const float ENHANCED_SOPWM_LUT_N15[ENHANCED_SOPWM_M_COUNT][7];
extern const float ENHANCED_SOPWM_LUT_N21[ENHANCED_SOPWM_M_COUNT][10];

#endif /* _ENHANCED_SOPWM_H_ */
