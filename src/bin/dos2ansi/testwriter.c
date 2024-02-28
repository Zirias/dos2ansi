#include "testwriter.h"

#include "codepage.h"
#include "config.h"
#include "stream.h"

#include <stdio.h>
#include <string.h>

int TestWriter_write(Stream *stream, const Config *config)
{
    Stream_printf(stream, "Codepage %s:\r\n\r\n\33[1;44m 0x ",
	    Codepage_name(Config_codepage(config)));
    for (unsigned char i = 0; i < 16; ++i)
    {
	Stream_printf(stream, " %02x ", i << 4);
    }
    Stream_puts(stream, "\r\n");
    for (unsigned char i = 0; i < 16; ++i)
    {
	Stream_printf(stream, "\33[1;44m %02x \33[m", i);
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
	    if (nm) Stream_printf(stream, "\33[1;36m %s\33[m", nm);
	    else Stream_printf(stream, "  %c ", c);
	}
	Stream_puts(stream, "\r\n");
    }
    Stream_puts(stream, "\r\nColors:\r\n\r\n    Foreground:    ");
    for (unsigned i = 1; i < 16; ++i)
    {
	if (i == 8) Stream_puts(stream, "\33[1m");
	Stream_printf(stream, "\33[3%um\36\37", i%8);
    }
    Stream_puts(stream, "\33[m\r\n    Background:    \33[30m");
    for (unsigned i = 1; i < 16; ++i)
    {
	if (i == 8) Stream_puts(stream, "\33[5m");
	Stream_printf(stream, "\33[4%um\1\2", i%8);
    }
    Stream_puts(stream, "\33[m\r\n\r\n  \3  dos2ansi v" DOS2ANSIVERSTR
	    "  \16\33[m\r\n");

    return -(Stream_status(stream) != SS_OK);
}

