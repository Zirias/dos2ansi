#include "config.h"

#include "bufferedwriter.h"
#include "codepage.h"
#include "strcicmp.h"
#include "stream.h"
#include "util.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define ARGBUFSZ 8

struct Config
{
#ifdef _WIN32
    char *argvstore;
#endif
    const char *infile;
    const char *outfile;
    int tabwidth;
    int width;
    int defcolors;
    int ignoreeof;
    int codepage;
    int cpflags;
    int format;
    int bom;
    int colors;
    int test;
    int crlf;
    int brokenpipe;
    int markltr;
    int euro;
    int intcolors;
    int rgbcolors;
    int blink;
    int reverse;
    int nobrown;
    int forceansi;
    int showsauce;
    int nosauce;
    int fullansi;
    int nowrap;
};

#ifdef WITH_CURSES
#  define OPT_TI "yes (curses)"
#else
#  define OPT_TI "no"
#endif
#if defined(USE_WIN32)
#  define OPT_IO "WinAPI (win32)"
#elif defined(USE_POSIX)
#  define OPT_IO "POSIX"
#else
#  define OPT_IO "C stdio"
#endif
#ifdef _WIN32
#  include <versionhelpers.h>
#  define printversion Stream_printf
#else
#  define printversion Stream_puts
#endif

static void version(void)
{
    Stream *out = Stream_createStandard(SST_STDOUT);
    printversion(out, "\nThis is dos2ansi v" DOS2ANSIVERSTR
#ifdef OSNAME
	    ", built for " OSNAME
#endif
	    "\nI/O Backend: " OPT_IO ", "
#ifdef _WIN32
	    "console colors: %s\n\n"
#else
	    "terminfo output: " OPT_TI "\n\n"
#endif
	    "WWW:      https://github.com/Zirias/dos2ansi\n"
	    "Author:   Felix Palmen <felix@palmen-it.de>\n"
	    "License:  BSD 2-clause (all rights reserved)\n\n"
#ifdef _WIN32
	    , IsWindows10OrGreater()
		? "256-color ANSI"
		: "legacy Console"
#endif
	    );
    Stream_destroy(out);
}

static void printusage(Stream *out, const char *prgname)
{
    Stream_printf(out,
	    "Usage: %s -V\n"
	    "       %s -h\n"
	    "       %s [-BCEPRSTWabdeiklprsvxy] [-c codepage]\n"
	    "\t\t[-o outfile] [-t tabwidth] [-u format] [-w width] [infile]\n",
	    prgname, prgname, prgname);
}

SUPPRESS(overlength-strings)
static void help(const char *prgname)
{
    Stream *out = Stream_createStandard(SST_STDOUT);
    out = BufferedWriter_create(out, 6*1024);
    printusage(out, prgname);
    Stream_puts(out, "\n"
	    "\t-A             Assume full support for ANSI SGR sequences\n"
	    "\t               including clearing individual attributes.\n"
	    "\t               This gives the shortest possible output in\n"
	    "\t               most cases, but might not work correctly with\n"
	    "\t               some terminals.\n"
	    "\t               Implies forcing generic ANSI output (-a).\n"
	    "\t-B             Disable writing a BOM\n"
	    "\t               (default: enabled for UTF16/UTF16LE)\n"
	    "\t-C             Disable colors in output\n"
	    "\t-E             Ignore the DOS EOF character (0x1a) and\n"
	    "\t               just continue reading when found.\n"
	    "\t-I             Using generic ANSI output, don't attempt to\n"
	    "\t               explicitly select intense colors but rely on\n"
	    "\t               the bold attribute for the foreground color\n"
	    "\t               instead. For background colors, see -k and -v.\n"
	    "\t-P             Force using a normal pipe bar symbol\n"
	    "\t               (default: replace with a broken bar for\n"
	    "\t               codepages not having an explicit broken bar)\n"
	    "\t-R             Line endings without CR (Unix format,\n"
	    "\t               default on non-Windows)\n"
	    "\t-S             Don't attempt to read SAUCE metadata.\n"
	    "\t               This enables setting width, blink and codepage\n"
	    "\t               different from the settings in SAUCE.\n"
	    "\t-T             Test mode, do not read any input, instead\n"
	    "\t               use some fixed 8bit encoding table.\n"
	    "\t               Implies -E.\n"
	    "\t-V             Print version information including build-time\n"
	    "\t               and OS-dependent configuration and exit.\n"
	    "\t-W             When showing SAUCE (-s), don't attempt to\n"
	    "\t               re-wrap the comment but show it as is.\n"
	    "\t-a             Force using the generic ANSI color writer.\n"
	    "\t               When built with curses support on non-Windows,\n"
	    "\t               a terminfo based writer is used instead,\n"
	    "\t               possibly creating terminal-specific output.\n"
	    "\t               On Windows 10 and later, exact colors are\n"
	    "\t               always enabled otherwise when output goes to\n"
	    "\t               the console.\n"
	    "\t               On older Windows versions, this flag does\n"
	    "\t               nothing and output to the console always uses\n"
	    "\t               a writer for the legacy Console API.\n"
	    "\t-b             Enable writing a BOM (default see -B above)\n"
	    "\t-c codepage    The DOS codepage used by the input file.\n"
	    "\t               May be prefixed with CP (any casing) and an\n"
	    "\t               optional space or dash.\n"
	    "\t               Ignored if codepage is available from SAUCE.\n"
	    "\t               supported: 437, 708, 720, 737, 775, 819, 850,\n"
	    "\t                          852, 855, 857, 860, 861, 862, 863,\n"
	    "\t                          864, 865, 869, KAM, MAZ, MIK\n"
	    "\t               aliases:   858           => 850 with Euro (-e)\n"
	    "\t                          872           => 855 with Euro (-e)\n"
	    "\t                          867, 895      => KAM\n"
	    "\t                          667, 790, 991 => MAZ\n"
	    "\t                          866           => MIK\n"
	    "\t               default:   437\n"
	    "\t-d             Use default terminal colors for VGA's gray\n"
	    "\t               on black. When not given, these colors are set\n"
	    "\t               explicitly.\n"
	    "\t-e             For codepages containing a generic currency\n"
	    "\t               symbol, use the Euro symbol instead. As\n"
	    "\t               special cases, replace other characters in\n"
	    "\t               codepages 850, 857, 864 and 869.\n"
	    "\t-h             Print this help text and exit.\n"
	    "\t-k             Use blink for intense background.\n"
	    "\t               Note that blink only works in some terminals.\n"
	    "\t               In any case, bright background colors will be\n"
	    "\t               disabled.\n"
	    "\t               Ignored if SAUCE is available.\n"
	    "\t-l             Attempt to enforce left-to-right direction by\n"
	    "\t               wrapping output in a Unicode LTR override\n"
	    "\t-o outfile     Write output to this file. If not given,\n"
	    "\t               output goes to the standard output.\n"
	    "\t-p             Force replacing the pipe bar with a broken bar\n"
	    "\t               matching the appearance of most VGA fonts\n"
	    "\t               (default: see -P above)\n"
	    "\t-r             Line endings with CR (DOS format,\n"
	    "\t               default on Windows)\n"
	    "\t-s             Show SAUCE metadata if available.\n"
	    "\t               Conflicts with ignoring EOF (-E).\n"
	    "\t-t tabwidth    Distance of tabstop positions.\n"
	    "\t               min: 2, default: 8, max: width or 255\n"
	    "\t-u format      Unicode output format, one of\n"
#ifdef _WIN32
	    "\t               UTF8 (default on terminal output),\n"
	    "\t               UTF16 (default on pipe/file), UTF16LE\n"
#else
	    "\t               UTF8 (default), UTF16, UTF16LE\n"
#endif
	    "\t-v             Use reverse for intense background.\n"
	    "\t               With most terminals, this will actually\n"
	    "\t               reverse colors. It's only included for exotic\n"
	    "\t               terminals enabling bright background colors\n"
	    "\t               instead.\n"
	    "\t               Implies disabling explicit intense colors (-I)\n"
	    "\t               and conflicts with blink (-k).\n"
	    "\t-w width       Width of the (virtual) screen.\n"
	    "\t               Ignored if the width is available from SAUCE.\n"
	    "\t               min: 16, default: 80, max: 1024\n"
	    "\t-x             Attempt to use exact CGA/VGA colors\n"
	    "\t-y             Do not replace dark yellow with brown,\n"
	    "\t               implies exact colors (-x).\n"
	    "\n"
	    "\tinfile         Read input from this file. If not given,\n"
	    "\t               input is read from the standard input.\n\n"
	    );
    Stream_destroy(out);
}
ENDSUPPRESS

static void usage(const char *prgname, const char *error)
{
    Stream *out = Stream_createStandard(SST_STDERR);
    if (error) out = BufferedWriter_create(out, 512);
    printusage(out, prgname);
    if (error) Stream_printf(out, "\nError: %s\n", error);
    Stream_destroy(out);
}

static int addArg(char *args, int *idx, char opt)
{
    if (*idx == ARGBUFSZ) return -1;
    memmove(args+1, args, (*idx)++);
    args[0] = opt;
    return 0;
}

static int intArg(int *setting, char *op, int min, int max, int base)
{
    char *endp;
    errno = 0;
    long val = strtol(op, &endp, base);
    if (errno == ERANGE || *endp || val < min || val > max) return -1;
    *setting = val;
    return 0;
}

static int optArg(Config *config, char *args, int *idx, char *op,
	const char **error)
{
    *error = 0;
    if (!*idx) return -1;
    switch (args[--*idx])
    {
	CodepageId cp;

	case 'c':
	    cp = CodepageId_byName(op);
	    if ((int)cp < 0)
	    {
		*error = "Unknown codepage";
		return -1;
	    }
	    config->codepage = cp;
	    config->cpflags = CodepageFlags_byName(op);
	    break;
	case 'o':
	    config->outfile = op;
	    break;
	case 't':
	    if (intArg(&config->tabwidth, op, 2, 255, 10) < 0)
	    {
		*error = "Tab width out of valid range";
		return -1;
	    }
	    break;
	case 'u':
	    if (!strcicmp(op, "utf8") || !strcicmp(op, "utf-8"))
		config->format = 0;
	    else if (!strcicmp(op, "utf16") || !strcicmp(op, "utf-16")
		    || !strcicmp(op, "utf16be") || !strcicmp(op, "utf-16be")
		    || !strcicmp(op, "utf16-be") || !strcicmp(op, "utf-16-be"))
		config->format = 1;
	    else if (!strcicmp(op, "utf16le") || !strcicmp(op, "utf-16le")
		    || !strcicmp(op, "utf16-le") || !strcicmp(op, "utf-16-le"))
		config->format = 2;
	    else
	    {
		*error = "Unknown output format";
		return -1;
	    }
	    break;
	case 'w':
	    if (intArg(&config->width, op, 16, 1024, 10) < 0)
	    {
		*error = "Width out of valid range";
		return -1;
	    }
	    break;
	default:
	    return -1;
    }
    return 0;
}

#ifdef _WIN32
#define CONFIGEXIT return ((void *)-1)
static Config *createConfig(int argc, char **argv);
Config *Config_fromOpts(int argc, char **argv)
{
    Config *config = 0;
    LPWSTR *wargv = 0;
    char *argvstore = 0;
    argv = 0;

    LPWSTR cmdline = GetCommandLineW();
    size_t argvstoresz = 3 * (wcslen(cmdline) + 1);
    argvstore = xmalloc(argvstoresz);
    wargv = CommandLineToArgvW(cmdline, &argc);
    if (!wargv) goto done;
    argv = xmalloc(argc * sizeof *argv);
    size_t argvstorepos = 0;
    for (int i = 0; i < argc; ++i)
    {
	size_t mblen;
	if (!(mblen = WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1,
			argvstore + argvstorepos, argvstoresz - argvstorepos,
			0, 0))) goto done;
	argv[i] = argvstore + argvstorepos;
	argvstorepos += mblen;
    }

    config = createConfig(argc, argv);
    if (config && config != ((void *)-1))
    {
	config->argvstore = xrealloc(argvstore, argvstorepos);
    }
    else free(argvstore);
    argvstore = 0;

done:
    free(argv);
    free(argvstore);
    if (wargv) LocalFree(wargv);
    if (config == ((void *)-1)) exit(0);
    return config;
}

static Config *createConfig(int argc, char **argv)
#else
#define CONFIGEXIT exit(0)
Config *Config_fromOpts(int argc, char **argv)
#endif
{
    int endflags = 0;
    int escapedash = 0;
    int arg;
    int naidx = 0;
    int haveinfile = 0;
    char needargs[ARGBUFSZ];
    const char onceflags[] = "ABCEIPRSTWabcdekloprstuvwxy";
    char seen[sizeof onceflags - 1] = {0};

    Config *config = xmalloc(sizeof *config);
    config->infile = 0;
    config->outfile = 0;
    config->tabwidth = -1;
    config->width = -1;
    config->defcolors = 0;
    config->ignoreeof = 0;
    config->codepage = CP_437;
    config->cpflags = CPF_NONE;
    config->format = -1;
    config->bom = -1;
    config->colors = 1;
    config->test = 0;
#ifdef _WIN32
    config->crlf = 1;
#else
    config->crlf = 0;
#endif
    config->brokenpipe = -1;
    config->markltr = 0;
    config->euro = 0;
    config->intcolors = 1;
    config->rgbcolors = 0;
    config->blink = 0;
    config->reverse = 0;
    config->nobrown = 0;
    config->forceansi = 0;
    config->showsauce = 0;
    config->nosauce = 0;
    config->nowrap = 0;

    const char *prgname = "dos2ansi";
    if (argc > 0) prgname = argv[0];

    const char *errstr;
    for (arg = 1; arg < argc; ++arg)
    {
	char *o = argv[arg];
	if (!escapedash && *o == '-' && o[1] == '-' && !o[2])
	{
	    escapedash = 1;
	    continue;
	}

	if (!endflags && !escapedash && *o == '-' && o[1])
	{
	    if (naidx)
	    {
		usage(prgname, "Missing argument(s) for given flags");
		goto error;
	    }

	    for (++o; *o; ++o)
	    {
		const char *sip = strchr(onceflags, *o);
		if (sip)
		{
		    int si = (int)(sip - onceflags);
		    if (seen[si])
		    {
			if (optArg(config, needargs, &naidx, o, &errstr) < 0)
			{
			    usage(prgname, errstr);
			    goto error;
			}
			else goto next;
		    }
		    seen[si] = 1;
		}
		switch (*o)
		{
		    case 'c':
		    case 'o':
		    case 't':
		    case 'u':
		    case 'w':
			if (addArg(needargs, &naidx, *o) < 0) goto error;
			break;

		    case 'A':
			config->forceansi = 1;
			config->fullansi = 1;
			break;

		    case 'B':
			config->bom = 0;
			break;

		    case 'C':
			config->colors = 0;
			break;

		    case 'E':
			config->ignoreeof = 1;
			break;

		    case 'I':
			config->intcolors = 0;
			break;

		    case 'P':
			config->brokenpipe = 0;
			break;

		    case 'R':
			config->crlf = 0;
			break;

		    case 'S':
			config->nosauce = 1;
			break;

		    case 'T':
			config->test = 1;
			config->ignoreeof = 1;
			break;

		    case 'V':
			free(config);
			version();
			CONFIGEXIT;

		    case 'W':
			config->nowrap = 1;
			break;

		    case 'a':
			config->forceansi = 1;
			break;

		    case 'b':
			config->bom = 1;
			break;

		    case 'd':
			config->defcolors = 1;
			break;

		    case 'e':
			config->euro = 1;
			break;

		    case 'h':
			free(config);
			help(prgname);
			CONFIGEXIT;

		    case 'k':
			config->blink = 1;
			break;

		    case 'l':
			config->markltr = 1;
			break;

		    case 'p':
			config->brokenpipe = 1;
			break;

		    case 'r':
			config->crlf = 1;
			break;

		    case 's':
			config->showsauce = 1;
			break;

		    case 'v':
			config->intcolors = 0;
			config->reverse = 1;
			break;

		    case 'x':
			config->rgbcolors = 1;
			break;

		    case 'y':
			config->rgbcolors = 1;
			config->nobrown = 1;
			break;

		    default:
			if (optArg(config, needargs, &naidx, o, &errstr) < 0)
			{
			    usage(prgname, errstr);
			    goto error;
			}
			goto next;
		}
	    }
	}
	else if (naidx)
	{
	    if (optArg(config, needargs, &naidx, o, &errstr) < 0)
	    {
		usage(prgname, errstr);
		goto error;
	    }
	}
	else
	{
	    if (!haveinfile)
	    {
		config->infile = o;
		haveinfile = 1;
	    }
	    else
	    {
		usage(prgname, "Extra arguments found");
		goto error;
	    }
	    endflags = 1;
	}
next:	;
    }
    if (naidx)
    {
	usage(prgname, "Missing argument(s) for given flags");
	goto error;
    }

    return config;

error:
    free(config);
    return 0;
}

const char *Config_infile(const Config *self)
{
    return self->infile;
}

const char *Config_outfile(const Config *self)
{
    return self->outfile;
}

int Config_tabwidth(const Config *self)
{
    return self->tabwidth;
}

int Config_width(const Config *self)
{
    return self->width;
}

int Config_defcolors(const Config *self)
{
    return self->defcolors;
}

int Config_ignoreeof(const Config *self)
{
    return self->ignoreeof;
}

int Config_codepage(const Config *self)
{
    return self->codepage;
}

int Config_cpflags(const Config *self)
{
    return self->cpflags;
}

int Config_format(const Config *self)
{
    return self->format;
}

int Config_bom(const Config *self)
{
    return self->bom;
}

int Config_colors(const Config *self)
{
    return self->colors;
}

int Config_test(const Config *self)
{
    return self->test;
}

int Config_crlf(const Config *self)
{
    return self->crlf;
}

int Config_brokenpipe(const Config *self)
{
    return self->brokenpipe;
}

int Config_markltr(const Config *self)
{
    return self->markltr;
}

int Config_euro(const Config *self)
{
    return self->euro;
}

int Config_intcolors(const Config *self)
{
    return self->intcolors;
}

int Config_rgbcolors(const Config *self)
{
    return self->rgbcolors;
}

int Config_blink(const Config *self)
{
    return self->blink;
}

int Config_reverse(const Config *self)
{
    return self->reverse;
}

int Config_nobrown(const Config *self)
{
    return self->nobrown;
}

int Config_forceansi(const Config *self)
{
    return self->forceansi;
}

int Config_showsauce(const Config *self)
{
    return self->showsauce;
}

int Config_nosauce(const Config *self)
{
    return self->nosauce;
}

int Config_fullansi(const Config *self)
{
    return self->fullansi;
}

int Config_nowrap(const Config *self)
{
    return self->nowrap;
}

void Config_destroy(Config *self)
{
#ifdef _WIN32
    if (!self) return;
    free(self->argvstore);
#endif
    free(self);
}

