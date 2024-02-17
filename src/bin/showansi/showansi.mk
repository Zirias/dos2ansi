# disable building and stripping:
showansi_TGTDIR=	$(showansi_SRCDIR)
showansi_STRIPWITH:=

$(call binrules, showansi)
