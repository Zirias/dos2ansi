#include "sauce.h"

#include "stream.h"
#include "util.h"

#include <stdlib.h>
#include <string.h>

#define MAXSAUCE 0x4100
#define MINSAUCE 0x80
#define RDBUFCHUNK 0x208
#define RDCHUNK 0x200

struct Sauce
{
    char *title;
    char *author;
    char *group;
    char *date;
};

typedef struct RawSauce
{
    size_t size;
    char bytes[];
} RawSauce;

static RawSauce *RawSauce_read(Stream *in)
{
    size_t avail = 2 * RDBUFCHUNK;
    RawSauce *self = xmalloc(sizeof *self + avail);
    self->size = 0;
    size_t read = 0;
    size_t rdchunk = RDCHUNK;
    while ((read = Stream_read(in, self->bytes + self->size, rdchunk)))
    {
	self->size += read;
	if (self->size + rdchunk > MAXSAUCE) rdchunk = MAXSAUCE - self->size;
	if (self->size + rdchunk > avail)
	{
	    avail += RDBUFCHUNK;
	    if (avail > MAXSAUCE) goto error;
	    self = xrealloc(self, sizeof *self + avail);
	}
    }
    int status = Stream_status(in);
    if (status != SS_EOF) goto error;
    if (self->size < MINSAUCE) goto error;
    if (strncmp(self->bytes + (self->size - MINSAUCE),
		"SAUCE00", 7) != 0) goto error;
    return self;

error:
    free(self);
    return 0;
}

static char *getSauceStr(RawSauce *raw, size_t pos, size_t maxlen)
{
    char *src = raw->bytes + (raw->size - MINSAUCE) + pos;
    size_t len = 0;
    for (size_t i = 0; i < maxlen; ++i)
    {
	if (!src[i]) break;
	if (src[i] != 0x20) len = i+1;
    }
    if (!len) return 0;
    char *str = xmalloc(len + 1);
    memcpy(str, src, len);
    str[len] = 0;
    return str;
}

Sauce *Sauce_read(Stream *in)
{
    RawSauce *raw = RawSauce_read(in);
    if (!raw) return 0;

    Sauce *self = xmalloc(sizeof *self);
    self->title = getSauceStr(raw, 7, 35);
    self->author = getSauceStr(raw, 42, 20);
    self->group = getSauceStr(raw, 62, 20);
    self->date = getSauceStr(raw, 82, 8);

    free(raw);
    return self;
}

void Sauce_destroy(Sauce *self)
{
    if (!self) return;
    free(self->date);
    free(self->group);
    free(self->author);
    free(self->title);
    free(self);
}

