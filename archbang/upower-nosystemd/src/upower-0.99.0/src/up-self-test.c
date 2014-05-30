/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007-2009 Richard Hughes <richard@hughsie.com>
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

#include "config.h"

#include <glib-object.h>
#include <glib/gstdio.h>
#include <up-history-item.h>
#include <stdlib.h>
#include <errno.h>
#include "up-backend.h"
#include "up-daemon.h"
#include "up-device.h"
#include "up-device-list.h"
#include "up-history.h"
#include "up-native.h"
#include "up-polkit.h"
#include "up-wakeups.h"

gchar *history_dir = NULL;

#define DBUS_SYSTEM_SOCKET "/var/run/dbus/system_bus_socket"

static void
up_test_native_func (void)
{
	const gchar *path;

	path = up_native_get_native_path (NULL);
	g_assert_cmpstr (path, ==, "/sys/dummy");
}

static void
up_test_backend_func (void)
{
	UpBackend *backend;

	backend = up_backend_new ();
	g_assert (backend != NULL);

	/* unref */
	g_object_unref (backend);
}

static void
up_test_daemon_func (void)
{
	UpDaemon *daemon;

	/* needs polkit, which only listens to the system bus */
	if (!g_file_test (DBUS_SYSTEM_SOCKET, G_FILE_TEST_EXISTS)) {
		puts("No system D-BUS running, skipping test");
		return;
	}

	daemon = up_daemon_new ();
	g_assert (daemon != NULL);

	/* unref */
	g_object_unref (daemon);
}

static void
up_test_device_func (void)
{
	UpDevice *device;

	device = up_device_new ();
	g_assert (device != NULL);

	/* unref */
	g_object_unref (device);
}

static void
up_test_device_list_func (void)
{
	UpDeviceList *list;
	GObject *native;
	GObject *device;
	GObject *found;
	gboolean ret;

	list = up_device_list_new ();
	g_assert (list != NULL);

	/* add device */
	native = g_object_new (G_TYPE_OBJECT, NULL);
	device = g_object_new (G_TYPE_OBJECT, NULL);
	ret = up_device_list_insert (list, native, device);
	g_assert (ret);

	/* find device */
	found = up_device_list_lookup (list, native);
	g_assert (found != NULL);
	g_object_unref (found);

	/* remove device */
	ret = up_device_list_remove (list, device);
	g_assert (ret);

	/* unref */
	g_object_unref (native);
	g_object_unref (device);
	g_object_unref (list);
}

static void
up_test_history_remove_temp_files (void)
{
	gchar *filename;
	filename = g_build_filename (history_dir, "history-time-full-test.dat", NULL);
	g_unlink (filename);
	g_free (filename);
	filename = g_build_filename (history_dir, "history-time-empty-test.dat", NULL);
	g_unlink (filename);
	g_free (filename);
	filename = g_build_filename (history_dir, "history-charge-test.dat", NULL);
	g_unlink (filename);
	g_free (filename);
	filename = g_build_filename (history_dir, "history-rate-test.dat", NULL);
	g_unlink (filename);
	g_free (filename);
}

static void
up_test_history_func (void)
{
	UpHistory *history;
	gboolean ret;
	GPtrArray *array;
	gchar *filename;
	UpHistoryItem *item, *item2, *item3;

	history = up_history_new ();
	g_assert (history != NULL);

	/* set a temporary directory for the history */
	history_dir = g_build_filename (g_get_tmp_dir(), "upower-test.XXXXXX", NULL);
	if (mkdtemp (history_dir) == NULL)
		g_error ("Cannot create temporary directory: %s", g_strerror(errno));
	up_history_set_directory (history, history_dir);

	/* remove previous test files */
	up_test_history_remove_temp_files ();

	/* setup fresh environment */
	ret = up_history_set_id (history, "test");
	g_assert (ret);

	/* get nonexistant data */
	array = up_history_get_data (history, UP_HISTORY_TYPE_CHARGE, 10, 100);
	g_assert (array != NULL);
	g_assert_cmpint (array->len, ==, 0);
	g_ptr_array_unref (array);

	/* setup some fake device and three data points */
	up_history_set_state (history, UP_DEVICE_STATE_CHARGING);
	up_history_set_charge_data (history, 85);
	up_history_set_rate_data (history, 0.99f);
	up_history_set_time_empty_data (history, 12346);
	up_history_set_time_full_data (history, 54322);

	g_usleep (2 * G_USEC_PER_SEC);
	up_history_set_charge_data (history, 90);
	up_history_set_rate_data (history, 1.00f);
	up_history_set_time_empty_data (history, 12345);
	up_history_set_time_full_data (history, 54321);

	g_usleep (2 * G_USEC_PER_SEC);
	up_history_set_charge_data (history, 95);
	up_history_set_rate_data (history, 1.01f);
	up_history_set_time_empty_data (history, 12344);
	up_history_set_time_full_data (history, 54320);

	/* get data for last 10 seconds */
	array = up_history_get_data (history, UP_HISTORY_TYPE_CHARGE, 10, 100);
	g_assert (array != NULL);
	g_assert_cmpint (array->len, ==, 3);

	/* get the first item, which should be the most recent */
	item = g_ptr_array_index (array, 0);
	g_assert (item != NULL);
	g_assert_cmpint (up_history_item_get_value (item), ==, 95);
	g_assert_cmpint (up_history_item_get_time (item), >, 1000000);

        /* the second one ought to be older */
	item2 = g_ptr_array_index (array, 1);
	g_assert (item2 != NULL);
	g_assert_cmpint (up_history_item_get_value (item2), ==, 90);
	g_assert_cmpint (up_history_item_get_time (item2), <, up_history_item_get_time (item));

        /* third one is the oldest */
	item3 = g_ptr_array_index (array, 2);
	g_assert (item3 != NULL);
	g_assert_cmpint (up_history_item_get_value (item3), ==, 85);
	g_assert_cmpint (up_history_item_get_time (item3), <, up_history_item_get_time (item2));

	g_ptr_array_unref (array);

        /* request fewer items than we have in our history; should have the
         * same order: first one is the most recent, and the data gets
         * interpolated */
	array = up_history_get_data (history, UP_HISTORY_TYPE_CHARGE, 10, 2);
	g_assert (array != NULL);
	g_assert_cmpint (array->len, ==, 2);

	item = g_ptr_array_index (array, 0);
	g_assert (item != NULL);
	item2 = g_ptr_array_index (array, 1);
	g_assert (item2 != NULL);

	g_assert_cmpint (up_history_item_get_time (item), >, 1000000);
	g_assert_cmpint (up_history_item_get_value (item), ==, 95);
	g_assert_cmpint (up_history_item_get_value (item2), ==, 87);

	g_ptr_array_unref (array);

	/* force a save to disk */
	ret = up_history_save_data (history);
	g_assert (ret);
	g_object_unref (history);

	/* ensure the file was created */
	filename = g_build_filename (history_dir, "history-charge-test.dat", NULL);
	g_assert (g_file_test (filename, G_FILE_TEST_EXISTS));
	g_free (filename);

	/* ensure we can load from disk */
	history = up_history_new ();
	up_history_set_directory (history, history_dir);
	up_history_set_id (history, "test");

	/* get data for last 10 seconds */
	array = up_history_get_data (history, UP_HISTORY_TYPE_CHARGE, 10, 100);
	g_assert (array != NULL);
	g_assert_cmpint (array->len, ==, 4); /* we have inserted an unknown as the first entry */
	item = g_ptr_array_index (array, 1);
	g_assert (item != NULL);
	g_assert_cmpint (up_history_item_get_value (item), ==, 95);
	g_assert_cmpint (up_history_item_get_time (item), >, 1000000);
	g_ptr_array_unref (array);

	/* ensure old entries are purged */
	up_history_set_max_data_age (history, 2);
	g_usleep (1100 * G_USEC_PER_SEC / 1000);
	g_object_unref (history);

	/* ensure only 2 points are returned */
	history = up_history_new ();
	up_history_set_directory (history, history_dir);
	up_history_set_id (history, "test");
	array = up_history_get_data (history, UP_HISTORY_TYPE_CHARGE, 10, 100);
	g_assert (array != NULL);
	g_assert_cmpint (array->len, ==, 2);
	g_ptr_array_unref (array);

	/* unref */
	g_object_unref (history);

	/* remove these test files */
	up_test_history_remove_temp_files ();
	rmdir (history_dir);
}

static void
up_test_polkit_func (void)
{
	UpPolkit *polkit;

	/* polkit only listens to the system bus */
	if (!g_file_test (DBUS_SYSTEM_SOCKET, G_FILE_TEST_EXISTS)) {
		puts("No system D-BUS running, skipping test");
		return;
	}

	polkit = up_polkit_new ();
	g_assert (polkit != NULL);

	/* unref */
	g_object_unref (polkit);
}

static void
up_test_wakeups_func (void)
{
	UpWakeups *wakeups;

	wakeups = up_wakeups_new ();
	g_assert (wakeups != NULL);

	/* unref */
	g_object_unref (wakeups);
}

int
main (int argc, char **argv)
{
#if !defined(GLIB_VERSION_2_36)
	g_type_init ();
#endif
	g_test_init (&argc, &argv, NULL);

	/* make check, vs. make distcheck */
	if (g_file_test ("../etc/UPower.conf", G_FILE_TEST_EXISTS))
		g_setenv ("UPOWER_CONF_FILE_NAME", "../etc/UPower.conf", TRUE);
	else
		g_setenv ("UPOWER_CONF_FILE_NAME", "../../etc/UPower.conf", TRUE);

	/* tests go here */
	g_test_add_func ("/power/backend", up_test_backend_func);
	g_test_add_func ("/power/device", up_test_device_func);
	g_test_add_func ("/power/device_list", up_test_device_list_func);
	g_test_add_func ("/power/history", up_test_history_func);
	g_test_add_func ("/power/native", up_test_native_func);
	g_test_add_func ("/power/polkit", up_test_polkit_func);
	g_test_add_func ("/power/wakeups", up_test_wakeups_func);
	g_test_add_func ("/power/daemon", up_test_daemon_func);

	return g_test_run ();
}

