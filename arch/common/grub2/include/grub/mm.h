/* mm.h - prototypes and declarations for memory manager */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002  Free Software Foundation, Inc.
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

#ifndef GRUB_MM_H
#define GRUB_MM_H	1

#include <grub/types.h>
#include <grub/symbol.h>

#ifndef NULL
# define NULL	((void *) 0)
#endif

void grub_mm_init_region (void *addr, grub_size_t size);
void *EXPORT_FUNC(grub_malloc) (grub_size_t size);
void EXPORT_FUNC(grub_free) (void *ptr);
void *EXPORT_FUNC(grub_realloc) (void *ptr, grub_size_t size);
void *EXPORT_FUNC(grub_memalign) (grub_size_t align, grub_size_t size);

/* For debugging.  */
#define MM_DEBUG       1
#if MM_DEBUG
void grub_mm_dump (unsigned lineno);
#endif

#endif /* ! GRUB_MM_H */
