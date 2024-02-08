#ifndef DOS2ANSI_CODEPAGE_H
#define DOS2ANSI_CODEPAGE_H

#include "decl.h"

#include <stdint.h>

C_CLASS_DECL(Codepage);

typedef enum CodepageId {
    CP_437,	/* Latin US (OEM) */
    CP_708,	/* Arabic (ASMO) */
    CP_720,	/* Arabic (Microsoft) */
    CP_737,	/* Greek */
    CP_775,	/* Baltic Rim */
    CP_819,	/* Latin-1 (ISO 8859-1) */
    CP_850,	/* Latin-1 (western Europe) */
    CP_852,	/* Latin-2 (middle Europe) */
    CP_855,	/* Cyrillic */
    CP_857,	/* Turkish */
    CP_860,	/* Portuguese */
    CP_861,	/* Icelandic */
    CP_862,	/* Hebrew */
    CP_863,	/* French Canada */
    CP_864,	/* Arabic (IBM) */
    CP_865,	/* Nordic */
    CP_869,	/* Greek 2 */
    CP_MAZ,	/* Mazovia */
    CP_MIK	/* Russian */
} CodepageId;

typedef enum CodepageFlags {
    CPF_NONE	    = 0,
    CPF_SOLIDBAR    = 1 << 0,	/* force using solid pipe bar */
    CPF_BROKENBAR   = 1 << 1,	/* force using broken pipe bar */
    CPF_EUROSYM	    = 1 << 2	/* use Euro variant if applicable */
} CodepageFlags;

CodepageId CodepageId_byName(const char *name) ATTR_NONNULL((1));
CodepageFlags CodepageFlags_byName(const char *name) ATTR_NONNULL((1));

Codepage *Codepage_create(CodepageId id, CodepageFlags flags);
uint16_t Codepage_map(const Codepage *self, uint8_t c) CMETHOD ATTR_PURE;
void Codepage_destroy(Codepage *self);

#endif
