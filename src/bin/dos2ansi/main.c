#include "dosreader.h"
#include "vgacanvas.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    if (argc <= 1) return EXIT_FAILURE;

    FILE *dosfile = fopen(argv[1], "r");
    if (!dosfile) return EXIT_FAILURE;

    VgaCanvas *canvas = VgaCanvas_create(80);
    DosReader_read(canvas, dosfile);

    VgaCanvas_destroy(canvas);
    fclose(dosfile);

    return EXIT_SUCCESS;
}

