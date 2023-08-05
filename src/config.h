#ifndef CONFIG_H
#define CONFIG_H

#include <toml.h>

typedef struct {
    int gap;
    char *terminal;
} wm_config;

wm_config get_config(void);
char *config_path(void);
wm_config default_config(void);

#endif // CONFIG_H
