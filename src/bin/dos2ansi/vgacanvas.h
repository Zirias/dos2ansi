#ifndef DOS2ANSI_VGACANVAS_H
#define DOS2ANSI_VGACANVAS_H

#include "decl.h"

C_CLASS_DECL(VgaCanvas);

VgaCanvas *VgaCanvas_create(int width);

void VgaCanvas_put(VgaCanvas *self, char c);
void VgaCanvas_setFg(VgaCanvas *self, char fg);
void VgaCanvas_setBg(VgaCanvas *self, char bg);
void VgaCanvas_up(VgaCanvas *self, unsigned n);
void VgaCanvas_down(VgaCanvas *self, unsigned n);
void VgaCanvas_left(VgaCanvas *self, unsigned n);
void VgaCanvas_right(VgaCanvas *self, unsigned n);

int VgaCanvas_x(const VgaCanvas *self);
int VgaCanvas_y(const VgaCanvas *self);
int VgaCanvas_fg(const VgaCanvas *self);
int VgaCanvas_bg(const VgaCanvas *self);

void VgaCanvas_destroy(VgaCanvas *self);

#endif
