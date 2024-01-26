#include "testwriter.h"

#include "stream.h"

static char line[256];

static int writeline(Stream *stream, int len)
{
    size_t wrpos = 0;
    size_t written;
    while ((written = Stream_write(stream, line, len-wrpos)))
    {
	wrpos += written;
	if (written == (size_t)len) return 0;
    }
    return -1;
}

int TestWriter_write(Stream *stream)
{
    int linepos = 0;
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
    return 0;
}

