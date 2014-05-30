/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2010 Richard Hughes <richard@hughsie.com>
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

#include <glib-object.h>
#include <gudev/gudev.h>

#include "up-daemon.h"
#include "up-dock.h"

struct UpDockPrivate
{
	UpDaemon		*daemon;
	GUdevClient		*gudev_client;
	guint			 poll_id;
};

G_DEFINE_TYPE (UpDock, up_dock, G_TYPE_OBJECT)
#define UP_DOCK_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), UP_TYPE_DOCK, UpDockPrivate))

/* the kernel is pretty crap at sending a uevent when the status changes */
#define UP_DOCK_POLL_TIMEOUT		30

/**
 * up_dock_device_check:
 **/
static gboolean
up_dock_device_check (GUdevDevice *d)
{
	const gchar *status;
	gboolean ret = FALSE;

	/* Get the boolean state from the kernel -- note that ideally
	 * the property value would be "1" or "true" but now it's
	 * set in stone as ABI. Urgh. */
	status = g_udev_device_get_sysfs_attr (d, "status");
	if (status == NULL)
		goto out;
	ret = (g_strcmp0 (status, "connected") == 0);
	g_debug ("graphics device %s is %s",
		 g_udev_device_get_sysfs_path (d),
		 ret ? "on" : "off");
out:
	return ret;
}

/**
 * up_dock_refresh:
 **/
static gboolean
up_dock_refresh (UpDock *dock)
{
	GList *devices;
	GList *l;
	GUdevDevice *native;
	guint count = 0;

	/* the metric we're using here is that a machine is docked when
	 * there is more than one active output */
	devices = g_udev_client_query_by_subsystem (dock->priv->gudev_client,
						    "drm");
	for (l = devices; l != NULL; l = l->next) {
		native = l->data;
		count += up_dock_device_check (native);
	}
	g_list_foreach (devices, (GFunc) g_object_unref, NULL);
	g_list_free (devices);

	/* update the daemon state, which causes DBus traffic */
	up_daemon_set_is_docked (dock->priv->daemon, (count > 1));

	return TRUE;
}

/**
 * up_dock_poll_cb:
 **/
static gboolean
up_dock_poll_cb (UpDock *dock)
{
	g_debug ("Doing the dock refresh");
	up_dock_refresh (dock);
	return TRUE;
}

/**
 * up_dock_coldplug:
 **/
void
up_dock_set_should_poll (UpDock *dock, gboolean should_poll)
{
	if (should_poll && dock->priv->poll_id == 0) {
		dock->priv->poll_id = g_timeout_add_seconds (UP_DOCK_POLL_TIMEOUT,
							     (GSourceFunc) up_dock_poll_cb,
							     dock);
		g_source_set_name_by_id (dock->priv->poll_id, "[upower] up_dock_poll_cb (linux)");
	} else if (dock->priv->poll_id > 0) {
		g_source_remove (dock->priv->poll_id);
		dock->priv->poll_id = 0;
	}
}

/**
 * up_dock_coldplug:
 **/
gboolean
up_dock_coldplug (UpDock *dock, UpDaemon *daemon)
{
	/* save daemon */
	dock->priv->daemon = g_object_ref (daemon);
	return up_dock_refresh (dock);
}


/**
 * up_dock_uevent_signal_handler_cb:
 **/
static void
up_dock_uevent_signal_handler_cb (GUdevClient *client, const gchar *action,
				  GUdevDevice *device, gpointer user_data)
{
	UpDock *dock = UP_DOCK (user_data);
	up_dock_refresh (dock);
}

/**
 * up_dock_init:
 **/
static void
up_dock_init (UpDock *dock)
{
	const gchar *subsystems[] = { "drm", NULL};
	dock->priv = UP_DOCK_GET_PRIVATE (dock);
	dock->priv->gudev_client = g_udev_client_new (subsystems);
	g_signal_connect (dock->priv->gudev_client, "uevent",
			  G_CALLBACK (up_dock_uevent_signal_handler_cb), dock);
}

/**
 * up_dock_finalize:
 **/
static void
up_dock_finalize (GObject *object)
{
	UpDock *dock;

	g_return_if_fail (object != NULL);
	g_return_if_fail (UP_IS_DOCK (object));

	dock = UP_DOCK (object);
	g_return_if_fail (dock->priv != NULL);

	g_object_unref (dock->priv->gudev_client);
	if (dock->priv->daemon != NULL)
		g_object_unref (dock->priv->daemon);
	if (dock->priv->poll_id != 0)
		g_source_remove (dock->priv->poll_id);
	G_OBJECT_CLASS (up_dock_parent_class)->finalize (object);
}

/**
 * up_dock_class_init:
 **/
static void
up_dock_class_init (UpDockClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = up_dock_finalize;
	g_type_class_add_private (klass, sizeof (UpDockPrivate));
}

/**
 * up_dock_new:
 **/
UpDock *
up_dock_new (void)
{
	return g_object_new (UP_TYPE_DOCK, NULL);
}

