BOOLCONFVARS=	WITH_CURSES FORCE_STDIO

WITH_CURSES?=	1	# Build curses/terminfo writer (non-Windows only)
FORCE_STDIO?=	0	# Don't use platform-specific file/stream I/O

include zimk/zimk.mk

$(call zinc, src/bin/dos2ansi/dos2ansi.mk)
