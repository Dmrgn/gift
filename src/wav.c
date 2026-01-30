#include "wav.h"
#include <stdlib.h>
#include <string.h>

static void write_u16(unsigned char *p, uint16_t v)
{
    p[0] = (unsigned char)(v & 0xFF);
    p[1] = (unsigned char)(v >> 8);
}

static void write_u32(unsigned char *p, uint32_t v)
{
    p[0] = (unsigned char)(v & 0xFF);
    p[1] = (unsigned char)((v >> 8) & 0xFF);
    p[2] = (unsigned char)((v >> 16) & 0xFF);
    p[3] = (unsigned char)(v >> 24);
}

bool wav_encode(const int16_t *samples, size_t num_samples,
                int sample_rate, unsigned char **out, size_t *out_len)
{
    uint32_t data_size = (uint32_t)(num_samples * 2); /* 16-bit = 2 bytes per sample */
    uint32_t file_size = 44 + data_size;

    unsigned char *buf = malloc(file_size);
    if (!buf) return false;

    /* RIFF header */
    memcpy(buf, "RIFF", 4);
    write_u32(buf + 4, file_size - 8);
    memcpy(buf + 8, "WAVE", 4);

    /* fmt  sub-chunk */
    memcpy(buf + 12, "fmt ", 4);
    write_u32(buf + 16, 16);                           /* sub-chunk size */
    write_u16(buf + 20, 1);                             /* PCM format */
    write_u16(buf + 22, 1);                             /* mono */
    write_u32(buf + 24, (uint32_t)sample_rate);
    write_u32(buf + 28, (uint32_t)(sample_rate * 2));   /* byte rate */
    write_u16(buf + 32, 2);                             /* block align */
    write_u16(buf + 34, 16);                            /* bits per sample */

    /* data sub-chunk */
    memcpy(buf + 36, "data", 4);
    write_u32(buf + 40, data_size);
    memcpy(buf + 44, samples, data_size);

    *out = buf;
    *out_len = file_size;
    return true;
}
