#include "testwriter.h"

#include "stream.h"

#include <stdio.h>
#include <string.h>

#define tryputs(o,s) do { \
    if (!Stream_puts((o),(s))) return -1; \
} while(0)
#define tryprintf(o,s,...) do { \
    if (!Stream_printf((o),(s),__VA_ARGS__)) return -1; \
} while(0)

int TestWriter_write(Stream *stream)
{
    tryputs(stream, "Codepage:\r\n\r\n");
    tryputs(stream, "\33[1;44m 0x ");
    for (unsigned char i = 0; i < 16; ++i)
    {
	tryprintf(stream, " %02x ", i << 4);
    }
    tryputs(stream, "\r\n");
    for (unsigned char i = 0; i < 16; ++i)
    {
	tryprintf(stream, "\33[1;44m %02x \33[m", i);
	for (unsigned char j = 0; j < 16; ++j)
	{
	    unsigned char c = (j << 4) | i;
	    const char *nm = 0;
	    switch (c)
	    {
		case 0x00: nm = "NUL"; break;
		case 0x07: nm = "BEL"; break;
		case 0x08: nm = "BS "; break;
		case 0x09: nm = "TAB"; break;
		case 0x0a: nm = "LF "; break;
		case 0x0d: nm = "CR "; break;
		case 0x1b: nm = "ESC"; break;
		default: break;
	    }
	    if (nm) tryprintf(stream, "\33[1;36m %s\33[m", nm);
	    else tryprintf(stream, "  %c ", c);
	}
	tryputs(stream, "\r\n");
    }
    tryputs(stream, "\r\nColors:\r\n\r\n    Foreground:    ");
    for (unsigned i = 1; i < 16; ++i)
    {
	if (i == 8) tryputs(stream, "\33[1m");
	tryprintf(stream, "\33[3%um\36\37", i%8);
    }
    tryputs(stream, "\33[m\r\n    Background:    \33[30m");
    for (unsigned i = 1; i < 16; ++i)
    {
	if (i == 8) tryputs(stream, "\33[5m");
	tryprintf(stream, "\33[4%um\1\2", i%8);
    }
    tryputs(stream, "\33[m\r\n\r\n  \3  dos2ansi v" DOS2ANSIVERSTR
	    "  \16\33[m\r\n");

    return 0;
}

