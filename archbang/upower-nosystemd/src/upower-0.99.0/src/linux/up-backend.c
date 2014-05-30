/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2010 Richard Hughes <richard@hughsie.com>
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
#include <math.h>
#include <sys/wait.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gudev/gudev.h>

#include "up-backend.h"
#include "up-daemon.h"
#include "up-marshal.h"
#include "up-device.h"

#include "up-device-supply.h"
#include "up-device-csr.h"
#include "up-device-unifying.h"
#include "up-device-wup.h"
#include "up-device-hid.h"
#include "up-input.h"
#include "up-dock.h"
#include "up-config.h"
#ifdef HAVE_IDEVICE
#include "up-device-idevice.h"
#endif /* HAVE_IDEVICE */

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

static void	up_backend_class_init	(UpBackendClass	*klass);
static void	up_backend_init	(UpBackend		*backend);
static void	up_backend_finalize	(GObject		*object);

#define LOGIND_DBUS_NAME                       "org.freedesktop.login1"
#define LOGIND_DBUS_PATH                       "/org/freedesktop/login1"
#define LOGIND_DBUS_INTERFACE                  "org.freedesktop.login1.Manager"

#define UP_BACKEND_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), UP_TYPE_BACKEND, UpBackendPrivate))

struct UpBackendPrivate
{
	UpDaemon		*daemon;
	UpDeviceList		*device_list;
	GUdevClient		*gudev_client;
	UpDeviceList		*managed_devices;
	UpDock			*dock;
	UpConfig		*config;
	DBusConnection		*connection;
	GDBusProxy		*logind_proxy;
};

enum {
	SIGNAL_DEVICE_ADDED,
	SIGNAL_DEVICE_REMOVED,
	SIGNAL_LAST
};

static guint signals [SIGNAL_LAST] = { 0 };

G_DEFINE_TYPE (UpBackend, up_backend, G_TYPE_OBJECT)

static gboolean up_backend_device_add (UpBackend *backend, GUdevDevice *native);
static void up_backend_device_remove (UpBackend *backend, GUdevDevice *native);

/**
 * up_backend_device_new:
 **/
static UpDevice *
up_backend_device_new (UpBackend *backend, GUdevDevice *native)
{
	const gchar *subsys;
	const gchar *native_path;
	UpDevice *device = NULL;
	UpInput *input;
	gboolean ret;

	subsys = g_udev_device_get_subsystem (native);
	if (g_strcmp0 (subsys, "power_supply") == 0) {

		/* are we a valid power supply */
		device = UP_DEVICE (up_device_supply_new ());
		ret = up_device_coldplug (device, backend->priv->daemon, G_OBJECT (native));
		if (ret)
			goto out;
		g_object_unref (device);

		/* no valid power supply object */
		device = NULL;

	} else if (g_strcmp0 (subsys, "hid") == 0) {

		/* see if this is a Unifying mouse or keyboard */
		device = UP_DEVICE (up_device_unifying_new ());
		ret = up_device_coldplug (device, backend->priv->daemon, G_OBJECT (native));
		if (ret)
			goto out;
		g_object_unref (device);

		/* no valid power supply object */
		device = NULL;

	} else if (g_strcmp0 (subsys, "tty") == 0) {

		/* see if this is a Watts Up Pro device */
		device = UP_DEVICE (up_device_wup_new ());
		ret = up_device_coldplug (device, backend->priv->daemon, G_OBJECT (native));
		if (ret)
			goto out;
		g_object_unref (device);

		/* no valid TTY object */
		device = NULL;

	} else if (g_strcmp0 (subsys, "usb") == 0 || g_strcmp0 (subsys, "usbmisc") == 0) {

#ifdef HAVE_IDEVICE
		/* see if this is an iDevice */
		device = UP_DEVICE (up_device_idevice_new ());
		ret = up_device_coldplug (device, backend->priv->daemon, G_OBJECT (native));
		if (ret)
			goto out;
		g_object_unref (device);
#endif /* HAVE_IDEVICE */

		/* see if this is a CSR mouse or keyboard */
		device = UP_DEVICE (up_device_csr_new ());
		ret = up_device_coldplug (device, backend->priv->daemon, G_OBJECT (native));
		if (ret)
			goto out;
		g_object_unref (device);

		/* try to detect a HID UPS */
		device = UP_DEVICE (up_device_hid_new ());
		ret = up_device_coldplug (device, backend->priv->daemon, G_OBJECT (native));
		if (ret)
			goto out;
		g_object_unref (device);

		/* no valid USB object */
		device = NULL;

	} else if (g_strcmp0 (subsys, "input") == 0) {

		/* check input device */
		input = up_input_new ();
		ret = up_input_coldplug (input, backend->priv->daemon, native);
		if (ret) {
			/* we now have a lid */
			up_daemon_set_lid_is_present (backend->priv->daemon, TRUE);

			/* not a power device */
			up_device_list_insert (backend->priv->managed_devices, G_OBJECT (native), G_OBJECT (input));

			/* no valid input object */
			device = NULL;
		}
		g_object_unref (input);
	} else {
		native_path = g_udev_device_get_sysfs_path (native);
		g_warning ("native path %s (%s) ignoring", native_path, subsys);
	}
out:
	return device;
}

/**
 * up_backend_device_changed:
 **/
static void
up_backend_device_changed (UpBackend *backend, GUdevDevice *native)
{
	GObject *object;
	UpDevice *device;
	gboolean ret;

	/* first, check the device and add it if it doesn't exist */
	object = up_device_list_lookup (backend->priv->device_list, G_OBJECT (native));
	if (object == NULL) {
		g_warning ("treating change event as add on %s", g_udev_device_get_sysfs_path (native));
		up_backend_device_add (backend, native);
		goto out;
	}

	/* need to refresh device */
	device = UP_DEVICE (object);
	ret = up_device_refresh_internal (device);
	if (!ret) {
		g_debug ("no changes on %s", up_device_get_object_path (device));
		goto out;
	}
out:
	if (object != NULL)
		g_object_unref (object);
}

/**
 * up_backend_device_add:
 **/
static gboolean
up_backend_device_add (UpBackend *backend, GUdevDevice *native)
{
	GObject *object;
	UpDevice *device;
	gboolean ret = TRUE;

	/* does device exist in db? */
	object = up_device_list_lookup (backend->priv->device_list, G_OBJECT (native));
	if (object != NULL) {
		device = UP_DEVICE (object);
		/* we already have the device; treat as change event */
		g_warning ("treating add event as change event on %s", up_device_get_object_path (device));
		up_backend_device_changed (backend, native);
		goto out;
	}

	/* get the right sort of device */
	device = up_backend_device_new (backend, native);
	if (device == NULL) {
		ret = FALSE;
		goto out;
	}

	/* emit */
	g_signal_emit (backend, signals[SIGNAL_DEVICE_ADDED], 0, native, device);
out:
	if (object != NULL)
		g_object_unref (object);
	return ret;
}

/**
 * up_backend_device_remove:
 **/
static void
up_backend_device_remove (UpBackend *backend, GUdevDevice *native)
{
	GObject *object;
	UpDevice *device;

	/* does device exist in db? */
	object = up_device_list_lookup (backend->priv->device_list, G_OBJECT (native));
	if (object == NULL) {
		g_debug ("ignoring remove event on %s", g_udev_device_get_sysfs_path (native));
		goto out;
	}

	device = UP_DEVICE (object);
	/* emit */
	g_debug ("emitting device-removed: %s", g_udev_device_get_sysfs_path (native));
	g_signal_emit (backend, signals[SIGNAL_DEVICE_REMOVED], 0, native, device);

out:
	if (object != NULL)
		g_object_unref (object);
}

/**
 * up_backend_uevent_signal_handler_cb:
 **/
static void
up_backend_uevent_signal_handler_cb (GUdevClient *client, const gchar *action,
				      GUdevDevice *device, gpointer user_data)
{
	UpBackend *backend = UP_BACKEND (user_data);

	if (g_strcmp0 (action, "add") == 0) {
		g_debug ("SYSFS add %s", g_udev_device_get_sysfs_path (device));
		up_backend_device_add (backend, device);
	} else if (g_strcmp0 (action, "remove") == 0) {
		g_debug ("SYSFS remove %s", g_udev_device_get_sysfs_path (device));
		up_backend_device_remove (backend, device);
	} else if (g_strcmp0 (action, "change") == 0) {
		g_debug ("SYSFS change %s", g_udev_device_get_sysfs_path (device));
		up_backend_device_changed (backend, device);
	} else {
		g_warning ("unhandled action '%s' on %s", action, g_udev_device_get_sysfs_path (device));
	}
}

/**
 * up_backend_coldplug:
 * @backend: The %UpBackend class instance
 * @daemon: The %UpDaemon controlling instance
 *
 * Finds all the devices already plugged in, and emits device-add signals for
 * each of them.
 *
 * Return value: %TRUE for success
 **/
gboolean
up_backend_coldplug (UpBackend *backend, UpDaemon *daemon)
{
	GUdevDevice *native;
	GList *devices;
	GList *l;
	guint i;
	gboolean ret;
	const gchar *subsystems_wup[] = {"power_supply", "usb", "usbmisc", "tty", "input", "hid", NULL};
	const gchar *subsystems[] = {"power_supply", "usb", "usbmisc", "input", "hid", NULL};

	backend->priv->daemon = g_object_ref (daemon);
	backend->priv->device_list = up_daemon_get_device_list (daemon);
	if (up_config_get_boolean (backend->priv->config, "EnableWattsUpPro"))
		backend->priv->gudev_client = g_udev_client_new (subsystems_wup);
	else
		backend->priv->gudev_client = g_udev_client_new (subsystems);
	g_signal_connect (backend->priv->gudev_client, "uevent",
			  G_CALLBACK (up_backend_uevent_signal_handler_cb), backend);

	/* add all subsystems */
	for (i=0; subsystems[i] != NULL; i++) {
		g_debug ("registering subsystem : %s", subsystems[i]);
		devices = g_udev_client_query_by_subsystem (backend->priv->gudev_client, subsystems[i]);
		for (l = devices; l != NULL; l = l->next) {
			native = l->data;
			up_backend_device_add (backend, native);
		}
		g_list_foreach (devices, (GFunc) g_object_unref, NULL);
		g_list_free (devices);
	}

	/* add dock update object */
	backend->priv->dock = up_dock_new ();
	ret = up_config_get_boolean (backend->priv->config, "PollDockDevices");
	g_debug ("Polling docks: %s", ret ? "YES" : "NO");
	up_dock_set_should_poll (backend->priv->dock, ret);
	ret = up_dock_coldplug (backend->priv->dock, daemon);
	if (!ret)
		g_warning ("failed to coldplug dock devices");

	return TRUE;
}

static gboolean
check_action_result (GVariant *result)
{
	if (result) {
		const char *s;

		g_variant_get (result, "(s)", &s);
		if (g_strcmp0 (s, "yes") == 0)
			return TRUE;
	}
	return FALSE;
}

/**
 * up_backend_get_critical_action:
 * @backend: The %UpBackend class instance
 *
 * Which action will be taken when %UP_DEVICE_LEVEL_ACTION
 * warning-level occurs.
 **/
const char *
up_backend_get_critical_action (UpBackend *backend)
{
	struct {
		const gchar *method;
		const gchar *can_method;
	} actions[] = {
		{ "HybridSleep", "CanHybridSleep" },
		{ "Hibernate", "CanHibernate" },
		{ "PowerOff", NULL },
	};
	guint i;

	g_return_val_if_fail (backend->priv->logind_proxy != NULL, NULL);

	for (i = 0; i < G_N_ELEMENTS (actions); i++) {
		GVariant *result;

		if (actions[i].can_method) {
			gboolean action_available;

			/* Check whether we can use the method */
			result = g_dbus_proxy_call_sync (backend->priv->logind_proxy,
							 actions[i].can_method,
							 NULL,
							 G_DBUS_CALL_FLAGS_NONE,
							 -1, NULL, NULL);
			action_available = check_action_result (result);
			g_variant_unref (result);

			if (!action_available)
				continue;
		}

		return actions[i].method;
	}
	g_assert_not_reached ();
}

/**
 * up_backend_take_action:
 * @backend: The %UpBackend class instance
 *
 * Act upon the %UP_DEVICE_LEVEL_ACTION warning-level.
 **/
void
up_backend_take_action (UpBackend *backend)
{
	const char *method;

	method = up_backend_get_critical_action (backend);
	g_assert (method != NULL);

	/* Take action */
	g_debug ("About to call logind method %s", method);
	g_dbus_proxy_call (backend->priv->logind_proxy,
			   method,
			   g_variant_new ("(b)", FALSE),
			   G_DBUS_CALL_FLAGS_NONE,
			   G_MAXINT,
			   NULL,
			   NULL,
			   NULL);
}

/**
 * up_backend_class_init:
 * @klass: The UpBackendClass
 **/
static void
up_backend_class_init (UpBackendClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = up_backend_finalize;

	signals [SIGNAL_DEVICE_ADDED] =
		g_signal_new ("device-added",
			      G_TYPE_FROM_CLASS (object_class), G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (UpBackendClass, device_added),
			      NULL, NULL, up_marshal_VOID__POINTER_POINTER,
			      G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_POINTER);
	signals [SIGNAL_DEVICE_REMOVED] =
		g_signal_new ("device-removed",
			      G_TYPE_FROM_CLASS (object_class), G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (UpBackendClass, device_removed),
			      NULL, NULL, up_marshal_VOID__POINTER_POINTER,
			      G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_POINTER);

	g_type_class_add_private (klass, sizeof (UpBackendPrivate));
}

/**
 * up_backend_init:
 **/
static void
up_backend_init (UpBackend *backend)
{
	backend->priv = UP_BACKEND_GET_PRIVATE (backend);
	backend->priv->config = up_config_new ();
	backend->priv->managed_devices = up_device_list_new ();
	backend->priv->logind_proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
								     0,
								     NULL,
								     LOGIND_DBUS_NAME,
								     LOGIND_DBUS_PATH,
								     LOGIND_DBUS_INTERFACE,
								     NULL,
								     NULL);
}

/**
 * up_backend_finalize:
 **/
static void
up_backend_finalize (GObject *object)
{
	UpBackend *backend;

	g_return_if_fail (UP_IS_BACKEND (object));

	backend = UP_BACKEND (object);

	g_object_unref (backend->priv->config);
	if (backend->priv->daemon != NULL)
		g_object_unref (backend->priv->daemon);
	if (backend->priv->device_list != NULL)
		g_object_unref (backend->priv->device_list);
	if (backend->priv->gudev_client != NULL)
		g_object_unref (backend->priv->gudev_client);
	g_clear_object (&backend->priv->logind_proxy);

	g_object_unref (backend->priv->managed_devices);

	G_OBJECT_CLASS (up_backend_parent_class)->finalize (object);
}

/**
 * up_backend_new:
 *
 * Return value: a new %UpBackend object.
 **/
UpBackend *
up_backend_new (void)
{
	return g_object_new (UP_TYPE_BACKEND, NULL);
}

