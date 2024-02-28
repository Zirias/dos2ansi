#ifndef DOS2ANSI_TESTWRITER_H
#define DOS2ANSI_TESTWRITER_H

#include "decl.h"

C_CLASS_DECL(Config);
C_CLASS_DECL(Stream);

int TestWriter_write(Stream *stream, const Config *config) ATTR_NONNULL((1));

#endif
