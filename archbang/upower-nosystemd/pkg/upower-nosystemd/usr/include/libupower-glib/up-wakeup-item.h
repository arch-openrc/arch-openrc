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

#ifndef __UP_WAKEUP_ITEM_H
#define __UP_WAKEUP_ITEM_H

#include <glib-object.h>
#include <libupower-glib/up-types.h>

G_BEGIN_DECLS

#define UP_TYPE_WAKEUP_ITEM		(up_wakeup_item_get_type ())
#define UP_WAKEUP_ITEM(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), UP_TYPE_WAKEUP_ITEM, UpWakeupItem))
#define UP_WAKEUP_ITEM_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), UP_TYPE_WAKEUP_ITEM, UpWakeupItemClass))
#define UP_IS_WAKEUP_ITEM(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), UP_TYPE_WAKEUP_ITEM))
#define UP_IS_WAKEUP_ITEM_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), UP_TYPE_WAKEUP_ITEM))
#define UP_WAKEUP_ITEM_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), UP_TYPE_WAKEUP_ITEM, UpWakeupItemClass))

typedef struct UpWakeupItemPrivate UpWakeupItemPrivate;

typedef struct
{
	 GObject		 parent;
	 UpWakeupItemPrivate	*priv;
} UpWakeupItem;

typedef struct
{
	GObjectClass		 parent_class;
} UpWakeupItemClass;

GType		 up_wakeup_item_get_type		(void);
UpWakeupItem	*up_wakeup_item_new			(void);

/* accessors */
gboolean	 up_wakeup_item_get_is_userspace	(UpWakeupItem		*wakeup_item);
void		 up_wakeup_item_set_is_userspace	(UpWakeupItem		*wakeup_item,
							 gboolean		 is_userspace);
guint		 up_wakeup_item_get_id			(UpWakeupItem		*wakeup_item);
void		 up_wakeup_item_set_id			(UpWakeupItem		*wakeup_item,
							 guint			 id);
guint		 up_wakeup_item_get_old			(UpWakeupItem		*wakeup_item);
void		 up_wakeup_item_set_old			(UpWakeupItem		*wakeup_item,
							 guint			 old);
gdouble		 up_wakeup_item_get_value		(UpWakeupItem		*wakeup_item);
void		 up_wakeup_item_set_value		(UpWakeupItem		*wakeup_item,
							 gdouble		 value);
const gchar	*up_wakeup_item_get_cmdline		(UpWakeupItem		*wakeup_item);
void		 up_wakeup_item_set_cmdline		(UpWakeupItem		*wakeup_item,
							 const gchar		*cmdline);
const gchar	*up_wakeup_item_get_details		(UpWakeupItem		*wakeup_item);
void		 up_wakeup_item_set_details		(UpWakeupItem		*wakeup_item,
							 const gchar		*details);

G_END_DECLS

#endif /* __UP_WAKEUP_ITEM_H */

