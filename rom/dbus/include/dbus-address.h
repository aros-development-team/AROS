/* -*- mode: C; c-file-style: "gnu" -*- */
/* dbus-address.h  Server address parser.
 *
 * Copyright (C) 2003  CodeFactory AB
 *
 * Licensed under the Academic Free License version 2.1
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#if !defined (DBUS_INSIDE_DBUS_H) && !defined (DBUS_COMPILATION)
#error "Only <dbus/dbus.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef DBUS_ADDRESS_H
#define DBUS_ADDRESS_H

#include <dbus/dbus-types.h>
#include <dbus/dbus-errors.h>

typedef struct DBusAddressEntry DBusAddressEntry;

dbus_bool_t dbus_parse_address            (const char         *address,
					   DBusAddressEntry ***entry,
					   int                *array_len,
					   DBusError          *error);
const char *dbus_address_entry_get_value  (DBusAddressEntry   *entry,
					   const char         *key);
const char *dbus_address_entry_get_method (DBusAddressEntry   *entry);
void        dbus_address_entries_free     (DBusAddressEntry  **entries);




#endif /* DBUS_ADDRESS_H */

