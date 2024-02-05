#ifndef DOS2ANSI_SAUCE_H
#define DOS2ANSI_SAUCE_H

#include "decl.h"

#include <time.h>

C_CLASS_DECL(Sauce);
C_CLASS_DECL(Stream);

Sauce *Sauce_read(Stream *in);
const char *Sauce_title(const Sauce *self);
const char *Sauce_author(const Sauce *self);
const char *Sauce_group(const Sauce *self);
time_t Sauce_date(const Sauce *self);
const char *Sauce_type(const Sauce *self);
int Sauce_width(const Sauce *self);
int Sauce_height(const Sauce *self);
void Sauce_destroy(Sauce *self);

#endif
