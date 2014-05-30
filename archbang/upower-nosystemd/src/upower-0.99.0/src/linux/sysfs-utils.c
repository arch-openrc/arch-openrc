/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 David Zeuthen <davidz@redhat.com>
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <stdint.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <glib.h>

#include "sysfs-utils.h"

double
sysfs_get_double_with_error (const char *dir, const char *attribute)
{
	double result;
	char *contents;
	char *filename;

	filename = g_build_filename (dir, attribute, NULL);
	if (g_file_get_contents (filename, &contents, NULL, NULL)) {
		result = g_ascii_strtod (contents, NULL);
		g_free (contents);
	} else {
		result = -1.0;
	}
	g_free (filename);

	return result;
}

double
sysfs_get_double (const char *dir, const char *attribute)
{
	double result;
	char *contents;
	char *filename;

	result = 0.0;
	filename = g_build_filename (dir, attribute, NULL);
	if (g_file_get_contents (filename, &contents, NULL, NULL)) {
		result = g_ascii_strtod (contents, NULL);
		g_free (contents);
	}
	g_free (filename);


	return result;
}

char *
sysfs_get_string (const char *dir, const char *attribute)
{
	char *result;
	char *filename;

	result = NULL;
	filename = g_build_filename (dir, attribute, NULL);
	if (!g_file_get_contents (filename, &result, NULL, NULL)) {
		result = g_strdup ("");
	}
	g_free (filename);

	return result;
}

int
sysfs_get_int (const char *dir, const char *attribute)
{
	int result;
	char *contents;
	char *filename;

	result = 0;
	filename = g_build_filename (dir, attribute, NULL);
	if (g_file_get_contents (filename, &contents, NULL, NULL)) {
		result = atoi (contents);
		g_free (contents);
	}
	g_free (filename);

	return result;
}

gboolean
sysfs_get_bool (const char *dir, const char *attribute)
{
	gboolean result;
	char *contents;
	char *filename;

	result = FALSE;
	filename = g_build_filename (dir, attribute, NULL);
	if (g_file_get_contents (filename, &contents, NULL, NULL)) {
		g_strdelimit (contents, "\n", '\0');
		result = (g_strcmp0 (contents, "1") == 0);
		g_free (contents);
	}
	g_free (filename);

	return result;
}

gboolean
sysfs_file_exists (const char *dir, const char *attribute)
{
	gboolean result;
	char *filename;

	result = FALSE;
	filename = g_build_filename (dir, attribute, NULL);
	if (g_file_test (filename, G_FILE_TEST_EXISTS)) {
		result = TRUE;
	}
	g_free (filename);

	return result;
}
