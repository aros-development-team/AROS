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
#include <grub/kernel.h>

void
grub_efi_init (void)
{
  /* First of all, initialize the console so that GRUB can display
     messages.  */
  grub_console_init ();

  /* Initialize the memory management system.  */
  grub_efi_mm_init ();

  efi_call_4 (grub_efi_system_table->boot_services->set_watchdog_timer,
	      0, 0, 0, NULL);

  grub_efidisk_init ();
}

void
grub_efi_set_prefix (void)
{
  grub_efi_loaded_image_t *image = NULL;
  char *device = NULL;
  char *path = NULL;

  {
    char *pptr = NULL;
    if (grub_prefix[0] == '(')
      {
	pptr = grub_strrchr (grub_prefix, ')');
	if (pptr)
	  {
	    device = grub_strndup (grub_prefix + 1, pptr - grub_prefix - 1);
	    pptr++;
	  }
      }
    if (!pptr)
      pptr = grub_prefix;
    if (pptr[0])
      path = grub_strdup (pptr);
  }

  if ((!device || device[0] == ',' || !device[0]) || !path)
    image = grub_efi_get_loaded_image (grub_efi_image_handle);
  if (image)
    {
      if (!device)
	device = grub_efidisk_get_device_name (image->device_handle);
      else if (device[0] == ',' || !device[0])
	{
	  /* We have a partition, but still need to fill in the drive.  */
	  char *image_device, *comma, *new_device;

	  image_device = grub_efidisk_get_device_name (image->device_handle);
	  comma = grub_strchr (image_device, ',');
	  if (comma)
	    {
	      char *drive = grub_strndup (image_device, comma - image_device);
	      new_device = grub_xasprintf ("%s%s", drive, device);
	      grub_free (drive);
	    }
	  else
	    new_device = grub_xasprintf ("%s%s", image_device, device);

	  grub_free (image_device);
	  grub_free (device);
	  device = new_device;
	}
    }

  if (image && !path)
    {
      char *p;

      path = grub_efi_get_filename (image->file_path);

      /* Get the directory.  */
      p = grub_strrchr (path, '/');
      if (p)
	*p = '\0';
    }

  if (device && path)
    {
      char *prefix;

      prefix = grub_xasprintf ("(%s)%s", device, path);
      if (prefix)
	{
	  grub_env_set ("prefix", prefix);
	  grub_free (prefix);
	}
    }

  grub_free (device);
  grub_free (path);
}

void
grub_efi_fini (void)
{
  grub_efidisk_fini ();
  grub_console_fini ();
}
