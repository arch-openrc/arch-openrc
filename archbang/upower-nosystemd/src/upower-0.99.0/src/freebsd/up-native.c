/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2009 Richard Hughes <richard@hughsie.com>
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

#include <glib.h>
#include "up-acpi-native.h"

#include "up-native.h"

/**
 * up_native_get_native_path:
 * @object: the native tracking object
 *
 * This converts a GObject used as the device data into a native path.
 *
 * Return value: The native path for the device which is unique, e.g. "/sys/class/power/BAT1"
 **/
const gchar *
up_native_get_native_path (GObject *object)
{
	return up_acpi_native_get_path (UP_ACPI_NATIVE (object));
}

/***************************************************************************
 ***                          MAKE CHECK TESTS                           ***
 ***************************************************************************/
#ifdef EGG_TEST
#include "egg-test.h"

void
up_native_test (gpointer user_data)
{
	EggTest *test = (EggTest *) user_data;
	UpAcpiNative *dan;
	const gchar *path;

	if (!egg_test_start (test, "UpNative"))
		return;

	/************************************************************/
	egg_test_title (test, "get instance");
	dan = up_acpi_native_new_driver_unit ("battery", 0);
	path = up_native_get_native_path (dan);
	egg_test_assert (test, (g_strcmp0 (path, "dev.battery.0") == 0));
	g_object_unref (dan);

	egg_test_end (test);
}
#endif

