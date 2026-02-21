#ifndef PLAYMED_H
#define PLAYMED_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif



int  playMED_Load(const char *filename, uint32_t outSampleRate);
void playMED_Free(void);

const char *playMED_LastError(void);

// Render interleaved stereo S16_LE, returns frames written.
uint32_t RenderMED_Interleaved(int16_t *out, uint32_t frames);


#ifdef __cplusplus
}
#endif

#endif // PLAYMED_H
