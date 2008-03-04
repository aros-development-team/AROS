/* ofdisk.c - Open Firmware disk access.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2004,2006,2007  Free Software Foundation, Inc.
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

#include <grub/misc.h>
#include <grub/disk.h>
#include <grub/mm.h>
#include <grub/ieee1275/ieee1275.h>
#include <grub/ieee1275/ofdisk.h>

static int
grub_ofdisk_iterate (int (*hook) (const char *name))
{
  auto int dev_iterate (struct grub_ieee1275_devalias *alias);

  int dev_iterate (struct grub_ieee1275_devalias *alias)
    {
      grub_dprintf ("disk", "disk name = %s\n", alias->name);

      if (grub_ieee1275_test_flag (GRUB_IEEE1275_FLAG_OFDISK_SDCARD_ONLY))
	{
	  grub_ieee1275_phandle_t dev;
	  char tmp[8];

	  if (grub_ieee1275_finddevice (alias->path, &dev))
	    {
	      grub_dprintf ("disk", "finddevice (%s) failed\n", alias->path);
	      return 0;
	    }

	  if (grub_ieee1275_get_property (dev, "iconname", tmp,
					  sizeof tmp, 0))
	    {
	      grub_dprintf ("disk", "get iconname failed\n");
	      return 0;
	    }

	  if (grub_strcmp (tmp, "sdmmc"))
	    {
	      grub_dprintf ("disk", "device is not an SD card\n");
	      return 0;
	    }
	}

      if (! grub_strcmp (alias->type, "block"))
	hook (alias->name);
      else if ((! grub_strcmp (alias->type, "scsi"))
	       || (! grub_strcmp (alias->type, "ide"))
	       || (! grub_strcmp (alias->type, "ata")))
	/* Search for block-type children of these bus controllers.  */
	grub_children_iterate (alias->name, dev_iterate);
      return 0;
    }

  grub_devalias_iterate (dev_iterate);
  return 0;
}

static grub_err_t
grub_ofdisk_open (const char *name, grub_disk_t disk)
{
  grub_ieee1275_phandle_t dev;
  grub_ieee1275_ihandle_t dev_ihandle = 0;
  char *devpath;
  /* XXX: This should be large enough for any possible case.  */
  char prop[64];
  grub_ssize_t actual;

  devpath = grub_strndup (name, grub_strlen (name) + 2);
  if (! devpath)
    return grub_errno;

  /* To access the complete disk add `:0'.  */
  if (! grub_ieee1275_test_flag (GRUB_IEEE1275_FLAG_NO_PARTITION_0))
    grub_strcat (devpath, ":0");

  grub_dprintf ("disk", "Opening `%s'.\n", devpath);

  grub_ieee1275_open (devpath, &dev_ihandle);
  if (! dev_ihandle)
    {
      grub_error (GRUB_ERR_UNKNOWN_DEVICE, "Can't open device");
      goto fail;
    }

  grub_dprintf ("disk", "Opened `%s' as handle %p.\n", devpath, (void *) dev_ihandle);

  if (grub_ieee1275_finddevice (devpath, &dev))
    {
      grub_error (GRUB_ERR_UNKNOWN_DEVICE, "Can't read device properties");
      goto fail;
    }

  if (grub_ieee1275_get_property (dev, "device_type", prop, sizeof (prop),
				  &actual))
    {
      grub_error (GRUB_ERR_BAD_DEVICE, "Can't read the device type");
      goto fail;
    }

  if (grub_strcmp (prop, "block"))
    {
      grub_error (GRUB_ERR_BAD_DEVICE, "Not a block device");
      goto fail;
    }

  /* XXX: There is no property to read the number of blocks.  There
     should be a property `#blocks', but it is not there.  Perhaps it
     is possible to use seek for this.  */
  disk->total_sectors = 0xFFFFFFFFUL;

  /* XXX: Is it ok to use this?  Perhaps it is better to use the path
     or some property.  */
  disk->id = dev;

  /* XXX: Read this, somehow.  */
  disk->has_partitions = 1;
  disk->data = (void *) dev_ihandle;
  grub_free (devpath);
  return 0;

 fail:
  if (dev_ihandle)
    grub_ieee1275_close (dev_ihandle);
  grub_free (devpath);
  return grub_errno;
}

static void
grub_ofdisk_close (grub_disk_t disk)
{
  grub_dprintf ("disk", "Closing handle %p.\n",
		(void *) disk->data);
  grub_ieee1275_close ((grub_ieee1275_ihandle_t) disk->data);
}

static grub_err_t
grub_ofdisk_read (grub_disk_t disk, grub_disk_addr_t sector,
		  grub_size_t size, char *buf)
{
  grub_ssize_t status, actual;
  unsigned long long pos;

  grub_dprintf ("disk",
		"Reading handle %p: sector 0x%llx, size 0x%lx, buf %p.\n",
		(void *) disk->data, sector, (long) size, buf);

  pos = sector * 512UL;

  grub_ieee1275_seek ((grub_ieee1275_ihandle_t) disk->data, (int) (pos >> 32),
		      (int) pos & 0xFFFFFFFFUL, &status);
  if (status < 0)
    return grub_error (GRUB_ERR_READ_ERROR,
		       "Seek error, can't seek block %llu",
		       sector);
  grub_ieee1275_read ((grub_ieee1275_ihandle_t) disk->data, buf,
		      size * 512UL, &actual);
  if (actual != actual)
    return grub_error (GRUB_ERR_READ_ERROR, "Read error on block: %llu",
		       sector);
    
  return 0;
}

static grub_err_t
grub_ofdisk_write (grub_disk_t disk __attribute ((unused)),
		   grub_disk_addr_t sector __attribute ((unused)),
		   grub_size_t size __attribute ((unused)),
		   const char *buf __attribute ((unused)))
{
  return GRUB_ERR_NOT_IMPLEMENTED_YET;
}

static struct grub_disk_dev grub_ofdisk_dev =
  {
    .name = "ofdisk",
    .id = GRUB_DISK_DEVICE_OFDISK_ID,
    .iterate = grub_ofdisk_iterate,
    .open = grub_ofdisk_open,
    .close = grub_ofdisk_close,
    .read = grub_ofdisk_read,
    .write = grub_ofdisk_write,
    .next = 0
  };

void
grub_ofdisk_init (void)
{
  grub_disk_dev_register (&grub_ofdisk_dev);
}

void
grub_ofdisk_fini (void)
{
  grub_disk_dev_unregister (&grub_ofdisk_dev);
}
