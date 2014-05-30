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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <glib-object.h>

#include "up-wakeups.h"
#include "up-wakeups-glue.h"

static void	up_wakeups_class_init	(UpWakeupsClass	*klass);
static void	up_wakeups_init		(UpWakeups	*wakeups);
static void	up_wakeups_finalize	(GObject	*object);

#define UP_WAKEUPS_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), UP_TYPE_WAKEUPS, UpWakeupsPrivate))

struct UpWakeupsPrivate
{
	UpWakeupsGlue		*proxy;
};

enum {
	UP_WAKEUPS_DATA_CHANGED,
	UP_WAKEUPS_TOTAL_CHANGED,
	UP_WAKEUPS_LAST_SIGNAL
};

static guint signals [UP_WAKEUPS_LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (UpWakeups, up_wakeups, G_TYPE_OBJECT)

/**
 * up_wakeups_get_total_sync:
 * @wakeups: a #UpWakeups instance.
 * @cancellable: a #GCancellable or %NULL
 * @error: a #GError, or %NULL.
 *
 * Gets the the total number of wakeups per second from the daemon.
 *
 * Return value: number of wakeups per second.
 *
 * Since: 0.9.1
 **/
guint
up_wakeups_get_total_sync (UpWakeups *wakeups, GCancellable *cancellable, GError **error)
{
	guint total = 0;
	gboolean ret;

	g_return_val_if_fail (UP_IS_WAKEUPS (wakeups), FALSE);
	g_return_val_if_fail (wakeups->priv->proxy != NULL, FALSE);

	ret = up_wakeups_glue_call_get_total_sync (wakeups->priv->proxy, &total,
						   cancellable, error);
	if (!ret)
		total = 0;
	return total;
}

/**
 * up_wakeups_get_data_sync:
 * @wakeups: a #UpWakeups instance.
 * @cancellable: a #GCancellable or %NULL
 * @error: a #GError, or %NULL.
 *
 * Gets the wakeups data from the daemon.
 *
 * Return value: (element-type UpWakeupItem) (transfer full): an array of %UpWakeupItem's
 *
 * Since: 0.9.1
 **/
GPtrArray *
up_wakeups_get_data_sync (UpWakeups *wakeups, GCancellable *cancellable, GError **error)
{
	GError *error_local = NULL;
	GVariant *gva;
	guint i;
	GPtrArray *array = NULL;
	gboolean ret;
	gsize len;
	GVariantIter *iter;

	g_return_val_if_fail (UP_IS_WAKEUPS (wakeups), NULL);
	g_return_val_if_fail (wakeups->priv->proxy != NULL, NULL);

	/* get compound data */
	ret = up_wakeups_glue_call_get_data_sync (wakeups->priv->proxy,
						  &gva,
						  NULL,
						  &error_local);

	if (!ret) {
		g_warning ("GetData on failed: %s", error_local->message);
		g_set_error (error, 1, 0, "%s", error_local->message);
		g_error_free (error_local);
		goto out;
	}

	/* no data */
	iter = g_variant_iter_new (gva);
	len = g_variant_iter_n_children (iter);
	if (len == 0) {
		g_variant_iter_free (iter);
		goto out;
	}

	/* convert */
	array = g_ptr_array_new_with_free_func ((GDestroyNotify) g_object_unref);
	for (i = 0; i < len; i++) {
		UpWakeupItem *obj;
		GVariant *v;
		gboolean is_userspace;
		guint32 id;
		double value;
		char *cmdline;
		char *details;

		v = g_variant_iter_next_value (iter);
		g_variant_get (v, "(budss)",
			       &is_userspace, &id, &value, &cmdline, &details);
		g_variant_unref (v);

		obj = up_wakeup_item_new ();
		up_wakeup_item_set_is_userspace (obj, is_userspace);
		up_wakeup_item_set_id (obj, id);
		up_wakeup_item_set_value (obj, value);
		up_wakeup_item_set_cmdline (obj, cmdline);
		up_wakeup_item_set_details (obj, details);
		g_free (cmdline);
		g_free (details);

		g_ptr_array_add (array, obj);
	}
	g_variant_iter_free (iter);
out:
	if (gva != NULL)
		g_variant_unref (gva);
	return array;
}

/**
 * up_wakeups_get_properties_sync:
 * @wakeups: a #UpWakeups instance.
 * @cancellable: a #GCancellable or %NULL
 * @error: a #GError, or %NULL.
 *
 * Gets properties from the daemon about wakeup data.
 *
 * Return value: %TRUE if supported
 *
 * Since: 0.9.1
 **/
gboolean
up_wakeups_get_properties_sync (UpWakeups *wakeups, GCancellable *cancellable, GError **error)
{
	g_return_val_if_fail (UP_IS_WAKEUPS (wakeups), FALSE);
	/* Nothing to do here */
	return TRUE;
}

/**
 * up_wakeups_get_has_capability:
 * @wakeups: a #UpWakeups instance.
 *
 * Returns if the daemon supports getting the wakeup data.
 *
 * Return value: %TRUE if supported
 *
 * Since: 0.9.1
 **/
gboolean
up_wakeups_get_has_capability (UpWakeups *wakeups)
{
	g_return_val_if_fail (UP_IS_WAKEUPS (wakeups), FALSE);
	return up_wakeups_glue_get_has_capability (wakeups->priv->proxy);
}

/**
 * up_wakeups_total_changed_cb:
 **/
static void
up_wakeups_total_changed_cb (UpWakeupsGlue *proxy, guint value, UpWakeups *wakeups)
{
	g_signal_emit (wakeups, signals [UP_WAKEUPS_TOTAL_CHANGED], 0, value);
}

/**
 * up_wakeups_data_changed_cb:
 **/
static void
up_wakeups_data_changed_cb (UpWakeupsGlue *proxy, UpWakeups *wakeups)
{
	g_signal_emit (wakeups, signals [UP_WAKEUPS_DATA_CHANGED], 0);
}

/**
 * up_wakeups_class_init:
 **/
static void
up_wakeups_class_init (UpWakeupsClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = up_wakeups_finalize;

	signals [UP_WAKEUPS_DATA_CHANGED] =
		g_signal_new ("data-changed",
			      G_TYPE_FROM_CLASS (object_class), G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (UpWakeupsClass, data_changed),
			      NULL, NULL, g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
	signals [UP_WAKEUPS_TOTAL_CHANGED] =
		g_signal_new ("total-changed",
			      G_TYPE_FROM_CLASS (object_class), G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (UpWakeupsClass, data_changed),
			      NULL, NULL, g_cclosure_marshal_VOID__UINT,
			      G_TYPE_NONE, 1, G_TYPE_UINT);

	g_type_class_add_private (klass, sizeof (UpWakeupsPrivate));
}

/**
 * up_wakeups_init:
 **/
static void
up_wakeups_init (UpWakeups *wakeups)
{
	GError *error = NULL;

	wakeups->priv = UP_WAKEUPS_GET_PRIVATE (wakeups);

	/* connect to main interface */
	wakeups->priv->proxy = up_wakeups_glue_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
								       G_DBUS_PROXY_FLAGS_NONE,
								       "org.freedesktop.UPower",
								       "/org/freedesktop/UPower/Wakeups",
								       NULL,
								       &error);
	if (wakeups->priv->proxy == NULL) {
		g_warning ("Couldn't connect to proxy: %s", error->message);
		g_error_free (error);
		return;
	}

	/* all callbacks */
	g_signal_connect (wakeups->priv->proxy, "total-changed",
			  G_CALLBACK (up_wakeups_total_changed_cb), wakeups);
	g_signal_connect (wakeups->priv->proxy, "data-changed",
			  G_CALLBACK (up_wakeups_data_changed_cb), wakeups);
}

/**
 * up_wakeups_finalize:
 **/
static void
up_wakeups_finalize (GObject *object)
{
	UpWakeups *wakeups;

	g_return_if_fail (UP_IS_WAKEUPS (object));

	wakeups = UP_WAKEUPS (object);
	if (wakeups->priv->proxy != NULL)
		g_object_unref (wakeups->priv->proxy);

	G_OBJECT_CLASS (up_wakeups_parent_class)->finalize (object);
}

/**
 * up_wakeups_new:
 *
 * Gets a new object to allow querying the wakeups data from the server.
 *
 * Return value: the a new @UpWakeups object.
 *
 * Since: 0.9.1
 **/
UpWakeups *
up_wakeups_new (void)
{
	return UP_WAKEUPS (g_object_new (UP_TYPE_WAKEUPS, NULL));
}

