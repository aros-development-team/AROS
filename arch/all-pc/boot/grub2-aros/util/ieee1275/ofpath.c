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
#include <grub/i18n.h>
#endif

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

#ifdef OFPATH_STANDALONE
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

void
grub_util_info (const char *fmt, ...)
{
  va_list ap;

  fprintf (stderr, "ofpath: info: ");
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fputc ('\n', stderr);
}

#define grub_util_warn grub_util_info
#define _(x) x
#define xstrdup strdup
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

#define MAX_DISK_CAT    64

static char *
find_obppath (const char *sysfs_path_orig)
{
  char *sysfs_path, *path;
  size_t path_size = strlen (sysfs_path_orig) + sizeof ("/obppath");

  sysfs_path = xstrdup (sysfs_path_orig);
  path = xmalloc (path_size);

  while (1)
    {
      int fd;
      char *of_path;
      struct stat st;
      size_t size;

      snprintf(path, path_size, "%s/obppath", sysfs_path);
#if 0
      printf("Trying %s\n", path);
#endif

      fd = open(path, O_RDONLY);
      if (fd < 0 || fstat (fd, &st) < 0)
	{
	  snprintf(path, path_size, "%s/devspec", sysfs_path);
	  fd = open(path, O_RDONLY);
	}

      if (fd < 0 || fstat (fd, &st) < 0)
	{
	  kill_trailing_dir(sysfs_path);
	  if (!strcmp(sysfs_path, "/sys"))
	    {
	      grub_util_info (_("'obppath' not found in parent dirs of %s,"
				" no IEEE1275 name discovery"),
			      sysfs_path_orig);
	      free (path);
	      free (sysfs_path);
	      return NULL;
	    }
	  continue;
	}
      size = st.st_size;
      of_path = xmalloc (size + MAX_DISK_CAT + 1);
      memset(of_path, 0, size + MAX_DISK_CAT + 1);
      read(fd, of_path, size);
      close(fd);

      trim_newline(of_path);
      free (path);
      free (sysfs_path);
      return of_path;
    }
}

static char *
xrealpath (const char *in)
{
  char *out;
#ifdef PATH_MAX
  out = xmalloc (PATH_MAX);
  out = realpath (in, out);
#else
  out = realpath (in, NULL);
#endif
  if (!out)
    grub_util_error (_("failed to get canonical path of %s"), in);
  return out;
}

static char *
block_device_get_sysfs_path_and_link(const char *devicenode)
{
  char *rpath;
  char *rpath2;
  char *ret;
  size_t tmp_size = strlen (devicenode) + sizeof ("/sys/block/");
  char *tmp = xmalloc (tmp_size);

  memcpy (tmp, "/sys/block/", sizeof ("/sys/block/"));
  strcat (tmp, devicenode);

  rpath = xrealpath (tmp);
  rpath2 = xmalloc (strlen (rpath) + sizeof ("/device"));
  strcpy (rpath2, rpath);
  strcat (rpath2, "/device");

  ret = xrealpath (rpath2);

  free (tmp);
  free (rpath);
  free (rpath2);
  return ret;
}

static inline int
my_isdigit (int c)
{
  return (c >= '0' && c <= '9');
}

static const char *
trailing_digits (const char *p)
{
  const char *end;

  end = p + strlen(p) - 1;
  while (end >= p)
    {
      if (! my_isdigit(*end))
	break;
      end--;
    }

  return end + 1;
}

static char *
__of_path_common(char *sysfs_path,
		 const char *device, int devno)
{
  const char *digit_string;
  char disk[MAX_DISK_CAT];
  char *of_path = find_obppath(sysfs_path);

  if (!of_path)
    return NULL;

  digit_string = trailing_digits (device);
  if (*digit_string == '\0')
    {
      snprintf(disk, sizeof (disk), "/disk@%d", devno);
    }
  else
    {
      int part;

      sscanf(digit_string, "%d", &part);
      snprintf(disk, sizeof (disk), "/disk@%d:%c", devno, 'a' + (part - 1));
    }
  strcat(of_path, disk);
  return of_path;
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

static char *
of_path_of_vdisk(const char *sys_devname __attribute__((unused)),
		 const char *device,
		 const char *devnode __attribute__((unused)),
		 const char *devicenode)
{
  char *sysfs_path, *p;
  int devno, junk;
  char *ret;

  sysfs_path = block_device_get_sysfs_path_and_link(devicenode);
  p = get_basename (sysfs_path);
  sscanf(p, "vdc-port-%d-%d", &devno, &junk);
  ret = __of_path_common (sysfs_path, device, devno);

  free (sysfs_path);
  return ret;
}

static char *
of_path_of_ide(const char *sys_devname __attribute__((unused)), const char *device,
	       const char *devnode __attribute__((unused)),
	       const char *devicenode)
{
  char *sysfs_path, *p;
  int chan, devno;
  char *ret;

  sysfs_path = block_device_get_sysfs_path_and_link(devicenode);
  p = get_basename (sysfs_path);
  sscanf(p, "%d.%d", &chan, &devno);

  ret = __of_path_common(sysfs_path, device, 2 * chan + devno);

  free (sysfs_path);
  return ret;
}

static int
vendor_is_ATA(const char *path)
{
  int fd, err;
  char *bufname;
  char bufcont[3];
  size_t path_size;

  path_size = strlen (path) + sizeof ("/vendor");

  bufname = xmalloc (path_size);

  snprintf (bufname, path_size, "%s/vendor", path);
  fd = open (bufname, O_RDONLY);
  if (fd < 0)
    grub_util_error (_("cannot open `%s': %s"), bufname, strerror (errno));

  memset(bufcont, 0, sizeof (bufcont));
  err = read(fd, bufcont, sizeof (bufcont));
  if (err < 0)
    grub_util_error (_("cannot open `%s': %s"), bufname, strerror (errno));

  close(fd);
  free (bufname);

  return (memcmp(bufcont, "ATA", 3) == 0);
}

static void
check_sas (char *sysfs_path, int *tgt)
{
  char *ed = strstr (sysfs_path, "end_device");
  char *p, *q, *path;
  char phy[16];
  int fd;
  size_t path_size;

  if (!ed)
    return;

  /* SAS devices are identified using disk@$PHY_ID */
  p = strdup (sysfs_path);
  ed = strstr(p, "end_device");

  q = ed;
  while (*q && *q != '/')
    q++;
  *q = '\0';

  path_size = (strlen (p) + strlen (ed)
	       + sizeof ("%s/sas_device/%s/phy_identifier"));
  path = xmalloc (path_size);
  snprintf (path, path_size, "%s/sas_device/%s/phy_identifier", p, ed);

  fd = open (path, O_RDONLY);
  if (fd < 0)
    grub_util_error (_("cannot open `%s': %s"), path, strerror (errno));

  memset (phy, 0, sizeof (phy));
  read (fd, phy, sizeof (phy));

  sscanf (phy, "%d", tgt);

  free (path);
  free (p);
  close (fd);
}

static char *
of_path_of_scsi(const char *sys_devname __attribute__((unused)), const char *device,
		const char *devnode __attribute__((unused)),
		const char *devicenode)
{
  const char *p, *digit_string, *disk_name;
  int host, bus, tgt, lun;
  char *sysfs_path, disk[MAX_DISK_CAT - sizeof ("/fp@0,0")];
  char *of_path;

  sysfs_path = block_device_get_sysfs_path_and_link(devicenode);
  p = get_basename (sysfs_path);
  sscanf(p, "%d:%d:%d:%d", &host, &bus, &tgt, &lun);
  check_sas (sysfs_path, &tgt);

  if (vendor_is_ATA(sysfs_path))
    {
      of_path = __of_path_common(sysfs_path, device, tgt);
      free (sysfs_path);
      return of_path;
    }

  of_path = find_obppath(sysfs_path);
  free (sysfs_path);
  if (!of_path)
    return NULL;

  if (strstr (of_path, "qlc"))
    strcat (of_path, "/fp@0,0");

  if (strstr (of_path, "sbus"))
    disk_name = "sd";
  else
    disk_name = "disk";

  digit_string = trailing_digits (device);
  if (strncmp (of_path, "/vdevice/", sizeof ("/vdevice/") - 1) == 0)
    {
      unsigned long id = 0x8000 | (tgt << 8) | (bus << 5) | lun;
      if (*digit_string == '\0')
	{
	  snprintf(disk, sizeof (disk), "/%s@%04lx000000000000", disk_name, id);
	}
      else
	{
	  int part;

	  sscanf(digit_string, "%d", &part);
	  snprintf(disk, sizeof (disk),
		   "/%s@%04lx000000000000:%c", disk_name, id, 'a' + (part - 1));
	}
    }
  else
    {
      if (*digit_string == '\0')
	{
	  snprintf(disk, sizeof (disk), "/%s@%x,%d", disk_name, tgt, lun);
	}
      else
	{
	  int part;

	  sscanf(digit_string, "%d", &part);
	  snprintf(disk, sizeof (disk),
		   "/%s@%x,%d:%c", disk_name, tgt, lun, 'a' + (part - 1));
	}
    }
  strcat(of_path, disk);
  return of_path;
}

static char *
strip_trailing_digits (const char *p)
{
  char *new, *end;

  new = strdup (p);
  end = new + strlen(new) - 1;
  while (end >= new)
    {
      if (! my_isdigit(*end))
	break;
      *end-- = '\0';
    }

  return new;
}

char *
grub_util_devname_to_ofpath (const char *sys_devname)
{
  char *name_buf, *device, *devnode, *devicenode, *ofpath;

  name_buf = xrealpath (sys_devname);

  device = get_basename (name_buf);
  devnode = strip_trailing_digits (name_buf);
  devicenode = strip_trailing_digits (device);

  if (device[0] == 'h' && device[1] == 'd')
    ofpath = of_path_of_ide(name_buf, device, devnode, devicenode);
  else if (device[0] == 's'
	   && (device[1] == 'd' || device[1] == 'r'))
    ofpath = of_path_of_scsi(name_buf, device, devnode, devicenode);
  else if (device[0] == 'v' && device[1] == 'd' && device[2] == 'i'
	   && device[3] == 's' && device[4] == 'k')
    ofpath = of_path_of_vdisk(name_buf, device, devnode, devicenode);
  else if (device[0] == 'f' && device[1] == 'd'
	   && device[2] == '0' && device[3] == '\0')
    /* All the models I've seen have a devalias "floppy".
       New models have no floppy at all. */
    ofpath = xstrdup ("floppy");
  else
    {
      grub_util_warn (_("unknown device type %s\n"), device);
      return NULL;
    }

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
      printf(_("Usage: %s DEVICE\n"), argv[0]);
      return 1;
    }

  of_path = grub_util_devname_to_ofpath (argv[1]);
  if (of_path)
    printf("%s\n", of_path);
  free (of_path);

  return 0;
}
#endif
