#include "dosreader.h"

#include "vgacanvas.h"
#include "util.h"

#include <string.h>

static char buf[1024];
static int ignoreeof = 0;

void DosReader_ignoreeof(int arg)
{
    ignoreeof = !!arg;
}

int DosReader_read(VgaCanvas *canvas, FILE *file)
{
    size_t bufsz;
    int escargs[8];
    int esc = 0;

    while ((bufsz = fread(buf, 1, sizeof buf, file)))
    {
	for (size_t bufpos = 0; bufpos < bufsz; ++bufpos)
	{
	    char c = buf[bufpos];
	    if (esc)
	    {
		if (esc < 0)
		{
		    if (c == 0x5b)
		    {
			esc = 1;
			memset(escargs, 0, sizeof escargs);
		    }
		    else esc = 0;
		}
		else
		{
		    if (c >= '0' && c <= '9')
		    {
			escargs[esc-1] *= 10;
			escargs[esc-1] += (c - '0');
		    }
		    else if (c == ';')
		    {
			if (esc == sizeof escargs / sizeof *escargs) return -1;
			++esc;
		    }
		    else if (c == 'm')
		    {
			int fg = VgaCanvas_fg(canvas);
			int bg = VgaCanvas_bg(canvas);
			for (int i = 0; i < esc; ++i)
			{
			    if (escargs[i] == 0)
			    {
				fg = 0x07;
				bg = 0x00;
			    }
			    else if (escargs[i] == 1)
			    {
				fg |= 0x08;
			    }
			    else if (escargs[i] == 2 || escargs[i] == 22)
			    {
				fg &= 0x07;
			    }
			    else if (escargs[i] == 5 || escargs[i] == 6)
			    {
				bg |= 0x08;
			    }
			    else if (escargs[i] == 25)
			    {
				bg &= 0x07;
			    }
			    else if (escargs[i] >= 30 && escargs[i] <= 37)
			    {
				fg = (fg & 0x08) | (escargs[i] - 30);
			    }
			    else if (escargs[i] == 39)
			    {
				fg = 0x07;
			    }
			    else if (escargs[i] >= 40 && escargs[i] <= 47)
			    {
				bg = (bg & 0x08) | (escargs[i] - 40);
			    }
			}
			VgaCanvas_setFg(canvas, fg);
			VgaCanvas_setBg(canvas, bg);
			esc = 0;
		    }
		    else
		    {
			if (!escargs[0]) escargs[0] = 1;
			if (c == 'A')
			{
			    VgaCanvas_up(canvas, escargs[0]);
			    esc = 0;
			}
			else if (c == 'B')
			{
			    VgaCanvas_down(canvas, escargs[0]);
			    esc = 0;
			}
			else if (c == 'C')
			{
			    VgaCanvas_right(canvas, escargs[0]);
			    esc = 0;
			}
			else if (c == 'D')
			{
			    VgaCanvas_left(canvas, escargs[0]);
			    esc = 0;
			}
			else esc = 0;
		    }
		}
	    }
	    else if (c == 0x1a && !ignoreeof) goto eof;
	    else if (c == 0x1b) esc = -1;
	    else VgaCanvas_put(canvas, c);
	}
    }

eof:
    return ferror(file);
}
