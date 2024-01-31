#ifndef DOS2ANSI_WINCONSOLEWRITER_H
#define DOS2ANSI_WINCONSOLEWRITER_H

#include "decl.h"

#include <windows.h>

C_CLASS_DECL(Stream);

Stream *WinConsoleWriter_create(HANDLE console, int stripcolors);

#endif
