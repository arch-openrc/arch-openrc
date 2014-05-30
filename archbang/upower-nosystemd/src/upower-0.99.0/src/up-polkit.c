/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 David Zeuthen <davidz@redhat.com>
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <polkit/polkit.h>

#include "up-polkit.h"
#include "up-daemon.h"

#define UP_POLKIT_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), UP_TYPE_POLKIT, UpPolkitPrivate))

struct UpPolkitPrivate
{
	DBusGConnection		*connection;
	PolkitAuthority         *authority;
};

G_DEFINE_TYPE (UpPolkit, up_polkit, G_TYPE_OBJECT)
static gpointer up_polkit_object = NULL;

/**
 * up_polkit_get_subject:
 **/
PolkitSubject *
up_polkit_get_subject (UpPolkit *polkit, DBusGMethodInvocation *context)
{
	GError *error;
	gchar *sender;
	PolkitSubject *subject;

	sender = dbus_g_method_get_sender (context);
	subject = polkit_system_bus_name_new (sender);
	g_free (sender);

	if (subject == NULL) {
		error = g_error_new (UP_DAEMON_ERROR, UP_DAEMON_ERROR_GENERAL, "failed to get PolicyKit subject");
		dbus_g_method_return_error (context, error);
		g_error_free (error);
	}

	return subject;
}

/**
 * up_polkit_check_auth:
 **/
gboolean
up_polkit_check_auth (UpPolkit *polkit, PolkitSubject *subject, const gchar *action_id, DBusGMethodInvocation *context)
{
	gboolean ret = FALSE;
	GError *error;
	GError *error_local = NULL;
	PolkitAuthorizationResult *result;

	/* check auth */
	result = polkit_authority_check_authorization_sync (polkit->priv->authority,
							    subject, action_id, NULL,
							    POLKIT_CHECK_AUTHORIZATION_FLAGS_ALLOW_USER_INTERACTION,
							    NULL, &error_local);
	if (result == NULL) {
		error = g_error_new (UP_DAEMON_ERROR, UP_DAEMON_ERROR_GENERAL, "failed to check authorisation: %s", error_local->message);
		dbus_g_method_return_error (context, error);
		g_error_free (error_local);
		g_error_free (error);
		goto out;
	}

	/* okay? */
	if (polkit_authorization_result_get_is_authorized (result)) {
		ret = TRUE;
	} else {
		error = g_error_new (UP_DAEMON_ERROR, UP_DAEMON_ERROR_GENERAL, "not authorized");
		dbus_g_method_return_error (context, error);
		g_error_free (error);
	}
out:
	if (result != NULL)
		g_object_unref (result);
	return ret;
}

/**
 * up_polkit_is_allowed:
 **/
gboolean
up_polkit_is_allowed (UpPolkit *polkit, PolkitSubject *subject, const gchar *action_id, GError **error)
{
	gboolean ret = FALSE;
	GError *error_local = NULL;
	PolkitAuthorizationResult *result;

	/* check auth */
	result = polkit_authority_check_authorization_sync (polkit->priv->authority,
							    subject, action_id, NULL,
							    POLKIT_CHECK_AUTHORIZATION_FLAGS_NONE,
							    NULL, &error_local);
	if (result == NULL) {
		g_set_error (error, UP_DAEMON_ERROR, UP_DAEMON_ERROR_GENERAL, "failed to check authorisation: %s", error_local->message);
		g_error_free (error_local);
		goto out;
	}

	ret = polkit_authorization_result_get_is_authorized (result) ||
	      polkit_authorization_result_get_is_challenge (result);
out:
	if (result != NULL)
		g_object_unref (result);
	return ret;
}

/**
 * up_polkit_get_uid:
 **/
gboolean
up_polkit_get_uid (UpPolkit *polkit, PolkitSubject *subject, uid_t *uid)
{
	DBusConnection *connection;
	const gchar *name;

	if (!POLKIT_IS_SYSTEM_BUS_NAME (subject)) {
		g_debug ("not system bus name");
		return FALSE;
	}

	connection = dbus_g_connection_get_connection (polkit->priv->connection);
	name = polkit_system_bus_name_get_name (POLKIT_SYSTEM_BUS_NAME (subject));
	*uid = dbus_bus_get_unix_user (connection, name, NULL);
	return TRUE;
}

/**
 * up_polkit_get_pid:
 **/
gboolean
up_polkit_get_pid (UpPolkit *polkit, PolkitSubject *subject, pid_t *pid)
{
	gboolean ret = FALSE;
	GError *error = NULL;
	const gchar *name;
	DBusGProxy *proxy = NULL;

	/* bus name? */
	if (!POLKIT_IS_SYSTEM_BUS_NAME (subject)) {
		g_debug ("not system bus name");
		goto out;
	}

	name = polkit_system_bus_name_get_name (POLKIT_SYSTEM_BUS_NAME (subject));
	proxy = dbus_g_proxy_new_for_name_owner (polkit->priv->connection,
						 "org.freedesktop.DBus",
						 "/org/freedesktop/DBus/Bus",
						 "org.freedesktop.DBus", &error);
	if (proxy == NULL) {
		g_warning ("DBUS error: %s", error->message);
		g_error_free (error);
		goto out;
	}

	/* get pid from DBus (quite slow) */
	ret = dbus_g_proxy_call (proxy, "GetConnectionUnixProcessID", &error,
				 G_TYPE_STRING, name,
				 G_TYPE_INVALID,
				 G_TYPE_UINT, pid,
				 G_TYPE_INVALID);
	if (!ret) {
		g_warning ("failed to get pid: %s", error->message);
		g_error_free (error);
		goto out;
        }
out:
	if (proxy != NULL)
		g_object_unref (proxy);
	return ret;
}

/**
 * up_polkit_finalize:
 **/
static void
up_polkit_finalize (GObject *object)
{
	UpPolkit *polkit;
	g_return_if_fail (UP_IS_POLKIT (object));
	polkit = UP_POLKIT (object);

	if (polkit->priv->connection != NULL)
		dbus_g_connection_unref (polkit->priv->connection);
	g_object_unref (polkit->priv->authority);

	G_OBJECT_CLASS (up_polkit_parent_class)->finalize (object);
}

/**
 * up_polkit_class_init:
 **/
static void
up_polkit_class_init (UpPolkitClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = up_polkit_finalize;
	g_type_class_add_private (klass, sizeof (UpPolkitPrivate));
}

/**
 * up_polkit_init:
 *
 * initializes the polkit class. NOTE: We expect polkit objects
 * to *NOT* be removed or added during the session.
 * We only control the first polkit object if there are more than one.
 **/
static void
up_polkit_init (UpPolkit *polkit)
{
	GError *error = NULL;

	polkit->priv = UP_POLKIT_GET_PRIVATE (polkit);

	polkit->priv->connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
	if (polkit->priv->connection == NULL) {
		if (error != NULL) {
			g_critical ("error getting system bus: %s", error->message);
			g_error_free (error);
		}
		goto out;
	}

	polkit->priv->authority = polkit_authority_get_sync (NULL, &error);
	if (polkit->priv->authority == NULL) {
		g_error ("failed to get pokit authority: %s", error->message);
		g_error_free (error);
		goto out;
	}
out:
	return;
}

/**
 * up_polkit_new:
 * Return value: A new polkit class instance.
 **/
UpPolkit *
up_polkit_new (void)
{
	if (up_polkit_object != NULL) {
		g_object_ref (up_polkit_object);
	} else {
		up_polkit_object = g_object_new (UP_TYPE_POLKIT, NULL);
		g_object_add_weak_pointer (up_polkit_object, &up_polkit_object);
	}
	return UP_POLKIT (up_polkit_object);
}

