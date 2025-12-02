#ifndef AUDIOSYS_H
#define AUDIOSYS_H


#include <alsa/asoundlib.h>

#define PCM_DEVICE "default"


// prototypes
int initAudioHardware();
int processAudio();
int closeAudioHardware();

void *audioThread(void *arg);

#endif
