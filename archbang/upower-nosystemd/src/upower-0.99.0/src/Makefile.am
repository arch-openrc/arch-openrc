## Process this file with automake to produce Makefile.in

SUBDIRS = dummy freebsd linux openbsd

AM_CPPFLAGS = \
	$(PIE_CFLAGS)						\
	-I$(top_builddir)/src -I$(top_srcdir)/src		\
	-DPACKAGE_LIBEXEC_DIR=\""$(libexecdir)"\"		\
	-DPACKAGE_SYSCONF_DIR=\""$(sysconfdir)"\"		\
	-DPACKAGE_DATA_DIR=\""$(datadir)"\"			\
	-DPACKAGE_BIN_DIR=\""$(bindir)"\"			\
	-DPACKAGE_LOCALSTATE_DIR=\""$(localstatedir)"\"		\
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\"			\
	-DPACKAGE_LIB_DIR=\""$(libdir)"\"			\
	-DHISTORY_DIR=\""$(historydir)"\"			\
	-D_POSIX_PTHREAD_SEMANTICS -D_REENTRANT			\
	-DUP_COMPILATION					\
	-DG_LOG_DOMAIN=\"UPower\"				\
	-I$(top_srcdir)/libupower-glib				\
	-I$(top_srcdir)						\
	$(GIO_CFLAGS)						\
	$(DBUS_GLIB_CFLAGS)					\
	$(GUDEV_CFLAGS)						\
	$(POLKIT_CFLAGS)					\
	$(GLIB_CFLAGS)

UPOWER_LIBS = $(top_builddir)/libupower-glib/libupower-glib.la

BUILT_SOURCES =							\
	up-daemon-glue.h					\
	up-device-glue.h					\
	up-kbd-backlight-glue.h					\
	up-wakeups-glue.h					\
	up-marshal.h						\
	up-marshal.c

up-marshal.h: up-marshal.list
	glib-genmarshal $< --prefix=up_marshal --header > $@

up-marshal.c: up-marshal.list
	echo "#include \"up-marshal.h\"" > $@ && glib-genmarshal $< --prefix=up_marshal --body >> $@

up-daemon-glue.h: org.freedesktop.UPower.xml Makefile.am
	dbus-binding-tool --prefix=up_daemon --mode=glib-server --output=up-daemon-glue.h $(srcdir)/org.freedesktop.UPower.xml

up-device-glue.h: org.freedesktop.UPower.Device.xml Makefile.am
	dbus-binding-tool --prefix=up_device --mode=glib-server --output=up-device-glue.h $(srcdir)/org.freedesktop.UPower.Device.xml

up-kbd-backlight-glue.h: org.freedesktop.UPower.KbdBacklight.xml Makefile.am
	dbus-binding-tool --prefix=up_kbd_backlight --mode=glib-server --output=up-kbd-backlight-glue.h $(srcdir)/org.freedesktop.UPower.KbdBacklight.xml

up-wakeups-glue.h: org.freedesktop.UPower.Wakeups.xml Makefile.am
	dbus-binding-tool --prefix=up_wakeups --mode=glib-server --output=up-wakeups-glue.h $(srcdir)/org.freedesktop.UPower.Wakeups.xml

libexec_PROGRAMS = upowerd

dbusifdir = $(datadir)/dbus-1/interfaces
dbusif_DATA =							\
	org.freedesktop.UPower.xml				\
	org.freedesktop.UPower.Device.xml			\
	org.freedesktop.UPower.KbdBacklight.xml			\
	org.freedesktop.UPower.Wakeups.xml

upowerd_SOURCES =						\
	up-polkit.h						\
	up-polkit.c						\
	up-daemon.h						\
	up-daemon.c						\
	up-device.h						\
	up-device.c						\
	up-device-list.h					\
	up-device-list.c					\
	up-config.h						\
	up-config.c						\
	up-kbd-backlight.h					\
	up-kbd-backlight.c					\
	up-wakeups.h						\
	up-wakeups.c						\
	up-history.h						\
	up-history.c						\
	up-backend.h						\
	up-native.h						\
	up-main.c						\
	$(BUILT_SOURCES)

upowerd_CPPFLAGS =						\
	-I$(top_srcdir)/src					\
	$(AM_CPPFLAGS)

upowerd_LDADD =							\
	-lm							\
	$(USB_LIBS)						\
	$(GIO_LIBS)						\
	$(DBUS_GLIB_LIBS)					\
	$(POLKIT_LIBS)						\
	$(UPOWER_LIBS)

if BACKEND_TYPE_DUMMY
upowerd_LDADD += 						\
	dummy/libupshared.la
endif

if BACKEND_TYPE_FREEBSD
upowerd_LDADD +=						\
	freebsd/libupshared.la
endif

if BACKEND_TYPE_OPENBSD
upowerd_LDADD +=						\
	openbsd/libupshared.la
endif

if BACKEND_TYPE_LINUX
upowerd_LDADD += 						\
	linux/libupshared.la					\
	$(USB_LIBS)						\
	$(GUDEV_LIBS)						\
	$(IDEVICE_LIBS)						\
	$(NULL)

upowerd_LDFLAGS =						\
	$(PIE_LDFLAGS)						\
	$(RELRO_LDFLAGS)

if UP_BUILD_TESTS
check-local: upowerd
	env GI_TYPELIB_PATH=$(top_builddir)/libupower-glib:$(GI_TYPELIB_PATH) LD_LIBRARY_PATH=$(top_builddir)/libupower-glib/.libs:$(LD_LIBRARY_PATH) top_builddir=$(top_builddir) $(srcdir)/linux/integration-test -v
endif

endif

upowerd_CFLAGS =						\
	$(WARNINGFLAGS_C)					\
	$(NULL)

if UP_BUILD_TESTS

check_PROGRAMS =						\
	up-self-test

up_self_test_SOURCES =						\
	up-self-test.c						\
	up-polkit.h						\
	up-polkit.c						\
	up-config.h						\
	up-config.c						\
	up-daemon.h						\
	up-daemon.c						\
	up-device.h						\
	up-device.c						\
	up-device-list.h					\
	up-device-list.c					\
	up-kbd-backlight.h					\
	up-kbd-backlight.c					\
	up-wakeups.h						\
	up-wakeups.c						\
	up-history.h						\
	up-history.c						\
	up-backend.h						\
	up-native.h						\
	$(BUILT_SOURCES)

up_self_test_LDADD =						\
	-lm							\
	dummy/libuptest.la					\
	$(GLIB_LIBS)						\
	$(GIO_CFLAGS)						\
	$(DBUS_GLIB_LIBS)					\
	$(POLKIT_LIBS)						\
	$(UPOWER_LIBS)

up_self_test_CFLAGS = $(AM_CFLAGS) $(WARNINGFLAGS_C)

TESTS_ENVIRONMENT = $(DBUS_LAUNCH)
TESTS = up-self-test

endif

dbusservicedir       = $(datadir)/dbus-1/system-services
dbusservice_in_files = org.freedesktop.UPower.service.in
dbusservice_DATA     = $(dbusservice_in_files:.service.in=.service)

$(dbusservice_DATA): $(dbusservice_in_files) Makefile
	@sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

dbusconfdir = $(sysconfdir)/dbus-1/system.d
dbusconf_in_files = org.freedesktop.UPower.conf.in
dbusconf_DATA = $(dbusconf_in_files:.conf.in=.conf)

$(dbusconf_DATA): $(dbusconf_in_files) Makefile
	cp $< $@

if HAVE_SYSTEMDSYSTEMUNITDIR
systemdservicedir       = $(systemdsystemunitdir)
systemdservice_in_files = upower.service.in
systemdservice_DATA     = $(systemdservice_in_files:.service.in=.service)

$(systemdservice_DATA): $(systemdservice_in_files) Makefile
	@sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@
endif

install-data-hook:
	if test -w $(DESTDIR)$(prefix)/; then \
		mkdir -p $(DESTDIR)$(historydir); \
	fi

CLEANFILES = $(BUILT_SOURCES)

MAINTAINERCLEANFILES =						\
	$(dbusconf_DATA)					\
	$(systemdservice_DATA)					\
	$(dbusservice_DATA)

EXTRA_DIST =							\
	org.freedesktop.UPower.xml				\
	org.freedesktop.UPower.Device.xml			\
	org.freedesktop.UPower.KbdBacklight.xml			\
	org.freedesktop.UPower.Wakeups.xml			\
	up-marshal.list						\
	$(systemdservice_in_files)				\
	$(dbusservice_in_files)					\
	$(dbusconf_in_files)

if HAVE_SYSTEMDSYSTEMUNITDIR
EXTRA_DIST += $(systemdservice_in_files)
endif

clean-local :
	rm -f *~ $(dbusservice_DATA) $(dbusconf_DATA) $(systemdservice_DATA)

-include $(top_srcdir)/git.mk
