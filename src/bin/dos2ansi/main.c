#include "ansitermwriter.h"
#include "dosreader.h"
#include "vgacanvas.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    int rc = EXIT_FAILURE;
    FILE *dosfile = 0;
    VgaCanvas *canvas = 0;

    if (argc <= 1) goto done;

    dosfile = fopen(argv[1], "r");
    if (!dosfile) goto done;

    canvas = VgaCanvas_create(80);
    if (DosReader_read(canvas, dosfile) != 0) goto done;
    fclose(dosfile);
    dosfile = 0;

    VgaCanvas_finalize(canvas);
    if (AnsiTermWriter_write(stdout, canvas) != 0) goto done;
    rc = EXIT_SUCCESS;

done:
    if (dosfile) fclose(dosfile);
    VgaCanvas_destroy(canvas);

    return rc;
}

