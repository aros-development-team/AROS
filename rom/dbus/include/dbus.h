/* -*- mode: C; c-file-style: "gnu" -*- */
/* dbus.h  Convenience header including all other headers
 *
 * Copyright (C) 2002, 2003  Red Hat Inc.
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

#ifndef DBUS_H
#define DBUS_H

#define DBUS_INSIDE_DBUS_H 1

#ifndef DBUS_API_SUBJECT_TO_CHANGE
#error "Please define DBUS_API_SUBJECT_TO_CHANGE to acknowledge your understanding that D-BUS hasn't reached 1.0 and is subject to protocol and API churn. See the README for a full explanation."
#endif

#include <dbus/dbus-arch-deps.h>
#include <dbus/dbus-address.h>
#include <dbus/dbus-bus.h>
#include <dbus/dbus-connection.h>
#include <dbus/dbus-errors.h>
#include <dbus/dbus-macros.h>
#include <dbus/dbus-message.h>
#include <dbus/dbus-pending-call.h>
#include <dbus/dbus-protocol.h>
#include <dbus/dbus-server.h>
#include <dbus/dbus-shared.h>
#include <dbus/dbus-threads.h>
#include <dbus/dbus-types.h>

#undef DBUS_INSIDE_DBUS_H

/**
 * @defgroup DBus D-BUS message system public API
 * @brief The exported public API of the D-BUS library.
 *
 * @{
 */

/** @} */


#endif /* DBUS_H */
