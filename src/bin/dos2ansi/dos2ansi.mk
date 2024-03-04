dos2ansi_MODULES:=	ansicolorwriter \
			ansisysrenderer \
			bufferedwriter \
			codepage \
			config \
			dosreader \
			main \
			sauce \
			sauceprinter \
			saucequery \
			stream \
			testwriter \
			unicodewriter \
			util \
			vgacanvas

dos2ansi_VERSION:=	1.7
dos2ansi_SUB_FILES:=	decl.h \
			dos2ansi.cdoc

ifeq ($(PLATFORM),posix)
ifeq ($(WITH_CURSES),1)
dos2ansi_MODULES+=	ticolorwriter
dos2ansi_LIBS+=		curses
dos2ansi_DEFINES+=	-DWITH_CURSES
endif
dos2ansi_MAN1:=		dos2ansi
endif

ifeq ($(PLATFORM),win32)
dos2ansi_MODULES+=	winconsolewriter
dos2ansi_win32_RES:=	windres
dos2ansi_DEFINES+=	-DOSNAME=\"Windows\"
dos2ansi_SUB_FILES+=	dos2ansi.exe.manifest \
			windres.rc
else
ifeq ($(CROSS_COMPILE),)
dos2ansi_DEFINES+=	-DOSNAME=\"$(SYSNAME)\"
endif
endif

ifeq ($(FORCE_STDIO),1)
dos2ansi_DEFINES+=	-DFORCE_STDIO
endif

$(call binrules, dos2ansi)

D2A_HELP_H=	$(dos2ansi_SRCDIR)$(PSEP)help.h
D2A_MANPAGE=	$(dos2ansi_SRCDIR)$(PSEP)dos2ansi.1
D2A_CDOC=	$(dos2ansi_SRCDIR)$(PSEP)dos2ansi.cdoc

$(D2A_HELP_H): $(D2A_CDOC) $(MKCLIDOC)
	@$(VGEN)
	@$(VR) $(MKCLIDOC) -fcpp -o "$@" "$<"

dos2ansi_sub:	$(D2A_HELP_H)

$(D2A_MANPAGE): $(D2A_CDOC) $(MKCLIDOC)
	@$(VGEN)
	@$(VR) $(MKCLIDOC) $(MANFORMAT) -o "$@" "$<"

all::	$(D2A_MANPAGE)

CLEAN += $(D2A_HELP_H) $(D2A_MANPAGE)
DISTCLEAN += $(D2A_HELP_H) $(D2A_MANPAGE)
