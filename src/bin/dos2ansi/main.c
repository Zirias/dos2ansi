#include "ansicolorwriter.h"
#include "bufferedwriter.h"
#include "codepage.h"
#include "config.h"
#include "dosreader.h"
#include "stream.h"
#include "testwriter.h"
#include "unicodewriter.h"
#include "vgacanvas.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef WITH_CURSES
#  include "ticolorwriter.h"
#else
#  define TiColorWriter_create(s,f) (s)
#endif

#ifdef _WIN32
#  include "winconsolewriter.h"
#  include <fcntl.h>
#  include <io.h>
#  include <versionhelpers.h>
#  define DEFFORMAT UF_UTF16
#  define BINMODE(f) _setmode(_fileno(f), _O_BINARY)
#else
#  define DEFFORMAT UF_UTF8
#  define BINMODE(f) (void)(f);
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
	    BINMODE(stdin);
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
    int useterm = 0;
    int forcergb = 0;
    int useconsole = 0;
    int defformat = DEFFORMAT;
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
	HANDLE outhdl = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD mode;
	if (GetConsoleMode(outhdl, &mode))
	{
	    if (IsWindows10OrGreater())
	    {
		mode |= 4; /* ENABLE_VIRTUAL_TERMINAL_PROCESSING */
		SetConsoleMode(outhdl, mode);
		SetConsoleOutputCP(CP_UTF8);
		defformat = UF_UTF8;
		if (!Config_forceansi(config)) forcergb = 1;
	    }
	    else
	    {
		out = WinConsoleWriter_create(outhdl,
			!Config_colors(config));
		useconsole = 1;
	    }
	}
	else
	{
	    int outfd = _fileno(stdout);
	    _setmode(outfd, _O_BINARY);
	}
#endif
#ifdef WITH_CURSES
	if (!Config_forceansi(config)) useterm = 1;
#endif
	if (!out) out = Stream_createFile(stdout);
    }

    int format = Config_format(config);
    if (format < 0) format = defformat;
    int wantbom = Config_bom(config);
    if (wantbom < 0) wantbom = format != UF_UTF8;
    if (useconsole) wantbom = 0;

    ColorFlags cflags = CF_NONE;
    if (!Config_colors(config)) cflags |= CF_STRIP;
    if (Config_defcolors(config)) cflags |= CF_DEFAULT;
    if (Config_intcolors(config)) cflags |= CF_BRIGHTCOLS;
    if (Config_rgbcolors(config)) cflags |= CF_RGBCOLS;
    if (Config_blink(config)) cflags |= CF_LBG_BLINK;
    if (Config_reverse(config)) cflags |= CF_LBG_REV;
    if (Config_nobrown(config)) cflags |= CF_RGBNOBROWN;
    if (forcergb) cflags |= CF_RGBCOLS;

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
    if (!useconsole)
    {
	out = UnicodeWriter_create(out, format);
	if (useterm) out = TiColorWriter_create(out, cflags);
	else out = AnsiColorWriter_create(out, cflags);
    }
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

