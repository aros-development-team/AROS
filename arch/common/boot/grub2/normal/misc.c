/* misc.c - miscellaneous functions */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2007,2008  Free Software Foundation, Inc.
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

#include <grub/normal.h>
#include <grub/disk.h>
#include <grub/fs.h>
#include <grub/err.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/datetime.h>

/* Print the information on the device NAME.  */
grub_err_t
grub_normal_print_device_info (const char *name)
{
  grub_device_t dev;
  char *p;

  p = grub_strchr (name, ',');
  if (p)
    grub_printf ("\tPartition %s: ", name);
  else
    grub_printf ("Device %s: ", name);

  dev = grub_device_open (name);
  if (! dev)
    grub_printf ("Filesystem cannot be accessed");
  else if (dev->disk)
    {
      grub_fs_t fs;

      fs = grub_fs_probe (dev);
      /* Ignore all errors.  */
      grub_errno = 0;

      if (fs)
	{
	  grub_printf ("Filesystem type %s", fs->name);
	  if (fs->label)
	    {
	      char *label;
	      (fs->label) (dev, &label);
	      if (grub_errno == GRUB_ERR_NONE)
		{
		  if (label && grub_strlen (label))
		    grub_printf (", Label %s", label);
		  grub_free (label);
		}
	      grub_errno = GRUB_ERR_NONE;
	    }
	  if (fs->mtime)
	    {
	      grub_int32_t tm;
	      struct grub_datetime datetime;
	      (fs->mtime) (dev, &tm);
	      if (grub_errno == GRUB_ERR_NONE)
		{
		  grub_unixtime2datetime (tm, &datetime);
		  grub_printf (", Last modification time %d-%02d-%02d "
			       "%02d:%02d:%02d %s",
			       datetime.year, datetime.month, datetime.day,
			       datetime.hour, datetime.minute, datetime.second,
			       grub_get_weekday_name (&datetime));

		}
	      grub_errno = GRUB_ERR_NONE;
	    }
	  if (fs->uuid)
	    {
	      char *uuid;
	      (fs->uuid) (dev, &uuid);
	      if (grub_errno == GRUB_ERR_NONE)
		{
		  if (uuid && grub_strlen (uuid))
		    grub_printf (", UUID %s", uuid);
		  grub_free (uuid);
		}
	      grub_errno = GRUB_ERR_NONE;
	    }
	}
      else if (! dev->disk->has_partitions || dev->disk->partition)
	grub_printf ("Unknown filesystem");
      else
	grub_printf ("Partition table");

      grub_device_close (dev);
    }

  grub_printf ("\n");
  return grub_errno;
}
