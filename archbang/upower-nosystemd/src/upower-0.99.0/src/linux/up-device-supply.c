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
#include <math.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gprintf.h>
#include <glib/gi18n-lib.h>
#include <glib-object.h>
#include <gudev/gudev.h>

#include "sysfs-utils.h"
#include "up-config.h"
#include "up-types.h"
#include "up-device-supply.h"

#define UP_DEVICE_SUPPLY_REFRESH_TIMEOUT	30	/* seconds */
#define UP_DEVICE_SUPPLY_UNKNOWN_TIMEOUT	1	/* seconds */
#define UP_DEVICE_SUPPLY_UNKNOWN_RETRIES	5
#define UP_DEVICE_SUPPLY_CHARGED_THRESHOLD	90.0f	/* % */

#define UP_DEVICE_SUPPLY_COLDPLUG_UNITS_CHARGE		TRUE
#define UP_DEVICE_SUPPLY_COLDPLUG_UNITS_ENERGY		FALSE

/* number of old energy values to keep cached */
#define UP_DEVICE_SUPPLY_ENERGY_OLD_LENGTH		4

struct UpDeviceSupplyPrivate
{
	guint			 poll_timer_id;
	gboolean		 has_coldplug_values;
	gboolean		 coldplug_units;
	gdouble			*energy_old;
	GTimeVal		*energy_old_timespec;
	guint			 energy_old_first;
	gdouble			 rate_old;
	guint			 unknown_retries;
	gboolean		 disable_battery_poll; /* from configuration */
	gboolean		 is_power_supply;
	gboolean		 shown_invalid_voltage_warning;
};

G_DEFINE_TYPE (UpDeviceSupply, up_device_supply, UP_TYPE_DEVICE)
#define UP_DEVICE_SUPPLY_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), UP_TYPE_DEVICE_SUPPLY, UpDeviceSupplyPrivate))

static gboolean		 up_device_supply_refresh	 	(UpDevice *device);

/**
 * up_device_supply_refresh_line_power:
 *
 * Return %TRUE on success, %FALSE if we failed to refresh or no data
 **/
static gboolean
up_device_supply_refresh_line_power (UpDeviceSupply *supply)
{
	UpDevice *device = UP_DEVICE (supply);
	GUdevDevice *native;
	const gchar *native_path;

	/* is providing power to computer? */
	g_object_set (device,
		      "power-supply", supply->priv->is_power_supply,
		      NULL);

	/* get new AC value */
	native = G_UDEV_DEVICE (up_device_get_native (device));
	native_path = g_udev_device_get_sysfs_path (native);
	g_object_set (device, "online", sysfs_get_int (native_path, "online"), NULL);

	return TRUE;
}

/**
 * up_device_supply_reset_values:
 **/
static void
up_device_supply_reset_values (UpDeviceSupply *supply)
{
	UpDevice *device = UP_DEVICE (supply);
	guint i;

	supply->priv->has_coldplug_values = FALSE;
	supply->priv->coldplug_units = UP_DEVICE_SUPPLY_COLDPLUG_UNITS_ENERGY;
	supply->priv->rate_old = 0;

	for (i = 0; i < UP_DEVICE_SUPPLY_ENERGY_OLD_LENGTH; ++i) {
		supply->priv->energy_old[i] = 0.0f;
		supply->priv->energy_old_timespec[i].tv_sec = 0;
	}
	supply->priv->energy_old_first = 0;

	/* reset to default */
	g_object_set (device,
		      "vendor", NULL,
		      "model", NULL,
		      "serial", NULL,
		      "update-time", (guint64) 0,
		      "power-supply", FALSE,
		      "online", FALSE,
		      "energy", (gdouble) 0.0,
		      "is-present", FALSE,
		      "is-rechargeable", FALSE,
		      "has-history", FALSE,
		      "has-statistics", FALSE,
		      "state", UP_DEVICE_STATE_UNKNOWN,
		      "capacity", (gdouble) 0.0,
		      "energy-empty", (gdouble) 0.0,
		      "energy-full", (gdouble) 0.0,
		      "energy-full-design", (gdouble) 0.0,
		      "energy-rate", (gdouble) 0.0,
		      "voltage", (gdouble) 0.0,
		      "time-to-empty", (gint64) 0,
		      "time-to-full", (gint64) 0,
		      "percentage", (gdouble) 0.0,
		      "temperature", (gdouble) 0.0,
		      "technology", UP_DEVICE_TECHNOLOGY_UNKNOWN,
		      NULL);
}

/**
 * up_device_supply_get_on_battery:
 **/
static gboolean
up_device_supply_get_on_battery (UpDevice *device, gboolean *on_battery)
{
	UpDeviceSupply *supply = UP_DEVICE_SUPPLY (device);
	UpDeviceKind type;
	UpDeviceState state;
	gboolean is_power_supply;
	gboolean is_present;

	g_return_val_if_fail (UP_IS_DEVICE_SUPPLY (supply), FALSE);
	g_return_val_if_fail (on_battery != NULL, FALSE);

	g_object_get (device,
		      "type", &type,
		      "state", &state,
		      "is-present", &is_present,
		      "power-supply", &is_power_supply,
		      NULL);

	if (!is_power_supply)
		return FALSE;
	if (type != UP_DEVICE_KIND_BATTERY)
		return FALSE;
	if (state == UP_DEVICE_STATE_UNKNOWN)
		return FALSE;
	if (!is_present)
		return FALSE;

	*on_battery = (state == UP_DEVICE_STATE_DISCHARGING);
	return TRUE;
}

/**
 * up_device_supply_get_online:
 **/
static gboolean
up_device_supply_get_online (UpDevice *device, gboolean *online)
{
	UpDeviceSupply *supply = UP_DEVICE_SUPPLY (device);
	UpDeviceKind type;
	gboolean online_tmp;

	g_return_val_if_fail (UP_IS_DEVICE_SUPPLY (supply), FALSE);
	g_return_val_if_fail (online != NULL, FALSE);

	g_object_get (device,
		      "type", &type,
		      "online", &online_tmp,
		      NULL);

	if (type != UP_DEVICE_KIND_LINE_POWER)
		return FALSE;

	*online = online_tmp;

	return TRUE;
}

/**
 * up_device_supply_push_new_energy:
 *
 * Store the new energy in the list of old energies of the supply, so
 * it can be used to determine the energy rate.
 */
static gboolean
up_device_supply_push_new_energy (UpDeviceSupply *supply, gdouble energy)
{
	guint first = supply->priv->energy_old_first;
	guint new_position = (first + UP_DEVICE_SUPPLY_ENERGY_OLD_LENGTH - 1) %
		UP_DEVICE_SUPPLY_ENERGY_OLD_LENGTH;

	/* check if the energy value has changed and, if that's the case,
	 * store the new values in the buffer. */
	if (supply->priv->energy_old[first] != energy) {
		supply->priv->energy_old[new_position] = energy;
		g_get_current_time (&supply->priv->energy_old_timespec[new_position]);
		supply->priv->energy_old_first = new_position;
		return TRUE;
	}

	return FALSE;
}

/**
 * up_device_supply_calculate_rate:
 **/
static gdouble
up_device_supply_calculate_rate (UpDeviceSupply *supply, gdouble energy)
{
	gdouble rate = 0.0f;
	gdouble sum_x = 0.0f; /* sum of the squared times difference */
	GTimeVal now;
	guint i;
	guint valid_values = 0;

	/* get the time difference from now and use linear regression to determine
	 * the discharge rate of the battery. */
	g_get_current_time (&now);

	/* store the data on the new energy received */
	up_device_supply_push_new_energy (supply, energy);

	if (energy < 0.1f)
		return 0.0f;

	if (supply->priv->energy_old[supply->priv->energy_old_first] < 0.1f)
		return 0.0f;

	/* don't use the new point obtained since it may cause instability in
	 * the estimate */
	i = supply->priv->energy_old_first;
	now = supply->priv->energy_old_timespec[i];
	do {
		/* only use this value if it seems valid */
		if (supply->priv->energy_old_timespec[i].tv_sec && supply->priv->energy_old[i]) {
			/* This is the square of t_i^2 */
			sum_x += (now.tv_sec - supply->priv->energy_old_timespec[i].tv_sec) *
				(now.tv_sec - supply->priv->energy_old_timespec[i].tv_sec);

			/* Sum the module of the energy difference */
			rate += fabs ((supply->priv->energy_old_timespec[i].tv_sec - now.tv_sec) *
				      (energy - supply->priv->energy_old[i]));
			valid_values++;
		}

		/* get the next element in the circular buffer */
		i = (i + 1) % UP_DEVICE_SUPPLY_ENERGY_OLD_LENGTH;
	} while (i != supply->priv->energy_old_first);

	/* Check that at least 3 points were involved in computation */
	if (sum_x == 0.0f || valid_values < 3)
		return supply->priv->rate_old;

	/* Compute the discharge per hour, and not per second */
	rate /= sum_x / 3600.0f;

	/* if the rate is zero, use the old rate. It will usually happens if no
	 * data is in the buffer yet. If the rate is too high, i.e. more than,
	 * 100W don't use it. */
	if (rate == 0.0f || rate > 100.0f)
		return supply->priv->rate_old;

	return rate;
}

/**
 * up_device_supply_convert_device_technology:
 **/
static UpDeviceTechnology
up_device_supply_convert_device_technology (const gchar *type)
{
	if (type == NULL)
		return UP_DEVICE_TECHNOLOGY_UNKNOWN;
	/* every case combination of Li-Ion is commonly used.. */
	if (g_ascii_strcasecmp (type, "li-ion") == 0 ||
	    g_ascii_strcasecmp (type, "lion") == 0)
		return UP_DEVICE_TECHNOLOGY_LITHIUM_ION;
	if (g_ascii_strcasecmp (type, "pb") == 0 ||
	    g_ascii_strcasecmp (type, "pbac") == 0)
		return UP_DEVICE_TECHNOLOGY_LEAD_ACID;
	if (g_ascii_strcasecmp (type, "lip") == 0 ||
	    g_ascii_strcasecmp (type, "lipo") == 0 ||
	    g_ascii_strcasecmp (type, "li-poly") == 0)
		return UP_DEVICE_TECHNOLOGY_LITHIUM_POLYMER;
	if (g_ascii_strcasecmp (type, "nimh") == 0)
		return UP_DEVICE_TECHNOLOGY_NICKEL_METAL_HYDRIDE;
	if (g_ascii_strcasecmp (type, "life") == 0)
		return UP_DEVICE_TECHNOLOGY_LITHIUM_IRON_PHOSPHATE;
	return UP_DEVICE_TECHNOLOGY_UNKNOWN;
}

/**
 * up_device_supply_get_string:
 **/
static gchar *
up_device_supply_get_string (const gchar *native_path, const gchar *key)
{
	gchar *value;

	/* get value, and strip to remove spaces */
	value = g_strstrip (sysfs_get_string (native_path, key));

	/* no value */
	if (value == NULL)
		goto out;

	/* empty value */
	if (value[0] == '\0') {
		g_free (value);
		value = NULL;
		goto out;
	}
out:
	return value;
}

/**
 * up_device_supply_get_design_voltage:
 **/
static gdouble
up_device_supply_get_design_voltage (UpDeviceSupply *device, const gchar *native_path)
{
	gdouble voltage;
	gchar *device_type = NULL;

	/* design maximum */
	voltage = sysfs_get_double (native_path, "voltage_max_design") / 1000000.0;
	if (voltage > 1.00f) {
		g_debug ("using max design voltage");
		goto out;
	}

	/* design minimum */
	voltage = sysfs_get_double (native_path, "voltage_min_design") / 1000000.0;
	if (voltage > 1.00f) {
		g_debug ("using min design voltage");
		goto out;
	}

	/* current voltage */
	voltage = sysfs_get_double (native_path, "voltage_present") / 1000000.0;
	if (voltage > 1.00f) {
		g_debug ("using present voltage");
		goto out;
	}

	/* current voltage, alternate form */
	voltage = sysfs_get_double (native_path, "voltage_now") / 1000000.0;
	if (voltage > 1.00f) {
		g_debug ("using present voltage (alternate)");
		goto out;
	}

	/* is this a USB device? */
	device_type = up_device_supply_get_string (native_path, "type");
	if (device_type != NULL && g_ascii_strcasecmp (device_type, "USB") == 0) {
		g_debug ("USB device, so assuming 5v");
		voltage = 5.0f;
		goto out;
	}

	/* no valid value found; display a warning the first time for each
	 * device */
	if (!device->priv->shown_invalid_voltage_warning) {
		device->priv->shown_invalid_voltage_warning = TRUE;
		g_warning ("no valid voltage value found for device %s, assuming 10V", native_path);
	}
	/* completely guess, to avoid getting zero values */
	g_debug ("no voltage values for device %s, using 10V as approximation", native_path);
	voltage = 10.0f;
out:
	g_free (device_type);
	return voltage;
}

/**
 * up_device_supply_make_safe_string:
 **/
static void
up_device_supply_make_safe_string (gchar *text)
{
	guint i;
	guint idx = 0;

	/* no point checking */
	if (text == NULL)
		return;

	if (g_utf8_validate (text, -1, NULL))
		return;

	/* shunt up only safe chars */
	for (i=0; text[i] != '\0'; i++) {
		if (g_ascii_isprint (text[i])) {
			/* only copy if the address is going to change */
			if (idx != i)
				text[idx] = text[i];
			idx++;
		} else {
			g_debug ("invalid char '%c'", text[i]);
		}
	}

	/* ensure null terminated */
	text[idx] = '\0';
}

static gboolean
up_device_supply_units_changed (UpDeviceSupply *supply, const gchar *native_path)
{
	if (supply->priv->coldplug_units == UP_DEVICE_SUPPLY_COLDPLUG_UNITS_CHARGE)
		if (sysfs_file_exists (native_path, "charge_now") ||
		    sysfs_file_exists (native_path, "charge_avg"))
			return FALSE;
	if (supply->priv->coldplug_units == UP_DEVICE_SUPPLY_COLDPLUG_UNITS_ENERGY)
		if (sysfs_file_exists (native_path, "energy_now") ||
		    sysfs_file_exists (native_path, "energy_avg"))
			return FALSE;
	return TRUE;
}

static UpDeviceState
up_device_supply_get_state (const gchar *native_path)
{
	UpDeviceState state;
	gchar *status;

	status = up_device_supply_get_string (native_path, "status");
	if (status == NULL ||
	    g_ascii_strcasecmp (status, "unknown") == 0 ||
	    *status == '\0') {
		state = UP_DEVICE_STATE_UNKNOWN;
	} else if (g_ascii_strcasecmp (status, "charging") == 0)
		state = UP_DEVICE_STATE_CHARGING;
	else if (g_ascii_strcasecmp (status, "discharging") == 0)
		state = UP_DEVICE_STATE_DISCHARGING;
	else if (g_ascii_strcasecmp (status, "full") == 0)
		state = UP_DEVICE_STATE_FULLY_CHARGED;
	else if (g_ascii_strcasecmp (status, "empty") == 0)
		state = UP_DEVICE_STATE_EMPTY;
	else if (g_ascii_strcasecmp (status, "not charging") == 0)
		state = UP_DEVICE_STATE_PENDING_CHARGE;
	else {
		g_warning ("unknown status string: %s", status);
		state = UP_DEVICE_STATE_UNKNOWN;
	}

	g_free (status);

	return state;
}

/**
 * up_device_supply_refresh_battery:
 *
 * Return %TRUE on success, %FALSE if we failed to refresh or no data
 **/
static gboolean
up_device_supply_refresh_battery (UpDeviceSupply *supply,
				  UpDeviceState  *out_state)
{
	gchar *technology_native = NULL;
	gboolean ret = TRUE;
	gdouble voltage_design;
	UpDeviceState old_state;
	UpDeviceState state;
	UpDevice *device = UP_DEVICE (supply);
	const gchar *native_path;
	GUdevDevice *native;
	gboolean is_present;
	gdouble energy;
	gdouble energy_full;
	gdouble energy_full_design;
	gdouble energy_rate;
	gdouble capacity = 100.0f;
	gdouble percentage = 0.0f;
	gdouble voltage;
	gint64 time_to_empty;
	gint64 time_to_full;
	gdouble temp;
	gchar *manufacturer = NULL;
	gchar *model_name = NULL;
	gchar *serial_number = NULL;
	UpDaemon *daemon;
	gboolean ac_online = FALSE;
	gboolean has_ac = FALSE;
	gboolean online;
	GPtrArray *devices;
	guint i;

	native = G_UDEV_DEVICE (up_device_get_native (device));
	native_path = g_udev_device_get_sysfs_path (native);

	/* have we just been removed? */
	if (sysfs_file_exists (native_path, "present")) {
		is_present = sysfs_get_bool (native_path, "present");
	} else {
		/* when no present property exists, handle as present */
		is_present = TRUE;
	}
	g_object_set (device, "is-present", is_present, NULL);
	if (!is_present) {
		up_device_supply_reset_values (supply);
		g_object_get (device, "state", out_state, NULL);
		goto out;
	}

	/* get the current charge */
	energy = sysfs_get_double (native_path, "energy_now") / 1000000.0;
	if (energy < 0.01)
		energy = sysfs_get_double (native_path, "energy_avg") / 1000000.0;

	/* used to convert A to W later */
	voltage_design = up_device_supply_get_design_voltage (supply, native_path);

	/* initial values */
	if (!supply->priv->has_coldplug_values ||
	    up_device_supply_units_changed (supply, native_path)) {

		g_object_set (device,
			      "power-supply", supply->priv->is_power_supply,
			      NULL);

		/* the ACPI spec is bad at defining battery type constants */
		technology_native = up_device_supply_get_string (native_path, "technology");
		g_object_set (device, "technology", up_device_supply_convert_device_technology (technology_native), NULL);

		/* get values which may be blank */
		manufacturer = up_device_supply_get_string (native_path, "manufacturer");
		model_name = up_device_supply_get_string (native_path, "model_name");
		serial_number = up_device_supply_get_string (native_path, "serial_number");

		/* some vendors fill this with binary garbage */
		up_device_supply_make_safe_string (manufacturer);
		up_device_supply_make_safe_string (model_name);
		up_device_supply_make_safe_string (serial_number);

		g_object_set (device,
			      "vendor", manufacturer,
			      "model", model_name,
			      "serial", serial_number,
			      "is-rechargeable", TRUE, /* assume true for laptops */
			      "has-history", TRUE,
			      "has-statistics", TRUE,
			      NULL);

		/* these don't change at runtime */
		energy_full = sysfs_get_double (native_path, "energy_full") / 1000000.0;
		energy_full_design = sysfs_get_double (native_path, "energy_full_design") / 1000000.0;

		/* convert charge to energy */
		if (energy_full < 0.01) {
			energy_full = sysfs_get_double (native_path, "charge_full") / 1000000.0;
			energy_full_design = sysfs_get_double (native_path, "charge_full_design") / 1000000.0;
			energy_full *= voltage_design;
			energy_full_design *= voltage_design;
			supply->priv->coldplug_units = UP_DEVICE_SUPPLY_COLDPLUG_UNITS_CHARGE;
		}

		/* the last full should not be bigger than the design */
		if (energy_full > energy_full_design)
			g_warning ("energy_full (%f) is greater than energy_full_design (%f)",
				     energy_full, energy_full_design);

		/* some systems don't have this */
		if (energy_full < 0.01 && energy_full_design > 0.01) {
			g_warning ("correcting energy_full (%f) using energy_full_design (%f)",
				     energy_full, energy_full_design);
			energy_full = energy_full_design;
		}

		/* calculate how broken our battery is */
		if (energy_full > 0) {
			capacity = (energy_full / energy_full_design) * 100.0f;
			if (capacity < 0)
				capacity = 0.0;
			if (capacity > 100.0)
				capacity = 100.0;
		}
		g_object_set (device, "capacity", capacity, NULL);

		/* we only coldplug once, as these values will never change */
		supply->priv->has_coldplug_values = TRUE;
	} else {
		/* get the old full */
		g_object_get (device,
			      "energy-full", &energy_full,
			      "energy-full-design", &energy_full_design,
			      NULL);
	}

	state = up_device_supply_get_state (native_path);
	*out_state = state;

	/* reset unknown counter */
	if (state != UP_DEVICE_STATE_UNKNOWN) {
		g_debug ("resetting unknown timeout after %i retries", supply->priv->unknown_retries);
		supply->priv->unknown_retries = 0;
	}

	/* this is the new value in uW */
	energy_rate = fabs (sysfs_get_double (native_path, "power_now") / 1000000.0);
	if (energy_rate < 0.01) {
		gdouble charge_full;

		/* convert charge to energy */
		if (energy < 0.01) {
			energy = sysfs_get_double (native_path, "charge_now") / 1000000.0;
			if (energy < 0.01)
				energy = sysfs_get_double (native_path, "charge_avg") / 1000000.0;
			energy *= voltage_design;
		}

		charge_full = sysfs_get_double (native_path, "charge_full") / 1000000.0;
		if (charge_full < 0.01)
			charge_full = sysfs_get_double (native_path, "charge_full_design") / 1000000.0;

		/* If charge_full exists, then current_now is always reported in uA.
		 * In the legacy case, where energy only units exist, and power_now isn't present
		 * current_now is power in uW. */
		energy_rate = fabs (sysfs_get_double (native_path, "current_now") / 1000000.0);
		if (charge_full != 0)
			energy_rate *= voltage_design;
	}

	/* some batteries don't update last_full attribute */
	if (energy > energy_full) {
		g_warning ("energy %f bigger than full %f", energy, energy_full);
		energy_full = energy;
	}

	/* present voltage */
	voltage = sysfs_get_double (native_path, "voltage_now") / 1000000.0;
	if (voltage < 0.01)
		voltage = sysfs_get_double (native_path, "voltage_avg") / 1000000.0;

	/* ACPI gives out the special 'Ones' value for rate when it's unable
	 * to calculate the true rate. We should set the rate zero, and wait
	 * for the BIOS to stabilise. */
	if (energy_rate == 0xffff)
		energy_rate = 0;

	/* sanity check to less than 100W */
	if (energy_rate > 100)
		energy_rate = 0;

	/* the hardware reporting failed -- try to calculate this */
	if (energy_rate < 0.01)
		energy_rate = up_device_supply_calculate_rate (supply, energy);

	/* get a precise percentage */
        if (sysfs_file_exists (native_path, "capacity")) {
		percentage = sysfs_get_double (native_path, "capacity");
		if (percentage < 0.0f)
			percentage = 0.0f;
		if (percentage > 100.0f)
			percentage = 100.0f;
                /* for devices which provide capacity, but not {energy,charge}_now */
                if (energy < 0.1f && energy_full > 0.0f)
                    energy = energy_full * percentage / 100;
        } else if (energy_full > 0.0f) {
		percentage = 100.0 * energy / energy_full;
		if (percentage < 0.0f)
			percentage = 0.0f;
		if (percentage > 100.0f)
			percentage = 100.0f;
	}

	/* the battery isn't charging or discharging, it's just
	 * sitting there half full doing nothing: try to guess a state */
	if (state == UP_DEVICE_STATE_UNKNOWN && supply->priv->is_power_supply) {
		daemon = up_device_get_daemon (device);

		/* If we have any online AC, assume charging, otherwise
		 * discharging */
		devices = up_device_list_get_array (up_daemon_get_device_list (daemon));
		for (i=0; i < devices->len; i++) {
			if (up_device_get_online ((UpDevice *) g_ptr_array_index (devices, i), &online)) {
			       has_ac = TRUE;
				if (online) {
					ac_online = TRUE;
				}
				break;
			}
		}
		g_ptr_array_unref (devices);

		if (has_ac) {
			if (ac_online) {
				if (percentage > UP_DEVICE_SUPPLY_CHARGED_THRESHOLD)
					state = UP_DEVICE_STATE_FULLY_CHARGED;
				else
					state = UP_DEVICE_STATE_CHARGING;
			} else {
				if (percentage < 1.0f)
					state = UP_DEVICE_STATE_EMPTY;
				else
					state = UP_DEVICE_STATE_DISCHARGING;
			}
		} else {
			/* only guess when we have only one battery */
			if (up_daemon_get_number_devices_of_type (daemon, UP_DEVICE_KIND_BATTERY)  == 1) {
				if (percentage < 1.0f)
					state = UP_DEVICE_STATE_EMPTY;
				else
					state = UP_DEVICE_STATE_DISCHARGING;
			}

			/* if we have multiple batteries and don't know their
			 * state, give up and leave it as "unknown". */
		}

		/* print what we did */
		g_debug ("guessing battery state '%s': AC present: %i, AC online: %i",
			   up_device_state_to_string (state), has_ac, ac_online);

		g_object_unref (daemon);
	}

	/* if empty, and BIOS does not know what to do */
	if (state == UP_DEVICE_STATE_UNKNOWN && energy < 0.01) {
		g_warning ("Setting %s state empty as unknown and very low", native_path);
		state = UP_DEVICE_STATE_EMPTY;
	}

	/* some batteries give out massive rate values when nearly empty */
	if (energy < 0.1f)
		energy_rate = 0.0f;

	/* calculate a quick and dirty time remaining value */
	time_to_empty = 0;
	time_to_full = 0;
	if (energy_rate > 0) {
		if (state == UP_DEVICE_STATE_DISCHARGING)
			time_to_empty = 3600 * (energy / energy_rate);
		else if (state == UP_DEVICE_STATE_CHARGING)
			time_to_full = 3600 * ((energy_full - energy) / energy_rate);
		/* TODO: need to factor in battery charge metrics */
	}

	/* check the remaining time is under a set limit, to deal with broken
	   primary batteries rate */
	if (time_to_empty > (240 * 60 * 60)) /* ten days for discharging */
		time_to_empty = 0;
	if (time_to_full > (20 * 60 * 60)) /* 20 hours for charging */
		time_to_full = 0;

	/* get temperature */
	temp = sysfs_get_double(native_path, "temp") / 10.0;

	/* check if the energy value has changed and, if that's the case,
	 * store the new values in the buffer. */
	if (up_device_supply_push_new_energy (supply, energy))
		supply->priv->rate_old = energy_rate;

	/* we changed state */
	g_object_get (device, "state", &old_state, NULL);
	if (old_state != state) {
		for (i = 0; i < UP_DEVICE_SUPPLY_ENERGY_OLD_LENGTH; ++i) {
			supply->priv->energy_old[i] = 0.0f;
			supply->priv->energy_old_timespec[i].tv_sec = 0;

		}
		supply->priv->energy_old_first = 0;
	}

	g_object_set (device,
		      "energy", energy,
		      "energy-full", energy_full,
		      "energy-full-design", energy_full_design,
		      "energy-rate", energy_rate,
		      "percentage", percentage,
		      "state", state,
		      "voltage", voltage,
		      "time-to-empty", time_to_empty,
		      "time-to-full", time_to_full,
		      "temperature", temp,
		      NULL);

out:
	g_free (technology_native);
	g_free (manufacturer);
	g_free (model_name);
	g_free (serial_number);
	return ret;
}

/**
 * up_device_supply_refresh_device:
 *
 * Return %TRUE on success, %FALSE if we failed to refresh or no data
 **/
static gboolean
up_device_supply_refresh_device (UpDeviceSupply *supply,
				 UpDeviceState  *out_state)
{
	gboolean ret = TRUE;
	UpDeviceState state;
	UpDevice *device = UP_DEVICE (supply);
	const gchar *native_path;
	GUdevDevice *native;
	gdouble percentage = 0.0f;

	native = G_UDEV_DEVICE (up_device_get_native (device));
	native_path = g_udev_device_get_sysfs_path (native);

	/* initial values */
	if (!supply->priv->has_coldplug_values) {
		gchar *model_name;

		/* get values which may be blank */
		model_name = up_device_supply_get_string (native_path, "model_name");

		/* some vendors fill this with binary garbage */
		up_device_supply_make_safe_string (model_name);

		g_object_set (device,
			      "is-present", TRUE,
			      "model", model_name,
			      "is-rechargeable", TRUE,
			      "has-history", TRUE,
			      "has-statistics", TRUE,
			      "power-supply", supply->priv->is_power_supply, /* always FALSE */
			      NULL);

		/* we only coldplug once, as these values will never change */
		supply->priv->has_coldplug_values = TRUE;

		g_free (model_name);
	}

	/* get a precise percentage */
	percentage = sysfs_get_double_with_error (native_path, "capacity");
	if (percentage < 0.0) {
		/* Probably talking to the device over Bluetooth */
		state = UP_DEVICE_STATE_UNKNOWN;
		g_object_set (device, "state", state, NULL);
		*out_state = state;
		return FALSE;
	}

	state = up_device_supply_get_state (native_path);

	/* Override whatever the device might have told us
	 * because a number of them are always discharging */
	if (percentage == 100.0)
		state = UP_DEVICE_STATE_FULLY_CHARGED;

	/* reset unknown counter */
	if (state != UP_DEVICE_STATE_UNKNOWN) {
		g_debug ("resetting unknown timeout after %i retries", supply->priv->unknown_retries);
		supply->priv->unknown_retries = 0;
	}

	g_object_set (device,
		      "percentage", percentage,
		      "state", state,
		      NULL);

	*out_state = state;

	return ret;
}

static gboolean
up_device_supply_poll_unknown_battery (UpDevice *device)
{
	UpDeviceSupply *supply = UP_DEVICE_SUPPLY (device);

	g_debug ("Unknown state on supply %s; forcing update after %i seconds",
		 up_device_get_object_path (device), UP_DEVICE_SUPPLY_UNKNOWN_TIMEOUT);

	supply->priv->poll_timer_id = 0;
	up_device_supply_refresh (device);

	return FALSE;
}

/**
 * up_device_supply_coldplug:
 *
 * Return %TRUE on success, %FALSE if we failed to get data and should be removed
 **/
static gboolean
up_device_supply_coldplug (UpDevice *device)
{
	UpDeviceSupply *supply = UP_DEVICE_SUPPLY (device);
	gboolean ret = FALSE;
	GUdevDevice *bluetooth;
	GUdevDevice *native;
	const gchar *file;
	const gchar *device_path = NULL;
	const gchar *native_path;
	const gchar *scope;
	gchar *device_type = NULL;
	gchar *input_path = NULL;
	GDir *dir = NULL;
	GError *error = NULL;
	UpDeviceKind type = UP_DEVICE_KIND_UNKNOWN;

	up_device_supply_reset_values (supply);

	/* detect what kind of device we are */
	native = G_UDEV_DEVICE (up_device_get_native (device));
	native_path = g_udev_device_get_sysfs_path (native);
	if (native_path == NULL) {
		g_warning ("could not get native path for %p", device);
		goto out;
	}

	/* try to work out if the device is powering the system */
	scope = g_udev_device_get_sysfs_attr (native, "scope");
	if (scope != NULL && g_ascii_strcasecmp (scope, "device") == 0) {
		supply->priv->is_power_supply = FALSE;
	} else if (scope != NULL && g_ascii_strcasecmp (scope, "system") == 0) {
		supply->priv->is_power_supply = TRUE;
	} else {
		g_debug ("taking a guess for power supply scope");
		supply->priv->is_power_supply = TRUE;
	}

	/* we don't use separate ACs for devices */
	if (supply->priv->is_power_supply == FALSE &&
	    !sysfs_file_exists (native_path, "capacity")) {
		g_debug ("Ignoring device AC, we'll monitor the device battery");
		goto out;
	}

	/* try to detect using the device type */
	device_type = up_device_supply_get_string (native_path, "type");
	if (device_type != NULL) {
		if (g_ascii_strcasecmp (device_type, "mains") == 0) {
			type = UP_DEVICE_KIND_LINE_POWER;
		} else if (g_ascii_strcasecmp (device_type, "battery") == 0) {

			/* Detect if the battery comes from bluetooth keyboard or mouse. */
			bluetooth = g_udev_device_get_parent_with_subsystem (native, "bluetooth", NULL);
			if (bluetooth != NULL) {
				device_path = g_udev_device_get_sysfs_path (bluetooth);
				if ((dir = g_dir_open (device_path, 0, &error))) {
					while ((file = g_dir_read_name (dir))) {
						/* Check if it is an input device. */
						if (g_str_has_prefix (file, "input")) {
							input_path = g_build_filename (device_path, file, NULL);
							break;
						}
					}
					g_dir_close (dir);
				} else {
					g_warning ("Can not open folder %s: %s", device_path, error->message);
					g_error_free (error);
				}
				g_object_unref (bluetooth);
			}

			if (input_path != NULL) {
				if ((dir = g_dir_open (input_path, 0, &error))) {
					while ((file = g_dir_read_name (dir))) {
						/* Check if it is a mouse device. */
						if (g_str_has_prefix (file, "mouse")) {
							type = UP_DEVICE_KIND_MOUSE;
							break;
						}
					}
					g_dir_close (dir);
				} else {
					g_warning ("Can not open folder %s: %s", input_path, error->message);
					g_error_free (error);
				}
				g_free (input_path);
				if (type == UP_DEVICE_KIND_UNKNOWN) {
					type = UP_DEVICE_KIND_KEYBOARD;
				}
			}

			if (type == UP_DEVICE_KIND_UNKNOWN) {
				type = UP_DEVICE_KIND_BATTERY;
			}
		} else if (g_ascii_strcasecmp (device_type, "USB") == 0) {

			/* use a heuristic to find the device type */
			if (g_strstr_len (native_path, -1, "wacom_") != NULL) {
				type = UP_DEVICE_KIND_TABLET;
			} else {
				g_warning ("did not recognise USB path %s, please report",
					   native_path);
			}
		} else {
			g_warning ("did not recognise type %s, please report", device_type);
		}
	}

	/* if reading the device type did not work, use the previous method */
	if (type == UP_DEVICE_KIND_UNKNOWN) {
		if (sysfs_file_exists (native_path, "online")) {
			type = UP_DEVICE_KIND_LINE_POWER;
		} else {
			/* this is a good guess as UPS and CSR are not in the kernel */
			type = UP_DEVICE_KIND_BATTERY;
		}
	}

	/* set the value */
	g_object_set (device, "type", type, NULL);

	if (type != UP_DEVICE_KIND_LINE_POWER &&
	    type != UP_DEVICE_KIND_BATTERY)
		up_daemon_start_poll (G_OBJECT (device), (GSourceFunc) up_device_supply_refresh);
	else if (type == UP_DEVICE_KIND_BATTERY &&
		 !supply->priv->disable_battery_poll)
		up_daemon_start_poll (G_OBJECT (device), (GSourceFunc) up_device_supply_refresh);

	/* coldplug values */
	ret = up_device_supply_refresh (device);
out:
	g_free (device_type);
	return ret;
}

/**
 * up_device_supply_setup_unknown_poll:
 **/
static void
up_device_supply_setup_unknown_poll (UpDevice      *device,
				     UpDeviceState  state)
{
	UpDeviceSupply *supply = UP_DEVICE_SUPPLY (device);

	if (supply->priv->disable_battery_poll)
		return;

	/* if it's unknown, poll faster than we would normally */
	if (state == UP_DEVICE_STATE_UNKNOWN &&
	    supply->priv->unknown_retries < UP_DEVICE_SUPPLY_UNKNOWN_RETRIES) {
		supply->priv->poll_timer_id =
			g_timeout_add_seconds (UP_DEVICE_SUPPLY_UNKNOWN_TIMEOUT,
					       (GSourceFunc) up_device_supply_poll_unknown_battery, supply);
		g_source_set_name_by_id (supply->priv->poll_timer_id, "[upower] up_device_supply_poll_unknown_battery (linux)");

		/* increase count, we don't want to poll at 0.5Hz forever */
		supply->priv->unknown_retries++;
	}
}

static void
up_device_supply_disable_unknown_poll (UpDevice *device)
{
	UpDeviceSupply *supply = UP_DEVICE_SUPPLY (device);

	if (supply->priv->poll_timer_id > 0) {
		g_source_remove (supply->priv->poll_timer_id);
		supply->priv->poll_timer_id = 0;
	}
}

/**
 * up_device_supply_refresh:
 *
 * Return %TRUE on success, %FALSE if we failed to refresh or no data
 **/
static gboolean
up_device_supply_refresh (UpDevice *device)
{
	gboolean ret;
	GTimeVal timeval;
	UpDeviceSupply *supply = UP_DEVICE_SUPPLY (device);
	UpDeviceKind type;
	UpDeviceState state;

	g_object_get (device, "type", &type, NULL);
	switch (type) {
	case UP_DEVICE_KIND_LINE_POWER:
		ret = up_device_supply_refresh_line_power (supply);
		break;
	case UP_DEVICE_KIND_BATTERY:
		up_device_supply_disable_unknown_poll (device);
		ret = up_device_supply_refresh_battery (supply, &state);
		up_device_supply_setup_unknown_poll (device, state);
		break;
	default:
		ret = up_device_supply_refresh_device (supply, &state);
		break;
	}

	/* reset time if we got new data */
	if (ret) {
		g_get_current_time (&timeval);
		g_object_set (device, "update-time", (guint64) timeval.tv_sec, NULL);
	}

	return ret;
}

/**
 * up_device_supply_init:
 **/
static void
up_device_supply_init (UpDeviceSupply *supply)
{
	UpConfig *config;

	supply->priv = UP_DEVICE_SUPPLY_GET_PRIVATE (supply);

	/* allocate the stats for the battery charging & discharging */
	supply->priv->energy_old = g_new (gdouble, UP_DEVICE_SUPPLY_ENERGY_OLD_LENGTH);
	supply->priv->energy_old_timespec = g_new (GTimeVal, UP_DEVICE_SUPPLY_ENERGY_OLD_LENGTH);

	supply->priv->shown_invalid_voltage_warning = FALSE;

	config = up_config_new ();
	/* Seems that we don't get change uevents from the
	 * kernel on some BIOS types, but if polling
	 * is disabled in the configuration, do nothing */
	supply->priv->disable_battery_poll = up_config_get_boolean (config, "NoPollBatteries");
	g_object_unref (config);
}

/**
 * up_device_supply_finalize:
 **/
static void
up_device_supply_finalize (GObject *object)
{
	UpDeviceSupply *supply;

	g_return_if_fail (object != NULL);
	g_return_if_fail (UP_IS_DEVICE_SUPPLY (object));

	supply = UP_DEVICE_SUPPLY (object);
	g_return_if_fail (supply->priv != NULL);

	up_daemon_stop_poll (object);

	if (supply->priv->poll_timer_id > 0)
		g_source_remove (supply->priv->poll_timer_id);

	g_free (supply->priv->energy_old);
	g_free (supply->priv->energy_old_timespec);

	G_OBJECT_CLASS (up_device_supply_parent_class)->finalize (object);
}

/**
 * up_device_supply_class_init:
 **/
static void
up_device_supply_class_init (UpDeviceSupplyClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	UpDeviceClass *device_class = UP_DEVICE_CLASS (klass);

	object_class->finalize = up_device_supply_finalize;
	device_class->get_on_battery = up_device_supply_get_on_battery;
	device_class->get_online = up_device_supply_get_online;
	device_class->coldplug = up_device_supply_coldplug;
	device_class->refresh = up_device_supply_refresh;

	g_type_class_add_private (klass, sizeof (UpDeviceSupplyPrivate));
}

/**
 * up_device_supply_new:
 **/
UpDeviceSupply *
up_device_supply_new (void)
{
	return g_object_new (UP_TYPE_DEVICE_SUPPLY, NULL);
}

