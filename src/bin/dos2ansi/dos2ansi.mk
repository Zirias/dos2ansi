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

dos2ansi_VERSION:=	1.3

dos2ansi_SUB_FILES:=	decl.h \
			dos2ansi.exe.manifest \
			windres.rc

ifeq ($(PLATFORM),posix)
ifeq ($(WITH_CURSES),1)
dos2ansi_MODULES+=	ticolorwriter
dos2ansi_LIBS+=		curses
dos2ansi_DEFINES+=	-DWITH_CURSES
endif
ifeq ($(WITH_SHOWANSI),1)
install:: install_showansi
endif
endif

ifeq ($(PLATFORM),win32)
dos2ansi_MODULES+=	winconsolewriter
dos2ansi_win32_RES:=	windres
dos2ansi_DEFINES+=	-DOSNAME=\"Windows\"
else
ifeq ($(CROSS_COMPILE),)
dos2ansi_DEFINES+=	-DOSNAME=\"$(SYSNAME)\"
endif
endif

ifeq ($(FORCE_STDIO),1)
dos2ansi_DEFINES+=	-DFORCE_STDIO
endif

$(call binrules, dos2ansi)

_ZIMK_0:=$(DESTDIR)$(bindir)$(PSEP)showansi
install_showansi: scripts$(PSEP)showansi
	$(VINST)
	$(VR)$(call instfile,$<,$(DESTDIR)$(bindir),755)

.PHONY: install_showansi
