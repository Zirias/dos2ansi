#include "saucequery.h"

#include "bufferedwriter.h"
#include "sauce.h"
#include "stream.h"
#include "util.h"

#include <string.h>
#include <time.h>

static const char *fields = "TAGDtwhbsafc";

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
