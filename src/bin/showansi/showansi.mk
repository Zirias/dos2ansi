showansi_fontsdir=	$(dos2ansi_datadir)$(PSEP)showansi
showansi_htmldir=	$(dos2ansi_htmldir)
showansi_sysconfdir=	$(sysconfdir)$(PSEP)dos2ansi

showansi_NOBUILD:=	1
showansi_VERSION:=	$(dos2ansi_VERSION)
showansi_SUB_FILES:=	fonts.bitmap fonts.mixed fonts.scalable \
			showansi showansi.cdoc showansirc.sample
showansi_SUB_LIST:=	"DOS2ANSI=$(bindir)$(PSEP)dos2ansi" \
			"FONTSDIR=$(showansi_fontsdir)" \
			"SYSCONFDIR=$(showansi_sysconfdir)"
showansi_GEN:=		SHELP
showansi_SHELP_FILES:=	showansi.in:showansi.cdoc:showansi.in.tmpl

showansi_EXTRADIRS:=		fonts sysconf
showansi_fonts_FILES:=		fonts.bitmap fonts.mixed fonts.scalable
showansi_sysconf_FILES:=	showansirc.sample

ifeq ($(WITH_MAN),1)
showansi_MAN1:=		showansi
showansi_GEN+=		MAN
showansi_MAN_FILES:=	showansi.1:showansi.cdoc
endif

ifeq ($(WITH_HTML),1)
showansi_EXTRADIRS+=	html
showansi_html_FILES:=	showansi.1.html
showansi_GEN+=		HTML
showansi_HTML_FILES:=	showansi.1.html:showansi.cdoc
endif

$(call binrules, showansi)
