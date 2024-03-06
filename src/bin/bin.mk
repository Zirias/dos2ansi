MANFMT:=	$(or $(MANFMT),$(if $(findstring BSD,$(SYSNAME)),mdoc,man))

GEN_CHELP_tool=	$(MKCLIDOC_TARGET)
GEN_CHELP_args=	-fcpp -o$1 $2
GEN_SHELP_tool=	$(MKCLIDOC_TARGET)
GEN_SHELP_args=	-fsh,t=$3 -o$1 $2
GEN_MAN_tool=	$(MKCLIDOC_TARGET)
GEN_MAN_args=	-f$(MANFMT) -o$1 $2

$(call zinc, dos2ansi/dos2ansi.mk)
ifeq ($(PLATFORM),posix)
  ifeq ($(WITH_SHOWANSI),1)
$(call zinc, showansi/showansi.mk)
  endif
endif
