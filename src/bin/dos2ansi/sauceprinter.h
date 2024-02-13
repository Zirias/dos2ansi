#ifndef DOS2ANSI_SAUCEPRINTER_H
#define DOS2ANSI_SAUCEPRINTER_H

#include "decl.h"

C_CLASS_DECL(Sauce);
C_CLASS_DECL(VgaCanvas);

void SaucePrinter_print(VgaCanvas *canvas, const Sauce *sauce, int nowrap)
    ATTR_NONNULL((1)) ATTR_NONNULL((2));

#endif
