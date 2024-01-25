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

typedef enum UnicodeFormat {
    UF_UTF8,
    UF_UTF16,
    UF_UTF16BE
} UnicodeFormat;

void AnsiTermWriter_usecolors(int arg);
void AnsiTermWriter_usedefcols(int arg);
void AnsiTermWriter_usecp(Codepage cp);
void AnsiTermWriter_useformat(UnicodeFormat format);
void AnsiTermWriter_usebom(int arg);

Codepage AnsiTermWriter_cpbyname(const char *name);
int AnsiTermWriter_write(FILE *file, const VgaCanvas *canvas);

#endif
