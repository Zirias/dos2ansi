# WITH_CURSES:		Build curses/terminfo writer (non-Windows only)
# WITH_HTML:		Build manpages in HTML format (always on for Windows)
# WITH_MAN:		Build manpages in MANFMT (always off for Windows)
# WITH_SHOWANSI:	Install the showansi script (non-Windows only)
# FORCE_STDIO:		Don't use platform-specific file/stream I/O
#
# MANFMT:		man:    classic troff/man
# 			mdoc:   BSD mandoc
# 			defaults to mdoc when `uname` contains "BSD",
# 			man otherwise.

BOOLCONFVARS_ON=	WITH_CURSES WITH_MAN WITH_SHOWANSI
BOOLCONFVARS_OFF=	FORCE_STDIO WITH_HTML
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

$(call zinc, src/bin/bin.mk)
