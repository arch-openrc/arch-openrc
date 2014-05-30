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
 * SECTION:up-stats-item
 * @short_description: Helper object representing one item of statistics data.
 *
 * This object represents one item of data which may be returned from the
 * daemon in response to a query.
 *
 * See also: #UpDevice, #UpClient
 */

#include "config.h"

#include <glib.h>

#include "up-stats-item.h"

static void	up_stats_item_class_init	(UpStatsItemClass	*klass);
static void	up_stats_item_init		(UpStatsItem		*stats_item);
static void	up_stats_item_finalize		(GObject		*object);

#define UP_STATS_ITEM_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), UP_TYPE_STATS_ITEM, UpStatsItemPrivate))

struct UpStatsItemPrivate
{
	gdouble			 value;
	gdouble			 accuracy;
};

enum {
	PROP_0,
	PROP_VALUE,
	PROP_ACCURACY,
	PROP_LAST
};

G_DEFINE_TYPE (UpStatsItem, up_stats_item, G_TYPE_OBJECT)

/**
 * up_stats_item_set_value:
 *
 * Sets the item value.
 *
 * Since: 0.9.0
 **/
void
up_stats_item_set_value (UpStatsItem *stats_item, gdouble value)
{
	g_return_if_fail (UP_IS_STATS_ITEM (stats_item));
	stats_item->priv->value = value;
	g_object_notify (G_OBJECT(stats_item), "value");
}

/**
 * up_stats_item_get_value:
 *
 * Gets the item value.
 *
 * Since: 0.9.0
 **/
gdouble
up_stats_item_get_value (UpStatsItem *stats_item)
{
	g_return_val_if_fail (UP_IS_STATS_ITEM (stats_item), G_MAXDOUBLE);
	return stats_item->priv->value;
}

/**
 * up_stats_item_set_accuracy:
 *
 * Sets the item accuracy.
 *
 * Since: 0.9.0
 **/
void
up_stats_item_set_accuracy (UpStatsItem *stats_item, gdouble accuracy)
{
	g_return_if_fail (UP_IS_STATS_ITEM (stats_item));

	/* constrain */
	if (accuracy < 0.0f)
		accuracy = 0.0f;
	else if (accuracy > 100.0f)
		accuracy = 100.0f;
	stats_item->priv->accuracy = accuracy;
	g_object_notify (G_OBJECT(stats_item), "accuracy");
}

/**
 * up_stats_item_get_accuracy:
 *
 * Gets the item accuracy.
 *
 * Since: 0.9.0
 **/
gdouble
up_stats_item_get_accuracy (UpStatsItem *stats_item)
{
	g_return_val_if_fail (UP_IS_STATS_ITEM (stats_item), G_MAXDOUBLE);
	return stats_item->priv->accuracy;
}

/**
 * up_stats_item_set_property:
 **/
static void
up_stats_item_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	UpStatsItem *stats_item = UP_STATS_ITEM (object);

	switch (prop_id) {
	case PROP_VALUE:
		stats_item->priv->value = g_value_get_double (value);
		break;
	case PROP_ACCURACY:
		stats_item->priv->accuracy = g_value_get_double (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

/**
 * up_stats_item_get_property:
 **/
static void
up_stats_item_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	UpStatsItem *stats_item = UP_STATS_ITEM (object);

	switch (prop_id) {
	case PROP_VALUE:
		g_value_set_double (value, stats_item->priv->value);
		break;
	case PROP_ACCURACY:
		g_value_set_double (value, stats_item->priv->accuracy);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
    }
}

/**
 * up_stats_item_class_init:
 * @klass: The UpStatsItemClass
 **/
static void
up_stats_item_class_init (UpStatsItemClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = up_stats_item_finalize;
	object_class->set_property = up_stats_item_set_property;
	object_class->get_property = up_stats_item_get_property;

	/**
	 * UpStatsItem:value:
	 *
	 * Since: 0.9.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_VALUE,
					 g_param_spec_double ("value", NULL, NULL,
							      0.0, G_MAXDOUBLE, 0.0,
							      G_PARAM_READWRITE));
	/**
	 * UpStatsItem:accuracy:
	 *
	 * Since: 0.9.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_ACCURACY,
					 g_param_spec_double ("accuracy", NULL, NULL,
							      0.0, G_MAXDOUBLE, 0.0,
							      G_PARAM_READWRITE));

	g_type_class_add_private (klass, sizeof (UpStatsItemPrivate));
}

/**
 * up_stats_item_init:
 * @stats_item: This class instance
 **/
static void
up_stats_item_init (UpStatsItem *stats_item)
{
	stats_item->priv = UP_STATS_ITEM_GET_PRIVATE (stats_item);
}

/**
 * up_stats_item_finalize:
 * @object: The object to finalize
 **/
static void
up_stats_item_finalize (GObject *object)
{
	g_return_if_fail (UP_IS_STATS_ITEM (object));
	G_OBJECT_CLASS (up_stats_item_parent_class)->finalize (object);
}

/**
 * up_stats_item_new:
 *
 * Return value: a new UpStatsItem object.
 *
 * Since: 0.9.0
 **/
UpStatsItem *
up_stats_item_new (void)
{
	return UP_STATS_ITEM (g_object_new (UP_TYPE_STATS_ITEM, NULL));
}

