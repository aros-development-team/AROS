/* ofpath.c - calculate OpenFirmware path names given an OS device */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
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

#undef OFPATH_STANDALONE

#ifndef OFPATH_STANDALONE
#include <grub/types.h>
#include <grub/util/misc.h>
#include <grub/util/ofpath.h>
#endif

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <malloc.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

#ifdef OFPATH_STANDALONE
#define UNUSED __attribute__((unused))
#define xmalloc malloc
void
grub_util_error (const char *fmt, ...)
{
  va_list ap;

  fprintf (stderr, "ofpath: error: ");
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fputc ('\n', stderr);
  exit (1);
}

#endif

static void
kill_trailing_dir(char *path)
{
  char *end = path + strlen(path) - 1;

  while (end >= path)
    {
      if (*end != '/')
	{
	  end--;
	  continue;
	}
      *end = '\0';
      break;
    }
}

static void
trim_newline (char *path)
{
  char *end = path + strlen(path) - 1;

  while (*end == '\n')
    *end-- = '\0';
}

#define OF_PATH_MAX	256

static void
find_obppath(char *of_path, const char *sysfs_path_orig)
{
  char *sysfs_path, *path;

  sysfs_path = xmalloc (PATH_MAX);
  path = xmalloc (PATH_MAX);

  strcpy(sysfs_path, sysfs_path_orig);
  while (1)
    {
      int fd;

      snprintf(path, PATH_MAX, "%s/obppath", sysfs_path);
#if 0
      printf("Trying %s\n", path);
#endif

      fd = open(path, O_RDONLY);
      if (fd < 0)
	{
	  kill_trailing_dir(sysfs_path);
	  if (!strcmp(sysfs_path, "/sys"))
	    grub_util_error("'obppath' not found in parent dirs of %s",
			    sysfs_path_orig);
	  continue;
	}
      memset(of_path, 0, OF_PATH_MAX);
      read(fd, of_path, OF_PATH_MAX);
      close(fd);

      trim_newline(of_path);
      break;
    }

  free (path);
  free (sysfs_path);
}

static void
block_device_get_sysfs_path_and_link(const char *devicenode,
				     char *sysfs_path, int sysfs_path_len)
{
  char *rpath = xmalloc (PATH_MAX);

  snprintf(sysfs_path, sysfs_path_len, "/sys/block/%s", devicenode);

  if (!realpath (sysfs_path, rpath))
    grub_util_error ("Cannot get the real path of `%s'", sysfs_path);

  strcat(rpath, "/device");

  if (!realpath (rpath, sysfs_path))
    grub_util_error ("Cannot get the real path of `%s'", rpath);

  free (rpath);
}

static const char *
trailing_digits (const char *p)
{
  const char *end;

  end = p + strlen(p) - 1;
  while (end >= p)
    {
      if (! isdigit(*end))
	break;
      end--;
    }

  return end + 1;
}

static void
__of_path_common(char *of_path, char *sysfs_path,
		 const char *device, int devno)
{
  const char *digit_string;
  char disk[64];

  find_obppath(of_path, sysfs_path);

  digit_string = trailing_digits (device);
  if (*digit_string == '\0')
    {
      sprintf(disk, "/disk@%d", devno);
    }
  else
    {
      int part;

      sscanf(digit_string, "%d", &part);
      sprintf(disk, "/disk@%d:%c", devno, 'a' + (part - 1));
    }
  strcat(of_path, disk);
}

static char *
get_basename(char *p)
{
  char *ret = p;

  while (*p)
    {
      if (*p == '/')
	ret = p + 1;
      p++;
    }

  return ret;
}

static void
of_path_of_vdisk(char *of_path,
		 const char *devname UNUSED, const char *device,
		 const char *devnode UNUSED, const char *devicenode)
{
  char *sysfs_path, *p;
  int devno, junk;

  sysfs_path = xmalloc (PATH_MAX);
  block_device_get_sysfs_path_and_link(devicenode,
				       sysfs_path, PATH_MAX);
  p = get_basename (sysfs_path);
  sscanf(p, "vdc-port-%d-%d", &devno, &junk);
  __of_path_common(of_path, sysfs_path, device, devno);

  free (sysfs_path);
}

static void
of_path_of_ide(char *of_path,
	       const char *devname UNUSED, const char *device,
	       const char *devnode UNUSED, const char *devicenode)
{
  char *sysfs_path, *p;
  int chan, devno;

  sysfs_path = xmalloc (PATH_MAX);
  block_device_get_sysfs_path_and_link(devicenode,
				       sysfs_path, PATH_MAX);
  p = get_basename (sysfs_path);
  sscanf(p, "%d.%d", &chan, &devno);

  __of_path_common(of_path, sysfs_path, device, devno);

  free (sysfs_path);
}

static int
vendor_is_ATA(const char *path)
{
  int fd, err;
  char *buf;

  buf = xmalloc (PATH_MAX);

  snprintf(buf, PATH_MAX, "%s/vendor", path);
  fd = open(buf, O_RDONLY);
  if (fd < 0)
    grub_util_error ("Cannot open 'vendor' node of `%s'", path);

  memset(buf, 0, PATH_MAX);
  err = read(fd, buf, PATH_MAX);
  if (err < 0)
    grub_util_error ("Cannot read 'vendor' node of `%s'", path);

  close(fd);

  free (buf);

  if (!strncmp(buf, "ATA", 3))
    return 1;
  return 0;
}

static void
check_sas (char *sysfs_path, int *tgt)
{
  char *ed = strstr (sysfs_path, "end_device");
  char *p, *q, *path;
  char phy[16];
  int fd;

  if (!ed)
    return;

  /* SAS devices are identified using disk@$PHY_ID */
  p = strdup (sysfs_path);
  ed = strstr(p, "end_device");

  q = ed;
  while (*q && *q != '/')
    q++;
  *q = '\0';

  path = xmalloc (PATH_MAX);
  sprintf (path, "%s/sas_device:%s/phy_identifier", p, ed);

  fd = open(path, O_RDONLY);
  if (fd < 0)
    grub_util_error("Cannot open SAS PHY ID '%s'\n", path);

  memset (phy, 0, sizeof (phy));
  read (fd, phy, sizeof (phy));

  sscanf (phy, "%d", tgt);

  free (path);
  free (p);
}

static void
of_path_of_scsi(char *of_path,
		const char *devname UNUSED, const char *device,
		const char *devnode UNUSED, const char *devicenode)
{
  const char *p, *digit_string, *disk_name;
  int host, bus, tgt, lun;
  char *sysfs_path, disk[64];

  sysfs_path = xmalloc (PATH_MAX);

  block_device_get_sysfs_path_and_link(devicenode,
				       sysfs_path, PATH_MAX);
  p = get_basename (sysfs_path);
  sscanf(p, "%d:%d:%d:%d", &host, &bus, &tgt, &lun);
  check_sas (sysfs_path, &tgt);

  if (vendor_is_ATA(sysfs_path))
    {
      __of_path_common(of_path, sysfs_path, device, tgt);
      free (sysfs_path);
      return;
    }

  find_obppath(of_path, sysfs_path);
  free (sysfs_path);

  if (strstr (of_path, "qlc"))
    strcat (of_path, "/fp@0,0");

  if (strstr (of_path, "sbus"))
    disk_name = "sd";
  else
    disk_name = "disk";

  digit_string = trailing_digits (device);
  if (*digit_string == '\0')
    {
      sprintf(disk, "/%s@%x,%d", disk_name, tgt, lun);
    }
  else
    {
      int part;

      sscanf(digit_string, "%d", &part);
      sprintf(disk, "/%s@%x,%d:%c", disk_name, tgt, lun, 'a' + (part - 1));
    }
  strcat(of_path, disk);
}

static char *
strip_trailing_digits (const char *p)
{
  char *new, *end;

  new = strdup (p);
  end = new + strlen(new) - 1;
  while (end >= new)
    {
      if (! isdigit(*end))
	break;
      *end-- = '\0';
    }

  return new;
}

char *
grub_util_devname_to_ofpath (char *devname)
{
  char *name_buf, *device, *devnode, *devicenode, *ofpath;

  name_buf = xmalloc (PATH_MAX);
  name_buf = realpath (devname, name_buf);
  if (! name_buf)
    grub_util_error ("Cannot get the real path of `%s'", devname);

  device = get_basename (devname);
  devnode = strip_trailing_digits (devname);
  devicenode = strip_trailing_digits (device);

  ofpath = xmalloc (OF_PATH_MAX);

  if (device[0] == 'h' && device[1] == 'd')
    of_path_of_ide(ofpath, name_buf, device, devnode, devicenode);
  else if (device[0] == 's'
	   && (device[1] == 'd' || device[1] == 'r'))
    of_path_of_scsi(ofpath, name_buf, device, devnode, devicenode);
  else if (device[0] == 'v' && device[1] == 'd' && device[2] == 'i'
	   && device[3] == 's' && device[4] == 'k')
    of_path_of_vdisk(ofpath, name_buf, device, devnode, devicenode);

  free (devnode);
  free (devicenode);
  free (name_buf);

  return ofpath;
}

#ifdef OFPATH_STANDALONE
int main(int argc, char **argv)
{
  char *of_path;

  if (argc != 2)
    {
      printf("Usage: grub-ofpathname DEVICE\n");
      return 1;
    }

  of_path = grub_util_devname_to_ofpath (argv[1]);
  printf("%s\n", of_path);

  return 0;
}
#endif
