/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2010 Richard Hughes <richard@hughsie.com>
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

#ifndef __UP_DOCK_H__
#define __UP_DOCK_H__

#include <glib-object.h>

#include "up-daemon.h"

G_BEGIN_DECLS

#define UP_TYPE_DOCK  		(up_dock_get_type ())
#define UP_DOCK(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), UP_TYPE_DOCK, UpDock))
#define UP_DOCK_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), UP_TYPE_DOCK, UpDockClass))
#define UP_IS_DOCK(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), UP_TYPE_DOCK))
#define UP_IS_DOCK_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), UP_TYPE_DOCK))
#define UP_DOCK_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), UP_TYPE_DOCK, UpDockClass))

typedef struct UpDockPrivate UpDockPrivate;

typedef struct
{
	GObject			 parent;
	UpDockPrivate		*priv;
} UpDock;

typedef struct
{
	GObjectClass		 parent_class;
} UpDockClass;

GType		 up_dock_get_type		(void);
UpDock		*up_dock_new			(void);
gboolean	 up_dock_coldplug		(UpDock		*dock,
						 UpDaemon	*daemon);
void		 up_dock_set_should_poll	(UpDock		*dock,
						 gboolean	 should_poll);

G_END_DECLS

#endif /* __UP_DOCK_H__ */

