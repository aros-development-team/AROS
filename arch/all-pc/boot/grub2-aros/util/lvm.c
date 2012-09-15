/* lvm.c - LVM support for GRUB utils.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2006,2007,2008,2011  Free Software Foundation, Inc.
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

#if defined(__linux__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#include <grub/emu/misc.h>
#include <grub/util/misc.h>
#include <grub/util/lvm.h>

#include <string.h>
#include <sys/stat.h>

int
grub_util_lvm_isvolume (char *name)
{
  char *lvmdevname;
  struct stat st;
  int err;

  lvmdevname = xmalloc (strlen (name) + sizeof (LVM_DEV_MAPPER_STRING));

  strcpy (lvmdevname, LVM_DEV_MAPPER_STRING);
  strcpy (lvmdevname + sizeof(LVM_DEV_MAPPER_STRING) - 1, name);

  err = stat (lvmdevname, &st);
  free (lvmdevname);

  if (err)
    return 0;
  else
    return 1;
}

#endif
