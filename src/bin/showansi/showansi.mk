showansi_NOBUILD:=	1
showansi_VERSION:=	$(dos2ansi_VERSION)
showansi_SUB_FILES:=	showansi
showansi_SUB_LIST:=	"DOS2ANSI=$(bindir)/dos2ansi"

$(call binrules, showansi)
