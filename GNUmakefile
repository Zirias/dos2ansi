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

DISTCLEANGOALS=	cleantools
NODIST=		tools/mkclidoc/zimk
include zimk/zimk.mk

MANFMT:=	$(or $(MANFMT),$(if $(findstring BSD,$(SYSNAME)),mdoc,man))

TOOLBINDIR=	tools$(PSEP)bin
MKCLIDOC=	$(TOOLBINDIR)$(PSEP)mkclidoc$(HOSTEXE)
MKCLIDOCSRC=	tools$(PSEP)mkclidoc
DISTCLEANDIRS=	$(TOOLBINDIR)

$(MKCLIDOC):
	+@$(MAKE) -C $(MKCLIDOCSRC) DESTDIR=..$(PSEP)bin HOSTBUILD=1 \
		PORTABLE=1 STATIC=0 zimkdir=../../zimk install

cleantools:
	+@$(MAKE) -C $(MKCLIDOCSRC) zimkdir=../../zimk distclean

.PHONY: cleantools

$(call zinc, src/bin/dos2ansi/dos2ansi.mk)
ifeq ($(PLATFORM),posix)
  ifeq ($(WITH_SHOWANSI),1)
$(call zinc, src/bin/showansi/showansi.mk)
  endif
endif
