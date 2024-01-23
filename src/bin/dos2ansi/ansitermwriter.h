#ifndef DOS2ANSI_ANSITERMWRITER_H
#define DOS2ANSI_ANSITERMWRITER_H

#include "decl.h"

#include <stdio.h>

C_CLASS_DECL(VgaCanvas);

typedef enum Codepage {
    CP_437,
    CP_850,
    CP_858
} Codepage;

void AnsiTermWriter_usedefcols(int arg);
void AnsiTermWriter_usecp(Codepage cp);
Codepage AnsiTermWriter_cpbyname(const char *name);
int AnsiTermWriter_write(FILE *file, const VgaCanvas *canvas);

#endif
