/*

$VER: initializers.h 41.5 (30.9.2007)
Copyright (C) 2000-2007 Neil Cafferkey

This file is free software; you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation; either version 2.1 of the
License, or (at your option) any later version.

This file is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this file; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA.

*/

#ifndef EXEC_INITIALIZERS_H
#define EXEC_INITIALIZERS_H

#include <exec/types.h>


/* Use the OFFSET macro as the offset parameter of one of the other
   macros */
/*
#ifdef OFFSET
#undef OFFSET
#endif
*/
#define OFFSET(struct_name, struct_field) \
   ((UPINT)(&(((struct struct_name *)0)->struct_field)))

/* Use the following macros in the structure definition */

#define INITBYTEDEF(name) \
   UBYTE name ## _c; UBYTE name ## _o1; UBYTE name ## _o2; \
   UBYTE name ## _o3; UBYTE name ## _v; UBYTE name ## _p
#define INITWORDDEF(name) \
   UBYTE name ## _c; UBYTE name ## _o1; UBYTE name ## _o2; \
   UWORD name ## _v
#define INITLONGDEF(name) \
   UBYTE name ## _c; UBYTE name ## _o1; UBYTE name ## _o2; \
   ULONG name ## _v
#define INITPINTDEF(name) \
   UBYTE name ## _c; UBYTE name ## _o1; UBYTE name ## _o2; \
   UPINT name ## _v

#define SMALLINITBYTEDEF(name) \
   UBYTE name ## _c; UBYTE name ## _o; UBYTE name ## _v; UBYTE name ## _p
#define SMALLINITWORDDEF(name) \
   UBYTE name ## _c; UBYTE name ## _o; UWORD name ## _v
#define SMALLINITLONGDEF(name) \
   UBYTE name ## _c; UBYTE name ## _o; ULONG name ## _v
#define SMALLINITPINTDEF(name) \
   UBYTE name ## _c; UBYTE name ## _o; UPINT name ## _v

#define INITENDDEF UBYTE the_end

/* Private macro */

#define PINTSIZECODE (((sizeof(PINT) == sizeof(ULONG)) ? 0 : 3) << 4)

/* Use the following macros to fill in a structure */

#define NEWINITBYTE(offset, value) \
   0xe0, (UBYTE)((offset) >> 16), (UBYTE)((offset) >> 8), (UBYTE)(offset), \
   (UBYTE)(value), 0
#define NEWINITWORD(offset, value) \
   0xd0, (UBYTE)((offset) >> 16), (UBYTE)((offset) >> 8), (UBYTE)(offset), \
   (UWORD)(value)
#define NEWINITLONG(offset, value) \
   0xc0, (UBYTE)((offset) >> 16), (UBYTE)((offset) >> 8), (UBYTE)(offset), \
   (ULONG)(value)
#define INITPINT(offset, value) \
   0xc0 | PINTSIZECODE, (UBYTE)((offset) >> 16), (UBYTE)((offset) >> 8), \
   (UBYTE)(offset), (UPINT)(value)

#define SMALLINITBYTE(offset, value) \
   0xa0, (offset), (UBYTE)(value), 0
#define SMALLINITWORD(offset, value) \
   0x90, (offset), (UWORD)(value)
#define SMALLINITLONG(offset, value) \
   0x80, (offset), (ULONG)(value)
#define SMALLINITPINT(offset, value) \
   0x80 | PINTSIZECODE, (offset), (UPINT)(value)

#define INITEND 0

/* Obsolete definitions */

#define INITBYTE(offset, value) \
   (0xe000 | ((UWORD)(offset) >> 16)), \
   (UWORD)(offset), (UWORD)((value) << 8)
#define INITWORD(offset, value) \
   (0xd000 | ((UWORD)(offset) >> 16)), (UWORD)(offset), (UWORD)(value)
#define INITLONG(offset, value) \
   (0xc000 | ((UWORD)(offset) >> 16)), (UWORD)(offset), \
   (UWORD)((value) >> 16), (UWORD)(value)
#define INITAPTR(offset, value) \
   (0xc000 | ((UWORD)(offset) >> 16)), (UWORD)(offset), \
   (UWORD)((ULONG)(value) >> 16), (UWORD)(value)

#endif
