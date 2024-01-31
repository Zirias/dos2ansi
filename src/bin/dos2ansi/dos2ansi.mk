dos2ansi_MODULES:=	ansicolorwriter \
			bufferedwriter \
			codepage \
			config \
			dosreader \
			main \
			stream \
			testwriter \
			unicodewriter \
			util \
			vgacanvas

ifeq ($(WITH_CURSES),1)
ifeq ($(PLATFORM),posix)
dos2ansi_MODULES+=	ticolorwriter
dos2ansi_LIBS+=		curses
dos2ansi_DEFINES+=	-DWITH_CURSES
endif
endif

ifeq ($(PLATFORM),win32)
dos2ansi_MODULES+=	winconsolewriter
dos2ansi_win32_RES:=	windres
endif

$(call binrules, dos2ansi)
