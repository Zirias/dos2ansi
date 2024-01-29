#ifndef DOS2ANSI_ANSICOLORWRITER_H
#define DOS2ANSI_ANSICOLORWRITER_H

#include "decl.h"

C_CLASS_DECL(Stream);

typedef enum AnsiColorFlags
{
    ACF_NONE	    = 0,
    ACF_STRIP	    = 1 << 0,	/* strip out any colors */
    ACF_DEFAULT	    = 1 << 1,	/* assume lightgray on black is default */
    ACF_LBG_BLINK   = 1 << 2,	/* blink is light bg instead of reverse */
    ACF_BRIGHTCOLS  = 1 << 3,	/* use explicit bright colors */
    ACF_RGBCOLS	    = 1 << 4	/* use explicit RGB colors */
} AnsiColorFlags;

Stream *AnsiColorWriter_create(Stream *out, AnsiColorFlags flags);

#endif