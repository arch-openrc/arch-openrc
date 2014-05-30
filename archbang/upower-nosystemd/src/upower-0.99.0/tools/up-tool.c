/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 David Zeuthen <davidz@redhat.com>
 * Copyright (C) 2008 Richard Hughes <richard@hughsie.com>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <locale.h>

#include "up-client.h"
#include "up-device.h"
#include "up-wakeups.h"

static GMainLoop *loop;
static gboolean opt_monitor_detail = FALSE;

/**
 * up_tool_get_timestamp:
 **/
static gchar *
up_tool_get_timestamp (void)
{
	gchar *str_time;
	gchar *timestamp;
	time_t the_time;
	struct timeval time_val;

	time (&the_time);
	gettimeofday (&time_val, NULL);
	str_time = g_new0 (gchar, 255);
	strftime (str_time, 254, "%H:%M:%S", localtime (&the_time));

	/* generate header text */
	timestamp = g_strdup_printf ("%s.%03i", str_time, (gint) time_val.tv_usec / 1000);
	g_free (str_time);
	return timestamp;
}

/**
 * up_tool_device_added_cb:
 **/
static void
up_tool_device_added_cb (UpClient *client, UpDevice *device, gpointer user_data)
{
	gchar *timestamp;
	gchar *text = NULL;
	timestamp = up_tool_get_timestamp ();
	g_print ("[%s]\tdevice added:     %s\n", timestamp, up_device_get_object_path (device));
	if (opt_monitor_detail) {
		text = up_device_to_text (device);
		g_print ("%s\n", text);
	}
	g_free (timestamp);
	g_free (text);
}

/**
 * up_tool_device_changed_cb:
 **/
static void
up_tool_device_changed_cb (UpDevice *device, GParamSpec *pspec, gpointer user_data)
{
	gchar *timestamp;
	gchar *text = NULL;
	timestamp = up_tool_get_timestamp ();
	g_print ("[%s]\tdevice changed:     %s\n", timestamp, up_device_get_object_path (device));
	if (opt_monitor_detail) {
		/* TODO: would be nice to just show the diff */
		text = up_device_to_text (device);
		g_print ("%s\n", text);
	}
	g_free (timestamp);
	g_free (text);
}

/**
 * up_tool_device_removed_cb:
 **/
static void
up_tool_device_removed_cb (UpClient *client, const char *object_path, gpointer user_data)
{
	gchar *timestamp;
	timestamp = up_tool_get_timestamp ();
	g_print ("[%s]\tdevice removed:   %s\n", timestamp, object_path);
	if (opt_monitor_detail)
		g_print ("\n");
	g_free (timestamp);
}

/**
 * up_client_print:
 **/
static void
up_client_print (UpClient *client)
{
	gchar *daemon_version;
	gboolean on_battery;
	UpDeviceLevel warning_level;
	gboolean lid_is_closed;
	gboolean lid_is_present;
	gboolean is_docked;
	char *action;

	g_object_get (client,
		      "daemon-version", &daemon_version,
		      "on-battery", &on_battery,
		      "lid-is-closed", &lid_is_closed,
		      "lid-is-present", &lid_is_present,
		      "is-docked", &is_docked,
		      NULL);

	g_print ("  daemon-version:  %s\n", daemon_version);
	g_print ("  on-battery:      %s\n", on_battery ? "yes" : "no");
	g_print ("  lid-is-closed:   %s\n", lid_is_closed ? "yes" : "no");
	g_print ("  lid-is-present:  %s\n", lid_is_present ? "yes" : "no");
	g_print ("  is-docked:       %s\n", is_docked ? "yes" : "no");
	action = up_client_get_critical_action (client);
	g_print ("  critical-action: %s\n", action);
	g_free (action);

	g_free (daemon_version);
}

/**
 * up_tool_changed_cb:
 **/
static void
up_tool_changed_cb (UpClient *client, GParamSpec *pspec, gpointer user_data)
{
	gchar *timestamp;
	timestamp = up_tool_get_timestamp ();
	g_print ("[%s]\tdaemon changed:\n", timestamp);
	if (opt_monitor_detail) {
		up_client_print (client);
		g_print ("\n");
	}
	g_free (timestamp);
}

/**
 * up_tool_do_monitor:
 **/
static gboolean
up_tool_do_monitor (UpClient *client)
{
	GPtrArray *devices;
	GError *error = NULL;
	guint i;

	g_print ("Monitoring activity from the power daemon. Press Ctrl+C to cancel.\n");

	g_signal_connect (client, "device-added", G_CALLBACK (up_tool_device_added_cb), NULL);
	g_signal_connect (client, "device-removed", G_CALLBACK (up_tool_device_removed_cb), NULL);
	g_signal_connect (client, "notify", G_CALLBACK (up_tool_changed_cb), NULL);

	devices = up_client_get_devices (client);
	for (i=0; i < devices->len; i++) {
		UpDevice *device;
		device = g_ptr_array_index (devices, i);
		g_signal_connect (device, "notify", G_CALLBACK (up_tool_device_changed_cb), NULL);
	}

	g_main_loop_run (loop);

	return FALSE;
}

/**
 * up_tool_print_wakeup_item:
 **/
static void
up_tool_print_wakeup_item (UpWakeupItem *item)
{
	g_print ("userspace:%i id:%i, interrupts:%.1f, cmdline:%s, details:%s\n",
		 up_wakeup_item_get_is_userspace (item),
		 up_wakeup_item_get_id (item),
		 up_wakeup_item_get_value (item),
		 up_wakeup_item_get_cmdline (item),
		 up_wakeup_item_get_details (item));
}

/**
 * up_tool_show_wakeups:
 **/
static gboolean
up_tool_show_wakeups (void)
{
	guint i;
	gboolean ret;
	UpWakeups *wakeups;
	UpWakeupItem *item;
	guint total;
	GPtrArray *array;

	/* create new object */
	wakeups = up_wakeups_new ();

	/* do we have support? */
	ret = up_wakeups_get_has_capability (wakeups);
	if (!ret) {
		g_print ("No wakeup capability\n");
		goto out;
	}

	/* get total */
	total = up_wakeups_get_total_sync (wakeups, NULL, NULL);
	g_print ("Total wakeups per minute: %i\n", total);

	/* get data */
	array = up_wakeups_get_data_sync (wakeups, NULL, NULL);
	if (array == NULL)
		goto out;
	g_print ("Wakeup sources:\n");
	for (i=0; i<array->len; i++) {
		item = g_ptr_array_index (array, i);
		up_tool_print_wakeup_item (item);
	}
	g_ptr_array_unref (array);
out:
	g_object_unref (wakeups);
	return ret;
}

/**
 * main:
 **/
int
main (int argc, char **argv)
{
	gint retval = EXIT_FAILURE;
	guint i;
	GOptionContext *context;
	gboolean opt_dump = FALSE;
	gboolean opt_wakeups = FALSE;
	gboolean opt_enumerate = FALSE;
	gboolean opt_monitor = FALSE;
	gchar *opt_show_info = FALSE;
	gboolean opt_version = FALSE;
	gboolean ret;
	GError *error = NULL;
	gchar *text = NULL;

	UpClient *client;
	UpDevice *device;

	const GOptionEntry entries[] = {
		{ "enumerate", 'e', 0, G_OPTION_ARG_NONE, &opt_enumerate, _("Enumerate objects paths for devices"), NULL },
		{ "dump", 'd', 0, G_OPTION_ARG_NONE, &opt_dump, _("Dump all parameters for all objects"), NULL },
		{ "wakeups", 'w', 0, G_OPTION_ARG_NONE, &opt_wakeups, _("Get the wakeup data"), NULL },
		{ "monitor", 'm', 0, G_OPTION_ARG_NONE, &opt_monitor, _("Monitor activity from the power daemon"), NULL },
		{ "monitor-detail", 0, 0, G_OPTION_ARG_NONE, &opt_monitor_detail, _("Monitor with detail"), NULL },
		{ "show-info", 'i', 0, G_OPTION_ARG_STRING, &opt_show_info, _("Show information about object path"), NULL },
		{ "version", 'v', 0, G_OPTION_ARG_NONE, &opt_version, "Print version of client and daemon", NULL },
		{ NULL }
	};

#if !defined(GLIB_VERSION_2_36)
	g_type_init ();
#endif
	setlocale(LC_ALL, "");

	context = g_option_context_new ("UPower tool");
	g_option_context_add_main_entries (context, entries, NULL);
	g_option_context_parse (context, &argc, &argv, NULL);
	g_option_context_free (context);

	loop = g_main_loop_new (NULL, FALSE);
	client = up_client_new ();

	if (opt_version) {
		gchar *daemon_version;
		g_object_get (client,
			      "daemon-version", &daemon_version,
			      NULL);
		g_print ("UPower client version %s\n"
			 "UPower daemon version %s\n",
			 PACKAGE_VERSION, daemon_version);
		g_free (daemon_version);
		retval = 0;
		goto out;
	}

	/* wakeups */
	if (opt_wakeups) {
		up_tool_show_wakeups ();
		retval = EXIT_SUCCESS;
		goto out;
	}

	if (opt_enumerate || opt_dump) {
		GPtrArray *devices;
		devices = up_client_get_devices (client);
		for (i=0; i < devices->len; i++) {
			device = (UpDevice*) g_ptr_array_index (devices, i);
			if (opt_enumerate) {
				g_print ("%s\n", up_device_get_object_path (device));
			} else {
				g_print ("Device: %s\n", up_device_get_object_path (device));
				text = up_device_to_text (device);
				g_print ("%s\n", text);
				g_free (text);
			}
		}
		g_ptr_array_unref (devices);
		device = up_client_get_display_device (client);
		if (opt_enumerate) {
			g_print ("%s\n", up_device_get_object_path (device));
		} else {
			g_print ("Device: %s\n", up_device_get_object_path (device));
			text = up_device_to_text (device);
			g_print ("%s\n", text);
			g_free (text);
		}
		g_object_unref (device);
		if (opt_dump) {
			g_print ("Daemon:\n");
			up_client_print (client);
		}
		retval = EXIT_SUCCESS;
		goto out;
	}

	if (opt_monitor || opt_monitor_detail) {
		if (!up_tool_do_monitor (client))
			goto out;
		retval = EXIT_SUCCESS;
		goto out;
	}

	if (opt_show_info != NULL) {
		device = up_device_new ();
		ret = up_device_set_object_path_sync (device, opt_show_info, NULL, &error);
		if (!ret) {
			g_print ("failed to set path: %s\n", error->message);
			g_error_free (error);
		} else {
			text = up_device_to_text (device);
			g_print ("%s\n", text);
			g_free (text);
		}
		g_object_unref (device);
		retval = EXIT_SUCCESS;
		goto out;
	}
out:
	g_object_unref (client);
	return retval;
}
