/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Partial file for Zune - Flavio Stanchina
 */

#ifndef __G_LIB_H__
#define __G_LIB_H__


#include <aros/debug.h> /* kprintf */


#define G_DIR_SEPARATOR '/'
#define G_DIR_SEPARATOR_S "/"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#undef	MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

#undef	MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))

#undef	ABS
#define ABS(a)	   (((a) < 0) ? -(a) : (a))

#undef	CLAMP
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))


#ifdef G_DISABLE_CHECKS

#define g_return_if_fail(expr)
#define g_return_val_if_fail(expr,val)

#else /* !G_DISABLE_CHECKS */

#define g_return_if_fail(expr)		({			\
     if (!(expr))							\
       {								\
	 kprintf("file %s: line %d (%s): assertion `%s' failed.",	\
		__FILE__, __LINE__, __PRETTY_FUNCTION__, #expr);	\
	 return;							\
       };				})

#define g_return_val_if_fail(expr,val)	({				\
     if (!(expr))							\
       {								\
	 kprintf("file %s: line %d (%s): assertion `%s' failed.",	\
		__FILE__, __LINE__, __PRETTY_FUNCTION__, #expr);	\
	 return val;							\
       };				})

#endif /* !G_DISABLE_CHECKS */


/* Logging mechanism
 */
#define g_error(format, args...)   kprintf("ERROR: " format , ## args)
#define g_warning(format, args...) kprintf("WARNING: " format , ## args)

#define g_print(format, args...)          kprintf(format , ## args)
#define g_printerr(format, args...)       kprintf("ERR: " format , ## args)


STRPTR g_strconcat(CONST_STRPTR string1, ...); /* NULL terminated */


void g_atexit(void (*func)(void));


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __G_LIB_H__ */
