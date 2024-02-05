#include "sauceprinter.h"

#include "sauce.h"
#include "vgacanvas.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

static void putright(VgaCanvas *canvas,
	unsigned width, char pad, const char *str)
{
    size_t len = strlen(str);
    if (len > width) str += (len - width);
    else while (len++ < width) VgaCanvas_put(canvas, pad);
    while (*str) VgaCanvas_put(canvas, *str++);
}

static void putleft(VgaCanvas *canvas,
	unsigned width, char pad, const char *str)
{
    unsigned len = 0;
    while (*str && len++ < width) VgaCanvas_put(canvas, *str++);
    while (len++ < width) VgaCanvas_put(canvas, pad);
}

static void putpair(VgaCanvas *canvas, const char *key, const char *val)
{
    VgaCanvas_put(canvas, 0xba);
    VgaCanvas_setFg(canvas, 3);
    putright(canvas, 16, ' ', key);
    VgaCanvas_put(canvas, ':');
    VgaCanvas_put(canvas, ' ');
    VgaCanvas_setBg(canvas, 7);
    VgaCanvas_setFg(canvas, 4);
    VgaCanvas_setBold(canvas, 0);
    VgaCanvas_put(canvas, ' ');
    putleft(canvas, 52, ' ', val);
    VgaCanvas_setBg(canvas, 4);
    VgaCanvas_setFg(canvas, 7);
    VgaCanvas_setBold(canvas, 1);
    for (int i = 0; i < 7; ++i) VgaCanvas_put(canvas, ' ');
    VgaCanvas_put(canvas, 0xba);
}

void SaucePrinter_print(VgaCanvas *canvas, const Sauce *sauce)
{
    char buf[32];

    VgaCanvas_setBg(canvas, 4);
    VgaCanvas_setFg(canvas, 7);
    VgaCanvas_setBold(canvas, 1);

    VgaCanvas_put(canvas, 0xc9);
    for (int i = 0; i < 78; ++i) VgaCanvas_put(canvas, 0xcd);
    VgaCanvas_put(canvas, 0xbb);

    putpair(canvas, "Title", Sauce_title(sauce));
    putpair(canvas, "Author", Sauce_author(sauce));
    putpair(canvas, "Group", Sauce_group(sauce));

    time_t date = Sauce_date(sauce);
    const char *datestr = "";
    if (date != (time_t)(-1))
    {
	strftime(buf, sizeof buf, "%d %b %Y", gmtime(&date));
	datestr = buf;
    }
    putpair(canvas, "Date", datestr);

    VgaCanvas_put(canvas, 0xc7);
    for (int i = 0; i < 78; ++i) VgaCanvas_put(canvas, 0xc4);
    VgaCanvas_put(canvas, 0xb6);

    putpair(canvas, "File type", Sauce_type(sauce));
    int width = Sauce_width(sauce);
    int height = Sauce_height(sauce);

    if (width >= 0)
    {
	if (height >= 0)
	{
	    sprintf(buf, "%d x %d", width, height);
	    putpair(canvas, "Size", buf);
	}
	else
	{
	    sprintf(buf, "%d", width);
	    putpair(canvas, "Width", buf);
	}
    }
    else if (height >= 0)
    {
	sprintf(buf, "%d", height);
	putpair(canvas, "Height", buf);
    }

    VgaCanvas_put(canvas, 0xc8);
    for (int i = 0; i < 78; ++i) VgaCanvas_put(canvas, 0xcd);
    VgaCanvas_put(canvas, 0xbc);
}

