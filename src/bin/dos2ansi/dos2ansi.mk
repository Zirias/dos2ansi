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

dos2ansi_VERSION:=	1.8
dos2ansi_SUB_FILES:=	decl.h \
			dos2ansi.cdoc

dos2ansi_GEN:=		CHELP
dos2ansi_CHELP_FILES:=	help.h:dos2ansi.cdoc

ifeq ($(WITH_CURSES),1)
dos2ansi_MODULES+=	ticolorwriter
dos2ansi_LIBS+=		curses
dos2ansi_DEFINES+=	-DWITH_CURSES
endif

ifeq ($(WITH_HTML),1)
dos2ansi_EXTRADIRS:=	doc
dos2ansi_doc_FILES:=	dos2ansi.1.html
dos2ansi_GEN+=		HTML
dos2ansi_HTML_FILES:=	dos2ansi.1.html:dos2ansi.cdoc
endif

ifeq ($(WITH_MAN),1)
dos2ansi_MAN1:=		dos2ansi
dos2ansi_GEN+=		MAN
dos2ansi_MAN_FILES:=	dos2ansi.1:dos2ansi.cdoc
endif

ifeq ($(FORCE_STDIO),1)
dos2ansi_DEFINES+=	-DFORCE_STDIO
endif

ifeq ($(PLATFORM),win32)
dos2ansi_MODULES+=	winconsolewriter
dos2ansi_win32_RES:=	windres
dos2ansi_DEFINES+=	-DOSNAME=\"Windows\"
dos2ansi_SUB_FILES+=	dos2ansi.exe.manifest \
			windres.rc
endif

ifeq ($(PLATFORM),posix)
  ifeq ($(CROSS_COMPILE),)
dos2ansi_DEFINES+=	-DOSNAME=\"$(SYSNAME)\"
  endif
endif

$(call binrules, dos2ansi)
