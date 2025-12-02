#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "audiosys.h"

#include <QThread>

snd_pcm_t *pcm_handle;


#define AUDIO_BUFFER_SIZE           1024   // frames

int16_t audiobuf[AUDIO_BUFFER_SIZE * 2];                         /* stereo interleaved frames */
//int frames_per_write = sizeof(audiobuf) / 4;    /* 4 bytes per frame: L+R */

volatile unsigned int writePos = 0;
volatile unsigned int readPos = 0;

volatile bool audioThreadRunning = true;


pthread_t audioThreadHandle;

int initAudioHardware(){
    snd_pcm_hw_params_t *params;
    unsigned int rate = 44100;
    int err;

    /* Open PCM device for playback */
    if ((err = snd_pcm_open(&pcm_handle, PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        fprintf(stderr, "Error opening PCM device: %s\n", snd_strerror(err));
        return 1; // WELL Crap!
    }

    /* Set hardware parameters */
    snd_pcm_hw_params_malloc(&params);
    snd_pcm_hw_params_any(pcm_handle, params);
    snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm_handle, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(pcm_handle, params, 2);
    snd_pcm_hw_params_set_rate_near(pcm_handle, params, &rate, 0);
    snd_pcm_hw_params(pcm_handle, params);
    snd_pcm_hw_params_free(params);


    pthread_create(&audioThreadHandle, NULL, audioThread, NULL);

    return 0; // it came back with zero issues
}



void addToAudio(int16_t l, int16_t r) {
    unsigned int next = (writePos + 1) % AUDIO_BUFFER_SIZE;

    // prevent overwrite if buffer full
    if (next == readPos) return;

    audiobuf[writePos*2 + 0] = l;
    audiobuf[writePos*2 + 1] = r;

    writePos = next;
}



void *audioThread(void *arg) {
    const int chunk = 64; // frames per write
    int16_t tmp[chunk * 2];

    while (audioThreadRunning) {
        // Only write if enough data available
        int available = (writePos >= readPos)
                            ? (writePos - readPos)
                            : (AUDIO_BUFFER_SIZE - readPos + writePos);

        if (available >= chunk) {
            // Copy from ring buffer
            for (int i = 0; i < chunk; i++) {
                tmp[i*2+0] = audiobuf[readPos*2+0];
                tmp[i*2+1] = audiobuf[readPos*2+1];
                readPos = (readPos + 1) % AUDIO_BUFFER_SIZE;
            }

            snd_pcm_sframes_t written = snd_pcm_writei(pcm_handle, tmp, chunk);
            //printf("Taud-out: %ld\n", written);
            if (written == -EPIPE) {
                //printf("XRUN!\n");
                snd_pcm_prepare(pcm_handle);
            }
            else if (written < 0) {
                printf("ALSA write error: %s\n", snd_strerror(written));
            }

        } else {
            // Too little audio → avoid underrun
            usleep(50);
        }
    }
    return 0;
}


int closeAudioHardware(){
    printf("closing audio\n");
    audioThreadRunning = false;
    pthread_join(audioThreadHandle, NULL);  // wait until it’s done

    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);



    return 0;
}



static double phase = 0.0;

int processAudio()
{
    int fps = 60;
    const int sampleRate = 44100;
    const int freq = 440;
    const double amp = 1000.0;

    int samples_per_frame = sampleRate / fps;
    double phaseInc = 2.0 * M_PI * freq / (double)sampleRate;

    for (int i = 0; i < samples_per_frame; ++i) {
        int16_t sample = (int16_t)(sin(phase) * amp);
        addToAudio(sample, sample); // push to ring buffer
        phase += phaseInc;
        if (phase >= 2.0 * M_PI) phase -= 2.0 * M_PI;
    }

    return 0;
}

#if 0
int processAudio(){

    audiobuf[1 * 2 + 0] = rand() * 16384; /* Left */
    audiobuf[1 * 2 + 1] = 255; /* Right */
    snd_pcm_sframes_t written = snd_pcm_writei(pcm_handle, audiobuf, frames_per_write);
    printf("aud-out: %ld\n", written);

    return 0;   // everything went well this frame.
}
#endif
