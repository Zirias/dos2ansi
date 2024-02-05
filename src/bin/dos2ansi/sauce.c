#include "sauce.h"

#include "codepage.h"
#include "stream.h"
#include "util.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAXSAUCE 0x4100
#define MINSAUCE 0x80
#define RDBUFCHUNK 0x208
#define RDCHUNK 0x200

#define CP_DEFAULT 0
#define CP_IMPLICIT 1
#define CP_NAMED 2
#define CP_OTHER 3

struct Sauce
{
    char *title;
    char *author;
    char *group;
    char *tinfos;
    const char *cpname;
    time_t date;
    uint16_t tinfo1;
    uint16_t tinfo2;
    uint8_t datatype;
    uint8_t filetype;
    uint8_t tflags;
    uint8_t cp;
};

typedef struct RawSauce
{
    size_t size;
    char bytes[];
} RawSauce;

static const char *cpfonts[] =
{
    "IBM VGA ",
    "IBM VGA50 ",
    "IBM VGA25G ",
    "IBM EGA ",
    "IBM EGA43 "
};

static RawSauce *RawSauce_read(Stream *in)
{
    size_t avail = 2 * RDBUFCHUNK;
    RawSauce *self = xmalloc(sizeof *self + avail);
    self->size = 0;
    size_t read = 0;
    size_t rdchunk = RDCHUNK;
    while ((read = Stream_read(in, self->bytes + self->size, rdchunk)))
    {
	self->size += read;
	if (self->size + rdchunk > MAXSAUCE) rdchunk = MAXSAUCE - self->size;
	if (self->size + rdchunk > avail)
	{
	    avail += RDBUFCHUNK;
	    if (avail > MAXSAUCE) goto error;
	    self = xrealloc(self, sizeof *self + avail);
	}
    }
    int status = Stream_status(in);
    if (status != SS_EOF) goto error;
    if (self->size < MINSAUCE) goto error;
    if (strncmp(self->bytes + (self->size - MINSAUCE),
		"SAUCE00", 7) != 0) goto error;
    return self;

error:
    free(self);
    return 0;
}

static char *getSauceStr(RawSauce *raw, size_t pos, size_t maxlen)
{
    char *src = raw->bytes + (raw->size - MINSAUCE) + pos;
    size_t len = 0;
    for (size_t i = 0; i < maxlen; ++i)
    {
	if (!src[i]) break;
	if (src[i] != 0x20) len = i+1;
    }
    if (!len) return 0;
    char *str = xmalloc(len + 1);
    memcpy(str, src, len);
    str[len] = 0;
    return str;
}

static time_t getSauceDate(RawSauce *raw, size_t pos)
{
    char *src = raw->bytes + (raw->size - MINSAUCE) + pos;
    size_t len = 0;
    for (; len < 8; ++len)
    {
	if (src[len] < '0' || src[len] > '9') break;
    }
    if (len != 8) return (time_t)(-1);
    struct tm tm = {0};
    char buf[5] = {0};
    memcpy(buf, src+6, 2);
    tm.tm_mday = atoi(buf);
    memcpy(buf, src+4, 2);
    tm.tm_mon = atoi(buf)-1;
    memcpy(buf, src, 4);
    tm.tm_year = atoi(buf)-1900;
    return mktime(&tm);
}

static unsigned getSauceInt(RawSauce *raw, size_t pos, int word)
{
    char *src = raw->bytes + (raw->size - MINSAUCE) + pos;
    unsigned v = (uint8_t)*src;
    if (word)
    {
	v |= (((uint8_t)src[1]) << 8);
    }
    return v;
}

Sauce *Sauce_read(Stream *in)
{
    RawSauce *raw = RawSauce_read(in);
    if (!raw) return 0;

    Sauce *self = xmalloc(sizeof *self);
    self->title = getSauceStr(raw, 7, 35);
    self->author = getSauceStr(raw, 42, 20);
    self->group = getSauceStr(raw, 62, 20);
    self->tinfos = getSauceStr(raw, 106, 22);
    self->cpname = 0;
    self->date = getSauceDate(raw, 82);
    self->tinfo1 = getSauceInt(raw, 96, 1);
    self->tinfo2 = getSauceInt(raw, 98, 1);
    self->datatype = getSauceInt(raw, 94, 0);
    self->filetype = getSauceInt(raw, 95, 0);
    self->tflags = getSauceInt(raw, 105, 0);
    self->cp = CP_DEFAULT;
    free(raw);

    if (self->datatype == 1 && self->filetype < 3 && self->tinfos)
    {
	for (unsigned i = 0; i < sizeof cpfonts / sizeof *cpfonts; ++i)
	{
	    size_t len = strlen(cpfonts[i]);
	    if (!strncmp(self->tinfos, cpfonts[i], len)
		    && strlen(self->tinfos + len) == 3)
	    {
		self->cpname = self->tinfos + len;
		self->tinfos[len-1] = 0;
		self->cp = CP_NAMED;
		break;
	    }
	}
	if (self->cp == CP_DEFAULT)
	{
	    if (!strncmp(self->tinfos, "IBM ", 4)) self->cp = CP_IMPLICIT;
	    else self->cp = CP_OTHER;
	}
    }

    return self;
}

const char *Sauce_title(const Sauce *self)
{
    return self->title;
}

const char *Sauce_author(const Sauce *self)
{
    return self->author;
}

const char *Sauce_group(const Sauce *self)
{
    return self->group;
}

time_t Sauce_date(const Sauce *self)
{
    return self->date;
}

const char *Sauce_type(const Sauce *self)
{
    switch (self->datatype)
    {
	case 0: return "Unspecified";
	case 1: switch (self->filetype)
		{
		    case 0: return "ASCII text";
		    case 1: return "ANSI text";
		    case 2: return "ANSiMation";
		    default: return "<Unsupported>";
		}
	default: return "<Unsupported>";
    }
}

int Sauce_width(const Sauce *self)
{
    if (self->datatype != 1 || self->filetype > 2) return -1;
    if (!self->tinfo1) return -1;
    return self->tinfo1;
}

int Sauce_height(const Sauce *self)
{
    if (self->datatype != 1 || self->filetype > 2) return -1;
    if (!self->tinfo2) return -1;
    return self->tinfo2;
}

int Sauce_nonblink(const Sauce *self)
{
    if (self->datatype != 1
	    || self->filetype < 1 || self->filetype > 2) return -1;
    return self->tflags & 1;
}

const char *Sauce_font(const Sauce *self)
{
    if (self->datatype != 1 || self->filetype > 2) return 0;
    if (!self->tinfos) return "IBM VGA (default)";
    return self->tinfos;
}

const char *Sauce_codepage(const Sauce *self)
{
    if (self->datatype != 1 || self->filetype > 2) return 0;
    switch (self->cp)
    {
	case CP_DEFAULT: return "437 (default)";
	case CP_IMPLICIT: return "437 (implicit)";
	case CP_NAMED: return self->cpname;
	case CP_OTHER: return "Other (unknown)";
	default: return 0;
    }
}

int Sauce_cpid(const Sauce *self)
{
    if (self->datatype != 1 || self->filetype > 2) return -1;
    if (self->cp == CP_DEFAULT || self->cp == CP_OTHER) return -1;
    if (self->cp == CP_IMPLICIT) return CP_437;
    const char *cpname = self->cpname;
    if (!strcmp(cpname, "MIK")) cpname = "866";
    return CodepageId_byName(cpname);
}

void Sauce_destroy(Sauce *self)
{
    if (!self) return;
    free(self->tinfos);
    free(self->group);
    free(self->author);
    free(self->title);
    free(self);
}

