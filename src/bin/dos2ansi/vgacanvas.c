#include "vgacanvas.h"

#include "util.h"

#include <stdint.h>
#include <stdlib.h>

#define LINESCHUNK 32

#define FL_BOLD 1
#define FL_BLINK 2
#define FL_REVERSE 4
#define FL_HIDDEN 8

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
    uint8_t hascolor;
};

typedef struct VgaChar
{
    uint8_t att;
    uint8_t chr;
} VgaChar;

struct VgaLine
{
    int len;
    VgaChar chars[];
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
    self->hascolor = 0;
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

void VgaCanvas_finalize(VgaCanvas *self)
{
    self->hascolor = 0;
    for (size_t i = 0; i < self->height; ++i)
    {
	VgaLine *l = self->lines[i];
	l->len = self->width;
	while (l->len > 0)
	{
	    if (l->chars[l->len-1].chr == 0x20 &&
		    !(l->chars[l->len-1].att & 0x70U)) --l->len;
	    else break;
	}
	if (!self->hascolor) for (int j = 0; j < l->len; ++j)
	{
	    if (l->chars[j].att != 0x07U)
	    {
		self->hascolor = 1;
		break;
	    }
	}
    }
}

int VgaCanvas_hascolor(const VgaCanvas *self)
{
    return self->hascolor;
}

size_t VgaCanvas_height(const VgaCanvas *self)
{
    return self->height;
}

const VgaLine *VgaCanvas_line(const VgaCanvas *self, size_t lineno)
{
    if (lineno >= self->height) return 0;
    return self->lines[lineno];
}

int VgaLine_len(const VgaLine *self)
{
    return self->len;
}

unsigned char VgaLine_att(const VgaLine *self, int pos)
{
    return self->chars[pos].att;
}

char VgaLine_chr(const VgaLine *self, int pos)
{
    return self->chars[pos].chr;
}

void VgaCanvas_destroy(VgaCanvas *self)
{
    if (!self) return;
    for (size_t i = 0; i < self->linecapa; ++i) free(self->lines[i]);
    free(self->lines);
    free(self);
}

