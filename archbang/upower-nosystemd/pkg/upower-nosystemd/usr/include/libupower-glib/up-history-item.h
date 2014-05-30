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

#ifndef __UP_HISTORY_ITEM_H
#define __UP_HISTORY_ITEM_H

#include <glib-object.h>
#include <libupower-glib/up-types.h>

G_BEGIN_DECLS

#define UP_TYPE_HISTORY_ITEM		(up_history_item_get_type ())
#define UP_HISTORY_ITEM(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), UP_TYPE_HISTORY_ITEM, UpHistoryItem))
#define UP_HISTORY_ITEM_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), UP_TYPE_HISTORY_ITEM, UpHistoryItemClass))
#define UP_IS_HISTORY_ITEM(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), UP_TYPE_HISTORY_ITEM))
#define UP_IS_HISTORY_ITEM_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), UP_TYPE_HISTORY_ITEM))
#define UP_HISTORY_ITEM_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), UP_TYPE_HISTORY_ITEM, UpHistoryItemClass))

typedef struct UpHistoryItemPrivate UpHistoryItemPrivate;

typedef struct
{
	 GObject		 parent;
	 UpHistoryItemPrivate	*priv;
} UpHistoryItem;

typedef struct
{
	GObjectClass		 parent_class;
} UpHistoryItemClass;

GType		 up_history_item_get_type			(void);
UpHistoryItem	*up_history_item_new			(void);

gdouble		 up_history_item_get_value		(UpHistoryItem		*history_item);
void		 up_history_item_set_value		(UpHistoryItem		*history_item,
							 gdouble		 value);
guint		 up_history_item_get_time		(UpHistoryItem		*history_item);
void		 up_history_item_set_time		(UpHistoryItem		*history_item,
							 guint			 time);
void		 up_history_item_set_time_to_present	(UpHistoryItem		*history_item);
UpDeviceState	 up_history_item_get_state		(UpHistoryItem		*history_item);
void		 up_history_item_set_state		(UpHistoryItem		*history_item,
							 UpDeviceState		 state);
gchar		*up_history_item_to_string		(UpHistoryItem		*history_item);
gboolean	 up_history_item_set_from_string	(UpHistoryItem		*history_item,
							 const gchar		*text);

G_END_DECLS

#endif /* __UP_HISTORY_ITEM_H */

