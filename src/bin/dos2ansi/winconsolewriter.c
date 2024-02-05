#include "winconsolewriter.h"

#include "stream.h"
#include "util.h"

#include <stdint.h>

typedef struct WinConsoleWriter
{
    StreamWriter base;
    HANDLE console;
    int stripcolors;
    int err;
} WinConsoleWriter;

static const char *magic = "";

static size_t write(StreamWriter *self, const void *ptr, size_t size)
{
    WinConsoleWriter *writer = (WinConsoleWriter *)self;
    const uint16_t *str = ptr;
    size_t written = 0;
    if (writer->err) goto done;
    size_t remaining = size / sizeof *str;
    while (remaining)
    {
	size_t pos = 0;
	while (pos < remaining &&
		(str[pos] < 0xee00 || str[pos] > 0xef00)) ++pos;
	if (pos)
	{
	    if (!WriteConsoleW(writer->console, str, pos, 0, 0))
	    {
		writer->err = 1;
		goto done;
	    }
	    written += pos;
	}
	if (pos < remaining)
	{
	    if (!writer->stripcolors)
	    {
		uint16_t ansiattr = str[pos++];
		if (ansiattr == 0xef00) ansiattr = 0x0007;
		else ansiattr &= 0xff;
		WORD winattr = 0;
		if (ansiattr & (1 << 0)) winattr |= FOREGROUND_RED;
		if (ansiattr & (1 << 1)) winattr |= FOREGROUND_GREEN;
		if (ansiattr & (1 << 2)) winattr |= FOREGROUND_BLUE;
		if (ansiattr & (1 << 3)) winattr |= FOREGROUND_INTENSITY;
		if (ansiattr & (1 << 4)) winattr |= BACKGROUND_RED;
		if (ansiattr & (1 << 5)) winattr |= BACKGROUND_GREEN;
		if (ansiattr & (1 << 6)) winattr |= BACKGROUND_BLUE;
		if (ansiattr & (1 << 7)) winattr |= BACKGROUND_INTENSITY;
		if (!SetConsoleTextAttribute(writer->console, winattr))
		{
		    writer->err = 1;
		    goto done;
		}
	    }
	    else ++pos;
	    ++written;
	}
	str += pos;
	remaining -= pos;
    }

done:
    return written * sizeof *str;
}

static int flush(StreamWriter *self)
{
    return ((WinConsoleWriter *)self)->err ? -1 : 0;
}

static int status(const StreamWriter *self)
{
    return ((const WinConsoleWriter *)self)->err ? SS_ERROR : SS_OK;
}

Stream *WinConsoleWriter_create(HANDLE console, int stripcolors)
{
    WinConsoleWriter *writer = xmalloc(sizeof *writer);
    writer->base.write = write;
    writer->base.flush = flush;
    writer->base.status = status;
    writer->base.destroy = 0;
    writer->base.stream = 0;
    writer->console = console;
    writer->stripcolors = stripcolors;
    writer->err = 0;
    return Stream_createWriter((StreamWriter *)writer, magic);
}

