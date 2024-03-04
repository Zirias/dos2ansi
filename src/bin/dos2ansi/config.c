#include "config.h"

#include "bufferedwriter.h"
#include "codepage.h"
#include "help.h"
#include "saucequery.h"
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
    const char *query;
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
    int visapprox;
    int scrheight;
    int meta;
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
    Stream_printf(out, DOS2ANSI_USAGE_FMT, DOS2ANSI_USAGE_ARGS(prgname));
}

SUPPRESS(overlength-strings)
static void help(const char *prgname)
{
    Stream *out = Stream_createStandard(SST_STDOUT);
    out = BufferedWriter_create(out, 10*1024);
    printusage(out, prgname);
    Stream_puts(out, DOS2ANSI_HELP);
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

	case 'H':
	    if (intArg(&config->scrheight, op, 1, 255, 10) < 0)
	    {
		*error = "Screen height out of valid range";
		return -1;
	    }
	    break;
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
	case 'q':
	    if (SauceQuery_check(op))
	    {
		*error = "Invalid SAUCE query";
		return -1;
	    }
	    config->query = op;
	    config->showsauce = 1;
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
    const char onceflags[] = "ABCEHIPRSTWXabcdeklmopqrstuvwxy";
    char seen[sizeof onceflags - 1] = {0};

    Config *config = xmalloc(sizeof *config);
    config->infile = 0;
    config->outfile = 0;
    config->query = 0;
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
    config->visapprox = 1;
    config->scrheight = -1;
    config->meta = 0;

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
		    case 'H':
		    case 'c':
		    case 'o':
		    case 'q':
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

		    case 'X':
			config->brokenpipe = 0;
			config->visapprox = 0;
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

		    case 'm':
			config->meta = 1;
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

const char *Config_query(const Config *self)
{
    return self->query;
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

int Config_visapprox(const Config *self)
{
    return self->visapprox;
}

int Config_scrheight(const Config *self)
{
    return self->scrheight;
}

int Config_meta(const Config *self)
{
    return self->meta;
}

void Config_destroy(Config *self)
{
#ifdef _WIN32
    if (!self) return;
    free(self->argvstore);
#endif
    free(self);
}

