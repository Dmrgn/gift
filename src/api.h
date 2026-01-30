#ifndef API_H
#define API_H

#include <stdbool.h>

/*
 * Send base64-encoded WAV audio to OpenRouter for transcription.
 * Returns malloc'd transcription string on success. Caller must free.
 * Returns NULL on error (message printed to stderr).
 */
char *api_transcribe(const char *api_key, const char *model,
                     const char *prompt, const char *audio_b64);

#endif
