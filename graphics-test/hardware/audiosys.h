#ifndef AUDIOSYS_H
#define AUDIOSYS_H


#include <alsa/asoundlib.h>

#define PCM_DEVICE "default"

#define FPS     60
#define SAMPLE_FREQ     44100
#define SAMPLES_PER_FRAME   (SAMPLE_FREQ / FPS)

// prototypes
int initAudioHardware();
int processAudio();
int closeAudioHardware();

void *audioThread(void *arg);

#endif
