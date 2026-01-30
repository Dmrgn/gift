#ifndef BASE64_H
#define BASE64_H

#include <stddef.h>

/* Returns malloc'd base64 string. Caller must free. */
char *base64_encode(const unsigned char *data, size_t len);

#endif
