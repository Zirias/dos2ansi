#include "ansicolorwriter.h"
#include "bufferedwriter.h"
#include "codepage.h"
#include "config.h"
#include "dosreader.h"
#include "stream.h"
#include "testwriter.h"
#include "unicodewriter.h"
#include "vgacanvas.h"

#ifdef WITH_CURSES
#  include "ticolorwriter.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#  include <io.h>
#  include <fcntl.h>
#  include <windows.h>
#endif

#define OUTBUFSIZE 4096

int main(int argc, char **argv)
{
    int rc = EXIT_FAILURE;
    Config *config = 0;
    Stream *in = 0;
    Stream *out = 0;
    VgaCanvas *canvas = 0;
    Codepage *cp = 0;

    config = Config_fromOpts(argc, argv);
    if (!config) goto done;

    int width = Config_width(config);
    if (Config_test(config))
    {
	in = Stream_createMemory();
	TestWriter_write(in);
	width = 70;
    }
    else
    {
	const char *infile = Config_infile(config);
	if (infile)
	{
	    FILE *f = fopen(infile, "rb");
	    if (!f)
	    {
		fprintf(stderr, "Error opening `%s' for reading.", infile);
		goto done;
	    }
	    in = Stream_createFile(f);
	}
	else
	{
#ifdef _WIN32
	    _setmode(_fileno(stdin), _O_BINARY);
#endif
	    in = Stream_createFile(stdin);
	}
    }

    canvas = VgaCanvas_create(width, Config_tabwidth(config));
    if (!canvas) goto done;
    if (Config_ignoreeof(config)) DosReader_ignoreeof(1);
    if (DosReader_read(canvas, in) != 0) goto done;
    Stream_destroy(in);
    in = 0;

    const char *outfile = Config_outfile(config);
    int defformat = UF_UTF8;
#if defined(_WIN32) || defined(WITH_CURSES)
    int useterm = 0;
#endif
    if (outfile)
    {
	FILE *f = fopen(outfile, "wb");
	if (!f)
	{
	    fprintf(stderr, "Error opening `%s' for writing.", outfile);
	    goto done;
	}
	out = Stream_createFile(f);
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
	    if (!Config_forceansi(config)) useterm = 1;
	}
	else defformat = UF_UTF16;
#endif
#ifdef WITH_CURSES
	if (!Config_forceansi(config)) useterm = 1;
#endif
	out = Stream_createFile(stdout);
    }

    int format = Config_format(config);
    if (format < 0) format = defformat;
    int wantbom = Config_bom(config);
    if (wantbom < 0) wantbom = format != UF_UTF8;

    AnsiColorFlags acflags = ACF_NONE;
#ifdef WITH_CURSES
    TiColorFlags tcflags = TCF_NONE;
    if (useterm)
    {
	if (!Config_colors(config)) tcflags |= TCF_STRIP;
	if (Config_defcolors(config)) tcflags |= TCF_DEFAULT;
	if (Config_blink(config)) tcflags |= TCF_LBG_BLINK;
	if (Config_reverse(config)) tcflags |= TCF_LBG_REV;
	if (Config_nobrown(config)) tcflags |= TCF_RGBNOBROWN;
    }
    else
    {
#endif
	if (!Config_colors(config)) acflags |= ACF_STRIP;
	if (Config_defcolors(config)) acflags |= ACF_DEFAULT;
	if (Config_intcolors(config)) acflags |= ACF_BRIGHTCOLS;
	if (Config_rgbcolors(config)) acflags |= ACF_RGBCOLS;
	if (Config_blink(config)) acflags |= ACF_LBG_BLINK;
	if (Config_reverse(config)) acflags |= ACF_LBG_REV;
	if (Config_nobrown(config)) acflags |= ACF_RGBNOBROWN;
#ifdef WITH_CURSES
    }
#endif

    CodepageFlags cpflags = CPF_NONE;
    if (Config_brokenpipe(config) == 0) cpflags |= CPF_SOLIDBAR;
    if (Config_brokenpipe(config) == 1) cpflags |= CPF_BROKENBAR;
    if (Config_euro(config)) cpflags |= CPF_EUROSYM;

    VgaSerFlags vsflags = VSF_NONE;
    if (wantbom) vsflags |= VSF_BOM;
    if (Config_crlf(config)) vsflags |= VSF_CRLF;
    if (Config_markltr(config)) vsflags |= VSF_LTRO;
    if (Config_defcolors(config)) vsflags |= VSF_CHOP;

    out = BufferedWriter_create(out, OUTBUFSIZE);
    out = UnicodeWriter_create(out, format);
#ifdef WITH_CURSES
    if (useterm)
	out = TiColorWriter_create(out, tcflags);
    else
#endif
	out = AnsiColorWriter_create(out, acflags);

    cp = Codepage_create(Config_codepage(config), cpflags);

    if (VgaCanvas_serialize(canvas, out, cp, vsflags) != 0) goto done;

    rc = EXIT_SUCCESS;

done:
    Codepage_destroy(cp);
    Stream_destroy(in);
    Stream_destroy(out);
    VgaCanvas_destroy(canvas);
    Config_destroy(config);

    return rc;
}

