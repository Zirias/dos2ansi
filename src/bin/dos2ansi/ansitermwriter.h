#ifndef DOS2ANSI_ANSITERMWRITER_H
#define DOS2ANSI_ANSITERMWRITER_H

#include "decl.h"

C_CLASS_DECL(Stream);
C_CLASS_DECL(VgaCanvas);

typedef enum Codepage {
    CP_437,
    CP_708,
    CP_720,
    CP_737,
    CP_775,
    CP_850,
    CP_852,
    CP_855,
    CP_857,
    CP_860,
    CP_861,
    CP_862,
    CP_863,
    CP_864,
    CP_865,
    CP_866,
    CP_869
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
void AnsiTermWriter_useeuro(int arg);
void AnsiTermWriter_crlf(int arg);
void AnsiTermWriter_brokenpipe(int arg);
void AnsiTermWriter_markltr(int arg);

Codepage AnsiTermWriter_cpbyname(const char *name);
int AnsiTermWriter_write(Stream *stream, const VgaCanvas *canvas);

#endif
