showansi_NOBUILD:=	1

_dos2ansi_name:=	$(dos2ansi_TARGET)
_dos2ansi_dir:=		$($(dos2ansi_INSTALLDIRNAME)dir)
showansi_SUB_FILES:=	showansi
showansi_SUB_LIST:=	"DOS2ANSI=$(_dos2ansi_dir)$(PSEP)$(_dos2ansi_name)"

$(call binrules, showansi)
