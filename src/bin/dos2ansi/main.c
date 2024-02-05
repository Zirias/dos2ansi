#include "ansicolorwriter.h"
#include "ansisysrenderer.h"
#include "bufferedwriter.h"
#include "codepage.h"
#include "config.h"
#include "dosreader.h"
#include "sauce.h"
#include "sauceprinter.h"
#include "stream.h"
#include "testwriter.h"
#include "unicodewriter.h"
#include "vgacanvas.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef WITH_CURSES
#  include "ticolorwriter.h"
#endif

#ifdef _WIN32
#  include "winconsolewriter.h"
#  include <fcntl.h>
#  include <io.h>
#  include <versionhelpers.h>
#  define BINMODE(f) _setmode(_fileno(f), _O_BINARY)
#else
#  define BINMODE(f) (void)(f);
#endif

#define STREAMBUFSIZE 4096

typedef struct InputStreamSettings {
    int forcedwidth;
} InputStreamSettings;

typedef struct OutputStreamSettings
{
    int forcedbom;
    int format;
} OutputStreamSettings;

static Stream *createInputStream(const Config *config,
	InputStreamSettings *settings)
{
    settings->forcedwidth = -1;
    Stream *in = 0;

    if (Config_test(config))
    {
	in = Stream_createMemory();
	TestWriter_write(in);
	settings->forcedwidth = 70;
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
		return 0;
	    }
	    in = Stream_createFile(f);
	}
	else
	{
	    BINMODE(stdin);
	    in = Stream_createFile(stdin);
	}
	in = DosReader_create(in, STREAMBUFSIZE, Config_ignoreeof(config));
    }

    return in;
}

#ifdef _WIN32

static void initOutFlags(int *cflags, int *defformat, int *forcedbom)
{
    *cflags = CF_NONE;
    *defformat = UF_UTF16;
    *forcedbom = -1;
}

static Stream *createStdoutStream(const Config *config,
	int *cflags, int *defformat, int *forcedbom)
{
    Stream *out = 0;
    initOutFlags(cflags, defformat, forcedbom);

    HANDLE outhdl = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;

    if (GetConsoleMode(outhdl, &mode))
    {
	if (IsWindows10OrGreater())
	{
	    /* Use generic ANSI writer, but default to exact colors */
	    mode |= 4; /* ENABLE_VIRTUAL_TERMINAL_PROCESSING */
	    SetConsoleMode(outhdl, mode);
	    SetConsoleOutputCP(CP_UTF8);
	    *defformat = UF_UTF8;
	    if (!Config_forceansi(config)) *cflags = CF_RGBCOLS;
	}
	else
	{
	    /* Use legacy Windows Console API for output, directly accepting
	     * UTF16 in machine byte order and handling color changes */
	    out = WinConsoleWriter_create(outhdl,
		    !Config_colors(config));
	    *cflags = -1; /* don't add a color writer */
	    *defformat = -1; /* don't add a unicode writer */
	}
	*forcedbom = 0; /* never write a BOM to Windows console */
    }
    else
    {
	int outfd = _fileno(stdout);
	_setmode(outfd, _O_BINARY);
    }
    if (!out) out = Stream_createFile(stdout);
    return out;
}

#else

static void initOutFlags(int *cflags, int *defformat, int *forcedbom)
{
    *cflags = CF_NONE;
    *defformat = UF_UTF8;
    *forcedbom = -1;
}

static Stream *createStdoutStream(const Config *config,
	int *cflags, int *defformat, int *forcedbom)
{
    (void)config;

    initOutFlags(cflags, defformat, forcedbom);
    return Stream_createFile(stdout);
}

#endif

#if defined(WITH_CURSES) && !defined(_WIN32)

static Stream *createColorWriter(const Config *config,
	Stream *out, ColorFlags flags)
{
    /* Can't use the terminfo color writer
     * - when output doesn't go to stdout, because it uses putp() to write
     *   control sequences directly there
     * - when the output format isn't UTF8 (an 8bit encoding), because
     *   terminfo output is plain ASCII
     */
    if (!Config_forceansi(config)
	    && !Config_outfile(config)
	    && Config_format(config) <= UF_UTF8)
    {
	return TiColorWriter_create(out, flags);
    }
    else return AnsiColorWriter_create(out, flags);
}

#else

static Stream *createColorWriter(const Config *config,
	Stream *out, ColorFlags flags)
{
    (void)config;

    return AnsiColorWriter_create(out, flags);
}

#endif

static Stream *createOutputStream(const Config *config,
	OutputStreamSettings *settings)
{
    Stream *out = 0;
    int cflags;
    int defformat;

    const char *outfile = Config_outfile(config);
    if (outfile)
    {
	FILE *f = fopen(outfile, "wb");
	if (!f)
	{
	    fprintf(stderr, "Error opening `%s' for writing.", outfile);
	    return 0;
	}
	initOutFlags(&cflags, &defformat, &settings->forcedbom);
	out = Stream_createFile(f);
    }
    else
    {
	out = createStdoutStream(config, &cflags, &defformat,
		&settings->forcedbom);
    }

    out = BufferedWriter_create(out, STREAMBUFSIZE);

    if (defformat >= 0)
    {
	settings->format = Config_format(config);
	if (settings->format < 0) settings->format = defformat;
	out = UnicodeWriter_create(out, settings->format);
    }
    else settings->format = -1;

    if (cflags >= 0)
    {
	if (!Config_colors(config)) cflags |= CF_STRIP;
	if (Config_defcolors(config)) cflags |= CF_DEFAULT;
	if (Config_intcolors(config)) cflags |= CF_BRIGHTCOLS;
	if (Config_rgbcolors(config)) cflags |= CF_RGBCOLS;
	if (Config_blink(config)) cflags |= CF_LBG_BLINK;
	if (Config_reverse(config)) cflags |= CF_LBG_REV;
	if (Config_nobrown(config)) cflags |= CF_RGBNOBROWN;
	out = createColorWriter(config, out, cflags);
    }

    return  out;
}

int main(int argc, char **argv)
{
    int rc = EXIT_FAILURE;
    Config *config = 0;
    Stream *in = 0;
    Stream *out = 0;
    Sauce *sauce = 0;
    VgaCanvas *canvas = 0;
    Codepage *cp = 0;

    config = Config_fromOpts(argc, argv);
    if (!config) goto done;

    InputStreamSettings insettings;
    in = createInputStream(config, &insettings);
    if (!in) goto done;

    int width = insettings.forcedwidth;
    if (width < 0) width = Config_width(config);
    if (Config_showsauce(config)) width = 80;
    canvas = VgaCanvas_create(width, Config_tabwidth(config));
    if (!canvas) goto done;

    int cpid = Config_codepage(config);
    if (Config_showsauce(config))
    {
	DosReader_seekAfterEof(in);
	if (Stream_status(in) == SS_DOSEOF) sauce = Sauce_read(in);
	if (sauce) SaucePrinter_print(canvas, sauce);
	else
	{
	    fputs("No SAUCE found!\n", stderr);
	    goto done;
	}
	cpid = CP_437;
    }
    else
    {
	if (AnsiSysRenderer_render(canvas, in) != 0) goto done;
	if (Stream_status(in) == SS_DOSEOF) sauce = Sauce_read(in);
    }
    Stream_destroy(in);
    in = 0;

    OutputStreamSettings outsettings;
    out = createOutputStream(config, &outsettings);
    if (!out) goto done;

    CodepageFlags cpflags = CPF_NONE;
    if (Config_brokenpipe(config) == 0) cpflags |= CPF_SOLIDBAR;
    if (Config_brokenpipe(config) == 1) cpflags |= CPF_BROKENBAR;
    if (Config_euro(config)) cpflags |= CPF_EUROSYM;
    cp = Codepage_create(cpid, cpflags);

    VgaSerFlags vsflags = VSF_NONE;
    int wantbom = outsettings.forcedbom;
    if (wantbom < 0) wantbom = Config_bom(config);
    if (wantbom < 0) wantbom = outsettings.format != UF_UTF8;
    if (wantbom) vsflags |= VSF_BOM;
    if (Config_crlf(config)) vsflags |= VSF_CRLF;
    if (Config_markltr(config)) vsflags |= VSF_LTRO;
    if (Config_defcolors(config)) vsflags |= VSF_CHOP;
    if (VgaCanvas_serialize(canvas, out, cp, vsflags) != 0) goto done;

    rc = EXIT_SUCCESS;

done:
    Codepage_destroy(cp);
    Sauce_destroy(sauce);
    Stream_destroy(in);
    Stream_destroy(out);
    VgaCanvas_destroy(canvas);
    Config_destroy(config);

    return rc;
}

