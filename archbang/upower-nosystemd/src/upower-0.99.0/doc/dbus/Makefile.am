
all : org.freedesktop.UPower.ref.xml org.freedesktop.UPower.Device.ref.xml org.freedesktop.UPower.KbdBacklight.ref.xml org.freedesktop.UPower.Wakeups.ref.xml

org.freedesktop.UPower.ref.xml : $(top_srcdir)/src/org.freedesktop.UPower.xml $(top_srcdir)/doc/dbus/spec-to-docbook.xsl
	echo "<?xml version=\"1.0\"?>""<!DOCTYPE refentry PUBLIC \"-//OASIS//DTD DocBook XML V4.1.2//EN\" \"http://www.oasis-open.org/docbook/xml/4.1.2/docbookx.dtd\">" > $@
	$(XSLTPROC) $(top_srcdir)/doc/dbus/spec-to-docbook.xsl $< | tail -n +2 >> $@

org.freedesktop.UPower.Device.ref.xml : $(top_srcdir)/src/org.freedesktop.UPower.Device.xml $(top_srcdir)/doc/dbus/spec-to-docbook.xsl
	echo "<?xml version=\"1.0\"?>""<!DOCTYPE refentry PUBLIC \"-//OASIS//DTD DocBook XML V4.1.2//EN\" \"http://www.oasis-open.org/docbook/xml/4.1.2/docbookx.dtd\">" > $@
	$(XSLTPROC) $(top_srcdir)/doc/dbus/spec-to-docbook.xsl $< | tail -n +2 >> $@

org.freedesktop.UPower.KbdBacklight.ref.xml : $(top_srcdir)/src/org.freedesktop.UPower.KbdBacklight.xml $(top_srcdir)/doc/dbus/spec-to-docbook.xsl
	echo "<?xml version=\"1.0\"?>""<!DOCTYPE refentry PUBLIC \"-//OASIS//DTD DocBook XML V4.1.2//EN\" \"http://www.oasis-open.org/docbook/xml/4.1.2/docbookx.dtd\">" > $@
	$(XSLTPROC) $(top_srcdir)/doc/dbus/spec-to-docbook.xsl $< | tail -n +2 >> $@

org.freedesktop.UPower.Wakeups.ref.xml : $(top_srcdir)/src/org.freedesktop.UPower.Wakeups.xml $(top_srcdir)/doc/dbus/spec-to-docbook.xsl
	echo "<?xml version=\"1.0\"?>""<!DOCTYPE refentry PUBLIC \"-//OASIS//DTD DocBook XML V4.1.2//EN\" \"http://www.oasis-open.org/docbook/xml/4.1.2/docbookx.dtd\">" > $@
	$(XSLTPROC) $(top_srcdir)/doc/dbus/spec-to-docbook.xsl $< | tail -n +2 >> $@

EXTRA_DIST = spec-to-docbook.xsl dbus-introspect-docs.dtd

MAINTAINERCLEANFILES = *.ref.xml

clean-local :
	rm -f *~ *.ref.xml

-include $(top_srcdir)/git.mk
