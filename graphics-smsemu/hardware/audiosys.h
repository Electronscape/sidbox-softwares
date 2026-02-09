#ifndef AUDIOSYS_H
#define AUDIOSYS_H
#ifdef __cplusplus
extern "C" {
#endif


// will need to install asoundlib-dev
/*

// will need the following lib
sudo dnf install alsa-lib-devel


*/

#include <alsa/asoundlib.h>

#define PCM_DEVICE "default"

#define FPS     60
#define SAMPLE_FREQ     44100
#define SAMPLES_PER_FRAME   (SAMPLE_FREQ / FPS)

// prototypes
int initAudioHardware();
int processAudio();
int closeAudioHardware();
void playFrameAudio(int16_t *snd, unsigned long SPF);


void *audioThread(void *arg);



#ifdef __cplusplus
}
#endif

#endif
