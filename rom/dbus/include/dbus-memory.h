/* -*- mode: C; c-file-style: "gnu" -*- */
/* dbus-memory.h  D-BUS memory handling
 *
 * Copyright (C) 2002  Red Hat Inc.
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

#ifndef DBUS_MEMORY_H
#define DBUS_MEMORY_H

#include <dbus/dbus-macros.h>
#include <stddef.h>

DBUS_BEGIN_DECLS;

void* dbus_malloc        (size_t bytes);
void* dbus_malloc0       (size_t bytes);
void* dbus_realloc       (void  *memory,
                          size_t bytes);
void  dbus_free          (void  *memory);

#define dbus_new(type, count)  ((type*)dbus_malloc (sizeof (type) * (count)));
#define dbus_new0(type, count) ((type*)dbus_malloc0 (sizeof (type) * (count)));

void dbus_free_string_array (char **str_array);

typedef void (* DBusFreeFunction) (void *memory);

void dbus_shutdown (void);

DBUS_END_DECLS;

#endif /* DBUS_MEMORY_H */
