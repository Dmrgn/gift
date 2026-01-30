# gift

Speech-to-text CLI tool. Records from your microphone, sends audio to OpenRouter for transcription, prints the result, and copies it to your clipboard.

i.e an open source, linux targeted, and terminal based version of WisprFlow

## Dependencies

- SDL3, libcurl (if you want to build from source)
- wl-copy, xclip, or xsel (for copying to clipboard)

## Build & Install

```sh
make
sudo make install    # installs binary to /usr/local/bin, config to ~/.config/gift/
```

## Configuration

Create `~/.config/gift/config.ini` (or `./config.ini`):

```ini
api_key=sk-or-v1-xxxxx
model=mistralai/voxtral-small-24b-2507
prompt=Transcribe this audio accurately. Return only the transcription text, do not wrap the transcription in quotes.
sample_rate=44100
```

`api_key`, `model`, and `prompt` are required. `sample_rate` defaults to 44100.

## Usage

```
$ gift
Recording... press ENTER to stop.
[########--------------------------------] 20%
Sending audio to API...
Hello, this is a test.
Transcription copied to clipboard.
```

## Project Structure

```
src/
├── main.c          Entry point, recording loop, volume meter
├── config.c/.h     INI config parsing
├── audio.c/.h      SDL3 microphone capture
├── wav.c/.h        In-memory WAV encoding
├── base64.c/.h     Base64 encoder
├── api.c/.h        OpenRouter API (libcurl + cJSON)
└── vendor/
    └── cJSON.c/.h  Vendored JSON library
```
