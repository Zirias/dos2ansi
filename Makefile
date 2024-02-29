# WITH_CURSES:		Build curses/terminfo writer (non-Windows only)
# WITH_SHOWANSI:	Install the showansi script (non-Windows only)
# FORCE_STDIO:		Don't use platform-specific file/stream I/O

BOOLCONFVARS_ON=	WITH_CURSES WITH_SHOWANSI
BOOLCONFVARS_OFF=	FORCE_STDIO

include zimk/zimk.mk

$(call zinc, src/bin/dos2ansi/dos2ansi.mk)
ifeq ($(PLATFORM),posix)
ifeq ($(WITH_SHOWANSI),1)
$(call zinc, src/bin/showansi/showansi.mk)
endif
endif
