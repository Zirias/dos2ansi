#ifndef DOS2ANSI_SAUCE_H
#define DOS2ANSI_SAUCE_H

#include "decl.h"

#include <time.h>

C_CLASS_DECL(Sauce);
C_CLASS_DECL(Stream);

Sauce *Sauce_read(Stream *in) ATTR_NONNULL((1));
long Sauce_startpos(const Sauce *self) CMETHOD ATTR_PURE;
const char *Sauce_title(const Sauce *self) CMETHOD ATTR_PURE;
const char *Sauce_author(const Sauce *self) CMETHOD ATTR_PURE;
const char *Sauce_group(const Sauce *self) CMETHOD ATTR_PURE;
time_t Sauce_date(const Sauce *self) CMETHOD ATTR_PURE;
const char *Sauce_type(const Sauce *self) CMETHOD ATTR_RETNONNULL ATTR_PURE;
int Sauce_width(const Sauce *self) CMETHOD ATTR_PURE;
int Sauce_height(const Sauce *self) CMETHOD ATTR_PURE;
int Sauce_nonblink(const Sauce *self) CMETHOD ATTR_PURE;
int Sauce_letterspacing(const Sauce *self) CMETHOD ATTR_PURE;
int Sauce_squarepixels(const Sauce *self) CMETHOD ATTR_PURE;
const char *Sauce_font(const Sauce *self) CMETHOD ATTR_PURE;
const char *Sauce_codepage(const Sauce *self) CMETHOD ATTR_PURE;
int Sauce_cpid(const Sauce *self) CMETHOD ATTR_PURE;
int Sauce_cpflags(const Sauce *self) CMETHOD ATTR_PURE;
int Sauce_scrheight(const Sauce *self) CMETHOD ATTR_PURE;
int Sauce_comments(const Sauce *self) CMETHOD ATTR_PURE;
const char *Sauce_comment(const Sauce *self, int lineno) CMETHOD ATTR_PURE;
void Sauce_destroy(Sauce *self);

#endif
