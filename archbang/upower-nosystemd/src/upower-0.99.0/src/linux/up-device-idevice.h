/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2010 Bastien Nocera <hadess@hadess.net>
 * Copyright (C) 2008 David Zeuthen <davidz@redhat.com>
 * Copyright (C) 2008 Richard Hughes <richard@hughsie.com>
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

#ifndef __UP_DEVICE_IDEVICE_H__
#define __UP_DEVICE_IDEVICE_H__

#include <glib-object.h>
#include "up-device.h"

G_BEGIN_DECLS

#define UP_TYPE_DEVICE_IDEVICE  			(up_device_idevice_get_type ())
#define UP_DEVICE_IDEVICE(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), UP_TYPE_DEVICE_IDEVICE, UpDeviceIdevice))
#define UP_DEVICE_IDEVICE_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), UP_TYPE_DEVICE_IDEVICE, UpDeviceIdeviceClass))
#define UP_IS_DEVICE_IDEVICE(o)			(G_TYPE_CHECK_INSTANCE_TYPE ((o), UP_TYPE_DEVICE_IDEVICE))
#define UP_IS_DEVICE_IDEVICE_CLASS(k)		(G_TYPE_CHECK_CLASS_TYPE ((k), UP_TYPE_DEVICE_IDEVICE))
#define UP_DEVICE_IDEVICE_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), UP_TYPE_DEVICE_IDEVICE, UpDeviceIdeviceClass))

typedef struct UpDeviceIdevicePrivate UpDeviceIdevicePrivate;

typedef struct
{
	UpDevice		 parent;
	UpDeviceIdevicePrivate	*priv;
} UpDeviceIdevice;

typedef struct
{
	UpDeviceClass		 parent_class;
} UpDeviceIdeviceClass;

GType		 up_device_idevice_get_type		(void);
UpDeviceIdevice	*up_device_idevice_new			(void);

G_END_DECLS

#endif /* __UP_DEVICE_IDEVICE_H__ */

