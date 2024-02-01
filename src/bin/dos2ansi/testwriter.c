#include "testwriter.h"

#include "stream.h"

#include <string.h>

static char line[256];

static int write(Stream *stream, const char *p, size_t len)
{
    size_t written;
    while ((written = Stream_write(stream, p, len)))
    {
	p += written;
	if (!(len -= written)) return 0;
    }
    return -1;
}

static int writeline(Stream *stream, size_t len)
{
    return write(stream, line, len);
}

static int writestr(Stream *stream, const char *str)
{
    return write(stream, str, strlen(str));
}

int TestWriter_write(Stream *stream)
{
    if (writestr(stream, "Codepage:\r\n\r\n") < 0) return -1;
    size_t linepos = 0;
    linepos += sprintf(line+linepos, "\33[1;44m 0x ");
    for (unsigned char i = 0; i < 16; ++i)
    {
	linepos += sprintf(line+linepos, " %02x ", i << 4);
    }
    linepos += sprintf(line+linepos, "\r\n");
    if (writeline(stream, linepos) < 0) return -1;
    for (unsigned char i = 0; i < 16; ++i)
    {
	linepos = 0;
	linepos += sprintf(line+linepos, "\33[1;44m %02x \33[m", i);
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
	    if (nm) linepos += sprintf(line+linepos, "\33[1;36m %s\33[m", nm);
	    else linepos += sprintf(line+linepos, "  %c ", c);
	}
	linepos += sprintf(line+linepos, "\r\n");
	if (writeline(stream, linepos) < 0) return -1;
    }
    if (writestr(stream,
		"\r\nColors:\r\n\r\n    Foreground:    ") < 0) return -1;
    linepos = 0;
    for (unsigned i = 1; i < 16; ++i)
    {
	if (i == 8) linepos += sprintf(line+linepos, "\33[1m");
	linepos += sprintf(line+linepos, "\33[3%um\36\37", i%8);
    }
    if (writeline(stream, linepos) < 0) return -1;
    if (writestr(stream,
		"\33[m\r\n    Background:    \33[30m") < 0) return -1;
    linepos = 0;
    for (unsigned i = 1; i < 16; ++i)
    {
	if (i == 8) linepos += sprintf(line+linepos, "\33[5m");
	linepos += sprintf(line+linepos, "\33[4%um\1\2", i%8);
    }
    if (writeline(stream, linepos) < 0) return -1;
    if (writestr(stream, "\33[m\r\n\r\n  \3  dos2ansi v"
		DOS2ANSIVERSTR "  \16\r\n") < 0) return -1;
    linepos += sprintf(line+linepos, "\33[m\r\n");

    return 0;
}

