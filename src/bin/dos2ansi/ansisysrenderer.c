#include "ansisysrenderer.h"

#include "stream.h"
#include "vgacanvas.h"
#include "util.h"

#include <string.h>

int AnsiSysRenderer_render(VgaCanvas *canvas, Stream *stream)
{
    int escargs[8];
    int esc = 0;
    int c;

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
		else if (c == ';')
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
	else if (c == 0x1b) esc = -1;
	else VgaCanvas_put(canvas, c);
    }

    return -(Stream_status(stream) == SS_ERROR);
}
