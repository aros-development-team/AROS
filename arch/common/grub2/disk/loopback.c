/* loopback.c - command to add loopback devices.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005  Free Software Foundation, Inc.
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

#include <grub/normal.h>
#include <grub/dl.h>
#include <grub/arg.h>
#include <grub/misc.h>
#include <grub/file.h>
#include <grub/disk.h>
#include <grub/mm.h>

struct grub_loopback
{
  char *devname;
  char *filename;
  int has_partitions;
  struct grub_loopback *next;
};

static struct grub_loopback *loopback_list; 

static const struct grub_arg_option options[] =
  {
    {"delete", 'd', 0, "delete the loopback device entry", 0, 0},
    {"partitions", 'p', 0, "set that the drive has partitions to"
     " simulate a harddrive", 0, 0},
    {0, 0, 0, 0, 0, 0}
  };

/* Delete the loopback device NAME.  */
static grub_err_t
delete_loopback (const char *name)
{
  struct grub_loopback *dev;
  struct grub_loopback **prev;

  /* Search for the device.  */
  for (dev = loopback_list, prev = &loopback_list;
       dev; prev = &dev->next, dev = dev->next)
    if (!grub_strcmp (dev->devname, name))
      break;
  if (!dev)
    return grub_error (GRUB_ERR_BAD_DEVICE, "Device not found");
  
  /* Remove the device from the list.  */
  *prev = dev->next;

  grub_free (dev->devname);
  grub_free (dev->filename);
  grub_free (dev);
  
  return 0;
}

/* The command to add and remove loopback devices.  */
static grub_err_t
grub_cmd_loopback (struct grub_arg_list *state,
		   int argc, char **args)
{
  grub_file_t file;
  struct grub_loopback *newdev;
  
  if (argc < 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "device name required");
  
  /* Check if `-d' was used.  */
  if (state[0].set)
      return delete_loopback (args[0]);
  
  if (argc < 2)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "file name required");

  file = grub_file_open (args[1]);
  if (! file)
    return grub_errno;
  
  /* Close the file, the only reason for opening it is validation.  */
  grub_file_close (file);
  
  /* First try to replace the old device.  */
  for (newdev = loopback_list; newdev; newdev = newdev->next)
    if (!grub_strcmp (newdev->devname, args[0]))
      break;
  if (newdev)
    {
      char *newname = grub_strdup (args[1]);
      if (!newname)
	return grub_errno;
      
      grub_free (newdev->filename);
      newdev->filename = newname;
      
      /* Set has_partitions when `--partitions' was used.  */
      newdev->has_partitions = state[1].set;
      
      return 0;
    }
  
  /* Unable to replace it, make a new entry.  */
  newdev = grub_malloc (sizeof (struct grub_loopback));
  if (!newdev)
    return grub_errno;
  
  newdev->devname = grub_strdup (args[0]);
  if (!newdev->devname)
    {
      grub_free (newdev);
      return grub_errno;
    }
  
  newdev->filename = grub_strdup (args[1]);
  if (!newdev->devname)
    {
      grub_free (newdev->devname);
      grub_free (newdev);
      return grub_errno;
    }
  
  /* Set has_partitions when `--partitions' was used.  */
  newdev->has_partitions = state[1].set;
  
  /* Add the new entry to the list.  */
  newdev->next = loopback_list;
  loopback_list = newdev;
  
  return 0;
}


static int
grub_loopback_iterate (int (*hook) (const char *name))
{
  struct grub_loopback *d;
  for (d = loopback_list; d; d = d->next)
    {
      if (hook (d->devname))
	return 1;
    }
  return 0;
}

static grub_err_t
grub_loopback_open (const char *name, grub_disk_t disk)
{
  grub_file_t file;
  struct grub_loopback *dev;
    
  for (dev = loopback_list; dev; dev = dev->next)
    if (!grub_strcmp (dev->devname, name))
      break;
  
  if (! dev)
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "Can't open device");

  file = grub_file_open (dev->filename);
  if (! file)
    return grub_errno;
  
  /* Use the filesize for the disk size, round up to a complete sector.  */
  disk->total_sectors = ((file->size + GRUB_DISK_SECTOR_SIZE - 1)
			 / GRUB_DISK_SECTOR_SIZE);
  disk->id = (int) dev;
  
  disk->has_partitions = dev->has_partitions;
  disk->data = file;
  
  return 0;
}

static void
grub_loopback_close (grub_disk_t disk)
{
  grub_file_t file = (grub_file_t) disk->data;
  
  grub_file_close (file);
}

static grub_err_t
grub_loopback_read (grub_disk_t disk, unsigned long sector,
		    unsigned long size, char *buf)
{
  grub_file_t file = (grub_file_t) disk->data;
  long pos;
  
  grub_file_seek (file, sector * GRUB_DISK_SECTOR_SIZE);
  
  grub_file_read (file, buf, size * GRUB_DISK_SECTOR_SIZE);
  if (grub_errno)
    return grub_errno;
  
  /* In case there is more data read than there is available, in case
     of files that are not a multiple of GRUB_DISK_SECTOR_SIZE, fill
     the rest with zeros.  */
  pos = sector * GRUB_DISK_SECTOR_SIZE + size * GRUB_DISK_SECTOR_SIZE;
  if (pos > file->size)
    {
      unsigned long amount = pos - file->size;
      grub_memset (buf + pos - amount, amount, 0);
    }
  
  return 0;
}

static grub_err_t
grub_loopback_write (grub_disk_t disk __attribute ((unused)),
		   unsigned long sector __attribute ((unused)),
		   unsigned long size __attribute ((unused)),
		   const char *buf __attribute ((unused)))
{
  return GRUB_ERR_NOT_IMPLEMENTED_YET;
}

static struct grub_disk_dev grub_loopback_dev =
  {
    .name = "loopback",
    .id = GRUB_DISK_DEVICE_LOOPBACK_ID,
    .iterate = grub_loopback_iterate,
    .open = grub_loopback_open,
    .close = grub_loopback_close,
    .read = grub_loopback_read,
    .write = grub_loopback_write,
    .next = 0
  };


#ifdef GRUB_UTIL
void
grub_loop_init (void)
{
  grub_register_command ("loopback", grub_cmd_loopback, GRUB_COMMAND_FLAG_BOTH,
			 "loopback [-d|-p] DEVICENAME FILE",
			 "Make a device of a file.", options);
  grub_disk_dev_register (&grub_loopback_dev);
}

void
grub_loop_fini (void)
{
  grub_unregister_command ("loopback");
  grub_disk_dev_unregister (&grub_loopback_dev);
}
#else /* ! GRUB_UTIL */
GRUB_MOD_INIT
{
  (void)mod;			/* To stop warning. */
  grub_register_command ("loopback", grub_cmd_loopback, GRUB_COMMAND_FLAG_BOTH,
			 "loopback [-d|-p] DEVICENAME FILE",
			 "Make a device of a file.", options);
  grub_disk_dev_register (&grub_loopback_dev);
}

GRUB_MOD_FINI
{
  grub_unregister_command ("loopback");
  grub_disk_dev_unregister (&grub_loopback_dev);
}
#endif /* ! GRUB_UTIL */
