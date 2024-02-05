#ifndef DOS2ANSI_SAUCE_H
#define DOS2ANSI_SAUCE_H

#include "decl.h"

C_CLASS_DECL(Sauce);
C_CLASS_DECL(Stream);

Sauce *Sauce_read(Stream *in);

void Sauce_destroy(Sauce *self);

#endif
