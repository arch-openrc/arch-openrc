/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Richard Hughes <richard@hughsie.com>
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

#ifndef __UP_DEVICE_LIST_H
#define __UP_DEVICE_LIST_H

#include <glib-object.h>

#include "up-types.h"

G_BEGIN_DECLS

#define UP_TYPE_DEVICE_LIST		(up_device_list_get_type ())
#define UP_DEVICE_LIST(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), UP_TYPE_DEVICE_LIST, UpDeviceList))
#define UP_DEVICE_LIST_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), UP_TYPE_DEVICE_LIST, UpDeviceListClass))
#define UP_IS_DEVICE_LIST(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), UP_TYPE_DEVICE_LIST))
#define UP_IS_DEVICE_LIST_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), UP_TYPE_DEVICE_LIST))
#define UP_DEVICE_LIST_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), UP_TYPE_DEVICE_LIST, UpDeviceListClass))
#define UP_DEVICE_LIST_ERROR		(up_device_list_error_quark ())
#define UP_DEVICE_LIST_TYPE_ERROR	(up_device_list_error_get_type ())

typedef struct UpDeviceListPrivate UpDeviceListPrivate;

typedef struct
{
	 GObject		 parent;
	 UpDeviceListPrivate	*priv;
} UpDeviceList;

typedef struct
{
	GObjectClass		 parent_class;
} UpDeviceListClass;

GType		 up_device_list_get_type		(void);
UpDeviceList	*up_device_list_new			(void);
void		 up_device_list_test			(gpointer		 user_data);

GObject		*up_device_list_lookup			(UpDeviceList		*list,
							 GObject		*native);
gboolean	 up_device_list_insert			(UpDeviceList		*list,
							 GObject		*native,
							 GObject		*device);
gboolean	 up_device_list_remove			(UpDeviceList		*list,
							 GObject		*device);
GPtrArray	*up_device_list_get_array		(UpDeviceList		*list);

G_END_DECLS

#endif /* __UP_DEVICE_LIST_H */

