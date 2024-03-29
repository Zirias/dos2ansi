#include "ansicolorwriter.h"
#include "ansisysrenderer.h"
#include "bufferedwriter.h"
#include "codepage.h"
#include "config.h"
#include "dosreader.h"
#include "sauce.h"
#include "sauceprinter.h"
#include "saucequery.h"
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
#  include <versionhelpers.h>
#endif

#define STREAMBUFSIZE (256*1024)
#define MAXSTREAMSIZE (4*1024*1024)

typedef struct InputStreamSettings {
    Sauce *sauce;
    int forcedwidth;
    int forcedheight;
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
    settings->forcedheight = -1;
    Stream *in = 0;

    if (Config_test(config))
    {
	in = Stream_createMemory(4096);
	TestWriter_write(in, config);
	settings->forcedwidth = 70;
	settings->forcedheight = 25;
	return in;
    }

    const char *infile = Config_infile(config);
    if (infile)
    {
	in = Stream_openFile(infile, FOF_READ);
	if (!in) fprintf(stderr, "Error opening `%s' for reading.\n", infile);
    }
    else in = Stream_createStandard(SST_STDIN);
    if (!in) return 0;

    long insz = Stream_size(in);
    if (insz < 0)
    {
	Stream *mem = Stream_fromStream(in, MAXSTREAMSIZE);
	Stream_destroy(in);
	in = mem;
	if (!in) return 0;
    }
    settings->sauce = Sauce_read(in, insz);

    if (Config_showsauce(config))
    {
	Stream_destroy(in);
	in = 0;
	settings->forcedwidth = 80;
	settings->forcedheight = 25;
	if (!settings->sauce)
	{
	    fputs("No SAUCE found!\n", stderr);
	}
    }
    else
    {
	Stream_seek(in, SSS_START, 0);
	long dossz = settings->sauce ? Sauce_startpos(settings->sauce) : -1;
	in = DosReader_create(in,
		STREAMBUFSIZE, dossz, Config_ignoreeof(config));

	if (Config_nosauce(config))
	{
	    Sauce_destroy(settings->sauce);
	    settings->sauce = 0;
	}
    }

    return in;
}

#ifdef _WIN32

static void initOutFlags(int *cflags, int *defformat, int *forcedbom)
{
    *cflags = CF_NONE;
    *defformat = UF_UTF16LE;
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
    if (!out) out = Stream_createStandard(SST_STDOUT);
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
    return Stream_createStandard(SST_STDOUT);
}

#endif

#if defined(WITH_CURSES) && !defined(_WIN32)

static Stream *createColorWriter(const Config *config,
	Stream *out, ColorFlags flags)
{
    if (Config_forceansi(config)) return AnsiColorWriter_create(out, flags);
    return TiColorWriter_create(out, flags);
}

#else

static Stream *createColorWriter(const Config *config,
	Stream *out, ColorFlags flags)
{
    (void)config;

    return AnsiColorWriter_create(out, flags);
}

#endif

static Stream *createOutputStream(const Config *config, const Sauce *sauce,
	OutputStreamSettings *settings)
{
    Stream *out = 0;
    int cflags;
    int defformat;

    const char *outfile = Config_outfile(config);
    if (outfile)
    {
	out = Stream_openFile(outfile, FOF_WRITE);
	if (!out) fprintf(stderr,
		"Error opening `%s' for writing.\n", outfile);
	initOutFlags(&cflags, &defformat, &settings->forcedbom);
    }
    else out = createStdoutStream(config, &cflags, &defformat,
	    &settings->forcedbom);
    if (!out) return 0;

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
	if ((sauce && !Sauce_nonblink(sauce))
		|| Config_blink(config)) cflags |= CF_LBG_BLINK;
	else if (Config_reverse(config)) cflags |= CF_LBG_REV;
	if (Config_nobrown(config)) cflags |= CF_RGBNOBROWN;
	if (Config_fullansi(config)) cflags |= CF_FULLANSI;
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
    Stream *meta = 0;
    VgaCanvas *canvas = 0;
    Codepage *cp = 0;
    InputStreamSettings insettings = {0};
    OutputStreamSettings outsettings = {0};

    config = Config_fromOpts(argc, argv);
    if (!config) goto done;

    in = createInputStream(config, &insettings);
    if (!in && !insettings.sauce) goto done;

    if (Config_query(config))
    {
	if (!insettings.sauce) goto done;
	if (SauceQuery_print(insettings.sauce, Config_query(config),
		    Config_crlf(config)) >= 0) rc = EXIT_SUCCESS;
	goto done;
    }

    int width = insettings.forcedwidth;
    if (width < 0 && insettings.sauce) width = Sauce_width(insettings.sauce);
    if (width < 0) width = Config_width(config);
    if (width < 0) width = 80;
    int height = insettings.forcedheight;
    if (insettings.sauce) height = Sauce_scrheight(insettings.sauce);
    if (height < 0) height = Config_scrheight(config);
    if (height < 0) height = 25;
    int tabwidth = Config_tabwidth(config);
    if (tabwidth < 0) tabwidth = 8;
    if (tabwidth > width) tabwidth = width;

    if (Config_meta(config))
    {
	meta = Stream_createStandard(SST_STDERR);
	meta = BufferedWriter_create(meta, 1024);
	Stream_printf(meta, "m_setwidth=%d\n", width);
	Stream_printf(meta, "m_setheight=%d\n", height);
    }

    canvas = VgaCanvas_create(width, height, tabwidth);
    if (!canvas) goto done;
    if (Config_showsauce(config))
    {
	SaucePrinter_print(canvas, insettings.sauce, Config_nowrap(config));
    }
    else
    {
	int rdsz = AnsiSysRenderer_render(canvas, meta, in);
	Stream_destroy(in);
	in = 0;
	if (meta && !rdsz)
	{
	    BufferedWriter_discard(meta);
	    --rdsz;
	}
	if (rdsz < 0) goto done;
    }

    out = createOutputStream(config, insettings.sauce, &outsettings);
    if (!out) goto done;

    int cpid = -1;
    CodepageFlags cpflags = CPF_NONE;
    if (Config_showsauce(config)) cpid = CP_437;
    if (cpid < 0 && insettings.sauce)
    {
	cpid = Sauce_cpid(insettings.sauce);
	cpflags = Sauce_cpflags(insettings.sauce);
    }
    if (cpid < 0)
    {
	cpid = Config_codepage(config);
	cpflags = Config_cpflags(config);
    }
    if (Config_brokenpipe(config) == 0) cpflags |= CPF_SOLIDBAR;
    if (Config_brokenpipe(config) == 1) cpflags |= CPF_BROKENBAR;
    if (Config_euro(config)) cpflags |= CPF_EUROSYM;
    if (Config_visapprox(config)) cpflags |= CPF_VISAPPROX;
    cp = Codepage_create(cpid, cpflags);

    VgaSerFlags vsflags = VSF_NONE;
    int wantbom = outsettings.forcedbom;
    if (wantbom < 0) wantbom = Config_bom(config);
    if (wantbom < 0) wantbom = outsettings.format != UF_UTF8;
    if (wantbom) vsflags |= VSF_BOM;
    if (Config_crlf(config)) vsflags |= VSF_CRLF;
    if (Config_markltr(config)) vsflags |= VSF_LTRO;
    if (Config_defcolors(config)) vsflags |= VSF_CHOP;
    if ((height = VgaCanvas_serialize(canvas, out, cp, vsflags)) >= 0)
    {
	if (meta) Stream_printf(meta, "m_height=%d\n", height);
	rc = EXIT_SUCCESS;
    }

done:
    Codepage_destroy(cp);
    Sauce_destroy(insettings.sauce);
    Stream_destroy(meta);
    Stream_destroy(in);
    Stream_destroy(out);
    VgaCanvas_destroy(canvas);
    Config_destroy(config);

    return rc;
}

