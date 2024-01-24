#include "ansitermwriter.h"
#include "config.h"
#include "dosreader.h"
#include "vgacanvas.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#  include <io.h>
#  include <fcntl.h>
#  include <windows.h>
#endif

int main(int argc, char **argv)
{
    int rc = EXIT_FAILURE;
    Config *config = 0;
    FILE *dosfile = 0;
    FILE *ansifile = 0;
    VgaCanvas *canvas = 0;

    config = Config_fromOpts(argc, argv);
    if (!config) goto done;

    const char *infile = Config_infile(config);
    if (infile)
    {
	dosfile = fopen(infile, "rb");
	if (!dosfile)
	{
	    fprintf(stderr, "Error opening `%s' for reading.", infile);
	    goto done;
	}
    }
    else
    {
#ifdef _WIN32
	_setmode(_fileno(stdin), _O_BINARY);
#endif
	dosfile = stdin;
    }

    canvas = VgaCanvas_create(Config_width(config), Config_tabwidth(config));
    if (!canvas) goto done;
    if (Config_ignoreeof(config)) DosReader_ignoreeof(1);
    if (DosReader_read(canvas, dosfile) != 0) goto done;
    if (dosfile != stdin)
    {
	fclose(dosfile);
	dosfile = 0;
    }
    VgaCanvas_finalize(canvas);

    const char *outfile = Config_outfile(config);
    if (outfile)
    {
	ansifile = fopen(outfile, "wb");
	if (!ansifile)
	{
	    fprintf(stderr, "Error opening `%s' for writing.", outfile);
	    goto done;
	}
    }
    else
    {
#ifdef _WIN32
	int outfd = _fileno(stdout);
	_setmode(outfd, _O_BINARY);
	if (_isatty(outfd))
	{
	    HANDLE outhdl = (HANDLE)_get_osfhandle(outfd);
	    DWORD mode;
	    GetConsoleMode(outhdl, &mode);
	    mode |= 4; /* ENABLE_VIRTUAL_TERMINAL_PROCESSING */
	    SetConsoleMode(outhdl, mode);
	    SetConsoleOutputCP(CP_UTF8);
	}
#endif
	ansifile = stdout;
    }
    if (Config_defcolors(config)) AnsiTermWriter_usedefcols(1);
    Codepage cp = Config_codepage(config);
    if ((int)cp >= 0) AnsiTermWriter_usecp(cp);
    if (AnsiTermWriter_write(ansifile, canvas) != 0) goto done;
    rc = EXIT_SUCCESS;

done:
    if (dosfile && dosfile != stdin) fclose(dosfile);
    if (ansifile && ansifile != stdout) fclose(ansifile);
    VgaCanvas_destroy(canvas);
    Config_destroy(config);

    return rc;
}

