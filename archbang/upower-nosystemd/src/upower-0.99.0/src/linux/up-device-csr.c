/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2005-2010 Richard Hughes <richard@hughsie.com>
 * Copyright (C) 2004 Sergey V. Udaltsov <svu@gnome.org>
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
#include <libusb.h>

#include "sysfs-utils.h"
#include "up-types.h"
#include "up-device-csr.h"

#define UP_DEVICE_CSR_REFRESH_TIMEOUT		30L

/* Internal CSR registers */
#define CSR_P6  			0
#define CSR_P0  			1
#define CSR_P4  			2
#define CSR_P5  			3
#define CSR_P8  			4
#define CSR_P9  			5
#define CSR_PB0 			6
#define CSR_PB1 			7

struct UpDeviceCsrPrivate
{
	gboolean		 is_dual;
	guint			 bus_num;
	guint			 dev_num;
	gint			 raw_value;
	libusb_context		*ctx;
	libusb_device		*device;
};

G_DEFINE_TYPE (UpDeviceCsr, up_device_csr, UP_TYPE_DEVICE)
#define UP_DEVICE_CSR_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), UP_TYPE_DEVICE_CSR, UpDeviceCsrPrivate))

static gboolean		 up_device_csr_refresh	 	(UpDevice *device);

/**
 * up_device_csr_poll_cb:
 **/
static gboolean
up_device_csr_poll_cb (UpDeviceCsr *csr)
{
	UpDevice *device = UP_DEVICE (csr);

	g_debug ("Polling: %s", up_device_get_object_path (device));
	up_device_csr_refresh (device);

	/* always continue polling */
	return TRUE;
}

/**
 * up_device_csr_find_device:
 **/
static libusb_device *
up_device_csr_find_device (UpDeviceCsr *csr)
{
	libusb_device *curr_device = NULL;
	libusb_device **devices = NULL;
	guint8 bus_num;
	guint8 dev_num;
	guint i;
	ssize_t cnt;

	g_debug ("Looking for: [%03d][%03d]", csr->priv->bus_num, csr->priv->dev_num);

	/* try to find the right device */
	cnt = libusb_get_device_list (csr->priv->ctx, &devices);
	if (cnt < 0) {
/*		need to depend on > libusb1-1.0.9 for libusb_strerror()
		g_warning ("failed to get device list: %s", libusb_strerror (cnt));
 */
		g_warning ("failed to get device list: %d", (int)cnt);
		goto out;
	}
	if (devices == NULL) {
		g_warning ("failed to get device list");
		goto out;
	}
	for (i=0; devices[i] != NULL; i++) {

		bus_num = libusb_get_bus_number (devices[i]);
		dev_num = libusb_get_device_address (devices[i]);
		if (bus_num == csr->priv->bus_num &&
		    dev_num == csr->priv->dev_num) {
			curr_device = libusb_ref_device (devices[i]);
			break;
		}
	}

	libusb_free_device_list (devices, TRUE);
out:
	return curr_device;
}

/**
 * up_device_csr_coldplug:
 *
 * Return %TRUE on success, %FALSE if we failed to get data and should be removed
 **/
static gboolean
up_device_csr_coldplug (UpDevice *device)
{
	UpDeviceCsr *csr = UP_DEVICE_CSR (device);
	GUdevDevice *native;
	gboolean ret = FALSE;
	const gchar *type;
	const gchar *native_path;
	const gchar *vendor;
	const gchar *product;

	/* get the type */
	native = G_UDEV_DEVICE (up_device_get_native (device));
	type = g_udev_device_get_property (native, "UPOWER_BATTERY_TYPE");
	if (type == NULL)
		goto out;

	/* which one? */
	if (g_strcmp0 (type, "mouse") == 0)
		g_object_set (device, "type", UP_DEVICE_KIND_MOUSE, NULL);
	else if (g_strcmp0 (type, "keyboard") == 0)
		g_object_set (device, "type", UP_DEVICE_KIND_KEYBOARD, NULL);
	else {
		g_debug ("not a recognised csr device");
		goto out;
	}

	/* get what USB device we are */
	native_path = g_udev_device_get_sysfs_path (native);
	csr->priv->bus_num = sysfs_get_int (native_path, "busnum");
	csr->priv->dev_num = sysfs_get_int (native_path, "devnum");

	/* get correct bus numbers? */
	if (csr->priv->bus_num == 0 || csr->priv->dev_num == 0) {
		g_warning ("unable to get bus or device numbers");
		goto out;
	}

	/* try to get the usb device */
	csr->priv->device = up_device_csr_find_device (csr);
	if (csr->priv->device == NULL) {
		g_debug ("failed to get device %p", csr);
		goto out;
	}

	/* get optional quirk parameters */
	ret = g_udev_device_has_property (native, "UPOWER_CSR_DUAL");
	if (ret)
		csr->priv->is_dual = g_udev_device_get_property_as_boolean (native, "UPOWER_CSR_DUAL");
	g_debug ("is_dual=%i", csr->priv->is_dual);

	/* prefer UPOWER names */
	vendor = g_udev_device_get_property (native, "UPOWER_VENDOR");
	if (vendor == NULL)
		vendor = g_udev_device_get_property (native, "ID_VENDOR");
	product = g_udev_device_get_property (native, "UPOWER_PRODUCT");
	if (product == NULL)
		product = g_udev_device_get_property (native, "ID_PRODUCT");

	/* hardcode some values */
	g_object_set (device,
		      "vendor", vendor,
		      "model", product,
		      "power-supply", FALSE,
		      "is-present", TRUE,
		      "is-rechargeable", TRUE,
		      "state", UP_DEVICE_STATE_DISCHARGING,
		      "has-history", TRUE,
		      NULL);

	/* coldplug */
	ret = up_device_csr_refresh (device);
	if (!ret)
		goto out;

	/* set up a poll */
	up_daemon_start_poll (G_OBJECT (device), (GSourceFunc) up_device_csr_poll_cb);
out:
	return ret;
}

/**
 * up_device_csr_refresh:
 *
 * Return %TRUE on success, %FALSE if we failed to refresh or no data
 **/
static gboolean
up_device_csr_refresh (UpDevice *device)
{
	gboolean ret = FALSE;
	GTimeVal timeval;
	UpDeviceCsr *csr = UP_DEVICE_CSR (device);
	libusb_device_handle *handle = NULL;
	guint8 buf[80];
	guint addr;
	gdouble percentage;
	gint retval;

	/* ensure we still have a device */
	if (csr->priv->device == NULL) {
		g_warning ("no device!");
		goto out;
	}

	/* open USB device */
	retval = libusb_open (csr->priv->device, &handle);
	if (retval < 0) {
		g_warning ("could not open device: %i", retval);
		goto out;
	}

	/* For dual receivers C502, C504 and C505, the mouse is the
	 * second device and uses an addr of 1 in the value and index
	 * fields' high byte */
	addr = csr->priv->is_dual ? 1<<8 : 0;

	/* get the charge */
	retval = libusb_control_transfer (handle, 0xc0, 0x09, 0x03|addr, 0x00|addr,
					  buf, 8, UP_DEVICE_CSR_REFRESH_TIMEOUT);
	if (retval < 0) {
		g_warning ("failed to write to device: %i", retval);
		goto out;
	}

	/* ensure we wrote 8 bytes */
	if (retval != 8) {
		g_warning ("failed to write to device, wrote %i bytes", retval);
		goto out;
	}

	/* is a C504 receiver busy? */
	if (buf[CSR_P0] == 0x3b && buf[CSR_P4] == 0) {
		g_warning ("receiver busy");
		goto out;
	}

	/* get battery status */
	csr->priv->raw_value = buf[CSR_P5] & 0x07;
	g_debug ("charge level: %d", csr->priv->raw_value);
	if (csr->priv->raw_value != 0) {
		percentage = (100.0 / 7.0) * csr->priv->raw_value;
		g_object_set (device, "percentage", percentage, NULL);
		g_debug ("percentage=%f", percentage);
	}

	/* reset time */
	g_get_current_time (&timeval);
	g_object_set (device, "update-time", (guint64) timeval.tv_sec, NULL);

	/* success */
	ret = TRUE;
out:
	if (handle != NULL)
		libusb_close (handle);
	return ret;
}

/**
 * up_device_csr_init:
 **/
static void
up_device_csr_init (UpDeviceCsr *csr)
{
	gint retval;
	csr->priv = UP_DEVICE_CSR_GET_PRIVATE (csr);

	csr->priv->raw_value = -1;
	retval = libusb_init (&csr->priv->ctx);
	if (retval < 0)
		g_warning ("could not initialize libusb: %i", retval);
}

/**
 * up_device_csr_finalize:
 **/
static void
up_device_csr_finalize (GObject *object)
{
	UpDeviceCsr *csr;

	g_return_if_fail (object != NULL);
	g_return_if_fail (UP_IS_DEVICE_CSR (object));

	csr = UP_DEVICE_CSR (object);
	g_return_if_fail (csr->priv != NULL);

	if (csr->priv->ctx != NULL)
		libusb_exit (csr->priv->ctx);
	up_daemon_stop_poll (object);

	G_OBJECT_CLASS (up_device_csr_parent_class)->finalize (object);
}

/**
 * up_device_csr_class_init:
 **/
static void
up_device_csr_class_init (UpDeviceCsrClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	UpDeviceClass *device_class = UP_DEVICE_CLASS (klass);

	object_class->finalize = up_device_csr_finalize;
	device_class->coldplug = up_device_csr_coldplug;
	device_class->refresh = up_device_csr_refresh;

	g_type_class_add_private (klass, sizeof (UpDeviceCsrPrivate));
}

/**
 * up_device_csr_new:
 **/
UpDeviceCsr *
up_device_csr_new (void)
{
	return g_object_new (UP_TYPE_DEVICE_CSR, NULL);
}

