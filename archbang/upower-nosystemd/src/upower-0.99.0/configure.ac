dnl Process this file with autoconf to produce a configure script.

m4_define([up_major_version], [0])
m4_define([up_minor_version], [99])
m4_define([up_micro_version], [0])
m4_define([up_version],
          [up_major_version.up_minor_version.up_micro_version])

AC_PREREQ(2.63)
AC_INIT([upower],
        [up_version],
        [https://bugs.freedesktop.org/enter_bug.cgi?product=upower],
        [upower],
        [http://upower.freedesktop.org/])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])

AM_INIT_AUTOMAKE([1.11 -Wno-portability no-dist-gzip dist-xz tar-ustar])
AM_MAINTAINER_MODE([enable])

# enable nice build output on automake1.11
AM_SILENT_RULES([yes])

GOBJECT_INTROSPECTION_CHECK([0.9.9])

# use this in up-version.h
UP_MAJOR_VERSION=up_major_version
UP_MINOR_VERSION=up_minor_version
UP_MICRO_VERSION=up_micro_version
AC_SUBST(VERSION)
AC_SUBST(UP_MAJOR_VERSION)
AC_SUBST(UP_MINOR_VERSION)
AC_SUBST(UP_MICRO_VERSION)

# libtool versioning - this applies to libupower-glib
#
# See http://sources.redhat.com/autobook/autobook/autobook_91.html#SEC91 for details
#
# increment;
# CURRENT	If the API or ABI interface has changed (reset REVISION to 0)
# REVISION	If the API and ABI remains the same, but bugs are fixed.
# AGE		Don't use.
LT_CURRENT=2
LT_REVISION=0
LT_AGE=0
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)

# Check for programs
AC_PROG_CC
AM_PROG_CC_C_O
AC_HEADER_STDC
AC_PROG_MAKE_SET
AC_PROG_LN_S
AC_SYS_LARGEFILE

# Initialize libtool
LT_INIT

# Build deprecated functionality
AC_ARG_ENABLE(deprecated, [  --enable-deprecated      build deprecated functionality],
	      enable_deprecated=$enableval,
	      enable_deprecated=no)
if test "${enable_deprecated}" != no; then
  AC_DEFINE(ENABLE_DEPRECATED, 1, [Define if we should build deprecated functionalty])
fi
AM_CONDITIONAL(ENABLE_DEPRECATED, test x$enable_deprecated = xyes)

# Build man pages?
AC_ARG_ENABLE(man-pages,        [  --enable-man-pages      build manual pages],enable_man_pages=$enableval,enable_man_pages=yes)
if test "${enable_man_page}" != no; then
  AC_PATH_PROG([XSLTPROC], [xsltproc])
  if test -z "$XSLTPROC"; then
    enable_man_pages=no
  fi
fi
AM_CONDITIONAL(MAN_PAGES_ENABLED, test x$enable_man_pages = xyes)

GTK_DOC_CHECK([1.11],[--flavour no-tmpl])

AC_ARG_WITH([udevrulesdir],
            AS_HELP_STRING([--with-udevrulesdir=DIR], [Directory for udev rules]),
            [],
            [with_udevrulesdir=$($PKG_CONFIG --variable=udevdir udev)"/rules.d"])
AC_SUBST([udevrulesdir], [$with_udevrulesdir])

# History/Db dir
AC_ARG_WITH([historydir],
	AS_HELP_STRING([--with-historydir=<dir>], [directory where the upower history files will be stored]))
if test "$with_historydir" != "no" -a "$with_historydir" != "" ; then
	historydir=$withval
else
	historydir=${localstatedir}/lib/upower
fi
AC_SUBST(historydir)

#### gcc warning flags

dnl ---------------------------------------------------------------------------
dnl - Extra verbose warning switches
dnl ---------------------------------------------------------------------------
CPPFLAGS="$CPPFLAGS -DG_DISABLE_SINGLE_INCLUDES -DGTK_DISABLE_SINGLE_INCLUDES"
CPPFLAGS="$CPPFLAGS -DG_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED"
CPPFLAGS="$CPPFLAGS -DGDK_MULTIHEAD_SAFE -DGTK_MULTIHEAD_SAFE"
CPPFLAGS="$CPPFLAGS -DGSEAL_ENABLE"

if test "$GCC" = "yes"; then
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wall"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wcast-align -Wno-uninitialized"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wmissing-declarations"
#	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wredundant-decls"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wpointer-arith"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wcast-align"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wwrite-strings"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Winit-self"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wreturn-type"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wformat-nonliteral"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wformat-security"
#	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wmissing-include-dirs"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wmissing-format-attribute"
#	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wclobbered"
#	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wempty-body"
#	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wignored-qualifiers"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wsign-compare"
#	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wtype-limits"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -O -Wuninitialized"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Waggregate-return"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wdeclaration-after-statement"
#	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wshadow"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wno-strict-aliasing"
#	WARNINGFLAGS_C="$WARNINGFLAGS_C -Winline"
#	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wmissing-parameter-type"
#	WARNINGFLAGS_C="$WARNINGFLAGS_C -Woverride-init"
else
	WARNINGFLAGS_C=""
fi
AC_SUBST(WARNINGFLAGS_C)

# check for PIE (position independent executable) support
AX_CHECK_COMPILE_FLAG([-fPIE],
		      [AX_CHECK_LINK_FLAG([-fPIE -pie],
					  [PIE_CFLAGS="-fPIE" PIE_LDFLAGS="-pie"])])
AC_SUBST(PIE_CFLAGS)
AC_SUBST(PIE_LDFLAGS)

# check for full RELRO (relocation read-only) support
AX_CHECK_LINK_FLAG([-Wl,-z,relro,-z,now],
		   [RELRO_LDFLAGS="-Wl,-z,relro,-z,now"])
AC_SUBST([RELRO_LDFLAGS])

PKG_CHECK_MODULES(GLIB, [glib-2.0 >= 2.29.19])
PKG_CHECK_MODULES(DBUS, [dbus-1 >= 1.0])
PKG_CHECK_MODULES(DBUS_GLIB, [dbus-glib-1 >= 0.76])
PKG_CHECK_MODULES(POLKIT, polkit-gobject-1 >= 0.97)
PKG_CHECK_MODULES(GIO, [gio-2.0 >= 2.16.1])
PKG_CHECK_MODULES(GIO_UNIX, [gio-unix-2.0])

dnl ====================================================================
dnl Check for systemd
dnl ====================================================================
AC_ARG_WITH([systemdutildir],
            AS_HELP_STRING([--with-systemdutildir=DIR], [Directory for systemd utilities]),
            [],
            [with_systemdutildir=$($PKG_CONFIG --variable=systemdutildir systemd)])
if test x$with_systemdutildir != xno; then
        AC_SUBST([systemdutildir], [$with_systemdutildir])
fi
AM_CONDITIONAL(HAVE_SYSTEMDUTILDIR, [test -n "$systemdutildir"])

AC_ARG_WITH([systemdsystemunitdir],
            AS_HELP_STRING([--with-systemdsystemunitdir=DIR], [Directory for systemd service files]),
            [],
            [with_systemdsystemunitdir=$($PKG_CONFIG --variable=systemdsystemunitdir systemd)])
if test x$with_systemdsystemunitdir != xno; then
        AC_SUBST([systemdsystemunitdir], [$with_systemdsystemunitdir])
fi
AM_CONDITIONAL(HAVE_SYSTEMDSYSTEMUNITDIR, [test -n "$systemdsystemunitdir"])

dnl ---------------------------------------------------------------------------
dnl - Compile time default choice of backend
dnl ---------------------------------------------------------------------------
AC_ARG_WITH([backend],
	    AS_HELP_STRING([--with-backend=<option>],
			   [Default backend to use linux, freebsd, openbsd, dummy (dummy)]))
# default to a sane option
AC_CANONICAL_HOST
if test x$with_backend = x; then
	AS_CASE([$host],
		[*-linux*],   [with_backend=linux],
		[*-*freebsd*], [with_backend=freebsd],
		[*-openbsd*], [with_backend=openbsd],
		              [with_backend=dummy])
fi
AC_DEFINE_UNQUOTED(BACKEND, "$with_backend", [backend])
AC_SUBST(BACKEND, "$with_backend")

# only need GUdev on linux
have_idevice=no
if test x$with_backend = xlinux; then
	PKG_CHECK_MODULES(GUDEV, [gudev-1.0 >= 147])
	PKG_CHECK_MODULES(USB, [libusb-1.0 >= 1.0.0])
	AC_ARG_WITH(idevice, AS_HELP_STRING([--without-idevice],[Build without libimobiledevice]),
		    with_idevice=$withval,with_idevice=yes)
	AS_IF([test "x$with_idevice" != "xno"],
	      [PKG_CHECK_MODULES(IDEVICE, libimobiledevice-1.0 >= 0.9.7 libplist >= 0.12, have_idevice=yes, have_idevice=no)],
	      [have_idevice=no])
	AS_IF([test "x$have_idevice" = "xyes"],
	      [AC_DEFINE(HAVE_IDEVICE, 1, [Define to 1 if AFC is going to be built])])
fi

AM_CONDITIONAL(HAVE_IDEVICE, [test x$have_idevice = xyes])

# export to Makefile.am
AM_CONDITIONAL(BACKEND_TYPE_DUMMY, [test x$with_backend = xdummy])
AM_CONDITIONAL(BACKEND_TYPE_LINUX, [test x$with_backend = xlinux])
AM_CONDITIONAL(BACKEND_TYPE_FREEBSD, [test x$with_backend = xfreebsd])
AM_CONDITIONAL(BACKEND_TYPE_OPENBSD, [test x$with_backend = xopenbsd])

dnl ---------------------------------------------------------------------------
dnl - Build self tests
dnl ---------------------------------------------------------------------------
AC_ARG_ENABLE(tests, AS_HELP_STRING([--enable-tests],[enable unit test code]),
	      enable_tests=$enableval,enable_tests=yes)
AM_CONDITIONAL(UP_BUILD_TESTS, test x$enable_tests = xyes)

if test "x$GCC" = "xyes"; then
  LDFLAGS="-Wl,--as-needed $LDFLAGS"
fi

# i18n support
IT_PROG_INTLTOOL([0.40.0])

AM_GNU_GETTEXT_VERSION([0.17])
AM_GNU_GETTEXT([external])

AC_SUBST([GETTEXT_PACKAGE],[${PACKAGE_TARNAME}])
AC_DEFINE([GETTEXT_PACKAGE],[PACKAGE_TARNAME],[Define to the gettext package name])


AC_CONFIG_FILES([
Makefile
upower-glib.pc
etc/Makefile
src/Makefile
src/dummy/Makefile
src/freebsd/Makefile
src/openbsd/Makefile
src/linux/Makefile
tools/Makefile
doc/Makefile
doc/version.xml
doc/man/Makefile
doc/dbus/Makefile
rules/Makefile
libupower-glib/Makefile
libupower-glib/up-version.h
po/Makefile.in
])
AC_OUTPUT

dnl ==========================================================================
echo "
                 UPower $VERSION
             =========================

        prefix:                     ${prefix}
        libdir:                     ${libdir}
        libexecdir:                 ${libexecdir}
        bindir:                     ${bindir}
        sbindir:                    ${sbindir}
        datadir:                    ${datadir}
        sysconfdir:                 ${sysconfdir}
        localstatedir:              ${localstatedir}
        docdir:                     ${docdir}

        compiler:                   ${CC}
        cflags:                     ${CFLAGS}
        cppflags:                   ${CPPFLAGS}
        xsltproc:                   ${XSLTPROC}

        Enable deprecated stuff:    ${enable_deprecated}
        Backend:                    ${with_backend}
        libimobiledevice support:   ${have_idevice}
        Maintainer mode:            ${USE_MAINTAINER_MODE}
        Building api docs:          ${enable_gtk_doc}
        Building man pages:         ${enable_man_pages}
        Building unit tests:        ${enable_tests}
"
