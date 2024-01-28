#ifndef DOS2ANSI_ANSITERMWRITER_H
#define DOS2ANSI_ANSITERMWRITER_H

#include "decl.h"

C_CLASS_DECL(Codepage);
C_CLASS_DECL(Stream);
C_CLASS_DECL(VgaCanvas);

typedef enum UnicodeFormat {
    UF_UTF8,
    UF_UTF16,
    UF_UTF16BE
} UnicodeFormat;

void AnsiTermWriter_usecolors(int arg);
void AnsiTermWriter_usedefcols(int arg);
void AnsiTermWriter_useformat(UnicodeFormat format);
void AnsiTermWriter_usebom(int arg);
void AnsiTermWriter_crlf(int arg);
void AnsiTermWriter_markltr(int arg);

int AnsiTermWriter_write(Stream *stream, const Codepage *cp,
	const VgaCanvas *canvas);

#endif
