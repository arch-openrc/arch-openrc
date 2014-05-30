/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2012 Julien Danjou <julien@danjou.info>
 * Copyright (C) 2012 Richard Hughes <richard@hughsie.com>
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

#include <fcntl.h>
#include <glib-object.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include "hidpp-device.h"

/* Arbitrary value used in ping */
#define HIDPP_PING_DATA						0x42

#define HIDPP_RECEIVER_ADDRESS					0xff

/* HID++ 1.0 */
#define HIDPP_READ_SHORT_REGISTER				0x81
#define HIDPP_READ_SHORT_REGISTER_BATTERY			0x0d
#define HIDPP_READ_SHORT_REGISTER_BATTERY_APPROX		0x07

#define HIDPP_READ_LONG_REGISTER				0x83
#define HIDPP_READ_LONG_REGISTER_DEVICE_TYPE			11
#define HIDPP_READ_LONG_REGISTER_DEVICE_TYPE_KEYBOARD		0x1
#define HIDPP_READ_LONG_REGISTER_DEVICE_TYPE_MOUSE		0x2
#define HIDPP_READ_LONG_REGISTER_DEVICE_TYPE_NUMPAD		0x3
#define HIDPP_READ_LONG_REGISTER_DEVICE_TYPE_PRESENTER		0x4
#define HIDPP_READ_LONG_REGISTER_DEVICE_TYPE_REMOTE_CONTROL	0x7
#define HIDPP_READ_LONG_REGISTER_DEVICE_TYPE_TRACKBALL		0x8
#define HIDPP_READ_LONG_REGISTER_DEVICE_TYPE_TOUCHPAD		0x9
#define HIDPP_READ_LONG_REGISTER_DEVICE_TYPE_TABLET		0xa
#define HIDPP_READ_LONG_REGISTER_DEVICE_TYPE_GAMEPAD		0xb
#define HIDPP_READ_LONG_REGISTER_DEVICE_TYPE_JOYSTICK		0xc

#define HIDPP_ERROR_MESSAGE					0x8f

/* HID++ 1.0 error codes */
#define HIDPP10_ERROR_CODE_SUCCESS				0x00
#define HIDPP10_ERROR_CODE_INVALID_SUBID			0x01
#define HIDPP10_ERROR_CODE_INVALID_ADDRESS			0x02
#define HIDPP10_ERROR_CODE_INVALID_VALUE			0x03
#define HIDPP10_ERROR_CODE_CONNECT_FAIL				0x04
#define HIDPP10_ERROR_CODE_TOO_MANY_DEVICES			0x05
#define HIDPP10_ERROR_CODE_ALREADY_EXISTS			0x06
#define HIDPP10_ERROR_CODE_BUSY					0x07
#define HIDPP10_ERROR_CODE_UNKNOWN_DEVICE			0x08
#define HIDPP10_ERROR_CODE_RESOURCE_ERROR			0x09
#define HIDPP10_ERROR_CODE_REQUEST_UNAVAILABLE			0x0A
#define HIDPP10_ERROR_CODE_INVALID_PARAM_VALUE			0x0B
#define HIDPP10_ERROR_CODE_WRONG_PIN_CODE			0x0C

/* HID++ 2.0 */

/* HID++2.0 error codes */
#define HIDPP_ERROR_CODE_NOERROR				0x00
#define HIDPP_ERROR_CODE_UNKNOWN				0x01
#define HIDPP_ERROR_CODE_INVALIDARGUMENT			0x02
#define HIDPP_ERROR_CODE_OUTOFRANGE				0x03
#define HIDPP_ERROR_CODE_HWERROR				0x04
#define HIDPP_ERROR_CODE_LOGITECH_INTERNAL			0x05
#define HIDPP_ERROR_CODE_INVALID_FEATURE_INDEX			0x06
#define HIDPP_ERROR_CODE_INVALID_FUNCTION_ID			0x07
#define HIDPP_ERROR_CODE_BUSY					0x08
#define HIDPP_ERROR_CODE_UNSUPPORTED				0x09

#define HIDPP_FEATURE_ROOT					0x0000
#define HIDPP_FEATURE_ROOT_INDEX				0x00
#define HIDPP_FEATURE_ROOT_FN_GET_FEATURE			(0x00 << 4)
#define HIDPP_FEATURE_ROOT_FN_PING				(0x01 << 4)
#define HIDPP_FEATURE_I_FEATURE_SET				0x0001
#define HIDPP_FEATURE_I_FEATURE_SET_FN_GET_COUNT		(0x00 << 4)
#define HIDPP_FEATURE_I_FEATURE_SET_FN_GET_FEATURE_ID		(0x01 << 4)
#define HIDPP_FEATURE_I_FIRMWARE_INFO				0x0003
#define HIDPP_FEATURE_I_FIRMWARE_INFO_FN_GET_COUNT		(0x00 << 4)
#define HIDPP_FEATURE_I_FIRMWARE_INFO_FN_GET_INFO		(0x01 << 4)
#define HIDPP_FEATURE_GET_DEVICE_NAME_TYPE			0x0005
#define HIDPP_FEATURE_GET_DEVICE_NAME_TYPE_FN_GET_COUNT		(0x00 << 4)
#define HIDPP_FEATURE_GET_DEVICE_NAME_TYPE_FN_GET_NAME		(0x01 << 4)
#define HIDPP_FEATURE_GET_DEVICE_NAME_TYPE_FN_GET_TYPE		(0x02 << 4)
#define HIDPP_FEATURE_BATTERY_LEVEL_STATUS			0x1000
//#define HIDPP_FEATURE_BATTERY_LEVEL_STATUS_FN_GET_STATUS	(0x00 << 4)
//#define HIDPP_FEATURE_BATTERY_LEVEL_STATUS_BE			(0x01 << 4)
#define HIDPP_FEATURE_BATTERY_LEVEL_STATUS_FN_GET_CAPABILITY	(0x02 << 4)

#define HIDPP_FEATURE_BATTERY_LEVEL_STATUS_FN_GET_STATUS	0x02
#define HIDPP_FEATURE_BATTERY_LEVEL_STATUS_BE			0x02

#define HIDPP_FEATURE_SPECIAL_KEYS_MSE_BUTTONS			0x1B00
#define HIDPP_FEATURE_WIRELESS_DEVICE_STATUS			0x1D4B
#define HIDPP_FEATURE_WIRELESS_DEVICE_STATUS_BE			(0x00 << 4)

#define HIDPP_FEATURE_SOLAR_DASHBOARD				0x4301
#define HIDPP_FEATURE_SOLAR_DASHBOARD_FN_SET_LIGHT_MEASURE	(0x00 << 4)
#define HIDPP_FEATURE_SOLAR_DASHBOARD_BE_BATTERY_LEVEL_STATUS	(0x01 << 4)

#define HIDPP_DEVICE_READ_RESPONSE_TIMEOUT			3000 /* miliseconds */

typedef struct {
#define HIDPP_MSG_TYPE_SHORT	0x10
#define HIDPP_MSG_TYPE_LONG	0x11
#define HIDPP_MSG_LENGTH(msg) ((msg)->type == HIDPP_MSG_TYPE_SHORT ? 7 : 20)
	guchar			 type;
	guchar			 device_idx;
	guchar			 feature_idx;
	guchar			 function_idx; /* funcId:software_id */
	union {
		struct {
			guchar	 params[3];
		} s; /* short */
		struct {
			guchar	 params[16];
		} l; /* long */
	};
} HidppMessage;

struct HidppDevicePrivate
{
	gboolean		 enable_debug;
	gchar			*hidraw_device;
	gchar			*model;
	GIOChannel		*channel;
	GPtrArray		*feature_index;
	guint			 batt_percentage;
	guint			 channel_source_id;
	guint			 device_idx;
	guint			 version;
	HidppDeviceBattStatus	 batt_status;
	gboolean		 batt_is_approx;
	HidppDeviceKind		 kind;
	int			 fd;
	gboolean		 is_present;
	gchar			*serial;
	double			 lux;
};

typedef struct {
	gint			 idx;
	guint16			 feature;
	gchar			*name;
} HidppDeviceMap;

G_DEFINE_TYPE (HidppDevice, hidpp_device, G_TYPE_OBJECT)
#define HIDPP_DEVICE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), HIDPP_TYPE_DEVICE, HidppDevicePrivate))

/**
 * hidpp_is_error:
 *
 * Checks whether a message is a protocol-level error.
 */
static gboolean
hidpp_is_error(HidppMessage *msg, guchar *error)
{
	if (msg->type == HIDPP_MSG_TYPE_SHORT &&
		msg->feature_idx == HIDPP_ERROR_MESSAGE) {
		if (error)
			*error = msg->s.params[1];
		return TRUE;
	}

	return FALSE;
}

/**
 * hidpp_device_map_print:
 **/
static void
hidpp_device_map_print (HidppDevice *device)
{
	guint i;
	HidppDeviceMap *map;
	HidppDevicePrivate *priv = device->priv;

	if (!device->priv->enable_debug)
		return;
	for (i = 0; i < priv->feature_index->len; i++) {
		map = g_ptr_array_index (priv->feature_index, i);
		g_print ("%02x\t%s [%i]\n", map->idx, map->name, map->feature);
	}
}

/**
 * hidpp_device_map_get_by_feature:
 *
 * Gets the cached index from the function number.
 **/
static const HidppDeviceMap *
hidpp_device_map_get_by_feature (HidppDevice *device, guint16 feature)
{
	guint i;
	HidppDeviceMap *map;
	HidppDevicePrivate *priv = device->priv;

	for (i = 0; i < priv->feature_index->len; i++) {
		map = g_ptr_array_index (priv->feature_index, i);
		if (map->feature == feature)
			return map;
	}
	return NULL;
}

/**
 * hidpp_device_map_get_by_idx:
 *
 * Gets the cached index from the function index.
 **/
static const HidppDeviceMap *
hidpp_device_map_get_by_idx (HidppDevice *device, gint idx)
{
	guint i;
	HidppDeviceMap *map;
	HidppDevicePrivate *priv = device->priv;

	for (i = 0; i < priv->feature_index->len; i++) {
		map = g_ptr_array_index (priv->feature_index, i);
		if (map->idx == idx)
			return map;
	}
	return NULL;
}

/**
 * hidpp_device_print_buffer:
 *
 * Pretty print the send/recieve buffer.
 **/
static void
hidpp_device_print_buffer (HidppDevice *device, const HidppMessage *msg)
{
	guint i, mlen;
	const HidppDeviceMap *map;

	if (!device->priv->enable_debug)
		return;

	mlen = HIDPP_MSG_LENGTH(msg);
	for (i = 0; i < mlen; i++)
		g_print ("%02x ", ((const guchar*) msg)[i]);
	g_print ("\n");

	/* direction/type */
	if (msg->type == HIDPP_MSG_TYPE_SHORT)
		g_print ("REQUEST/SHORT\n");
	else if (msg->type == HIDPP_MSG_TYPE_LONG)
		g_print ("RESPONSE/LONG\n");
	else
		g_print ("??\n");

	/* dev index */
	g_print ("device-idx=%02x ", msg->device_idx);
	if (msg->device_idx == HIDPP_RECEIVER_ADDRESS) {
		g_print ("[Receiver]\n");
	} else if (device->priv->device_idx == msg->device_idx) {
		g_print ("[This Device]\n");
	} else {
		g_print ("[Random Device]\n");
	}

	/* feature index */
	if (msg->feature_idx == HIDPP_READ_LONG_REGISTER) {
		g_print ("feature-idx=%s [%02x]\n",
			 "v1(ReadLongRegister)", msg->feature_idx);
	} else {
		map = hidpp_device_map_get_by_idx (device, msg->feature_idx);
		g_print ("feature-idx=v2(%s) [%02x]\n",
			 map != NULL ? map->name : "unknown", msg->feature_idx);
	}

	g_print ("function-id=%01x\n", msg->function_idx & 0xf);
	g_print ("software-id=%01x\n", msg->function_idx >> 4);
	g_print ("param[0]=%02x\n\n", msg->s.params[0]);
}

static void
hidpp_discard_messages (HidppDevice	*device)
{
	HidppDevicePrivate *priv = device->priv;
	GPollFD poll[] = {
		{
			.fd = priv->fd,
			.events = G_IO_IN | G_IO_OUT | G_IO_ERR,
		},
	};
	char c;
	int r;

	while (g_poll (poll, G_N_ELEMENTS(poll), 0) > 0) {
		/* kernel discards remainder of packet */
		r = read (priv->fd, &c, 1);
		if (r < 0 && errno != EINTR)
			break;
	}
}

static gboolean
hidpp_device_read_resp (HidppDevice	*device,
			guchar		 device_index,
			guchar		 feature_index,
			guchar		 function_index,
			HidppMessage	*response,
			GError	**error);

/**
 * hidpp_device_cmd:
 **/
static gboolean
hidpp_device_cmd (HidppDevice	*device,
		  const HidppMessage	*request,
		  HidppMessage	*response,
		  GError	**error)
{
	gssize wrote;
	guint msg_len;
	HidppDevicePrivate *priv = device->priv;

	g_assert (request->type == HIDPP_MSG_TYPE_SHORT ||
			request->type == HIDPP_MSG_TYPE_LONG);

	hidpp_device_print_buffer (device, request);

	msg_len = HIDPP_MSG_LENGTH(request);

	/* ignore all unrelated queued messages */
	hidpp_discard_messages(device);

	/* write to the device */
	wrote = write (priv->fd, (const char *)request, msg_len);
	if ((gsize) wrote != msg_len) {
		if (wrote < 0) {
			g_set_error (error, 1, 0,
					"Failed to write HID++ request: %s",
					g_strerror (errno));
		} else {
			g_set_error (error, 1, 0,
					"Could not fully write HID++ request, wrote %" G_GSIZE_FORMAT " bytes",
					wrote);
		}
		return FALSE;
	}

	return hidpp_device_read_resp (device,
		request->device_idx,
		request->feature_idx,
		request->function_idx,
		response,
		error);
}

/**
 * hidpp_device_read_resp:
 */
static gboolean
hidpp_device_read_resp (HidppDevice	*device,
			guchar		 device_index,
			guchar		 feature_index,
			guchar		 function_index,
			HidppMessage	*response,
			GError	**error)
{
	HidppDevicePrivate *priv = device->priv;
	gboolean ret = TRUE;
	gssize r;
	GPollFD poll[] = {
		{
			.fd = priv->fd,
			.events = G_IO_IN | G_IO_OUT | G_IO_ERR,
		},
	};
	guint64 begin_time;
	gint remaining_time;
	guchar error_code;

	/* read from the device */
	begin_time = g_get_monotonic_time () / 1000;
	for (;;) {
		/* avoid infinite loop when there is no response */
		remaining_time = HIDPP_DEVICE_READ_RESPONSE_TIMEOUT -
			(g_get_monotonic_time () / 1000 - begin_time);
		if (remaining_time <= 0) {
			g_set_error (error, 1, 0,
					"timeout while reading response");
			ret = FALSE;
			goto out;
		}

		r = g_poll (poll, G_N_ELEMENTS(poll), remaining_time);
		if (r < 0) {
			if (errno == EINTR)
				continue;

			g_set_error (error, 1, 0,
					"Failed to read from device: %s",
					g_strerror (errno));
			ret = FALSE;
			goto out;
		} else if (r == 0) {
			g_set_error (error, 1, 0,
					"Attempt to read response from device timed out");
			ret = FALSE;
			goto out;
		}

		r = read (priv->fd, response, sizeof (*response));
		if (r <= 0) {
			if (r == -1 && errno == EINTR)
				continue;

			g_set_error (error, 1, 0,
					"Unable to read response from device: %s",
					g_strerror (errno));
			ret = FALSE;
			goto out;
		}

		hidpp_device_print_buffer (device, response);

		/* validate response */
		if (response->type != HIDPP_MSG_TYPE_SHORT &&
			response->type != HIDPP_MSG_TYPE_LONG) {
			/* ignore key presses, mouse motions, etc. */
			continue;
		}

		/* not our device */
		if (response->device_idx != device_index) {
			continue;
		}

		/* yep, this is our request */
		if (response->feature_idx == feature_index &&
			response->function_idx == function_index) {
			break;
		}

		/* recognize HID++ 1.0 errors */
		if (hidpp_is_error(response, &error_code) &&
			response->function_idx == feature_index &&
			response->s.params[0] == function_index) {
			g_set_error (error, 1, 0,
				"Unable to satisfy request, HID++ error %02x", error_code);
			ret = FALSE;
			goto out;
		}

		/* not our message, ignore it and try again */
	}

out:
	return ret;
}

/**
 * hidpp_device_map_add:
 *
 * Requests the index for a function, and adds it to the memeory cache
 * if it exists.
 **/
static gboolean
hidpp_device_map_add (HidppDevice *device,
		      guint16 feature,
		      const gchar *name)
{
	gboolean ret;
	GError *error = NULL;
	HidppMessage msg;
	HidppDeviceMap *map;
	HidppDevicePrivate *priv = device->priv;

	msg.type = HIDPP_MSG_TYPE_SHORT;
	msg.device_idx = priv->device_idx;
	msg.feature_idx = HIDPP_FEATURE_ROOT_INDEX;
	msg.function_idx = HIDPP_FEATURE_ROOT_FN_GET_FEATURE;
	msg.s.params[0] = feature >> 8;
	msg.s.params[1] = feature;
	msg.s.params[2] = 0x00;

	g_debug ("Getting idx for feature %s [%02x]", name, feature);
	ret = hidpp_device_cmd (device,
				&msg, &msg,
				&error);
	if (!ret) {
		g_warning ("Failed to get feature idx: %s", error->message);
		g_error_free (error);
		goto out;
	}

	/* zero index */
	if (msg.s.params[0] == 0x00) {
		ret = FALSE;
		g_debug ("Feature not found");
		goto out;
	}

	/* add to map */
	map = g_new0 (HidppDeviceMap, 1);
	map->idx = msg.s.params[0];
	map->feature = feature;
	map->name = g_strdup (name);
	g_ptr_array_add (priv->feature_index, map);
	g_debug ("Added feature %s [%02x] as idx %02x",
		 name, feature, map->idx);
out:
	return ret;
}

/**
 * hidpp_device_get_model:
 **/
const gchar *
hidpp_device_get_model (HidppDevice *device)
{
	g_return_val_if_fail (HIDPP_IS_DEVICE (device), NULL);
	return device->priv->model;
}

/**
 * hidpp_device_get_batt_percentage:
 **/
guint
hidpp_device_get_batt_percentage (HidppDevice *device)
{
	g_return_val_if_fail (HIDPP_IS_DEVICE (device), 0);
	return device->priv->batt_percentage;
}

/**
 * hidpp_device_get_version:
 **/
guint
hidpp_device_get_version (HidppDevice *device)
{
	g_return_val_if_fail (HIDPP_IS_DEVICE (device), 0);
	return device->priv->version;
}

/**
 * hidpp_device_get_batt_status:
 **/
HidppDeviceBattStatus
hidpp_device_get_batt_status (HidppDevice *device)
{
	g_return_val_if_fail (HIDPP_IS_DEVICE (device), HIDPP_DEVICE_BATT_STATUS_UNKNOWN);
	return device->priv->batt_status;
}

/**
 * hidpp_device_get_kind:
 **/
HidppDeviceKind
hidpp_device_get_kind (HidppDevice *device)
{
	g_return_val_if_fail (HIDPP_IS_DEVICE (device), HIDPP_DEVICE_KIND_UNKNOWN);
	return device->priv->kind;
}

/**
 * hidpp_device_get_serial:
 **/
const gchar *
hidpp_device_get_serial (HidppDevice *device)
{
	g_return_val_if_fail (HIDPP_IS_DEVICE (device), NULL);
	return device->priv->serial;
}

/**
 * hidpp_device_is_reachable:
 **/
gboolean
hidpp_device_is_reachable (HidppDevice *device)
{
	g_return_val_if_fail (HIDPP_IS_DEVICE (device), FALSE);
	return device->priv->is_present;
}

/**
 * hidpp_device_get_luminosity:
 * Determine the luminosity of the device in lux or negative if unknown.
 */
double
hidpp_device_get_luminosity (HidppDevice *device)
{
	g_return_val_if_fail (HIDPP_IS_DEVICE (device), -1);
	return device->priv->lux;
}

/**
 * hidpp_device_set_hidraw_device:
 **/
void
hidpp_device_set_hidraw_device (HidppDevice *device,
				const gchar *hidraw_device)
{
	g_return_if_fail (HIDPP_IS_DEVICE (device));
	device->priv->hidraw_device = g_strdup (hidraw_device);
}

/**
 * hidpp_device_set_index:
 **/
void
hidpp_device_set_index (HidppDevice *device,
			guint device_idx)
{
	g_return_if_fail (HIDPP_IS_DEVICE (device));
	device->priv->device_idx = device_idx;
}

/**
 * hidpp_device_set_enable_debug:
 **/
void
hidpp_device_set_enable_debug (HidppDevice *device,
			       gboolean enable_debug)
{
	g_return_if_fail (HIDPP_IS_DEVICE (device));
	device->priv->enable_debug = enable_debug;
}

/**
 * hidpp_device_refresh:
 **/
gboolean
hidpp_device_refresh (HidppDevice *device,
		      HidppRefreshFlags refresh_flags,
		      GError **error)
{
	const HidppDeviceMap *map;
	gboolean ret = TRUE;
	HidppMessage msg = { };
	guint len;
	HidppDevicePrivate *priv = device->priv;
	guchar error_code = 0;

	g_return_val_if_fail (HIDPP_IS_DEVICE (device), FALSE);

	/* open the device if it's not already opened */
	if (priv->fd < 0) {
		priv->fd = open (device->priv->hidraw_device, O_RDWR | O_NONBLOCK);
		if (priv->fd < 0) {
			g_set_error (error, 1, 0,
				     "cannot open device file %s",
				     priv->hidraw_device);
			ret = FALSE;
			goto out;
		}
	}

	/* get version */
	if ((refresh_flags & HIDPP_REFRESH_FLAGS_VERSION) > 0) {
		guint version_old = priv->version;

		msg.type = HIDPP_MSG_TYPE_SHORT;
		msg.device_idx = priv->device_idx;
		msg.feature_idx = HIDPP_FEATURE_ROOT_INDEX;
		msg.function_idx = HIDPP_FEATURE_ROOT_FN_PING;
		msg.s.params[0] = 0x00;
		msg.s.params[1] = 0x00;
		msg.s.params[2] = HIDPP_PING_DATA;
		ret = hidpp_device_cmd (device,
					&msg, &msg,
					error);
		if (!ret) {
			if (hidpp_is_error(&msg, &error_code) &&
				(error_code == HIDPP10_ERROR_CODE_INVALID_SUBID ||
				/* if a device is unreachable, assume HID++ 1.0.
				 * By doing so, we are still able to get the
				 * device type (e.g. mouse or keyboard) at
				 * enumeration time. */
				error_code == HIDPP10_ERROR_CODE_RESOURCE_ERROR)) {

				/* assert HID++ 1.0 for the device only if we
				 * are sure (i.e.  when the ping request
				 * returned INVALID_SUBID) */
				if (error_code == HIDPP10_ERROR_CODE_INVALID_SUBID) {
					priv->version = 1;
					priv->is_present = TRUE;
				} else {
					g_debug("Cannot detect version, unreachable device");
					priv->is_present = FALSE;
				}

				/* do not execute the error handler at the end
				 * of this function */
				memset(&msg, 0, sizeof (msg));
				g_error_free(*error);
				*error = NULL;
				ret = TRUE;
			}
		} else {
			priv->version = msg.s.params[0];
			priv->is_present = TRUE;
		}

		if (!ret)
			goto out;

		if (version_old != priv->version)
			g_debug("protocol for hid++ device changed from v%d to v%d",
					version_old, priv->version);

		if (version_old < 2 && priv->version >= 2)
			refresh_flags |= HIDPP_REFRESH_FLAGS_FEATURES;

	}

	if ((refresh_flags & HIDPP_REFRESH_FLAGS_FEATURES) > 0) {
		/* add features we are going to use */
//		hidpp_device_map_add (device,
//				      HIDPP_FEATURE_I_FEATURE_SET,
//				      "IFeatureSet");
//		hidpp_device_map_add (device,
//				      HIDPP_FEATURE_I_FIRMWARE_INFO,
//				      "IFirmwareInfo");
//		hidpp_device_map_add (device,
//				HIDPP_FEATURE_GET_DEVICE_NAME_TYPE,
//				"GetDeviceNameType");
		hidpp_device_map_add (device,
				HIDPP_FEATURE_BATTERY_LEVEL_STATUS,
				"BatteryLevelStatus");
//		hidpp_device_map_add (device,
//				      HIDPP_FEATURE_WIRELESS_DEVICE_STATUS,
//				      "WirelessDeviceStatus");
		hidpp_device_map_add (device,
				HIDPP_FEATURE_SOLAR_DASHBOARD,
				"SolarDashboard");
		hidpp_device_map_print (device);
	}

	/* get device kind */
	if ((refresh_flags & HIDPP_REFRESH_FLAGS_KIND) > 0) {

		/* the device type can always be queried using HID++ 1.0 on the
		 * receiver, regardless of the device version. */
		if (priv->version <= 1 || priv->version == 2) {
			msg.type = HIDPP_MSG_TYPE_SHORT;
			msg.device_idx = HIDPP_RECEIVER_ADDRESS;
			msg.feature_idx = HIDPP_READ_LONG_REGISTER;
			msg.function_idx = 0xb5;
			msg.s.params[0] = 0x20 | (priv->device_idx - 1);
			msg.s.params[1] = 0x00;
			msg.s.params[2] = 0x00;
			ret = hidpp_device_cmd (device,
						&msg, &msg,
						error);
			if (!ret)
				goto out;
			switch (msg.l.params[7]) {
			case HIDPP_READ_LONG_REGISTER_DEVICE_TYPE_KEYBOARD:
			case HIDPP_READ_LONG_REGISTER_DEVICE_TYPE_NUMPAD:
			case HIDPP_READ_LONG_REGISTER_DEVICE_TYPE_REMOTE_CONTROL:
				priv->kind = HIDPP_DEVICE_KIND_KEYBOARD;
				break;
			case HIDPP_READ_LONG_REGISTER_DEVICE_TYPE_MOUSE:
			case HIDPP_READ_LONG_REGISTER_DEVICE_TYPE_TRACKBALL:
			case HIDPP_READ_LONG_REGISTER_DEVICE_TYPE_TOUCHPAD:
			case HIDPP_READ_LONG_REGISTER_DEVICE_TYPE_PRESENTER:
				priv->kind = HIDPP_DEVICE_KIND_MOUSE;
				break;
			case HIDPP_READ_LONG_REGISTER_DEVICE_TYPE_TABLET:
				priv->kind = HIDPP_DEVICE_KIND_TABLET;
				break;
			case HIDPP_READ_LONG_REGISTER_DEVICE_TYPE_GAMEPAD:
			case HIDPP_READ_LONG_REGISTER_DEVICE_TYPE_JOYSTICK:
				/* upower doesn't have something for this yet */
				priv->kind = HIDPP_DEVICE_KIND_UNKNOWN;
				break;
			}
		}
	}

	/* get device model string */
	if ((refresh_flags & HIDPP_REFRESH_FLAGS_MODEL) > 0) {
		/* the device name can always be queried using HID++ 1.0 on the
		 * receiver, regardless of the device version. */
		if (priv->version <= 1 || priv->version == 2) {
			msg.type = HIDPP_MSG_TYPE_SHORT;
			msg.device_idx = HIDPP_RECEIVER_ADDRESS;
			msg.feature_idx = HIDPP_READ_LONG_REGISTER;
			msg.function_idx = 0xb5;
			msg.s.params[0] = 0x40 | (priv->device_idx - 1);
			msg.s.params[1] = 0x00;
			msg.s.params[2] = 0x00;

			ret = hidpp_device_cmd (device,
					&msg, &msg,
					error);
			if (!ret)
				goto out;

			len = msg.l.params[1];
			priv->model = g_strdup_printf ("%.*s", len, msg.l.params + 2);
		}
	}

	/* get serial number, this can be queried from the receiver */
	if ((refresh_flags & HIDPP_REFRESH_FLAGS_SERIAL) > 0) {
		guint32 *serialp;

		msg.type = HIDPP_MSG_TYPE_SHORT;
		msg.device_idx = HIDPP_RECEIVER_ADDRESS;
		msg.feature_idx = HIDPP_READ_LONG_REGISTER;
		msg.function_idx = 0xb5;
		msg.s.params[0] = 0x30 | (priv->device_idx - 1);
		msg.s.params[1] = 0x00;
		msg.s.params[2] = 0x00;

		ret = hidpp_device_cmd (device,
				&msg, &msg,
				error);
		if (!ret)
			goto out;

		serialp = (guint32 *) &msg.l.params[1];
		priv->serial = g_strdup_printf ("%08X", g_ntohl(*serialp));
	}

	/* get battery status */
	if ((refresh_flags & HIDPP_REFRESH_FLAGS_BATTERY) > 0) {
		if (priv->version == 1) {
			if (!priv->batt_is_approx) {
				msg.type = HIDPP_MSG_TYPE_SHORT;
				msg.device_idx = priv->device_idx;
				msg.feature_idx = HIDPP_READ_SHORT_REGISTER;
				msg.function_idx = HIDPP_READ_SHORT_REGISTER_BATTERY;
				memset(msg.s.params, 0, sizeof(msg.s.params));
				ret = hidpp_device_cmd (device,
							&msg, &msg,
							error);
				if (!ret && hidpp_is_error(&msg, &error_code) &&
					error_code == HIDPP10_ERROR_CODE_INVALID_ADDRESS) {
					g_error_free(*error);
					*error = NULL;
					priv->batt_is_approx = TRUE;
				}
			}

			if (priv->batt_is_approx) {
				msg.type = HIDPP_MSG_TYPE_SHORT;
				msg.device_idx = priv->device_idx;
				msg.feature_idx = HIDPP_READ_SHORT_REGISTER;
				msg.function_idx = HIDPP_READ_SHORT_REGISTER_BATTERY_APPROX;
				memset(msg.s.params, 0, sizeof(msg.s.params));

				ret = hidpp_device_cmd (device,
							&msg, &msg,
							error);
			}
			if (!ret)
				goto out;
			if (msg.function_idx == HIDPP_READ_SHORT_REGISTER_BATTERY) {
				priv->batt_percentage = msg.s.params[0];
				priv->batt_status = HIDPP_DEVICE_BATT_STATUS_DISCHARGING;
			} else {
				/* approximate battery levels */
				switch (msg.s.params[0]) {
				case 1: /* 0 - 10 */
					priv->batt_percentage = 5;
					break;
				case 3: /* 11 - 30 */
					priv->batt_percentage = 20;
					break;
				case 5: /* 31 - 80 */
					priv->batt_percentage = 55;
					break;
				case 7: /* 81 - 100 */
					priv->batt_percentage = 90;
					break;
				default:
					g_debug("Unknown battery percentage: %i", priv->batt_percentage);
					break;
				}
				switch (msg.s.params[1]) {
				case 0x00:
					priv->batt_status = HIDPP_DEVICE_BATT_STATUS_DISCHARGING;
					break;
				case 0x22:
				case 0x26: /* for notification, probably N/A for reg read */
					priv->batt_status = HIDPP_DEVICE_BATT_STATUS_CHARGED;
					break;
				case 0x25:
					priv->batt_status = HIDPP_DEVICE_BATT_STATUS_CHARGING;
					break;
				default:
					g_debug("Unknown battery status: 0x%02x", priv->batt_status);
					break;
				}
			}
		} else if (priv->version == 2) {

			/* sent a SetLightMeasure report */
			map = hidpp_device_map_get_by_feature (device, HIDPP_FEATURE_SOLAR_DASHBOARD);
			if (map != NULL) {
				msg.type = HIDPP_MSG_TYPE_SHORT;
				msg.device_idx = priv->device_idx;
				msg.feature_idx = map->idx;
				msg.function_idx = HIDPP_FEATURE_SOLAR_DASHBOARD_FN_SET_LIGHT_MEASURE;
				msg.s.params[0] = 0x01; /* Max number of reports: number of report sent after function call */
				msg.s.params[1] = 0x01; /* Report period: time between reports, in seconds */
				ret = hidpp_device_cmd (device,
							&msg, &msg,
							error);
				if (!ret)
					goto out;

				/* assume a BattLightMeasureEvent after previous command */
				ret = hidpp_device_read_resp (device,
							priv->device_idx,
							map->idx,
							HIDPP_FEATURE_SOLAR_DASHBOARD_BE_BATTERY_LEVEL_STATUS,
							&msg,
							error);
				if (!ret)
					goto out;

				priv->batt_percentage = msg.l.params[0];
				priv->lux = (msg.l.params[1] << 8) | msg.l.params[2];
				if (priv->lux > 200) {
					priv->batt_status = HIDPP_DEVICE_BATT_STATUS_CHARGING;
				} else {
					priv->batt_status = HIDPP_DEVICE_BATT_STATUS_DISCHARGING;
				}
			}

			/* send a BatteryLevelStatus report */
			map = hidpp_device_map_get_by_feature (device, HIDPP_FEATURE_BATTERY_LEVEL_STATUS);
			if (map != NULL) {
				msg.type = HIDPP_MSG_TYPE_SHORT;
				msg.device_idx = priv->device_idx;
				msg.feature_idx = map->idx;
				msg.function_idx = HIDPP_FEATURE_BATTERY_LEVEL_STATUS_FN_GET_STATUS;
				msg.s.params[0] = 0x00;
				msg.s.params[1] = 0x00;
				msg.s.params[2] = 0x00;
				ret = hidpp_device_cmd (device,
							&msg, &msg,
							error);
				if (!ret)
					goto out;

				/* convert the HID++ v2 status into something
				 * we can set on the device */
				switch (msg.s.params[2]) {
				case 0: /* discharging */
					priv->batt_status = HIDPP_DEVICE_BATT_STATUS_DISCHARGING;
					break;
				case 1: /* recharging */
				case 2: /* charge nearly complete */
				case 4: /* charging slowly */
					priv->batt_status = HIDPP_DEVICE_BATT_STATUS_CHARGING;
					break;
				case 3: /* charging complete */
					priv->batt_percentage = 100;
					priv->batt_status = HIDPP_DEVICE_BATT_STATUS_CHARGED;
					break;
				default:
					break;
				}

				/* do not overwrite battery status with 0 (unknown) */
				if (msg.s.params[0] != 0)
					priv->batt_percentage = msg.s.params[0];

				g_debug ("level=%i%%, next-level=%i%%, battery-status=%i",
					 msg.s.params[0], msg.s.params[1], msg.s.params[2]);
			}
		}
	}

	/* when no error occured for the requests done by the following refresh
	 * flags, assume the device present. Note that the is_present flag is
	 * always set when using HIDPP_REFRESH_FLAGS_VERSION */
	if (priv->version > 0 && refresh_flags &
			(HIDPP_REFRESH_FLAGS_MODEL |
			 HIDPP_REFRESH_FLAGS_BATTERY)) {
		priv->is_present = TRUE;
	}
out:
	/* do not spam when device is unreachable */
	if (hidpp_is_error(&msg, &error_code) &&
			(error_code == HIDPP10_ERROR_CODE_RESOURCE_ERROR)) {
		g_debug("HID++ error: %s", (*error)->message);
		g_error_free(*error);
		*error = NULL;
		/* the device is unreachable but paired, consider the refresh
		 * successful. Use is_present to determine if battery
		 * information is actually updated */
		ret = TRUE;
		priv->is_present = FALSE;
	}
	return ret;
}

/**
 * hidpp_device_init:
 **/
static void
hidpp_device_init (HidppDevice *device)
{
	HidppDeviceMap *map;

	device->priv = HIDPP_DEVICE_GET_PRIVATE (device);
	device->priv->fd = -1;
	device->priv->feature_index = g_ptr_array_new_with_free_func (g_free);
	device->priv->batt_status = HIDPP_DEVICE_BATT_STATUS_UNKNOWN;
	device->priv->kind = HIDPP_DEVICE_KIND_UNKNOWN;
	device->priv->lux = -1;

	/* add known root */
	map = g_new0 (HidppDeviceMap, 1);
	map->idx = HIDPP_FEATURE_ROOT_INDEX;
	map->feature = HIDPP_FEATURE_ROOT;
	map->name = g_strdup ("Root");
	g_ptr_array_add (device->priv->feature_index, map);
}

/**
 * hidpp_device_finalize:
 **/
static void
hidpp_device_finalize (GObject *object)
{
	HidppDevice *device;

	g_return_if_fail (object != NULL);
	g_return_if_fail (HIDPP_IS_DEVICE (object));

	device = HIDPP_DEVICE (object);
	g_return_if_fail (device->priv != NULL);

	if (device->priv->channel_source_id > 0)
		g_source_remove (device->priv->channel_source_id);

	if (device->priv->channel) {
		g_io_channel_shutdown (device->priv->channel, FALSE, NULL);
		g_io_channel_unref (device->priv->channel);
	}
	g_ptr_array_unref (device->priv->feature_index);

	g_free (device->priv->hidraw_device);
	g_free (device->priv->model);
	g_free (device->priv->serial);

	if (device->priv->fd > 0)
		close (device->priv->fd);

	G_OBJECT_CLASS (hidpp_device_parent_class)->finalize (object);
}

/**
 * hidpp_device_class_init:
 **/
static void
hidpp_device_class_init (HidppDeviceClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = hidpp_device_finalize;
	g_type_class_add_private (klass, sizeof (HidppDevicePrivate));
}

/**
 * hidpp_device_new:
 **/
HidppDevice *
hidpp_device_new (void)
{
	return g_object_new (HIDPP_TYPE_DEVICE, NULL);
}
