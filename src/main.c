#include "config.h"
#include "audio.h"
#include "wav.h"
#include "base64.h"
#include "api.h"

#include <SDL3/SDL.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>

static struct termios orig_termios;
static int terminal_modified = 0;

static void restore_terminal(void)
{
    if (terminal_modified) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
        terminal_modified = 0;
    }
}

static void signal_handler(int sig)
{
    restore_terminal();
    _exit(128 + sig);
}

static void set_raw_mode(void)
{
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    terminal_modified = 1;
}

static int key_pressed(void)
{
    fd_set fds;
    struct timeval tv = {0, 0};
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0) {
        char c;
        if (read(STDIN_FILENO, &c, 1) == 1)
            return 1;
    }
    return 0;
}

static void display_volume_bar(float rms)
{
    const int width = 40;
    int filled = (int)(rms * width * 10);
    if (filled > width) filled = width;

    fprintf(stderr, "\r[");
    for (int i = 0; i < width; i++)
        fputc(i < filled ? '#' : '-', stderr);
    fprintf(stderr, "] %3d%%", (int)(rms * 100));
    fflush(stderr);
}

static bool copy_to_clipboard(const char *text)
{
    /* Try wl-copy first (Wayland), then xclip, then xsel */
    const char *cmds[] = {
        "wl-copy 2>/dev/null",
        "xclip -selection clipboard 2>/dev/null",
        "xsel --clipboard --input 2>/dev/null",
    };

    for (int i = 0; i < 3; i++) {
        FILE *p = popen(cmds[i], "w");
        if (!p) continue;
        size_t len = strlen(text);
        size_t written = fwrite(text, 1, len, p);
        int ret = pclose(p);
        if (ret == 0 && written == len)
            return true;
    }

    return false;
}

int main(void)
{
    Config cfg;
    if (!config_load(&cfg))
        return 1;

    if (!audio_init(cfg.sample_rate)) {
        config_free(&cfg);
        return 1;
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);

    AudioState audio;
    if (!audio_start(&audio)) {
        config_free(&cfg);
        SDL_Quit();
        curl_global_cleanup();
        return 1;
    }

    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(restore_terminal);
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    set_raw_mode();
    fprintf(stderr, "Recording... press ENTER to stop.\n");

    /* Recording loop */
    for (;;) {
        audio_poll(&audio);
        display_volume_bar(audio.rms);
        if (key_pressed()) break;
        SDL_Delay(50);
    }

    fprintf(stderr, "\n");
    restore_terminal();

    audio_stop(&audio);

    if (audio.count == 0) {
        fprintf(stderr, "Error: no audio recorded\n");
        audio_free(&audio);
        config_free(&cfg);
        SDL_Quit();
        curl_global_cleanup();
        return 1;
    }

    fprintf(stderr, "Recorded %zu samples (%.1f seconds)\n",
            audio.count, (double)audio.count / cfg.sample_rate);

    /* Encode WAV */
    unsigned char *wav_buf = NULL;
    size_t wav_len = 0;
    if (!wav_encode(audio.samples, audio.count, cfg.sample_rate, &wav_buf, &wav_len)) {
        fprintf(stderr, "Error: WAV encoding failed\n");
        audio_free(&audio);
        config_free(&cfg);
        SDL_Quit();
        curl_global_cleanup();
        return 1;
    }

    audio_free(&audio);

    /* Base64 encode */
    char *b64 = base64_encode(wav_buf, wav_len);
    free(wav_buf);
    if (!b64) {
        fprintf(stderr, "Error: base64 encoding failed\n");
        config_free(&cfg);
        SDL_Quit();
        curl_global_cleanup();
        return 1;
    }

    /* Send to API */
    fprintf(stderr, "Sending audio to API...\n");
    char *transcription = api_transcribe(cfg.api_key, cfg.model, cfg.prompt, b64);
    free(b64);

    if (!transcription) {
        config_free(&cfg);
        SDL_Quit();
        curl_global_cleanup();
        return 1;
    }

    /* Print transcription */
    printf("%s\n", transcription);

    /* Copy to clipboard */
    if (!copy_to_clipboard(transcription))
        fprintf(stderr, "Warning: failed to copy to clipboard (install wl-copy, xclip, or xsel)\n");
    else
        fprintf(stderr, "Transcription copied to clipboard.\n");

    free(transcription);
    config_free(&cfg);
    SDL_Quit();
    curl_global_cleanup();
    return 0;
}
