/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008-2010 Richard Hughes <richard@hughsie.com>
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

#ifndef __UP_CLIENT_H
#define __UP_CLIENT_H

#include <glib-object.h>
#include <gio/gio.h>

#include <libupower-glib/up-device.h>

G_BEGIN_DECLS

#define UP_TYPE_CLIENT			(up_client_get_type ())
#define UP_CLIENT(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), UP_TYPE_CLIENT, UpClient))
#define UP_CLIENT_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), UP_TYPE_CLIENT, UpClientClass))
#define UP_IS_CLIENT(o)			(G_TYPE_CHECK_INSTANCE_TYPE ((o), UP_TYPE_CLIENT))
#define UP_IS_CLIENT_CLASS(k)		(G_TYPE_CHECK_CLASS_TYPE ((k), UP_TYPE_CLIENT))
#define UP_CLIENT_GET_CLASS(o)		(G_TYPE_INSTANCE_GET_CLASS ((o), UP_TYPE_CLIENT, UpClientClass))
#define UP_CLIENT_ERROR			(up_client_error_quark ())
#define UP_CLIENT_TYPE_ERROR		(up_client_error_get_type ())

typedef struct _UpClientPrivate UpClientPrivate;

typedef struct
{
	 GObject		 parent;
	 UpClientPrivate	*priv;
} UpClient;

typedef struct
{
	GObjectClass		 parent_class;
	void			(*device_added)		(UpClient		*client,
							 UpDevice		*device);
	void			(*device_removed)	(UpClient		*client,
							 const gchar		*object_path);
	/*< private >*/
	/* Padding for future expansion */
	void (*_up_client_reserved1) (void);
	void (*_up_client_reserved2) (void);
	void (*_up_client_reserved3) (void);
	void (*_up_client_reserved4) (void);
	void (*_up_client_reserved5) (void);
	void (*_up_client_reserved6) (void);
	void (*_up_client_reserved7) (void);
	void (*_up_client_reserved8) (void);
} UpClientClass;

/* general */
GType		 up_client_get_type			(void);
UpClient	*up_client_new				(void);

/* sync versions */
UpDevice *	 up_client_get_display_device		(UpClient *client);
char *		 up_client_get_critical_action		(UpClient *client);

/* accessors */
GPtrArray	*up_client_get_devices			(UpClient		*client);
const gchar	*up_client_get_daemon_version		(UpClient		*client);
gboolean	 up_client_get_lid_is_closed		(UpClient		*client);
gboolean	 up_client_get_lid_is_present		(UpClient		*client);
gboolean	 up_client_get_is_docked		(UpClient		*client);
gboolean	 up_client_get_on_battery		(UpClient		*client);

G_END_DECLS

#endif /* __UP_CLIENT_H */

