#ifndef DOS2ANSI_DOSREADER_H
#define DOS2ANSI_DOSREADER_H

#include "decl.h"

C_CLASS_DECL(Stream);
C_CLASS_DECL(VgaCanvas);

void DosReader_ignoreeof(int arg);
int DosReader_read(VgaCanvas *canvas, Stream *stream);

#endif
