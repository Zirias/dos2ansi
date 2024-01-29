#include "ansitermwriter.h"

#include "codepage.h"
#include "stream.h"
#include "vgacanvas.h"

#include <stdint.h>
#include <string.h>

static int usebom = 1;
static int crlf = 0;
static int markltr = 0;

static int out(Stream *stream, uint16_t c)
{
    return Stream_write(stream, &c, sizeof c);
}

void AnsiTermWriter_usebom(int arg)
{
    usebom = !!arg;
}

void AnsiTermWriter_crlf(int arg)
{
    crlf = !!arg;
}

void AnsiTermWriter_markltr(int arg)
{
    markltr = !!arg;
}

int AnsiTermWriter_write(Stream *stream, const Codepage *cp,
	const VgaCanvas *canvas)
{
    size_t height = VgaCanvas_height(canvas);
    if (!height) return 0;

    if (usebom && !out(stream, 0xfeffU)) return -1;
    if (markltr && !out(stream, 0x202dU)) return -1;

    int hascolor = VgaCanvas_hascolor(canvas);
    int att = -1;
    for (size_t i = 0; i < height; ++i)
    {
	const VgaLine *line = VgaCanvas_line(canvas, i);
	int len = VgaLine_len(line);
	for (int j = 0; j < len; ++j)
	{
	    if (hascolor)
	    {
		unsigned char newatt = VgaLine_att(line, j);
		if (newatt != att)
		{
		    if (!out(stream, 0xee00U | newatt)) return -1;
		    att = newatt;
		}
	    }
	    if (!out(stream, Codepage_map(cp,
			    VgaLine_chr(line, j)))) return -1;
	}
	if (hascolor)
	{
	    if (i < height-1)
	    {
		att &= 0x8fU;
		if (!out(stream, 0xee00U | att)) return -1;
	    }
	    else if (!out(stream, 0xef00U)) return -1;
	}
	if (crlf && !out(stream, '\r')) return -1;
	if (!out(stream, '\n')) return -1;
    }
    if (markltr && !out(stream, 0x202cU)) return -1;
    return Stream_flush(stream);
}

