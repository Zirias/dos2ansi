BOOLCONFVARS=	WITH_CURSES WITH_SHOWANSI FORCE_STDIO

WITH_CURSES?=	1	# Build curses/terminfo writer (non-Windows only)
WITH_SHOWANSI?=	1	# Install the showansi script (non-Windows only)
FORCE_STDIO?=	0	# Don't use platform-specific file/stream I/O

include zimk/zimk.mk

$(call zinc, src/bin/dos2ansi/dos2ansi.mk)
ifeq ($(PLATFORM),posix)
ifeq ($(WITH_SHOWANSI),1)
$(call zinc, src/bin/showansi/showansi.mk)
endif
endif
