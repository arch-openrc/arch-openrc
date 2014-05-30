## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-I$(top_builddir)/src -I$(top_srcdir)/src		\
	-DUP_COMPILATION					\
	-DG_LOG_DOMAIN=\"UPower-Linux\"				\
	-DPACKAGE_SYSCONF_DIR=\""$(sysconfdir)"\"		\
	-I$(top_srcdir)/libupower-glib				\
	$(USB_CFLAGS)						\
	$(GIO_CFLAGS)						\
	$(DBUS_GLIB_CFLAGS)					\
	$(GUDEV_CFLAGS)						\
	$(POLKIT_CFLAGS)					\
	$(GLIB_CFLAGS)						\
	$(IDEVICE_CFLAGS)					\
	$(NULL)

if BACKEND_TYPE_LINUX
noinst_LTLIBRARIES = libupshared.la
endif

if HAVE_IDEVICE
idevice_files = up-device-idevice.c up-device-idevice.h
else
idevice_files =
endif

libupshared_la_SOURCES =					\
	up-device-supply.c					\
	up-device-supply.h					\
	up-device-csr.c						\
	up-device-csr.h						\
	up-device-unifying.c					\
	up-device-unifying.h					\
	up-device-hid.c						\
	up-device-hid.h						\
	up-device-wup.c						\
	up-device-wup.h						\
	up-input.c						\
	up-input.h						\
	up-dock.c						\
	up-dock.h						\
	up-backend.c						\
	up-native.c						\
	hidpp-device.c						\
	hidpp-device.h						\
	sysfs-utils.c						\
	sysfs-utils.h						\
	$(idevice_files)					\
	$(BUILT_SOURCES)

noinst_PROGRAMS =						\
	hidpp-test
hidpp_test_SOURCES =						\
	hidpp-device.c						\
	hidpp-device.h						\
	hidpp-test.c
hidpp_test_LDADD =						\
	-lm							\
	$(GLIB_LIBS)						\
	$(GIO_LIBS)
hidpp_test_CFLAGS = $(AM_CFLAGS) $(WARNINGFLAGS_C)

EXTRA_DIST = $(libupshared_la_SOURCES) 			\
	     integration-test

libupshared_la_CFLAGS =					\
	$(WARNINGFLAGS_C)

libupshared_la_LIBADD = $(GIO_LIBS)

clean-local :
	rm -f *~

-include $(top_srcdir)/git.mk
