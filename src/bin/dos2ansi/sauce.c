#include "sauce.h"

#include "codepage.h"
#include "stream.h"
#include "util.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SAUCESZ 0x80

#define CP_DEFAULT 0
#define CP_IMPLICIT 1
#define CP_NAMED 2
#define CP_OTHER 3

#define is_text(s) ((s)->datatype == 1 && (s)->filetype < 3)
#define is_ansi(s) (is_text(s) && (s)->filetype > 0)
#define likely_text(s) (is_text(s) || (s)->datatype == 0)

struct Sauce
{
    char *title;
    char *author;
    char *group;
    char *tinfos;
    const char *cpname;
    long startpos;
    time_t date;
    uint16_t tinfo1;
    uint16_t tinfo2;
    uint8_t datatype;
    uint8_t filetype;
    uint8_t tflags;
    uint8_t cp;
    uint8_t scrheight;
    uint8_t lines;
    char comment[][65];
};

static const char *cpfonts[] =
{
    "IBM VGA ",
    "IBM VGA50 ",
    "IBM VGA25G ",
    "IBM EGA ",
    "IBM EGA43 "
};

static unsigned checkSauceStr(uint8_t *src, unsigned maxlen)
{
    unsigned len = 0;
    for (unsigned i = 0; i < maxlen; ++i)
    {
	if (!src[i]) break;
	if (src[i] != 0x20)
	{
	    /* filter control characters, they should never appear */
	    if ((src[i] >= 7 && src[i] <= 10) || src[i] == 13 || src[i] == 27)
	    {
		src[i] = 0x20;
	    }
	    len = i+1;
	}
    }
    return len;
}

static char *getSauceStr(uint8_t *raw, unsigned pos, unsigned maxlen)
{
    uint8_t *src = raw + pos;
    unsigned len = checkSauceStr(src, maxlen);
    if (!len) return 0;
    char *str = xmalloc(len + 1);
    memcpy(str, src, len);
    str[len] = 0;
    return str;
}

static void getSauceComment(char *dst, uint8_t *raw, int lineno)
{
    uint8_t *src = raw + 64 * lineno + 5;
    unsigned len = checkSauceStr(src, 64);
    if (len > 0 && len < 64 && src[len] == 0x20) ++len;
    memcpy(dst, src, len);
    dst[len] = 0;
}

static time_t getSauceDate(uint8_t *raw, unsigned pos)
{
    uint8_t *src = raw + pos;
    struct tm tm = {0};
    char buf[5] = {0};
    char *bufp;
    char *endp;
    memcpy(bufp = buf, src + 6, 2);
    if (*bufp < '0' || *bufp > '9') ++bufp;
    tm.tm_mday = strtol(bufp, &endp, 10);
    if (endp == buf) return (time_t)(-1);
    memcpy(bufp = buf, src + 4, 2);
    if (*bufp < '0' || *bufp > '9') ++bufp;
    tm.tm_mon = strtol(bufp, &endp, 10) - 1;
    if (endp == buf) return (time_t)(-1);
    memcpy(bufp = buf, src, 4);
    if (*bufp < '0' || *bufp > '9') ++bufp;
    if (*bufp < '0' || *bufp > '9') ++bufp;
    int year = strtol(bufp, &endp, 10);
    if (endp < buf+2) return (time_t)(-1);
    if (year < 100) tm.tm_year = year + (year < 70);
    else if (year < 1970) return (time_t)(-1);
    else tm.tm_year = year - 1900;
    return mktime(&tm);
}

static unsigned getSauceInt(uint8_t *raw, unsigned pos, int word)
{
    uint8_t *src = raw + pos;
    unsigned v = *src;
    if (word)
    {
	v |= (src[1] << 8);
    }
    return v;
}

Sauce *Sauce_read(Stream *in, long streamsz)
{
    uint8_t rawsauce[SAUCESZ];
    uint8_t *rawcmnt = 0;
    Sauce *self = 0;
    long startpos = 0;

    long insz = streamsz;
    if (insz < 0) insz = Stream_size(in);
    if (insz < SAUCESZ) goto done;
    if ((startpos = Stream_seek(in, SSS_END, -SAUCESZ)) < 0) goto done;
    if (Stream_read(in, rawsauce, SAUCESZ) != SAUCESZ) goto done;
    if (strncmp((const char *)rawsauce, "SAUCE00", 7) != 0) goto done;

    unsigned lines = getSauceInt(rawsauce, 104, 0);
    if (lines)
    {
	long cmntsz = 64 * lines + 5;
	if (cmntsz + SAUCESZ > insz) goto done;
	if ((startpos = Stream_seek(in,
			SSS_END, -(cmntsz + SAUCESZ))) < 0) goto done;
	rawcmnt = xmalloc(cmntsz);
	if (Stream_read(in, rawcmnt, cmntsz) != (unsigned)cmntsz) goto done;
	if (strncmp((const char *)rawcmnt, "COMNT", 5) != 0) goto done;
    }

    self = xmalloc(sizeof *self + lines * sizeof *self->comment);
    self->title = getSauceStr(rawsauce, 7, 35);
    self->author = getSauceStr(rawsauce, 42, 20);
    self->group = getSauceStr(rawsauce, 62, 20);
    self->tinfos = getSauceStr(rawsauce, 106, 22);
    self->cpname = 0;
    self->startpos = startpos;
    self->date = getSauceDate(rawsauce, 82);
    self->tinfo1 = getSauceInt(rawsauce, 96, 1);
    self->tinfo2 = getSauceInt(rawsauce, 98, 1);
    self->datatype = getSauceInt(rawsauce, 94, 0);
    self->filetype = getSauceInt(rawsauce, 95, 0);
    self->tflags = getSauceInt(rawsauce, 105, 0);
    self->cp = CP_DEFAULT;
    self->scrheight = 0;
    self->lines = lines;
    for (unsigned i = 0; i < lines; ++i)
    {
	getSauceComment(self->comment[i], rawcmnt, i);
    }

    if (likely_text(self) && self->tinfos)
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

	if (!strncmp(self->tinfos, "IBM VGA50", 9)) self->scrheight = 50;
	else if (!strncmp(self->tinfos, "IBM EGA43", 9)) self->scrheight = 43;
	else if (!strncmp(self->tinfos, "IBM ", 4)) self->scrheight = 25;
    }

done:
    free(rawcmnt);
    return self;
}

long Sauce_startpos(const Sauce *self)
{
    return self->startpos;
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
	case 0: return "<Unspecified>";
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
    if (!likely_text(self)) return -1;
    if (!self->tinfo1) return -1;
    return self->tinfo1;
}

int Sauce_height(const Sauce *self)
{
    if (!likely_text(self)) return -1;
    if (!self->tinfo2) return -1;
    return self->tinfo2;
}

int Sauce_nonblink(const Sauce *self)
{
    if (!is_ansi(self)) return -1;
    return self->tflags & 1;
}

int Sauce_letterspacing(const Sauce *self)
{
    if (!is_text(self)) return -1;
    int lsval = ((self->tflags >> 1) & 3) - 1;
    if (lsval > 1) lsval = -1;
    return lsval;
}

int Sauce_squarepixels(const Sauce *self)
{
    if (!is_text(self)) return -1;
    int spval = ((self->tflags >> 3) & 3) - 1;
    if (spval > 1) spval = -1;
    return spval;
}

const char *Sauce_font(const Sauce *self)
{
    if (!likely_text(self)) return 0;
    if (!self->tinfos) return is_text(self) ? "<IBM VGA (default)>" : 0;
    return self->tinfos;
}

const char *Sauce_codepage(const Sauce *self)
{
    if (!likely_text(self)) return 0;
    switch (self->cp)
    {
	case CP_DEFAULT: return is_text(self) ? "<437 (default)>" : 0;
	case CP_IMPLICIT: return "<437 (implicit)>";
	case CP_NAMED: return self->cpname;
	case CP_OTHER: return is_text(self) ? "<Other (unknown)>" : 0;
	default: return 0;
    }
}

static const char *validCpStr(const Sauce *self)
{
    if (self->datatype != 1 || self->filetype > 2) return 0;
    if (self->cp == CP_DEFAULT || self->cp == CP_OTHER) return 0;
    if (self->cp == CP_IMPLICIT) return "437";
    return self->cpname;
}

int Sauce_cpid(const Sauce *self)
{
    const char *cpname = validCpStr(self);
    if (!cpname) return -1;
    return CodepageId_byName(cpname);
}

int Sauce_cpflags(const Sauce *self)
{
    const char *cpname = validCpStr(self);
    if (!cpname) return CPF_NONE;
    return CodepageFlags_byName(cpname);
}

int Sauce_scrheight(const Sauce *self)
{
    return self->scrheight ? self->scrheight : -1;
}

int Sauce_comments(const Sauce *self)
{
    return self->lines;
}

const char *Sauce_comment(const Sauce *self, int lineno)
{
    if (lineno < 0 || lineno >= self->lines) return 0;
    return self->comment[lineno];
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

