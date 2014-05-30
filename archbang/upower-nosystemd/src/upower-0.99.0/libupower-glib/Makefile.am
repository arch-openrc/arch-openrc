AM_CPPFLAGS = \
	$(GLIB_CFLAGS)						\
	$(GIO_UNIX_CFLAGS)					\
	-I$(top_builddir)					\
	-I$(top_srcdir)						\
	-I$(top_srcdir)/libupower-glib				\
	-DUP_COMPILATION					\
	-DG_LOG_DOMAIN=\"libupower-glib\"			\
	-DPACKAGE_DATA_DIR=\""$(datadir)"\"			\
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"

-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --add-include-path=$(srcdir)
INTROSPECTION_COMPILER_ARGS = --includedir=$(srcdir)

BUILT_SOURCES = $(NULL)

up_client_dbus_built_sources = up-client-glue.c up-client-glue.h
BUILT_SOURCES += $(up_client_dbus_built_sources)

$(up_client_dbus_built_sources) : Makefile.am $(top_srcdir)/src/org.freedesktop.UPower.xml
	$(AM_V_GEN) gdbus-codegen                                       \
		--interface-prefix org.freedesktop.UPower.              \
		--generate-c-code up-client-glue                        \
		--c-namespace Up                                        \
		--annotate "org.freedesktop.UPower"                     \
		"org.gtk.GDBus.C.Name" ClientGlue                       \
		$(top_srcdir)/src/org.freedesktop.UPower.xml

up_device_dbus_built_sources = up-device-glue.c up-device-glue.h
BUILT_SOURCES += $(up_device_dbus_built_sources)

$(up_device_dbus_built_sources) : Makefile.am $(top_srcdir)/src/org.freedesktop.UPower.Device.xml
	$(AM_V_GEN) gdbus-codegen                                       \
		--interface-prefix org.freedesktop.UPower.Device.       \
		--generate-c-code up-device-glue                        \
		--c-namespace Up                                        \
		--annotate "org.freedesktop.UPower.Device"              \
		"org.gtk.GDBus.C.Name" DeviceGlue                       \
		$(top_srcdir)/src/org.freedesktop.UPower.Device.xml

up_wakeups_dbus_built_sources = up-wakeups-glue.c up-wakeups-glue.h
BUILT_SOURCES += $(up_wakeups_dbus_built_sources)

$(up_wakeups_dbus_built_sources) : Makefile.am $(top_srcdir)/src/org.freedesktop.UPower.Wakeups.xml
	$(AM_V_GEN) gdbus-codegen                                       \
		--interface-prefix org.freedesktop.UPower.Wakeups.      \
		--generate-c-code up-wakeups-glue                       \
		--c-namespace Up                                        \
		--annotate "org.freedesktop.UPower.Wakeups"             \
		"org.gtk.GDBus.C.Name" WakeupsGlue                      \
		$(top_srcdir)/src/org.freedesktop.UPower.Wakeups.xml

lib_LTLIBRARIES =						\
	libupower-glib.la

libupower_glib_includedir = $(includedir)/libupower-glib
libupower_glib_include_HEADERS =				\
	upower.h						\
	up-version.h						\
	up-types.h						\
	up-device.h						\
	up-wakeup-item.h					\
	up-stats-item.h						\
	up-history-item.h					\
	up-wakeups.h						\
	up-client.h

libupower_glib_la_SOURCES =					\
	up-types.c						\
	up-client.c						\
	up-wakeups.c						\
	up-wakeup-item.c					\
	up-stats-item.c						\
	up-history-item.c					\
	up-device.c						\
	$(up_client_dbus_built_sources)				\
	$(up_device_dbus_built_sources)				\
	$(up_wakeups_dbus_built_sources)

libupower_glib_la_LIBADD =					\
	$(INTLLIBS)						\
	$(GLIB_LIBS)						\
	$(GIO_UNIX_LIBS)

libupower_glib_la_LDFLAGS =					\
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)	\
	-export-dynamic						\
	-no-undefined						\
	-export-symbols-regex '^up_.*'

libupower_glib_la_CFLAGS =					\
	$(WARNINGFLAGS_C)					\
	$(NULL)

EXTRA_DIST =							\
	up-version.h.in

CLEANFILES = $(BUILT_SOURCES)

if HAVE_INTROSPECTION
introspection_sources = $(libupower_glib_include_HEADERS) $(libupower_glib_la_SOURCES)

UPowerGlib-1.0.gir: libupower-glib.la
UPowerGlib_1_0_gir_INCLUDES = GObject-2.0 Gio-2.0
UPowerGlib_1_0_gir_CFLAGS = $(AM_CPPFLAGS)
UPowerGlib_1_0_gir_SCANNERFLAGS = --identifier-prefix=Up --symbol-prefix=up_ --warn-all --add-include-path=$(srcdir)
UPowerGlib_1_0_gir_LIBS = libupower-glib.la
UPowerGlib_1_0_gir_FILES = $(introspection_sources)
UPowerGlib_1_0_gir_NAMESPACE = UPowerGlib
INTROSPECTION_GIRS += UPowerGlib-1.0.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)
endif

clean-local:
	rm -f *~

-include $(top_srcdir)/git.mk
