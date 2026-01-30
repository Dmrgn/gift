#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

typedef struct {
    char *api_key;
    char *model;
    char *prompt;
    int   sample_rate;
} Config;

bool config_load(Config *cfg);
void config_free(Config *cfg);

#endif
