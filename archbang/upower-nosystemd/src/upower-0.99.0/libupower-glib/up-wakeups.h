/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2009 Richard Hughes <richard@hughsie.com>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#if !defined (__UPOWER_H_INSIDE__) && !defined (UP_COMPILATION)
#error "Only <upower.h> can be included directly."
#endif

#ifndef __UP_WAKEUPS_H
#define __UP_WAKEUPS_H

#include <glib-object.h>
#include <gio/gio.h>

#include <libupower-glib/up-types.h>
#include <libupower-glib/up-device.h>
#include <libupower-glib/up-wakeup-item.h>

G_BEGIN_DECLS

#define UP_TYPE_WAKEUPS		(up_wakeups_get_type ())
#define UP_WAKEUPS(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), UP_TYPE_WAKEUPS, UpWakeups))
#define UP_WAKEUPS_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), UP_TYPE_WAKEUPS, UpWakeupsClass))
#define UP_IS_WAKEUPS(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), UP_TYPE_WAKEUPS))
#define UP_IS_WAKEUPS_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), UP_TYPE_WAKEUPS))
#define UP_WAKEUPS_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), UP_TYPE_WAKEUPS, UpWakeupsClass))
#define UP_WAKEUPS_ERROR	(up_wakeups_error_quark ())
#define UP_WAKEUPS_TYPE_ERROR	(up_wakeups_error_get_type ())

typedef struct UpWakeupsPrivate UpWakeupsPrivate;

typedef struct
{
	 GObject		 parent;
	 UpWakeupsPrivate	*priv;
} UpWakeups;

typedef struct
{
	GObjectClass		 parent_class;
	void			(*data_changed)		(UpWakeups		*wakeups);
	void			(*total_changed)	(UpWakeups		*wakeups,
							 guint			 value);
} UpWakeupsClass;

GType		 up_wakeups_get_type			(void);
UpWakeups	*up_wakeups_new				(void);

/* sync versions */
guint		 up_wakeups_get_total_sync		(UpWakeups		*wakeups,
							 GCancellable		*cancellable,
							 GError			**error);
GPtrArray	*up_wakeups_get_data_sync		(UpWakeups		*wakeups,
							 GCancellable		*cancellable,
							 GError			**error);
gboolean	 up_wakeups_get_properties_sync		(UpWakeups		*wakeups,
							 GCancellable		*cancellable,
							 GError			**error);

/* accessors */
gboolean	 up_wakeups_get_has_capability		(UpWakeups		*wakeups);

G_END_DECLS

#endif /* __UP_WAKEUPS_H */

