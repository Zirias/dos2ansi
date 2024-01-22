#ifndef DOS2ANSI_CONFIG_H
#define DOS2ANSI_CONFIG_H

#include "decl.h"

C_CLASS_DECL(Config);

Config *Config_fromOpts(int argc, char **argv);
const char *Config_infile(const Config *self);
const char *Config_outfile(const Config *self);
int Config_tabwidth(const Config *self);
int Config_width(const Config *self);
int Config_defcolors(const Config *self);
int Config_ignoreeof(const Config *self);
void Config_destroy(Config *self);

#endif
