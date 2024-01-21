#ifndef DOS2ANSI_ANSITERMWRITER_H
#define DOS2ANSI_ANSITERMWRITER_H

#include "decl.h"

#include <stdio.h>

C_CLASS_DECL(VgaCanvas);

int AnsiTermWriter_write(FILE *file, const VgaCanvas *canvas);

#endif
