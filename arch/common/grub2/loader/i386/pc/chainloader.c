/* chainloader.c - boot another boot loader */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2004  Free Software Foundation, Inc.
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

#include <grub/loader.h>
#include <grub/machine/loader.h>
#include <grub/machine/chainloader.h>
#include <grub/file.h>
#include <grub/err.h>
#include <grub/device.h>
#include <grub/disk.h>
#include <grub/misc.h>
#include <grub/types.h>
#include <grub/machine/init.h>
#include <grub/partition.h>
#include <grub/machine/memory.h>
#include <grub/rescue.h>
#include <grub/dl.h>

static grub_dl_t my_mod;

static grub_err_t
grub_chainloader_boot (void)
{
  grub_device_t dev;
  int drive = -1;
  void *part_addr = 0;
  
  /* Open the root device.  */
  dev = grub_device_open (0);
  if (dev)
    {
      grub_disk_t disk = dev->disk;
      
      if (disk)
	{
	  grub_partition_t p = disk->partition;
	  
	  /* In i386-pc, the id is equal to the BIOS drive number.  */
	  drive = (int) disk->id;

	  if (p)
	    {
	      grub_disk_read (disk, p->offset, 446, 64,
			      (char *) GRUB_MEMORY_MACHINE_PART_TABLE_ADDR);
	      
	      /* Ignore errors. Perhaps it's not fatal.  */
	      part_addr = (void *) (GRUB_MEMORY_MACHINE_PART_TABLE_ADDR
				    + (p->index << 4));
	    }
	}

      grub_device_close (dev);
    }

  grub_chainloader_real_boot (drive, part_addr);

  /* Never reach here.  */
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_chainloader_unload (void)
{
  grub_dl_unref (my_mod);
  return GRUB_ERR_NONE;
}

void
grub_chainloader_cmd (const char *filename, grub_chainloader_flags_t flags)
{
  grub_file_t file = 0;
  grub_uint16_t signature;

  grub_dl_ref (my_mod);
  
  file = grub_file_open (filename);
  if (! file)
    goto fail;

  /* Read the first block.  */
  if (grub_file_read (file, (char *) 0x7C00, GRUB_DISK_SECTOR_SIZE)
      != GRUB_DISK_SECTOR_SIZE)
    {
      if (grub_errno == GRUB_ERR_NONE)
	grub_error (GRUB_ERR_BAD_OS, "too small");

      goto fail;
    }

  /* Check the signature.  */
  signature = *((grub_uint16_t *) (0x7C00 + GRUB_DISK_SECTOR_SIZE - 2));
  if (signature != grub_le_to_cpu16 (0xaa55)
      && ! (flags & GRUB_CHAINLOADER_FORCE))
    {
      grub_error (GRUB_ERR_BAD_OS, "invalid signature");
      goto fail;
    }

  grub_file_close (file);
  grub_loader_set (grub_chainloader_boot, grub_chainloader_unload);
  return;
  
 fail:

  if (file)
    grub_file_close (file);
  
  grub_dl_unref (my_mod);
}

static void
grub_rescue_cmd_chainloader (int argc, char *argv[])
{
  grub_chainloader_flags_t flags = 0;

  if (argc > 0 && grub_strcmp (argv[0], "--force") == 0)
    {
      flags |= GRUB_CHAINLOADER_FORCE;
      argc--;
      argv++;
    }
  
  if (argc == 0)
    grub_error (GRUB_ERR_BAD_ARGUMENT, "no file specified");
  else
    grub_chainloader_cmd (argv[0], flags);
}

static const char loader_name[] = "chainloader";

GRUB_MOD_INIT
{
  grub_rescue_register_command (loader_name,
				grub_rescue_cmd_chainloader,
				"load another boot loader");
  my_mod = mod;
}

GRUB_MOD_FINI
{
  grub_rescue_unregister_command (loader_name);
}
