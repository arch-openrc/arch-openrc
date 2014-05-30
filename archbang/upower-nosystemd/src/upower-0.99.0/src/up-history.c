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

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

#include "up-history.h"
#include "up-stats-item.h"
#include "up-history-item.h"

static void	up_history_finalize	(GObject		*object);

#define UP_HISTORY_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), UP_TYPE_HISTORY, UpHistoryPrivate))

#define UP_HISTORY_FILE_HEADER		"PackageKit Profile"
#define UP_HISTORY_SAVE_INTERVAL	(10*60)		/* seconds */
#define UP_HISTORY_DEFAULT_MAX_DATA_AGE	(7*24*60*60)	/* seconds */

struct UpHistoryPrivate
{
	gchar			*id;
	gdouble			 rate_last;
	gint64			 time_full_last;
	gint64			 time_empty_last;
	gdouble			 percentage_last;
	UpDeviceState		 state;
	GPtrArray		*data_rate;
	GPtrArray		*data_charge;
	GPtrArray		*data_time_full;
	GPtrArray		*data_time_empty;
	guint			 save_id;
	guint			 max_data_age;
	gchar			*dir;
};

enum {
	UP_HISTORY_PROGRESS,
	UP_HISTORY_LAST_SIGNAL
};

G_DEFINE_TYPE (UpHistory, up_history, G_TYPE_OBJECT)

/**
 * up_history_set_max_data_age:
 **/
void
up_history_set_max_data_age (UpHistory *history, guint max_data_age)
{
	history->priv->max_data_age = max_data_age;
}

/**
 * up_history_array_copy_cb:
 **/
static void
up_history_array_copy_cb (UpHistoryItem *item, GPtrArray *dest)
{
	g_ptr_array_add (dest, g_object_ref (item));
}

/**
 * up_history_array_limit_resolution:
 * @array: The data we have for a specific graph
 * @max_num: The max desired points
 *
 * We need to reduce the number of data points else the graph will take a long
 * time to plot accuracy we don't need at the larger scales.
 * This will not reduce the scale or range of the data.
 *
 *  100  +     + |  +   |  +   | +     +
 *       |  A    |      |      |
 *   80  +     + |  +   |  +   | +     +
 *       |       | B  C |      |
 *   60  +     + |  +   |  +   | +     +
 *       |       |      |      |
 *   40  +     + |  +   |  +   | + E   +
 *       |       |      |      | D
 *   20  +     + |  +   |  +   | +   F +
 *       |       |      |      |
 *    0  +-----+-----+-----+-----+-----+
 *            20    40    60    80   100
 *
 * A = 15,90
 * B = 30,70
 * C = 52,70
 * D = 80,30
 * E = 85,40
 * F = 90,20
 *
 * 1 = 15,90
 * 2 = 41,70
 * 3 = 85,30
 **/
static GPtrArray *
up_history_array_limit_resolution (GPtrArray *array, guint max_num)
{
	UpHistoryItem *item;
	UpHistoryItem *item_new;
	gfloat division;
	guint length;
	guint i;
	guint last;
	guint first;
	GPtrArray *new;
	UpDeviceState state = UP_DEVICE_STATE_UNKNOWN;
	guint64 time_s = 0;
	gdouble value = 0;
	guint64 count = 0;
	guint step = 1;
	gfloat preset;

	new = g_ptr_array_new_with_free_func ((GDestroyNotify) g_object_unref);
	g_debug ("length of array (before) %i", array->len);

	/* check length */
	length = array->len;
	if (length == 0)
		goto out;
	if (length < max_num) {
		/* need to copy array */
		g_ptr_array_foreach (array, (GFunc) up_history_array_copy_cb, new);
		goto out;
	}

	/* last element */
	item = (UpHistoryItem *) g_ptr_array_index (array, length-1);
	last = up_history_item_get_time (item);
	item = (UpHistoryItem *) g_ptr_array_index (array, 0);
	first = up_history_item_get_time (item);

	division = (first - last) / (gfloat) max_num;
	g_debug ("Using a x division of %f (first=%i,last=%i)", division, first, last);

	/* Reduces the number of points to a pre-set level using a time
	 * division algorithm so we don't keep diluting the previous
	 * data with a conventional 1-in-x type algorithm. */
	for (i = 0; i < length; i++) {
		item = (UpHistoryItem *) g_ptr_array_index (array, i);
		preset = last + (division * (gfloat) step);

		/* if state changed or we went over the preset do a new point */
		if (count > 0 &&
		    (up_history_item_get_time (item) > preset ||
		     up_history_item_get_state (item) != state)) {
			item_new = up_history_item_new ();
			up_history_item_set_time (item_new, time_s / count);
			up_history_item_set_value (item_new, value / count);
			up_history_item_set_state (item_new, state);
			g_ptr_array_add (new, item_new);

			step++;
			time_s = up_history_item_get_time (item);
			value = up_history_item_get_value (item);
			state = up_history_item_get_state (item);
			count = 1;
		} else {
			count++;
			time_s += up_history_item_get_time (item);
			value += up_history_item_get_value (item);
		}
	}

	/* only add if nonzero */
	if (count > 0) {
		item_new = up_history_item_new ();
		up_history_item_set_time (item_new, time_s / count);
		up_history_item_set_value (item_new, value / count);
		up_history_item_set_state (item_new, state);
		g_ptr_array_add (new, item_new);
	}

	/* check length */
	g_debug ("length of array (after) %i", new->len);
out:
	return new;
}

/**
 * up_history_copy_array_timespan:
 **/
static GPtrArray *
up_history_copy_array_timespan (const GPtrArray *array, guint timespan)
{
	guint i;
	UpHistoryItem *item;
	GPtrArray *array_new;
	GTimeVal timeval;

	/* no data */
	if (array->len == 0)
		return NULL;

	/* no limit on data */
	if (timespan == 0) {
		array_new = g_ptr_array_ref ((GPtrArray *) array);
		goto out;
	}

	/* new data */
	array_new = g_ptr_array_new ();
	g_get_current_time (&timeval);
	g_debug ("limiting data to last %i seconds", timespan);

	/* treat the timespan like a range, and search backwards */
	timespan *= 0.95f;
	for (i=array->len-1; i>0; i--) {
		item = (UpHistoryItem *) g_ptr_array_index (array, i);
		if (timeval.tv_sec - up_history_item_get_time (item) < timespan)
			g_ptr_array_add (array_new, g_object_ref (item));
	}
out:
	return array_new;
}

/**
 * up_history_get_data:
 **/
GPtrArray *
up_history_get_data (UpHistory *history, UpHistoryType type, guint timespan, guint resolution)
{
	GPtrArray *array;
	GPtrArray *array_resolution;
	const GPtrArray *array_data = NULL;

	g_return_val_if_fail (UP_IS_HISTORY (history), NULL);

	if (history->priv->id == NULL)
		return NULL;

	if (type == UP_HISTORY_TYPE_CHARGE)
		array_data = history->priv->data_charge;
	else if (type == UP_HISTORY_TYPE_RATE)
		array_data = history->priv->data_rate;
	else if (type == UP_HISTORY_TYPE_TIME_FULL)
		array_data = history->priv->data_time_full;
	else if (type == UP_HISTORY_TYPE_TIME_EMPTY)
		array_data = history->priv->data_time_empty;

	/* not recognised */
	if (array_data == NULL)
		return NULL;

	/* only return a certain time */
	array = up_history_copy_array_timespan (array_data, timespan);
	if (array == NULL)
		return NULL;

	/* only add a certain number of points */
	array_resolution = up_history_array_limit_resolution (array, resolution);
	g_ptr_array_unref (array);

	return array_resolution;
}

/**
 * up_history_get_profile_data:
 **/
GPtrArray *
up_history_get_profile_data (UpHistory *history, gboolean charging)
{
	guint i;
	guint non_zero_accuracy = 0;
	gfloat average = 0.0f;
	guint bin;
	guint oldbin = 999;
	UpHistoryItem *item_last = NULL;
	UpHistoryItem *item;
	UpHistoryItem *item_old = NULL;
	UpStatsItem *stats;
	GPtrArray *array;
	GPtrArray *data;
	guint time_s;
	gdouble value;
	gdouble total_value = 0.0f;

	g_return_val_if_fail (UP_IS_HISTORY (history), NULL);

	/* create 100 item list and set to zero */
	data = g_ptr_array_new ();
	for (i=0; i<101; i++) {
		stats = up_stats_item_new ();
		g_ptr_array_add (data, stats);
	}

	array = history->priv->data_charge;
	for (i=0; i<array->len; i++) {
		item = (UpHistoryItem *) g_ptr_array_index (array, i);
		if (item_last == NULL ||
		    up_history_item_get_state (item) != up_history_item_get_state (item_last)) {
			item_old = NULL;
			goto cont;
		}

		/* round to the nearest int */
		bin = rint (up_history_item_get_value (item));

		/* ensure bin is in range */
		if (bin >= data->len)
			bin = data->len - 1;

		/* different */
		if (oldbin != bin) {
			oldbin = bin;
			if (item_old != NULL) {
				/* not enough or too much difference */
				value = fabs (up_history_item_get_value (item) - up_history_item_get_value (item_old));
				if (value < 0.01f) {
					item_old = NULL;
					goto cont;
				}
				if (value > 3.0f) {
					item_old = NULL;
					goto cont;
				}

				time_s = up_history_item_get_time (item) - up_history_item_get_time (item_old);
				/* use the accuracy field as a counter for now */
				if ((charging && up_history_item_get_state (item) == UP_DEVICE_STATE_CHARGING) ||
				    (!charging && up_history_item_get_state (item) == UP_DEVICE_STATE_DISCHARGING)) {
					stats = (UpStatsItem *) g_ptr_array_index (data, bin);
					up_stats_item_set_value (stats, up_stats_item_get_value (stats) + time_s);
					up_stats_item_set_accuracy (stats, up_stats_item_get_accuracy (stats) + 1);
				}
			}
			item_old = item;
		}
cont:
		item_last = item;
	}

	/* divide the value by the number of samples to make the average */
	for (i=0; i<101; i++) {
		stats = (UpStatsItem *) g_ptr_array_index (data, i);
		if (up_stats_item_get_accuracy (stats) != 0)
			up_stats_item_set_value (stats, up_stats_item_get_value (stats) / up_stats_item_get_accuracy (stats));
	}

	/* find non-zero accuracy values for the average */
	for (i=0; i<101; i++) {
		stats = (UpStatsItem *) g_ptr_array_index (data, i);
		if (up_stats_item_get_accuracy (stats) > 0) {
			total_value += up_stats_item_get_value (stats);
			non_zero_accuracy++;
		}
	}

	/* average */
	if (non_zero_accuracy != 0)
		average = total_value / non_zero_accuracy;
	g_debug ("average is %f", average);

	/* make the values a factor of 0, so that 1.0 is twice the
	 * average, and -1.0 is half the average */
	for (i=0; i<101; i++) {
		stats = (UpStatsItem *) g_ptr_array_index (data, i);
		if (up_stats_item_get_accuracy (stats) > 0)
			up_stats_item_set_value (stats, (up_stats_item_get_value (stats) - average) / average);
		else
			up_stats_item_set_value (stats, 0.0f);
	}

	/* accuracy is a percentage scale, where each cycle = 20% */
	for (i=0; i<101; i++) {
		stats = (UpStatsItem *) g_ptr_array_index (data, i);
		up_stats_item_set_accuracy (stats, up_stats_item_get_accuracy (stats) * 20.0f);
	}

	return data;
}

/**
 * up_history_get_filename:
 **/
static gchar *
up_history_get_filename (UpHistory *history, const gchar *type)
{
	gchar *path;
	gchar *filename;

	filename = g_strdup_printf ("history-%s-%s.dat", type, history->priv->id);
	path = g_build_filename (history->priv->dir, filename, NULL);
	g_free (filename);
	return path;
}

/**
 * up_history_set_directory:
 **/
void
up_history_set_directory (UpHistory *history, const gchar *dir)
{
	g_free (history->priv->dir);
	history->priv->dir = g_strdup (dir);
	g_mkdir_with_parents (dir, 0755);
}

/**
 * up_history_array_to_file:
 * @list: a valid #GPtrArray instance
 * @filename: a filename
 *
 * Saves a copy of the list to a file
 **/
static gboolean
up_history_array_to_file (UpHistory *history, GPtrArray *list, const gchar *filename)
{
	guint i;
	UpHistoryItem *item;
	gchar *part;
	GString *string;
	gboolean ret = TRUE;
	GError *error = NULL;
	GTimeVal time_now;
	guint time_item;
	guint cull_count = 0;

	/* get current time */
	g_get_current_time (&time_now);

	/* generate data */
	string = g_string_new ("");
	for (i=0; i<list->len; i++) {
		item = g_ptr_array_index (list, i);

		/* only save entries for the last 24 hours */
		time_item = up_history_item_get_time (item);
		if (time_now.tv_sec - time_item > history->priv->max_data_age) {
			cull_count++;
			continue;
		}
		part = up_history_item_to_string (item);
		if (part == NULL) {
			ret = FALSE;
			break;
		}
		g_string_append_printf (string, "%s\n", part);
		g_free (part);
	}
	part = g_string_free (string, FALSE);

	/* how many did we kill? */
	g_debug ("culled %i of %i", cull_count, list->len);

	/* we failed to convert to string */
	if (!ret) {
		g_warning ("failed to convert");
		goto out;
	}

	/* save to disk */
	ret = g_file_set_contents (filename, part, -1, &error);
	if (!ret) {
		g_warning ("failed to set data: %s", error->message);
		g_error_free (error);
		goto out;
	}
	g_debug ("saved %s", filename);

out:
	g_free (part);
	return ret;
}

/**
 * up_history_array_from_file:
 * @list: a valid #GPtrArray instance
 * @filename: a filename
 *
 * Appends the list from a file
 **/
static gboolean
up_history_array_from_file (GPtrArray *list, const gchar *filename)
{
	gboolean ret;
	GError *error = NULL;
	gchar *data = NULL;
	gchar **parts = NULL;
	guint i;
	guint length;
	UpHistoryItem *item;

	/* do we exist */
	ret = g_file_test (filename, G_FILE_TEST_EXISTS);
	if (!ret) {
		g_debug ("failed to get data from %s as file does not exist", filename);
		goto out;
	}

	/* get contents */
	ret = g_file_get_contents (filename, &data, NULL, &error);
	if (!ret) {
		g_warning ("failed to get data: %s", error->message);
		g_error_free (error);
		goto out;
	}

	/* split by line ending */
	parts = g_strsplit (data, "\n", 0);
	length = g_strv_length (parts);
	if (length == 0) {
		g_debug ("no data in %s", filename);
		goto out;
	}

	/* add valid entries */
	g_debug ("loading %i items of data from %s", length, filename);
	for (i=0; i<length-1; i++) {
		item = up_history_item_new ();
		ret = up_history_item_set_from_string (item, parts[i]);
		if (ret)
			g_ptr_array_add (list, item);
	}

out:
	g_strfreev (parts);
	g_free (data);
	return ret;
}

/**
 * up_history_save_data:
 **/
gboolean
up_history_save_data (UpHistory *history)
{
	gboolean ret = FALSE;
	gchar *filename_rate = NULL;
	gchar *filename_charge = NULL;
	gchar *filename_time_full = NULL;
	gchar *filename_time_empty = NULL;

	/* we have an ID? */
	if (history->priv->id == NULL) {
		g_warning ("no ID, cannot save");
		goto out;
	}

	/* get filenames */
	filename_rate = up_history_get_filename (history, "rate");
	filename_charge = up_history_get_filename (history, "charge");
	filename_time_full = up_history_get_filename (history, "time-full");
	filename_time_empty = up_history_get_filename (history, "time-empty");

	/* save to disk */
	ret = up_history_array_to_file (history, history->priv->data_rate, filename_rate);
	if (!ret)
		goto out;
	ret = up_history_array_to_file (history, history->priv->data_charge, filename_charge);
	if (!ret)
		goto out;
	ret = up_history_array_to_file (history, history->priv->data_time_full, filename_time_full);
	if (!ret)
		goto out;
	ret = up_history_array_to_file (history, history->priv->data_time_empty, filename_time_empty);
	if (!ret)
		goto out;
out:
	g_free (filename_rate);
	g_free (filename_charge);
	g_free (filename_time_full);
	g_free (filename_time_empty);
	return ret;
}

/**
 * up_history_schedule_save_cb:
 **/
static gboolean
up_history_schedule_save_cb (UpHistory *history)
{
	up_history_save_data (history);
	history->priv->save_id = 0;
	return FALSE;
}

/**
 * up_history_is_low_power:
 **/
static gboolean
up_history_is_low_power (UpHistory *history)
{
	guint length;
	UpHistoryItem *item;

	/* current status is always up to date */
	if (history->priv->state != UP_DEVICE_STATE_DISCHARGING)
		return FALSE;

	/* have we got any data? */
	length = history->priv->data_charge->len;
	if (length == 0)
		return FALSE;

	/* get the last saved charge object */
	item = (UpHistoryItem *) g_ptr_array_index (history->priv->data_charge, length-1);
	if (up_history_item_get_state (item) != UP_DEVICE_STATE_DISCHARGING)
		return FALSE;

	/* high enough */
	if (up_history_item_get_value (item) > 10)
		return FALSE;

	/* we are low power */
	return TRUE;
}

/**
 * up_history_schedule_save:
 **/
static gboolean
up_history_schedule_save (UpHistory *history)
{
	gboolean ret;

	/* if low power, then don't batch up save requests */
	ret = up_history_is_low_power (history);
	if (ret) {
		g_debug ("saving directly to disk as low power");
		up_history_save_data (history);
		return TRUE;
	}

	/* we already have one saved */
	if (history->priv->save_id != 0) {
		g_debug ("deferring as others queued");
		return TRUE;
	}

	/* nothing scheduled, do new */
	g_debug ("saving in %i seconds", UP_HISTORY_SAVE_INTERVAL);
	history->priv->save_id = g_timeout_add_seconds (UP_HISTORY_SAVE_INTERVAL,
							(GSourceFunc) up_history_schedule_save_cb, history);
	g_source_set_name_by_id (history->priv->save_id, "[upower] up_history_schedule_save_cb");
	return TRUE;
}

/**
 * up_history_load_data:
 **/
static gboolean
up_history_load_data (UpHistory *history)
{
	gchar *filename;
	UpHistoryItem *item;

	/* load rate history from disk */
	filename = up_history_get_filename (history, "rate");
	up_history_array_from_file (history->priv->data_rate, filename);
	g_free (filename);

	/* load charge history from disk */
	filename = up_history_get_filename (history, "charge");
	up_history_array_from_file (history->priv->data_charge, filename);
	g_free (filename);

	/* load charge history from disk */
	filename = up_history_get_filename (history, "time-full");
	up_history_array_from_file (history->priv->data_time_full, filename);
	g_free (filename);

	/* load charge history from disk */
	filename = up_history_get_filename (history, "time-empty");
	up_history_array_from_file (history->priv->data_time_empty, filename);
	g_free (filename);

	/* save a marker so we don't use incomplete percentages */
	item = up_history_item_new ();
	up_history_item_set_time_to_present (item);
	g_ptr_array_add (history->priv->data_rate, g_object_ref (item));
	g_ptr_array_add (history->priv->data_charge, g_object_ref (item));
	g_ptr_array_add (history->priv->data_time_full, g_object_ref (item));
	g_ptr_array_add (history->priv->data_time_empty, g_object_ref (item));
	g_object_unref (item);
	up_history_schedule_save (history);

	return TRUE;
}

/**
 * up_history_set_id:
 **/
gboolean
up_history_set_id (UpHistory *history, const gchar *id)
{
	gboolean ret;

	g_return_val_if_fail (UP_IS_HISTORY (history), FALSE);

	if (history->priv->id != NULL)
		return FALSE;
	if (id == NULL)
		return FALSE;

	g_debug ("using id: %s", id);
	history->priv->id = g_strdup (id);
	/* load all previous data */
	ret = up_history_load_data (history);
	return ret;
}

/**
 * up_history_set_state:
 **/
gboolean
up_history_set_state (UpHistory *history, UpDeviceState state)
{
	g_return_val_if_fail (UP_IS_HISTORY (history), FALSE);

	if (history->priv->id == NULL)
		return FALSE;
	history->priv->state = state;
	return TRUE;
}

/**
 * up_history_set_charge_data:
 **/
gboolean
up_history_set_charge_data (UpHistory *history, gdouble percentage)
{
	UpHistoryItem *item;

	g_return_val_if_fail (UP_IS_HISTORY (history), FALSE);

	if (history->priv->id == NULL)
		return FALSE;
	if (history->priv->state == UP_DEVICE_STATE_UNKNOWN)
		return FALSE;
	if (history->priv->percentage_last == percentage)
		return FALSE;

	/* add to array and schedule save file */
	item = up_history_item_new ();
	up_history_item_set_time_to_present (item);
	up_history_item_set_value (item, percentage);
	up_history_item_set_state (item, history->priv->state);
	g_ptr_array_add (history->priv->data_charge, item);
	up_history_schedule_save (history);

	/* save last value */
	history->priv->percentage_last = percentage;

	return TRUE;
}

/**
 * up_history_set_rate_data:
 **/
gboolean
up_history_set_rate_data (UpHistory *history, gdouble rate)
{
	UpHistoryItem *item;

	g_return_val_if_fail (UP_IS_HISTORY (history), FALSE);

	if (history->priv->id == NULL)
		return FALSE;
	if (history->priv->state == UP_DEVICE_STATE_UNKNOWN)
		return FALSE;
	if (history->priv->rate_last == rate)
		return FALSE;

	/* add to array and schedule save file */
	item = up_history_item_new ();
	up_history_item_set_time_to_present (item);
	up_history_item_set_value (item, rate);
	up_history_item_set_state (item, history->priv->state);
	g_ptr_array_add (history->priv->data_rate, item);
	up_history_schedule_save (history);

	/* save last value */
	history->priv->rate_last = rate;

	return TRUE;
}

/**
 * up_history_set_time_full_data:
 **/
gboolean
up_history_set_time_full_data (UpHistory *history, gint64 time_s)
{
	UpHistoryItem *item;

	g_return_val_if_fail (UP_IS_HISTORY (history), FALSE);

	if (history->priv->id == NULL)
		return FALSE;
	if (history->priv->state == UP_DEVICE_STATE_UNKNOWN)
		return FALSE;
	if (time_s < 0)
		return FALSE;
	if (history->priv->time_full_last == time_s)
		return FALSE;

	/* add to array and schedule save file */
	item = up_history_item_new ();
	up_history_item_set_time_to_present (item);
	up_history_item_set_value (item, (gdouble) time_s);
	up_history_item_set_state (item, history->priv->state);
	g_ptr_array_add (history->priv->data_time_full, item);
	up_history_schedule_save (history);

	/* save last value */
	history->priv->time_full_last = time_s;

	return TRUE;
}

/**
 * up_history_set_time_empty_data:
 **/
gboolean
up_history_set_time_empty_data (UpHistory *history, gint64 time_s)
{
	UpHistoryItem *item;

	g_return_val_if_fail (UP_IS_HISTORY (history), FALSE);

	if (history->priv->id == NULL)
		return FALSE;
	if (history->priv->state == UP_DEVICE_STATE_UNKNOWN)
		return FALSE;
	if (time_s < 0)
		return FALSE;
	if (history->priv->time_empty_last == time_s)
		return FALSE;

	/* add to array and schedule save file */
	item = up_history_item_new ();
	up_history_item_set_time_to_present (item);
	up_history_item_set_value (item, (gdouble) time_s);
	up_history_item_set_state (item, history->priv->state);
	g_ptr_array_add (history->priv->data_time_empty, item);
	up_history_schedule_save (history);

	/* save last value */
	history->priv->time_empty_last = time_s;

	return TRUE;
}

/**
 * up_history_class_init:
 * @klass: The UpHistoryClass
 **/
static void
up_history_class_init (UpHistoryClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = up_history_finalize;
	g_type_class_add_private (klass, sizeof (UpHistoryPrivate));
}

/**
 * up_history_init:
 * @history: This class instance
 **/
static void
up_history_init (UpHistory *history)
{
	history->priv = UP_HISTORY_GET_PRIVATE (history);
	history->priv->data_rate = g_ptr_array_new_with_free_func ((GDestroyNotify) g_object_unref);
	history->priv->data_charge = g_ptr_array_new_with_free_func ((GDestroyNotify) g_object_unref);
	history->priv->data_time_full = g_ptr_array_new_with_free_func ((GDestroyNotify) g_object_unref);
	history->priv->data_time_empty = g_ptr_array_new_with_free_func ((GDestroyNotify) g_object_unref);
	history->priv->max_data_age = UP_HISTORY_DEFAULT_MAX_DATA_AGE;

	up_history_set_directory (history, HISTORY_DIR);
}

/**
 * up_history_finalize:
 * @object: The object to finalize
 **/
static void
up_history_finalize (GObject *object)
{
	UpHistory *history;

	g_return_if_fail (UP_IS_HISTORY (object));

	history = UP_HISTORY (object);

	/* save */
	if (history->priv->save_id > 0)
		g_source_remove (history->priv->save_id);
	if (history->priv->id != NULL)
		up_history_save_data (history);

	g_ptr_array_unref (history->priv->data_rate);
	g_ptr_array_unref (history->priv->data_charge);
	g_ptr_array_unref (history->priv->data_time_full);
	g_ptr_array_unref (history->priv->data_time_empty);

	g_free (history->priv->id);
	g_free (history->priv->dir);

	g_return_if_fail (history->priv != NULL);

	G_OBJECT_CLASS (up_history_parent_class)->finalize (object);
}

/**
 * up_history_new:
 *
 * Return value: a new UpHistory object.
 **/
UpHistory *
up_history_new (void)
{
	return g_object_new (UP_TYPE_HISTORY, NULL);
}

