BOOLCONFVARS=	WITH_CURSES

WITH_CURSES?=	1

include zimk/zimk.mk

$(call zinc, src/bin/dos2ansi/dos2ansi.mk)
