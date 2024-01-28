#ifndef DOS2ANSI_UNICODEWRITER_H
#define DOS2ANSI_UNICODEWRITER_H

#include "decl.h"

C_CLASS_DECL(Stream);

typedef enum UnicodeFormat
{
    UF_UTF8,
    UF_UTF16,
    UF_UTF16LE
} UnicodeFormat;

Stream *UnicodeWriter_create(Stream *out, UnicodeFormat format);

#endif
