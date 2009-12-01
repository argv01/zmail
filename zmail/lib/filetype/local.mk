# lib/filetype/local.mk

zmail.ftr:	zmail.ftr.m4
	m4 $(CONFIG_DEFS) $@.m4 > $@
