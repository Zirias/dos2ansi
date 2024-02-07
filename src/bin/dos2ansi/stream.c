#include "stream.h"

#include "util.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MS_CHUNKSZ 1024

#if defined(USE_WIN32)
#  include <windows.h>
#  define FLUSHFILE(f) (FlushFileBuffers(f) ? 0 : -1)
#  define CLOSEFILE(f) CloseHandle(f)

static HANDLE getStdStream(StandardStreamType type)
{
    switch (type)
    {
	case SST_STDIN: return GetStdHandle(STD_INPUT_HANDLE);
	case SST_STDOUT: return GetStdHandle(STD_OUTPUT_HANDLE);
	case SST_STDERR: return GetStdHandle(STD_ERROR_HANDLE);
	default: return INVALID_HANDLE_VALUE;
    }
}

static HANDLE openFile(const char *filename, FileOpenFlags flags)
{
    DWORD access = 0;
    DWORD share = 0;
    DWORD disposition = OPEN_EXISTING;
    if (flags & FOF_READ)
    {
	access |= GENERIC_READ;
	if (!(flags & FOF_WRITE)) share |= FILE_SHARE_READ;
    }
    if (flags & FOF_WRITE)
    {
	access |= GENERIC_WRITE;
	disposition = CREATE_ALWAYS;
    }
    size_t namechars = strlen(filename) + 1;
    LPWSTR wname = xmalloc((namechars + 4) * sizeof *wname);
    memcpy(wname, L"\\\\?\\", 4 * sizeof *wname);
    HANDLE file = INVALID_HANDLE_VALUE;
    namechars = MultiByteToWideChar(CP_UTF8, 0,
	    filename, -1, wname+4, namechars);
    if (!namechars) goto done;
    if (namechars < MAX_PATH)
    {
	/* This is a best-effort hack. To be able to access paths longer
	 * than MAX_PATH, we need to use the win32 namespace \\?\ passing
	 * the path unprocessed to the filesystem. For this to work, it
	 * must be an absolute path.
	 * To also support relative paths, we must convert to an absolute
	 * path which only works without this namespace.
	 * Therefore, if the path is already longer than MAX_PATH, we just
	 * assume it's already an absolute path and skip the conversion.
	 */
	DWORD abssz = namechars + 1024;
	LPWSTR absname = xmalloc((abssz + 4) * sizeof *absname);
	memcpy(absname, L"\\\\?\\", 4 * sizeof *wname);
	DWORD abslen = GetFullPathNameW(wname+4, abssz, absname+4, 0);
	free(wname);
	wname = absname;
	if (!abslen || abslen >= abssz) goto done;
    }
    file = CreateFileW(wname, access, share, 0, disposition, 0, 0);

done:
    free(wname);
    return file;
}

static StreamStatus writeFile(HANDLE file, size_t *nwritten,
	const void *ptr, size_t sz)
{
    StreamStatus status = SS_OK;
    DWORD wsz;
    if (!WriteFile(file, ptr, sz, &wsz, 0)) status = SS_ERROR;
    else if (!wsz) status = SS_EOF;
    *nwritten = wsz;
    return status;
}

static StreamStatus readFile(HANDLE file, size_t *nread,
	void *ptr, size_t sz)
{
    StreamStatus status = SS_OK;
    DWORD rsz;
    if (!ReadFile(file, ptr, sz, &rsz, 0)) status = SS_ERROR;
    else if (!rsz) status = SS_EOF;
    *nread = rsz;
    return status;
}

#elif defined(USE_POSIX)
#  include <fcntl.h>
#  include <unistd.h>
#  define FLUSHFILE(f) fdatasync(f)
#  define CLOSEFILE(f) close(f)

static int getStdStream(StandardStreamType type)
{
    switch (type)
    {
	case SST_STDIN: return STDIN_FILENO;
	case SST_STDOUT: return STDOUT_FILENO;
	case SST_STDERR: return STDERR_FILENO;
	default: return -1;
    }
}

static int openFile(const char *filename, FileOpenFlags flags)
{
    int openflags = 0;
    if (flags & FOF_WRITE)
    {
	if (flags & FOF_READ) openflags |= O_RDWR;
	else openflags |= O_WRONLY;
	openflags |= O_CREAT|O_TRUNC;
    }
    else if (flags & FOF_READ) openflags |= O_RDONLY;
    return open(filename, openflags);
}

static StreamStatus writeFile(int file, size_t *nwritten,
	const void *ptr, size_t sz)
{
    ssize_t rc = write(file, ptr, sz);
    if (rc < 0)
    {
	*nwritten = 0;
	return SS_ERROR;
    }
    *nwritten = rc;
    return rc == 0 ? SS_EOF : SS_OK;
}

static StreamStatus readFile(int file, size_t *nread,
	void *ptr, size_t sz)
{
    ssize_t rc = read(file, ptr, sz);
    if (rc < 0)
    {
	*nread = 0;
	return SS_ERROR;
    }
    *nread = rc;
    return rc == 0 ? SS_EOF : SS_OK;
}

#else /* !USE_WIN32 && !USE_POSIX */
#  define FLUSHFILE(f) (fflush(f) < 0 ? -1 : 0)
#  define CLOSEFILE(f) fclose(f)
#  ifdef _WIN32
#    include <fcntl.h>
#    include <io.h>
#    define BINMODE(f) _setmode(_fileno(f), _O_BINARY)
#  else
#    define BINMODE(f) ((void)(f))
#  endif

static int streamConfigured[] = {0, 0, 0};

static FILE *getStdStream(StandardStreamType type)
{
    FILE *file = 0;
    switch (type)
    {
	case SST_STDIN: file = stdin; break;
	case SST_STDOUT: file = stdout; break;
	case SST_STDERR: file = stderr; break;
    }
    if (file && !streamConfigured[type])
    {
	setvbuf(file, 0, _IONBF, 0);
	BINMODE(file);
	streamConfigured[type] = 1;
    }
    return file;
}

static FILE *openFile(const char *filename, FileOpenFlags flags)
{
    char mode[4] = {0};
    char *mptr = mode;
    if (flags & FOF_WRITE)
    {
	*mptr++ = 'w';
	if (flags & FOF_READ) *mptr++ = '+';
    }
    else if (flags & FOF_READ) *mptr++ = 'r';
    *mptr = 'b';
    FILE *file = fopen(filename, mode);
    if (file) setvbuf(file, 0, _IONBF, 0);
    return file;
}

static StreamStatus writeFile(FILE *file, size_t *nwritten,
	const void *ptr, size_t sz)
{
    *nwritten = fwrite(ptr, 1, sz, file);
    if (!*nwritten)
    {
	if (ferror(file)) return SS_ERROR;
	if (feof(file)) return SS_EOF;
    }
    return SS_OK;
}

static StreamStatus readFile(FILE *file, size_t *nread,
	void *ptr, size_t sz)
{
    *nread = fread(ptr, 1, sz, file);
    if (!*nread)
    {
	if (ferror(file)) return SS_ERROR;
	if (feof(file)) return SS_EOF;
    }
    return SS_OK;
}

#endif

struct Stream
{
    size_t size;
};

typedef struct MemoryStream
{
    struct Stream base;
    size_t readpos;
    size_t writepos;
    unsigned char *mem;
} MemoryStream;

#define T_FILESTREAM 1
typedef struct FileStream
{
    struct Stream base;
    FILEHANDLE file;
    FileOpenFlags flags;
    StreamStatus status;
} FileStream;

#define T_READERSTREAM 2
typedef struct ReaderStream
{
    struct Stream base;
    const void *magic;
    StreamReader *reader;
} ReaderStream;

#define T_WRITERSTREAM 3
typedef struct WriterStream
{
    struct Stream base;
    const void *magic;
    StreamWriter *writer;
} WriterStream;

Stream *Stream_createMemory(void)
{
    MemoryStream *self = xmalloc(sizeof *self);
    self->base.size = MS_CHUNKSZ;
    self->readpos = 0;
    self->writepos = 0;
    self->mem = xmalloc(self->base.size * sizeof *self->mem);
    return (Stream *)self;
}

Stream *Stream_fromFile(FILEHANDLE file, FileOpenFlags flags)
{
    FileStream *self = xmalloc(sizeof *self);
    self->base.size = T_FILESTREAM;
    self->file = file;
    self->flags = flags;
    self->status = SS_OK;
    return (Stream *)self;
}

Stream *Stream_createStandard(StandardStreamType type)
{
    FILEHANDLE std = getStdStream(type);
    if (std == NOTAFILE) return 0;
    return Stream_fromFile(std,
	    (type == SST_STDIN ? FOF_READ : FOF_WRITE) | FOF_NOCLOSE);
}

Stream *Stream_openFile(const char *filename, FileOpenFlags flags)
{
    if (!(flags & (FOF_READ|FOF_WRITE))) return 0;
    FILEHANDLE file = openFile(filename, flags);
    if (file == NOTAFILE) return 0;
    return Stream_fromFile(file, flags);
}

Stream *Stream_createReader(StreamReader *reader, const void *magic)
{
    if (!magic || !reader->read ||
	    (!reader->status && !reader->stream)) return 0;
    ReaderStream *self = xmalloc(sizeof *self);
    self->base.size = T_READERSTREAM;
    self->magic = magic;
    self->reader = reader;
    return (Stream *)self;
}

Stream *Stream_createWriter(StreamWriter *writer, const void *magic)
{
    if (!magic || !writer->write ||
	    (!writer->status && !writer->stream)) return 0;
    WriterStream *self = xmalloc(sizeof *self);
    self->base.size = T_WRITERSTREAM;
    self->magic = magic;
    self->writer = writer;
    return (Stream *)self;
}

FILEHANDLE Stream_file(Stream *self)
{
    if (self->size == T_READERSTREAM)
    {
	StreamReader *reader = ((ReaderStream *)self)->reader;
	if (!reader->stream) return NOTAFILE;
	return Stream_file(reader->stream);
    }
    else if (self->size == T_WRITERSTREAM)
    {
	StreamWriter *writer = ((WriterStream *)self)->writer;
	if (!writer->stream) return NOTAFILE;
	return Stream_file(writer->stream);
    }
    else if (self->size == T_FILESTREAM) return ((FileStream *)self)->file;
    return NOTAFILE;
}

StreamReader *Stream_reader(Stream *self, const void *magic)
{
    if (!magic || self->size != T_READERSTREAM) return 0;
    ReaderStream *rs = (ReaderStream *)self;
    if (rs->magic == magic) return rs->reader;
    if (!rs->reader->stream) return 0;
    return Stream_reader(rs->reader->stream, magic);
}

StreamWriter *Stream_writer(Stream *self, const void *magic)
{
    if (!magic || self->size != T_WRITERSTREAM) return 0;
    WriterStream *ws = (WriterStream *)self;
    if (ws->magic == magic) return ws->writer;
    if (!ws->writer->stream) return 0;
    return Stream_writer(ws->writer->stream, magic);
}

static size_t MemoryStream_write(MemoryStream *self,
	const void *ptr, size_t sz)
{
    size_t newpos = self->writepos + sz;
    if (newpos > self->base.size)
    {
	size_t newsz = newpos;
	size_t fragsz = newsz % MS_CHUNKSZ;
	if (fragsz) newsz += (MS_CHUNKSZ - fragsz);
	self->mem = xrealloc(self->mem, newsz * sizeof *self->mem);
	self->base.size = newsz;
    }
    memcpy(self->mem + self->writepos, ptr, sz);
    self->writepos = newpos;
    return sz;
}

static size_t FileStream_write(FileStream *self, const void *ptr, size_t sz)
{
    if (!(self->flags & FOF_WRITE)) return 0;
    if (self->status != SS_OK) return 0;
    size_t nwritten;
    self->status = writeFile(self->file, &nwritten, ptr, sz);
    return nwritten;
}

static size_t WriterStream_write(WriterStream *self,
	const void *ptr, size_t sz)
{
    return self->writer->write(self->writer, ptr, sz);
}

size_t Stream_write(Stream *self, const void *ptr, size_t sz)
{
    switch (self->size)
    {
	case T_FILESTREAM:
	    return FileStream_write((FileStream *)self, ptr, sz);
	case T_READERSTREAM:
	    return 0;
	case T_WRITERSTREAM:
	    return WriterStream_write((WriterStream *)self, ptr, sz);
	default:
	    return MemoryStream_write((MemoryStream *)self, ptr, sz);
    }
}

size_t Stream_puts(Stream *self, const char *str)
{
    size_t sz = strlen(str)+1;
    size_t wr = 0;
    while (wr < sz)
    {
	size_t cs = Stream_write(self, str+wr, sz-wr);
	if (!cs) return -1;
	wr += cs;
    }
    return wr;
}

size_t Stream_printf(Stream *self, const char *format, ...)
{
    char buf[1024];
    char *xbuf = 0;
    va_list ap;
    va_start(ap, format);
    size_t sz = vsnprintf(buf, sizeof buf, format, ap);
    va_end(ap);
    if (sz >= sizeof buf)
    {
	xbuf = xmalloc(sz+1);
	va_start(ap, format);
	vsnprintf(xbuf, sz+1, format, ap);
	va_end(ap);
    }
    size_t wr = Stream_puts(self, xbuf ? xbuf : buf);
    free(xbuf);
    return wr;
}

static size_t MemoryStream_read(MemoryStream *self, void *ptr, size_t sz)
{
    size_t avail = self->writepos - self->readpos;
    if (sz > avail) sz = avail;
    memcpy(ptr, self->mem + self->readpos, sz);
    self->readpos += sz;
    return sz;
}

static size_t FileStream_read(FileStream *self, void *ptr, size_t sz)
{
    if (!(self->flags & FOF_READ)) return 0;
    if (self->status != SS_OK) return 0;
    size_t nread;
    self->status = readFile(self->file, &nread, ptr, sz);
    return nread;
}

static size_t ReaderStream_read(ReaderStream *self, void *ptr, size_t sz)
{
    return self->reader->read(self->reader, ptr, sz);
}

size_t Stream_read(Stream *self, void *ptr, size_t sz)
{
    switch (self->size)
    {
	case T_FILESTREAM:
	    return FileStream_read((FileStream *)self, ptr, sz);
	case T_READERSTREAM:
	    return ReaderStream_read((ReaderStream *)self, ptr, sz);
	case T_WRITERSTREAM:
	    return 0;
	default:
	    return MemoryStream_read((MemoryStream *)self, ptr, sz);
    }
}

static int FileStream_flush(FileStream *self)
{
    if (!(self->flags & FOF_WRITE)) return -1;
    return FLUSHFILE(self->file);
}

static int WriterStream_flush(WriterStream *self)
{
    if (self->writer->flush) return self->writer->flush(self->writer);
    else if (self->writer->stream) return Stream_flush(self->writer->stream);
    else return -1;
}

int Stream_flush(Stream *self)
{
    switch (self->size)
    {
	case T_FILESTREAM: return FileStream_flush((FileStream *)self);
	case T_WRITERSTREAM: return WriterStream_flush((WriterStream *)self);
	default: return -1;
    }
}

static StreamStatus MemoryStream_status(const MemoryStream *self)
{
    return self->readpos == self->writepos ? SS_EOF : SS_OK;
}

static StreamStatus FileStream_status(const FileStream *self)
{
    return self->status;
}

static StreamStatus ReaderStream_status(const ReaderStream *self)
{
    if (self->reader->status) return self->reader->status(self->reader);
    return Stream_status(self->reader->stream);
}

static StreamStatus WriterStream_status(const WriterStream *self)
{
    if (self->writer->status) return self->writer->status(self->writer);
    return Stream_status(self->writer->stream);
}

StreamStatus Stream_status(const Stream *self)
{
    switch (self->size)
    {
	case T_FILESTREAM:
	    return FileStream_status((const FileStream *)self);
	case T_READERSTREAM:
	    return ReaderStream_status((const ReaderStream *)self);
	case T_WRITERSTREAM:
	    return WriterStream_status((const WriterStream *)self);
	default:
	    return MemoryStream_status((const MemoryStream *)self);
    }
}

static void MemoryStream_destroy(MemoryStream *self)
{
    free(self->mem);
}

static void FileStream_destroy(FileStream *self)
{
    if (self->flags & FOF_NOCLOSE) return;
    CLOSEFILE(self->file);
}

static void ReaderStream_destroy(ReaderStream *self)
{
    if (self->reader->destroy) self->reader->destroy(self->reader);
    else
    {
	Stream_destroy(self->reader->stream);
	free(self->reader);
    }
}

static void WriterStream_destroy(WriterStream *self)
{
    if (self->writer->destroy) self->writer->destroy(self->writer);
    else
    {
	WriterStream_flush(self);
	Stream_destroy(self->writer->stream);
	free(self->writer);
    }
}

void Stream_destroy(Stream *self)
{
    if (!self) return;
    switch (self->size)
    {
	case T_FILESTREAM: FileStream_destroy((FileStream *)self); break;
	case T_READERSTREAM: ReaderStream_destroy((ReaderStream *)self); break;
	case T_WRITERSTREAM: WriterStream_destroy((WriterStream *)self); break;
	default: MemoryStream_destroy((MemoryStream *)self);
    }
    free(self);
}

