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

#ifndef __UP_STATS_ITEM_H
#define __UP_STATS_ITEM_H

#include <glib-object.h>

G_BEGIN_DECLS

#define UP_TYPE_STATS_ITEM		(up_stats_item_get_type ())
#define UP_STATS_ITEM(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), UP_TYPE_STATS_ITEM, UpStatsItem))
#define UP_STATS_ITEM_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), UP_TYPE_STATS_ITEM, UpStatsItemClass))
#define UP_IS_STATS_ITEM(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), UP_TYPE_STATS_ITEM))
#define UP_IS_STATS_ITEM_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), UP_TYPE_STATS_ITEM))
#define UP_STATS_ITEM_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), UP_TYPE_STATS_ITEM, UpStatsItemClass))

typedef struct UpStatsItemPrivate UpStatsItemPrivate;

typedef struct
{
	 GObject		 parent;
	 UpStatsItemPrivate	*priv;
} UpStatsItem;

typedef struct
{
	GObjectClass		 parent_class;
} UpStatsItemClass;

GType		 up_stats_item_get_type			(void);
UpStatsItem	*up_stats_item_new			(void);

gdouble		 up_stats_item_get_value		(UpStatsItem		*stats_item);
void		 up_stats_item_set_value		(UpStatsItem		*stats_item,
							 gdouble		 value);
gdouble		 up_stats_item_get_accuracy		(UpStatsItem		*stats_item);
void		 up_stats_item_set_accuracy		(UpStatsItem		*stats_item,
							 gdouble		 accuracy);

G_END_DECLS

#endif /* __UP_STATS_ITEM_H */

