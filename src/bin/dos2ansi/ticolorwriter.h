#ifndef DOS2ANSI_TICOLORWRITER_H
#define DOS2ANSI_TICOLORWRITER_H

#include "colorflags.h"
#include "decl.h"

C_CLASS_DECL(Stream);

Stream *TiColorWriter_create(Stream *out, ColorFlags flags);

#endif
