#include "vgacanvas.h"

#include "util.h"

#include <stdint.h>
#include <stdlib.h>

#define LINESCHUNK 32

struct VgaCanvas
{
    size_t width;
    size_t height;
    size_t linecapa;
    size_t x;
    size_t y;
    VgaLine **lines;
    uint8_t tab;
    uint8_t att;
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
    self->att = 0x07;
    expand(self);
    return self;
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
	self->lines[self->y]->chars[self->x].att = self->att;
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
	self->lines[self->y]->chars[self->x].att = self->att;
	self->lines[self->y]->chars[self->x].chr = c;
	++self->x;
	if (self->y >= self->height) self->height = self->y+1;
    }
}

void VgaCanvas_setFg(VgaCanvas *self, char fg)
{
    self->att &= 0xf0U;
    self->att |= (unsigned char)fg & 0xfU;
}

void VgaCanvas_setBg(VgaCanvas *self, char bg)
{
    self->att &= 0xfU;
    self->att |= ((unsigned char)bg & 0xfU) << 4;
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
    }
}

int VgaCanvas_fg(const VgaCanvas *self)
{
    return self->att & 0x0fU;
}

int VgaCanvas_bg(const VgaCanvas *self)
{
    return self->att >> 4;
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

int VgaLine_fg(const VgaLine *self, int pos)
{
    if (pos >= self->len || pos < 0) return -1;
    return self->chars[pos].att & 0x0fU;
}

int VgaLine_bg(const VgaLine *self, int pos)
{
    if (pos >= self->len || pos < 0) return -1;
    return self->chars[pos].att >> 4;
}

char VgaLine_chr(const VgaLine *self, int pos)
{
    return self->chars[pos].chr;
}

void VgaCanvas_destroy(VgaCanvas *self)
{
    if (!self) return;
    for (size_t i = 0; i < self->linecapa; ++i) free(self->lines[i]);
    free(self);
}

