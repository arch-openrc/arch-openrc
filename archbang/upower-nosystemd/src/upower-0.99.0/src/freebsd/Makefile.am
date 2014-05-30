## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-I$(top_builddir)/src -I$(top_srcdir)/src		\
	-DUP_COMPILATION					\
	-DG_LOG_DOMAIN=\"UPower-Freebsd\"			\
	-I$(top_srcdir)/libupower-glib				\
	$(DBUS_GLIB_CFLAGS)					\
	$(POLKIT_CFLAGS)					\
	$(GLIB_CFLAGS)

if BACKEND_TYPE_FREEBSD
noinst_LTLIBRARIES = libupshared.la
endif

libupshared_la_SOURCES =					\
	up-acpi-native.c					\
	up-acpi-native.h					\
	up-backend-acpi.h					\
	up-backend.c						\
	up-devd.c						\
	up-devd.h						\
	up-device-supply.c					\
	up-device-supply.h					\
	up-native.c						\
	up-util.c						\
	up-util.h						\
	$(BUILT_SOURCES)

libupshared_la_CFLAGS =					\
	$(WARNINGFLAGS_C)

libupshared_la_LIBADD =					\
	-lkvm

EXTRA_DIST =							\
	up-acpi-native.vala					\
	TODO

clean-local :
	rm -f *~

-include $(top_srcdir)/git.mk
