#include "sauceprinter.h"

#include "sauce.h"
#include "vgacanvas.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#define isgfx(c) ((unsigned char)(c) >= 0xb0)

typedef struct SauceWrapper
{
    const Sauce *sauce;
    const char *line;
    const char *rest;
    int join;
    int joinnext;
    int nowrap;
    int nowrapnext;
    int donl;
    int donlnext;
    int nextline;
    int buflen;
    char linebuf[65];
} SauceWrapper;

static void SauceWrapper_checkline(SauceWrapper *self)
{
    self->joinnext = 0;
    self->nowrapnext = 0;
    self->donlnext = 0;
    if (self->line)
    {
	int newlen = strlen(self->line);
	if (!newlen) self->nowrapnext = 1;
	else for (int i = 0; i < newlen; ++i)
	{
	    if (isgfx(self->line[i]))
	    {
		self->nowrapnext = 1;
		break;
	    }
	}
	if (!self->nowrapnext)
	{
	    if (newlen < 62) self->donlnext = 1;
	    else if (newlen == 64
		    && self->line[63] != 0x20) self->joinnext = 1;
	}
    }
}

static void SauceWrapper_init(SauceWrapper *self, const Sauce *sauce)
{
    memset(self, 0, sizeof *self);
    self->sauce = sauce;
    self->line = Sauce_comment(sauce, 0);
    SauceWrapper_checkline(self);
}

static int SauceWrapper_fetch(SauceWrapper *self)
{
    if (!self->line) return 0;
    self->rest = self->line;
    self->line = Sauce_comment(self->sauce, ++self->nextline);
    self->join = self->joinnext && self->line && *self->line != 0x20;
    self->nowrap = self->nowrapnext;
    self->donl = self->donlnext;
    SauceWrapper_checkline(self);
    if (self->nowrapnext) self->join = 0;
    return 1;
}

static const char *SauceWrapper_line(SauceWrapper *self)
{
    if (!self->rest) SauceWrapper_fetch(self);
    if (self->nowrap)
    {
	const char *line = self->rest;
	self->rest = 0;
	return line;
    }

    int haveword = 0;
    int len = 0;
    while (self->rest || (SauceWrapper_fetch(self) && !self->nowrap))
    {
	int joined = 0;
	const char *rp = self->rest;
	while (*rp && *rp == 0x20 && self->buflen + len < 64)
	{
	    if (self->buflen) self->linebuf[self->buflen + len++] = *rp++;
	    else ++rp;
	}
	if (self->buflen + len == 64) break;
	if (!*rp && self->join && !joined && self->line)
	{
	    rp = self->line;
	    joined = 1;
	    while (*rp && *rp == 0x20 && self->buflen + len < 64)
	    {
		self->linebuf[self->buflen + len++] = *rp++;
	    }
	}
	if (self->buflen + len == 64) break;
	if (len) haveword = 1;
	while (*rp && *rp != 0x20 && self->buflen + len < 64)
	{
	    self->linebuf[self->buflen + len++] = *rp++;
	}
	if (!*rp && self->join && !joined && self->line)
	{
	    rp = self->line;
	    joined = 1;
	    while (*rp && *rp != 0x20 && self->buflen + len < 64)
	    {
		self->linebuf[self->buflen + len++] = *rp++;
	    }
	}
	if (haveword && self->buflen + len == 64 &&
		((self->line && *rp != 0x20) || (!self->line && *rp))) break;
	self->buflen += len;
	len = 0;
	if (joined)
	{
	    self->line = rp;
	    self->rest = 0;
	}
	else if (!*rp)
	{
	    self->rest = 0;
	    if (self->donl) break;
	}
	else self->rest = rp;
    }
    if (self->buflen)
    {
	self->linebuf[self->buflen] = 0;
	self->buflen = 0;
	return self->linebuf;
    }
    return 0;
}

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

static const int widths[][3] =
{
    {16, 52, 7},
    {8, 65, 2}
};

static void putpair(VgaCanvas *canvas, int layout,
	const char *key, const char *val)
{
    VgaCanvas_put(canvas, (char)0xba);
    VgaCanvas_setFg(canvas, 3);
    putright(canvas, widths[layout][0], ' ', key);
    VgaCanvas_put(canvas, *key?':':' ');
    VgaCanvas_put(canvas, ' ');
    VgaCanvas_setBg(canvas, 7);
    VgaCanvas_setFg(canvas, 4);
    VgaCanvas_setBold(canvas, 0);
    VgaCanvas_put(canvas, ' ');
    putleft(canvas, widths[layout][1], ' ', val);
    VgaCanvas_setBg(canvas, 4);
    VgaCanvas_setFg(canvas, 7);
    VgaCanvas_setBold(canvas, 1);
    for (int i = 0; i < widths[layout][2]; ++i) VgaCanvas_put(canvas, ' ');
    VgaCanvas_put(canvas, (char)0xba);
}

int SaucePrinter_print(VgaCanvas *canvas, const Sauce *sauce, int nowrap)
{
    char buf[32];
    int lines = 0;

    VgaCanvas_setBg(canvas, 4);
    VgaCanvas_setFg(canvas, 7);
    VgaCanvas_setBold(canvas, 1);

    putleft(canvas, 79, (char)0xcd, "\xc9\xcd\xcd\xcd\xcd\xb5 SAUCE \xc6");
    VgaCanvas_put(canvas, (char)0xbb);
    ++lines;

    const char *title = Sauce_title(sauce);
    if (!title) title = "<Unnamed>";
    putpair(canvas, 0, "Title", title);
    ++lines;
    const char *author = Sauce_author(sauce);
    if (!author) author = "<Unknown>";
    putpair(canvas, 0, "Author", author);
    ++lines;
    const char *group = Sauce_group(sauce);
    if (group) putpair(canvas, 0, "Group", group), ++lines;

    time_t date = Sauce_date(sauce);
    const char *datestr = "<Unknown>";
    if (date != (time_t)(-1))
    {
	strftime(buf, sizeof buf, "%d %b %Y", localtime(&date));
	datestr = buf;
    }
    putpair(canvas, 0, "Date", datestr);
    ++lines;

    VgaCanvas_put(canvas, (char)0xc7);
    for (int i = 0; i < 78; ++i) VgaCanvas_put(canvas, (char)0xc4);
    VgaCanvas_put(canvas, (char)0xb6);
    ++lines;

    putpair(canvas, 0, "File type", Sauce_type(sauce));
    ++lines;
    int width = Sauce_width(sauce);
    int height = Sauce_height(sauce);
    if (width >= 0)
    {
	if (height >= 0)
	{
	    sprintf(buf, "%d x %d", width, height);
	    putpair(canvas, 0, "Size", buf);
	}
	else
	{
	    sprintf(buf, "%d", width);
	    putpair(canvas, 0, "Width", buf);
	}
	++lines;
    }
    else if (height >= 0)
    {
	sprintf(buf, "%d", height);
	putpair(canvas, 0, "Height", buf);
	++lines;
    }
    int nonblink = Sauce_nonblink(sauce);
    if (nonblink >= 0) putpair(canvas, 0, "Background mode", nonblink ?
	    "bright colors" : "blinking (default)"), ++lines;
    int letterspacing = Sauce_letterspacing(sauce);
    if (letterspacing >= 0) putpair(canvas, 0, "Letter spacing",
	    letterspacing ? "spaced (9px VGA)" : "unspaced (8px VGA)"), ++lines;
    int squarepixels = Sauce_squarepixels(sauce);
    if (squarepixels >= 0) putpair(canvas, 0, "Pixel aspect", squarepixels ?
	    "square pixels" : "rectangular pixels"), ++lines;
    const char *font = Sauce_font(sauce);
    if (font) putpair(canvas, 0, "Font", font), ++lines;
    const char *codepage = Sauce_codepage(sauce);
    if (codepage) putpair(canvas, 0, "Codepage", codepage), ++lines;

    int comments = Sauce_comments(sauce);
    if (comments)
    {
	VgaCanvas_put(canvas, (char)0xc7);
	for (int i = 0; i < 78; ++i) VgaCanvas_put(canvas, (char)0xc4);
	VgaCanvas_put(canvas, (char)0xb6);
	++lines;

	if (nowrap)
	{
	    for (int i = 0; i < comments; ++i)
	    {
		putpair(canvas, 1, i?"":"Comment", Sauce_comment(sauce, i));
		++lines;
	    }
	}
	else
	{
	    SauceWrapper w;
	    SauceWrapper_init(&w, sauce);
	    const char *cap = "Comment";
	    const char *line = 0;
	    while ((line = SauceWrapper_line(&w)))
	    {
		putpair(canvas, 1, cap, line);
		cap = "";
		++lines;
	    }
	}
    }

    VgaCanvas_put(canvas, (char)0xc8);
    putright(canvas, 79, (char)0xcd, "\xb5 \3  dos2ansi v"
	    DOS2ANSIVERSTR "  \16 \xc6\xcd\xcd\xcd\xcd\xbc");
    return ++lines;
}

