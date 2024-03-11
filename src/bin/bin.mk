ifeq ($(PLATFORM),win32)
WITH_CURSES:=	0
WITH_HTML:=	1
WITH_MAN:=	0
WITH_SHOWANSI:=	0
else
MANFMT:=	$(or $(MANFMT),$(if $(findstring BSD,$(SYSNAME)),mdoc,man))
endif

GEN_CHELP_tool=	$(MKCLIDOC_TARGET)
GEN_CHELP_args=	-fcpp -o$1 $2
GEN_SHELP_tool=	$(MKCLIDOC_TARGET)
GEN_SHELP_args=	-fsh,t=$3 -o$1 $2
GEN_MAN_tool=	$(MKCLIDOC_TARGET)
GEN_MAN_args=	-f$(MANFMT) -o$1 $2
GEN_HTML_tool=	$(MKCLIDOC_TARGET)
GEN_HTML_args=	-fhtml -o$1 $2

$(call zinc, dos2ansi/dos2ansi.mk)

ifeq ($(WITH_SHOWANSI),1)
$(call zinc, showansi/showansi.mk)
endif
