#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

wm_config default_config() {
    wm_config config;
    config.gap = 10;
    config.terminal = "st";
    return config;
}

char *config_path(){
    char *xdg_config_home = getenv("XDG_CONFIG_HOME");
    char *config_path = malloc(200);
    if (xdg_config_home) {
        snprintf(config_path, 200, "%s/quirk/config.toml", xdg_config_home);
    } else {
        snprintf(config_path, 200, "%s/.config/quirk/config.toml", getenv("HOME"));
    }
    return config_path;
}


wm_config get_config() {
    wm_config config;
    FILE *config_file;

    char errbuf[200];
    config_file = fopen(config_path(), "r");
    if (!config_file) {
        printf("There is no config file.");
        return default_config();
    }

    toml_table_t *conf = toml_parse_file(config_file, errbuf, sizeof(errbuf));
    if (!conf) {
        printf("Parsing error: %s\n", errbuf);
        return default_config();
    }

    toml_table_t *wm = toml_table_in(conf, "wm");
    if (wm) {
        toml_datum_t tmp = toml_int_in(wm, "gap");
        if (tmp.ok && tmp.u.i != 0) {
            config.gap = tmp.u.i;
        } else {
            config.gap = default_config().gap;
        }
    } else {
        config.gap = default_config().gap;
    }

    toml_table_t *preferences = toml_table_in(conf, "pref");
    if (preferences) {
        toml_datum_t tmp = toml_string_in(preferences, "term");
        if (tmp.ok && tmp.u.s != NULL) {
            config.terminal = strdup(tmp.u.s);
            free(tmp.u.s);
        } else {
            config.terminal = default_config().terminal;
        }
    } else {
        config.terminal = default_config().terminal;
    }


    toml_free(conf);
    fclose(config_file);

    return config;
}