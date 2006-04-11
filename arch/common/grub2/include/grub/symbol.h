/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002  Free Software Foundation, Inc.
 *
 *  GRUB is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef GRUB_SYMBOL_HEADER
#define GRUB_SYMBOL_HEADER	1

#include <config.h>

/* Add an underscore to a C symbol in assembler code if needed. */
#ifdef HAVE_ASM_USCORE
# define EXT_C(sym)	_ ## sym
#else
# define EXT_C(sym)	sym
#endif

#define FUNCTION(x)	.globl EXT_C(x) ; .type EXT_C(x), @function ; EXT_C(x):
#define VARIABLE(x)	.globl EXT_C(x) ; .type EXT_C(x), @object ; EXT_C(x):

/* Mark an exported symbol.  */
#define EXPORT_FUNC(x)	x
#define EXPORT_VAR(x)	x

#endif /* ! GRUB_SYMBOL_HEADER */
