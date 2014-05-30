/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 David Zeuthen <davidz@redhat.com>
 * Copyright (C) 2008 Richard Hughes <richard@hughsie.com>
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

#include <string.h>
#include <stdlib.h>

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <glib-object.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "up-config.h"
#include "up-polkit.h"
#include "up-device-list.h"
#include "up-device.h"
#include "up-backend.h"
#include "up-daemon.h"

#include "up-daemon-glue.h"
#include "up-marshal.h"

enum
{
	PROP_0,
	PROP_DAEMON_VERSION,
	PROP_ON_BATTERY,
	PROP_LID_IS_CLOSED,
	PROP_LID_IS_PRESENT,
	PROP_IS_DOCKED,
	PROP_LAST
};

enum
{
	SIGNAL_DEVICE_ADDED,
	SIGNAL_DEVICE_REMOVED,
	SIGNAL_LAST,
};

static guint signals[SIGNAL_LAST] = { 0 };

struct UpDaemonPrivate
{
	DBusGConnection		*connection;
	DBusGProxy		*proxy;
	UpConfig		*config;
	UpPolkit		*polkit;
	UpBackend		*backend;
	UpDeviceList		*power_devices;
	guint			 action_timeout_id;
	GHashTable		*poll_timeouts;

	/* Properties */
	gboolean		 on_battery;
	UpDeviceLevel		 warning_level;
	gboolean		 lid_is_closed;
	gboolean		 lid_is_present;
	gboolean		 is_docked;

	/* PropertiesChanged to be emitted */
	GHashTable		*changed_props;
	guint			 props_idle_id;

	/* Display battery properties */
	UpDevice		*display_device;
	UpDeviceKind		 kind;
	UpDeviceState		 state;
	gdouble			 percentage;
	gdouble			 energy;
	gdouble			 energy_full;
	gdouble			 energy_rate;
	gint64			 time_to_empty;
	gint64			 time_to_full;

	/* WarningLevel configuration */
	gboolean		 use_percentage_for_policy;
	guint			 low_percentage;
	guint			 critical_percentage;
	guint			 action_percentage;
	guint			 low_time;
	guint			 critical_time;
	guint			 action_time;
};

static void	up_daemon_finalize		(GObject	*object);
static gboolean	up_daemon_get_on_battery_local	(UpDaemon	*daemon);
static gboolean	up_daemon_get_warning_level_local(UpDaemon	*daemon);
static gboolean	up_daemon_get_on_ac_local 	(UpDaemon	*daemon);

G_DEFINE_TYPE (UpDaemon, up_daemon, G_TYPE_OBJECT)

#define UP_DAEMON_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), UP_TYPE_DAEMON, UpDaemonPrivate))

#define UP_DAEMON_ACTION_DELAY				20 /* seconds */

/**
 * up_daemon_get_on_battery_local:
 *
 * As soon as _any_ battery goes discharging, this is true
 **/
static gboolean
up_daemon_get_on_battery_local (UpDaemon *daemon)
{
	guint i;
	gboolean ret;
	gboolean result = FALSE;
	gboolean on_battery;
	UpDevice *device;
	GPtrArray *array;

	/* ask each device */
	array = up_device_list_get_array (daemon->priv->power_devices);
	for (i=0; i<array->len; i++) {
		device = (UpDevice *) g_ptr_array_index (array, i);
		ret = up_device_get_on_battery (device, &on_battery);
		if (ret && on_battery) {
			result = TRUE;
			break;
		}
	}
	g_ptr_array_unref (array);
	return result;
}

/**
 * up_daemon_get_number_devices_of_type:
 **/
guint
up_daemon_get_number_devices_of_type (UpDaemon *daemon, UpDeviceKind type)
{
	guint i;
	UpDevice *device;
	GPtrArray *array;
	UpDeviceKind type_tmp;
	guint count = 0;

	/* ask each device */
	array = up_device_list_get_array (daemon->priv->power_devices);
	for (i=0; i<array->len; i++) {
		device = (UpDevice *) g_ptr_array_index (array, i);
		g_object_get (device,
			      "type", &type_tmp,
			      NULL);
		if (type == type_tmp)
			count++;
	}
	g_ptr_array_unref (array);
	return count;
}

/**
 * up_daemon_update_display_battery:
 *
 * Update our internal state.
 *
 * Returns: %TRUE if the state changed.
 **/
static gboolean
up_daemon_update_display_battery (UpDaemon *daemon)
{
	guint i;
	GPtrArray *array;

	UpDeviceKind kind_total = UP_DEVICE_KIND_UNKNOWN;
	UpDeviceState state_total = UP_DEVICE_STATE_UNKNOWN;
	gdouble percentage_total = 0.0;
	gdouble energy_total = 0.0;
	gdouble energy_full_total = 0.0;
	gdouble energy_rate_total = 0.0;
	gint64 time_to_empty_total = 0;
	gint64 time_to_full_total = 0;
	gboolean is_present_total = FALSE;
	guint num_batteries = 0;

	/* Gather state from each device */
	array = up_device_list_get_array (daemon->priv->power_devices);
	for (i = 0; i < array->len; i++) {
		UpDevice *device;

		UpDeviceState state = UP_DEVICE_STATE_UNKNOWN;
		UpDeviceKind kind = UP_DEVICE_KIND_UNKNOWN;
		gdouble percentage = 0.0;
		gdouble energy = 0.0;
		gdouble energy_full = 0.0;
		gdouble energy_rate = 0.0;
		gint64 time_to_empty = 0;
		gint64 time_to_full = 0;

		device = g_ptr_array_index (array, i);
		g_object_get (device,
			      "type", &kind,
			      "state", &state,
			      "percentage", &percentage,
			      "energy", &energy,
			      "energy-full", &energy_full,
			      "energy-rate", &energy_rate,
			      "time-to-empty", &time_to_empty,
			      "time-to-full", &time_to_full,
			      NULL);

		/* When we have a UPS, it's either a desktop, and
		 * has no batteries, or a laptop, in which case we
		 * ignore the batteries */
		if (kind == UP_DEVICE_KIND_UPS) {
			kind_total = kind;
			state_total = state;
			energy_total = energy;
			energy_full_total = energy_full;
			energy_rate_total = energy_rate;
			time_to_empty_total = time_to_empty;
			time_to_full_total = time_to_full;
			percentage_total = percentage;
			is_present_total = TRUE;
			break;
		}
		if (kind != UP_DEVICE_KIND_BATTERY)
			continue;

		/* If one battery is charging, then the composite is charging
		 * If all batteries are discharging, then the composite is discharging
		 * If all batteries are fully charged, then they're all fully charged
		 * Everything else is unknown */
		if (state == UP_DEVICE_STATE_CHARGING)
			state_total = UP_DEVICE_STATE_CHARGING;
		else if (state == UP_DEVICE_STATE_DISCHARGING &&
			 state_total != UP_DEVICE_STATE_CHARGING)
			state_total = UP_DEVICE_STATE_DISCHARGING;
		else if (state == UP_DEVICE_STATE_FULLY_CHARGED &&
			 state_total == UP_DEVICE_STATE_UNKNOWN)
			state_total = UP_DEVICE_STATE_FULLY_CHARGED;

		/* sum up composite */
		kind_total = UP_DEVICE_KIND_BATTERY;
		is_present_total = TRUE;
		energy_total += energy;
		energy_full_total += energy_full;
		energy_rate_total += energy_rate;
		time_to_empty_total += time_to_empty;
		time_to_full_total += time_to_full;
		/* Will be recalculated for multiple batteries, no worries */
		percentage_total += percentage;
		num_batteries++;
	}

	/* Handle multiple batteries */
	if (num_batteries <= 1)
		goto out;

	g_debug ("Calculating percentage and time to full/to empty for %i batteries", num_batteries);

	/* use percentage weighted for each battery capacity */
	if (energy_full_total > 0.0)
		percentage_total = 100.0 * energy_total / energy_full_total;

	/* calculate a quick and dirty time remaining value */
	if (energy_rate_total > 0) {
		if (state_total == UP_DEVICE_STATE_DISCHARGING)
			time_to_empty_total = 3600 * (energy_total / energy_rate_total);
		else if (state_total == UP_DEVICE_STATE_CHARGING)
			time_to_full_total = 3600 * ((energy_full_total - energy_total) / energy_rate_total);
	}

out:
	/* Did anything change? */
	if (daemon->priv->kind == kind_total &&
	    daemon->priv->state == state_total &&
	    daemon->priv->energy == energy_total &&
	    daemon->priv->energy_full == energy_full_total &&
	    daemon->priv->energy_rate == energy_rate_total &&
	    daemon->priv->time_to_empty == time_to_empty_total &&
	    daemon->priv->time_to_full == time_to_full_total &&
	    daemon->priv->percentage == percentage_total)
		return FALSE;

	daemon->priv->kind = kind_total;
	daemon->priv->state = state_total;
	daemon->priv->energy = energy_total;
	daemon->priv->energy_full = energy_full_total;
	daemon->priv->energy_rate = energy_rate_total;
	daemon->priv->time_to_empty = time_to_empty_total;
	daemon->priv->time_to_full = time_to_full_total;

	daemon->priv->percentage = percentage_total;

	g_object_set (daemon->priv->display_device,
		      "type", kind_total,
		      "state", state_total,
		      "energy", energy_total,
		      "energy-full", energy_full_total,
		      "energy-rate", energy_rate_total,
		      "time-to-empty", time_to_empty_total,
		      "time-to-full", time_to_full_total,
		      "percentage", percentage_total,
		      "is-present", is_present_total,
		      "power-supply", TRUE,
		      NULL);

	return TRUE;
}

/**
 * up_daemon_get_warning_level_local:
 *
 * As soon as _all_ batteries are low, this is true
 **/
static gboolean
up_daemon_get_warning_level_local (UpDaemon *daemon)
{
	up_daemon_update_display_battery (daemon);
	if (daemon->priv->kind != UP_DEVICE_KIND_UPS &&
	    daemon->priv->kind != UP_DEVICE_KIND_BATTERY)
		return UP_DEVICE_LEVEL_NONE;

	if (daemon->priv->kind == UP_DEVICE_KIND_UPS &&
	    daemon->priv->state != UP_DEVICE_STATE_DISCHARGING)
		return UP_DEVICE_LEVEL_NONE;

	/* Check to see if the batteries have not noticed we are on AC */
	if (daemon->priv->kind == UP_DEVICE_KIND_BATTERY &&
	    up_daemon_get_on_ac_local (daemon))
		return UP_DEVICE_LEVEL_NONE;

	return up_daemon_compute_warning_level (daemon,
						daemon->priv->state,
						daemon->priv->kind,
						TRUE, /* power_supply */
						daemon->priv->percentage,
						daemon->priv->time_to_empty);
}

/**
 * up_daemon_get_on_ac_local:
 *
 * As soon as _any_ ac supply goes online, this is true
 **/
static gboolean
up_daemon_get_on_ac_local (UpDaemon *daemon)
{
	guint i;
	gboolean ret;
	gboolean result = FALSE;
	gboolean online;
	UpDevice *device;
	GPtrArray *array;

	/* ask each device */
	array = up_device_list_get_array (daemon->priv->power_devices);
	for (i=0; i<array->len; i++) {
		device = (UpDevice *) g_ptr_array_index (array, i);
		ret = up_device_get_online (device, &online);
		if (ret && online) {
			result = TRUE;
			break;
		}
	}
	g_ptr_array_unref (array);
	return result;
}

/**
 * up_daemon_refresh_battery_devices:
 **/
static gboolean
up_daemon_refresh_battery_devices (UpDaemon *daemon)
{
	guint i;
	GPtrArray *array;
	UpDevice *device;
	UpDeviceKind type;

	/* refresh all devices in array */
	array = up_device_list_get_array (daemon->priv->power_devices);
	for (i=0; i<array->len; i++) {
		device = (UpDevice *) g_ptr_array_index (array, i);
		/* only refresh battery devices */
		g_object_get (device,
			      "type", &type,
			      NULL);
		if (type == UP_DEVICE_KIND_BATTERY)
			up_device_refresh_internal (device);
	}
	g_ptr_array_unref (array);

	return TRUE;
}

/**
 * up_daemon_enumerate_devices:
 **/
gboolean
up_daemon_enumerate_devices (UpDaemon *daemon, DBusGMethodInvocation *context)
{
	guint i;
	GPtrArray *array;
	GPtrArray *object_paths;
	UpDevice *device;

	/* build a pointer array of the object paths */
	object_paths = g_ptr_array_new_with_free_func (g_free);
	array = up_device_list_get_array (daemon->priv->power_devices);
	for (i=0; i<array->len; i++) {
		device = (UpDevice *) g_ptr_array_index (array, i);
		g_ptr_array_add (object_paths, g_strdup (up_device_get_object_path (device)));
	}
	g_ptr_array_unref (array);

	/* return it on the bus */
	dbus_g_method_return (context, object_paths);

	/* free */
	g_ptr_array_unref (object_paths);
	return TRUE;
}

/**
 * up_daemon_get_display_device:
 **/
gboolean
up_daemon_get_display_device (UpDaemon			*daemon,
			      DBusGMethodInvocation	*context)
{
	dbus_g_method_return (context, up_device_get_object_path (daemon->priv->display_device));
	return TRUE;
}

/**
 * up_daemon_get_critical_action:
 **/
gboolean
up_daemon_get_critical_action (UpDaemon			*daemon,
			      DBusGMethodInvocation	*context)
{
	dbus_g_method_return (context, up_backend_get_critical_action (daemon->priv->backend));
	return TRUE;
}

/**
 * up_daemon_register_power_daemon:
 **/
static gboolean
up_daemon_register_power_daemon (UpDaemon *daemon)
{
	GError *error = NULL;
	gboolean ret = FALSE;
	UpDaemonPrivate *priv = daemon->priv;

	priv->connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
	if (priv->connection == NULL) {
		if (error != NULL) {
			g_critical ("error getting system bus: %s", error->message);
			g_error_free (error);
		}
		goto out;
	}

	/* Register the display device */
	up_device_register_display_device (daemon->priv->display_device,
					   daemon);

	/* connect to DBUS */
	priv->proxy = dbus_g_proxy_new_for_name (priv->connection,
						 DBUS_SERVICE_DBUS,
						 DBUS_PATH_DBUS,
						 DBUS_INTERFACE_DBUS);

	/* register GObject */
	dbus_g_connection_register_g_object (priv->connection,
					     "/org/freedesktop/UPower",
					     G_OBJECT (daemon));

	/* success */
	ret = TRUE;
out:
	return ret;
}

/**
 * up_daemon_startup:
 **/
gboolean
up_daemon_startup (UpDaemon *daemon)
{
	gboolean ret;
	gboolean on_battery;
	UpDeviceLevel warning_level;
	UpDaemonPrivate *priv = daemon->priv;

	/* stop signals and callbacks */
	g_debug ("daemon now coldplug");

	/* coldplug backend backend */
	ret = up_backend_coldplug (priv->backend, daemon);
	if (!ret) {
		g_warning ("failed to coldplug backend");
		goto out;
	}

	/* get battery state */
	on_battery = (up_daemon_get_on_battery_local (daemon) &&
		      !up_daemon_get_on_ac_local (daemon));
	warning_level = up_daemon_get_warning_level_local (daemon);
	up_daemon_set_on_battery (daemon, on_battery);
	up_daemon_set_warning_level (daemon, warning_level);

	/* start signals and callbacks */
	g_debug ("daemon now not coldplug");

	/* register on bus */
	ret = up_daemon_register_power_daemon (daemon);
	if (!ret) {
		g_warning ("failed to register");
		goto out;
	}

out:
	return ret;
}

/**
 * up_daemon_get_device_list:
 **/
UpDeviceList *
up_daemon_get_device_list (UpDaemon *daemon)
{
	return g_object_ref (daemon->priv->power_devices);
}

static void
changed_props_add_to_msg (gpointer key,
			  gpointer _value,
			  gpointer user_data)
{
	GVariant *value = _value;
	const gchar *property = key;
	DBusMessageIter *subiter = user_data;
	DBusMessageIter v_iter;
	DBusMessageIter dict_iter;

	dbus_message_iter_open_container (subiter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_iter);
	dbus_message_iter_append_basic (&dict_iter, DBUS_TYPE_STRING, &property);

	if (g_variant_is_of_type (value, G_VARIANT_TYPE_UINT32)) {
		guint32 val = g_variant_get_uint32 (value);
		dbus_message_iter_open_container (&dict_iter, DBUS_TYPE_VARIANT, "u", &v_iter);
		dbus_message_iter_append_basic (&v_iter, DBUS_TYPE_UINT32, &val);
	} else if (g_variant_is_of_type (value, G_VARIANT_TYPE_BOOLEAN)) {
		gboolean val = g_variant_get_boolean (value);
		dbus_message_iter_open_container (&dict_iter, DBUS_TYPE_VARIANT, "b", &v_iter);
		dbus_message_iter_append_basic (&v_iter, DBUS_TYPE_BOOLEAN, &val);
	} else if (g_variant_is_of_type (value, G_VARIANT_TYPE_STRING)) {
		const gchar *val = g_variant_get_string (value, NULL);
		dbus_message_iter_open_container (&dict_iter, DBUS_TYPE_VARIANT, "s", &v_iter);
		dbus_message_iter_append_basic (&v_iter, DBUS_TYPE_STRING, &val);
	} else if (g_variant_is_of_type (value, G_VARIANT_TYPE_DOUBLE)) {
		gdouble val = g_variant_get_double (value);
		dbus_message_iter_open_container (&dict_iter, DBUS_TYPE_VARIANT, "d", &v_iter);
		dbus_message_iter_append_basic (&v_iter, DBUS_TYPE_DOUBLE, &val);
	} else if (g_variant_is_of_type (value, G_VARIANT_TYPE_UINT64)) {
		guint64 val = g_variant_get_uint64 (value);
		dbus_message_iter_open_container (&dict_iter, DBUS_TYPE_VARIANT, "t", &v_iter);
		dbus_message_iter_append_basic (&v_iter, DBUS_TYPE_UINT64, &val);
	} else if (g_variant_is_of_type (value, G_VARIANT_TYPE_INT64)) {
		gint64 val = g_variant_get_int64 (value);
		dbus_message_iter_open_container (&dict_iter, DBUS_TYPE_VARIANT, "x", &v_iter);
		dbus_message_iter_append_basic (&v_iter, DBUS_TYPE_INT64, &val);
	} else {
		g_assert_not_reached ();
	}
	dbus_message_iter_close_container (&dict_iter, &v_iter);
	dbus_message_iter_close_container (subiter, &dict_iter);
}

void
up_daemon_emit_properties_changed (DBusGConnection *gconnection,
				   const gchar     *object_path,
				   const gchar     *interface,
				   GHashTable      *props)
{
	DBusConnection *connection;
	DBusMessage *message;
	DBusMessageIter iter;
	DBusMessageIter subiter;

	g_return_if_fail (gconnection != NULL);
	g_return_if_fail (object_path != NULL);
	g_return_if_fail (interface != NULL);
	g_return_if_fail (props != NULL);

	connection = dbus_g_connection_get_connection (gconnection);
	message = dbus_message_new_signal (object_path,
					   "org.freedesktop.DBus.Properties",
					   "PropertiesChanged");
	dbus_message_iter_init_append (message, &iter);
	dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface);
	/* changed */
	dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY, "{sv}", &subiter);

	g_hash_table_foreach (props, changed_props_add_to_msg, &subiter);

	dbus_message_iter_close_container (&iter, &subiter);

	/* invalidated */
	dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY, "s", &subiter);
	dbus_message_iter_close_container (&iter, &subiter);

	dbus_connection_send (connection, message, NULL);
	dbus_message_unref (message);
}

static gboolean
changed_props_idle_cb (gpointer user_data)
{
	UpDaemon *daemon = user_data;

	/* D-Bus */
	up_daemon_emit_properties_changed (daemon->priv->connection,
					   "/org/freedesktop/UPower",
					   "org.freedesktop.UPower",
					   daemon->priv->changed_props);
	g_clear_pointer (&daemon->priv->changed_props, g_hash_table_unref);
	daemon->priv->props_idle_id = 0;

	return G_SOURCE_REMOVE;
}

/**
 * up_daemon_queue_changed_property:
 **/
static void
up_daemon_queue_changed_property (UpDaemon    *daemon,
				  const gchar *property,
				  GVariant    *value)
{
	g_return_if_fail (UP_IS_DAEMON (daemon));

	if (daemon->priv->connection == NULL)
		return;

	if (!daemon->priv->changed_props) {
		daemon->priv->changed_props = g_hash_table_new_full (g_str_hash, g_str_equal,
								     NULL, (GDestroyNotify) g_variant_unref);
	}

	g_hash_table_insert (daemon->priv->changed_props,
			     (gpointer) property,
			     value);

	if (daemon->priv->props_idle_id == 0)
		daemon->priv->props_idle_id = g_idle_add (changed_props_idle_cb, daemon);
}

/**
 * up_daemon_set_lid_is_closed:
 **/
void
up_daemon_set_lid_is_closed (UpDaemon *daemon, gboolean lid_is_closed)
{
	UpDaemonPrivate *priv = daemon->priv;

	/* check if we are ignoring the lid */
	if (up_config_get_boolean (priv->config, "IgnoreLid")) {
		g_debug ("ignoring lid state");
		return;
	}

	g_debug ("lid_is_closed = %s", lid_is_closed ? "yes" : "no");
	priv->lid_is_closed = lid_is_closed;
	g_object_notify (G_OBJECT (daemon), "lid-is-closed");

	up_daemon_queue_changed_property (daemon, "LidIsClosed", g_variant_new_boolean (lid_is_closed));
}

/**
 * up_daemon_set_lid_is_present:
 **/
void
up_daemon_set_lid_is_present (UpDaemon *daemon, gboolean lid_is_present)
{
	UpDaemonPrivate *priv = daemon->priv;

	/* check if we are ignoring the lid */
	if (up_config_get_boolean (priv->config, "IgnoreLid")) {
		g_debug ("ignoring lid state");
		return;
	}

	g_debug ("lid_is_present = %s", lid_is_present ? "yes" : "no");
	priv->lid_is_present = lid_is_present;
	g_object_notify (G_OBJECT (daemon), "lid-is-present");

	up_daemon_queue_changed_property (daemon, "LidIsPresent", g_variant_new_boolean (lid_is_present));
}

/**
 * up_daemon_set_is_docked:
 **/
void
up_daemon_set_is_docked (UpDaemon *daemon, gboolean is_docked)
{
	UpDaemonPrivate *priv = daemon->priv;
	g_debug ("is_docked = %s", is_docked ? "yes" : "no");
	priv->is_docked = is_docked;
	g_object_notify (G_OBJECT (daemon), "is-docked");

	up_daemon_queue_changed_property (daemon, "IsDocked", g_variant_new_boolean (is_docked));
}

/**
 * up_daemon_set_on_battery:
 **/
void
up_daemon_set_on_battery (UpDaemon *daemon, gboolean on_battery)
{
	UpDaemonPrivate *priv = daemon->priv;
	g_debug ("on_battery = %s", on_battery ? "yes" : "no");
	priv->on_battery = on_battery;
	g_object_notify (G_OBJECT (daemon), "on-battery");

	up_daemon_queue_changed_property (daemon, "OnBattery", g_variant_new_boolean (on_battery));
}

static gboolean
take_action_timeout_cb (UpDaemon *daemon)
{
	up_backend_take_action (daemon->priv->backend);
	return G_SOURCE_REMOVE;
}

/**
 * up_daemon_set_warning_level:
 **/
void
up_daemon_set_warning_level (UpDaemon *daemon, UpDeviceLevel warning_level)
{
	UpDaemonPrivate *priv = daemon->priv;

	if (priv->warning_level == warning_level)
		return;

	g_debug ("warning_level = %s", up_device_level_to_string (warning_level));
	priv->warning_level = warning_level;

	g_object_set (G_OBJECT (daemon->priv->display_device), "warning-level", warning_level, NULL);

	if (daemon->priv->warning_level == UP_DEVICE_LEVEL_ACTION) {
		if (daemon->priv->action_timeout_id == 0) {
			g_debug ("About to take action in %d seconds", UP_DAEMON_ACTION_DELAY);
			daemon->priv->action_timeout_id = g_timeout_add_seconds (UP_DAEMON_ACTION_DELAY,
										 (GSourceFunc) take_action_timeout_cb,
										 daemon);
			g_source_set_name_by_id (daemon->priv->action_timeout_id, "[upower] take_action_timeout_cb");
		} else {
			g_debug ("Not taking action, timeout id already set");
		}
	} else {
		if (daemon->priv->action_timeout_id > 0) {
			g_debug ("Removing timeout as action level changed");
			g_source_remove (daemon->priv->action_timeout_id);
		}
	}
}

UpDeviceLevel
up_daemon_compute_warning_level (UpDaemon      *daemon,
				 UpDeviceState  state,
				 UpDeviceKind   kind,
				 gboolean       power_supply,
				 gdouble        percentage,
				 gint64         time_to_empty)
{
	gboolean use_percentage = TRUE;
	UpDeviceLevel default_level = UP_DEVICE_LEVEL_NONE;

	if (state != UP_DEVICE_STATE_DISCHARGING)
		return UP_DEVICE_LEVEL_NONE;

	/* Keyboard and mice usually have a coarser
	 * battery level, so this avoids falling directly
	 * into critical (or off) before any warnings */
	if (kind == UP_DEVICE_KIND_MOUSE ||
	    kind == UP_DEVICE_KIND_KEYBOARD) {
		if (percentage < 13.0f)
			return UP_DEVICE_LEVEL_CRITICAL;
		else if (percentage < 26.0f)
			return  UP_DEVICE_LEVEL_LOW;
		else
			return UP_DEVICE_LEVEL_NONE;
	} else if (kind == UP_DEVICE_KIND_UPS) {
		default_level = UP_DEVICE_LEVEL_DISCHARGING;
	}

	if (power_supply &&
	    !daemon->priv->use_percentage_for_policy &&
	    time_to_empty > 0.0)
		use_percentage = FALSE;

	if (use_percentage) {
		if (percentage > daemon->priv->low_percentage)
			return default_level;
		if (percentage > daemon->priv->critical_percentage)
			return UP_DEVICE_LEVEL_LOW;
		if (percentage > daemon->priv->action_percentage)
			return UP_DEVICE_LEVEL_CRITICAL;
		return UP_DEVICE_LEVEL_ACTION;
	} else {
		if (time_to_empty > daemon->priv->low_time)
			return default_level;
		if (time_to_empty > daemon->priv->critical_time)
			return UP_DEVICE_LEVEL_LOW;
		if (time_to_empty > daemon->priv->action_time)
			return UP_DEVICE_LEVEL_CRITICAL;
		return UP_DEVICE_LEVEL_ACTION;
	}
	g_assert_not_reached ();
}

/**
 * up_daemon_device_changed_cb:
 **/
static void
up_daemon_device_changed_cb (UpDevice *device, GParamSpec *pspec, UpDaemon *daemon)
{
	UpDeviceKind type;
	gboolean ret;
	UpDaemonPrivate *priv = daemon->priv;
	UpDeviceLevel warning_level;

	g_return_if_fail (UP_IS_DAEMON (daemon));
	g_return_if_fail (UP_IS_DEVICE (device));

	/* refresh battery devices when AC state changes */
	g_object_get (device,
		      "type", &type,
		      NULL);
	if (type == UP_DEVICE_KIND_LINE_POWER) {
		/* refresh now */
		up_daemon_refresh_battery_devices (daemon);
	}

	/* second, check if the on_battery and warning_level state has changed */
	ret = (up_daemon_get_on_battery_local (daemon) && !up_daemon_get_on_ac_local (daemon));
	if (ret != priv->on_battery) {
		up_daemon_set_on_battery (daemon, ret);
	}
	warning_level = up_daemon_get_warning_level_local (daemon);
	if (warning_level != priv->warning_level)
		up_daemon_set_warning_level (daemon, warning_level);
}

typedef struct {
	guint id;
	guint timeout;
	GSourceFunc callback;
} TimeoutData;

static void
change_idle_timeout (UpDevice   *device,
		     GParamSpec *pspec,
		     gpointer    user_data)
{
	TimeoutData *data;
	GSourceFunc callback;
	UpDaemon *daemon;

	daemon = up_device_get_daemon (device);

	data = g_hash_table_lookup (daemon->priv->poll_timeouts, device);
	callback = data->callback;

	up_daemon_stop_poll (G_OBJECT (device));
	up_daemon_start_poll (G_OBJECT (device), callback);
}

static void
device_destroyed (gpointer  user_data,
		  GObject  *where_the_object_was)
{
	UpDaemon *daemon = user_data;
	TimeoutData *data;

	data = g_hash_table_lookup (daemon->priv->poll_timeouts, where_the_object_was);
	if (data == NULL)
		return;
	g_source_remove (data->id);
	g_hash_table_remove (daemon->priv->poll_timeouts, where_the_object_was);
}

static gboolean
fire_timeout_callback (gpointer user_data)
{
	UpDevice *device = user_data;
	TimeoutData *data;
	UpDaemon *daemon;

	daemon = up_device_get_daemon (device);

	data = g_hash_table_lookup (daemon->priv->poll_timeouts, device);
	g_assert (data);

	g_debug ("Firing timeout for '%s' after %u seconds",
		 up_device_get_object_path (device), data->timeout);

	/* Fire the actual callback */
	(data->callback) (device);

	return G_SOURCE_CONTINUE;
}

static guint
calculate_timeout (UpDevice *device)
{
	UpDeviceLevel warning_level;

	g_object_get (G_OBJECT (device), "warning-level", &warning_level, NULL);
	if (warning_level >= UP_DEVICE_LEVEL_DISCHARGING)
		return 30;
	return 120;
}

void
up_daemon_start_poll (GObject     *object,
		      GSourceFunc  callback)
{
	UpDaemon *daemon;
	UpDevice *device;
	TimeoutData *data;
	guint timeout;
	char *name;

	device = UP_DEVICE (object);
	daemon = up_device_get_daemon (device);

	if (g_hash_table_lookup (daemon->priv->poll_timeouts, device) != NULL) {
		g_warning ("Poll already started for device '%s'",
			   up_device_get_object_path (device));
		return;
	}

	data = g_new0 (TimeoutData, 1);
	data->callback = callback;

	timeout = calculate_timeout (device);
	data->timeout = timeout;

	g_signal_connect (device, "notify::warning-level",
			  G_CALLBACK (change_idle_timeout), NULL);
	g_object_weak_ref (object, device_destroyed, daemon);

	data->id = g_timeout_add_seconds (timeout, fire_timeout_callback, device);
	name = g_strdup_printf ("[upower] UpDevice::poll for %s (%u secs)",
				up_device_get_object_path (device), timeout);
	g_source_set_name_by_id (data->id, name);
	g_free (name);

	g_hash_table_insert (daemon->priv->poll_timeouts, device, data);

	g_debug ("Setup poll for '%s' every %u seconds",
		 up_device_get_object_path (device), timeout);
}

void
up_daemon_stop_poll (GObject *object)
{
	UpDevice *device;
	TimeoutData *data;
	UpDaemon *daemon;

	device = UP_DEVICE (object);
	daemon = up_device_get_daemon (device);

	data = g_hash_table_lookup (daemon->priv->poll_timeouts, device);
	if (data == NULL)
		return;

	g_source_remove (data->id);
	g_object_weak_unref (object, device_destroyed, daemon);
	g_hash_table_remove (daemon->priv->poll_timeouts, device);
}

/**
 * up_daemon_device_added_cb:
 **/
static void
up_daemon_device_added_cb (UpBackend *backend, GObject *native, UpDevice *device, UpDaemon *daemon)
{
	const gchar *object_path;
	UpDaemonPrivate *priv = daemon->priv;

	g_return_if_fail (UP_IS_DAEMON (daemon));
	g_return_if_fail (UP_IS_DEVICE (device));
	g_return_if_fail (G_IS_OBJECT (native));

	/* add to device list */
	up_device_list_insert (priv->power_devices, native, G_OBJECT (device));

	/* connect, so we get changes */
	g_signal_connect (device, "notify",
			  G_CALLBACK (up_daemon_device_changed_cb), daemon);

	/* emit */
	object_path = up_device_get_object_path (device);
	g_debug ("emitting added: %s", object_path);

	/* don't crash the session */
	if (object_path == NULL) {
		g_warning ("INTERNAL STATE CORRUPT (device-added): not sending NULL, native:%p, device:%p", native, device);
		return;
	}
	g_signal_emit (daemon, signals[SIGNAL_DEVICE_ADDED], 0, object_path);
}

/**
 * up_daemon_device_removed_cb:
 **/
static void
up_daemon_device_removed_cb (UpBackend *backend, GObject *native, UpDevice *device, UpDaemon *daemon)
{
	const gchar *object_path;
	UpDaemonPrivate *priv = daemon->priv;

	g_return_if_fail (UP_IS_DAEMON (daemon));
	g_return_if_fail (UP_IS_DEVICE (device));
	g_return_if_fail (G_IS_OBJECT (native));

	/* remove from list */
	up_device_list_remove (priv->power_devices, G_OBJECT(device));

	/* emit */
	object_path = up_device_get_object_path (device);
	g_debug ("emitting device-removed: %s", object_path);

	/* don't crash the session */
	if (object_path == NULL) {
		g_warning ("INTERNAL STATE CORRUPT (device-removed): not sending NULL, native:%p, device:%p", native, device);
		return;
	}
	g_signal_emit (daemon, signals[SIGNAL_DEVICE_REMOVED], 0, object_path);

	/* finalise the object */
	g_object_unref (device);
}

#define LOAD_OR_DEFAULT(val, str, def) val = (load_default ? def : up_config_get_uint (daemon->priv->config, str))

static void
load_percentage_policy (UpDaemon    *daemon,
			gboolean     load_default)
{
	LOAD_OR_DEFAULT (daemon->priv->low_percentage, "PercentageLow", 10);
	LOAD_OR_DEFAULT (daemon->priv->critical_percentage, "PercentageCritical", 3);
	LOAD_OR_DEFAULT (daemon->priv->action_percentage, "PercentageAction", 2);
}

static void
load_time_policy (UpDaemon    *daemon,
		  gboolean     load_default)
{
	LOAD_OR_DEFAULT (daemon->priv->low_time, "TimeLow", 1200);
	LOAD_OR_DEFAULT (daemon->priv->critical_time, "TimeCritical", 300);
	LOAD_OR_DEFAULT (daemon->priv->action_time, "TimeAction", 120);
}

#define IS_DESCENDING(x, y, z) (x > y && y > z)

static void
policy_config_validate (UpDaemon *daemon)
{
	if (daemon->priv->low_percentage >= 100 ||
	    daemon->priv->critical_percentage >= 100 ||
	    daemon->priv->action_percentage >= 100) {
		load_percentage_policy (daemon, TRUE);
	} else if (!IS_DESCENDING (daemon->priv->low_percentage,
				   daemon->priv->critical_percentage,
				   daemon->priv->action_percentage)) {
		load_percentage_policy (daemon, TRUE);
	}

	if (!IS_DESCENDING (daemon->priv->low_time,
			    daemon->priv->critical_time,
			    daemon->priv->action_time)) {
		load_time_policy (daemon, TRUE);
	}
}

/**
 * up_daemon_init:
 **/
static void
up_daemon_init (UpDaemon *daemon)
{
	daemon->priv = UP_DAEMON_GET_PRIVATE (daemon);
	daemon->priv->polkit = up_polkit_new ();
	daemon->priv->config = up_config_new ();
	daemon->priv->power_devices = up_device_list_new ();
	daemon->priv->display_device = up_device_new ();

	daemon->priv->use_percentage_for_policy = up_config_get_boolean (daemon->priv->config, "UsePercentageForPolicy");
	load_percentage_policy (daemon, FALSE);
	load_time_policy (daemon, FALSE);
	policy_config_validate (daemon);

	daemon->priv->backend = up_backend_new ();
	g_signal_connect (daemon->priv->backend, "device-added",
			  G_CALLBACK (up_daemon_device_added_cb), daemon);
	g_signal_connect (daemon->priv->backend, "device-removed",
			  G_CALLBACK (up_daemon_device_removed_cb), daemon);

	daemon->priv->poll_timeouts = g_hash_table_new_full (g_direct_hash, g_direct_equal,
							     NULL, g_free);
}

/**
 * up_daemon_error_quark:
 **/
GQuark
up_daemon_error_quark (void)
{
	static GQuark ret = 0;
	if (ret == 0)
		ret = g_quark_from_static_string ("up_daemon_error");
	return ret;
}

#define ENUM_ENTRY(NAME, DESC) { NAME, "" #NAME "", DESC }
/**
 * up_daemon_error_get_type:
 **/
GType
up_daemon_error_get_type (void)
{
	static GType etype = 0;

	if (etype == 0) {
		static const GEnumValue values[] = {
			ENUM_ENTRY (UP_DAEMON_ERROR_GENERAL, "GeneralError"),
			ENUM_ENTRY (UP_DAEMON_ERROR_NOT_SUPPORTED, "NotSupported"),
			ENUM_ENTRY (UP_DAEMON_ERROR_NO_SUCH_DEVICE, "NoSuchDevice"),
			{ 0, 0, 0 }
		};
		g_assert (UP_DAEMON_NUM_ERRORS == G_N_ELEMENTS (values) - 1);
		etype = g_enum_register_static ("UpDaemonError", values);
	}
	return etype;
}

/**
 * up_daemon_get_property:
 **/
static void
up_daemon_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	UpDaemon *daemon = UP_DAEMON (object);
	UpDaemonPrivate *priv = daemon->priv;

	switch (prop_id) {
	case PROP_DAEMON_VERSION:
		g_value_set_string (value, PACKAGE_VERSION);
		break;
	case PROP_ON_BATTERY:
		g_value_set_boolean (value, priv->on_battery);
		break;
	case PROP_LID_IS_CLOSED:
		g_value_set_boolean (value, priv->lid_is_closed);
		break;
	case PROP_LID_IS_PRESENT:
		g_value_set_boolean (value, priv->lid_is_present);
		break;
	case PROP_IS_DOCKED:
		g_value_set_boolean (value, priv->is_docked);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

/**
 * up_daemon_class_init:
 **/
static void
up_daemon_class_init (UpDaemonClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = up_daemon_finalize;
	object_class->get_property = up_daemon_get_property;

	g_type_class_add_private (klass, sizeof (UpDaemonPrivate));

	signals[SIGNAL_DEVICE_ADDED] =
		g_signal_new ("device-added",
			      G_OBJECT_CLASS_TYPE (klass),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
			      0, NULL, NULL,
			      g_cclosure_marshal_generic,
			      G_TYPE_NONE, 1, DBUS_TYPE_G_OBJECT_PATH);

	signals[SIGNAL_DEVICE_REMOVED] =
		g_signal_new ("device-removed",
			      G_OBJECT_CLASS_TYPE (klass),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
			      0, NULL, NULL,
			      g_cclosure_marshal_generic,
			      G_TYPE_NONE, 1, DBUS_TYPE_G_OBJECT_PATH);

	g_object_class_install_property (object_class,
					 PROP_DAEMON_VERSION,
					 g_param_spec_string ("daemon-version",
							      "Daemon Version",
							      "The version of the running daemon",
							      NULL,
							      G_PARAM_READABLE));

	g_object_class_install_property (object_class,
					 PROP_LID_IS_PRESENT,
					 g_param_spec_boolean ("lid-is-present",
							       "Is a laptop",
							       "If this computer is probably a laptop",
							       FALSE,
							       G_PARAM_READABLE));

	g_object_class_install_property (object_class,
					 PROP_IS_DOCKED,
					 g_param_spec_boolean ("is-docked",
							       "Is docked",
							       "If this computer is docked",
							       FALSE,
							       G_PARAM_READABLE));

	g_object_class_install_property (object_class,
					 PROP_ON_BATTERY,
					 g_param_spec_boolean ("on-battery",
							       "On Battery",
							       "Whether the system is running on battery",
							       FALSE,
							       G_PARAM_READABLE));

	g_object_class_install_property (object_class,
					 PROP_LID_IS_CLOSED,
					 g_param_spec_boolean ("lid-is-closed",
							       "Laptop lid is closed",
							       "If the laptop lid is closed",
							       FALSE,
							       G_PARAM_READABLE));

	dbus_g_object_type_install_info (UP_TYPE_DAEMON, &dbus_glib_up_daemon_object_info);

	dbus_g_error_domain_register (UP_DAEMON_ERROR, NULL, UP_DAEMON_TYPE_ERROR);
}

/**
 * up_daemon_finalize:
 **/
static void
up_daemon_finalize (GObject *object)
{
	UpDaemon *daemon = UP_DAEMON (object);
	UpDaemonPrivate *priv = daemon->priv;

	if (priv->action_timeout_id != 0)
		g_source_remove (priv->action_timeout_id);
	if (priv->props_idle_id != 0)
		g_source_remove (priv->props_idle_id);

	g_clear_pointer (&priv->poll_timeouts, g_hash_table_destroy);

	g_clear_pointer (&daemon->priv->changed_props, g_hash_table_unref);
	if (priv->proxy != NULL)
		g_object_unref (priv->proxy);
	if (priv->connection != NULL)
		dbus_g_connection_unref (priv->connection);
	g_object_unref (priv->power_devices);
	g_object_unref (priv->polkit);
	g_object_unref (priv->config);
	g_object_unref (priv->backend);

	G_OBJECT_CLASS (up_daemon_parent_class)->finalize (object);
}

/**
 * up_daemon_new:
 **/
UpDaemon *
up_daemon_new (void)
{
	return UP_DAEMON (g_object_new (UP_TYPE_DAEMON, NULL));
}

