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

static const char *fields = ":TAGDCtwhbsafc";

typedef struct CP437Writer
{
    StreamWriter base;
    Codepage *cp;
    int quote;
    int crlf;
    int eol;
} CP437Writer;

static size_t writecp437(StreamWriter *self, const void *ptr, size_t sz)
{
    if (!sz) return 0;
    CP437Writer *writer = (CP437Writer *)self;
    uint16_t uc;
    if (writer->eol)
    {
	writer->eol = 0;
	if (writer->quote)
	{
	    uc = U'\'';
	    Stream_write(self->stream, &uc, 2);
	}
    }
    uint8_t c = (uint8_t)*((char *)ptr);
    if (c == U'\r') return 1;
    if (c == U'\n')
    {
	writer->eol = 1;
	if (writer->quote)
	{
	    uc = U'\'';
	    Stream_write(self->stream, &uc, 2);
	}
	if (writer->crlf)
	{
	    uc = U'\r';
	    Stream_write(self->stream, &uc, 2);
	}
	uc = U'\n';
	return Stream_write(self->stream, &uc, 2) / 2;
    }
    if (writer->quote && c == U'\'')
    {
	uc = U'\\';
	Stream_write(self->stream, &uc, 2);
    }
    uc = Codepage_map(writer->cp, c);
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

static Stream *CP437Writer_create(Stream *out, int crlf)
{
    CP437Writer *writer = xmalloc(sizeof *writer);
    writer->base.write = writecp437;
    writer->base.flush = 0;
    writer->base.status = 0;
    writer->base.destroy = destroycp437;
    writer->base.stream = out;
    writer->cp = Codepage_create(CP_437, CPF_SOLIDBAR);
    writer->quote = 0;
    writer->crlf = !!crlf;
    writer->eol = 1;
    return Stream_createWriter((StreamWriter *)writer, fields);
}

static void CP437Writer_setQuote(Stream *stream, int quote)
{
    StreamWriter *self = Stream_writer(stream, fields);
    if (!self) return;
    ((CP437Writer *)self)->quote = !!quote;
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
    out = CP437Writer_create(out, crlf);
    int quote = 0;
    for (const char *f = query; *f; ++f)
    {
	switch (*f)
	{
	    const char *sv;
	    char dv[10];
	    time_t tv;
	    int iv;

	    case ':':
		quote = !quote;
		CP437Writer_setQuote(out, quote);
		break;

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

	    case 'C':
		iv = Sauce_comments(sauce);
		Stream_printf(out, "%d\n", iv);
		for (int i = 0; i < iv; ++i)
		{
		    Stream_printf(out, "%s\n", Sauce_comment(sauce, i));
		}
		break;

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
		Stream_printf(out, "%s\n", sv);
		break;

	    numorempty:
		if (iv < 0) Stream_puts(out, "\n");
		else Stream_printf(out, "%d\n", iv);
		break;
	}
    }
    int rc = -(Stream_status(out) != SS_OK);
    Stream_destroy(out);
    return rc;
}
