#include "audio.h"
#include <SDL3/SDL.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

bool audio_init(int sample_rate)
{
    (void)sample_rate;
    if (!SDL_Init(SDL_INIT_AUDIO)) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }
    return true;
}

bool audio_start(AudioState *a)
{
    memset(a, 0, sizeof(*a));

    SDL_AudioSpec spec;
    spec.freq     = 44100;
    spec.format   = SDL_AUDIO_S16LE;
    spec.channels = 1;

    a->stream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_RECORDING, &spec, NULL, NULL);
    if (!a->stream) {
        fprintf(stderr, "Failed to open microphone: %s\n", SDL_GetError());
        return false;
    }

    if (!SDL_ResumeAudioStreamDevice((SDL_AudioStream *)a->stream)) {
        fprintf(stderr, "Failed to start recording: %s\n", SDL_GetError());
        return false;
    }

    a->capacity = 44100 * 10; /* pre-alloc 10 seconds */
    a->samples  = malloc(a->capacity * sizeof(int16_t));
    if (!a->samples) {
        fprintf(stderr, "Out of memory\n");
        return false;
    }
    a->count = 0;
    a->rms   = 0.0f;
    return true;
}

void audio_poll(AudioState *a)
{
    int16_t buf[4096];
    int avail = SDL_GetAudioStreamAvailable((SDL_AudioStream *)a->stream);
    if (avail <= 0) return;

    int to_read = avail;
    if (to_read > (int)sizeof(buf)) to_read = (int)sizeof(buf);

    int got = SDL_GetAudioStreamData((SDL_AudioStream *)a->stream, buf, to_read);
    if (got <= 0) return;

    int n = got / 2; /* number of int16 samples */

    /* Grow buffer if needed */
    while (a->count + (size_t)n > a->capacity) {
        a->capacity *= 2;
        int16_t *tmp = realloc(a->samples, a->capacity * sizeof(int16_t));
        if (!tmp) return;
        a->samples = tmp;
    }

    memcpy(a->samples + a->count, buf, (size_t)got);
    a->count += (size_t)n;

    /* Compute RMS of this chunk */
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        double s = buf[i] / 32768.0;
        sum += s * s;
    }
    float rms = (float)sqrt(sum / n);
    if (rms > 1.0f) rms = 1.0f;
    a->rms = rms;
}

void audio_stop(AudioState *a)
{
    if (a->stream) {
        SDL_PauseAudioStreamDevice((SDL_AudioStream *)a->stream);
        /* Flush remaining samples */
        audio_poll(a);
    }
}

void audio_free(AudioState *a)
{
    if (a->stream) {
        SDL_DestroyAudioStream((SDL_AudioStream *)a->stream);
        a->stream = NULL;
    }
    free(a->samples);
    a->samples = NULL;
    a->count = 0;
}
