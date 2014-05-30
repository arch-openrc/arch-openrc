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

#if !defined (__UPOWER_H_INSIDE__) && !defined (UP_COMPILATION)
#error "Only <upower.h> can be included directly."
#endif

#ifndef __UP_TYPES_H
#define __UP_TYPES_H

#include <glib-object.h>

G_BEGIN_DECLS

/**
 * UpDeviceKind:
 *
 * The device type.
 **/
typedef enum {
	UP_DEVICE_KIND_UNKNOWN,
	UP_DEVICE_KIND_LINE_POWER,
	UP_DEVICE_KIND_BATTERY,
	UP_DEVICE_KIND_UPS,
	UP_DEVICE_KIND_MONITOR,
	UP_DEVICE_KIND_MOUSE,
	UP_DEVICE_KIND_KEYBOARD,
	UP_DEVICE_KIND_PDA,
	UP_DEVICE_KIND_PHONE,
	UP_DEVICE_KIND_MEDIA_PLAYER,
	UP_DEVICE_KIND_TABLET,
	UP_DEVICE_KIND_COMPUTER,
	UP_DEVICE_KIND_LAST
} UpDeviceKind;

/**
 * UpDeviceState:
 *
 * The device state.
 **/
typedef enum {
	UP_DEVICE_STATE_UNKNOWN,
	UP_DEVICE_STATE_CHARGING,
	UP_DEVICE_STATE_DISCHARGING,
	UP_DEVICE_STATE_EMPTY,
	UP_DEVICE_STATE_FULLY_CHARGED,
	UP_DEVICE_STATE_PENDING_CHARGE,
	UP_DEVICE_STATE_PENDING_DISCHARGE,
	UP_DEVICE_STATE_LAST
} UpDeviceState;

/**
 * UpDeviceTechnology:
 *
 * The device technology.
 **/
typedef enum {
	UP_DEVICE_TECHNOLOGY_UNKNOWN,
	UP_DEVICE_TECHNOLOGY_LITHIUM_ION,
	UP_DEVICE_TECHNOLOGY_LITHIUM_POLYMER,
	UP_DEVICE_TECHNOLOGY_LITHIUM_IRON_PHOSPHATE,
	UP_DEVICE_TECHNOLOGY_LEAD_ACID,
	UP_DEVICE_TECHNOLOGY_NICKEL_CADMIUM,
	UP_DEVICE_TECHNOLOGY_NICKEL_METAL_HYDRIDE,
	UP_DEVICE_TECHNOLOGY_LAST
} UpDeviceTechnology;

/**
 * UpDeviceLevel:
 *
 * The warning level of a battery.
 **/
typedef enum {
	UP_DEVICE_LEVEL_UNKNOWN,
	UP_DEVICE_LEVEL_NONE,
	UP_DEVICE_LEVEL_DISCHARGING,
	UP_DEVICE_LEVEL_LOW,
	UP_DEVICE_LEVEL_CRITICAL,
	UP_DEVICE_LEVEL_ACTION,
	UP_DEVICE_LEVEL_LAST
} UpDeviceLevel;

const gchar	*up_device_kind_to_string		(UpDeviceKind		 type_enum);
const gchar	*up_device_state_to_string		(UpDeviceState		 state_enum);
const gchar	*up_device_technology_to_string		(UpDeviceTechnology	 technology_enum);
const gchar	*up_device_level_to_string		(UpDeviceLevel		 level_enum);
UpDeviceKind	 up_device_kind_from_string		(const gchar		*type);
UpDeviceState	 up_device_state_from_string		(const gchar		*state);
UpDeviceTechnology up_device_technology_from_string	(const gchar		*technology);
UpDeviceLevel	 up_device_level_from_string		(const gchar		*level);

G_END_DECLS

#endif /* __UP_TYPES_H */

