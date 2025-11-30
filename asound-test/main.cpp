#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <alsa/asoundlib.h>

#define PCM_DEVICE "default"

int main() {
    snd_pcm_t *pcm_handle;
    snd_pcm_hw_params_t *params;
    unsigned int rate = 44100;
    int err;

    /* Open PCM device for playback */
    if ((err = snd_pcm_open(&pcm_handle, PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        fprintf(stderr, "Error opening PCM device: %s\n", snd_strerror(err));
        return 1;
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

    /* Prepare buffer and generate PWM (SID-style) audio */
    int16_t buffer[4096];                         /* stereo interleaved frames */
    int frames_per_write = sizeof(buffer) / 4;    /* 4 bytes per frame: L+R */

    /* SID-like PWM parameters */
    double freq = 440.0;     /* A4 tone */
    double lfo_rate = 2.0;   /* 2 Hz duty modulation */
    double lfo_depth = 0.35; /* pulse modulation depth */
    double base_duty = 0.5;  /* 50% square wave */
    double amp = 16000.0;

    /* Phase accumulators */
    double phase = 0.0;
    double lfo_phase = 0.0;
    double phase_inc = freq / (double)rate;
    double lfo_inc  = lfo_rate / (double)rate;

    /* Playback duration */
    int total_writes = 40; /* around 4–5 seconds */

    for (int w = 0; w < total_writes; ++w) {

        for (int f = 0; f < frames_per_write; ++f) {

            /* LFO (0..1) */
            double lfo = (sin(2.0 * M_PI * lfo_phase) * 0.5) + 0.5;

            /* Modulated duty */
            double duty = base_duty + ((lfo - 0.5) * 2.0 * lfo_depth);
            if (duty < 0.01) duty = 0.01;
            if (duty > 0.99) duty = 0.99;

            /* Pulse wave: +1 or -1 */
            double sample = (phase < duty) ? 1.0 : -1.0;

            /* Soft shaping to reduce harshness */
            sample *= (1.0 - 0.2 * fabs(sample));

            int16_t out = (int16_t)(sample * amp);

            buffer[f * 2 + 0] = out; /* Left */
            buffer[f * 2 + 1] = out; /* Right */

            /* Advance phases */
            phase += phase_inc;
            if (phase >= 1.0) phase -= 1.0;

            lfo_phase += lfo_inc;
            if (lfo_phase >= 1.0) lfo_phase -= 1.0;
        }

        snd_pcm_sframes_t written = snd_pcm_writei(pcm_handle, buffer, frames_per_write);

        if (written == -EPIPE) {
            snd_pcm_prepare(pcm_handle);
        }
        else if (written < 0) {
            fprintf(stderr, "Write error: %s\n", snd_strerror((int)written));
        }
    }

    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);

    return 0;
}
