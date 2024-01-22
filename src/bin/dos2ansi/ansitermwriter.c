#include "ansitermwriter.h"

#include "vgacanvas.h"

#include <stdint.h>
#include <stdio.h>

static const uint16_t lowascii[] = {
    0x0020,0x263A,0x263B,0x2665,0x2666,0x2663,0x2660,0x2022,
    0x25D8,0x25CB,0x25D9,0x2642,0x2640,0x266A,0x266B,0x263C,
    0x25BA,0x25C4,0x2195,0x203C,0x00B6,0x00A7,0x25AC,0x21AB,
    0x2191,0x2193,0x2192,0x2190,0x221F,0x2194,0x25B2,0x25BC
};

static const uint16_t cp437high[] = {
    0x2302,
    0x00C7,0x00FC,0x00E9,0x00E2,0x00E4,0x00E0,0x00E5,0x00E7,
    0x00EA,0x00EB,0x00E8,0x00EF,0x00EE,0x00EC,0x00C4,0x00C5,
    0x00C9,0x00E6,0x00C6,0x00F4,0x00F6,0x00F2,0x00FB,0x00F9,
    0x00FF,0x00D6,0x00DC,0x00A2,0x00A3,0x00A5,0x20A7,0x0192,
    0x00E1,0x00ED,0x00F3,0x00FA,0x00F1,0x00D1,0x00AA,0x00BA,
    0x00BF,0x2310,0x00AC,0x00BD,0x00BC,0x00A1,0x00AB,0x00BB,
    0x2591,0x2592,0x2593,0x2502,0x2524,0x2561,0x2562,0x2556,
    0x2555,0x2563,0x2551,0x2557,0x255D,0x255C,0x255B,0x2510,
    0x2514,0x2534,0x252C,0x251C,0x2500,0x253C,0x255E,0x255F,
    0x255A,0x2554,0x2569,0x2566,0x2560,0x2550,0x256C,0x2567,
    0x2568,0x2564,0x2565,0x2559,0x2558,0x2552,0x2553,0x256B,
    0x256A,0x2518,0x250C,0x2588,0x2584,0x258C,0x2590,0x2580,
    0x03B1,0x00DF,0x0393,0x03C0,0x03A3,0x03C3,0x00B5,0x03C4,
    0x03A6,0x0398,0x03A9,0x03B4,0x221E,0x03C6,0x03B5,0x2229,
    0x2261,0x00B1,0x2265,0x2264,0x2320,0x2321,0x00F7,0x2248,
    0x00B0,0x2219,0x00B7,0x221A,0x207F,0x00B2,0x25A0,0x00A0
};

static char buf[1024];
static size_t bufsz = 0;

static int writebuf(FILE *file)
{
    if (!bufsz) return 0;
    size_t bufpos = 0;
    while (bufpos < bufsz)
    {
	size_t written = fwrite(buf+bufpos, 1, bufsz-bufpos, file);
	if (!written) return -1;
	bufpos += written;
    }
    bufsz = 0;
    return 0;
}

static int putbuf(FILE *file, char c)
{
    if (bufsz == sizeof buf)
    {
	if (writebuf(file) != 0) return -1;
    }
    buf[bufsz++] = c;
    return 0;
}

static int toutf8(FILE *file, uint16_t c)
{
    if (c < 0x80) return putbuf(file, c);
    unsigned char lb = c & 0xff;
    unsigned char hb = c >> 8;
    if (c < 0x800)
    {
	if (putbuf(file, 0xc0U | (hb << 2) | (lb >> 6)) != 0) return -1;
	if (putbuf(file, 0x80U | (lb & 0x3fU)) != 0) return -1;
    }
    else
    {
	if (putbuf(file, 0xe0U | (hb >> 4)) != 0) return -1;
	if (putbuf(file, 0x80U | (hb << 2) | (lb >> 6)) != 0) return -1;
	if (putbuf(file, 0x80U | (lb & 0x3fU)) != 0) return -1;
    }
    return 0;
}

static int writeansi(FILE *file, int newbg, int bg, int newfg, int fg)
{
    char nextarg = '[';
    if (putbuf(file, 0x1b) != 0) return -1;
    if ((newbg & 0x08U) != (bg & 0x08U))
    {
	if (putbuf(file, nextarg) != 0) return -1;
	nextarg = ';';
	if (newbg & 0x08U)
	{
	    if (putbuf(file, '5') != 0) return -1;
	}
	else
	{
	    if (putbuf(file, '2') != 0) return -1;
	    if (putbuf(file, '5') != 0) return -1;
	}
    }
    if ((newfg & 0x08U) != (fg & 0x08U))
    {
	if (putbuf(file, nextarg) != 0) return -1;
	nextarg = ';';
	if (newfg & 0x08U)
	{
	    if (putbuf(file, '1') != 0) return -1;
	}
	else
	{
	    if (putbuf(file, '2') != 0) return -1;
	    if (putbuf(file, '2') != 0) return -1;
	}
    }
    if ((newbg & 0x07U) != (bg & 0x07U))
    {
	if (putbuf(file, nextarg) != 0) return -1;
	nextarg = ';';
	if (putbuf(file, '4') != 0) return -1;
	if (putbuf(file, (newbg & 0x07U) + '0') != 0) return -1;
    }
    if ((newfg & 0x07U) != (fg & 0x07U))
    {
	if (putbuf(file, nextarg) != 0) return -1;
	nextarg = ';';
	if (putbuf(file, '3') != 0) return -1;
	if (putbuf(file, (newfg & 0x07U) + '0') != 0) return -1;
    }
    return putbuf(file, 'm');
}

int AnsiTermWriter_write(FILE *file, const VgaCanvas *canvas)
{
    int bg = -1;
    int fg = -1;

    size_t height = VgaCanvas_height(canvas);
    if (!height) return 0;

    for (size_t i = 0; i < height; ++i)
    {
	const VgaLine *line = VgaCanvas_line(canvas, i);
	int len = VgaLine_len(line);
	for (int j = 0; j < len; ++j)
	{
	    int newbg = VgaLine_bg(line, j);
	    int newfg = VgaLine_fg(line, j);
	    if (newbg != bg || newfg != fg)
	    {
		if (writeansi(file, newbg, bg, newfg, fg) != 0) return -1;
		bg = newbg;
		fg = newfg;
	    }
	    unsigned char vgachr = VgaLine_chr(line, j);
	    uint16_t unichr;
	    if (vgachr < 0x20U) unichr = lowascii[vgachr];
	    else if (vgachr >= 0x7fU) unichr = cp437high[vgachr-0x7fU];
	    else unichr = vgachr;
	    if (toutf8(file, unichr) != 0) return -1;
	}
	if (i == height-1)
	{
	    if (putbuf(file, 0x1b) != 0) return -1;
	    if (putbuf(file, '[') != 0) return -1;
	    if (putbuf(file, 'm') != 0) return -1;
	}
	if (putbuf(file, '\n') != 0) return -1;
    }
    return writebuf(file);
}

