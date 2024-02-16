#include "saucequery.h"

#include "bufferedwriter.h"
#include "codepage.h"
#include "sauce.h"
#include "stream.h"
#include "unicodewriter.h"
#include "util.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char *fields = "TAGDtwhbsafc";

typedef struct CP437Writer
{
    StreamWriter base;
    Codepage *cp;
} CP437Writer;

static size_t writecp437(StreamWriter *self, const void *ptr, size_t sz)
{
    if (!sz) return 0;
    CP437Writer *writer = (CP437Writer *)self;
    uint8_t c = (uint8_t)*((char *)ptr);
    uint16_t uc;
    if (c == U'\r' || c == U'\n') uc = c;
    else uc = Codepage_map(writer->cp, c);
    return Stream_write(self->stream, &uc, 2) / 2;
}

static void destroycp437(StreamWriter *self)
{
    if (!self) return;
    CP437Writer *writer = (CP437Writer *)self;
    Stream_destroy(self->stream);
    Codepage_destroy(writer->cp);
    free(writer);
}

static Stream *CP437Writer_create(Stream *out)
{
    CP437Writer *writer = xmalloc(sizeof *writer);
    writer->base.write = writecp437;
    writer->base.flush = 0;
    writer->base.status = 0;
    writer->base.destroy = destroycp437;
    writer->base.stream = out;
    writer->cp = Codepage_create(CP_437, CPF_SOLIDBAR);
    return Stream_createWriter((StreamWriter *)writer, fields);
}

const char *SauceQuery_check(const char *query)
{
    for (const char *f = query; *f; ++f)
    {
	if (!(strchr(fields, *f))) return f;
    }
    return 0;
}

int SauceQuery_print(const Sauce *sauce, const char *query, int crlf)
{
    if (SauceQuery_check(query)) return -1;
    Stream *out = Stream_createStandard(SST_STDOUT);
    out = BufferedWriter_create(out, 1024);
    out = UnicodeWriter_create(out, UF_UTF8);
    out = CP437Writer_create(out);
    for (const char *f = query; *f; ++f)
    {
	switch (*f)
	{
	    const char *sv;
	    char dv[10];
	    time_t tv;
	    int iv;

	    case 'T':
		sv = Sauce_title(sauce);
		goto stringorempty;

	    case 'A':
		sv = Sauce_author(sauce);
		goto stringorempty;

	    case 'G':
		sv = Sauce_group(sauce);
		goto stringorempty;

	    case 'D':
		dv[0] = 0;
		sv = dv;
		tv = Sauce_date(sauce);
		if (tv != (time_t)(-1)) strftime(dv, sizeof dv,
			"%Y%m%d", localtime(&tv));
		goto string;

	    case 't':
		sv = Sauce_type(sauce);
		if (*sv == '<') sv = "";
		goto string;

	    case 'w':
		iv = Sauce_width(sauce);
		goto numorempty;

	    case 'h':
		iv = Sauce_height(sauce);
		goto numorempty;

	    case 'b':
		iv = Sauce_nonblink(sauce);
		goto numorempty;

	    case 's':
		iv = Sauce_letterspacing(sauce);
		goto numorempty;

	    case 'a':
		iv = Sauce_squarepixels(sauce);
		goto numorempty;

	    case 'f':
		sv = Sauce_font(sauce);
		if (*sv == '<') sv = "";
		goto string;

	    case 'c':
		sv = Sauce_codepage(sauce);
		if (*sv == '<') sv = "";
		goto string;

	    default:
		break;

	    stringorempty:
		if (!sv) sv = "";
	    string:
		Stream_printf(out, crlf ? "%s\r\n" : "%s\n", sv);
		break;

	    numorempty:
		if (iv < 0) Stream_puts(out, crlf ? "\r\n" : "\n");
		else Stream_printf(out, crlf ? "%d\r\n" : "%d\n", iv);
		break;
	}
    }
    int rc = -(Stream_status(out) != SS_OK);
    Stream_destroy(out);
    return rc;
}
