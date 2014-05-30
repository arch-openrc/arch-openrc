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

#ifndef __UP_HISTORY_H
#define __UP_HISTORY_H

#include <glib-object.h>

#include "up-types.h"

G_BEGIN_DECLS

#define UP_TYPE_HISTORY		(up_history_get_type ())
#define UP_HISTORY(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), UP_TYPE_HISTORY, UpHistory))
#define UP_HISTORY_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), UP_TYPE_HISTORY, UpHistoryClass))
#define UP_IS_HISTORY(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), UP_TYPE_HISTORY))
#define UP_IS_HISTORY_CLASS(k)		(G_TYPE_CHECK_CLASS_TYPE ((k), UP_TYPE_HISTORY))
#define UP_HISTORY_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), UP_TYPE_HISTORY, UpHistoryClass))
#define UP_HISTORY_ERROR		(up_history_error_quark ())
#define UP_HISTORY_TYPE_ERROR		(up_history_error_get_type ())

typedef struct UpHistoryPrivate UpHistoryPrivate;

typedef struct
{
	 GObject		 parent;
	 UpHistoryPrivate	*priv;
} UpHistory;

typedef struct
{
	GObjectClass		 parent_class;
} UpHistoryClass;

typedef enum {
	UP_HISTORY_TYPE_CHARGE,
	UP_HISTORY_TYPE_RATE,
	UP_HISTORY_TYPE_TIME_FULL,
	UP_HISTORY_TYPE_TIME_EMPTY,
	UP_HISTORY_TYPE_UNKNOWN
} UpHistoryType;


GType		 up_history_get_type			(void);
UpHistory	*up_history_new				(void);

GPtrArray	*up_history_get_data			(UpHistory		*history,
							 UpHistoryType		 type,
							 guint			 timespan,
							 guint			 resolution);
GPtrArray	*up_history_get_profile_data		(UpHistory		*history,
							 gboolean		 charging);
gboolean	 up_history_set_id			(UpHistory		*history,
							 const gchar		*id);
gboolean	 up_history_set_state			(UpHistory		*history,
							 UpDeviceState		 state);
gboolean	 up_history_set_charge_data		(UpHistory		*history,
							 gdouble		 percentage);
gboolean	 up_history_set_rate_data		(UpHistory		*history,
							 gdouble		 rate);
gboolean	 up_history_set_time_full_data		(UpHistory		*history,
							 gint64			 time);
gboolean	 up_history_set_time_empty_data		(UpHistory		*history,
							 gint64			 time);
void		 up_history_set_max_data_age		(UpHistory		*history,
							 guint			 max_data_age);
gboolean	 up_history_save_data			(UpHistory		*history);

void		 up_history_set_directory		(UpHistory		*history,
							 const gchar		*dir);

G_END_DECLS

#endif /* __UP_HISTORY_H */

