#include "ansisysrenderer.h"

#include "stream.h"
#include "vgacanvas.h"
#include "util.h"

#include <string.h>

int AnsiSysRenderer_render(VgaCanvas *canvas, Stream *meta, Stream *stream)
{
    int escargs[8];
    int esc = 0;
    int c;
    unsigned x = 0;
    unsigned y = 0;

    while ((c = Stream_getc(stream)) >= 0)
    {
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
		else if (c == ';' || c == ',')
		{
		    if (esc == sizeof escargs / sizeof *escargs) return -1;
		    ++esc;
		}
		else if (c == 'm')
		{
		    for (int i = 0; i < esc; ++i)
		    {
			if (escargs[i] == 0)
			{
			    VgaCanvas_resetAttr(canvas);
			}
			else if (escargs[i] == 1)
			{
			    VgaCanvas_setBold(canvas, 1);
			}
			else if (escargs[i] == 2 || escargs[i] == 22)
			{
			    VgaCanvas_setBold(canvas, 0);
			}
			else if (escargs[i] == 5 || escargs[i] == 6)
			{
			    VgaCanvas_setBlink(canvas, 1);
			}
			else if (escargs[i] == 25)
			{
			    VgaCanvas_setBlink(canvas, 0);
			}
			else if (escargs[i] == 7)
			{
			    VgaCanvas_setReverse(canvas, 1);
			}
			else if (escargs[i] == 27)
			{
			    VgaCanvas_setReverse(canvas, 0);
			}
			else if (escargs[i] == 8)
			{
			    VgaCanvas_setHidden(canvas, 1);
			}
			else if (escargs[i] == 28)
			{
			    VgaCanvas_setHidden(canvas, 0);
			}
			else if (escargs[i] >= 30 && escargs[i] <= 37)
			{
			    VgaCanvas_setFg(canvas, escargs[i] - 30);
			}
			else if (escargs[i] == 39)
			{
			    VgaCanvas_setFg(canvas, 7);
			}
			else if (escargs[i] >= 40 && escargs[i] <= 47)
			{
			    VgaCanvas_setBg(canvas, escargs[i] - 40);
			}
			else if (escargs[i] == 49)
			{
			    VgaCanvas_setBg(canvas, 0);
			}
		    }
		    esc = 0;
		}
		else if (c == '?' || c == '=' || c == '>')
		{
		    ; // ignore "set screen mode" flags
		}
		else if (c == 'h' || c == 'l')
		{
		    switch (escargs[0])
		    {
			case 0:
			case 1:
			    VgaCanvas_reset(canvas, 40, 25);
			    if (meta) Stream_puts(meta,
				    "m_resetwidth=40\nm_resetheight=25\n");
			    break;
			case 2:
			case 3:
			    VgaCanvas_reset(canvas, 80, 25);
			    if (meta) Stream_puts(meta,
				    "m_resetwidth=80\nm_resetheight=25\n");
			    break;
			case 7:
			    VgaCanvas_setWrap(canvas, c == 'h');
			    break;
			default:
			    break;
		    }
		    esc = 0;
		}
		else
		{
		    int n = escargs[0];
		    if (!n) n = 1;
		    int m = escargs[1];
		    if (!m) m = 1;

		    if (c == 'A')
		    {
			VgaCanvas_up(canvas, n);
		    }
		    else if (c == 'B')
		    {
			VgaCanvas_down(canvas, n);
		    }
		    else if (c == 'C')
		    {
			VgaCanvas_right(canvas, n);
		    }
		    else if (c == 'D')
		    {
			VgaCanvas_left(canvas, n);
		    }
		    else if (c == 'H' || c == 'f')
		    {
			VgaCanvas_gotoxy(canvas, m-1, n-1);
		    }
		    else if (c == 's')
		    {
			VgaCanvas_xy(canvas, &x, &y);
		    }
		    else if (c == 'u')
		    {
			VgaCanvas_gotoxy(canvas, x, y);
		    }
		    else if (c == 'J') switch (escargs[0])
		    {
			case 0: VgaCanvas_clearAfter(canvas); break;
			case 1: VgaCanvas_clearBefore(canvas); break;
			default:
				VgaCanvas_gotoxy(canvas, 0, 0);
				VgaCanvas_clearAll(canvas);
				break;
		    }
		    else if (c == 'K') switch (escargs[0])
		    {
			case 0: VgaCanvas_clearLineAfter(canvas); break;
			case 1: VgaCanvas_clearLineBefore(canvas); break;
			default: VgaCanvas_clearLine(canvas); break;
		    }
		    esc = 0;
		}
	    }
	}
	else if (c == 0x1b) esc = -1;
	else VgaCanvas_put(canvas, c);
    }

    return -(Stream_status(stream) == SS_ERROR);
}
