/* loader.h - OS loaders */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2004  Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef GRUB_LOADER_HEADER
#define GRUB_LOADER_HEADER	1

#include <grub/file.h>
#include <grub/symbol.h>
#include <grub/err.h>
#include <grub/types.h>

int EXPORT_FUNC(grub_loader_is_loaded) (void);
void EXPORT_FUNC(grub_loader_set) (grub_err_t (*boot) (void),
				   grub_err_t (*unload) (void));
void EXPORT_FUNC(grub_loader_unset) (void);

grub_err_t EXPORT_FUNC(grub_loader_boot) (void);

#endif /* ! GRUB_LOADER_HEADER */
