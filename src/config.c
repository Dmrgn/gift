#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *strip(char *s)
{
    while (*s == ' ' || *s == '\t') s++;
    char *end = s + strlen(s) - 1;
    while (end > s && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r'))
        *end-- = '\0';
    return s;
}

static bool parse_file(const char *path, Config *cfg)
{
    FILE *f = fopen(path, "r");
    if (!f) return false;

    char line[4096];
    while (fgets(line, sizeof(line), f)) {
        char *s = strip(line);
        if (*s == '#' || *s == '\0') continue;

        char *eq = strchr(s, '=');
        if (!eq) continue;

        *eq = '\0';
        char *key = strip(s);
        char *val = strip(eq + 1);

        if (strcmp(key, "api_key") == 0)      { free(cfg->api_key); cfg->api_key = strdup(val); }
        else if (strcmp(key, "model") == 0)    { free(cfg->model);   cfg->model   = strdup(val); }
        else if (strcmp(key, "prompt") == 0)   { free(cfg->prompt);  cfg->prompt  = strdup(val); }
        else if (strcmp(key, "sample_rate") == 0) { cfg->sample_rate = atoi(val); }
    }

    fclose(f);
    return true;
}

bool config_load(Config *cfg)
{
    memset(cfg, 0, sizeof(*cfg));
    cfg->sample_rate = 44100;

    bool found = false;

    /* Try ./config.ini first */
    if (parse_file("config.ini", cfg)) {
        found = true;
    } else {
        /* Try ~/.config/gift/config.ini */
        const char *home = getenv("HOME");
        if (home) {
            char path[4096];
            snprintf(path, sizeof(path), "%s/.config/gift/config.ini", home);
            if (parse_file(path, cfg))
                found = true;
        }
    }

    if (!found) {
        fprintf(stderr, "Error: config.ini not found (tried ./config.ini and ~/.config/gift/config.ini)\n");
        return false;
    }

    if (!cfg->api_key || !cfg->model || !cfg->prompt) {
        fprintf(stderr, "Error: config.ini must contain api_key, model, and prompt\n");
        return false;
    }

    return true;
}

void config_free(Config *cfg)
{
    free(cfg->api_key);
    free(cfg->model);
    free(cfg->prompt);
    memset(cfg, 0, sizeof(*cfg));
}
