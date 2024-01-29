#ifndef DOS2ANSI_ANSITERMWRITER_H
#define DOS2ANSI_ANSITERMWRITER_H

#include "decl.h"

C_CLASS_DECL(Codepage);
C_CLASS_DECL(Stream);
C_CLASS_DECL(VgaCanvas);

void AnsiTermWriter_usebom(int arg);
void AnsiTermWriter_crlf(int arg);
void AnsiTermWriter_markltr(int arg);

int AnsiTermWriter_write(Stream *stream, const Codepage *cp,
	const VgaCanvas *canvas);

#endif
