/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2012 Richard Hughes <richard@hughsie.com>
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

#include <glib.h>
#include <glib-object.h>

#include "hidpp-device.h"

int
main (int argc, char **argv)
{
//	const gchar *model;
//	guint batt_percentage;
//	guint version;
//	HidppDeviceBattStatus batt_status;
	HidppDevice *d;
//	HidppDeviceKind kind;
	gboolean ret;
	GError *error = NULL;

#if !defined(GLIB_VERSION_2_36)
	g_type_init ();
#endif
	g_test_init (&argc, &argv, NULL);

	d = hidpp_device_new ();
	hidpp_device_set_enable_debug (d, TRUE);
	g_assert_cmpint (hidpp_device_get_version (d), ==, 0);
	g_assert_cmpstr (hidpp_device_get_model (d), ==, NULL);
	g_assert_cmpint (hidpp_device_get_batt_percentage (d), ==, 0);
	g_assert_cmpint (hidpp_device_get_batt_status (d), ==, HIDPP_DEVICE_BATT_STATUS_UNKNOWN);
	g_assert_cmpint (hidpp_device_get_kind (d), ==, HIDPP_DEVICE_KIND_UNKNOWN);

	/* setup */
	hidpp_device_set_hidraw_device (d, "/dev/hidraw0");
	hidpp_device_set_index (d, 1);
	ret = hidpp_device_refresh (d,
				    HIDPP_REFRESH_FLAGS_VERSION |
				    HIDPP_REFRESH_FLAGS_KIND |
				    HIDPP_REFRESH_FLAGS_BATTERY |
				    HIDPP_REFRESH_FLAGS_MODEL,
				    &error);
	g_assert_no_error (error);
	g_assert (ret);

	g_assert_cmpint (hidpp_device_get_version (d), !=, 0);
	g_assert_cmpstr (hidpp_device_get_model (d), !=, NULL);
	g_assert_cmpint (hidpp_device_get_batt_percentage (d), !=, 0);
	g_assert_cmpint (hidpp_device_get_batt_status (d), !=, HIDPP_DEVICE_BATT_STATUS_UNKNOWN);
	g_assert_cmpint (hidpp_device_get_kind (d), !=, HIDPP_DEVICE_KIND_UNKNOWN);

	g_print ("Version:\t\t%i\n", hidpp_device_get_version (d));
	g_print ("Kind:\t\t\t%i\n", hidpp_device_get_kind (d));
	g_print ("Model:\t\t\t%s\n", hidpp_device_get_model (d));
	g_print ("Battery Percentage:\t%i\n", hidpp_device_get_batt_percentage (d));
	g_print ("Battery Status:\t\t%i\n", hidpp_device_get_batt_status (d));

	g_object_unref (d);
	return 0;
}
