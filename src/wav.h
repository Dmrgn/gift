#ifndef WAV_H
#define WAV_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/*
 * Encode raw S16LE mono PCM samples into a WAV buffer.
 * Sets *out and *out_len on success. Caller must free *out.
 */
bool wav_encode(const int16_t *samples, size_t num_samples,
                int sample_rate, unsigned char **out, size_t *out_len);

#endif
