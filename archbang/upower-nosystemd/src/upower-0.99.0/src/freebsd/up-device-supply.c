/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2009 Joe Marcus Clarke <marcus@FreeBSD.org>
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

#include "config.h"

#include <string.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <dev/acpica/acpiio.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gprintf.h>
#include <glib/gi18n-lib.h>
#include <glib-object.h>

#include "up-acpi-native.h"
#include "up-util.h"

#include "up-types.h"
#include "up-device-supply.h"

#define UP_ACPIDEV			"/dev/acpi"

G_DEFINE_TYPE (UpDeviceSupply, up_device_supply, UP_TYPE_DEVICE)

static gboolean		 up_device_supply_refresh	 	(UpDevice *device);
static UpDeviceTechnology	up_device_supply_convert_device_technology (const gchar *type);
static gboolean		up_device_supply_acline_coldplug	(UpDevice *device);
static gboolean		up_device_supply_battery_coldplug	(UpDevice *device, UpAcpiNative *native);
static gboolean		up_device_supply_acline_set_properties	(UpDevice *device);
static gboolean		up_device_supply_battery_set_properties	(UpDevice *device, UpAcpiNative *native);
static gboolean		up_device_supply_get_on_battery	(UpDevice *device, gboolean *on_battery);
static gboolean		up_device_supply_get_online		(UpDevice *device, gboolean *online);

/**
 * up_device_supply_convert_device_technology:
 *
 * This is taken from linux/up-device-supply.c.
 **/
static UpDeviceTechnology
up_device_supply_convert_device_technology (const gchar *type)
{
	if (type == NULL)
		return UP_DEVICE_TECHNOLOGY_UNKNOWN;
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
	if (g_ascii_strcasecmp (type, "lifo") == 0)
		return UP_DEVICE_TECHNOLOGY_LITHIUM_IRON_PHOSPHATE;
	return UP_DEVICE_TECHNOLOGY_UNKNOWN;
}

/**
 * up_device_supply_reset_values:
 **/
static void
up_device_supply_reset_values (UpDevice *device)
{
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
		      "technology", UP_DEVICE_TECHNOLOGY_UNKNOWN,
		      NULL);
}

/**
 * up_device_supply_acline_coldplug:
 **/
static gboolean
up_device_supply_acline_coldplug (UpDevice *device)
{
	gboolean ret;

	g_object_set (device,
		      "online", FALSE,
		      "power-supply", TRUE,
		      "type", UP_DEVICE_KIND_LINE_POWER,
		      NULL);

	ret = up_device_supply_acline_set_properties (device);

	return ret;
}

/**
 * up_device_supply_battery_coldplug:
 **/
static gboolean
up_device_supply_battery_coldplug (UpDevice *device, UpAcpiNative *native)
{
	gboolean ret;

	g_object_set (device, "type", UP_DEVICE_KIND_BATTERY, NULL);
	ret = up_device_supply_battery_set_properties (device, native);

	return ret;
}

/**
 * up_device_supply_battery_set_properties:
 **/
static gboolean
up_device_supply_battery_set_properties (UpDevice *device, UpAcpiNative *native)
{
	gint fd;
	gdouble volt, dvolt, rate, lastfull, cap, dcap, lcap, capacity;
	gboolean is_present;
	gboolean ret = FALSE;
	gint64 time_to_empty, time_to_full;
	gchar *vendor, *model, *serial;
	UpDeviceTechnology technology;
	UpDeviceState state;
	union acpi_battery_ioctl_arg battif, battst, battinfo;

	if (!up_has_sysctl ("hw.acpi.battery.units"))
		return FALSE;

	battif.unit = battst.unit = battinfo.unit =
		up_acpi_native_get_unit (native);
	fd = open (UP_ACPIDEV, O_RDONLY);
	if (fd < 0) {
		g_warning ("unable to open %s: '%s'", UP_ACPIDEV, g_strerror (errno));
		return FALSE;
	}

	if (ioctl (fd, ACPIIO_BATT_GET_BIF, &battif) == -1) {
		g_warning ("ioctl ACPIIO_BATT_GET_BIF failed for battery %d: '%s'", battif.unit, g_strerror (errno));
		goto end;
	}

	if (ioctl (fd, ACPIIO_BATT_GET_BST, &battst) == -1) {
		g_warning ("ioctl ACPIIO_BATT_GET_BST failed for battery %d: '%s'", battst.unit, g_strerror (errno));
		goto end;
	}

	if (ioctl (fd, ACPIIO_BATT_GET_BATTINFO, &battinfo) == -1) {
		g_warning ("ioctl ACPIIO_BATT_GET_BATTINFO failed for battery %d: '%s'", battinfo.unit, g_strerror (errno));
		goto end;
	}

	ret = TRUE;

	is_present = (battst.bst.state == ACPI_BATT_STAT_NOT_PRESENT) ? FALSE : TRUE;
	g_object_set (device, "is-present", is_present, NULL);

	if (!is_present) {
		up_device_supply_reset_values (device);
		goto end;
	}

	vendor = up_make_safe_string (battif.bif.oeminfo);
	model = up_make_safe_string (battif.bif.model);
	serial = up_make_safe_string (battif.bif.serial);
	technology = up_device_supply_convert_device_technology (battif.bif.type);

	g_object_set (device,
		      "vendor", vendor,
		      "model", model,
		      "serial", serial,
		      "power-supply", TRUE,
		      "technology", technology,
		      "has-history", TRUE,
		      "has-statistics", TRUE,
		      NULL);
	g_free (vendor);
	g_free (model);
	g_free (serial);

	g_object_set (device, "is-rechargeable",
		      battif.bif.btech == 0 ? FALSE : TRUE, NULL);

	volt = (gdouble) battst.bst.volt;
	dvolt = (gdouble) battif.bif.dvol;
	lastfull = (gdouble) battif.bif.lfcap;
	dcap = (gdouble) battif.bif.dcap;
	rate = (gdouble) battst.bst.rate;
	cap = (gdouble) battst.bst.cap;
	lcap = (gdouble) battif.bif.lcap;
	if (rate == 0xffff)
		rate = 0.0f;
	if (rate > 100.0f * 1000.0f)
		rate = 0.0f;

	dvolt /= 1000.0f;
	volt /= 1000.0f;

	if (battif.bif.units == ACPI_BIF_UNITS_MA) {
		if (dvolt <= 0.0f)
			dvolt = 1.0f;
		if (volt <= 0.0f || volt > dvolt)
			volt = dvolt;

		dcap *= volt;
		lastfull *= volt;
		rate *= volt;
		cap *= volt;
		lcap *= volt;
	}

	dcap /= 1000.0f;
	lastfull /= 1000.0f;
	rate /= 1000.0f;
	cap /= 1000.0f;
	lcap /= 1000.0f;

	if (cap == 0.0f)
		rate = 0.0f;

	capacity = 0.0f;
	if (lastfull > 0 && dcap > 0) {
		capacity = (lastfull / dcap) * 100.0f;
		if (capacity < 0)
			capacity = 0.0f;
		if (capacity > 100.0)
			capacity = 100.0f;
	}

	g_object_set (device,
		      "percentage", (gdouble) battinfo.battinfo.cap,
		      "energy", cap,
		      "energy-full", lastfull,
		      "energy-full-design", dcap,
		      "energy-rate", rate,
		      "energy-empty", lcap,
		      "voltage", volt,
		      "capacity", capacity,
		      NULL);

	time_to_empty = 0;
	time_to_full = 0;

	if (battinfo.battinfo.state & ACPI_BATT_STAT_DISCHARG) {
		state = UP_DEVICE_STATE_DISCHARGING;
		if (battinfo.battinfo.min > 0)
			time_to_empty = battinfo.battinfo.min * 60;
		else if (rate > 0) {
			time_to_empty = 3600 * (cap / rate);
			if (time_to_empty > (20 * 60 * 60))
				time_to_empty = 0;
		}
	} else if (battinfo.battinfo.state & ACPI_BATT_STAT_CHARGING) {
		state = UP_DEVICE_STATE_CHARGING;
		if (battinfo.battinfo.min > 0)
			time_to_full = battinfo.battinfo.min * 60;
		else if (rate > 0) {
			time_to_full = 3600 * ((lastfull - cap) / rate);
			if (time_to_full > (20 * 60 * 60))
				time_to_full = 0;
		}
	} else if (battinfo.battinfo.state & ACPI_BATT_STAT_CRITICAL) {
		state = UP_DEVICE_STATE_EMPTY;
	} else if (battinfo.battinfo.state == 0) {
		state = UP_DEVICE_STATE_FULLY_CHARGED;
	} else {
		state = UP_DEVICE_STATE_UNKNOWN;
	}

	g_object_set (device,
		      "state", state,
		      "time-to-empty", time_to_empty,
		      "time-to-full", time_to_full,
		      NULL);

end:
	close (fd);
	return ret;
}

/**
 * up_device_supply_acline_set_properties:
 **/
static gboolean
up_device_supply_acline_set_properties (UpDevice *device)
{
	int acstate;

	if (up_get_int_sysctl (&acstate, NULL, "hw.acpi.acline")) {
		g_object_set (device, "online", acstate ? TRUE : FALSE, NULL);
		return TRUE;
	}

	return FALSE;
}

/**
 * up_device_supply_coldplug:
 * Return %TRUE on success, %FALSE if we failed to get data and should be removed
 **/
static gboolean
up_device_supply_coldplug (UpDevice *device)
{
	UpAcpiNative *native;
	const gchar *native_path;
	const gchar *driver;
	gboolean ret = FALSE;

	up_device_supply_reset_values (device);

	native = UP_ACPI_NATIVE (up_device_get_native (device));
	native_path = up_acpi_native_get_path (native);
	driver = up_acpi_native_get_driver (native);
	if (native_path == NULL) {
		g_warning ("could not get native path for %p", device);
		goto out;
	}

	if (!strcmp (native_path, "hw.acpi.acline")) {
		ret = up_device_supply_acline_coldplug (device);
		goto out;
	}

	if (!g_strcmp0 (driver, "battery")) {
		ret = up_device_supply_battery_coldplug (device, native);
		goto out;
	}

	g_warning ("invalid device %s with driver %s", native_path, driver);

out:
	return ret;
}

/**
 * up_device_supply_refresh:
 *
 * Return %TRUE on success, %FALSE if we failed to refresh or no data
 **/
static gboolean
up_device_supply_refresh (UpDevice *device)
{
	GObject *object;
	GTimeVal timeval;
	UpDeviceKind type;
	gboolean ret;

	g_object_get (device, "type", &type, NULL);
	switch (type) {
		case UP_DEVICE_KIND_LINE_POWER:
			ret = up_device_supply_acline_set_properties (device);
			break;
		case UP_DEVICE_KIND_BATTERY:
			object = up_device_get_native (device);
			ret = up_device_supply_battery_set_properties (device, UP_ACPI_NATIVE (object));
			break;
		default:
			g_assert_not_reached ();
			break;
	}

	if (ret) {
		g_get_current_time (&timeval);
		g_object_set (device, "update-time", (guint64) timeval.tv_sec, NULL);
	}

	return ret;
}

/**
 * up_device_supply_get_on_battery:
 **/
static gboolean
up_device_supply_get_on_battery (UpDevice *device, gboolean *on_battery)
{
	UpDeviceKind type;
	UpDeviceState state;
	gboolean is_present;

	g_return_val_if_fail (on_battery != NULL, FALSE);

	g_object_get (device,
		      "type", &type,
		      "state", &state,
		      "is-present", &is_present,
		      NULL);

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
	UpDeviceKind type;
	gboolean online_tmp;

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
 * up_device_supply_init:
 **/
static void
up_device_supply_init (UpDeviceSupply *supply)
{
}

/**
 * up_device_supply_class_init:
 **/
static void
up_device_supply_class_init (UpDeviceSupplyClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	UpDeviceClass *device_class = UP_DEVICE_CLASS (klass);

	device_class->get_on_battery = up_device_supply_get_on_battery;
	device_class->get_online = up_device_supply_get_online;
	device_class->coldplug = up_device_supply_coldplug;
	device_class->refresh = up_device_supply_refresh;
}

/**
 * up_device_supply_new:
 **/
UpDeviceSupply *
up_device_supply_new (void)
{
	return g_object_new (UP_TYPE_DEVICE_SUPPLY, NULL);
}

