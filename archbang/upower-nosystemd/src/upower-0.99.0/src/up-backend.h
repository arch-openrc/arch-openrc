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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __UP_BACKEND_H
#define __UP_BACKEND_H

#include <glib-object.h>

#include "config.h"

#include "up-types.h"
#include "up-device.h"
#include "up-daemon.h"

G_BEGIN_DECLS

#define UP_TYPE_BACKEND		(up_backend_get_type ())
#define UP_BACKEND(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), UP_TYPE_BACKEND, UpBackend))
#define UP_BACKEND_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), UP_TYPE_BACKEND, UpBackendClass))
#define UP_IS_BACKEND(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), UP_TYPE_BACKEND))
#define UP_IS_BACKEND_CLASS(k)		(G_TYPE_CHECK_CLASS_TYPE ((k), UP_TYPE_BACKEND))
#define UP_BACKEND_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), UP_TYPE_BACKEND, UpBackendClass))
#define UP_BACKEND_ERROR		(up_backend_error_quark ())
#define UP_BACKEND_TYPE_ERROR		(up_backend_error_get_type ())

typedef struct UpBackendPrivate UpBackendPrivate;

typedef struct
{
	 GObject		 parent;
	 UpBackendPrivate	*priv;
} UpBackend;

typedef struct
{
	GObjectClass	 parent_class;
	void		(* device_added)	(UpBackend	*backend,
						 GObject	*native,
						 UpDevice	*device);
	void		(* device_changed)	(UpBackend	*backend,
						 GObject	*native,
						 UpDevice	*device);
	void		(* device_removed)	(UpBackend	*backend,
						 GObject	*native,
						 UpDevice	*device);
} UpBackendClass;

GType		 up_backend_get_type			(void);
UpBackend	*up_backend_new				(void);
void		 up_backend_test			(gpointer	 user_data);

gboolean	 up_backend_coldplug			(UpBackend	*backend,
							 UpDaemon	*daemon);
void		 up_backend_take_action			(UpBackend	*backend);
const char	*up_backend_get_critical_action		(UpBackend	*backend);

G_END_DECLS

#endif /* __UP_BACKEND_H */

