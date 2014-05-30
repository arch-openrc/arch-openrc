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

#if !defined (__UPOWER_H_INSIDE__) && !defined (UP_COMPILATION)
#error "Only <upower.h> can be included directly."
#endif

#ifndef __UP_DEVICE_H
#define __UP_DEVICE_H

#include <glib-object.h>
#include <gio/gio.h>

#include <libupower-glib/up-types.h>

G_BEGIN_DECLS

#define UP_TYPE_DEVICE		(up_device_get_type ())
#define UP_DEVICE(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), UP_TYPE_DEVICE, UpDevice))
#define UP_DEVICE_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), UP_TYPE_DEVICE, UpDeviceClass))
#define UP_IS_DEVICE(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), UP_TYPE_DEVICE))
#define UP_IS_DEVICE_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), UP_TYPE_DEVICE))
#define UP_DEVICE_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), UP_TYPE_DEVICE, UpDeviceClass))
#define UP_DEVICE_ERROR		(up_device_error_quark ())
#define UP_DEVICE_TYPE_ERROR	(up_device_error_get_type ())

typedef struct _UpDevicePrivate UpDevicePrivate;

typedef struct
{
	 GObject		 parent;
	 UpDevicePrivate	*priv;
} UpDevice;

typedef struct
{
	GObjectClass		 parent_class;
	/*< private >*/
	/* Padding for future expansion */
	void (*_up_device_reserved1) (void);
	void (*_up_device_reserved2) (void);
	void (*_up_device_reserved3) (void);
	void (*_up_device_reserved4) (void);
	void (*_up_device_reserved5) (void);
	void (*_up_device_reserved6) (void);
	void (*_up_device_reserved7) (void);
	void (*_up_device_reserved8) (void);
} UpDeviceClass;

/* general */
GType		 up_device_get_type			(void);
UpDevice	*up_device_new				(void);
gchar		*up_device_to_text			(UpDevice		*device);

/* sync versions */
gboolean	 up_device_refresh_sync			(UpDevice		*device,
							 GCancellable		*cancellable,
							 GError			**error);
gboolean	 up_device_set_object_path_sync		(UpDevice		*device,
							 const gchar		*object_path,
							 GCancellable		*cancellable,
							 GError			**error);
GPtrArray	*up_device_get_history_sync		(UpDevice		*device,
							 const gchar		*type,
							 guint			 timespec,
							 guint			 resolution,
							 GCancellable		*cancellable,
							 GError			**error);
GPtrArray	*up_device_get_statistics_sync		(UpDevice		*device,
							 const gchar		*type,
							 GCancellable		*cancellable,
							 GError			**error);

/* accessors */
const gchar	*up_device_get_object_path		(UpDevice		*device);

G_END_DECLS

#endif /* __UP_DEVICE_H */

