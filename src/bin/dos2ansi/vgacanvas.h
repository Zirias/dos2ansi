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

/* Serialize the canvas contents as a stream of uint16_t values containing
 * characters of the Unicode BMP in machine byte order, using a supplied
 * codepage for the character mapping.
 *
 * Attribute changes are encoded using Private Use Area codepoints U+EE00 to
 * U+EEFF with the low byte containing the new attribute where the low nibble
 * contains the foreground color and the high nibble the background color,
 * encoded according to ANSI with intensity bit. Codepoint U+EF00 is used to
 * reset all attributes.
 *
 * An output stream is expected to interpret these codepoints. It only needs
 * to accept a single uint16_t at a time.
 */
int VgaCanvas_serialize(const VgaCanvas *self,
	Stream *out, const Codepage *cp, VgaSerFlags flags);

void VgaCanvas_destroy(VgaCanvas *self);

#endif
