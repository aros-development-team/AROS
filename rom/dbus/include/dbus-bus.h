/* -*- mode: C; c-file-style: "gnu" -*- */
/* dbus-bus.h  Convenience functions for communicating with the bus.
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

#ifndef DBUS_BUS_H
#define DBUS_BUS_H

#include <dbus/dbus-connection.h>

DBUS_BEGIN_DECLS;

DBusConnection *dbus_bus_get              (DBusBusType     type,
					   DBusError      *error);
dbus_bool_t     dbus_bus_register         (DBusConnection *connection,
					   DBusError      *error);
dbus_bool_t     dbus_bus_set_base_service (DBusConnection *connection,
					   const char     *base_service);
const char*     dbus_bus_get_base_service (DBusConnection *connection);
unsigned long   dbus_bus_get_unix_user    (DBusConnection *connection,
			                   const char     *service,
                                           DBusError      *error);
int             dbus_bus_acquire_service  (DBusConnection *connection,
					   const char     *service_name,
					   unsigned int    flags,
					   DBusError      *error);
dbus_bool_t     dbus_bus_service_exists   (DBusConnection *connection,
					   const char     *service_name,
					   DBusError      *error);

dbus_bool_t     dbus_bus_activate_service (DBusConnection *connection,
					   const char     *service_name,
					   dbus_uint32_t   flags,
					   dbus_uint32_t  *reply,
					   DBusError      *error);

void            dbus_bus_add_match        (DBusConnection *connection,
                                           const char     *rule,
                                           DBusError      *error);
void            dbus_bus_remove_match     (DBusConnection *connection,
                                           const char     *rule,
                                           DBusError      *error);

DBUS_END_DECLS;

#endif /* DBUS_BUS_H */
