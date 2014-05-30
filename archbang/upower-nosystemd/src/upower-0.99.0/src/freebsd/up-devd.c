/***************************************************************************
 *
 * up-devd.c : process devd events
 *
 * Copyright (C) 2006, 2007 Joe Marcus Clarke <marcus@FreeBSD.org>
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
 **************************************************************************/

#include "config.h"

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "up-backend.h"
#include "up-backend-acpi.h"
#include "up-devd.h"

#define UP_DEVD_SOCK_PATH		"/var/run/devd.pipe"

#define UP_DEVD_EVENT_NOTIFY		'!'
#define UP_DEVD_EVENT_ADD		'+'
#define UP_DEVD_EVENT_REMOVE		'-'
#define UP_DEVD_EVENT_NOMATCH		'?'

static UpDevdHandler *handlers[] = {
	&up_backend_acpi_devd_handler,
};

static gboolean up_devd_inited = FALSE;

#if 0
static GHashTable *
up_devd_parse_params (const gchar *str)
{
	GHashTable *params;
	gchar **pairs;
	gint i;

	g_return_val_if_fail (str != NULL, FALSE);

	params = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

	pairs = g_strsplit(str, " ", 0);
	for (i = 0; pairs[i]; i++) {
		gchar *equal;

		equal = strchr (pairs[i], '=');
		g_hash_table_insert (params,
				     equal ? g_strndup(pairs[i], equal - pairs[i]) : g_strdup(pairs[i]),
				     equal ? g_strdup(equal + 1) : NULL);
	}
	g_strfreev (pairs);

	return params;
}
#endif

static gboolean
up_devd_parse_notify (const gchar *event,
		       gchar **system,
		       gchar **subsystem,
		       gchar **type,
		       gchar **data)
{
	gchar **items;
	gboolean status = FALSE;

	/*
	!system=IFNET subsystem=bge0 type=LINK_DOWN
	*/

	g_return_val_if_fail(event != NULL, FALSE);
	g_return_val_if_fail(system != NULL, FALSE);
	g_return_val_if_fail(subsystem != NULL, FALSE);
	g_return_val_if_fail(type != NULL, FALSE);
	g_return_val_if_fail(data != NULL, FALSE);

	items = g_strsplit(event, " ", 0);
	if (g_strv_length(items) < 3)
		goto end;

	if (! g_str_has_prefix(items[0], "system=") ||
			! g_str_has_prefix(items[1], "subsystem=") ||
			! g_str_has_prefix(items[2], "type="))
		goto end;

	*system = g_strdup(items[0] + 7);
	*subsystem = g_strdup(items[1] + 10);
	*type = g_strdup(items[2] + 5);
	*data = g_strdup(items[3]);	/* may be NULL */

	status = TRUE;

end:
	g_strfreev(items);
	return status;
}

static void
up_devd_process_notify_event (UpBackend *backend,
			       const gchar *system,
			       const gchar *subsystem,
			       const gchar *type,
			       const gchar *data)
{
	gint i;

	g_return_if_fail(backend != NULL);
	g_return_if_fail(system != NULL);
	g_return_if_fail(subsystem != NULL);
	g_return_if_fail(type != NULL);
	g_return_if_fail(data != NULL);

	for (i = 0; i < (int) G_N_ELEMENTS(handlers); i++)
		if (handlers[i]->notify && handlers[i]->notify (backend, system, subsystem, type, data))
			return;

	/* no default action */
}

static void
up_devd_process_event (const gchar *event, gpointer user_data)
{
	UpBackend *backend;

	g_return_if_fail(event != NULL);

	backend = UP_BACKEND(user_data);

	g_debug("received devd event: '%s'", event);

	switch (event[0]) {
	case UP_DEVD_EVENT_ADD:
	case UP_DEVD_EVENT_REMOVE:
		/* Do nothing as we don't currently hotplug ACPI devices. */
		return;

	case UP_DEVD_EVENT_NOTIFY: {
		gchar *system;
		gchar *subsystem;
		gchar *type;
		gchar *data;

		if (!up_devd_parse_notify (event + 1, &system, &subsystem, &type, &data))
			goto malformed;

		up_devd_process_notify_event (backend, system, subsystem, type, data);

		g_free (system);
		g_free (subsystem);
		g_free (type);
		g_free (data);
	}
	return;

	case UP_DEVD_EVENT_NOMATCH:
		/* Do nothing. */
		return;
	}

malformed:
	g_warning("malformed devd event: %s", event);
}

static gboolean
up_devd_event_cb (GIOChannel *source, GIOCondition condition,
		   gpointer user_data)
{
	gchar *event;
	gsize terminator;
	GIOStatus status;

	status = g_io_channel_read_line(source, &event, NULL, &terminator, NULL);

	if (status == G_IO_STATUS_NORMAL) {
		event[terminator] = 0;
		up_devd_process_event(event, user_data);
		g_free(event);
	} else if (status == G_IO_STATUS_AGAIN) {
		up_devd_init (UP_BACKEND(user_data));
		if (up_devd_inited) {
			int fd;

			fd = g_io_channel_unix_get_fd (source);
			g_io_channel_shutdown (source, FALSE, NULL);
			close (fd);

			return FALSE;
		}
	}

	return TRUE;
}

void
up_devd_init (UpBackend *backend)
{
	int event_fd;
	struct sockaddr_un addr;

	event_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if (event_fd < 0) {
		g_warning("failed to create event socket: '%s'", g_strerror(errno));
		up_devd_inited = FALSE;
		return;
	}

	addr.sun_family = AF_UNIX;
	strncpy (addr.sun_path, UP_DEVD_SOCK_PATH, sizeof(addr.sun_path));
	if (connect (event_fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
		GIOChannel *channel;

		channel = g_io_channel_unix_new (event_fd);
		g_io_add_watch (channel, G_IO_IN, up_devd_event_cb, backend);
		g_io_channel_unref (channel);
		up_devd_inited = TRUE;
	} else {
		g_warning ("failed to connect to %s: '%s'", UP_DEVD_SOCK_PATH,
			     g_strerror(errno));
		close (event_fd);
		up_devd_inited = FALSE;
	}
}
