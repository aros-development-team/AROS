/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2013 Free Software Foundation, Inc.
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

#include <config.h>

#include <grub/util/install.h>
#include <grub/emu/exec.h>
#include <grub/emu/misc.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

#include <sys/utsname.h>

static int
is_not_empty_directory (const char *dir)
{
  DIR *d;
  struct dirent *de;

  d = opendir (dir);
  if (!d)
    return 0;
  while ((de = readdir (d)))
    {
      if (strcmp (de->d_name, ".") == 0
	  || strcmp (de->d_name, "..") == 0)
	continue;
      closedir (d);
      return 1;
    }

  closedir (d);
  return 0;
}

static int
is_64_kernel (void)
{
  struct utsname un;

  if (uname (&un) < 0)
    return 0;

  return strcmp (un.machine, "x86_64") == 0;
}

const char *
grub_install_get_default_x86_platform (void)
{ 
  /*
     On Linux, we need the efivars kernel modules.
     If no EFI is available this module just does nothing
     besides a small hello and if we detect efi we'll load it
     anyway later. So it should be safe to
     try to load it here.
   */
  grub_util_exec_redirect_all ((const char * []){ "modprobe", "efivars", NULL },
			       NULL, NULL, "/dev/null");

  grub_util_info ("Looking for /sys/firmware/efi ..");
  if (is_not_empty_directory ("/sys/firmware/efi"))
    {
      grub_util_info ("...found");
      if (is_64_kernel ())
	return "x86_64-efi";
      else
	return "i386-efi";
    }

  grub_util_info ("... not found. Looking for /proc/device-tree ..");
  if (is_not_empty_directory ("/proc/device-tree"))
    {
      grub_util_info ("...found");
      return "i386-ieee1275";
    }

  grub_util_info ("... not found");
  return "i386-pc";
}
