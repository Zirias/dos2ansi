# WITH_CURSES:		Build curses/terminfo writer (non-Windows only)
# WITH_SHOWANSI:	Install the showansi script (non-Windows only)
# FORCE_STDIO:		Don't use platform-specific file/stream I/O
#
# MANFMT:		man:    classic troff/man
# 			mdoc:   BSD mandoc
# 			defaults to mdoc when `uname` contains "BSD",
# 			man otherwise.

BOOLCONFVARS_ON=	WITH_CURSES WITH_SHOWANSI
BOOLCONFVARS_OFF=	FORCE_STDIO
SINGLECONFVARS=		MANFMT

NODIST=			tools/mkclidoc/zimk
DISTCLEANDIRS=		tools/bin

SUBBUILD=		MKCLIDOC
MKCLIDOC_TARGET=	tools/bin/mkclidoc
MKCLIDOC_SRCDIR=	tools/mkclidoc
MKCLIDOC_MAKEARGS=	DESTDIR=../bin prefix= bindir= zimkdir=../../zimk \
			HOSTBUILD=1 PORTABLE=1 STATIC=0
MKCLIDOC_MAKEGOAL=	install
MKCLIDOC_CLEANGOAL=	distclean

include zimk/zimk.mk

MANFMT:=	$(or $(MANFMT),$(if $(findstring BSD,$(SYSNAME)),mdoc,man))
MKCLIDOC:=	$(MKCLIDOC_TARGET)

$(call zinc, src/bin/dos2ansi/dos2ansi.mk)
ifeq ($(PLATFORM),posix)
  ifeq ($(WITH_SHOWANSI),1)
$(call zinc, src/bin/showansi/showansi.mk)
  endif
endif
