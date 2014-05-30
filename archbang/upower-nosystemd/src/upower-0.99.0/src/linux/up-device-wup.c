/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2006-2008 Richard Hughes <richard@hughsie.com>
 *
 * Data values taken from wattsup.c: Copyright (C) 2005 Patrick Mochel
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
#include <glib-object.h>
#include <gudev/gudev.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <ctype.h>
#include <getopt.h>
#include <errno.h>

#include "sysfs-utils.h"
#include "up-types.h"
#include "up-device-wup.h"

#define UP_DEVICE_WUP_REFRESH_TIMEOUT			10 /* seconds */
#define UP_DEVICE_WUP_RESPONSE_OFFSET_WATTS		0x0
#define UP_DEVICE_WUP_RESPONSE_OFFSET_VOLTS		0x1
#define UP_DEVICE_WUP_RESPONSE_OFFSET_AMPS		0x2
#define UP_DEVICE_WUP_RESPONSE_OFFSET_KWH		0x3
#define UP_DEVICE_WUP_RESPONSE_OFFSET_COST		0x4
#define UP_DEVICE_WUP_RESPONSE_OFFSET_MONTHLY_KWH	0x5
#define UP_DEVICE_WUP_RESPONSE_OFFSET_MONTHLY_COST	0x6
#define UP_DEVICE_WUP_RESPONSE_OFFSET_MAX_WATTS	0x7
#define UP_DEVICE_WUP_RESPONSE_OFFSET_MAX_VOLTS	0x8
#define UP_DEVICE_WUP_RESPONSE_OFFSET_MAX_AMPS	0x9
#define UP_DEVICE_WUP_RESPONSE_OFFSET_MIN_WATTS	0xa
#define UP_DEVICE_WUP_RESPONSE_OFFSET_MIN_VOLTS	0xb
#define UP_DEVICE_WUP_RESPONSE_OFFSET_MIN_AMPS	0xc
#define UP_DEVICE_WUP_RESPONSE_OFFSET_POWER_FACTOR	0xd
#define UP_DEVICE_WUP_RESPONSE_OFFSET_DUTY_CYCLE	0xe
#define UP_DEVICE_WUP_RESPONSE_OFFSET_POWER_CYCLE	0xf

/* commands can never be bigger then this */
#define UP_DEVICE_WUP_COMMAND_LEN			256

struct UpDeviceWupPrivate
{
	guint			 poll_timer_id;
	int			 fd;
};

G_DEFINE_TYPE (UpDeviceWup, up_device_wup, UP_TYPE_DEVICE)
#define UP_DEVICE_WUP_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), UP_TYPE_DEVICE_WUP, UpDeviceWupPrivate))

static gboolean		 up_device_wup_refresh	 	(UpDevice *device);

/**
 * up_device_wup_poll_cb:
 **/
static gboolean
up_device_wup_poll_cb (UpDeviceWup *wup)
{
	UpDevice *device = UP_DEVICE (wup);

	g_debug ("Polling: %s", up_device_get_object_path (device));
	up_device_wup_refresh (device);

	/* always continue polling */
	return TRUE;
}

/**
 * up_device_wup_set_speed:
 **/
static gboolean
up_device_wup_set_speed (UpDeviceWup *wup)
{
	struct termios t;
	int retval;

	retval = tcgetattr (wup->priv->fd, &t);
	if (retval != 0) {
		g_debug ("failed to get speed");
		return FALSE;
	}

	cfmakeraw (&t);
	cfsetispeed (&t, B115200);
	cfsetospeed (&t, B115200);
	tcflush (wup->priv->fd, TCIFLUSH);

	t.c_iflag |= IGNPAR;
	t.c_cflag &= ~CSTOPB;
	retval = tcsetattr (wup->priv->fd, TCSANOW, &t);
	if (retval != 0) {
		g_debug ("failed to set speed");
		return FALSE;
	}

	return TRUE;
}

/**
 * up_device_wup_write_command:
 *
 * data: a command string in the form "#command,subcommand,datalen,data[n]", e.g. "#R,W,0"
 **/
static gboolean
up_device_wup_write_command (UpDeviceWup *wup, const gchar *data)
{
	guint ret = TRUE;
	gint retval;
	gint length;

	length = strlen (data);
	g_debug ("writing [%s]", data);
	retval = write (wup->priv->fd, data, length);
	if (retval != length) {
		g_debug ("Writing [%s] to device failed", data);
		ret = FALSE;
	}
	return ret;
}

/**
 * up_device_wup_read_command:
 *
 * Return value: Some data to parse
 **/
static gchar *
up_device_wup_read_command (UpDeviceWup *wup)
{
	int retval;
	gchar buffer[UP_DEVICE_WUP_COMMAND_LEN];
	retval = read (wup->priv->fd, &buffer, UP_DEVICE_WUP_COMMAND_LEN);
	if (retval < 0) {
		g_debug ("failed to read from fd: %s", strerror (errno));
		return NULL;
	}
	return g_strdup (buffer);
}

/**
 * up_device_wup_parse_command:
 *
 * Return value: Som data to parse
 **/
static gboolean
up_device_wup_parse_command (UpDeviceWup *wup, const gchar *data)
{
	gboolean ret = FALSE;
	gchar command;
	gchar subcommand;
	gchar **tokens = NULL;
	gchar *packet = NULL;
	guint i;
	guint size;
	guint length;
	guint number_tokens;
	UpDevice *device = UP_DEVICE (wup);
	const guint offset = 3;

	/* invalid */
	if (data == NULL)
		goto out;

	/* Try to find a valid packet in the data stream
	 * Data may be sdfsd#P,-,0;sdfs and we only want this bit:
	 *                  \-----/
	 * so try to find the start and the end */

	/* ensure we have a long enough response */
	length = strlen (data);
	if (length < 3) {
		g_debug ("not enough data '%s'", data);
		goto out;
	}

	/* strip to the first '#' char */
	for (i=0; i<length-1; i++)
		if (data[i] == '#')
			packet = g_strdup (data+i);

	/* does packet exist? */
	if (packet == NULL) {
		g_debug ("no start char in %s", data);
		goto out;
	}

	/* replace the first ';' char with a NULL if it exists */
	length = strlen (packet);
	for (i=0; i<length; i++) {
		if (packet[i] < 0x20 && packet[i] > 0x7e)
			packet[i] = '?';
		if (packet[i] == ';') {
			packet[i] = '\0';
			break;
		}
	}

	/* check we have enough data inthe packet */
	tokens = g_strsplit (packet, ",", -1);
	number_tokens = g_strv_length (tokens);
	if (number_tokens < 3) {
		g_debug ("not enough tokens '%s'", packet);
		goto out;
	}

	/* remove leading or trailing whitespace in tokens */
	for (i=0; i<number_tokens; i++)
		g_strstrip (tokens[i]);

	/* check the first token */
	length = strlen (tokens[0]);
	if (length != 2) {
		g_debug ("expected command '#?' but got '%s'", tokens[0]);
		goto out;
	}
	if (tokens[0][0] != '#') {
		g_debug ("expected command '#?' but got '%s'", tokens[0]);
		goto out;
	}
	command = tokens[0][1];

	/* check the second token */
	length = strlen (tokens[1]);
	if (length != 1) {
		g_debug ("expected command '?' but got '%s'", tokens[1]);
		goto out;
	}
	subcommand = tokens[1][0]; /* expect to be '-' */

	/* check the length is present */
	length = strlen (tokens[2]);
	if (length == 0) {
		g_debug ("length value not present");
		goto out;
	}

	/* check the length matches what data we've got*/
	size = atoi (tokens[2]);
	if (size != number_tokens - offset) {
		g_debug ("size expected to be '%i' but got '%i'", number_tokens - offset, size);
		goto out;
	}

	/* update the command fields */
	if (command == 'd' && subcommand == '-' && number_tokens - offset == 18) {
		g_object_set (device,
			      "energy-rate", strtod (tokens[offset+UP_DEVICE_WUP_RESPONSE_OFFSET_WATTS], NULL) / 10.0f,
			      "voltage", strtod (tokens[offset+UP_DEVICE_WUP_RESPONSE_OFFSET_VOLTS], NULL) / 10.0f,
			      NULL);
		ret = TRUE;
	} else {
		g_debug ("ignoring command '%c'", command);
	}

out:
	g_free (packet);
	g_strfreev (tokens);
	return ret;
}

/**
 * up_device_wup_coldplug:
 *
 * Return %TRUE on success, %FALSE if we failed to get data and should be removed
 **/
static gboolean
up_device_wup_coldplug (UpDevice *device)
{
	UpDeviceWup *wup = UP_DEVICE_WUP (device);
	GUdevDevice *native;
	gboolean ret = FALSE;
	const gchar *device_file;
	const gchar *type;
	const gchar *native_path;
	gchar *data;
	const gchar *vendor;
	const gchar *product;

	/* detect what kind of device we are */
	native = G_UDEV_DEVICE (up_device_get_native (device));
	type = g_udev_device_get_property (native, "UP_MONITOR_TYPE");
	if (type == NULL || g_strcmp0 (type, "wup") != 0)
		goto out;

	/* get the device file */
	device_file = g_udev_device_get_device_file (native);
	if (device_file == NULL) {
		g_debug ("could not get device file for WUP device");
		goto out;
	}

	/* connect to the device */
	wup->priv->fd = open (device_file, O_RDWR | O_NONBLOCK);
	if (wup->priv->fd < 0) {
		g_debug ("cannot open device file %s", device_file);
		goto out;
	}
	g_debug ("opened %s", device_file);

	/* set speed */
	ret = up_device_wup_set_speed (wup);
	if (!ret) {
		g_debug ("not a WUP device (cannot set speed): %s", device_file);
		goto out;
	}

	/* attempt to clear */
	ret = up_device_wup_write_command (wup, "#R,W,0;");
	if (!ret)
		g_debug ("failed to clear, nonfatal");

	/* setup logging interval */
	data = g_strdup_printf ("#L,W,3,E,1,%i;", UP_DEVICE_WUP_REFRESH_TIMEOUT);
	ret = up_device_wup_write_command (wup, data);
	if (!ret)
		g_debug ("failed to setup logging interval, nonfatal");
	g_free (data);

	/* dummy read */
	data = up_device_wup_read_command (wup);
	g_debug ("data after clear %s", data);

	/* shouldn't do anything */
	up_device_wup_parse_command (wup, data);
	g_free (data);

	/* prefer UPOWER names */
	vendor = g_udev_device_get_property (native, "UPOWER_VENDOR");
	if (vendor == NULL)
		vendor = g_udev_device_get_property (native, "ID_VENDOR");
	product = g_udev_device_get_property (native, "UPOWER_PRODUCT");
	if (product == NULL)
		product = g_udev_device_get_property (native, "ID_PRODUCT");

	/* hardcode some values */
	native_path = g_udev_device_get_sysfs_path (native);
	g_object_set (device,
		      "type", UP_DEVICE_KIND_MONITOR,
		      "is-rechargeable", FALSE,
		      "power-supply", FALSE,
		      "is-present", FALSE,
		      "vendor", vendor,
		      "model", product,
		      "serial", g_strstrip (sysfs_get_string (native_path, "serial")),
		      "has-history", TRUE,
		      "state", UP_DEVICE_STATE_DISCHARGING,
		      NULL);

	/* coldplug */
	g_debug ("coldplug");
	ret = up_device_wup_refresh (device);
out:
	return ret;
}

/**
 * up_device_wup_refresh:
 *
 * Return %TRUE on success, %FALSE if we failed to refresh or no data
 **/
static gboolean
up_device_wup_refresh (UpDevice *device)
{
	gboolean ret = FALSE;
	GTimeVal timeval;
	gchar *data = NULL;
	UpDeviceWup *wup = UP_DEVICE_WUP (device);

	/* get data */
	data = up_device_wup_read_command (wup);
	if (data == NULL) {
		g_debug ("no data");
		goto out;
	}

	/* parse */
	ret = up_device_wup_parse_command (wup, data);
	if (!ret) {
		g_debug ("failed to parse %s", data);
		goto out;
	}

	/* reset time */
	g_get_current_time (&timeval);
	g_object_set (device, "update-time", (guint64) timeval.tv_sec, NULL);

out:
	g_free (data);
	/* FIXME: always true? */
	return TRUE;
}

/**
 * up_device_wup_init:
 **/
static void
up_device_wup_init (UpDeviceWup *wup)
{
	wup->priv = UP_DEVICE_WUP_GET_PRIVATE (wup);
	wup->priv->fd = -1;
	wup->priv->poll_timer_id = g_timeout_add_seconds (UP_DEVICE_WUP_REFRESH_TIMEOUT,
							  (GSourceFunc) up_device_wup_poll_cb, wup);
	g_source_set_name_by_id (wup->priv->poll_timer_id, "[upower] up_device_wup_poll_cb (linux)");
}

/**
 * up_device_wup_finalize:
 **/
static void
up_device_wup_finalize (GObject *object)
{
	UpDeviceWup *wup;

	g_return_if_fail (object != NULL);
	g_return_if_fail (UP_IS_DEVICE_WUP (object));

	wup = UP_DEVICE_WUP (object);
	g_return_if_fail (wup->priv != NULL);

	if (wup->priv->fd > 0)
		close (wup->priv->fd);
	if (wup->priv->poll_timer_id > 0)
		g_source_remove (wup->priv->poll_timer_id);

	G_OBJECT_CLASS (up_device_wup_parent_class)->finalize (object);
}

/**
 * up_device_wup_class_init:
 **/
static void
up_device_wup_class_init (UpDeviceWupClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	UpDeviceClass *device_class = UP_DEVICE_CLASS (klass);

	object_class->finalize = up_device_wup_finalize;
	device_class->coldplug = up_device_wup_coldplug;
	device_class->refresh = up_device_wup_refresh;

	g_type_class_add_private (klass, sizeof (UpDeviceWupPrivate));
}

/**
 * up_device_wup_new:
 **/
UpDeviceWup *
up_device_wup_new (void)
{
	return g_object_new (UP_TYPE_DEVICE_WUP, NULL);
}

