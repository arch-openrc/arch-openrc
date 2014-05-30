/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2011 Richard Hughes <richard@hughsie.com>
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

#ifndef __UP_CONFIG_H
#define __UP_CONFIG_H

#include <glib-object.h>

G_BEGIN_DECLS

#define UP_TYPE_CONFIG		(up_config_get_type ())
#define UP_CONFIG(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), UP_TYPE_CONFIG, UpConfig))
#define UP_CONFIG_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), UP_TYPE_CONFIG, UpConfigClass))
#define UP_IS_CONFIG(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), UP_TYPE_CONFIG))

typedef struct _UpConfigPrivate		UpConfigPrivate;
typedef struct _UpConfig		UpConfig;
typedef struct _UpConfigClass		UpConfigClass;

struct _UpConfig
{
	 GObject		 parent;
	 UpConfigPrivate	*priv;
};

struct _UpConfigClass
{
	GObjectClass		 parent_class;
};

GType		 up_config_get_type		(void);
UpConfig	*up_config_new			(void);
gboolean	 up_config_get_boolean		(UpConfig	*config,
						 const gchar	*key);
guint		 up_config_get_uint		(UpConfig	*config,
						 const gchar	*key);

G_END_DECLS

#endif /* __UP_CONFIG_H */
