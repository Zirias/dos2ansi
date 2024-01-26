#include "ansitermwriter.h"
#include "config.h"
#include "dosreader.h"
#include "stream.h"
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
    Stream *dosfile = 0;
    Stream *ansifile = 0;
    VgaCanvas *canvas = 0;

    config = Config_fromOpts(argc, argv);
    if (!config) goto done;

    const char *infile = Config_infile(config);
    if (infile)
    {
	FILE *f = fopen(infile, "rb");
	if (!f)
	{
	    fprintf(stderr, "Error opening `%s' for reading.", infile);
	    goto done;
	}
	dosfile = Stream_createFile(f);
    }
    else
    {
#ifdef _WIN32
	_setmode(_fileno(stdin), _O_BINARY);
#endif
	dosfile = Stream_createFile(stdin);
    }

    canvas = VgaCanvas_create(Config_width(config), Config_tabwidth(config));
    if (!canvas) goto done;
    if (Config_ignoreeof(config)) DosReader_ignoreeof(1);
    if (DosReader_read(canvas, dosfile) != 0) goto done;
    Stream_destroy(dosfile);
    dosfile = 0;
    VgaCanvas_finalize(canvas);

    const char *outfile = Config_outfile(config);
    int defformat = UF_UTF8;
    if (outfile)
    {
	FILE *f = fopen(outfile, "wb");
	if (!f)
	{
	    fprintf(stderr, "Error opening `%s' for writing.", outfile);
	    goto done;
	}
	ansifile = Stream_createFile(f);
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
	else defformat = UF_UTF16;
#endif
	ansifile = Stream_createFile(stdout);
    }
    Codepage cp = Config_codepage(config);
    if ((int)cp >= 0) AnsiTermWriter_usecp(cp);
    int format = Config_format(config);
    if (format < 0) format = defformat;
    AnsiTermWriter_useformat(format);
    int wantbom = Config_bom(config);
    if (wantbom < 0) wantbom = format != UF_UTF8;
    AnsiTermWriter_usebom(wantbom);
    AnsiTermWriter_usecolors(Config_colors(config));
    AnsiTermWriter_usedefcols(Config_defcolors(config));
    if (AnsiTermWriter_write(ansifile, canvas) != 0) goto done;
    rc = EXIT_SUCCESS;

done:
    Stream_destroy(dosfile);
    Stream_destroy(ansifile);
    VgaCanvas_destroy(canvas);
    Config_destroy(config);

    return rc;
}

