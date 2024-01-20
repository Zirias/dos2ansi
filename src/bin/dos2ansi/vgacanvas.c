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
    uint8_t att;
    size_t width;
    size_t height;
    size_t linecapa;
    size_t x;
    size_t y;
    VgaChar *lines[];
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

static void createlines(VgaCanvas *self, size_t oldcapa)
{
    for (size_t i = oldcapa; i < self->linecapa; ++i)
    {
	self->lines[i] = createline(self->width);
    }
}

VgaCanvas *VgaCanvas_create(int width)
{
    VgaCanvas *self = xmalloc(sizeof *self + LINESCHUNK * sizeof *self->lines);
    self->att = 0x07;
    self->width = width;
    self->height = 0;
    self->linecapa = LINESCHUNK;
    self->x = 0;
    self->y = 0;
    createlines(self, 0);
    return self;
}

void VgaCanvas_destroy(VgaCanvas *self)
{
    if (!self) return;
    for (size_t i = 0; i < self->linecapa; ++i) free(self->lines[i]);
    free(self);
}

