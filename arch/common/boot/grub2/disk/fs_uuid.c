/* fs_uuid.c - Access disks by their filesystem UUID.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2007,2008  Free Software Foundation, Inc.
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

#include <grub/disk.h>
#include <grub/dl.h>
#include <grub/kernel.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/types.h>

#include <grub/fs.h>
#include <grub/partition.h>

static grub_device_t
search_fs_uuid (const char *key, unsigned long *count)
{
  *count = 0;
  grub_device_t ret = NULL;

  auto int iterate_device (const char *name);
  int iterate_device (const char *name)
    {
      grub_device_t dev;

      dev = grub_device_open (name);
      if (dev)
	{
	  grub_fs_t fs;

	  fs = grub_fs_probe (dev);
	  if (fs && fs->uuid)
	    {
	      char *uuid;

	      (fs->uuid) (dev, &uuid);
	      if (grub_errno == GRUB_ERR_NONE && uuid)
		{
		  (*count)++;

		  if (grub_strcasecmp (uuid, key) == 0)
		    {
		      ret = dev;
		      grub_free (uuid);
		      return 1;
		    }
		  grub_free (uuid);
		}
	    }

	  grub_device_close (dev);
	}

      grub_errno = GRUB_ERR_NONE;
      return 0;
    }

  grub_device_iterate (iterate_device);

  return ret;
}

static grub_err_t
grub_fs_uuid_open (const char *name, grub_disk_t disk)
{
  grub_device_t dev;

  if (grub_strncmp (name, "UUID=", sizeof ("UUID=")-1))
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "not a UUID virtual volume");

  dev = search_fs_uuid (name + sizeof ("UUID=") - 1, &disk->id);
  if (! dev)
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "no matching UUID found");

  disk->total_sectors = dev->disk->total_sectors;
  disk->has_partitions = 0;
  if (dev->disk->partition)
    {
      disk->partition = grub_malloc (sizeof (*disk->partition));
      if (disk->partition)
	grub_memcpy (disk->partition, dev->disk->partition,
		     sizeof (*disk->partition));
    }
  else
    disk->partition = NULL;

  disk->data = dev;

  return GRUB_ERR_NONE;
}

static void
grub_fs_uuid_close (grub_disk_t disk __attribute((unused)))
{
  grub_device_t parent = disk->data;
  grub_device_close (parent);
}

static grub_err_t
grub_fs_uuid_read (grub_disk_t disk, grub_disk_addr_t sector,
		   grub_size_t size, char *buf)
{
  grub_device_t parent = disk->data;
  return parent->disk->dev->read (parent->disk, sector, size, buf);
}

static grub_err_t
grub_fs_uuid_write (grub_disk_t disk, grub_disk_addr_t sector,
		    grub_size_t size, const char *buf)
{
  grub_device_t parent = disk->data;
  return parent->disk->dev->write (parent->disk, sector, size, buf);
}

static struct grub_disk_dev grub_fs_uuid_dev =
  {
    .name = "fs_uuid",
    .id = GRUB_DISK_DEVICE_UUID_ID,
    .open = grub_fs_uuid_open,
    .close = grub_fs_uuid_close,
    .read = grub_fs_uuid_read,
    .write = grub_fs_uuid_write,
    .next = 0
  };

GRUB_MOD_INIT(fs_uuid)
{
  grub_disk_dev_register (&grub_fs_uuid_dev);
}

GRUB_MOD_FINI(fs_uuid)
{
  grub_disk_dev_unregister (&grub_fs_uuid_dev);
}
