/* init.c - generic EFI initialization and finalization */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2006,2007  Free Software Foundation, Inc.
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

#include <grub/efi/efi.h>
#include <grub/efi/console.h>
#include <grub/efi/disk.h>
#include <grub/term.h>
#include <grub/misc.h>
#include <grub/env.h>
#include <grub/mm.h>
#include <grub/machine/kernel.h>

void
grub_efi_init (void)
{
  /* First of all, initialize the console so that GRUB can display
     messages.  */
  grub_console_init ();

  /* Initialize the memory management system.  */
  grub_efi_mm_init ();

  grub_efidisk_init ();
}

void
grub_efi_set_prefix (void)
{
  grub_efi_loaded_image_t *image;

  image = grub_efi_get_loaded_image (grub_efi_image_handle);
  if (image)
    {
      char *device;
      char *file;

      device = grub_efidisk_get_device_name (image->device_handle);
      file = grub_efi_get_filename (image->file_path);

      if (device && file)
	{
	  char *p;
	  char *prefix;

	  /* Get the directory.  */
	  p = grub_strrchr (file, '/');
	  if (p)
	    *p = '\0';

	  prefix = grub_malloc (1 + grub_strlen (device) + 1
				+ grub_strlen (file) + 1);
	  if (prefix)
	    {
	      grub_sprintf (prefix, "(%s)%s", device, file);
	      grub_env_set ("prefix", prefix);
	      grub_free (prefix);
	    }
	}

      grub_free (device);
      grub_free (file);
    }
}

void
grub_efi_fini (void)
{
  grub_efidisk_fini ();
  grub_efi_mm_fini ();
  grub_console_fini ();
}
