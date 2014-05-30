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
 * SECTION:up-device
 * @short_description: Client object for accessing information about UPower devices
 *
 * A helper GObject to use for accessing UPower devices, and to be notified
 * when it is changed.
 *
 * See also: #UpClient
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <glib-object.h>
#include <string.h>

#include "up-device.h"
#include "up-device-glue.h"
#include "up-stats-item.h"
#include "up-history-item.h"

static void	up_device_class_init	(UpDeviceClass	*klass);
static void	up_device_init		(UpDevice	*device);
static void	up_device_finalize	(GObject		*object);

#define UP_DEVICE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), UP_TYPE_DEVICE, UpDevicePrivate))

/**
 * UpDevicePrivate:
 *
 * Private #PkDevice data
 **/
struct _UpDevicePrivate
{
	UpDeviceGlue		*proxy_device;

	/* For use when a UpDevice isn't backed by a D-Bus object
	 * by the UPower daemon */
	GHashTable		*offline_props;
};

enum {
	PROP_0,
	PROP_UPDATE_TIME,
	PROP_VENDOR,
	PROP_MODEL,
	PROP_SERIAL,
	PROP_NATIVE_PATH,
	PROP_POWER_SUPPLY,
	PROP_ONLINE,
	PROP_IS_PRESENT,
	PROP_IS_RECHARGEABLE,
	PROP_HAS_HISTORY,
	PROP_HAS_STATISTICS,
	PROP_KIND,
	PROP_STATE,
	PROP_TECHNOLOGY,
	PROP_CAPACITY,
	PROP_ENERGY,
	PROP_ENERGY_EMPTY,
	PROP_ENERGY_FULL,
	PROP_ENERGY_FULL_DESIGN,
	PROP_ENERGY_RATE,
	PROP_VOLTAGE,
	PROP_LUMINOSITY,
	PROP_TIME_TO_EMPTY,
	PROP_TIME_TO_FULL,
	PROP_PERCENTAGE,
	PROP_TEMPERATURE,
	PROP_WARNING_LEVEL,
	PROP_ICON_NAME,
	PROP_LAST
};

G_DEFINE_TYPE (UpDevice, up_device, G_TYPE_OBJECT)

/*
 * up_device_changed_cb:
 */
static void
up_device_changed_cb (UpDeviceGlue *proxy, GParamSpec *pspec, UpDevice *device)
{
	if (g_strcmp0 (pspec->name, "type") == 0)
		g_object_notify (G_OBJECT (device), "kind");
	else
		g_object_notify (G_OBJECT (device), pspec->name);
}

/**
 * up_device_set_object_path_sync:
 * @device: a #UpDevice instance.
 * @object_path: The UPower object path.
 * @cancellable: a #GCancellable or %NULL
 * @error: a #GError, or %NULL.
 *
 * Sets the object path of the object and fills up initial properties.
 *
 * Return value: #TRUE for success, else #FALSE and @error is used
 *
 * Since: 0.9.0
 **/
gboolean
up_device_set_object_path_sync (UpDevice *device, const gchar *object_path, GCancellable *cancellable, GError **error)
{
	UpDeviceGlue *proxy_device;

	g_return_val_if_fail (UP_IS_DEVICE (device), FALSE);
	g_return_val_if_fail (object_path != NULL, FALSE);

	if (device->priv->proxy_device != NULL)
		return FALSE;

	g_clear_pointer (&device->priv->offline_props, g_hash_table_unref);

	/* connect to the correct path for all the other methods */
	proxy_device = up_device_glue_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
							      G_DBUS_PROXY_FLAGS_NONE,
							      "org.freedesktop.UPower",
							      object_path,
							      cancellable,
							      error);
	if (proxy_device == NULL)
		return FALSE;

	/* listen to Changed */
	g_signal_connect (proxy_device, "notify",
			  G_CALLBACK (up_device_changed_cb), device);

	/* yay */
	device->priv->proxy_device = proxy_device;

	return TRUE;
}

/**
 * up_device_get_object_path:
 * @device: a #UpDevice instance.
 *
 * Gets the object path for the device.
 *
 * Return value: the object path, or %NULL
 *
 * Since: 0.9.0
 **/
const gchar *
up_device_get_object_path (UpDevice *device)
{
	g_return_val_if_fail (UP_IS_DEVICE (device), NULL);
	return g_dbus_proxy_get_object_path (G_DBUS_PROXY (device->priv->proxy_device));
}

/*
 * up_device_to_text_history:
 */
static void
up_device_to_text_history (UpDevice *device, GString *string, const gchar *type)
{
	guint i;
	GPtrArray *array;
	UpHistoryItem *item;

	/* get a fair chunk of data */
	array = up_device_get_history_sync (device, type, 120, 10, NULL, NULL);
	if (array == NULL)
		return;

	/* pretty print */
	g_string_append_printf (string, "  History (%s):\n", type);
	for (i=0; i<array->len; i++) {
		item = (UpHistoryItem *) g_ptr_array_index (array, i);
		g_string_append_printf (string, "    %i\t%.3f\t%s\n",
				 up_history_item_get_time (item),
				 up_history_item_get_value (item),
				 up_device_state_to_string (up_history_item_get_state (item)));
	}
	g_ptr_array_unref (array);
}

/*
 * up_device_bool_to_string:
 */
static const gchar *
up_device_bool_to_string (gboolean ret)
{
	return ret ? "yes" : "no";
}

/*
 * up_device_to_text_time_to_string:
 */
static gchar *
up_device_to_text_time_to_string (gint seconds)
{
	gfloat value = seconds;

	if (value < 0)
		return g_strdup ("unknown");
	if (value < 60)
		return g_strdup_printf ("%.0f seconds", value);
	value /= 60.0;
	if (value < 60)
		return g_strdup_printf ("%.1f minutes", value);
	value /= 60.0;
	if (value < 60)
		return g_strdup_printf ("%.1f hours", value);
	value /= 24.0;
	return g_strdup_printf ("%.1f days", value);
}

/**
 * up_device_to_text:
 * @device: a #UpDevice instance.
 *
 * Converts the device to a string description.
 *
 * Return value: text representation of #UpDevice
 *
 * Since: 0.9.0
 **/
gchar *
up_device_to_text (UpDevice *device)
{
	struct tm *time_tm;
	time_t t;
	gchar time_buf[256];
	gchar *time_str;
	GString *string;
	UpDevicePrivate *priv;
	const gchar *vendor;
	const gchar *model;
	const gchar *serial;
	UpDeviceKind kind;
	gboolean is_display;

	g_return_val_if_fail (UP_IS_DEVICE (device), NULL);
	g_return_val_if_fail (device->priv->proxy_device != NULL, NULL);

	priv = device->priv;

	is_display = (g_strcmp0 ("/org/freedesktop/UPower/devices/DisplayDevice", up_device_get_object_path (device)) == 0);

	/* get a human readable time */
	t = (time_t) up_device_glue_get_update_time (priv->proxy_device);
	time_tm = localtime (&t);
	strftime (time_buf, sizeof time_buf, "%c", time_tm);

	string = g_string_new ("");
	if (!is_display)
		g_string_append_printf (string, "  native-path:          %s\n", up_device_glue_get_native_path (priv->proxy_device));
	vendor = up_device_glue_get_vendor (priv->proxy_device);
	if (vendor != NULL && vendor[0] != '\0')
		g_string_append_printf (string, "  vendor:               %s\n", vendor);
	model = up_device_glue_get_model (priv->proxy_device);
	if (model != NULL && model[0] != '\0')
		g_string_append_printf (string, "  model:                %s\n", model);
	serial = up_device_glue_get_serial (priv->proxy_device);
	if (serial != NULL && serial[0] != '\0')
		g_string_append_printf (string, "  serial:               %s\n", serial);
	g_string_append_printf (string, "  power supply:         %s\n", up_device_bool_to_string (up_device_glue_get_power_supply (priv->proxy_device)));
	g_string_append_printf (string, "  updated:              %s (%d seconds ago)\n", time_buf, (int) (time (NULL) - up_device_glue_get_update_time (priv->proxy_device)));
	g_string_append_printf (string, "  has history:          %s\n", up_device_bool_to_string (up_device_glue_get_has_history (priv->proxy_device)));
	g_string_append_printf (string, "  has statistics:       %s\n", up_device_bool_to_string (up_device_glue_get_has_statistics (priv->proxy_device)));

	kind = up_device_glue_get_type_ (priv->proxy_device);
	g_string_append_printf (string, "  %s\n", up_device_kind_to_string (kind));

	if (kind == UP_DEVICE_KIND_BATTERY ||
	    kind == UP_DEVICE_KIND_MOUSE ||
	    kind == UP_DEVICE_KIND_KEYBOARD ||
	    kind == UP_DEVICE_KIND_UPS)
		g_string_append_printf (string, "    present:             %s\n", up_device_bool_to_string (up_device_glue_get_is_present (priv->proxy_device)));
	if ((kind == UP_DEVICE_KIND_PHONE ||
	     kind == UP_DEVICE_KIND_BATTERY ||
	     kind == UP_DEVICE_KIND_MOUSE ||
	     kind == UP_DEVICE_KIND_KEYBOARD) &&
	    !is_display)
		g_string_append_printf (string, "    rechargeable:        %s\n", up_device_bool_to_string (up_device_glue_get_is_rechargeable (priv->proxy_device)));
	if (kind == UP_DEVICE_KIND_BATTERY ||
	    kind == UP_DEVICE_KIND_MOUSE ||
	    kind == UP_DEVICE_KIND_KEYBOARD ||
	    kind == UP_DEVICE_KIND_UPS)
		g_string_append_printf (string, "    state:               %s\n", up_device_state_to_string (up_device_glue_get_state (priv->proxy_device)));
	g_string_append_printf (string, "    warning-level:       %s\n", up_device_level_to_string (up_device_glue_get_warning_level (priv->proxy_device)));
	if (kind == UP_DEVICE_KIND_BATTERY) {
		g_string_append_printf (string, "    energy:              %g Wh\n", up_device_glue_get_energy (priv->proxy_device));
		if (!is_display)
			g_string_append_printf (string, "    energy-empty:        %g Wh\n", up_device_glue_get_energy_empty (priv->proxy_device));
		g_string_append_printf (string, "    energy-full:         %g Wh\n", up_device_glue_get_energy_full (priv->proxy_device));
		if (!is_display)
			g_string_append_printf (string, "    energy-full-design:  %g Wh\n", up_device_glue_get_energy_full_design (priv->proxy_device));
	}
	if (kind == UP_DEVICE_KIND_BATTERY ||
	    kind == UP_DEVICE_KIND_MONITOR)
		g_string_append_printf (string, "    energy-rate:         %g W\n", up_device_glue_get_energy_rate (priv->proxy_device));
	if (kind == UP_DEVICE_KIND_UPS ||
	    kind == UP_DEVICE_KIND_BATTERY ||
	    kind == UP_DEVICE_KIND_MONITOR) {
		if (up_device_glue_get_voltage (priv->proxy_device) > 0)
			g_string_append_printf (string, "    voltage:             %g V\n", up_device_glue_get_voltage (priv->proxy_device));
	}
	if (kind == UP_DEVICE_KIND_KEYBOARD) {
		if (up_device_glue_get_luminosity (priv->proxy_device) > 0)
			g_string_append_printf (string, "    luminosity:          %g lx\n", up_device_glue_get_luminosity (priv->proxy_device));
	}
	if (kind == UP_DEVICE_KIND_BATTERY ||
	    kind == UP_DEVICE_KIND_UPS) {
		if (up_device_glue_get_time_to_full (priv->proxy_device) > 0) {
			time_str = up_device_to_text_time_to_string (up_device_glue_get_time_to_full (priv->proxy_device));
			g_string_append_printf (string, "    time to full:        %s\n", time_str);
			g_free (time_str);
		}
		if (up_device_glue_get_time_to_empty (priv->proxy_device) > 0) {
			time_str = up_device_to_text_time_to_string (up_device_glue_get_time_to_empty (priv->proxy_device));
			g_string_append_printf (string, "    time to empty:       %s\n", time_str);
			g_free (time_str);
		}
	}
	if (kind == UP_DEVICE_KIND_BATTERY ||
	    kind == UP_DEVICE_KIND_MOUSE ||
	    kind == UP_DEVICE_KIND_KEYBOARD ||
	    kind == UP_DEVICE_KIND_PHONE ||
	    kind == UP_DEVICE_KIND_TABLET ||
	    kind == UP_DEVICE_KIND_COMPUTER ||
	    kind == UP_DEVICE_KIND_MEDIA_PLAYER ||
	    kind == UP_DEVICE_KIND_UPS)
		g_string_append_printf (string, "    percentage:          %g%%\n", up_device_glue_get_percentage (priv->proxy_device));
	if (kind == UP_DEVICE_KIND_BATTERY) {
		if (up_device_glue_get_temperature (priv->proxy_device) > 0)
			g_string_append_printf (string, "    temperature:         %g degrees C\n", up_device_glue_get_temperature (priv->proxy_device));
		if (up_device_glue_get_capacity (priv->proxy_device) > 0)
			g_string_append_printf (string, "    capacity:            %g%%\n", up_device_glue_get_capacity (priv->proxy_device));
	}
	if (kind == UP_DEVICE_KIND_BATTERY) {
		if (up_device_glue_get_technology (priv->proxy_device) != UP_DEVICE_TECHNOLOGY_UNKNOWN)
			g_string_append_printf (string, "    technology:          %s\n", up_device_technology_to_string (up_device_glue_get_technology (priv->proxy_device)));
	}
	if (kind == UP_DEVICE_KIND_LINE_POWER)
		g_string_append_printf (string, "    online:              %s\n", up_device_bool_to_string (up_device_glue_get_online (priv->proxy_device)));

	g_string_append_printf (string, "    icon-name:          '%s'\n", up_device_glue_get_icon_name (priv->proxy_device));

	/* if we can, get history */
	if (up_device_glue_get_has_history (priv->proxy_device)) {
		up_device_to_text_history (device, string, "charge");
		up_device_to_text_history (device, string, "rate");
	}

	return g_string_free (string, FALSE);
}

/**
 * up_device_refresh_sync:
 * @device: a #UpDevice instance.
 * @cancellable: a #GCancellable or %NULL
 * @error: a #GError, or %NULL.
 *
 * Refreshes properties on the device.
 * This function is normally not required.
 *
 * Return value: #TRUE for success, else #FALSE and @error is used
 *
 * Since: 0.9.0
 **/
gboolean
up_device_refresh_sync (UpDevice *device, GCancellable *cancellable, GError **error)
{
	g_return_val_if_fail (UP_IS_DEVICE (device), FALSE);
	g_return_val_if_fail (device->priv->proxy_device != NULL, FALSE);

	return up_device_glue_call_refresh_sync (device->priv->proxy_device, cancellable, error);
}

/**
 * up_device_get_history_sync:
 * @device: a #UpDevice instance.
 * @type: The type of history, known values are "rate" and "charge".
 * @timespec: the amount of time to look back into time.
 * @resolution: the resolution of data.
 * @cancellable: a #GCancellable or %NULL
 * @error: a #GError, or %NULL.
 *
 * Gets the device history.
 *
 * Return value: (element-type UpHistoryItem) (transfer full): an array of #UpHistoryItem's, with the most
 *               recent one being first; %NULL if @error is set or @device is
 *               invalid
 *
 * Since: 0.9.0
 **/
GPtrArray *
up_device_get_history_sync (UpDevice *device, const gchar *type, guint timespec, guint resolution, GCancellable *cancellable, GError **error)
{
	GError *error_local = NULL;
	GVariant *gva;
	guint i;
	GPtrArray *array = NULL;
	gboolean ret;
	gsize len;
	GVariantIter *iter;

	g_return_val_if_fail (UP_IS_DEVICE (device), NULL);
	g_return_val_if_fail (device->priv->proxy_device != NULL, NULL);

	/* get compound data */
	ret = up_device_glue_call_get_history_sync (device->priv->proxy_device,
						    type,
						    timespec,
						    resolution,
						    &gva,
						    NULL,
						    &error_local);
	if (!ret) {
		g_set_error (error, 1, 0, "GetHistory(%s,%i) on %s failed: %s", type, timespec,
			     up_device_get_object_path (device), error_local->message);
		g_error_free (error_local);
		goto out;
	}

	iter = g_variant_iter_new (gva);
	len = g_variant_iter_n_children (iter);

	/* no data */
	if (len == 0) {
		g_set_error_literal (error, 1, 0, "no data");
		g_variant_iter_free (iter);
		goto out;
	}

	/* convert */
	array = g_ptr_array_new_with_free_func ((GDestroyNotify) g_object_unref);
	for (i = 0; i < len; i++) {
		UpHistoryItem *obj;
		GVariant *v;
		gdouble value;
		guint32 time, state;

		v = g_variant_iter_next_value (iter);
		g_variant_get (v, "(udu)",
			       &time, &value, &state);
		g_variant_unref (v);

		obj = up_history_item_new ();
		up_history_item_set_time (obj, time);
		up_history_item_set_value (obj, value);
		up_history_item_set_state (obj, state);

		g_ptr_array_add (array, obj);
	}
	g_variant_iter_free (iter);

out:
	if (gva != NULL)
		g_variant_unref (gva);
	return array;
}

/**
 * up_device_get_statistics_sync:
 * @device: a #UpDevice instance.
 * @type: the type of statistics.
 * @cancellable: a #GCancellable or %NULL
 * @error: a #GError, or %NULL.
 *
 * Gets the device current statistics.
 *
 * Return value: (element-type UpStatsItem) (transfer full): an array of #UpStatsItem's, else #NULL and @error is used
 *
 * Since: 0.9.0
 **/
GPtrArray *
up_device_get_statistics_sync (UpDevice *device, const gchar *type, GCancellable *cancellable, GError **error)
{
	GError *error_local = NULL;
	GVariant *gva;
	guint i;
	GPtrArray *array = NULL;
	gboolean ret;
	gsize len;
	GVariantIter *iter;

	g_return_val_if_fail (UP_IS_DEVICE (device), NULL);
	g_return_val_if_fail (device->priv->proxy_device != NULL, NULL);

	/* get compound data */
	ret = up_device_glue_call_get_statistics_sync (device->priv->proxy_device,
						       type,
						       &gva,
						       NULL,
						       &error_local);
	if (!ret) {
		g_set_error (error, 1, 0, "GetStatistics(%s) on %s failed: %s", type,
				      up_device_get_object_path (device), error_local->message);
		g_error_free (error_local);
		goto out;
	}

	iter = g_variant_iter_new (gva);
	len = g_variant_iter_n_children (iter);

	/* no data */
	if (len == 0) {
		g_set_error_literal (error, 1, 0, "no data");
		g_variant_iter_free (iter);
		goto out;
	}

	/* convert */
	array = g_ptr_array_new_with_free_func ((GDestroyNotify) g_object_unref);
	for (i = 0; i < len; i++) {
		UpStatsItem *obj;
		GVariant *v;
		gdouble value, accuracy;

		v = g_variant_iter_next_value (iter);
		g_variant_get (v, "(dd)",
			       &value, &accuracy);
		g_variant_unref (v);

		obj = up_stats_item_new ();
		up_stats_item_set_value (obj, value);
		up_stats_item_set_accuracy (obj, accuracy);

		g_ptr_array_add (array, obj);
	}
	g_variant_iter_free (iter);

out:
	if (gva != NULL)
		g_variant_unref (gva);
	return array;
}

/*
 * up_device_set_property:
 */
static void
up_device_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	UpDevice *device = UP_DEVICE (object);

	if (device->priv->proxy_device == NULL) {
		GValue *v;

		v = g_slice_new0 (GValue);
		g_value_init (v, G_VALUE_TYPE (value));
		g_value_copy (value, v);
		g_hash_table_insert (device->priv->offline_props, GUINT_TO_POINTER (prop_id), v);

		return;
	}

	switch (prop_id) {
	case PROP_NATIVE_PATH:
		up_device_glue_set_native_path (device->priv->proxy_device, g_value_get_string (value));
		break;
	case PROP_VENDOR:
		up_device_glue_set_vendor (device->priv->proxy_device, g_value_get_string (value));
		break;
	case PROP_MODEL:
		up_device_glue_set_model (device->priv->proxy_device, g_value_get_string (value));
		break;
	case PROP_SERIAL:
		up_device_glue_set_serial (device->priv->proxy_device, g_value_get_string (value));
		break;
	case PROP_UPDATE_TIME:
		up_device_glue_set_update_time (device->priv->proxy_device, g_value_get_uint64 (value));
		break;
	case PROP_KIND:
		up_device_glue_set_type_ (device->priv->proxy_device, g_value_get_uint (value));
		break;
	case PROP_POWER_SUPPLY:
		up_device_glue_set_power_supply (device->priv->proxy_device, g_value_get_boolean (value));
		break;
	case PROP_ONLINE:
		up_device_glue_set_online (device->priv->proxy_device, g_value_get_boolean (value));
		break;
	case PROP_IS_PRESENT:
		up_device_glue_set_is_present (device->priv->proxy_device, g_value_get_boolean (value));
		break;
	case PROP_IS_RECHARGEABLE:
		up_device_glue_set_is_rechargeable (device->priv->proxy_device, g_value_get_boolean (value));
		break;
	case PROP_HAS_HISTORY:
		up_device_glue_set_has_history (device->priv->proxy_device, g_value_get_boolean (value));
		break;
	case PROP_HAS_STATISTICS:
		up_device_glue_set_has_statistics (device->priv->proxy_device, g_value_get_boolean (value));
		break;
	case PROP_STATE:
		up_device_glue_set_state (device->priv->proxy_device, g_value_get_uint (value));
		break;
	case PROP_CAPACITY:
		up_device_glue_set_capacity (device->priv->proxy_device, g_value_get_double (value));
		break;
	case PROP_ENERGY:
		up_device_glue_set_energy (device->priv->proxy_device, g_value_get_double (value));
		break;
	case PROP_ENERGY_EMPTY:
		up_device_glue_set_energy_empty (device->priv->proxy_device, g_value_get_double (value));
		break;
	case PROP_ENERGY_FULL:
		up_device_glue_set_energy_full (device->priv->proxy_device, g_value_get_double (value));
		break;
	case PROP_ENERGY_FULL_DESIGN:
		up_device_glue_set_energy_full_design (device->priv->proxy_device, g_value_get_double (value));
		break;
	case PROP_ENERGY_RATE:
		up_device_glue_set_energy_rate (device->priv->proxy_device, g_value_get_double (value));
		break;
	case PROP_VOLTAGE:
		up_device_glue_set_voltage (device->priv->proxy_device, g_value_get_double (value));
		break;
	case PROP_LUMINOSITY:
		up_device_glue_set_luminosity (device->priv->proxy_device, g_value_get_double (value));
		break;
	case PROP_TIME_TO_EMPTY:
		up_device_glue_set_time_to_empty (device->priv->proxy_device, g_value_get_int64 (value));
		break;
	case PROP_TIME_TO_FULL:
		up_device_glue_set_time_to_full (device->priv->proxy_device, g_value_get_int64 (value));
		break;
	case PROP_PERCENTAGE:
		up_device_glue_set_percentage (device->priv->proxy_device, g_value_get_double (value));
		break;
	case PROP_TEMPERATURE:
		up_device_glue_set_temperature (device->priv->proxy_device, g_value_get_double (value));
		break;
	case PROP_TECHNOLOGY:
		up_device_glue_set_technology (device->priv->proxy_device, g_value_get_uint (value));
		break;
	case PROP_WARNING_LEVEL:
		up_device_glue_set_warning_level (device->priv->proxy_device, g_value_get_uint (value));
		break;
	case PROP_ICON_NAME:
		up_device_glue_set_icon_name (device->priv->proxy_device, g_value_get_string (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

/*
 * up_device_get_property:
 */
static void
up_device_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	UpDevice *device = UP_DEVICE (object);

	if (device->priv->proxy_device == NULL) {
		GValue *v;

		v = g_hash_table_lookup (device->priv->offline_props, GUINT_TO_POINTER(prop_id));
		if (v)
			g_value_copy (v, value);
		else
			g_warning ("Property ID '%s' (%d) was never set", pspec->name, prop_id);

		return;
	}

	switch (prop_id) {
	case PROP_UPDATE_TIME:
		g_value_set_uint64 (value, up_device_glue_get_update_time (device->priv->proxy_device));
		break;
	case PROP_VENDOR:
		g_value_set_string (value, up_device_glue_get_vendor (device->priv->proxy_device));
		break;
	case PROP_MODEL:
		g_value_set_string (value, up_device_glue_get_model (device->priv->proxy_device));
		break;
	case PROP_SERIAL:
		g_value_set_string (value, up_device_glue_get_serial (device->priv->proxy_device));
		break;
	case PROP_NATIVE_PATH:
		g_value_set_string (value, up_device_glue_get_native_path (device->priv->proxy_device));
		break;
	case PROP_POWER_SUPPLY:
		g_value_set_boolean (value, up_device_glue_get_power_supply (device->priv->proxy_device));
		break;
	case PROP_ONLINE:
		g_value_set_boolean (value, up_device_glue_get_online (device->priv->proxy_device));
		break;
	case PROP_IS_PRESENT:
		g_value_set_boolean (value, up_device_glue_get_is_present (device->priv->proxy_device));
		break;
	case PROP_IS_RECHARGEABLE:
		g_value_set_boolean (value, up_device_glue_get_is_rechargeable (device->priv->proxy_device));
		break;
	case PROP_HAS_HISTORY:
		g_value_set_boolean (value, up_device_glue_get_has_history (device->priv->proxy_device));
		break;
	case PROP_HAS_STATISTICS:
		g_value_set_boolean (value, up_device_glue_get_has_statistics (device->priv->proxy_device));
		break;
	case PROP_KIND:
		g_value_set_uint (value, up_device_glue_get_type_ (device->priv->proxy_device));
		break;
	case PROP_STATE:
		g_value_set_uint (value, up_device_glue_get_state (device->priv->proxy_device));
		break;
	case PROP_TECHNOLOGY:
		g_value_set_uint (value, up_device_glue_get_technology (device->priv->proxy_device));
		break;
	case PROP_CAPACITY:
		g_value_set_double (value, up_device_glue_get_capacity (device->priv->proxy_device));
		break;
	case PROP_ENERGY:
		g_value_set_double (value, up_device_glue_get_energy (device->priv->proxy_device));
		break;
	case PROP_ENERGY_EMPTY:
		g_value_set_double (value, up_device_glue_get_energy_empty (device->priv->proxy_device));
		break;
	case PROP_ENERGY_FULL:
		g_value_set_double (value, up_device_glue_get_energy_full (device->priv->proxy_device));
		break;
	case PROP_ENERGY_FULL_DESIGN:
		g_value_set_double (value, up_device_glue_get_energy_full_design (device->priv->proxy_device));
		break;
	case PROP_ENERGY_RATE:
		g_value_set_double (value, up_device_glue_get_energy_rate (device->priv->proxy_device));
		break;
	case PROP_VOLTAGE:
		g_value_set_double (value, up_device_glue_get_voltage (device->priv->proxy_device));
		break;
	case PROP_LUMINOSITY:
		g_value_set_double (value, up_device_glue_get_luminosity (device->priv->proxy_device));
		break;
	case PROP_TIME_TO_EMPTY:
		g_value_set_int64 (value, up_device_glue_get_time_to_empty (device->priv->proxy_device));
		break;
	case PROP_TIME_TO_FULL:
		g_value_set_int64 (value, up_device_glue_get_time_to_full (device->priv->proxy_device));
		break;
	case PROP_PERCENTAGE:
		g_value_set_double (value, up_device_glue_get_percentage (device->priv->proxy_device));
		break;
	case PROP_TEMPERATURE:
		g_value_set_double (value, up_device_glue_get_temperature (device->priv->proxy_device));
		break;
	case PROP_WARNING_LEVEL:
		g_value_set_uint (value, up_device_glue_get_warning_level (device->priv->proxy_device));
		break;
	case PROP_ICON_NAME:
		g_value_set_string (value, up_device_glue_get_icon_name (device->priv->proxy_device));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
    }
}

/*
 * up_device_class_init:
 */
static void
up_device_class_init (UpDeviceClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = up_device_finalize;
	object_class->set_property = up_device_set_property;
	object_class->get_property = up_device_get_property;

	/**
	 * UpDevice:update-time:
	 *
	 * The last time the device was updated.
	 *
	 * Since: 0.9.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_UPDATE_TIME,
					 g_param_spec_uint64 ("update-time",
							      NULL, NULL,
							      0, G_MAXUINT64, 0,
							      G_PARAM_READWRITE));
	/**
	 * UpDevice:vendor:
	 *
	 * The vendor of the device.
	 *
	 * Since: 0.9.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_VENDOR,
					 g_param_spec_string ("vendor",
							      NULL, NULL,
							      NULL,
							      G_PARAM_READWRITE));
	/**
	 * UpDevice:model:
	 *
	 * The model of the device.
	 *
	 * Since: 0.9.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_MODEL,
					 g_param_spec_string ("model",
							      NULL, NULL,
							      NULL,
							      G_PARAM_READWRITE));
	/**
	 * UpDevice:serial:
	 *
	 * The serial number of the device.
	 *
	 * Since: 0.9.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_SERIAL,
					 g_param_spec_string ("serial",
							      NULL, NULL,
							      NULL,
							      G_PARAM_READWRITE));
	/**
	 * UpDevice:native-path:
	 *
	 * The native path of the device, useful for direct device access.
	 *
	 * Since: 0.9.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_NATIVE_PATH,
					 g_param_spec_string ("native-path",
							      NULL, NULL,
							      NULL,
							      G_PARAM_READWRITE));
	/**
	 * UpDevice:power-supply:
	 *
	 * If the device is powering the system.
	 *
	 * Since: 0.9.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_POWER_SUPPLY,
					 g_param_spec_boolean ("power-supply",
							       NULL, NULL,
							       FALSE,
							       G_PARAM_READWRITE));
	/**
	 * UpDevice:online:
	 *
	 * If the device is online, i.e. connected.
	 *
	 * Since: 0.9.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_ONLINE,
					 g_param_spec_boolean ("online",
							       NULL, NULL,
							       FALSE,
							       G_PARAM_READWRITE));
	/**
	 * UpDevice:is-present:
	 *
	 * If the device is present, as some devices like laptop batteries
	 * can be removed, leaving an empty bay that is still technically a
	 * device.
	 *
	 * Since: 0.9.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_IS_PRESENT,
					 g_param_spec_boolean ("is-present",
							       NULL, NULL,
							       FALSE,
							       G_PARAM_READWRITE));
	/**
	 * UpDevice:is-rechargeable:
	 *
	 * If the device has a rechargable battery.
	 *
	 * Since: 0.9.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_IS_RECHARGEABLE,
					 g_param_spec_boolean ("is-rechargeable",
							       NULL, NULL,
							       FALSE,
							       G_PARAM_READWRITE));
	/**
	 * UpDevice:has-history:
	 *
	 * If the device has history data that might be useful.
	 *
	 * Since: 0.9.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_HAS_HISTORY,
					 g_param_spec_boolean ("has-history",
							       NULL, NULL,
							       FALSE,
							       G_PARAM_READWRITE));
	/**
	 * UpDevice:has-statistics:
	 *
	 * If the device has statistics data that might be useful.
	 *
	 * Since: 0.9.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_HAS_STATISTICS,
					 g_param_spec_boolean ("has-statistics",
							       NULL, NULL,
							       FALSE,
							       G_PARAM_READWRITE));
	/**
	 * UpDevice:kind:
	 *
	 * The device kind, e.g. %UP_DEVICE_KIND_KEYBOARD.
	 *
	 * Since: 0.9.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_KIND,
					 g_param_spec_uint ("kind",
							    NULL, NULL,
							    UP_DEVICE_KIND_UNKNOWN,
							    UP_DEVICE_KIND_LAST,
							    UP_DEVICE_KIND_UNKNOWN,
							    G_PARAM_READWRITE));
	/**
	 * UpDevice:state:
	 *
	 * The state the device is in at this time, e.g. %UP_DEVICE_STATE_EMPTY.
	 *
	 * Since: 0.9.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_STATE,
					 g_param_spec_uint ("state",
							    NULL, NULL,
							    UP_DEVICE_STATE_UNKNOWN,
							    UP_DEVICE_STATE_LAST,
							    UP_DEVICE_STATE_UNKNOWN,
							    G_PARAM_READWRITE));
	/**
	 * UpDevice:technology:
	 *
	 * The battery technology e.g. %UP_DEVICE_TECHNOLOGY_LITHIUM_ION.
	 *
	 * Since: 0.9.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_TECHNOLOGY,
					 g_param_spec_uint ("technology",
							    NULL, NULL,
							    UP_DEVICE_TECHNOLOGY_UNKNOWN,
							    UP_DEVICE_TECHNOLOGY_LAST,
							    UP_DEVICE_TECHNOLOGY_UNKNOWN,
							    G_PARAM_READWRITE));
	/**
	 * UpDevice:capacity:
	 *
	 * The percentage capacity of the device where 100% means the device has
	 * the same charge potential as when it was manufactured.
	 *
	 * Since: 0.9.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_CAPACITY,
					 g_param_spec_double ("capacity", NULL, NULL,
							      0.0, 100.f, 100.0,
							      G_PARAM_READWRITE));
	/**
	 * UpDevice:energy:
	 *
	 * The energy left in the device. Measured in mWh.
	 *
	 * Since: 0.9.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_ENERGY,
					 g_param_spec_double ("energy", NULL, NULL,
							      0.0, G_MAXDOUBLE, 0.0,
							      G_PARAM_READWRITE));
	/**
	 * UpDevice:energy-empty:
	 *
	 * The energy the device will have when it is empty. This is usually zero.
	 * Measured in mWh.
	 *
	 * Since: 0.9.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_ENERGY_EMPTY,
					 g_param_spec_double ("energy-empty", NULL, NULL,
							      0.0, G_MAXDOUBLE, 0.0,
							      G_PARAM_READWRITE));
	/**
	 * UpDevice:energy-full:
	 *
	 * The amount of energy when the device is fully charged. Measured in mWh.
	 *
	 * Since: 0.9.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_ENERGY_FULL,
					 g_param_spec_double ("energy-full", NULL, NULL,
							      0.0, G_MAXDOUBLE, 0.0,
							      G_PARAM_READWRITE));
	/**
	 * UpDevice:energy-full-design:
	 *
	 * The amount of energy when the device was brand new. Measured in mWh.
	 *
	 * Since: 0.9.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_ENERGY_FULL_DESIGN,
					 g_param_spec_double ("energy-full-design", NULL, NULL,
							      0.0, G_MAXDOUBLE, 0.0,
							      G_PARAM_READWRITE));
	/**
	 * UpDevice:energy-rate:
	 *
	 * The rate of discharge or charge. Measured in mW.
	 *
	 * Since: 0.9.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_ENERGY_RATE,
					 g_param_spec_double ("energy-rate", NULL, NULL,
							      0.0, G_MAXDOUBLE, 0.0,
							      G_PARAM_READWRITE));
	/**
	 * UpDevice:voltage:
	 *
	 * The current voltage of the device.
	 *
	 * Since: 0.9.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_VOLTAGE,
					 g_param_spec_double ("voltage", NULL, NULL,
							      0.0, G_MAXDOUBLE, 0.0,
							      G_PARAM_READWRITE));

	/**
	 * UpDevice:luminosity:
	 *
	 * The current luminosity of the device.
	 *
	 * Since: 0.9.19
	 **/
	g_object_class_install_property (object_class,
					 PROP_LUMINOSITY,
					 g_param_spec_double ("luminosity", NULL, NULL,
							      0.0, G_MAXDOUBLE, 0.0,
							      G_PARAM_READWRITE));
	/**
	 * UpDevice:time-to-empty:
	 *
	 * The amount of time until the device is empty.
	 *
	 * Since: 0.9.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_TIME_TO_EMPTY,
					 g_param_spec_int64 ("time-to-empty", NULL, NULL,
							      0, G_MAXINT64, 0,
							      G_PARAM_READWRITE));
	/**
	 * UpDevice:time-to-full:
	 *
	 * The amount of time until the device is fully charged.
	 *
	 * Since: 0.9.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_TIME_TO_FULL,
					 g_param_spec_int64 ("time-to-full", NULL, NULL,
							      0, G_MAXINT64, 0,
							      G_PARAM_READWRITE));
	/**
	 * UpDevice:percentage:
	 *
	 * The percentage charge of the device.
	 *
	 * Since: 0.9.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_PERCENTAGE,
					 g_param_spec_double ("percentage", NULL, NULL,
							      0.0, 100.f, 100.0,
							      G_PARAM_READWRITE));
	/**
	 * UpDevice:temperature:
	 *
	 * The temperature of the device in degrees Celsius.
	 *
	 * Since: 0.9.22
	 **/
	g_object_class_install_property (object_class,
					 PROP_TEMPERATURE,
					 g_param_spec_double ("temperature", NULL, NULL,
							      0.0, G_MAXDOUBLE, 0.0,
							      G_PARAM_READWRITE));
	/**
	 * UpDevice:warning-level:
	 *
	 * The warning level e.g. %UP_DEVICE_LEVEL_WARNING.
	 *
	 * Since: 1.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_WARNING_LEVEL,
					 g_param_spec_uint ("warning-level",
							    NULL, NULL,
							    UP_DEVICE_LEVEL_UNKNOWN,
							    UP_DEVICE_LEVEL_LAST,
							    UP_DEVICE_LEVEL_UNKNOWN,
							    G_PARAM_READWRITE));

	/**
	 * UpDevice:icon-name:
	 *
	 * The icon name, following the Icon Naming Speficiation
	 *
	 * Since: 1.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_ICON_NAME,
					 g_param_spec_string ("icon-name",
							      NULL, NULL, NULL,
							      G_PARAM_READWRITE));

	g_type_class_add_private (klass, sizeof (UpDevicePrivate));
}

static void
value_free (GValue *value)
{
	g_value_unset (value);
	g_slice_free (GValue, value);
}

/*
 * up_device_init:
 */
static void
up_device_init (UpDevice *device)
{
	device->priv = UP_DEVICE_GET_PRIVATE (device);
	device->priv->offline_props = g_hash_table_new_full (g_direct_hash,
							     g_direct_equal,
							     NULL,
							     (GDestroyNotify) value_free);
}

/*
 * up_device_finalize:
 */
static void
up_device_finalize (GObject *object)
{
	UpDevice *device;

	g_return_if_fail (UP_IS_DEVICE (object));

	device = UP_DEVICE (object);

	if (device->priv->proxy_device != NULL)
		g_object_unref (device->priv->proxy_device);

	g_clear_pointer (&device->priv->offline_props, g_hash_table_unref);

	G_OBJECT_CLASS (up_device_parent_class)->finalize (object);
}

/**
 * up_device_new:
 *
 * Creates a new #UpDevice object.
 *
 * Return value: a new UpDevice object.
 *
 * Since: 0.9.0
 **/
UpDevice *
up_device_new (void)
{
	return UP_DEVICE (g_object_new (UP_TYPE_DEVICE, NULL));
}
