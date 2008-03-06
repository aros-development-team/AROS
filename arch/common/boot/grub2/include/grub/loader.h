/* loader.h - OS loaders */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2004,2006,2007  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GRUB_LOADER_HEADER
#define GRUB_LOADER_HEADER	1

#include <grub/file.h>
#include <grub/symbol.h>
#include <grub/err.h>
#include <grub/types.h>

/* Check if a loader is loaded.  */
int EXPORT_FUNC(grub_loader_is_loaded) (void);

/* Set loader functions. NORETURN must be set to true, if BOOT won't return
   to the original state.  */
void EXPORT_FUNC(grub_loader_set) (grub_err_t (*boot) (void),
				   grub_err_t (*unload) (void),
				   int noreturn);

/* Unset current loader, if any.  */
void EXPORT_FUNC(grub_loader_unset) (void);

/* Call the boot hook in current loader. This may or may not return,
   depending on the setting by grub_loader_set.  */
grub_err_t EXPORT_FUNC(grub_loader_boot) (void);

#endif /* ! GRUB_LOADER_HEADER */
