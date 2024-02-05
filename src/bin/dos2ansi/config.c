#include "config.h"

#include "codepage.h"
#include "util.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARGBUFSZ 8

#define STR(m) XSTR(m)
#define XSTR(m) #m

#ifdef _WIN32
#  define strcmplc _stricmp
#else
#  define _POSIX_C_SOURCE 200112L
#  include <strings.h>
#  define strcmplc strcasecmp
#endif

struct Config
{
    const char *infile;
    const char *outfile;
    int tabwidth;
    int width;
    int defcolors;
    int ignoreeof;
    int codepage;
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
};

static void usage(const char *prgname)
{
    fprintf(stderr, "Usage: %s [-BCEPRTabdeiklprsvxy] [-c codepage]\n"
	    "\t\t[-o outfile] [-t tabwidth] [-u format] [-w width] [infile]\n",
	    prgname);
    fputs("\n\t-B             Disable writing a BOM\n"
	    "\t               (default: enabled for UTF16/UTF16LE)\n"
	    "\t-C             Disable colors in output\n"
	    "\t-E             Ignore the DOS EOF character (0x1a) and\n"
	    "\t               just continue reading when found.\n"
	    "\t-P             Force using a normal pipe bar symbol\n"
	    "\t               (default: replace with a broken bar for\n"
	    "\t               codepages not having an explicit broken bar)\n"
	    "\t-R             Line endings without CR (Unix format,\n"
	    "\t               default on non-Windows)\n"
	    "\t-T             Test mode, do not read any input, instead\n"
	    "\t               use some fixed 8bit encoding table.\n"
	    "\t               Implies -E.\n"
	    "\t-a             Force using the generic ANSI color writer.\n"
	    "\t               When built with curses support on non-Windows,\n"
	    "\t               a terminfo based writer is used instead unless\n"
	    "\t               an output file is given with -o.\n"
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
	    "\t               supported: 437, 708, 720, 737, 775, 850, 852,\n"
	    "\t                          855, 857, 860, 861, 862, 863, 864,\n"
	    "\t                          865, 866, 869\n"
	    "\t               default:   437\n"
	    "\t-d             Use default terminal colors for VGA's gray\n"
	    "\t               on black. When not given, these colors are set\n"
	    "\t               explicitly.\n"
	    "\t-e             For codepages containing a generic currency\n"
	    "\t               symbol, use the Euro symbol instead. As\n"
	    "\t               special cases, replace other characters in\n"
	    "\t               codepages 850, 857, 864 and 869.\n"
	    "\t-i             Attempt to use explicit intense colors.\n"
	    "\t-k             Use blink for intense background.\n"
	    "\t               Note that blink is unlikely to work in modern\n"
	    "\t               terminals.\n"
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
	    "\t               Conflicts with blink (-k) and ignored with\n"
	    "\t               intense (-i) or exact (-x) colors.\n"
	    "\t-w width       Width of the (virtual) screen.\n"
	    "\t               min: 16, default: 80, max: 1024\n"
	    "\t-x             Attempt to use exact CGA/VGA colors\n"
	    "\t-y             Do not replace dark yellow with brown,\n"
	    "\t               implies exact colors (-x).\n"
	    "\n"
	    "\tinfile         Read input from this file. If not given,\n"
	    "\t               input is read from the standard input.\n\n",
	    stderr);
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

static int optArg(Config *config, char *args, int *idx, char *op)
{
    if (!*idx) return -1;
    switch (args[--*idx])
    {
	CodepageId cp;

	case 'c':
	    cp = CodepageId_byName(op);
	    if ((int)cp < 0) return -1;
	    config->codepage = cp;
	    break;
	case 'o':
	    config->outfile = op;
	    break;
	case 't':
	    if (intArg(&config->tabwidth, op, 2, 255, 10) < 0) return -1;
	    break;
	case 'u':
	    if (!strcmplc(op, "utf8") || !strcmplc(op, "utf-8"))
		config->format = 0;
	    else if (!strcmplc(op, "utf16") || !strcmplc(op, "utf-16")
		    || !strcmplc(op, "utf16be") || !strcmplc(op, "utf-16be")
		    || !strcmplc(op, "utf16-be") || !strcmplc(op, "utf-16-be"))
		config->format = 1;
	    else if (!strcmplc(op, "utf16le") || !strcmplc(op, "utf-16le")
		    || !strcmplc(op, "utf16-le") || !strcmplc(op, "utf-16-le"))
		config->format = 2;
	    else return -1;
	    break;
	case 'w':
	    if (intArg(&config->width, op, 16, 1024, 10) < 0) return -1;
	    break;
	default:
	    return -1;
    }
    return 0;
}

Config *Config_fromOpts(int argc, char **argv)
{
    int endflags = 0;
    int escapedash = 0;
    int arg;
    int naidx = 0;
    int haveinfile = 0;
    char needargs[ARGBUFSZ];
    const char onceflags[] = "BCEPRTabcdeikloprstuvwxy";
    char seen[sizeof onceflags - 1] = {0};

    Config *config = xmalloc(sizeof *config);
    config->infile = 0;
    config->outfile = 0;
    config->tabwidth = 8;
    config->width = 80;
    config->defcolors = 0;
    config->ignoreeof = 0;
    config->codepage = CP_437;
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
    config->intcolors = 0;
    config->rgbcolors = 0;
    config->blink = 0;
    config->reverse = 0;
    config->nobrown = 0;
    config->forceansi = 0;
    config->showsauce = 0;

    const char *prgname = "dos2ansi";
    if (argc > 0) prgname = argv[0];

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
		usage(prgname);
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
			if (optArg(config, needargs, &naidx, o) < 0)
			{
			    usage(prgname);
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

		    case 'B':
			config->bom = 0;
			break;

		    case 'C':
			config->colors = 0;
			break;

		    case 'E':
			config->ignoreeof = 1;
			break;

		    case 'P':
			config->brokenpipe = 0;
			break;

		    case 'R':
			config->crlf = 0;
			break;

		    case 'T':
			config->test = 1;
			config->ignoreeof = 1;
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

		    case 'i':
			config->intcolors = 1;
			break;

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
			if (optArg(config, needargs, &naidx, o) < 0)
			{
			    usage(prgname);
			    goto error;
			}
			goto next;
		}
	    }
	}
	else if (naidx)
	{
	    if (optArg(config, needargs, &naidx, o) < 0)
	    {
		usage(prgname);
		goto error;
	    }
	}
	else
	{
	    if (!haveinfile)
	    {
		config->infile = o;
	    }
	    else
	    {
		usage(prgname);
		goto error;
	    }
	    endflags = 1;
	}
next:	;
    }
    if (naidx || config->tabwidth >= config->width)
    {
	usage(prgname);
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

void Config_destroy(Config *self)
{
    free(self);
}

