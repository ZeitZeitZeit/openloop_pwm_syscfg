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

// ============================================================================
// Open-Loop SOPWM API
// ============================================================================

/*
 * SOPWM_openLoopInit - Initialise the open-loop SOPWM module.
 *   Udc       - DC link voltage (V)
 *   magnitude - peak reference voltage amplitude (V)
 *   N         - preset pulse number: 7, 9, 11, 13, or 15
 *   sampleRes - ISR samples per fundamental cycle (max 200)
 */
void SOPWM_openLoopInit(float Udc, float magnitude, uint16_t N, int sampleRes);

/*
 * SOPWM_openLoopRun - Call once per PWM ISR.
 * Computes m from Ualpha/Ubeta, looks up switching angles for preset N,
 * and writes the angle array and count to the provided output pointers.
 * Results are also mirrored in the observable module variables below.
 */
void SOPWM_openLoopRun(float *angles, uint16_t *num_angles);

// Observable variables (individually named so SignalSight can address each one)
extern float    sopwm_m;
extern float    sopwm_theta;
extern float    sopwm_angle1;
extern float    sopwm_angle2;
extern float    sopwm_angle3;
extern float    sopwm_angle4;
extern float    sopwm_angle5;
extern float    sopwm_angle6;
extern float    sopwm_angle7;
extern uint16_t sopwm_num_angles;

// ============================================================================
// DMA-Driven Schedule Table API
// ============================================================================

#define SOPWM_N_CARR          50U      /* carrier cycles per fundamental        */
#define SOPWM_CMP_OFF         0xFFFFU  /* disabled CMP value — never fires      */
#define SOPWM_MID_SLOT_A      25U      /* 180° half-wave toggle (0° + 180°)     */
#define SOPWM_PHASE_B_OFFSET  33U      /* B[k]=A[(k+33)%50]  lags A by 120°    */
#define SOPWM_PHASE_C_OFFSET  17U      /* C[k]=A[(k+17)%50]  lags A by 240°    */

/*
 * One carrier-cycle compare pair.  Packed into the DMA source table.
 *   cmpa / cmpb : counter counts at which the AQ toggles EPWMxA.
 *   Set to SOPWM_CMP_OFF for cycles that carry no switching event.
 */
typedef struct {
    uint16_t cmpa;
    uint16_t cmpb;
} SOPWM_CycleCmp_t;

/*
 * Double-buffered schedule tables — [phase][buffer][cycle]
 *   phase  : 0=A (ePWM1), 1=B (ePWM4), 2=C (ePWM2)
 *   buffer : 0 or 1 (double-buffer)
 *   cycle  : 0..SOPWM_N_CARR-1
 *
 * Phase C may need EPWM dead-band RED polarity Active Low (invert HS) so the
 * rotated waveform matches A+240 deg — see phase_pulse_sim.py / SysConfig myEPWM2.
 * DMA source address for phase A = &sopwm_sched[0][sopwm_sched_active][0]
 * Allocated in DMA-accessible RAM (see #pragma DATA_SECTION in sopwm.c).
 */
extern SOPWM_CycleCmp_t  sopwm_sched[3][2][SOPWM_N_CARR];

/* Buffer index currently being consumed by DMA (0 or 1) */
extern volatile uint16_t sopwm_sched_active;

/*
 * Set by SOPWM_schedISR at the start of every fundamental cycle.
 * Clear in the main loop after calling SOPWM_BuildSchedule().
 */
extern volatile uint16_t sopwm_fund_tick;

/* Debug observables — add these to CCS Watch Expressions */
extern volatile uint16_t sopwm_init_done;   /* expect 1 after init           */
extern volatile uint16_t sopwm_lut_n_base;  /* expect 3 for N=7              */
extern volatile uint16_t sopwm_build_count; /* expect 2 after InitSchedule   */
extern          uint16_t sopwm_cycle_idx;   /* current carrier cycle 0..49   */

/*
 * SOPWM_InitSchedule
 *
 * One-time startup call: populates BOTH double-buffers with the same
 * m / N / tbprd so the DMA has valid data from the very first carrier
 * cycle.  Sets sopwm_sched_active = 0 and leaves sopwm_sched_next = 1
 * ready for the first main-loop rebuild.  Do NOT call CommitSchedule
 * after this — no swap is needed.
 *
 *   m      - initial modulation index [0.01, 1.00]
 *   N      - pulse number (7, 9, 11, 13 or 15)
 *   tbprd  - TBPRD value of the ePWM modules (counts per carrier cycle)
 */
void SOPWM_InitSchedule(float m, uint16_t N, uint16_t tbprd);

/*
 * SOPWM_BuildSchedule
 *
 * Compute the 50-slot CMPA/CMPB schedule for all three phases from the
 * current modulation index and pulse number.  Writes into the INACTIVE
 * buffer.  Call from the main loop whenever sopwm_fund_tick is set.
 * Follow with SOPWM_CommitSchedule() to arm the new buffer for DMA swap.
 *
 *   m      - modulation index [0.01, 1.00]
 *   N      - pulse number (7, 9, 11, 13 or 15)
 *   tbprd  - TBPRD value of the ePWM modules (counts per carrier cycle)
 */
void SOPWM_BuildSchedule(float m, uint16_t N, uint16_t tbprd);

/*
 * SOPWM_CommitSchedule
 *
 * Mark the inactive buffer as ready.  The next SOPWM_schedISR fundamental
 * boundary will atomically promote it to active so DMA picks it up.
 */
void SOPWM_CommitSchedule(void);

/*
 * SOPWM_schedISR
 *
 * Call once from the ePWM1 ZERO interrupt (50 kHz).
 * Tracks the carrier-cycle index, swaps the double-buffer at each
 * fundamental boundary, and sets sopwm_fund_tick.
 * Does NOT write to CMPA/CMPB — DMA handles register loading.
 */
void SOPWM_schedISR(void);

#endif /* _SOPWM_H_ */
