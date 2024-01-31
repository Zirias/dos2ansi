#ifndef DOS2ANSI_TICOLORWRITER_H
#define DOS2ANSI_TICOLORWRITER_H

#include "decl.h"

C_CLASS_DECL(Stream);

typedef enum TiColorFlags
{
    TCF_NONE	    = 0,
    TCF_STRIP	    = 1 << 0,	/* strip out any colors */
    TCF_DEFAULT	    = 1 << 1,	/* assume lightgray on black is default */
    TCF_LBG_REV	    = 1 << 2,	/* use reverse for "light" background */
    TCF_LBG_BLINK   = 1 << 3,	/* use blink for "light" background */
    TCF_RGBNOBROWN  = 1 << 4,	/* with RGB, use dark yellow, not brown */
} TiColorFlags;

Stream *TiColorWriter_create(Stream *out, TiColorFlags flags);

#endif
