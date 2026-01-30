#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    int16_t *samples;
    size_t   count;
    size_t   capacity;
    float    rms;           /* 0.0 - 1.0, updated each poll */
    void    *stream;        /* SDL_AudioStream* */
} AudioState;

bool  audio_init(int sample_rate);
bool  audio_start(AudioState *a);
void  audio_poll(AudioState *a);
void  audio_stop(AudioState *a);
void  audio_free(AudioState *a);

#endif
