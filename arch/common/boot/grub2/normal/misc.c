/* misc.c - miscellaneous functions */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2007  Free Software Foundation, Inc.
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
      char *label;
      grub_fs_t fs;

      fs = grub_fs_probe (dev);
      /* Ignore all errors.  */
      grub_errno = 0;

      if (fs)
	grub_printf ("Filesystem type %s", fs->name);
      else if (! dev->disk->has_partitions || dev->disk->partition)
	grub_printf ("Unknown filesystem");
      else
	grub_printf ("Partition table");
	  
      if (fs && fs->label)
	{
	  (fs->label) (dev, &label);
	  if (grub_errno == GRUB_ERR_NONE)
	    {
	      if (label && grub_strlen (label))
		grub_printf (", Label %s", label);
	      grub_free (label);
	    }
	  grub_errno = GRUB_ERR_NONE;
	}
      grub_device_close (dev);
    }

  grub_printf ("\n");
  return grub_errno;
}
