/* -*- mode: C; c-file-style: "gnu" -*- */
/* dbus-macros.h  generic macros
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

#ifndef DBUS_MACROS_H
#define DBUS_MACROS_H

#ifdef  __cplusplus
#  define DBUS_BEGIN_DECLS  extern "C" {
#  define DBUS_END_DECLS    }
#else
#  define DBUS_BEGIN_DECLS
#  define DBUS_END_DECLS
#endif

#ifndef TRUE
#  define TRUE 1
#endif
#ifndef FALSE
#  define FALSE 0
#endif

#ifndef NULL
#  ifdef __cplusplus
#    define NULL        (0L)
#  else /* !__cplusplus */
#    define NULL        ((void*) 0)
#  endif /* !__cplusplus */
#endif

/* Normally docs are in .c files, but there isn't a .c file for this. */
/**
 * @defgroup DBusMacros Utility macros
 * @ingroup  DBus
 * @brief #TRUE, #FALSE, #NULL, and so on
 *
 * Utility macros.
 *
 * @{
 */

/**
 * @def DBUS_BEGIN_DECLS
 *
 * Macro used prior to declaring functions in the D-BUS header
 * files. Expands to "extern "C"" when using a C++ compiler,
 * and expands to nothing when using a C compiler.
 */
/**
 * @def DBUS_END_DECLS
 *
 * Macro used after declaring functions in the D-BUS header
 * files. Expands to "}" when using a C++ compiler,
 * and expands to nothing when using a C compiler.
 */
/**
 * @def TRUE
 *
 * Expands to "1"
 */
/**
 * @def FALSE
 *
 * Expands to "0"
 */
/**
 * @def NULL
 *
 * A null pointer, defined appropriately for C or C++.
 */

/** }@ */

#endif /* DBUS_MACROS_H */
