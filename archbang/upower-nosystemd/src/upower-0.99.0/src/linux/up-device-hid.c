/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2006-2008 Richard Hughes <richard@hughsie.com>
 *
 * Based on hid-ups.c: Copyright (c) 2001 Vojtech Pavlik <vojtech@ucw.cz>
 *                     Copyright (c) 2001 Paul Stewart <hiddev@wetlogic.net>
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

#include <string.h>
#include <math.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gprintf.h>
#include <glib/gi18n-lib.h>
#include <glib-object.h>
#include <gudev/gudev.h>

/* asm/types.h required for __s32 in linux/hiddev.h */
#include <asm/types.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/hiddev.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "sysfs-utils.h"
#include "up-types.h"
#include "up-device-hid.h"

#define UP_DEVICE_HID_REFRESH_TIMEOUT			30l

#define UP_DEVICE_HID_USAGE				0x840000
#define UP_DEVICE_HID_SERIAL				0x8400fe
#define UP_DEVICE_HID_CHEMISTRY			0x850089
#define UP_DEVICE_HID_CAPACITY_MODE			0x85002c
#define UP_DEVICE_HID_BATTERY_VOLTAGE			0x840030
#define UP_DEVICE_HID_BELOW_RCL			0x840042
#define UP_DEVICE_HID_SHUTDOWN_IMMINENT		0x840069
#define UP_DEVICE_HID_PRODUCT				0x8400fe
#define UP_DEVICE_HID_SERIAL_NUMBER			0x8400ff
#define UP_DEVICE_HID_CHARGING				0x850044
#define UP_DEVICE_HID_DISCHARGING 			0x850045
#define UP_DEVICE_HID_REMAINING_CAPACITY		0x850066
#define UP_DEVICE_HID_RUNTIME_TO_EMPTY			0x850068
#define UP_DEVICE_HID_AC_PRESENT			0x8500d0
#define UP_DEVICE_HID_BATTERY_PRESENT			0x8500d1
#define UP_DEVICE_HID_DESIGN_CAPACITY			0x850083
#define UP_DEVICE_HID_DEVICE_NAME			0x850088
#define UP_DEVICE_HID_DEVICE_CHEMISTRY			0x850089
#define UP_DEVICE_HID_RECHARGEABLE			0x85008b
#define UP_DEVICE_HID_OEM_INFORMATION			0x85008f

#define UP_DEVICE_HID_PAGE_GENERIC_DESKTOP		0x01
#define UP_DEVICE_HID_PAGE_CONSUMER_PRODUCT		0x0c
#define UP_DEVICE_HID_PAGE_USB_MONITOR			0x80
#define UP_DEVICE_HID_PAGE_USB_ENUMERATED_VALUES	0x81
#define UP_DEVICE_HID_PAGE_VESA_VIRTUAL_CONTROLS	0x82
#define UP_DEVICE_HID_PAGE_RESERVED_MONITOR		0x83
#define UP_DEVICE_HID_PAGE_POWER_DEVICE		0x84
#define UP_DEVICE_HID_PAGE_BATTERY_SYSTEM		0x85

struct UpDeviceHidPrivate
{
	guint			 poll_timer_id;
	int			 fd;
};

G_DEFINE_TYPE (UpDeviceHid, up_device_hid, UP_TYPE_DEVICE)
#define UP_DEVICE_HID_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), UP_TYPE_DEVICE_HID, UpDeviceHidPrivate))

static gboolean		 up_device_hid_refresh	 	(UpDevice *device);

/**
 * up_device_hid_is_ups:
 **/
static gboolean
up_device_hid_is_ups (UpDeviceHid *hid)
{
	guint i;
	int retval;
	gboolean ret = FALSE;
	struct hiddev_devinfo device_info;

	/* get device info */
	retval = ioctl (hid->priv->fd, HIDIOCGDEVINFO, &device_info);
	if (retval < 0) {
		g_debug ("HIDIOCGDEVINFO failed: %s", strerror (errno));
		goto out;
	}

	/* can we use the hid device as a UPS? */
	for (i = 0; i < device_info.num_applications; i++) {
		retval = ioctl (hid->priv->fd, HIDIOCAPPLICATION, i);
		if (retval >> 16 == UP_DEVICE_HID_PAGE_POWER_DEVICE) {
			ret = TRUE;
			goto out;
		}
	}
out:
	return ret;
}

/**
 * up_device_hid_poll:
 **/
static gboolean
up_device_hid_poll (UpDeviceHid *hid)
{
	UpDevice *device = UP_DEVICE (hid);

	g_debug ("Polling: %s", up_device_get_object_path (device));
	up_device_hid_refresh (device);

	/* always continue polling */
	return TRUE;
}

/**
 * up_device_hid_get_string:
 **/
static const gchar *
up_device_hid_get_string (UpDeviceHid *hid, int sindex)
{
	static struct hiddev_string_descriptor sdesc;

	/* nothing to get */
	if (sindex == 0)
		return "";

	sdesc.index = sindex;

	/* failed */
	if (ioctl (hid->priv->fd, HIDIOCGSTRING, &sdesc) < 0)
		return "";

	g_debug ("value: '%s'", sdesc.value);
	return sdesc.value;
}

/**
 * up_device_hid_convert_device_technology:
 **/
static UpDeviceTechnology
up_device_hid_convert_device_technology (const gchar *type)
{
	if (type == NULL)
		return UP_DEVICE_TECHNOLOGY_UNKNOWN;
	if (g_ascii_strcasecmp (type, "pb") == 0 ||
	    g_ascii_strcasecmp (type, "pbac") == 0)
		return UP_DEVICE_TECHNOLOGY_LEAD_ACID;
	return UP_DEVICE_TECHNOLOGY_UNKNOWN;
}

/**
 * up_device_hid_set_values:
 **/
static gboolean
up_device_hid_set_values (UpDeviceHid *hid, int code, int value)
{
	const gchar *type;
	gboolean ret = TRUE;
	UpDevice *device = UP_DEVICE (hid);

	switch (code) {
	case UP_DEVICE_HID_REMAINING_CAPACITY:
		g_object_set (device, "percentage", (gfloat) CLAMP (value, 0, 100), NULL);
		break;
	case UP_DEVICE_HID_RUNTIME_TO_EMPTY:
		g_object_set (device, "time-to-empty", (gint64) value, NULL);
		break;
	case UP_DEVICE_HID_CHARGING:
		if (value != 0)
			g_object_set (device, "state", UP_DEVICE_STATE_CHARGING, NULL);
		break;
	case UP_DEVICE_HID_DISCHARGING:
		if (value != 0)
			g_object_set (device, "state", UP_DEVICE_STATE_DISCHARGING, NULL);
		break;
	case UP_DEVICE_HID_BATTERY_PRESENT:
		g_object_set (device, "is-present", (value != 0), NULL);
		break;
	case UP_DEVICE_HID_DEVICE_NAME:
		g_object_set (device, "device-name", up_device_hid_get_string (hid, value), NULL);
		break;
	case UP_DEVICE_HID_CHEMISTRY:
		type = up_device_hid_get_string (hid, value);
		g_object_set (device, "technology", up_device_hid_convert_device_technology (type), NULL);
		break;
	case UP_DEVICE_HID_RECHARGEABLE:
		g_object_set (device, "is-rechargeable", (value != 0), NULL);
		break;
	case UP_DEVICE_HID_OEM_INFORMATION:
		g_object_set (device, "vendor", up_device_hid_get_string (hid, value), NULL);
		break;
	case UP_DEVICE_HID_PRODUCT:
		g_object_set (device, "model", up_device_hid_get_string (hid, value), NULL);
		break;
	case UP_DEVICE_HID_SERIAL_NUMBER:
		g_object_set (device, "serial", up_device_hid_get_string (hid, value), NULL);
		break;
	case UP_DEVICE_HID_DESIGN_CAPACITY:
		g_object_set (device, "energy-full-design", (gfloat) value, NULL);
		break;
	default:
		ret = FALSE;
		break;
	}
	return ret;
}

/**
 * up_device_hid_get_all_data:
 **/
static gboolean
up_device_hid_get_all_data (UpDeviceHid *hid)
{
	struct hiddev_report_info rinfo;
	struct hiddev_field_info finfo;
	struct hiddev_usage_ref uref;
	int rtype;
	guint i, j;
	gboolean ret = FALSE;

	/* get all results */
	for (rtype = HID_REPORT_TYPE_MIN; rtype <= HID_REPORT_TYPE_MAX; rtype++) {
		rinfo.report_type = rtype;
		rinfo.report_id = HID_REPORT_ID_FIRST;
		while (ioctl (hid->priv->fd, HIDIOCGREPORTINFO, &rinfo) >= 0) {
			for (i = 0; i < rinfo.num_fields; i++) { 
				memset (&finfo, 0, sizeof (finfo));
				finfo.report_type = rinfo.report_type;
				finfo.report_id = rinfo.report_id;
				finfo.field_index = i;
				ioctl (hid->priv->fd, HIDIOCGFIELDINFO, &finfo);

				memset (&uref, 0, sizeof (uref));
				for (j = 0; j < finfo.maxusage; j++) {
					uref.report_type = finfo.report_type;
					uref.report_id = finfo.report_id;
					uref.field_index = i;
					uref.usage_index = j;
					ioctl (hid->priv->fd, HIDIOCGUCODE, &uref);
					ioctl (hid->priv->fd, HIDIOCGUSAGE, &uref);

					/* process each */
					up_device_hid_set_values (hid, uref.usage_code, uref.value);

					/* we got some data */
					ret = TRUE;
				}
			}
			rinfo.report_id |= HID_REPORT_ID_NEXT;
		}
	}
	return ret;
}

/**
 * up_device_hid_fixup_state:
 **/
static void
up_device_hid_fixup_state (UpDevice *device)
{
	gdouble percentage;

	/* map states the UPS cannot express */
	g_object_get (device, "percentage", &percentage, NULL);
	if (percentage < 0.01)
		g_object_set (device, "state", UP_DEVICE_STATE_EMPTY, NULL);
	if (percentage > 99.9)
		g_object_set (device, "state", UP_DEVICE_STATE_FULLY_CHARGED, NULL);
}

/**
 * up_device_hid_coldplug:
 *
 * Return %TRUE on success, %FALSE if we failed to get data and should be removed
 **/
static gboolean
up_device_hid_coldplug (UpDevice *device)
{
	UpDeviceHid *hid = UP_DEVICE_HID (device);
	GUdevDevice *native;
	gboolean ret = FALSE;
	gboolean fake_device;
	const gchar *device_file;
	const gchar *type;
	const gchar *vendor;

	/* detect what kind of device we are */
	native = G_UDEV_DEVICE (up_device_get_native (device));
	type = g_udev_device_get_property (native, "UPOWER_BATTERY_TYPE");
	if (type == NULL || g_strcmp0 (type, "ups") != 0)
		goto out;

	/* get the device file */
	device_file = g_udev_device_get_device_file (native);
	if (device_file == NULL) {
		g_debug ("could not get device file for HID device");
		goto out;
	}

	/* connect to the device */
	g_debug ("using device: %s", device_file);
	hid->priv->fd = open (device_file, O_RDONLY | O_NONBLOCK);
	if (hid->priv->fd < 0) {
		g_debug ("cannot open device file %s", device_file);
		goto out;
	}

	/* first check that we are an UPS */
	fake_device = g_udev_device_has_property (native, "UPOWER_FAKE_DEVICE");
	if (!fake_device)
	{
		ret = up_device_hid_is_ups (hid);
		if (!ret) {
			g_debug ("not a HID device: %s", device_file);
			goto out;
		}
	}

	/* prefer UPOWER names */
	vendor = g_udev_device_get_property (native, "UPOWER_VENDOR");
	if (vendor == NULL)
		vendor = g_udev_device_get_property (native, "ID_VENDOR");

	/* hardcode some values */
	g_object_set (device,
		      "type", UP_DEVICE_KIND_UPS,
		      "is-rechargeable", TRUE,
		      "power-supply", TRUE,
		      "is-present", TRUE,
		      "vendor", vendor,
		      "has-history", TRUE,
		      "has-statistics", TRUE,
		      NULL);

	/* coldplug everything */
	if (fake_device)
	{
		ret = TRUE;
		if (g_udev_device_get_property_as_boolean (native, "UPOWER_FAKE_HID_CHARGING"))
			up_device_hid_set_values (hid, UP_DEVICE_HID_CHARGING, 1);
		else
			up_device_hid_set_values (hid, UP_DEVICE_HID_DISCHARGING, 1);
		up_device_hid_set_values (hid, UP_DEVICE_HID_REMAINING_CAPACITY,
			g_udev_device_get_property_as_int (native, "UPOWER_FAKE_HID_PERCENTAGE"));
	} else {
		ret = up_device_hid_get_all_data (hid);
		if (!ret) {
			g_debug ("failed to coldplug UPS: %s", device_file);
			goto out;
		}
	}

	/* fix up device states */
	up_device_hid_fixup_state (device);
out:
	return ret;
}

/**
 * up_device_hid_refresh:
 *
 * Return %TRUE on success, %FALSE if we failed to refresh or no data
 **/
static gboolean
up_device_hid_refresh (UpDevice *device)
{
	gboolean set = FALSE;
	gboolean ret = FALSE;
	GTimeVal timeval;
	guint i;
	struct hiddev_event ev[64];
	int rd;
	UpDeviceHid *hid = UP_DEVICE_HID (device);

	/* read any data */
	rd = read (hid->priv->fd, ev, sizeof (ev));

	/* it's okay if there's nothing as we are non-blocking */
	if (rd == -1) {
		g_debug ("no data");
		ret = FALSE;
		goto out;
	}

	/* did we read enough data? */
	if (rd < (int) sizeof (ev[0])) {
		g_warning ("incomplete read (%i<%i)", rd, (int) sizeof (ev[0]));
		goto out;
	}

	/* process each event */
	for (i=0; i < rd / sizeof (ev[0]); i++) {
		set = up_device_hid_set_values (hid, ev[i].hid, ev[i].value);

		/* if only takes one match to make refresh a success */
		if (set)
			ret = TRUE;
	}

	/* fix up device states */
	up_device_hid_fixup_state (device);

	/* reset time */
	g_get_current_time (&timeval);
	g_object_set (device, "update-time", (guint64) timeval.tv_sec, NULL);
out:
	return ret;
}

/**
 * up_device_hid_get_on_battery:
 **/
static gboolean
up_device_hid_get_on_battery (UpDevice *device, gboolean *on_battery)
{
	UpDeviceHid *hid = UP_DEVICE_HID (device);
	UpDeviceKind type;
	UpDeviceState state;
	gboolean is_present;
	g_return_val_if_fail (UP_IS_DEVICE_HID (hid), FALSE);
	g_return_val_if_fail (on_battery != NULL, FALSE);

	g_object_get (device,
		      "type", &type,
		      "state", &state,
		      "is-present", &is_present,
		      NULL);


	if (type != UP_DEVICE_KIND_UPS)
		return FALSE;
	if (state == UP_DEVICE_STATE_UNKNOWN)
		return FALSE;
	if (!is_present)
		return FALSE;

	*on_battery = (state == UP_DEVICE_STATE_DISCHARGING);
	return TRUE;
}

/**
 * up_device_hid_init:
 **/
static void
up_device_hid_init (UpDeviceHid *hid)
{
	hid->priv = UP_DEVICE_HID_GET_PRIVATE (hid);
	hid->priv->fd = -1;
	hid->priv->poll_timer_id = g_timeout_add_seconds (UP_DEVICE_HID_REFRESH_TIMEOUT,
							  (GSourceFunc) up_device_hid_poll, hid);
	g_source_set_name_by_id (hid->priv->poll_timer_id, "[upower] up_device_hid_poll (linux)");
}

/**
 * up_device_hid_finalize:
 **/
static void
up_device_hid_finalize (GObject *object)
{
	UpDeviceHid *hid;

	g_return_if_fail (object != NULL);
	g_return_if_fail (UP_IS_DEVICE_HID (object));

	hid = UP_DEVICE_HID (object);
	g_return_if_fail (hid->priv != NULL);

	if (hid->priv->fd > 0)
		close (hid->priv->fd);
	if (hid->priv->poll_timer_id > 0)
		g_source_remove (hid->priv->poll_timer_id);

	G_OBJECT_CLASS (up_device_hid_parent_class)->finalize (object);
}

/**
 * up_device_hid_class_init:
 **/
static void
up_device_hid_class_init (UpDeviceHidClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	UpDeviceClass *device_class = UP_DEVICE_CLASS (klass);

	object_class->finalize = up_device_hid_finalize;
	device_class->coldplug = up_device_hid_coldplug;
	device_class->get_on_battery = up_device_hid_get_on_battery;
	device_class->refresh = up_device_hid_refresh;

	g_type_class_add_private (klass, sizeof (UpDeviceHidPrivate));
}

/**
 * up_device_hid_new:
 **/
UpDeviceHid *
up_device_hid_new (void)
{
	return g_object_new (UP_TYPE_DEVICE_HID, NULL);
}

