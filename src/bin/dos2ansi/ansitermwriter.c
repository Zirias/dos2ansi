#include "ansitermwriter.h"

#include "stream.h"
#include "vgacanvas.h"

#include <stdint.h>
#include <string.h>

static const uint16_t lowascii[] = {
    0x0020,0x263A,0x263B,0x2665,0x2666,0x2663,0x2660,0x2022,
    0x25D8,0x25CB,0x25D9,0x2642,0x2640,0x266A,0x266B,0x263C,
    0x25BA,0x25C4,0x2195,0x203C,0x00B6,0x00A7,0x2582,0x21A8,
    0x2191,0x2193,0x2192,0x2190,0x221F,0x2194,0x25B2,0x25BC
};

static const uint16_t codepages[][0x80] = {
    /* CP-437 */
    {
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
    },

    /* CP-708 */
    {
	0x2502,0x2524,0x00E9,0x00E2,0x2561,0x00E0,0x2562,0x00E7,
	0x00EA,0x00EB,0x00E8,0x00EF,0x00EE,0x2556,0x2555,0x2563,
	0x2551,0x2557,0x255D,0x00F4,0x255C,0x255C,0x255B,0x00F9,
	0x2510,0x2514,0x25AE,0x25AE,0x25AE,0x25AE,0x25AE,0x25AE,
	0x25AE,0x2534,0x252C,0x251C,0x00A4,0x2500,0x253C,0x255E,
	0x255F,0x255A,0x2554,0x2569,0x060C,0x2566,0x00AB,0x00BB,
	0x2591,0x2592,0x2593,0x2560,0x2550,0x256C,0x2567,0x2568,
	0x2564,0x2565,0x2559,0x061B,0x2558,0x2552,0x2553,0x061F,
	0x256B,0xFE80,0xFE88,0xFE83,0xFE85,0xFE87,0xFE89,0xFE8D,
	0xFE8F,0xFE93,0xFE95,0xFE99,0xFE9D,0xFEA1,0xFEA5,0xFEA9,
	0xFEAB,0xFEAD,0xFEAF,0xFEB1,0xFEB5,0xFEB9,0xFEBD,0xFEC1,
	0xFEC5,0xFEC9,0xFECD,0x2588,0x2584,0x258C,0x2590,0x2580,
	0x0640,0xFED1,0xFED5,0xFED9,0xFEDD,0xFEE1,0xFEE5,0xFEE9,
	0xFEED,0xFEEF,0xFEF1,0xFE70,0xFE72,0xFE74,0xFE76,0xFE78,
	0xFE7A,0xFE7C,0xFE7E,0x25AE,0x25AE,0x25AE,0x25AE,0x25AE,
	0x25AE,0x256A,0x2518,0x250C,0x00B5,0x00A3,0x25A0,0x00A0
    },

    /* CP-720 */
    {
	0x25AE,0x25AE,0x00E9,0x00E2,0x25AE,0x00E0,0x25AE,0x00E7,
	0x00EA,0x00EB,0x00E8,0x00EF,0x00EE,0x25AE,0x25AE,0x25AE,
	0x25AE,0xFE7C,0xFE7E,0x00F4,0x00A4,0x0640,0x00FB,0x00F9,
	0xFE80,0xFE81,0xFE83,0xFE85,0x00A3,0xFE87,0xFE89,0xFE8D,
	0xFE8F,0xFE93,0xFE95,0xFE99,0xFE9D,0xFEA1,0xFEA5,0xFEA9,
	0xFEAB,0xFEAD,0xFEAF,0xFEB1,0xFEB5,0xFEB9,0x00AB,0x00BB,
	0x2591,0x2592,0x2593,0x2502,0x2524,0x2561,0x2562,0x2556,
	0x2555,0x2563,0x2551,0x2557,0x255D,0x255C,0x255B,0x2510,
	0x2514,0x2534,0x252C,0x251C,0x2500,0x253C,0x255E,0x255F,
	0x255A,0x2554,0x2569,0x2566,0x2560,0x2550,0x256C,0x2567,
	0x2568,0x2564,0x2565,0x2559,0x2558,0x2552,0x2553,0x256B,
	0x256A,0x2518,0x250C,0x2588,0x2584,0x258C,0x2590,0x2580,
	0xFEBD,0xFEC1,0xFEC5,0xFEC9,0xFECD,0xFED1,0x00B5,0xFED5,
	0xFED9,0xFEDD,0xFEE1,0xFEE5,0xFEE9,0xFEED,0xFEEF,0xFEF1,
	0x2261,0xFE70,0xFE72,0xFE74,0xFE76,0xFE78,0xFE7A,0x2248,
	0x00B0,0x2219,0x00B7,0x221A,0x207F,0x00B2,0x25A0,0x00A0
    },

    /* CP-737 */
    {
	0x0391,0x0392,0x0393,0x0394,0x0395,0x0396,0x0397,0x0398,
	0x0399,0x039A,0x039B,0x039C,0x039D,0x039E,0x039F,0x03A0,
	0x03A1,0x03A3,0x03A4,0x03A5,0x03A6,0x03A7,0x03A8,0x03A9,
	0x03B1,0x03B2,0x03B3,0x03B4,0x03B5,0x03B6,0x03B7,0x03B8,
	0x03B9,0x03BA,0x03BB,0x03BC,0x03BD,0x03BE,0x03BF,0x03C0,
	0x03C1,0x03C3,0x03C2,0x03C4,0x03C5,0x03C6,0x03C7,0x03C8,
	0x2591,0x2592,0x2593,0x2502,0x2524,0x2561,0x2562,0x2556,
	0x2555,0x2563,0x2551,0x2557,0x255D,0x255C,0x255B,0x2510,
	0x2514,0x2534,0x252C,0x251C,0x2500,0x253C,0x255E,0x255F,
	0x255A,0x2554,0x2569,0x2566,0x2560,0x2550,0x256C,0x2567,
	0x2568,0x2564,0x2565,0x2559,0x2558,0x2552,0x2553,0x256B,
	0x256A,0x2518,0x250C,0x2588,0x2584,0x258C,0x2590,0x2580,
	0x03C9,0x03AC,0x03AD,0x03AE,0x03CA,0x03AF,0x03CC,0x03CD,
	0x03CB,0x03CE,0x0386,0x0388,0x0389,0x038A,0x038C,0x038E,
	0x038F,0x00B1,0x2265,0x2264,0x03AA,0x03AB,0x00F7,0x2248,
	0x00B0,0x2219,0x00B7,0x221A,0x207F,0x00B2,0x25A0,0x00A0
    },

    /* CP-775 */
    {
	0x0106,0x00FC,0x00E9,0x0101,0x00E4,0x0123,0x00E5,0x0107,
	0x0142,0x0113,0x0156,0x0157,0x012B,0x0179,0x00C4,0x00C5,
	0x00C9,0x00E6,0x00C6,0x014D,0x00F6,0x0122,0x00A2,0x015A,
	0x015B,0x00D6,0x00DC,0x00F8,0x00A3,0x00D8,0x00D7,0x00A4,
	0x0100,0x012A,0x00F3,0x017B,0x017C,0x017A,0x201D,0x00A6,
	0x00A9,0x00AE,0x00AC,0x00BD,0x00BC,0x0141,0x00AB,0x00BB,
	0x2591,0x2592,0x2593,0x2502,0x2524,0x0104,0x010C,0x0118,
	0x0116,0x2563,0x2551,0x2557,0x255D,0x012E,0x0160,0x2510,
	0x2514,0x2534,0x252C,0x251C,0x2500,0x253C,0x0172,0x016A,
	0x255A,0x2554,0x2569,0x2566,0x2560,0x2550,0x256C,0x2567,
	0x0105,0x010D,0x0119,0x0117,0x012F,0x0161,0x0173,0x016B,
	0x017E,0x2518,0x250C,0x2588,0x2584,0x258C,0x2590,0x2580,
	0x00D1,0x00DF,0x014C,0x0143,0x00F5,0x00D5,0x00B5,0x0144,
	0x0136,0x0137,0x013B,0x013C,0x0146,0x0112,0x0145,0x2019,
	0x00AD,0x00B1,0x201C,0x00BE,0x00B6,0x00A7,0x00F7,0x201E,
	0x00B0,0x2219,0x00B7,0x00B9,0x00B3,0x00B2,0x25A0,0x00A0
    },

    /* CP-850 */
    {
	0x00C7,0x00FC,0x00E9,0x00E2,0x00E4,0x00E0,0x00E5,0x00E7,
	0x00EA,0x00EB,0x00E8,0x00EF,0x00EE,0x00EC,0x00C4,0x00C5,
	0x00C9,0x00E6,0x00C6,0x00F4,0x00F6,0x00F2,0x00FB,0x00F9,
	0x00FF,0x00D6,0x00DC,0x00F8,0x00A3,0x00D8,0x00D7,0x0192,
	0x00E1,0x00ED,0x00F3,0x00FA,0x00F1,0x00D1,0x00AA,0x00BA,
	0x00BF,0x00AE,0x00AC,0x00BD,0x00BC,0x00A1,0x00AB,0x00BB,
	0x2591,0x2592,0x2593,0x2502,0x2524,0x00C1,0x00C2,0x00C0,
	0x00A9,0x2563,0x2551,0x2557,0x255D,0x00A2,0x00A5,0x2510,
	0x2514,0x2534,0x252C,0x251C,0x2500,0x253C,0x00E3,0x00C3,
	0x255A,0x2554,0x2569,0x2566,0x2560,0x2550,0x256C,0x00A4,
	0x00F0,0x00D0,0x00CA,0x00CB,0x00C8,0x0131,0x00CD,0x00CE,
	0x00CF,0x2518,0x250C,0x2588,0x2584,0x00A6,0x00CC,0x2580,
	0x00D3,0x00DF,0x00D4,0x00D2,0x00F5,0x00D5,0x00B5,0x00FE,
	0x00DE,0x00DA,0x00DB,0x00D9,0x00FD,0x00DD,0x00AF,0x00B4,
	0x00AD,0x00B1,0x2017,0x00BE,0x00B6,0x00A7,0x00F7,0x00B8,
	0x00B0,0x00A8,0x00B7,0x00B9,0x00B3,0x00B2,0x25A0,0x00A0
    },

    /* CP-852 */
    {
	0x00C7,0x00FC,0x00E9,0x00E2,0x00E4,0x016F,0x0107,0x00E7,
	0x0142,0x00EB,0x0150,0x0151,0x00EE,0x0179,0x00C4,0x0106,
	0x00C9,0x0139,0x013A,0x00F4,0x00F6,0x013D,0x013E,0x015A,
	0x015B,0x00D6,0x00DC,0x0164,0x0165,0x0141,0x00D7,0x010D,
	0x00E1,0x00ED,0x00F3,0x00FA,0x0104,0x0105,0x017D,0x017E,
	0x0118,0x0119,0x00AC,0x017A,0x010C,0x015F,0x00AB,0x00BB,
	0x2591,0x2592,0x2593,0x2502,0x2524,0x00C1,0x00C2,0x011A,
	0x015E,0x2563,0x2551,0x2557,0x255D,0x017B,0x017C,0x2510,
	0x2514,0x2534,0x252C,0x251C,0x2500,0x253C,0x0102,0x0103,
	0x255A,0x2554,0x2569,0x2566,0x2560,0x2550,0x256C,0x00A4,
	0x0111,0x0110,0x010E,0x00CB,0x010F,0x0147,0x00CD,0x00CE,
	0x011B,0x2518,0x250C,0x2588,0x2584,0x0162,0x016E,0x2580,
	0x00D3,0x00DF,0x00D4,0x0143,0x0144,0x0148,0x0160,0x0161,
	0x0154,0x00DA,0x0155,0x0170,0x00FD,0x00DD,0x0163,0x00B4,
	0x00AD,0x02DD,0x02DB,0x02C7,0x02D8,0x00A7,0x00F7,0x00B8,
	0x00B0,0x00A8,0x02D9,0x0171,0x0158,0x0159,0x25A0,0x00A0
    },

    /* CP-858 */
    {
	0x00C7,0x00FC,0x00E9,0x00E2,0x00E4,0x00E0,0x00E5,0x00E7,
	0x00EA,0x00EB,0x00E8,0x00EF,0x00EE,0x00EC,0x00C4,0x00C5,
	0x00C9,0x00E6,0x00C6,0x00F4,0x00F6,0x00F2,0x00FB,0x00F9,
	0x00FF,0x00D6,0x00DC,0x00F8,0x00A3,0x00D8,0x00D7,0x0192,
	0x00E1,0x00ED,0x00F3,0x00FA,0x00F1,0x00D1,0x00AA,0x00BA,
	0x00BF,0x00AE,0x00AC,0x00BD,0x00BC,0x00A1,0x00AB,0x00BB,
	0x2591,0x2592,0x2593,0x2502,0x2524,0x00C1,0x00C2,0x00C0,
	0x00A9,0x2563,0x2551,0x2557,0x255D,0x00A2,0x00A5,0x2510,
	0x2514,0x2534,0x252C,0x251C,0x2500,0x253C,0x00E3,0x00C3,
	0x255A,0x2554,0x2569,0x2566,0x2560,0x2550,0x256C,0x00A4,
	0x00F0,0x00D0,0x00CA,0x00CB,0x00C8,0x20AC,0x00CD,0x00CE,
	0x00CF,0x2518,0x250C,0x2588,0x2584,0x00A6,0x00CC,0x2580,
	0x00D3,0x00DF,0x00D4,0x00D2,0x00F5,0x00D5,0x00B5,0x00FE,
	0x00DE,0x00DA,0x00DB,0x00D9,0x00FD,0x00DD,0x00AF,0x00B4,
	0x00AD,0x00B1,0x2017,0x00BE,0x00B6,0x00A7,0x00F7,0x00B8,
	0x00B0,0x00A8,0x00B7,0x00B9,0x00B3,0x00B2,0x25A0,0x00A0
    }
};

static const char cpnames[][4] = {
    "437",
    "708",
    "720",
    "737",
    "775",
    "850",
    "852",
    "858"
};

static char buf[1024];
static size_t bufsz = 0;
static int usecolors = 1;
static int usedefcols = 0;
static Codepage doscp = CP_437;
static UnicodeFormat outformat = UF_UTF8;
static int usebom = 1;
static int crlf = 0;
static int brokenpipe = -1;
static int markltr = 0;

static int writebuf(Stream *stream)
{
    if (!bufsz) return 0;
    size_t bufpos = 0;
    while (bufpos < bufsz)
    {
	size_t written = Stream_write(stream, buf+bufpos, bufsz-bufpos);
	if (!written) return -1;
	bufpos += written;
    }
    bufsz = 0;
    return 0;
}

static int putbuf(Stream *stream, char c)
{
    if (bufsz == sizeof buf)
    {
	if (writebuf(stream) != 0) return -1;
    }
    buf[bufsz++] = c;
    return 0;
}

static int toutf8(Stream *stream, uint16_t c)
{
    if (c < 0x80) return putbuf(stream, c);
    unsigned char lb = c & 0xff;
    unsigned char hb = c >> 8;
    if (c < 0x800)
    {
	if (putbuf(stream, 0xc0U | (hb << 2) | (lb >> 6)) != 0) return -1;
	if (putbuf(stream, 0x80U | (lb & 0x3fU)) != 0) return -1;
    }
    else
    {
	if (putbuf(stream, 0xe0U | (hb >> 4)) != 0) return -1;
	if (putbuf(stream,  0x80U | ((hb << 2) & 0x3fU)
		    | (lb >> 6)) != 0) return -1;
	if (putbuf(stream, 0x80U | (lb & 0x3fU)) != 0) return -1;
    }
    return 0;
}

static int toutf16(Stream *stream, uint16_t c)
{
    if (putbuf(stream, c >> 8) != 0) return -1;
    if (putbuf(stream, c & 0xffU) != 0) return -1;
    return 0;
}

static int toutf16le(Stream *stream, uint16_t c)
{
    if (putbuf(stream, c & 0xffU) != 0) return -1;
    if (putbuf(stream, c >> 8) != 0) return -1;
    return 0;
}

static int (*const outfuncs[])(Stream *, uint16_t) = {
    toutf8,
    toutf16,
    toutf16le
};

static int out(Stream *stream, uint16_t c)
{
    return outfuncs[outformat](stream, c);
}

static int writeansidc(Stream *stream, int newbg, int bg, int newfg, int fg,
	int defcols)
{
    char nextarg = '[';
    if (out(stream, 0x1b) != 0) return -1;
    if (defcols && newbg == 0x00 && newfg == 0x07)
    {
	if (out(stream, nextarg) != 0) return -1;
	goto done;
    }
    if (bg < 0 || (newbg & 0x08U) != (bg & 0x08U))
    {
	if (out(stream, nextarg) != 0) return -1;
	nextarg = ';';
	if (newbg & 0x08U)
	{
	    if (out(stream, '5') != 0) return -1;
	}
	else
	{
	    if (out(stream, '2') != 0) return -1;
	    if (out(stream, '5') != 0) return -1;
	}
    }
    if (fg < 0 || (newfg & 0x08U) != (fg & 0x08U))
    {
	if (out(stream, nextarg) != 0) return -1;
	nextarg = ';';
	if (newfg & 0x08U)
	{
	    if (out(stream, '1') != 0) return -1;
	}
	else
	{
	    if (out(stream, '2') != 0) return -1;
	    if (out(stream, '2') != 0) return -1;
	}
    }
    if (bg < 0 || (newbg & 0x07U) != (bg & 0x07U))
    {
	if (out(stream, nextarg) != 0) return -1;
	nextarg = ';';
	if (out(stream, '4') != 0) return -1;
	if (defcols && newbg == 0x00)
	{
	    if (out(stream, '9') != 0) return -1;
	}
	else if (out(stream, (newbg & 0x07U) + '0') != 0) return -1;
    }
    if (fg < 0 || (newfg & 0x07U) != (fg & 0x07U))
    {
	if (out(stream, nextarg) != 0) return -1;
	nextarg = ';';
	if (out(stream, '3') != 0) return -1;
	if (defcols && newfg == 0x07)
	{
	    if (out(stream, '9') != 0) return -1;
	}
	else if (out(stream, (newfg & 0x07U) + '0') != 0) return -1;
    }
done:
    return out(stream, 'm');
}

static int writeansi(Stream *stream, int newbg, int bg, int newfg, int fg)
{
    return writeansidc(stream, newbg, bg, newfg, fg, usedefcols);
}

void AnsiTermWriter_usecolors(int arg)
{
    usecolors = !!arg;
}

void AnsiTermWriter_usedefcols(int arg)
{
    usedefcols = !!arg;
}

void AnsiTermWriter_usecp(Codepage cp)
{
    doscp = cp;
}

void AnsiTermWriter_useformat(UnicodeFormat format)
{
    outformat = format;
}

void AnsiTermWriter_usebom(int arg)
{
    usebom = !!arg;
}

void AnsiTermWriter_crlf(int arg)
{
    crlf = !!arg;
}

void AnsiTermWriter_brokenpipe(int arg)
{
    brokenpipe = arg;
}

void AnsiTermWriter_markltr(int arg)
{
    markltr = !!arg;
}

Codepage AnsiTermWriter_cpbyname(const char *name)
{
    if ((name[0] == 'c' || name[0] == 'C')
	    && (name[1] == 'p' || name[1] == 'P'))
    {
	name += 2;
	if (*name == ' ' || *name == '-') ++name;
    }

    for (int i = 0; i < (int)(sizeof cpnames / sizeof *cpnames); ++i)
    {
	if (!strcmp(name, cpnames[i])) return i;
    }

    return -1;
}

static int usebrokenpipe(void)
{
    if (brokenpipe < 0)
    {
	for (int i = 0; i < 0x80; ++i)
	{
	    if (codepages[doscp][i] == 0x00a6U) return 0;
	}
	return 1;
    }
    else return brokenpipe;
}

int AnsiTermWriter_write(Stream *stream, const VgaCanvas *canvas)
{
    int bg = -1;
    int fg = -1;
    if (usedefcols)
    {
	bg = 0x00;
	fg = 0x07;
    }
    int dobrokenpipe = usebrokenpipe();

    size_t height = VgaCanvas_height(canvas);
    if (!height) return 0;

    if (usebom && out(stream, 0xfeffU) != 0) return -1;
    if (markltr && out(stream, 0x202dU) != 0) return -1;

    for (size_t i = 0; i < height; ++i)
    {
	const VgaLine *line = VgaCanvas_line(canvas, i);
	int len = VgaLine_len(line);
	for (int j = 0; j < len; ++j)
	{
	    if (usecolors && VgaCanvas_hascolor(canvas))
	    {
		int newbg = VgaLine_bg(line, j);
		int newfg = VgaLine_fg(line, j);
		if (newbg != bg || newfg != fg)
		{
		    if (writeansi(stream, newbg, bg, newfg, fg) != 0) return -1;
		    bg = newbg;
		    fg = newfg;
		}
	    }
	    unsigned char vgachr = VgaLine_chr(line, j);
	    uint16_t unichr;
	    if (vgachr < 0x20U) unichr = lowascii[vgachr];
	    else if (vgachr >= 0x80U) unichr = codepages[doscp][vgachr-0x80U];
	    else if (vgachr == 0x7fU) unichr = 0x2302U;
	    else if (dobrokenpipe && vgachr == 0x7cU) unichr = 0xa6U;
	    else unichr = vgachr;
	    if (out(stream, unichr) != 0) return -1;
	}
	if (usecolors && VgaCanvas_hascolor(canvas))
	{
	    if (i < height-1)
	    {
		if (writeansi(stream, bg & 0x08U, bg, fg, fg) != 0) return -1;
		bg &= 0x08U;
	    }
	    else if (writeansidc(stream, 0, bg, 0x07U, fg, 1) != 0) return -1;
	}
	if (crlf && out(stream, '\r') != 0) return -1;
	if (out(stream, '\n') != 0) return -1;
    }
    if (markltr && out(stream, 0x202cU) != 0) return -1;
    return writebuf(stream);
}

