/***************************************************************************
 *
 * up-util.h : utilities
 *
 * Copyright (C) 2006, 2007 Jean-Yves Lefort <jylefort@FreeBSD.org>
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
 **************************************************************************/

#ifndef _UP_UTIL_H
#define _UP_UTIL_H

#include "config.h"

#include <stdarg.h>
#include <glib.h>

#define UP_BOOL_TO_STRING(val)	((val) ? "true" : "false")

#define UP_LIST_FOREACH(var, head)	\
  for ((var) = (head);			\
       (var);				\
       (var) = (var)->next)

gboolean up_has_sysctl (const gchar *format, ...) G_GNUC_PRINTF(1, 2);
gboolean up_get_int_sysctl (int *value,
			     GError **err,
			     const gchar *format,
			     ...) G_GNUC_PRINTF(3, 4);
gchar *up_get_string_sysctl (GError **err,
			      const gchar *format,
			      ...) G_GNUC_PRINTF(2, 3);

gchar *up_make_safe_string (const gchar *text);

#endif /* _UP_UTIL_H */
