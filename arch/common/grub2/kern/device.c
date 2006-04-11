/* device.c - device manager */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2005  Free Software Foundation, Inc.
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

#include <grub/device.h>
#include <grub/disk.h>
#include <grub/net.h>
#include <grub/fs.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/env.h>
#include <grub/partition.h>

grub_device_t
grub_device_open (const char *name)
{
  grub_disk_t disk = 0;
  grub_device_t dev = 0;

  if (! name)
    {
      name = grub_env_get ("root");
      if (*name == '\0')
	{
	  grub_error (GRUB_ERR_BAD_DEVICE, "no device is set");
	  goto fail;
	}
    }
    
  dev = grub_malloc (sizeof (*dev));
  if (! dev)
    goto fail;
  
  /* Try to open a disk.  */
  disk = grub_disk_open (name);
  if (! disk)
    {
      grub_error (GRUB_ERR_BAD_DEVICE, "unknown device");
      goto fail;
    }

  dev->disk = disk;
  dev->net = 0;	/* FIXME */

  return dev;

 fail:
  if (disk)
    grub_disk_close (disk);
  
  grub_free (dev);

  return 0;
}

grub_err_t
grub_device_close (grub_device_t device)
{
  if (device->disk)
    grub_disk_close (device->disk);

  grub_free (device);

  return grub_errno;
}

int
grub_device_iterate (int (*hook) (const char *name))
{
  auto int iterate_disk (const char *disk_name);
  auto int iterate_partition (grub_disk_t disk,
			      const grub_partition_t partition);
  
  int iterate_disk (const char *disk_name)
    {
      grub_device_t dev;

      if (hook (disk_name))
	return 1;
      
      dev = grub_device_open (disk_name);
      if (! dev)
	return 1;
      
      if (dev->disk && dev->disk->has_partitions)
	if (grub_partition_iterate (dev->disk, iterate_partition))
	  {
	    grub_device_close (dev);
	    return 1;
	  }

      grub_device_close (dev);
      return 0;
    }
  
  int iterate_partition (grub_disk_t disk, const grub_partition_t partition)
    {
      char *partition_name;
      char *device_name;
      int ret;
      
      partition_name = grub_partition_get_name (partition);
      if (! partition_name)
	return 1;
      
      device_name = grub_malloc (grub_strlen (disk->name) + 1
				 + grub_strlen (partition_name) + 1);
      if (! device_name)
	{
	  grub_free (partition_name);
	  return 1;
	}

      grub_sprintf (device_name, "%s,%s", disk->name, partition_name);
      grub_free (partition_name);

      ret = hook (device_name);
      grub_free (device_name);
      return ret;
    }

  /* Only disk devices are supported at the moment.  */
  return grub_disk_dev_iterate (iterate_disk);
}
