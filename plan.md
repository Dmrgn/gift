# Plan: `gift` -- Speech-to-Text CLI App in C

## Overview

A command-line C application that records audio from the microphone, displays a real-time volume meter, sends the recording to OpenRouter for transcription via a multimodal API, prints the result, and copies it to the clipboard. No GUI, no CLI arguments -- configuration via an INI file.

## Project Structure

```
gift/
├── Makefile
├── .gitignore
├── config.ini.example
├── src/
│   ├── main.c           # Entry point, recording loop, terminal I/O, volume bar
│   ├── config.c / .h    # INI-style config parsing
│   ├── audio.c / .h     # SDL3 microphone capture + RMS volume
│   ├── wav.c / .h       # In-memory PCM WAV encoding (44-byte header)
│   ├── base64.c / .h    # Base64 encoder (~40 lines)
│   ├── api.c / .h       # libcurl POST to OpenRouter + cJSON response parsing
│   └── vendor/
│       ├── cJSON.c      # Vendored from DaveGamble/cJSON (MIT)
│       └── cJSON.h
```

## Dependencies

- **SDL3** -- audio capture (pkg-config: `sdl3`)
- **libcurl** -- HTTP POST (pkg-config: `libcurl`)
- **cJSON** -- vendored, no system install needed
- **math** (`-lm`) -- `sqrt()` for RMS

## Config File (`config.ini`)

Simple `key=value` format, no extra dependency needed. Lines starting with `#` are comments.

```ini
api_key=sk-or-v1-xxxxx
model=google/gemini-2.5-flash
prompt=Transcribe this audio accurately. Return only the transcription text.
sample_rate=44100
```

Search order: `./config.ini` then `~/.config/gift/config.ini`.

Required keys: `api_key`, `model`, `prompt`. Optional: `sample_rate` (default 44100).

## Program Flow

1. **Load config** from INI file
2. **Init SDL3** (`SDL_INIT_AUDIO`)
3. **Init libcurl** (`curl_global_init`)
4. **Open microphone** via `SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_RECORDING, ...)` with S16LE mono
5. **Set terminal to raw mode** (disable canonical/echo via `termios`, `VMIN=0 VTIME=0`)
6. **Print** `"Recording... press ENTER to stop."`
7. **Recording loop** (~20fps, `SDL_Delay(50)`):
   - `audio_poll()` -- read available samples from SDL stream into growing buffer, compute RMS
   - `display_volume_bar()` -- render `[####----] 20%` on stderr with `\r`
   - `input_check_stop()` -- `select()` on stdin with zero timeout; break on any keypress
8. **Restore terminal**, stop audio capture, flush remaining samples
9. **Encode WAV** in memory (44-byte header + raw PCM)
10. **Base64-encode** the WAV buffer
11. **Send to OpenRouter** via libcurl POST with JSON body:
    ```json
    {
      "model": "<from config>",
      "messages": [{
        "role": "user",
        "content": [
          {"type": "text", "text": "<prompt from config>"},
          {"type": "input_audio", "input_audio": {"data": "<base64>", "format": "wav"}}
        ]
      }]
    }
    ```
12. **Parse response** -- extract `choices[0].message.content`, or report `error.message`
13. **Print transcription** to stdout
14. **Copy to clipboard** via `wl-copy` / `xclip` / `xsel` (tried in order via `popen`)
15. **Cleanup and exit**

## Key Implementation Details

### Volume Meter (in `main.c`)
```
[########--------------------------------] 20%
```
- 40-char wide bar on stderr, updated with `\r` + `fflush`
- RMS computed from latest audio chunk, normalized to 0.0-1.0

### Stop Recording
- `termios` raw mode on stdin (no echo, non-canonical)
- `select()` with zero timeout each loop iteration
- `SIGINT`/`SIGTERM` handlers + `atexit()` ensure terminal is always restored

### WAV Encoding (`wav.c`)
- 44-byte RIFF/WAVE header followed by raw PCM samples
- Little-endian byte writes via helper functions
- S16LE mono, sample rate from config

### API Module (`api.c`)
- Build JSON with cJSON, send via `curl_easy_perform`
- Headers: `Content-Type: application/json`, `Authorization: Bearer <key>`
- Parse response with cJSON, check for `error` object first
- Extract `choices[0].message.content` as transcription text

### Error Handling
- All module functions return `bool`, errors printed to stderr
- SDL errors via `SDL_GetError()`, curl via `curl_easy_strerror()`
- Terminal always restored via signal handlers + `atexit()`

## Files to Create (in order)

1. `Makefile` -- build system with pkg-config for SDL3 and libcurl
2. `.gitignore` -- ignore `build/`, `gift`, `config.ini`, `*.o`
3. `config.ini.example` -- example config
4. `src/vendor/cJSON.c` + `cJSON.h` -- vendor from GitHub
5. `src/config.c` + `src/config.h` -- config parsing
6. `src/base64.c` + `src/base64.h` -- base64 encoder
7. `src/wav.c` + `src/wav.h` -- WAV encoding
8. `src/audio.c` + `src/audio.h` -- SDL3 audio capture
9. `src/api.c` + `src/api.h` -- HTTP + JSON
10. `src/main.c` -- wire everything together

## Verification

1. `make` -- should compile without errors or warnings
2. Create `config.ini` with a valid OpenRouter API key and model that supports audio input
3. Run `./gift` -- should show "Recording... press ENTER to stop." with a live volume bar
4. Speak into the mic, press Enter
5. Should print "Sending audio to API..." then the transcription text
6. Transcription should appear in system clipboard (Ctrl+V to verify)
7. Test error cases: missing config file, invalid API key, empty recording
