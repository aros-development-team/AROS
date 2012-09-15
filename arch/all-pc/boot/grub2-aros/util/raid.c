/* raid.c - RAID support for GRUB utils.  */
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

/* We only support RAID on Linux.  */
#ifdef __linux__
#include <grub/emu/misc.h>
#include <grub/util/misc.h>
#include <grub/emu/getroot.h>

#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/types.h>

#include <linux/types.h>
#include <linux/major.h>
#include <linux/raid/md_p.h>
#include <linux/raid/md_u.h>
#include <grub/i18n.h>

char **
grub_util_raid_getmembers (const char *name, int bootable)
{
  int fd, ret, i, j;
  char **devicelist;
  mdu_version_t version;
  mdu_array_info_t info;
  mdu_disk_info_t disk;

  fd = open (name, O_RDONLY);

  if (fd == -1)
    grub_util_error (_("cannot open `%s': %s"), name, strerror (errno));

  ret = ioctl (fd, RAID_VERSION, &version);
  if (ret != 0)
    grub_util_error (_("ioctl RAID_VERSION error: %s"), strerror (errno));

  if ((version.major != 0 || version.minor != 90)
      && (version.major != 1 || version.minor != 0)
      && (version.major != 1 || version.minor != 1)
      && (version.major != 1 || version.minor != 2))
    grub_util_error (_("unsupported RAID version: %d.%d"),
		     version.major, version.minor);

  if (bootable && (version.major != 0 || version.minor != 90))
    grub_util_error (_("unsupported RAID version: %d.%d"),
		     version.major, version.minor);

  ret = ioctl (fd, GET_ARRAY_INFO, &info);
  if (ret != 0)
    grub_util_error (_("ioctl GET_ARRAY_INFO error: %s"), strerror (errno));

  devicelist = xmalloc ((info.nr_disks + 1) * sizeof (char *));

  for (i = 0, j = 0; j < info.nr_disks; i++)
    {
      disk.number = i;
      ret = ioctl (fd, GET_DISK_INFO, &disk);
      if (ret != 0)
	grub_util_error (_("ioctl GET_DISK_INFO error: %s"), strerror (errno));
      
      if (disk.state & (1 << MD_DISK_REMOVED))
	continue;

      if (disk.state & (1 << MD_DISK_ACTIVE))
	devicelist[j] = grub_find_device (NULL,
					  makedev (disk.major, disk.minor));
      else
	devicelist[j] = NULL;
      j++;
    }

  devicelist[j] = NULL;

  close (fd);

  return devicelist;
}

#endif /* ! __linux__ */
