#include "vgacanvas.h"

#include "util.h"

#include <stdint.h>
#include <stdlib.h>

#define LINESCHUNK 32

typedef struct VgaChar
{
    uint8_t att;
    uint8_t chr;
} VgaChar;

struct VgaCanvas
{
    size_t width;
    size_t height;
    size_t linecapa;
    size_t x;
    size_t y;
    VgaChar **lines;
    uint8_t att;
};

static VgaChar *createline(int width)
{
    VgaChar *line = xmalloc(sizeof *line * width);
    for (int i = 0; i < width; ++i)
    {
	line[i].att = 0x07;
	line[i].chr = 0x20;
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

VgaCanvas *VgaCanvas_create(int width)
{
    VgaCanvas *self = xmalloc(sizeof *self);
    self->width = width;
    self->height = 0;
    self->linecapa = 0;
    self->x = 0;
    self->y = 0;
    self->lines = 0;
    self->att = 0x07;
    expand(self);
    return self;
}

void VgaCanvas_put(VgaCanvas *self, char c)
{
    if (c == 0x0a) self->x = 0;
    else if (c == 0x0d) VgaCanvas_down(self, 1);
    else if ((unsigned char)c >= 0x20U)
    {
	if (self->x == self->width)
	{
	    self->x = 0;
	    VgaCanvas_down(self, 1);
	}
	self->lines[self->y][self->x].att = self->att;
	self->lines[self->y][self->x].chr = c;
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

int VgaCanvas_fg(const VgaCanvas *self)
{
    return self->att & 0x0fU;
}

int VgaCanvas_bg(const VgaCanvas *self)
{
    return self->att >> 4;
}

void VgaCanvas_destroy(VgaCanvas *self)
{
    if (!self) return;
    for (size_t i = 0; i < self->linecapa; ++i) free(self->lines[i]);
    free(self);
}

