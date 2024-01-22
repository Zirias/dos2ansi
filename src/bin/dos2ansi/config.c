#include "config.h"

#include "util.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARGBUFSZ 8

#define STR(m) XSTR(m)
#define XSTR(m) #m

struct Config
{
    const char *infile;
    const char *outfile;
    int tabwidth;
    int width;
    int defcolors;
};

static void usage(const char *prgname)
{
    fprintf(stderr, "Usage: %s [-d] [-o outfile] [-t tabwidth] [-w width]\n"
	    "\t\t[infile]\n",
	    prgname);
    fputs("\n\t-d             Use default terminal colors for VGA's gray\n"
	    "\t               on black. When not given, these colors are set\n"
	    "\t               explicitly.\n"
	    "\t-o outfile     Write output to this file. If not given,\n"
	    "\t               output goes to the standard output.\n"
	    "\t-t tabwidth    Distance of tabstop positions.\n"
	    "\t               min: 2, default: 8, max: width or 255\n"
	    "\t-w width       Width of the (virtual) screen.\n"
	    "\t               min: 16, default: 80, max: 1024\n"
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
	case 'o':
	    config->outfile = op;
	    break;
	case 't':
	    if (intArg(&config->tabwidth, op, 2, 255, 10) < 0) return -1;
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
    const char onceflags[] = "dotw";
    char seen[sizeof onceflags - 1] = {0};

    Config *config = xmalloc(sizeof *config);
    config->infile = 0;
    config->outfile = 0;
    config->tabwidth = 8;
    config->width = 80;

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
			usage(prgname);
			goto error;
		    }
		    seen[si] = 1;
		}
		switch (*o)
		{
		    case 'o':
		    case 't':
		    case 'w':
			if (addArg(needargs, &naidx, *o) < 0) goto error;
			break;

		    case 'd':
			config->defcolors = 1;
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
	else if (optArg(config, needargs, &naidx, o) < 0)
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

void Config_destroy(Config *self)
{
    free(self);
}

