## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-I$(top_builddir)/src -I$(top_srcdir)/src		\
	-DUP_COMPILATION					\
	-DG_LOG_DOMAIN=\"UPower-Dummy\"				\
	-I$(top_srcdir)/libupower-glib				\
	$(DBUS_GLIB_CFLAGS)					\
	$(POLKIT_CFLAGS)					\
	$(GLIB_CFLAGS)

noinst_LTLIBRARIES =

if BACKEND_TYPE_DUMMY
noinst_LTLIBRARIES += libupshared.la
endif

libupshared_la_SOURCES =					\
	up-backend.c						\
	up-native.c						\
	$(BUILT_SOURCES)

if UP_BUILD_TESTS
noinst_LTLIBRARIES += libuptest.la
libuptest_la_CFLAGS = -DEGG_TEST
libuptest_la_SOURCES = $(libupshared_la_SOURCES)
endif

clean-local :
	rm -f *~

-include $(top_srcdir)/git.mk
