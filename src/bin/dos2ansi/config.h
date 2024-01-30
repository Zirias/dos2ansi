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
int Config_codepage(const Config *self);
int Config_format(const Config *self);
int Config_bom(const Config *self);
int Config_colors(const Config *self);
int Config_test(const Config *self);
int Config_crlf(const Config *self);
int Config_brokenpipe(const Config *self);
int Config_markltr(const Config *self);
int Config_euro(const Config *self);
int Config_intcolors(const Config *self);
int Config_rgbcolors(const Config *self);
int Config_blink(const Config *self);
int Config_reverse(const Config *self);
int Config_nobrown(const Config *self);
void Config_destroy(Config *self);

#endif
