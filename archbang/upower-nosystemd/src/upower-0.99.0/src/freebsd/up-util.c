/***************************************************************************
 *
 * up-util.c : utilities
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

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <glib.h>

#include "up-util.h"

gboolean
up_has_sysctl (const gchar *format, ...)
{
	va_list args;
	gchar *name;
	size_t value_len;
	gboolean status;

	g_return_val_if_fail (format != NULL, FALSE);

	va_start (args, format);
	name = g_strdup_vprintf (format, args);
	va_end (args);

	status = sysctlbyname (name, NULL, &value_len, NULL, 0) == 0;

	g_free (name);
	return status;
}

gboolean
up_get_int_sysctl (int *value, GError **err, const gchar *format, ...)
{
	va_list args;
	gchar *name;
	size_t value_len = sizeof(int);
	gboolean status;

	g_return_val_if_fail (value != NULL, FALSE);
	g_return_val_if_fail (format != NULL, FALSE);

	va_start (args, format);
	name = g_strdup_vprintf (format, args);
	va_end (args);

	status = sysctlbyname (name, value, &value_len, NULL, 0) == 0;
	if (!status)
		g_set_error(err, 0, 0, "%s", g_strerror (errno));

	g_free (name);
	return status;
}

gchar *
up_get_string_sysctl (GError **err, const gchar *format, ...)
{
	va_list args;
	gchar *name;
	size_t value_len;
	gchar *str = NULL;

	g_return_val_if_fail(format != NULL, FALSE);

	va_start (args, format);
	name = g_strdup_vprintf (format, args);
	va_end (args);

	if (sysctlbyname (name, NULL, &value_len, NULL, 0) == 0) {
		str = g_new (char, value_len + 1);
		if (sysctlbyname (name, str, &value_len, NULL, 0) == 0)
			str[value_len] = 0;
		else {
			g_free (str);
			str = NULL;
		}
	}

	if (!str)
		g_set_error (err, 0, 0, "%s", g_strerror(errno));

	g_free(name);
	return str;
}

/**
 * up_util_make_safe_string:
 *
 * This is adapted from linux/up-device-supply.c.
 **/
gchar *
up_make_safe_string (const gchar *text)
{
	guint i;
	guint idx = 0;
	gchar *ret;

	/* no point in checking */
	if (text == NULL)
		return NULL;

	ret = g_strdup (text);

	/* shunt up only safe chars */
	for (i = 0; ret[i] != '\0'; i++) {
		if (g_ascii_isprint (ret[i])) {
			/* only copy if the address is going to change */
			if (idx != i)
				ret[idx] = ret[i];
			idx++;
		} else {
			g_debug ("invalid char '%c'", ret[i]);
		}
	}

	/* ensure null terminated */
	ret[idx] = '\0';

	return ret;
}
