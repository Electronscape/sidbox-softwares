#ifndef YM2612_H
#define YM2612_H
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/* envelope generator */
#define ENV_BITS    10
#define ENV_LEN      (1<<ENV_BITS)
#define ENV_STEP    (128.0/ENV_LEN)

#define MAX_ATT_INDEX  (ENV_LEN-1) /* 1023 */
#define MIN_ATT_INDEX  (0)      /* 0 */

#define EG_ATT      4
#define EG_DEC      3
#define EG_SUS      2
#define EG_REL      1
#define EG_OFF      0

/* phase generator (detune mask) */
#define DT_BITS     17
#define DT_LEN      (1 << DT_BITS)
#define DT_MASK     (DT_LEN - 1)

/* operator unit */
#define SIN_BITS    10
#define SIN_LEN      (1<<SIN_BITS)
#define SIN_MASK    (SIN_LEN-1)

#define TL_RES_LEN    (256) /* 8 bits addressing (real chip) */

#define TL_BITS    12 /* channel output */

/*  TL_TAB_LEN is calculated as:
*   13 - sinus amplitude bits     (Y axis)
*   2  - sinus sign bit           (Y axis)
*   TL_RES_LEN - sinus resolution (X axis)
*/
#define TL_TAB_LEN (13*2*TL_RES_LEN)
#define ENV_QUIET    (TL_TAB_LEN>>3)

#define SC(db) (UINT32) ( db * (4.0/ENV_STEP) )
#define RATE_STEPS (8)















enum {
    YM2612_DISCRETE = 0,
    YM2612_INTEGRATED,
    YM2612_ENHANCED
};



void YM2612Init(void);
void YM2612Config(int type);
void YM2612ResetChip(void);
void YM2612Write(unsigned int a, unsigned int v);

long YM2612Update(int32_t *buffer, int length);











#ifdef __cplusplus
}
#endif
#endif // YM2612_H
