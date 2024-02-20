#ifndef DOS2ANSI_ANSISYSRENDERER_H
#define DOS2ANSI_ANSISYSRENDERER_H

#include "decl.h"

C_CLASS_DECL(Stream);
C_CLASS_DECL(VgaCanvas);

int AnsiSysRenderer_render(VgaCanvas *canvas, Stream *meta, Stream *stream)
    ATTR_NONNULL((1)) ATTR_NONNULL((3));

#endif
