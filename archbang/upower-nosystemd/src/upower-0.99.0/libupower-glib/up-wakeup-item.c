/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008-2010 Richard Hughes <richard@hughsie.com>
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

/**
 * SECTION:up-wakeup-item
 * @short_description: Helper object representing one item of wakeup data.
 *
 * This object represents one item of data which may be returned from the
 * daemon in response to a query.
 *
 * See also: #UpDevice, #UpClient
 */

#include "config.h"

#include <glib.h>

#include "up-wakeup-item.h"

static void	up_wakeup_item_class_init	(UpWakeupItemClass	*klass);
static void	up_wakeup_item_init		(UpWakeupItem		*wakeup_item);
static void	up_wakeup_item_finalize		(GObject		*object);

#define UP_WAKEUP_ITEM_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), UP_TYPE_WAKEUP_ITEM, UpWakeupItemPrivate))

struct UpWakeupItemPrivate
{
	gboolean		 is_userspace;
	guint			 id;
	guint			 old;
	gdouble			 value;
	gchar			*cmdline;
	gchar			*details;
};

enum {
	PROP_0,
	PROP_IS_USERSPACE,
	PROP_ID,
	PROP_OLD,
	PROP_VALUE,
	PROP_CMDLINE,
	PROP_DETAILS,
	PROP_LAST
};

G_DEFINE_TYPE (UpWakeupItem, up_wakeup_item, G_TYPE_OBJECT)

/**
 * up_wakeup_item_get_is_userspace:
 * @wakeup_item: #UpWakeupItem
 *
 * Gets if the item is userspace.
 *
 * Return value: the value
 *
 * Since: 0.9.0
 **/
gboolean
up_wakeup_item_get_is_userspace (UpWakeupItem *wakeup_item)
{
	g_return_val_if_fail (UP_IS_WAKEUP_ITEM (wakeup_item), FALSE);
	return wakeup_item->priv->is_userspace;
}

/**
 * up_wakeup_item_set_is_userspace:
 * @wakeup_item: #UpWakeupItem
 * @is_userspace: the new value
 *
 * Sets if the item is userspace.
 *
 * Since: 0.9.0
 **/
void
up_wakeup_item_set_is_userspace (UpWakeupItem *wakeup_item, gboolean is_userspace)
{
	g_return_if_fail (UP_IS_WAKEUP_ITEM (wakeup_item));
	wakeup_item->priv->is_userspace = is_userspace;
	g_object_notify (G_OBJECT(wakeup_item), "is-userspace");
}

/**
 * up_wakeup_item_get_id:
 * @wakeup_item: #UpWakeupItem
 *
 * Gets the item id.
 *
 * Return value: the value
 *
 * Since: 0.9.0
 **/
guint
up_wakeup_item_get_id (UpWakeupItem *wakeup_item)
{
	g_return_val_if_fail (UP_IS_WAKEUP_ITEM (wakeup_item), G_MAXUINT);
	return wakeup_item->priv->id;
}

/**
 * up_wakeup_item_set_id:
 * @wakeup_item: #UpWakeupItem
 * @id: the new value
 *
 * Sets the item id.
 *
 * Since: 0.9.0
 **/
void
up_wakeup_item_set_id (UpWakeupItem *wakeup_item, guint id)
{
	g_return_if_fail (UP_IS_WAKEUP_ITEM (wakeup_item));
	wakeup_item->priv->id = id;
	g_object_notify (G_OBJECT(wakeup_item), "id");
}

/**
 * up_wakeup_item_get_old:
 * @wakeup_item: #UpWakeupItem
 *
 * Gets the item old.
 *
 * Return value: the value
 *
 * Since: 0.9.0
 **/
guint
up_wakeup_item_get_old (UpWakeupItem *wakeup_item)
{
	g_return_val_if_fail (UP_IS_WAKEUP_ITEM (wakeup_item), G_MAXUINT);
	return wakeup_item->priv->old;
}

/**
 * up_wakeup_item_set_old:
 * @wakeup_item: #UpWakeupItem
 * @old: the new value
 *
 * Sets the item old.
 *
 * Since: 0.9.0
 **/
void
up_wakeup_item_set_old (UpWakeupItem *wakeup_item, guint old)
{
	g_return_if_fail (UP_IS_WAKEUP_ITEM (wakeup_item));
	wakeup_item->priv->old = old;
	g_object_notify (G_OBJECT(wakeup_item), "old");
}

/**
 * up_wakeup_item_get_value:
 * @wakeup_item: #UpWakeupItem
 *
 * Gets the item value.
 *
 * Return value: the value
 *
 * Since: 0.9.0
 **/
gdouble
up_wakeup_item_get_value (UpWakeupItem *wakeup_item)
{
	g_return_val_if_fail (UP_IS_WAKEUP_ITEM (wakeup_item), G_MAXDOUBLE);
	return wakeup_item->priv->value;
}

/**
 * up_wakeup_item_set_value:
 * @wakeup_item: #UpWakeupItem
 * @value: the new value
 *
 * Sets the item value.
 *
 * Since: 0.9.0
 **/
void
up_wakeup_item_set_value (UpWakeupItem *wakeup_item, gdouble value)
{
	g_return_if_fail (UP_IS_WAKEUP_ITEM (wakeup_item));
	wakeup_item->priv->value = value;
	g_object_notify (G_OBJECT(wakeup_item), "value");
}

/**
 * up_wakeup_item_get_cmdline:
 * @wakeup_item: #UpWakeupItem
 *
 * Gets the item cmdline.
 *
 * Return value: the value
 *
 * Since: 0.9.0
 **/
const gchar *
up_wakeup_item_get_cmdline (UpWakeupItem *wakeup_item)
{
	g_return_val_if_fail (UP_IS_WAKEUP_ITEM (wakeup_item), NULL);
	return wakeup_item->priv->cmdline;
}

/**
 * up_wakeup_item_set_cmdline:
 * @wakeup_item: #UpWakeupItem
 * @cmdline: the new value
 *
 * Sets the item cmdline.
 *
 * Since: 0.9.0
 **/
void
up_wakeup_item_set_cmdline (UpWakeupItem *wakeup_item, const gchar *cmdline)
{
	g_return_if_fail (UP_IS_WAKEUP_ITEM (wakeup_item));
	g_free (wakeup_item->priv->cmdline);
	wakeup_item->priv->cmdline = g_strdup (cmdline);
	g_object_notify (G_OBJECT(wakeup_item), "cmdline");
}

/**
 * up_wakeup_item_get_details:
 * @wakeup_item: #UpWakeupItem
 *
 * Gets the item details.
 *
 * Return value: the value
 *
 * Since: 0.9.0
 **/
const gchar *
up_wakeup_item_get_details (UpWakeupItem *wakeup_item)
{
	g_return_val_if_fail (UP_IS_WAKEUP_ITEM (wakeup_item), NULL);
	return wakeup_item->priv->details;
}

/**
 * up_wakeup_item_set_details:
 * @wakeup_item: #UpWakeupItem
 * @details: the new value
 *
 * Sets the item details.
 *
 * Since: 0.9.0
 **/
void
up_wakeup_item_set_details (UpWakeupItem *wakeup_item, const gchar *details)
{
	g_return_if_fail (UP_IS_WAKEUP_ITEM (wakeup_item));
	g_free (wakeup_item->priv->details);
	wakeup_item->priv->details = g_strdup (details);
	g_object_notify (G_OBJECT(wakeup_item), "details");
}

/**
 * up_wakeup_item_set_property:
 **/
static void
up_wakeup_item_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	UpWakeupItem *wakeup_item = UP_WAKEUP_ITEM (object);

	switch (prop_id) {
	case PROP_IS_USERSPACE:
		wakeup_item->priv->is_userspace = g_value_get_boolean (value);
		break;
	case PROP_ID:
		wakeup_item->priv->id = g_value_get_uint (value);
		break;
	case PROP_OLD:
		wakeup_item->priv->old = g_value_get_uint (value);
		break;
	case PROP_VALUE:
		wakeup_item->priv->value = g_value_get_double (value);
		break;
	case PROP_CMDLINE:
		g_free (wakeup_item->priv->cmdline);
		wakeup_item->priv->cmdline = g_strdup (g_value_get_string (value));
		break;
	case PROP_DETAILS:
		g_free (wakeup_item->priv->details);
		wakeup_item->priv->details = g_strdup (g_value_get_string (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

/**
 * up_wakeup_item_get_property:
 **/
static void
up_wakeup_item_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	UpWakeupItem *wakeup_item = UP_WAKEUP_ITEM (object);

	switch (prop_id) {
	case PROP_IS_USERSPACE:
		g_value_set_boolean (value, wakeup_item->priv->is_userspace);
		break;
	case PROP_ID:
		g_value_set_uint (value, wakeup_item->priv->id);
		break;
	case PROP_OLD:
		g_value_set_uint (value, wakeup_item->priv->old);
		break;
	case PROP_VALUE:
		g_value_set_double (value, wakeup_item->priv->value);
		break;
	case PROP_CMDLINE:
		g_value_set_string (value, wakeup_item->priv->cmdline);
		break;
	case PROP_DETAILS:
		g_value_set_string (value, wakeup_item->priv->details);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
    }
}

/**
 * up_wakeup_item_class_init:
 * @klass: The UpWakeupItemClass
 **/
static void
up_wakeup_item_class_init (UpWakeupItemClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = up_wakeup_item_finalize;
	object_class->set_property = up_wakeup_item_set_property;
	object_class->get_property = up_wakeup_item_get_property;

	/**
	 * UpWakeupItem:is-userspace:
	 *
	 * Since: 0.9.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_IS_USERSPACE,
					 g_param_spec_boolean ("is-userspace", NULL, NULL,
							       FALSE,
							       G_PARAM_READWRITE));
	/**
	 * UpWakeupItem:id:
	 *
	 * Since: 0.9.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_ID,
					 g_param_spec_uint ("id", NULL, NULL,
							    0, G_MAXUINT, 0,
							    G_PARAM_READWRITE));
	/**
	 * UpWakeupItem:old:
	 *
	 * Since: 0.9.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_OLD,
					 g_param_spec_uint ("old", NULL, NULL,
							    0, G_MAXUINT, 0,
							    G_PARAM_READWRITE));
	/**
	 * UpWakeupItem:value:
	 *
	 * Since: 0.9.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_VALUE,
					 g_param_spec_double ("value", NULL, NULL,
							      0.0, G_MAXDOUBLE, 0.0,
							      G_PARAM_READWRITE));
	/**
	 * UpWakeupItem:cmdline:
	 *
	 * Since: 0.9.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_CMDLINE,
					 g_param_spec_string ("cmdline", NULL, NULL,
							      NULL,
							      G_PARAM_READWRITE));
	/**
	 * UpWakeupItem:details:
	 *
	 * Since: 0.9.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_DETAILS,
					 g_param_spec_string ("details", NULL, NULL,
							      NULL,
							      G_PARAM_READWRITE));

	g_type_class_add_private (klass, sizeof (UpWakeupItemPrivate));
}

/**
 * up_wakeup_item_init:
 * @wakeup_item: This class instance
 **/
static void
up_wakeup_item_init (UpWakeupItem *wakeup_item)
{
	wakeup_item->priv = UP_WAKEUP_ITEM_GET_PRIVATE (wakeup_item);
}

/**
 * up_wakeup_item_finalize:
 * @object: The object to finalize
 **/
static void
up_wakeup_item_finalize (GObject *object)
{
	UpWakeupItem *wakeup_item;

	g_return_if_fail (UP_IS_WAKEUP_ITEM (object));

	wakeup_item = UP_WAKEUP_ITEM (object);
	g_free (wakeup_item->priv->cmdline);
	g_free (wakeup_item->priv->details);

	G_OBJECT_CLASS (up_wakeup_item_parent_class)->finalize (object);
}

/**
 * up_wakeup_item_new:
 *
 * Return value: a new UpWakeupItem object.
 *
 * Since: 0.9.0
 **/
UpWakeupItem *
up_wakeup_item_new (void)
{
	return UP_WAKEUP_ITEM (g_object_new (UP_TYPE_WAKEUP_ITEM, NULL));
}

