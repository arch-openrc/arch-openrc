/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008-2009 Richard Hughes <richard@hughsie.com>
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

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <glib.h>

#include "up-native.h"
#include "up-device-list.h"

static void	up_device_list_finalize	(GObject		*object);

#define UP_DEVICE_LIST_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), UP_TYPE_DEVICE_LIST, UpDeviceListPrivate))

struct UpDeviceListPrivate
{
	GPtrArray		*array;
	GHashTable		*map_native_path_to_device;
};

G_DEFINE_TYPE (UpDeviceList, up_device_list, G_TYPE_OBJECT)

/**
 * up_device_list_lookup:
 *
 * Convert a native %GObject into a %UpDevice -- we use the native path
 * to look these up as it's the only thing they share.
 *
 * Return value: the object, or %NULL if not found. Free with g_object_unref()
 **/
GObject *
up_device_list_lookup (UpDeviceList *list, GObject *native)
{
	GObject *device;
	const gchar *native_path;

	g_return_val_if_fail (UP_IS_DEVICE_LIST (list), NULL);

	/* does device exist in db? */
	native_path = up_native_get_native_path (native);
	if (native_path == NULL)
		return NULL;
	device = g_hash_table_lookup (list->priv->map_native_path_to_device, native_path);
	if (device == NULL)
		return NULL;
	return g_object_ref (device);
}

/**
 * up_device_list_insert:
 *
 * Insert a %GObject device and it's mapping to a backing %UpDevice
 * into a list of devices.
 **/
gboolean
up_device_list_insert (UpDeviceList *list, GObject *native, GObject *device)
{
	const gchar *native_path;

	g_return_val_if_fail (UP_IS_DEVICE_LIST (list), FALSE);
	g_return_val_if_fail (native != NULL, FALSE);
	g_return_val_if_fail (device != NULL, FALSE);

	native_path = up_native_get_native_path (native);
	if (native_path == NULL) {
		g_warning ("failed to get native path");
		return FALSE;
	}
	g_hash_table_insert (list->priv->map_native_path_to_device,
			     g_strdup (native_path), g_object_ref (device));
	g_ptr_array_add (list->priv->array, g_object_ref (device));
	g_debug ("added %s", native_path);
	return TRUE;
}

/**
 * up_device_list_remove_cb:
 **/
static gboolean
up_device_list_remove_cb (gpointer key, gpointer value, gpointer user_data)
{
	if (value == user_data) {
		g_debug ("removed %s", (char *) key);
		return TRUE;
	}
	return FALSE;
}

/**
 * up_device_list_remove:
 **/
gboolean
up_device_list_remove (UpDeviceList *list, GObject *device)
{
	g_return_val_if_fail (UP_IS_DEVICE_LIST (list), FALSE);
	g_return_val_if_fail (device != NULL, FALSE);

	/* remove the device from the db */
	g_hash_table_foreach_remove (list->priv->map_native_path_to_device,
				     up_device_list_remove_cb, device);
	g_ptr_array_remove (list->priv->array, device);

	/* we're removed the last instance? */
	if (!G_IS_OBJECT (device)) {
		g_warning ("INTERNAL STATE CORRUPT: we've removed the last instance of %p", device);
		return FALSE;
	}

	return TRUE;
}

/**
 * up_device_list_get_array:
 *
 * This is quick to iterate when we don't have GObject's to resolve
 *
 * Return value: the array, free with g_ptr_array_unref()
 **/
GPtrArray	*
up_device_list_get_array (UpDeviceList *list)
{
	g_return_val_if_fail (UP_IS_DEVICE_LIST (list), NULL);
	return g_ptr_array_ref (list->priv->array);
}

/**
 * up_device_list_class_init:
 * @klass: The UpDeviceListClass
 **/
static void
up_device_list_class_init (UpDeviceListClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = up_device_list_finalize;
	g_type_class_add_private (klass, sizeof (UpDeviceListPrivate));
}

/**
 * up_device_list_init:
 * @list: This class instance
 **/
static void
up_device_list_init (UpDeviceList *list)
{
	list->priv = UP_DEVICE_LIST_GET_PRIVATE (list);
	list->priv->array = g_ptr_array_new_with_free_func (g_object_unref);
	list->priv->map_native_path_to_device = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
}

/**
 * up_device_list_finalize:
 * @object: The object to finalize
 **/
static void
up_device_list_finalize (GObject *object)
{
	UpDeviceList *list;

	g_return_if_fail (UP_IS_DEVICE_LIST (object));

	list = UP_DEVICE_LIST (object);

	g_ptr_array_unref (list->priv->array);
	g_hash_table_unref (list->priv->map_native_path_to_device);

	G_OBJECT_CLASS (up_device_list_parent_class)->finalize (object);
}

/**
 * up_device_list_new:
 *
 * Return value: a new UpDeviceList object.
 **/
UpDeviceList *
up_device_list_new (void)
{
	return g_object_new (UP_TYPE_DEVICE_LIST, NULL);
}

