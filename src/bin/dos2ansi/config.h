#ifndef DOS2ANSI_CONFIG_H
#define DOS2ANSI_CONFIG_H

#include "decl.h"

C_CLASS_DECL(Config);

Config *Config_fromOpts(int argc, char **argv) ATTR_NONNULL((2));
const char *Config_infile(const Config *self) CMETHOD ATTR_PURE;
const char *Config_outfile(const Config *self) CMETHOD ATTR_PURE;
int Config_tabwidth(const Config *self) CMETHOD ATTR_PURE;
int Config_width(const Config *self) CMETHOD ATTR_PURE;
int Config_defcolors(const Config *self) CMETHOD ATTR_PURE;
int Config_ignoreeof(const Config *self) CMETHOD ATTR_PURE;
int Config_codepage(const Config *self) CMETHOD ATTR_PURE;
int Config_cpflags(const Config *self) CMETHOD ATTR_PURE;
int Config_format(const Config *self) CMETHOD ATTR_PURE;
int Config_bom(const Config *self) CMETHOD ATTR_PURE;
int Config_colors(const Config *self) CMETHOD ATTR_PURE;
int Config_test(const Config *self) CMETHOD ATTR_PURE;
int Config_crlf(const Config *self) CMETHOD ATTR_PURE;
int Config_brokenpipe(const Config *self) CMETHOD ATTR_PURE;
int Config_markltr(const Config *self) CMETHOD ATTR_PURE;
int Config_euro(const Config *self) CMETHOD ATTR_PURE;
int Config_intcolors(const Config *self) CMETHOD ATTR_PURE;
int Config_rgbcolors(const Config *self) CMETHOD ATTR_PURE;
int Config_blink(const Config *self) CMETHOD ATTR_PURE;
int Config_reverse(const Config *self) CMETHOD ATTR_PURE;
int Config_nobrown(const Config *self) CMETHOD ATTR_PURE;
int Config_forceansi(const Config *self) CMETHOD ATTR_PURE;
int Config_showsauce(const Config *self) CMETHOD ATTR_PURE;
int Config_nosauce(const Config *self) CMETHOD ATTR_PURE;
int Config_fullansi(const Config *self) CMETHOD ATTR_PURE;
void Config_destroy(Config *self);

#endif
