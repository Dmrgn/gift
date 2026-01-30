#include "base64.h"
#include <stdlib.h>

static const char table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char *base64_encode(const unsigned char *data, size_t len)
{
    size_t out_len = 4 * ((len + 2) / 3);
    char *out = malloc(out_len + 1);
    if (!out) return NULL;

    size_t i, j;
    for (i = 0, j = 0; i + 2 < len; i += 3) {
        unsigned int v = ((unsigned int)data[i] << 16) |
                         ((unsigned int)data[i+1] << 8) |
                          (unsigned int)data[i+2];
        out[j++] = table[(v >> 18) & 0x3F];
        out[j++] = table[(v >> 12) & 0x3F];
        out[j++] = table[(v >>  6) & 0x3F];
        out[j++] = table[ v        & 0x3F];
    }

    if (i < len) {
        unsigned int v = (unsigned int)data[i] << 16;
        if (i + 1 < len) v |= (unsigned int)data[i+1] << 8;
        out[j++] = table[(v >> 18) & 0x3F];
        out[j++] = table[(v >> 12) & 0x3F];
        out[j++] = (i + 1 < len) ? table[(v >> 6) & 0x3F] : '=';
        out[j++] = '=';
    }

    out[j] = '\0';
    return out;
}
