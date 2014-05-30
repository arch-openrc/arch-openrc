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

#ifndef __UP_POLKIT_H
#define __UP_POLKIT_H

#include <glib-object.h>
#include <polkit/polkit.h>

G_BEGIN_DECLS

#define UP_TYPE_POLKIT		(up_polkit_get_type ())
#define UP_POLKIT(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), UP_TYPE_POLKIT, UpPolkit))
#define UP_POLKIT_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), UP_TYPE_POLKIT, UpPolkitClass))
#define UP_IS_POLKIT(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), UP_TYPE_POLKIT))
#define UP_IS_POLKIT_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), UP_TYPE_POLKIT))
#define UP_POLKIT_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), UP_TYPE_POLKIT, UpPolkitClass))

typedef struct UpPolkitPrivate UpPolkitPrivate;

typedef struct
{
	GObject parent;
	UpPolkitPrivate	*priv;
} UpPolkit;

typedef struct
{
	GObjectClass parent_class;
} UpPolkitClass;

GType		 up_polkit_get_type		(void);
UpPolkit	*up_polkit_new			(void);
void		 up_polkit_test			(gpointer		 user_data);

PolkitSubject	*up_polkit_get_subject		(UpPolkit		*polkit,
						 DBusGMethodInvocation	*context);
gboolean	 up_polkit_check_auth		(UpPolkit		*polkit,
						 PolkitSubject		*subject,
						 const gchar		*action_id,
						 DBusGMethodInvocation	*context);
gboolean	 up_polkit_is_allowed		(UpPolkit		*polkit,
						 PolkitSubject		*subject,
						 const gchar		*action_id,
						 GError		 	**error);
gboolean         up_polkit_get_uid		(UpPolkit              *polkit,
                                                 PolkitSubject          *subject,
                                                 uid_t                  *uid);
gboolean         up_polkit_get_pid		(UpPolkit              *polkit,
                                                 PolkitSubject          *subject,
                                                 pid_t                  *pid);

G_END_DECLS

#endif /* __UP_POLKIT_H */

