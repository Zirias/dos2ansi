#ifndef DOS2ANSI_VGACANVAS_H
#define DOS2ANSI_VGACANVAS_H

#include "decl.h"

#include <stddef.h>

C_CLASS_DECL(VgaCanvas);
C_CLASS_DECL(VgaLine);

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
void VgaCanvas_finalize(VgaCanvas *self);

int VgaCanvas_hascolor(const VgaCanvas *self);
size_t VgaCanvas_height(const VgaCanvas *self);
const VgaLine *VgaCanvas_line(const VgaCanvas *self, size_t lineno);
int VgaLine_len(const VgaLine *self);
int VgaLine_fg(const VgaLine *self, int pos);
int VgaLine_bg(const VgaLine *self, int pos);
char VgaLine_chr(const VgaLine *self, int pos);

void VgaCanvas_destroy(VgaCanvas *self);

#endif
