/* -*- mode: C; c-file-style: "gnu" -*- */
/* dbus-errors.h Error reporting
 *
 * Copyright (C) 2002  Red Hat Inc.
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

#ifndef DBUS_ERROR_H
#define DBUS_ERROR_H

#include <dbus/dbus-macros.h>
#include <dbus/dbus-types.h>

DBUS_BEGIN_DECLS;

typedef struct DBusError DBusError;

/**
 * Object representing an exception.
 */
struct DBusError
{
  const char *name;    /**< error name */
  const char *message; /**< error message */

  unsigned int dummy1 : 1; /**< placeholder */
  unsigned int dummy2 : 1; /**< placeholder */
  unsigned int dummy3 : 1; /**< placeholder */
  unsigned int dummy4 : 1; /**< placeholder */
  unsigned int dummy5 : 1; /**< placeholder */

  void *padding1; /**< placeholder */
};

void        dbus_error_init      (DBusError       *error);
void        dbus_error_free      (DBusError       *error);
void        dbus_set_error       (DBusError       *error,
                                  const char      *name,
                                  const char      *message,
                                  ...);
void        dbus_set_error_const (DBusError       *error,
                                  const char      *name,
                                  const char      *message);
void        dbus_move_error      (DBusError       *src,
                                  DBusError       *dest);
dbus_bool_t dbus_error_has_name  (const DBusError *error,
                                  const char      *name);
dbus_bool_t dbus_error_is_set    (const DBusError *error);

DBUS_END_DECLS;

#endif /* DBUS_ERROR_H */
