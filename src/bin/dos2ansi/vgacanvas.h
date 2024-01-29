#ifndef DOS2ANSI_VGACANVAS_H
#define DOS2ANSI_VGACANVAS_H

#include "decl.h"

C_CLASS_DECL(VgaCanvas);
C_CLASS_DECL(Codepage);
C_CLASS_DECL(Stream);

typedef enum VgaSerFlags
{
    VSF_NONE	= 0,
    VSF_CRLF	= 1 << 0,   /* write newlines as CRLF instead of just LF */
    VSF_BOM	= 1 << 1,   /* start with a BOM (byte order mark) */
    VSF_LTRO	= 1 << 2    /* wrap output in left-to-right override */
} VgaSerFlags;

VgaCanvas *VgaCanvas_create(int width, int tabwidth);

void VgaCanvas_put(VgaCanvas *self, char c);
void VgaCanvas_setFg(VgaCanvas *self, char fg);
void VgaCanvas_setBg(VgaCanvas *self, char bg);
void VgaCanvas_setBold(VgaCanvas *self, int bold);
void VgaCanvas_setBlink(VgaCanvas *self, int blink);
void VgaCanvas_setReverse(VgaCanvas *self, int reverse);
void VgaCanvas_setHidden(VgaCanvas *self, int hidden);
void VgaCanvas_resetAttr(VgaCanvas *self);
void VgaCanvas_up(VgaCanvas *self, unsigned n);
void VgaCanvas_down(VgaCanvas *self, unsigned n);
void VgaCanvas_left(VgaCanvas *self, unsigned n);
void VgaCanvas_right(VgaCanvas *self, unsigned n);

int VgaCanvas_serialize(const VgaCanvas *self,
	Stream *out, const Codepage *cp, VgaSerFlags flags);

void VgaCanvas_destroy(VgaCanvas *self);

#endif
