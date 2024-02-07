#ifndef DOS2ANSI_ANSICOLORWRITER_H
#define DOS2ANSI_ANSICOLORWRITER_H

#include "colorflags.h"
#include "decl.h"

C_CLASS_DECL(Stream);

Stream *AnsiColorWriter_create(Stream *out, ColorFlags flags)
    ATTR_NONNULL((1)) ATTR_RETNONNULL;

#endif
