#ifndef DOS2ANSI_COLORFLAGS_H
#define DOS2ANSI_COLORFLAGS_H

typedef enum ColorFlags
{
    CF_NONE	    = 0,
    CF_STRIP	    = 1 << 0,	/* strip out any colors */
    CF_DEFAULT	    = 1 << 1,	/* assume lightgray on black is default */
    CF_LBG_REV	    = 1 << 2,	/* use reverse for "light" background */
    CF_LBG_BLINK    = 1 << 3,	/* use blink for "light" background */
    CF_BRIGHTCOLS   = 1 << 4,	/* use explicit bright colors */
    CF_RGBCOLS	    = 1 << 5,	/* use explicit RGB colors */
    CF_RGBNOBROWN   = 1 << 6,	/* with RGB, use dark yellow, not brown */
    CF_FULLANSI	    = 1 << 7	/* use attribute-disabling sequences */
} ColorFlags;

#endif
