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
#include <gudev/gudev.h>

#include "up-native.h"

/**
 * up_native_get_native_path:
 * @object: the native tracking object
 *
 * This converts a GObject used as the device data into a native path.
 * This would be implemented on a Linux system using:
 *  g_udev_device_get_sysfs_path (G_UDEV_DEVICE (object))
 *
 * Return value: Device name for devices of subsystem "power_supply", otherwise
 * the native path for the device which is unique.
 **/
const gchar *
up_native_get_native_path (GObject *object)
{
	/* Device names within the same subsystem must be unique. To avoid
	 * treating the same power supply device on variable buses as different
	 * only because e. g. the USB or bluetooth tree layout changed, only
	 * use their name as identification. Also see
	 * http://bugzilla.kernel.org/show_bug.cgi?id=62041 */
	if (g_strcmp0 (g_udev_device_get_subsystem (G_UDEV_DEVICE (object)), "power_supply") == 0)
		return g_udev_device_get_name (G_UDEV_DEVICE (object));

	/* we do not expect other devices than power_supply, but provide this
	 * fallback for completeness */
	return g_udev_device_get_sysfs_path (G_UDEV_DEVICE (object));
}

