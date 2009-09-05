/* lvm.c - LVM support for GRUB utils.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2006,2007,2008  Free Software Foundation, Inc.
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

/* We only support LVM on Linux.  */
#ifdef __linux__

#include <grub/util/misc.h>
#include <grub/util/lvm.h>

#include <string.h>
#include <sys/stat.h>

int
grub_util_lvm_isvolume (char *name)
{
  char *devname;
  struct stat st;
  int err;

  devname = xmalloc (strlen (name) + 13);

  strcpy (devname, "/dev/mapper/");
  strcpy (devname+12, name);

  err = stat (devname, &st);
  free (devname);

  if (err)
    return 0;
  else
    return 1;
}

#endif /* ! __linux__ */
