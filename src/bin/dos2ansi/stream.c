#include "stream.h"

#include "util.h"

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MS_CHUNKSZ 1024

#if defined(USE_WIN32)
#  include <windows.h>
#  define FOF_WINCONSOLE 1 << 31
#  define GETEXTRAFLAGS(f, fl) do { \
    DWORD mode; \
    if (GetConsoleMode(f, &mode)) fl |= FOF_WINCONSOLE; \
} while (0)
#  define FLUSHFILE(f, fl) \
    (fl & FOF_WINCONSOLE ? 0 : (FlushFileBuffers(f) ? 0 : -1))
#  define CLOSEFILE(f) CloseHandle(f)

static HANDLE getStdStream(StandardStreamType type)
{
    switch (type)
    {
	case SST_STDIN: return GetStdHandle(STD_INPUT_HANDLE);
	case SST_STDOUT: return GetStdHandle(STD_OUTPUT_HANDLE);
	default: return GetStdHandle(STD_ERROR_HANDLE);
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
	const void *ptr, size_t sz, FileOpenFlags flags)
{
    StreamStatus status = SS_OK;
    DWORD wsz;
    if (!(flags & FOF_WINCONSOLE
		? WriteConsole(file, ptr, sz, &wsz, 0)
		: WriteFile(file, ptr, sz, &wsz, 0))) status = SS_ERROR;
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

static long fileSize(HANDLE file)
{
    LARGE_INTEGER sz;
    if (GetFileSizeEx(file, &sz)) return sz.QuadPart;
    else return -1;
}

static long seekFile(HANDLE file, StreamSeekStart start, long offset)
{
    LARGE_INTEGER off;
    LARGE_INTEGER pos;
    DWORD method = 0;
    off.QuadPart = offset;
    switch (start)
    {
	case SSS_START:
	    method = FILE_BEGIN;
	    break;
	case SSS_POS:
	    method = FILE_CURRENT;
	    break;
	case SSS_END:
	    method = FILE_END;
	    break;
    }
    if (SetFilePointerEx(file, off, &pos, method)) return pos.QuadPart;
    else return -1;
}

#elif defined(USE_POSIX)
#  include <fcntl.h>
#  include <sys/stat.h>
#  include <unistd.h>
#  define GETEXTRAFLAGS(f, fl) (void)(fl);
#  define FLUSHFILE(f, fl) fdatasync(f)
#  define CLOSEFILE(f) close(f)

static int getStdStream(StandardStreamType type)
{
    switch (type)
    {
	case SST_STDIN: return STDIN_FILENO;
	case SST_STDOUT: return STDOUT_FILENO;
	default: return STDERR_FILENO;
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
	const void *ptr, size_t sz, FileOpenFlags flags)
{
    (void)flags;

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

static long fileSize(int file)
{
    struct stat st;
    if (fstat(file, &st) == 0) return st.st_size;
    else return -1;
}

static long seekFile(int file, StreamSeekStart start, long offset)
{
    int whence = 0;
    switch (start)
    {
	case SSS_START:
	    whence = SEEK_SET;
	    break;
	case SSS_POS:
	    whence = SEEK_CUR;
	    break;
	case SSS_END:
	    whence = SEEK_END;
	    break;
    }
    return lseek(file, offset, whence);
}

#else /* !USE_WIN32 && !USE_POSIX */
#  define GETEXTRAFLAGS(f, fl) (void)(fl);
#  define FLUSHFILE(f, fl) (fflush(f) < 0 ? -1 : 0)
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
    FILE *file;
    switch (type)
    {
	case SST_STDIN: file = stdin; break;
	case SST_STDOUT: file = stdout; break;
	default: file = stderr; break;
    }
    if (!streamConfigured[type])
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
	const void *ptr, size_t sz, FileOpenFlags flags)
{
    (void)flags;

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

static long fileSize(FILE *file)
{
    long pos = ftell(file);
    if (pos < 0) return -1;
    if (fseek(file, 0, SEEK_END) < 0) return -1;
    long sz = ftell(file);
    fseek(file, pos, SEEK_SET);
    return sz >= 0 ? sz : -1;
}

static long seekFile(FILE *file, StreamSeekStart start, long offset)
{
    int whence = 0;
    switch (start)
    {
	case SSS_START:
	    whence = SEEK_SET;
	    break;
	case SSS_POS:
	    whence = SEEK_CUR;
	    break;
	case SSS_END:
	    whence = SEEK_END;
	    break;
    }
    if (fseek(file, offset, whence) < 0) return -1;
    return ftell(file);
}
#endif

struct Stream
{
    size_t size;
};

typedef struct MemoryStream
{
    struct Stream base;
    size_t maxsize;
    size_t readpos;
    size_t writepos;
    StreamStatus status;
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

Stream *Stream_createMemory(size_t maxsize)
{
    MemoryStream *self = xmalloc(sizeof *self);
    self->base.size = MS_CHUNKSZ > maxsize ? maxsize : MS_CHUNKSZ;
    self->maxsize = maxsize;
    self->readpos = 0;
    self->writepos = 0;
    self->status = SS_OK;
    self->mem = xmalloc(self->base.size * sizeof *self->mem);
    return (Stream *)self;
}

Stream *Stream_fromStream(Stream *in, size_t maxsize)
{
    char buf[16*1024];
    Stream *self = Stream_createMemory(maxsize);
    size_t rdsz;
    while ((rdsz = Stream_read(in, buf, sizeof buf)))
    {
	if (Stream_write(self, buf, rdsz) < rdsz)
	{
	    Stream_destroy(self);
	    return 0;
	}
    }
    return self;
}

Stream *Stream_fromFile(FILEHANDLE file, FileOpenFlags flags)
{
    GETEXTRAFLAGS(file, flags);
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
    ReaderStream *self = xmalloc(sizeof *self);
    self->base.size = T_READERSTREAM;
    self->magic = magic;
    self->reader = reader;
    return (Stream *)self;
}

Stream *Stream_createWriter(StreamWriter *writer, const void *magic)
{
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
    if (self->size != T_READERSTREAM) return 0;
    ReaderStream *rs = (ReaderStream *)self;
    if (rs->magic == magic) return rs->reader;
    if (!rs->reader->stream) return 0;
    return Stream_reader(rs->reader->stream, magic);
}

StreamWriter *Stream_writer(Stream *self, const void *magic)
{
    if (self->size != T_WRITERSTREAM) return 0;
    WriterStream *ws = (WriterStream *)self;
    if (ws->magic == magic) return ws->writer;
    if (!ws->writer->stream) return 0;
    return Stream_writer(ws->writer->stream, magic);
}

static size_t MemoryStream_write(MemoryStream *self,
	const void *ptr, size_t sz)
{
    if (self->writepos + sz > self->maxsize)
    {
	self->status = SS_ERROR;
	sz = self->maxsize - self->writepos;
	if (!sz) return 0;
    }
    size_t newpos = self->writepos + sz;
    if (newpos > self->base.size)
    {
	size_t newsz = newpos;
	size_t fragsz = newsz % MS_CHUNKSZ;
	if (fragsz) newsz += (MS_CHUNKSZ - fragsz);
	if (newsz > self->maxsize) newsz = self->maxsize;
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
    self->status = writeFile(self->file, &nwritten, ptr, sz, self->flags);
    return nwritten;
}

static size_t WriterStream_write(WriterStream *self,
	const void *ptr, size_t sz)
{
    if (!self->writer->write) return 0;
    return self->writer->write(self->writer, ptr, sz);
}

static size_t writeDispatch(Stream *self, const void *ptr, size_t sz)
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

size_t Stream_write(Stream *self, const void *ptr, size_t sz)
{
    size_t wr = 0;
    const char *cptr = ptr;
    while (wr < sz)
    {
	size_t cs = writeDispatch(self, cptr, sz-wr);
	if (!cs) return wr;
	wr += cs;
	cptr += cs;
    }
    return wr;
}

int Stream_putc(Stream *self, int c)
{
    unsigned char b = c;
    if (!Stream_write(self, &b, 1)) return -1;
    return c;
}

size_t Stream_puts(Stream *self, const char *str)
{
    return Stream_write(self, str, strlen(str));
}

int Stream_vprintf(Stream *self, const char *format, va_list ap)
{
    char buf[1024];
    char *xbuf = 0;
    va_list ap2;
    va_copy(ap2, ap);
    int rc = vsnprintf(buf, sizeof buf, format, ap);
    if (rc < 0) goto done;
    if (rc == INT_MAX)
    {
	rc = -1;
	errno = EOVERFLOW;
	goto done;
    }
    if ((size_t)rc >= sizeof buf)
    {
	xbuf = xmalloc(rc+1);
	rc = vsnprintf(xbuf, rc+1, format, ap2);
	if (rc < 0) goto done;
    }
    rc = Stream_write(self, xbuf ? xbuf : buf, rc);
done:
    free(xbuf);
    va_end(ap2);
    return rc;
}

int Stream_printf(Stream *self, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    int rc = Stream_vprintf(self, format, ap);
    va_end(ap);
    return rc;
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
    if (!self->reader->read) return 0;
    return self->reader->read(self->reader, ptr, sz);
}

static size_t readDispatch(Stream *self, void *ptr, size_t sz)
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

size_t Stream_read(Stream *self, void *ptr, size_t sz)
{
    size_t rd = 0;
    char *cptr = ptr;
    while (rd < sz)
    {
	size_t cs = readDispatch(self, cptr, sz-rd);
	if (!cs) return rd;
	rd += cs;
	cptr += cs;
    }
    return rd;
}

static long MemoryStream_size(MemoryStream *self)
{
    return self->writepos;
}

static long FileStream_size(FileStream *self)
{
    return fileSize(self->file);
}

static long ReaderStream_size(ReaderStream *self)
{
    return self->reader->stream ? Stream_size(self->reader->stream) : -1;
}

long Stream_size(Stream *self)
{
    switch (self->size)
    {
	case T_FILESTREAM:
	    return FileStream_size((FileStream *)self);
	case T_READERSTREAM:
	    return ReaderStream_size((ReaderStream *)self);
	case T_WRITERSTREAM:
	    return -1;
	default:
	    return MemoryStream_size((MemoryStream *)self);
    }
}

static long MemoryStream_seek(MemoryStream *self,
	StreamSeekStart start, long offset)
{
    switch (start)
    {
	case SSS_START:
	    if (offset <= 0) self->readpos = 0;
	    else self->readpos = (size_t)offset > self->writepos
		? self->writepos : (size_t)offset;
	    break;
	case SSS_POS:
	    if (!offset) break;
	    if (offset < 0) self->readpos = (size_t)(-offset) > self->readpos
		? 0 : self->readpos - (size_t)(-offset);
	    else self->readpos =
		self->readpos + (size_t)offset > self->writepos
		? self->writepos : self->readpos + offset;
	    break;
	case SSS_END:
	    if (offset >= 0) self->readpos = self->writepos;
	    else self->readpos = (size_t)(-offset) > self->writepos
		? 0 : self->writepos - (size_t)(-offset);
	    break;
    }
    return self->readpos;
}

static long FileStream_seek(FileStream *self,
	StreamSeekStart start, long offset)
{
    return seekFile(self->file, start, offset);
}

static long ReaderStream_seek(ReaderStream *self,
	StreamSeekStart start, long offset)
{
    if (!self->reader->stream) return -1;
    return Stream_seek(self->reader->stream, start, offset);
}

long Stream_seek(Stream *self, StreamSeekStart start, long offset)
{
    switch (self->size)
    {
	case T_FILESTREAM:
	    return FileStream_seek((FileStream *)self, start, offset);
	case T_READERSTREAM:
	    return ReaderStream_seek((ReaderStream *)self, start, offset);
	case T_WRITERSTREAM:
	    return -1;
	default:
	    return MemoryStream_seek((MemoryStream *)self, start, offset);
    }
}

int Stream_getc(Stream *self)
{
    unsigned char b;
    if (!Stream_read(self, &b, 1)) return -1;
    return b;
}

static int FileStream_flush(FileStream *self)
{
    if (!(self->flags & FOF_WRITE)) return -1;
    return FLUSHFILE(self->file, self->flags);
}

static int WriterStream_flush(WriterStream *self, int sys)
{
    if (self->writer->flush) return self->writer->flush(self->writer, sys);
    if (self->writer->stream) return Stream_flush(self->writer->stream, sys);
    return -1;
}

int Stream_flush(Stream *self, int sys)
{
    switch (self->size)
    {
	case T_FILESTREAM: return sys ? FileStream_flush(
				   (FileStream *)self) : 0;
	case T_WRITERSTREAM: return WriterStream_flush(
				     (WriterStream *)self, sys);
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
    if (self->reader->stream) return Stream_status(self->reader->stream);
    return SS_ERROR;
}

static StreamStatus WriterStream_status(const WriterStream *self)
{
    if (self->writer->status) return self->writer->status(self->writer);
    if (self->writer->stream) return Stream_status(self->writer->stream);
    return SS_ERROR;
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
	WriterStream_flush(self, 0);
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

