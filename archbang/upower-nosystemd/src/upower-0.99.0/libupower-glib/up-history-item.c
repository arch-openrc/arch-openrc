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

/**
 * SECTION:up-history-item
 * @short_description: Helper object representing one item of historical data.
 *
 * This object represents one item of data which may be returned from the
 * daemon in response to a query.
 *
 * See also: #UpDevice, #UpClient
 */

#include "config.h"

#include <glib.h>
#include <stdlib.h>

#include "up-history-item.h"

static void	up_history_item_class_init	(UpHistoryItemClass	*klass);
static void	up_history_item_init		(UpHistoryItem		*history_item);
static void	up_history_item_finalize		(GObject		*object);

#define UP_HISTORY_ITEM_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), UP_TYPE_HISTORY_ITEM, UpHistoryItemPrivate))

struct UpHistoryItemPrivate
{
	gdouble			 value;
	guint			 time;
	UpDeviceState		 state;
};

enum {
	PROP_0,
	PROP_VALUE,
	PROP_TIME,
	PROP_STATE,
	PROP_LAST
};

G_DEFINE_TYPE (UpHistoryItem, up_history_item, G_TYPE_OBJECT)

/**
 * up_history_item_set_value:
 * @history_item: #UpHistoryItem
 * @value: the new value
 *
 * Sets the item value.
 *
 * Since: 0.9.0
 **/
void
up_history_item_set_value (UpHistoryItem *history_item, gdouble value)
{
	g_return_if_fail (UP_IS_HISTORY_ITEM (history_item));
	history_item->priv->value = value;
	g_object_notify (G_OBJECT(history_item), "value");
}

/**
 * up_history_item_get_value:
 * @history_item: #UpHistoryItem
 *
 * Gets the item value.
 *
 * Since: 0.9.0
 **/
gdouble
up_history_item_get_value (UpHistoryItem *history_item)
{
	g_return_val_if_fail (UP_IS_HISTORY_ITEM (history_item), G_MAXDOUBLE);
	return history_item->priv->value;
}

/**
 * up_history_item_set_time:
 * @history_item: #UpHistoryItem
 * @time: the new value
 *
 * Sets the item time.
 *
 * Since: 0.9.0
 **/
void
up_history_item_set_time (UpHistoryItem *history_item, guint time)
{
	g_return_if_fail (UP_IS_HISTORY_ITEM (history_item));
	history_item->priv->time = time;
	g_object_notify (G_OBJECT(history_item), "time");
}

/**
 * up_history_item_set_time_to_present:
 * @history_item: #UpHistoryItem
 *
 * Sets the item time to the present value.
 *
 * Since: 0.9.1
 **/
void
up_history_item_set_time_to_present (UpHistoryItem *history_item)
{
	GTimeVal timeval;

	g_return_if_fail (UP_IS_HISTORY_ITEM (history_item));

	g_get_current_time (&timeval);
	history_item->priv->time = timeval.tv_sec;
	g_object_notify (G_OBJECT(history_item), "time");
}

/**
 * up_history_item_get_time:
 * @history_item: #UpHistoryItem
 *
 * Gets the item time.
 *
 * Since: 0.9.0
 **/
guint
up_history_item_get_time (UpHistoryItem *history_item)
{
	g_return_val_if_fail (UP_IS_HISTORY_ITEM (history_item), G_MAXUINT);
	return history_item->priv->time;
}

/**
 * up_history_item_set_state:
 * @history_item: #UpHistoryItem
 * @state: the new value
 *
 * Sets the item state.
 *
 * Since: 0.9.0
 **/
void
up_history_item_set_state (UpHistoryItem *history_item, UpDeviceState state)
{
	g_return_if_fail (UP_IS_HISTORY_ITEM (history_item));
	history_item->priv->state = state;
	g_object_notify (G_OBJECT(history_item), "state");
}

/**
 * up_history_item_get_state:
 * @history_item: #UpHistoryItem
 *
 * Gets the item state.
 *
 * Since: 0.9.0
 **/
UpDeviceState
up_history_item_get_state (UpHistoryItem *history_item)
{
	g_return_val_if_fail (UP_IS_HISTORY_ITEM (history_item), G_MAXUINT);
	return history_item->priv->state;
}

/**
 * up_history_item_to_string:
 * @history_item: #UpHistoryItem
 *
 * Converts the history item to a string representation.
 *
 * Since: 0.9.1
 **/
gchar *
up_history_item_to_string (UpHistoryItem *history_item)
{
	g_return_val_if_fail (UP_IS_HISTORY_ITEM (history_item), NULL);
	return g_strdup_printf ("%i\t%.3f\t%s",
				history_item->priv->time,
				history_item->priv->value,
				up_device_state_to_string (history_item->priv->state));
}

/**
 * up_history_item_set_from_string:
 * @history_item: #UpHistoryItem
 *
 * Converts the history item to a string representation.
 *
 * Since: 0.9.1
 **/
gboolean
up_history_item_set_from_string (UpHistoryItem *history_item, const gchar *text)
{
	gchar **parts = NULL;
	guint length;
	gboolean ret = FALSE;

	g_return_val_if_fail (UP_IS_HISTORY_ITEM (history_item), FALSE);
	g_return_val_if_fail (text != NULL, FALSE);

	/* split by tab */
	parts = g_strsplit (text, "\t", 0);
	length = g_strv_length (parts);
	if (length != 3) {
		g_warning ("invalid string: '%s'", text);
		goto out;
	}

	/* parse */
	up_history_item_set_time (history_item, atoi (parts[0]));
	up_history_item_set_value (history_item, atof (parts[1]));
	up_history_item_set_state (history_item, up_device_state_from_string (parts[2]));

	/* success */
	ret = TRUE;
out:
	g_strfreev (parts);
	return ret;
}

/**
 * up_history_item_set_property:
 **/
static void
up_history_item_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	UpHistoryItem *history_item = UP_HISTORY_ITEM (object);

	switch (prop_id) {
	case PROP_VALUE:
		history_item->priv->value = g_value_get_double (value);
		break;
	case PROP_TIME:
		history_item->priv->time = g_value_get_uint (value);
		break;
	case PROP_STATE:
		history_item->priv->state = g_value_get_uint (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

/**
 * up_history_item_get_property:
 **/
static void
up_history_item_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	UpHistoryItem *history_item = UP_HISTORY_ITEM (object);

	switch (prop_id) {
	case PROP_VALUE:
		g_value_set_double (value, history_item->priv->value);
		break;
	case PROP_TIME:
		g_value_set_uint (value, history_item->priv->time);
		break;
	case PROP_STATE:
		g_value_set_uint (value, history_item->priv->state);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
    }
}

/**
 * up_history_item_class_init:
 **/
static void
up_history_item_class_init (UpHistoryItemClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = up_history_item_finalize;
	object_class->set_property = up_history_item_set_property;
	object_class->get_property = up_history_item_get_property;

	/**
	 * UpHistoryItem:value:
	 *
	 * Since: 0.9.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_VALUE,
					 g_param_spec_double ("value", NULL, NULL,
							      0.0, G_MAXDOUBLE, 0.0,
							      G_PARAM_READWRITE));
	/**
	 * UpHistoryItem:time:
	 *
	 * Since: 0.9.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_TIME,
					 g_param_spec_uint ("time", NULL, NULL,
							    0, G_MAXUINT, 0,
							    G_PARAM_READWRITE));

	/**
	 * UpHistoryItem:state:
	 *
	 * Since: 0.9.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_STATE,
					 g_param_spec_uint ("state", NULL, NULL,
							    0, G_MAXUINT,
							    UP_DEVICE_STATE_UNKNOWN,
							    G_PARAM_READWRITE));

	g_type_class_add_private (klass, sizeof (UpHistoryItemPrivate));
}

/**
 * up_history_item_init:
 **/
static void
up_history_item_init (UpHistoryItem *history_item)
{
	history_item->priv = UP_HISTORY_ITEM_GET_PRIVATE (history_item);
}

/**
 * up_history_item_finalize:
 **/
static void
up_history_item_finalize (GObject *object)
{
	g_return_if_fail (UP_IS_HISTORY_ITEM (object));
	G_OBJECT_CLASS (up_history_item_parent_class)->finalize (object);
}

/**
 * up_history_item_new:
 *
 * Return value: a new UpHistoryItem object.
 *
 * Since: 0.9.0
 **/
UpHistoryItem *
up_history_item_new (void)
{
	return UP_HISTORY_ITEM (g_object_new (UP_TYPE_HISTORY_ITEM, NULL));
}

