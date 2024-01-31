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

$(call binrules, dos2ansi)
