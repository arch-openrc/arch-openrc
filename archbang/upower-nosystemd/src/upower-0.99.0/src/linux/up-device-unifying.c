/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2012 Julien Danjou <julien@danjou.info>
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

#include <glib-object.h>
#include <gudev/gudev.h>

#include "hidpp-device.h"

#include "up-device-unifying.h"
#include "up-types.h"

struct UpDeviceUnifyingPrivate
{
	HidppDevice		*hidpp_device;
};

G_DEFINE_TYPE (UpDeviceUnifying, up_device_unifying, UP_TYPE_DEVICE)
#define UP_DEVICE_UNIFYING_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), UP_TYPE_DEVICE_UNIFYING, UpDeviceUnifyingPrivate))

/**
 * up_device_unifying_refresh:
 *
 * Return %TRUE on success, %FALSE if we failed to refresh or no data
 **/
static gboolean
up_device_unifying_refresh (UpDevice *device)
{
	gboolean ret;
	GError *error = NULL;
	GTimeVal timeval;
	HidppRefreshFlags refresh_flags;
	UpDeviceState state = UP_DEVICE_STATE_UNKNOWN;
	UpDeviceUnifying *unifying = UP_DEVICE_UNIFYING (device);
	UpDeviceUnifyingPrivate *priv = unifying->priv;
	double lux;

	/* refresh the battery stats */
	refresh_flags = HIDPP_REFRESH_FLAGS_BATTERY;

	/*
	 * When a device is initially unreachable, the HID++ version cannot be
	 * determined.  Therefore try determining the HID++ version, otherwise
	 * battery information cannot be retrieved. Assume that the HID++
	 * version does not change once detected.
	 */
	if (hidpp_device_get_version (priv->hidpp_device) == 0)
		refresh_flags |= HIDPP_REFRESH_FLAGS_VERSION;

	ret = hidpp_device_refresh (priv->hidpp_device,
				    refresh_flags,
				    &error);
	if (!ret) {
		g_warning ("failed to coldplug unifying device: %s",
			   error->message);
		g_error_free (error);
		goto out;
	}
	switch (hidpp_device_get_batt_status (priv->hidpp_device)) {
	case HIDPP_DEVICE_BATT_STATUS_CHARGING:
		state = UP_DEVICE_STATE_CHARGING;
		break;
	case HIDPP_DEVICE_BATT_STATUS_DISCHARGING:
		state = UP_DEVICE_STATE_DISCHARGING;
		break;
	case HIDPP_DEVICE_BATT_STATUS_CHARGED:
		state = UP_DEVICE_STATE_FULLY_CHARGED;
		break;
	default:
		break;
	}

	/* if a device is unreachable, some known values do not make sense */
	if (!hidpp_device_is_reachable (priv->hidpp_device)) {
		state = UP_DEVICE_STATE_UNKNOWN;
	}

	g_get_current_time (&timeval);
	lux = hidpp_device_get_luminosity (priv->hidpp_device);
	if (lux >= 0) {
		g_object_set (device, "luminosity", lux, NULL);
	}

	g_object_set (device,
		      "is-present", hidpp_device_is_reachable (priv->hidpp_device),
		      "percentage", (gdouble) hidpp_device_get_batt_percentage (priv->hidpp_device),
		      "state", state,
		      "update-time", (guint64) timeval.tv_sec,
		      NULL);
out:
	return TRUE;
}

static UpDeviceKind
up_device_unifying_get_device_kind (UpDeviceUnifying *unifying)
{
	UpDeviceKind kind;
	switch (hidpp_device_get_kind (unifying->priv->hidpp_device)) {
	case HIDPP_DEVICE_KIND_MOUSE:
	case HIDPP_DEVICE_KIND_TOUCHPAD:
	case HIDPP_DEVICE_KIND_TRACKBALL:
		kind = UP_DEVICE_KIND_MOUSE;
		break;
	case HIDPP_DEVICE_KIND_KEYBOARD:
		kind = UP_DEVICE_KIND_KEYBOARD;
		break;
	case HIDPP_DEVICE_KIND_TABLET:
		kind = UP_DEVICE_KIND_TABLET;
		break;
	default:
		kind = UP_DEVICE_KIND_UNKNOWN;
	}
	return kind;
}

/**
 * up_device_unifying_coldplug:
 *
 * Return %TRUE on success, %FALSE if we failed to get data and should be removed
 **/
static gboolean
up_device_unifying_coldplug (UpDevice *device)
{
	const gchar *bus_address;
	const gchar *device_file;
	const gchar *type;
	const gchar *vendor;
	gboolean ret = FALSE;
	gchar *endptr = NULL;
	gchar *tmp;
	GError *error = NULL;
	GUdevDevice *native;
	GUdevDevice *parent = NULL;
	GUdevDevice *receiver = NULL;
	UpDeviceUnifying *unifying = UP_DEVICE_UNIFYING (device);
	GUdevClient *client = NULL;
	GList *hidraw_list = NULL;
	GList *l;

	native = G_UDEV_DEVICE (up_device_get_native (device));

	/* check if we have the right device */
	type = g_udev_device_get_property (native, "UPOWER_BATTERY_TYPE");
	if (type == NULL)
		goto out;
	if ((g_strcmp0 (type, "unifying") != 0) && (g_strcmp0 (type, "lg-wireless") != 0))
		goto out;

	/* get the device index */
	unifying->priv->hidpp_device = hidpp_device_new ();
	bus_address = g_udev_device_get_property (native, "HID_PHYS");
	tmp = g_strrstr (bus_address, ":");
	if (tmp == NULL) {
		g_debug ("Could not get physical device index");
		goto out;
	}

	if (g_strcmp0 (type, "lg-wireless") == 0)
		hidpp_device_set_index (unifying->priv->hidpp_device, 1);
	else {
		hidpp_device_set_index (unifying->priv->hidpp_device,
				g_ascii_strtoull (tmp + 1, &endptr, 10));
		if (endptr != NULL && endptr[0] != '\0') {
			g_debug ("HID_PHYS malformed: '%s'", bus_address);
			goto out;
		}
	}

	/* find the hidraw device that matches */
	parent = g_udev_device_get_parent (native);
	client = g_udev_client_new (NULL);
	hidraw_list = g_udev_client_query_by_subsystem (client, "hidraw");
	for (l = hidraw_list; l != NULL; l = l->next) {
		gboolean receiver_found = FALSE;

		if (g_strcmp0 (type, "lg-wireless") == 0) {
			const gchar *filename;
			GDir* dir;
			GUdevDevice *parent_dev;

			parent_dev = g_udev_device_get_parent(l->data);
			receiver_found = g_strcmp0 (g_udev_device_get_sysfs_path (native),
						g_udev_device_get_sysfs_path(parent_dev)) == 0;
			g_object_unref (parent_dev);

			if (!receiver_found)
				continue;

			/* hidraw device which exposes hiddev interface is our receiver */
			tmp = g_build_filename(g_udev_device_get_sysfs_path (g_udev_device_get_parent(native)),
					"usbmisc", NULL);
			dir = g_dir_open (tmp, 0, &error);
			g_free(tmp);
			if (error) {
				g_clear_error(&error);
				continue;
			}
			while ( (filename = g_dir_read_name(dir)) ) {
				if (g_ascii_strncasecmp(filename, "hiddev", 6) == 0) {
					receiver_found = TRUE;
					break;
				}
			}
			g_dir_close(dir);
		} else {
			GUdevDevice *parent_dev;

			/* Unifying devices are located under their receiver */
			parent_dev = g_udev_device_get_parent(l->data);
			receiver_found = g_strcmp0 (g_udev_device_get_sysfs_path (parent),
						g_udev_device_get_sysfs_path(parent_dev)) == 0;
			g_object_unref (parent_dev);
		}

		if (receiver_found) {
			receiver = g_object_ref (l->data);
			break;
		}
	}
	if (receiver == NULL) {
		g_debug ("Unable to find an hidraw device for Unifying receiver");
		return FALSE;
	}

	/* connect to the receiver */
	device_file = g_udev_device_get_device_file (receiver);
	if (device_file == NULL) {
		g_debug ("Could not get device file for Unifying receiver device");
		goto out;
	}
	g_debug ("Using Unifying receiver hidraw device file: %s", device_file);
	hidpp_device_set_hidraw_device (unifying->priv->hidpp_device,
					device_file);

	/* give newly paired devices a chance to complete pairing */
	g_usleep(30000);

	/* coldplug initial parameters */
	ret = hidpp_device_refresh (unifying->priv->hidpp_device,
				    HIDPP_REFRESH_FLAGS_VERSION |
				    HIDPP_REFRESH_FLAGS_KIND |
				    HIDPP_REFRESH_FLAGS_SERIAL |
				    HIDPP_REFRESH_FLAGS_MODEL,
				    &error);
	if (!ret) {
		g_warning ("failed to coldplug unifying device: %s",
			   error->message);
		g_error_free (error);
		goto out;
	}

	vendor = g_udev_device_get_property (native, "UPOWER_VENDOR");
	if (vendor == NULL)
		vendor = g_udev_device_get_property (native, "ID_VENDOR");

	/* set some default values */
	g_object_set (device,
		      "vendor", vendor,
		      "type", up_device_unifying_get_device_kind (unifying),
		      "model", hidpp_device_get_model (unifying->priv->hidpp_device),
		      "serial", hidpp_device_get_serial (unifying->priv->hidpp_device),
		      "has-history", TRUE,
		      "is-rechargeable", TRUE,
		      "power-supply", FALSE,
		      NULL);

	/* set up a poll to send the magic packet */
	up_device_unifying_refresh (device);
	up_daemon_start_poll (G_OBJECT (device), (GSourceFunc) up_device_unifying_refresh);
	ret = TRUE;
out:
	g_list_foreach (hidraw_list, (GFunc) g_object_unref, NULL);
	g_list_free (hidraw_list);
	if (parent != NULL)
		g_object_unref (parent);
	if (receiver != NULL)
		g_object_unref (receiver);
	if (client != NULL)
		g_object_unref (client);
	return ret;
}

/**
 * up_device_unifying_init:
 **/
static void
up_device_unifying_init (UpDeviceUnifying *unifying)
{
	unifying->priv = UP_DEVICE_UNIFYING_GET_PRIVATE (unifying);
}

/**
 * up_device_unifying_finalize:
 **/
static void
up_device_unifying_finalize (GObject *object)
{
	UpDeviceUnifying *unifying;

	g_return_if_fail (object != NULL);
	g_return_if_fail (UP_IS_DEVICE_UNIFYING (object));

	unifying = UP_DEVICE_UNIFYING (object);
	g_return_if_fail (unifying->priv != NULL);

	up_daemon_stop_poll (object);
	if (unifying->priv->hidpp_device != NULL)
		g_object_unref (unifying->priv->hidpp_device);

	G_OBJECT_CLASS (up_device_unifying_parent_class)->finalize (object);
}

/**
 * up_device_unifying_class_init:
 **/
static void
up_device_unifying_class_init (UpDeviceUnifyingClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	UpDeviceClass *device_class = UP_DEVICE_CLASS (klass);

	object_class->finalize = up_device_unifying_finalize;
	device_class->coldplug = up_device_unifying_coldplug;
	device_class->refresh = up_device_unifying_refresh;

	g_type_class_add_private (klass, sizeof (UpDeviceUnifyingPrivate));
}

/**
 * up_device_unifying_new:
 **/
UpDeviceUnifying *
up_device_unifying_new (void)
{
	return g_object_new (UP_TYPE_DEVICE_UNIFYING, NULL);
}
