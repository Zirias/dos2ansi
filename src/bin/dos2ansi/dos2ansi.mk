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

dos2ansi_VERSION:=	1.6.1
dos2ansi_SUB_FILES:=	decl.h

ifeq ($(PLATFORM),posix)
ifeq ($(WITH_CURSES),1)
dos2ansi_MODULES+=	ticolorwriter
dos2ansi_LIBS+=		curses
dos2ansi_DEFINES+=	-DWITH_CURSES
endif
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

ifeq ($(PLATFORM),posix)
ifeq ($(WITH_SHOWANSI),1)

showansi_NOBUILD:=	1
showansi_VERSION:=	$(dos2ansi_VERSION)
showansi_SUB_FILES:=	showansi showansirc.sample
showansi_SUB_LIST:=	"DOS2ANSI=$(bindir)/dos2ansi" \
			"SYSCONFDIR=$(sysconfdir)"
showansi_SYSCONF:=	showansirc.sample

$(call binrules, showansi)

endif
endif
