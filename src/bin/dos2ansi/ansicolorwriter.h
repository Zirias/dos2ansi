#ifndef DOS2ANSI_ANSICOLORWRITER_H
#define DOS2ANSI_ANSICOLORWRITER_H

#include "decl.h"

C_CLASS_DECL(Stream);

typedef enum AnsiColorFlags
{
    ACF_NONE	    = 0,
    ACF_STRIP	    = 1 << 0,	/* strip out any colors */
    ACF_DEFAULT	    = 1 << 1,	/* assume lightgray on black is default */
    ACF_LBG_REV	    = 1 << 2,	/* use reverse for "light" background */
    ACF_LBG_BLINK   = 1 << 3,	/* use blink for "light" background */
    ACF_BRIGHTCOLS  = 1 << 4,	/* use explicit bright colors */
    ACF_RGBCOLS	    = 1 << 5,	/* use explicit RGB colors */
    ACF_RGBNOBROWN  = 1 << 6,	/* with RGB, use dark yellow, not brown */
} AnsiColorFlags;

Stream *AnsiColorWriter_create(Stream *out, AnsiColorFlags flags);

#endif
