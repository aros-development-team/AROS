/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2004  Free Software Foundation, Inc.
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

#include <grub/loader.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/err.h>
#include <grub/kernel.h>

static grub_err_t (*grub_loader_boot_func) (void);
static grub_err_t (*grub_loader_unload_func) (void);

static int grub_loader_loaded;

int
grub_loader_is_loaded (void)
{
  return grub_loader_loaded;
}

void
grub_loader_set (grub_err_t (*boot) (void),
		 grub_err_t (*unload) (void))
{
  if (grub_loader_loaded && grub_loader_unload_func)
    grub_loader_unload_func ();
  
  grub_loader_boot_func = boot;
  grub_loader_unload_func = unload;

  grub_loader_loaded = 1;
}

void
grub_loader_unset(void)
{
  if (grub_loader_loaded && grub_loader_unload_func)
    grub_loader_unload_func ();
  
  grub_loader_boot_func = 0;
  grub_loader_unload_func = 0;

  grub_loader_loaded = 0;
}

grub_err_t
grub_loader_boot (void)
{
  if (! grub_loader_loaded)
    return grub_error (GRUB_ERR_NO_KERNEL, "no loaded kernel");

  grub_machine_fini ();
  
  return (grub_loader_boot_func) ();
}

