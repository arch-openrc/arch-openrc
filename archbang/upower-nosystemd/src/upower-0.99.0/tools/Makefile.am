## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	$(PIE_CFLAGS)						\
	-DUP_COMPILATION					\
	-DG_LOG_DOMAIN=\"UPower\"				\
	-I$(top_srcdir)						\
	-I$(top_srcdir)/libupower-glib				\
	$(DBUS_GLIB_CFLAGS)					\
	$(POLKIT_DBUS_CFLAGS)					\
	$(GLIB_CFLAGS)

UPOWER_LIBS = $(top_builddir)/libupower-glib/libupower-glib.la

bin_PROGRAMS = upower

upower_SOURCES = 					\
	up-tool.c					\
	$(BUILT_SOURCES)

upower_CPPFLAGS = 					\
	$(AM_CPPFLAGS)

upower_LDFLAGS =
	-pthread					\
	$(PIE_LDFLAGS)

upower_LDADD = 						\
	$(DBUS_GLIB_LIBS)				\
	$(UPOWER_LIBS)					\
	$(POLKIT_DBUS_LIBS)

CLEANFILES = $(BUILT_SOURCES)

clean-local :
	rm -f *~ $(service_DATA) $(dbusconf_DATA)

-include $(top_srcdir)/git.mk
