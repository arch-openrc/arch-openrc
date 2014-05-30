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

#ifndef __HIDPP_DEVICE_H__
#define __HIDPP_DEVICE_H__

#include <glib-object.h>

#include "hidpp-device.h"

G_BEGIN_DECLS

#define HIDPP_TYPE_DEVICE  		(hidpp_device_get_type ())
#define HIDPP_DEVICE(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), HIDPP_TYPE_DEVICE, HidppDevice))
#define HIDPP_DEVICE_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), HIDPP_TYPE_DEVICE, HidppDeviceClass))
#define HIDPP_IS_DEVICE(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), HIDPP_TYPE_DEVICE))
#define HIDPP_IS_DEVICE_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), HIDPP_TYPE_DEVICE))
#define HIDPP_DEVICE_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), HIDPP_TYPE_DEVICE, HidppDeviceClass))

typedef struct HidppDevicePrivate HidppDevicePrivate;

typedef struct
{
	GObject			 parent;
	HidppDevicePrivate	*priv;
} HidppDevice;

typedef struct
{
	GObjectClass		 parent_class;
} HidppDeviceClass;

typedef enum {
	HIDPP_DEVICE_KIND_KEYBOARD,
	HIDPP_DEVICE_KIND_MOUSE,
	HIDPP_DEVICE_KIND_TOUCHPAD,
	HIDPP_DEVICE_KIND_TRACKBALL,
	HIDPP_DEVICE_KIND_TABLET,
	HIDPP_DEVICE_KIND_UNKNOWN
} HidppDeviceKind;

typedef enum {
	HIDPP_DEVICE_BATT_STATUS_CHARGING,
	HIDPP_DEVICE_BATT_STATUS_DISCHARGING,
	HIDPP_DEVICE_BATT_STATUS_CHARGED,
	HIDPP_DEVICE_BATT_STATUS_UNKNOWN
} HidppDeviceBattStatus;

typedef enum {
	HIDPP_REFRESH_FLAGS_VERSION	= 1,
	HIDPP_REFRESH_FLAGS_KIND	= 2,
	HIDPP_REFRESH_FLAGS_BATTERY	= 4,
	HIDPP_REFRESH_FLAGS_MODEL	= 8,
	HIDPP_REFRESH_FLAGS_FEATURES	= 16,
	HIDPP_REFRESH_FLAGS_SERIAL	= 32
} HidppRefreshFlags;

GType			 hidpp_device_get_type			(void);
const gchar		*hidpp_device_get_model			(HidppDevice	*device);
guint			 hidpp_device_get_batt_percentage	(HidppDevice	*device);
guint			 hidpp_device_get_version		(HidppDevice	*device);
HidppDeviceBattStatus	 hidpp_device_get_batt_status		(HidppDevice	*device);
HidppDeviceKind		 hidpp_device_get_kind			(HidppDevice	*device);
const gchar		*hidpp_device_get_serial		(HidppDevice	*device);
double			 hidpp_device_get_luminosity		(HidppDevice	*device);
void			 hidpp_device_set_hidraw_device		(HidppDevice	*device,
								 const gchar	*hidraw_device);
void			 hidpp_device_set_index			(HidppDevice	*device,
								 guint		 device_idx);
void			 hidpp_device_set_enable_debug		(HidppDevice	*device,
								 gboolean	 enable_debug);
gboolean		 hidpp_device_is_reachable		(HidppDevice	*device);
gboolean		 hidpp_device_refresh			(HidppDevice	*device,
								 HidppRefreshFlags refresh_flags,
								 GError		**error);
HidppDevice		*hidpp_device_new			(void);

G_END_DECLS

#endif /* __HIDPP_DEVICE_H__ */

