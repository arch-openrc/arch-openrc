/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Richard Hughes <richard@hughsie.com>
 *               2010 Alex Murray <murray.alex@gmail.com>
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

#ifndef __UP_KBD_BACKLIGHT_H
#define __UP_KBD_BACKLIGHT_H

#include <glib-object.h>
#include <dbus/dbus-glib.h>

G_BEGIN_DECLS

#define UP_TYPE_KBD_BACKLIGHT		(up_kbd_backlight_get_type ())
#define UP_KBD_BACKLIGHT(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), UP_TYPE_KBD_BACKLIGHT, UpKbdBacklight))
#define UP_KBD_BACKLIGHT_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), UP_TYPE_KBD_BACKLIGHT, UpKbdBacklightClass))
#define UP_IS_KBD_BACKLIGHT(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), UP_TYPE_KBD_BACKLIGHT))
#define UP_IS_KBD_BACKLIGHT_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), UP_TYPE_KBD_BACKLIGHT))
#define UP_KBD_BACKLIGHT_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), UP_TYPE_KBD_BACKLIGHT, UpKbdBacklightClass))

typedef struct UpKbdBacklightPrivate UpKbdBacklightPrivate;

typedef struct
{
	GObject		  parent;
	UpKbdBacklightPrivate	 *priv;
} UpKbdBacklight;

typedef struct
{
	GObjectClass	parent_class;
	void		(* brightness_changed)		(UpKbdBacklight	*kbd_backlight,
							 gint		 value);
} UpKbdBacklightClass;

UpKbdBacklight	*up_kbd_backlight_new			(void);
GType		 up_kbd_backlight_get_type		(void);

gboolean	 up_kbd_backlight_set_brightness	(UpKbdBacklight	*kbd_backlight,
							 gint		 value,
							 GError		**error);
gboolean	 up_kbd_backlight_get_brightness	(UpKbdBacklight	*kbd_backlight,
							 gint		*value,
							 GError		**error);
gboolean	 up_kbd_backlight_get_max_brightness	(UpKbdBacklight	*kbd_backlight,
							 gint		*value,
							 GError		**error);

G_END_DECLS

#endif	/* __UP_KBD_BACKLIGHT_H */
