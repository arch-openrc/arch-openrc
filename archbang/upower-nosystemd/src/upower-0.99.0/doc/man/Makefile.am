if MAN_PAGES_ENABLED

man_MANS = 				\
	upower.1			\
	upowerd.8			\
	UPower.7

%.1 %.7 %.8 : %.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

MAINTAINERCLEANFILES = $(man_MANS)

endif # MAN_PAGES_ENABLED

EXTRA_DIST= 				\
	upower.xml			\
	upowerd.xml			\
	UPower.xml

clean-local:
	rm -f *~ *.[178]

-include $(top_srcdir)/git.mk
