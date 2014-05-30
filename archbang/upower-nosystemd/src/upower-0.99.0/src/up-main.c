/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 David Zeuthen <davidz@redhat.com>
 * Copyright (C) 2008-2009 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

#include <glib.h>
#include <glib-unix.h>
#include <glib/gi18n-lib.h>
#include <glib-object.h>
#include <locale.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "up-daemon.h"
#include "up-kbd-backlight.h"
#include "up-wakeups.h"

#define DEVKIT_POWER_SERVICE_NAME "org.freedesktop.UPower"
static GMainLoop *loop = NULL;

/**
 * up_main_acquire_name_on_proxy:
 **/
static gboolean
up_main_acquire_name_on_proxy (DBusGProxy *bus_proxy, const gchar *name)
{
	GError *error = NULL;
	guint result;
	gboolean ret = FALSE;

	if (bus_proxy == NULL)
		goto out;

	ret = dbus_g_proxy_call (bus_proxy, "RequestName", &error,
				 G_TYPE_STRING, name,
				 G_TYPE_UINT, 0,
				 G_TYPE_INVALID,
				 G_TYPE_UINT, &result,
				 G_TYPE_INVALID);
	if (!ret) {
		if (error != NULL) {
			g_warning ("Failed to acquire %s: %s", name, error->message);
			g_error_free (error);
		} else {
			g_warning ("Failed to acquire %s", name);
		}
		goto out;
	}

	/* already taken */
 	if (result != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
		if (error != NULL) {
			g_warning ("Failed to acquire %s: %s", name, error->message);
			g_error_free (error);
		} else {
			g_warning ("Failed to acquire %s", name);
		}
		ret = FALSE;
		goto out;
	}
out:
	return ret;
}

/**
 * up_main_sigint_cb:
 **/
static gboolean
up_main_sigint_cb (gpointer user_data)
{
	g_debug ("Handling SIGINT");
	g_main_loop_quit (loop);
	return FALSE;
}

/**
 * up_main_timed_exit_cb:
 *
 * Exits the main loop, which is helpful for valgrinding.
 **/
static gboolean
up_main_timed_exit_cb (GMainLoop *loop)
{
	g_main_loop_quit (loop);
	return FALSE;
}

/**
 * up_main_log_ignore_cb:
 **/
static void
up_main_log_ignore_cb (const gchar *log_domain, GLogLevelFlags log_level,
		       const gchar *message, gpointer user_data)
{
}

/**
 * up_main_log_handler_cb:
 **/
static void
up_main_log_handler_cb (const gchar *log_domain, GLogLevelFlags log_level,
			const gchar *message, gpointer user_data)
{
	gchar str_time[255];
	time_t the_time;

	/* header always in green */
	time (&the_time);
	strftime (str_time, 254, "%H:%M:%S", localtime (&the_time));
	g_print ("%c[%dmTI:%s\t", 0x1B, 32, str_time);

	/* critical is also in red */
	if (log_level == G_LOG_LEVEL_CRITICAL ||
	    log_level == G_LOG_LEVEL_WARNING ||
	    log_level == G_LOG_LEVEL_ERROR) {
		g_print ("%c[%dm%s\n%c[%dm", 0x1B, 31, message, 0x1B, 0);
	} else {
		/* debug in blue */
		g_print ("%c[%dm%s\n%c[%dm", 0x1B, 34, message, 0x1B, 0);
	}
}

/**
 * main:
 **/
gint
main (gint argc, gchar **argv)
{
	GError *error = NULL;
	UpDaemon *daemon = NULL;
	UpKbdBacklight *kbd_backlight = NULL;
	UpWakeups *wakeups = NULL;
	GOptionContext *context;
	DBusGProxy *bus_proxy;
	DBusGConnection *bus;
	gboolean ret;
	gint retval = 1;
	gboolean timed_exit = FALSE;
	gboolean immediate_exit = FALSE;
	guint timer_id = 0;
	gboolean verbose = FALSE;

	const GOptionEntry options[] = {
		{ "timed-exit", '\0', 0, G_OPTION_ARG_NONE, &timed_exit,
		  /* TRANSLATORS: exit after we've started up, used for user profiling */
		  _("Exit after a small delay"), NULL },
		{ "immediate-exit", '\0', 0, G_OPTION_ARG_NONE, &immediate_exit,
		  /* TRANSLATORS: exit straight away, used for automatic profiling */
		  _("Exit after the engine has loaded"), NULL },
		{ "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose,
		  _("Show extra debugging information"), NULL },
		{ NULL}
	};

#if !defined(GLIB_VERSION_2_36)
	g_type_init ();
#endif
	setlocale(LC_ALL, "");

	context = g_option_context_new ("upower daemon");
	g_option_context_add_main_entries (context, options, NULL);
	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		g_warning ("Failed to parse command-line options: %s", error->message);
		g_error_free (error);
		return 1;
	}
	g_option_context_free (context);

	/* verbose? */
	if (verbose) {
		g_log_set_fatal_mask (NULL, G_LOG_LEVEL_ERROR |
					    G_LOG_LEVEL_CRITICAL);
		g_log_set_handler (G_LOG_DOMAIN,
				   G_LOG_LEVEL_ERROR |
				   G_LOG_LEVEL_CRITICAL |
				   G_LOG_LEVEL_DEBUG |
				   G_LOG_LEVEL_WARNING,
				   up_main_log_handler_cb, NULL);
		g_log_set_handler ("UPower-Linux",
				   G_LOG_LEVEL_ERROR |
				   G_LOG_LEVEL_CRITICAL |
				   G_LOG_LEVEL_DEBUG |
				   G_LOG_LEVEL_WARNING,
				   up_main_log_handler_cb, NULL);
	} else {
		/* hide all debugging */
		g_log_set_fatal_mask (NULL, G_LOG_LEVEL_ERROR);
		g_log_set_handler (G_LOG_DOMAIN,
				   G_LOG_LEVEL_DEBUG,
				   up_main_log_ignore_cb,
				   NULL);
		g_log_set_handler ("UPower-Linux",
				   G_LOG_LEVEL_DEBUG,
				   up_main_log_ignore_cb,
				   NULL);
	}

	/* get bus connection */
	bus = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
	if (bus == NULL) {
		g_warning ("Couldn't connect to system bus: %s", error->message);
		g_error_free (error);
		goto out;
	}

	/* get proxy */
	bus_proxy = dbus_g_proxy_new_for_name (bus, DBUS_SERVICE_DBUS,
					       DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS);
	if (bus_proxy == NULL) {
		g_warning ("Could not construct bus_proxy object; bailing out");
		goto out;
	}

	/* aquire name */
	ret = up_main_acquire_name_on_proxy (bus_proxy, DEVKIT_POWER_SERVICE_NAME);
	if (!ret) {
		g_warning ("Could not acquire name; bailing out");
		goto out;
	}

	/* do stuff on ctrl-c */
	g_unix_signal_add_full (G_PRIORITY_DEFAULT,
				SIGINT,
				up_main_sigint_cb,
				loop,
				NULL);

	g_debug ("Starting upowerd version %s", PACKAGE_VERSION);

	kbd_backlight = up_kbd_backlight_new ();
	wakeups = up_wakeups_new ();
	daemon = up_daemon_new ();
	loop = g_main_loop_new (NULL, FALSE);
	ret = up_daemon_startup (daemon);
	if (!ret) {
		g_warning ("Could not startup; bailing out");
		goto out;
	}

	/* only timeout and close the mainloop if we have specified it on the command line */
	if (timed_exit) {
		timer_id = g_timeout_add_seconds (30, (GSourceFunc) up_main_timed_exit_cb, loop);
		g_source_set_name_by_id (timer_id, "[upower] up_main_timed_exit_cb");
	}

	/* immediatly exit */
	if (immediate_exit) {
		g_timeout_add (50, (GSourceFunc) up_main_timed_exit_cb, loop);
		g_source_set_name_by_id (timer_id, "[upower] up_main_timed_exit_cb");
	}

	/* wait for input or timeout */
	g_main_loop_run (loop);
	retval = 0;
out:
	if (kbd_backlight != NULL)
		g_object_unref (kbd_backlight);
	if (wakeups != NULL)
		g_object_unref (wakeups);
	if (daemon != NULL)
		g_object_unref (daemon);
	if (loop != NULL)
		g_main_loop_unref (loop);
	return retval;
}

