# WITH_CURSES:		Build curses/terminfo writer (non-Windows only)
# WITH_SHOWANSI:	Install the showansi script (non-Windows only)
# FORCE_STDIO:		Don't use platform-specific file/stream I/O

BOOLCONFVARS_ON=	WITH_CURSES WITH_SHOWANSI
BOOLCONFVARS_OFF=	FORCE_STDIO

DISTCLEANGOALS=	cleanmkclidoc
NODIST=		tools/mkclidoc/zimk

include zimk/zimk.mk

MKCLIDOCDIR=	tools$(PSEP)mkclidoc
MKCLIDOC=	$(MKCLIDOCDIR)$(PSEP)mkclidoc$(EXE)
DISTCLEAN+=	$(MKCLIDOC)

$(MKCLIDOC):
	+@$(MAKE) -C $(MKCLIDOCDIR) DESTDIR=. GIT=$(GIT) PORTABLE=1 \
		zimkdir=../../zimk install

cleanmkclidoc:
	+@$(MAKE) -C $(MKCLIDOCDIR) distclean

.PHONY: cleanmkclidoc

$(call zinc, src/bin/dos2ansi/dos2ansi.mk)
ifeq ($(PLATFORM),posix)
  ifeq ($(CROSS_COMPILE),)
MANFORMAT:=	-f$(if $(findstring BSD,$(SYSNAME)),mdoc,man)
  endif
  ifeq ($(WITH_SHOWANSI),1)
$(call zinc, src/bin/showansi/showansi.mk)
  endif
endif
