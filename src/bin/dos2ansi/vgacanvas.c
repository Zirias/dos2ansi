#include "vgacanvas.h"

#include "codepage.h"
#include "stream.h"
#include "util.h"

#include <stdint.h>
#include <stdlib.h>

#define LINESCHUNK 32

#define FL_BOLD 1
#define FL_BLINK 2
#define FL_REVERSE 4
#define FL_HIDDEN 8

typedef struct VgaChar
{
    uint8_t att;
    uint8_t chr;
} VgaChar;

typedef struct VgaLine
{
    int len;
    VgaChar chars[];
} VgaLine;

struct VgaCanvas
{
    size_t width;
    size_t height;
    size_t linecapa;
    size_t x;
    size_t y;
    VgaLine **lines;
    uint8_t tab;
    uint8_t fg;
    uint8_t bg;
    uint8_t flags;
};

static VgaLine *createline(int width)
{
    VgaLine *line = xmalloc(sizeof *line + width * sizeof *line->chars);
    line->len = width;
    for (int i = 0; i < width; ++i)
    {
	line->chars[i].att = 0x07;
	line->chars[i].chr = 0x20;
    }
    return line;
}

static void expand(VgaCanvas *self)
{
    if (self->y >= self->linecapa)
    {
	size_t oldcapa = self->linecapa;
	if (self->y - self->linecapa < LINESCHUNK)
	    self->linecapa += LINESCHUNK;
	else self->linecapa = self->y+1;
	self->lines = xrealloc(self->lines,
		self->linecapa * sizeof *self->lines);
	for (size_t i = oldcapa; i < self->linecapa; ++i)
	{
	    self->lines[i] = createline(self->width);
	}
    }
}

VgaCanvas *VgaCanvas_create(int width, int tabwidth)
{
    if (width < 0 || tabwidth < 0
	    || tabwidth > 0xff || tabwidth > width) return 0;
    VgaCanvas *self = xmalloc(sizeof *self);
    self->width = width;
    self->height = 0;
    self->linecapa = 0;
    self->x = 0;
    self->y = 0;
    self->lines = 0;
    self->tab = tabwidth;
    self->fg = 7U;
    self->bg = 0;
    self->flags = 0;
    expand(self);
    return self;
}

static uint8_t VgaCanvas_att(const VgaCanvas *self)
{
    uint8_t fg = self->fg;
    uint8_t bg = self->bg;
    if (self->flags & FL_REVERSE)
    {
	if (self->flags & FL_HIDDEN)
	{
	    bg = fg;
	}
	else
	{
	    fg = self->bg;
	    bg = self->fg;
	}
    }
    else if (self->flags & FL_HIDDEN)
    {
	fg = bg;
    }
    if (self->flags & FL_BOLD) fg |= 8U;
    if (self->flags & FL_BLINK) bg |= 8U;
    return (bg << 4) | fg;
}

void VgaCanvas_put(VgaCanvas *self, char c)
{
    /* ignore NUL, BEL and ESC */
    if (c == 0x00 || c == 0x07 || c == 0x1b) return;
    /* execute BS */
    else if (c == 0x08)
    {
	VgaCanvas_left(self, 1);
    }
    /* execute TAB */
    else if (c == 0x09) do
    {
	if (self->x == self->width)
	{
	    self->x = 0;
	    VgaCanvas_down(self, 1);
	}
	self->lines[self->y]->chars[self->x].att = VgaCanvas_att(self);
	self->lines[self->y]->chars[self->x].chr = 0x20;
	++self->x;
	if (self->y >= self->height) self->height = self->y+1;
    } while (self->x % self->tab);
    /* execute LF */
    else if (c == 0x0a) VgaCanvas_down(self, 1);
    /* execute CR */
    else if (c == 0x0d) self->x = 0;
    /* put any other character to canvas */
    else
    {
	if (self->x == self->width)
	{
	    self->x = 0;
	    VgaCanvas_down(self, 1);
	}
	self->lines[self->y]->chars[self->x].att = VgaCanvas_att(self);
	self->lines[self->y]->chars[self->x].chr = c;
	++self->x;
	if (self->y >= self->height) self->height = self->y+1;
    }
}

void VgaCanvas_setFg(VgaCanvas *self, char fg)
{
    self->fg = fg & 7;
}

void VgaCanvas_setBg(VgaCanvas *self, char bg)
{
    self->bg = bg & 7;
}

void VgaCanvas_setBold(VgaCanvas *self, int bold)
{
    if (bold) self->flags |= FL_BOLD;
    else self->flags &= ~(uint8_t)FL_BOLD;
}

void VgaCanvas_setBlink(VgaCanvas *self, int bold)
{
    if (bold) self->flags |= FL_BLINK;
    else self->flags &= ~(uint8_t)FL_BLINK;
}

void VgaCanvas_setReverse(VgaCanvas *self, int bold)
{
    if (bold) self->flags |= FL_REVERSE;
    else self->flags &= ~(uint8_t)FL_REVERSE;
}

void VgaCanvas_setHidden(VgaCanvas *self, int bold)
{
    if (bold) self->flags |= FL_HIDDEN;
    else self->flags &= ~(uint8_t)FL_HIDDEN;
}

void VgaCanvas_resetAttr(VgaCanvas *self)
{
    self->fg = 7U;
    self->bg = 0;
    self->flags = 0;
}

void VgaCanvas_up(VgaCanvas *self, unsigned n)
{
    if (!n) return;
    if (n >= self->y) self->y = 0;
    else self->y -= n;
}

void VgaCanvas_down(VgaCanvas *self, unsigned n)
{
    if (!n) return;
    self->y += n;
    expand(self);
}

void VgaCanvas_left(VgaCanvas *self, unsigned n)
{
    if (!n) return;
    if (self->x == self->width)
    {
	self->x = 0;
	++self->y;
    }
    if (n > self->x)
    {
	VgaCanvas_up(self, (n - self->x) / self->width + 1);
	n %= self->width;
	self->x += self->width;
    }
    self->x -= n;
}

void VgaCanvas_right(VgaCanvas *self, unsigned n)
{
    if (self->x == self->width)
    {
	self->x = 0;
	VgaCanvas_down(self, 1);
    }
    if (n > self->width - self->x)
    {
	VgaCanvas_down(self, (n - (self->width - self->x)) / self->width + 1);
	self->x = (self->x + n) % self->width;
    }
    else self->x += n;
}

void VgaCanvas_gotoxy(VgaCanvas *self, unsigned x, unsigned y)
{
    if (x >= self->width) x = self->width;
    self->x = x;
    self->y = y;
    expand(self);
}

static void clearInLine(VgaLine *l, size_t from, size_t to)
{
    for (size_t i = from; i < to; ++i)
    {
	l->chars[i].chr = 0x20U;
	l->chars[i].att = 0x07U;
    }
}

void VgaCanvas_clearLineAfter(VgaCanvas *self)
{
    clearInLine(self->lines[self->y], self->x, self->width);
}

void VgaCanvas_clearLineBefore(VgaCanvas *self)
{
    clearInLine(self->lines[self->y], 0, self->x);
}

void VgaCanvas_clearLine(VgaCanvas *self)
{
    clearInLine(self->lines[self->y], 0, self->width);
}

static void clearLines(VgaCanvas *self, size_t from, size_t to)
{
    for (size_t i = from; i < to; ++i)
    {
	clearInLine(self->lines[i], 0, self->width);
    }
}

void VgaCanvas_clearAfter(VgaCanvas *self)
{
    VgaCanvas_clearLineAfter(self);
    if (self->y < self->height) clearLines(self, self->y+1, self->height);
}

void VgaCanvas_clearBefore(VgaCanvas *self)
{
    VgaCanvas_clearLineBefore(self);
    if (self->y > 0) clearLines(self, 0, self->y-1);
}

void VgaCanvas_clearAll(VgaCanvas *self)
{
    clearLines(self, 0, self->height);
}

void VgaCanvas_xy(const VgaCanvas *self, unsigned *x, unsigned *y)
{
    if (self->x == self->width)
    {
	*x = 0;
	*y = self->y + 1;
    }
    else
    {
	*x = self->x;
	*y = self->y;
    }
}

static int put(Stream *stream, uint16_t c)
{
    return Stream_write(stream, &c, sizeof c);
}

int VgaCanvas_serialize(const VgaCanvas *self,
	Stream *out, const Codepage *cp, VgaSerFlags flags)
{
    if (!self->height) return 0;

    if ((flags & VSF_BOM) && !put(out, 0xfeffU)) return -1;
    if ((flags & VSF_LTRO) && !put(out, 0x202dU)) return -1;

    int hascolor = 0;
    for (size_t i = 0; i < self->height; ++i)
    {
	VgaLine *l = self->lines[i];
	l->len = self->width;
	if (flags & VSF_CHOP) while (l->len > 0)
	{
	    if (l->chars[l->len-1].chr == 0x20 &&
		    !(l->chars[l->len-1].att & 0xf0U)) --l->len;
	    else break;
	}
	if (!hascolor) for (int j = 0; j < l->len; ++j)
	{
	    if (l->chars[j].att != 0x07U)
	    {
		hascolor = 1;
		break;
	    }
	}
    }

    for (size_t i = 0; i < self->height; ++i)
    {
	const VgaLine *line = self->lines[i];
	int att = -1;
	for (int j = 0; j < line->len; ++j)
	{
	    if (hascolor)
	    {
		unsigned char newatt = line->chars[j].att;
		if (newatt != att)
		{
		    if (!put(out, 0xee00U | newatt)) return -1;
		    att = newatt;
		}
	    }
	    if (!put(out, Codepage_map(cp, line->chars[j].chr))) return -1;
	}
	if (hascolor && !put(out, 0xef00U)) return -1;
	if ((flags & VSF_CRLF) && !put(out, '\r')) return -1;
	if (!put(out, '\n')) return -1;
    }

    if ((flags & VSF_LTRO) && !put(out, 0x202cU)) return -1;

    return Stream_status(out) == SS_OK ? 0 : -1;
}

void VgaCanvas_destroy(VgaCanvas *self)
{
    if (!self) return;
    for (size_t i = 0; i < self->linecapa; ++i) free(self->lines[i]);
    free(self->lines);
    free(self);
}

