/* biosdisk.c - emulate biosdisk */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004,2006,2007,2008,2009  Free Software Foundation, Inc.
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

#include <grub/disk.h>
#include <grub/partition.h>
#include <grub/msdos_partition.h>
#include <grub/types.h>
#include <grub/err.h>
#include <grub/util/misc.h>
#include <grub/util/hostdisk.h>
#include <grub/misc.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>

#ifdef __linux__
# include <sys/ioctl.h>         /* ioctl */
# if !defined(__GLIBC__) || \
        ((__GLIBC__ < 2) || ((__GLIBC__ == 2) && (__GLIBC_MINOR__ < 1)))
/* Maybe libc doesn't have large file support.  */
#  include <linux/unistd.h>     /* _llseek */
# endif /* (GLIBC < 2) || ((__GLIBC__ == 2) && (__GLIBC_MINOR < 1)) */
# ifndef BLKFLSBUF
#  define BLKFLSBUF     _IO (0x12,97)   /* flush buffer cache */
# endif /* ! BLKFLSBUF */
# include <sys/ioctl.h>		/* ioctl */
# ifndef HDIO_GETGEO
#  define HDIO_GETGEO	0x0301	/* get device geometry */
/* If HDIO_GETGEO is not defined, it is unlikely that hd_geometry is
   defined.  */
struct hd_geometry
{
  unsigned char heads;
  unsigned char sectors;
  unsigned short cylinders;
  unsigned long start;
};
# endif /* ! HDIO_GETGEO */
# ifndef BLKGETSIZE64
#  define BLKGETSIZE64  _IOR(0x12,114,size_t)    /* return device size */
# endif /* ! BLKGETSIZE64 */
# ifndef MAJOR
#  ifndef MINORBITS
#   define MINORBITS	8
#  endif /* ! MINORBITS */
#  define MAJOR(dev)	((unsigned) ((dev) >> MINORBITS))
# endif /* ! MAJOR */
# ifndef FLOPPY_MAJOR
#  define FLOPPY_MAJOR	2
# endif /* ! FLOPPY_MAJOR */
# ifndef LOOP_MAJOR
#  define LOOP_MAJOR	7
# endif /* ! LOOP_MAJOR */
#endif /* __linux__ */

#ifdef __CYGWIN__
# include <sys/ioctl.h>
# include <cygwin/fs.h> /* BLKGETSIZE64 */
# include <cygwin/hdreg.h> /* HDIO_GETGEO */
# define MAJOR(dev)	((unsigned) ((dev) >> 16))
# define FLOPPY_MAJOR	2
#endif

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
# include <sys/disk.h> /* DIOCGMEDIASIZE */
# include <sys/param.h>
# include <sys/sysctl.h>
#endif

#if defined(__APPLE__)
# include <sys/disk.h>
#endif

struct
{
  char *drive;
  char *device;
} map[256];

#ifdef __linux__
/* Check if we have devfs support.  */
static int
have_devfs (void)
{
  static int dev_devfsd_exists = -1;

  if (dev_devfsd_exists < 0)
    {
      struct stat st;

      dev_devfsd_exists = stat ("/dev/.devfsd", &st) == 0;
    }

  return dev_devfsd_exists;
}
#endif /* __linux__ */

static int
find_grub_drive (const char *name)
{
  unsigned int i;

  if (name)
    {
      for (i = 0; i < sizeof (map) / sizeof (map[0]); i++)
	if (map[i].drive && ! strcmp (map[i].drive, name))
	  return i;
    }

  return -1;
}

static int
find_free_slot ()
{
  unsigned int i;

  for (i = 0; i < sizeof (map) / sizeof (map[0]); i++)
    if (! map[i].drive)
      return i;

  return -1;
}

static int
grub_util_biosdisk_iterate (int (*hook) (const char *name))
{
  unsigned i;

  for (i = 0; i < sizeof (map) / sizeof (map[0]); i++)
    if (map[i].drive && hook (map[i].drive))
      return 1;

  return 0;
}

static grub_err_t
grub_util_biosdisk_open (const char *name, grub_disk_t disk)
{
  int drive;
  struct stat st;

  drive = find_grub_drive (name);
  if (drive < 0)
    return grub_error (GRUB_ERR_BAD_DEVICE,
		       "no mapping exists for `%s'", name);

  disk->has_partitions = 1;
  disk->id = drive;

  /* Get the size.  */
#if defined(__MINGW32__)
  {
    grub_uint64_t size;

    size = grub_util_get_disk_size (map[drive].device);

    if (size % 512)
      grub_util_error ("unaligned device size");

    disk->total_sectors = size >> 9;

    grub_util_info ("the size of %s is %llu", name, disk->total_sectors);

    return GRUB_ERR_NONE;
  }
#elif defined(__linux__) || defined(__CYGWIN__) || defined(__FreeBSD__) || \
      defined(__FreeBSD_kernel__) || defined(__APPLE__)
  {
    unsigned long long nr;
    int fd;

    fd = open (map[drive].device, O_RDONLY);
    if (fd == -1)
      return grub_error (GRUB_ERR_BAD_DEVICE, "cannot open `%s' while attempting to get disk size", map[drive].device);

# if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__APPLE__)
    if (fstat (fd, &st) < 0 || ! S_ISCHR (st.st_mode))
# else
    if (fstat (fd, &st) < 0 || ! S_ISBLK (st.st_mode))
# endif
      {
	close (fd);
	goto fail;
      }

# if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
    if (ioctl (fd, DIOCGMEDIASIZE, &nr))
# elif defined(__APPLE__)
    if (ioctl (fd, DKIOCGETBLOCKCOUNT, &nr))
# else
    if (ioctl (fd, BLKGETSIZE64, &nr))
# endif
      {
	close (fd);
	goto fail;
      }

    close (fd);

#if defined (__APPLE__)
    disk->total_sectors = nr;
#else
    disk->total_sectors = nr / 512;

    if (nr % 512)
      grub_util_error ("unaligned device size");
#endif

    grub_util_info ("the size of %s is %llu", name, disk->total_sectors);

    return GRUB_ERR_NONE;
  }

 fail:
  /* In GNU/Hurd, stat() will return the right size.  */
#elif !defined (__GNU__)
# warning "No special routine to get the size of a block device is implemented for your OS. This is not possibly fatal."
#endif
  if (stat (map[drive].device, &st) < 0)
    return grub_error (GRUB_ERR_BAD_DEVICE, "cannot stat `%s'", map[drive].device);

  disk->total_sectors = st.st_size >> GRUB_DISK_SECTOR_BITS;

  grub_util_info ("the size of %s is %lu", name, disk->total_sectors);

  return GRUB_ERR_NONE;
}

#ifdef __linux__
static int
linux_find_partition (char *dev, unsigned long sector)
{
  size_t len = strlen (dev);
  const char *format;
  char *p;
  int i;
  char real_dev[PATH_MAX];

  strcpy(real_dev, dev);

  if (have_devfs () && strcmp (real_dev + len - 5, "/disc") == 0)
    {
      p = real_dev + len - 4;
      format = "part%d";
    }
  else if (real_dev[len - 1] >= '0' && real_dev[len - 1] <= '9')
    {
      p = real_dev + len;
      format = "p%d";
    }
  else
    {
      p = real_dev + len;
      format = "%d";
    }

  for (i = 1; i < 10000; i++)
    {
      int fd;
      struct hd_geometry hdg;

      sprintf (p, format, i);
      fd = open (real_dev, O_RDONLY);
      if (fd == -1)
	return 0;

      if (ioctl (fd, HDIO_GETGEO, &hdg))
	{
	  close (fd);
	  return 0;
	}

      close (fd);

      if (hdg.start == sector)
	{
	  strcpy (dev, real_dev);
	  return 1;
	}
    }

  return 0;
}
#endif /* __linux__ */

static int
open_device (const grub_disk_t disk, grub_disk_addr_t sector, int flags)
{
  int fd;

#ifdef O_LARGEFILE
  flags |= O_LARGEFILE;
#endif
#ifdef O_SYNC
  flags |= O_SYNC;
#endif
#ifdef O_FSYNC
  flags |= O_FSYNC;
#endif
#ifdef O_BINARY
  flags |= O_BINARY;
#endif

#ifdef __linux__
  /* Linux has a bug that the disk cache for a whole disk is not consistent
     with the one for a partition of the disk.  */
  {
    int is_partition = 0;
    char dev[PATH_MAX];

    strcpy (dev, map[disk->id].device);
    if (disk->partition && strncmp (map[disk->id].device, "/dev/", 5) == 0)
      is_partition = linux_find_partition (dev, disk->partition->start);

    /* Open the partition.  */
    grub_dprintf ("hostdisk", "opening the device `%s' in open_device()", dev);
    fd = open (dev, flags);
    if (fd < 0)
      {
	grub_error (GRUB_ERR_BAD_DEVICE, "cannot open `%s'", dev);
	return -1;
      }

    /* Make the buffer cache consistent with the physical disk.  */
    ioctl (fd, BLKFLSBUF, 0);

    if (is_partition)
      sector -= disk->partition->start;
  }
#else /* ! __linux__ */
#if defined (__FreeBSD__) || defined(__FreeBSD_kernel__)
  int sysctl_flags, sysctl_oldflags;
  size_t sysctl_size = sizeof (sysctl_flags);

  if (sysctlbyname ("kern.geom.debugflags", &sysctl_oldflags, &sysctl_size, NULL, 0))
    {
      grub_error (GRUB_ERR_BAD_DEVICE, "cannot get current flags of sysctl kern.geom.debugflags");
      return -1;
    }
  sysctl_flags = sysctl_oldflags | 0x10;
  if (! (sysctl_oldflags & 0x10)
      && sysctlbyname ("kern.geom.debugflags", NULL , 0, &sysctl_flags, sysctl_size))
    {
      grub_error (GRUB_ERR_BAD_DEVICE, "cannot set flags of sysctl kern.geom.debugflags");
      return -1;
    }
#endif

  fd = open (map[disk->id].device, flags);

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
  if (! (sysctl_oldflags & 0x10)
      && sysctlbyname ("kern.geom.debugflags", NULL , 0, &sysctl_oldflags, sysctl_size))
    {
      grub_error (GRUB_ERR_BAD_DEVICE, "cannot set flags back to the old value for sysctl kern.geom.debugflags");
      return -1;
    }
#endif

#if defined(__APPLE__)
  /* If we can't have exclusive access, try shared access */
  if (fd < 0)
    fd = open(map[disk->id].device, flags | O_SHLOCK);
#endif

  if (fd < 0)
    {
      grub_error (GRUB_ERR_BAD_DEVICE, "cannot open `%s' in open_device()", map[disk->id].device);
      return -1;
    }
#endif /* ! __linux__ */

#if defined(__linux__) && (!defined(__GLIBC__) || \
        ((__GLIBC__ < 2) || ((__GLIBC__ == 2) && (__GLIBC_MINOR__ < 1))))
  /* Maybe libc doesn't have large file support.  */
  {
    loff_t offset, result;
    static int _llseek (uint filedes, ulong hi, ulong lo,
                        loff_t *res, uint wh);
    _syscall5 (int, _llseek, uint, filedes, ulong, hi, ulong, lo,
               loff_t *, res, uint, wh);

    offset = (loff_t) sector << GRUB_DISK_SECTOR_BITS;
    if (_llseek (fd, offset >> 32, offset & 0xffffffff, &result, SEEK_SET))
      {
	grub_error (GRUB_ERR_BAD_DEVICE, "cannot seek `%s'", map[disk->id].device);
	close (fd);
	return -1;
      }
  }
#else
  {
    off_t offset = (off_t) sector << GRUB_DISK_SECTOR_BITS;

    if (lseek (fd, offset, SEEK_SET) != offset)
      {
	grub_error (GRUB_ERR_BAD_DEVICE, "cannot seek `%s'", map[disk->id].device);
	close (fd);
	return -1;
      }
  }
#endif

  return fd;
}

/* Read LEN bytes from FD in BUF. Return less than or equal to zero if an
   error occurs, otherwise return LEN.  */
static ssize_t
nread (int fd, char *buf, size_t len)
{
  ssize_t size = len;

  while (len)
    {
      ssize_t ret = read (fd, buf, len);

      if (ret <= 0)
        {
          if (errno == EINTR)
            continue;
          else
            return ret;
        }

      len -= ret;
      buf += ret;
    }

  return size;
}

/* Write LEN bytes from BUF to FD. Return less than or equal to zero if an
   error occurs, otherwise return LEN.  */
static ssize_t
nwrite (int fd, const char *buf, size_t len)
{
  ssize_t size = len;

  while (len)
    {
      ssize_t ret = write (fd, buf, len);

      if (ret <= 0)
        {
          if (errno == EINTR)
            continue;
          else
            return ret;
        }

      len -= ret;
      buf += ret;
    }

  return size;
}

static grub_err_t
grub_util_biosdisk_read (grub_disk_t disk, grub_disk_addr_t sector,
			 grub_size_t size, char *buf)
{
  int fd;

  fd = open_device (disk, sector, O_RDONLY);
  if (fd < 0)
    return grub_errno;

#ifdef __linux__
  if (sector == 0 && size > 1)
    {
      /* Work around a bug in Linux ez remapping.  Linux remaps all
	 sectors that are read together with the MBR in one read.  It
	 should only remap the MBR, so we split the read in two
	 parts. -jochen  */
      if (nread (fd, buf, GRUB_DISK_SECTOR_SIZE) != GRUB_DISK_SECTOR_SIZE)
	{
	  grub_error (GRUB_ERR_READ_ERROR, "cannot read `%s'", map[disk->id].device);
	  close (fd);
	  return grub_errno;
	}

      buf += GRUB_DISK_SECTOR_SIZE;
      size--;
    }
#endif /* __linux__ */

  if (nread (fd, buf, size << GRUB_DISK_SECTOR_BITS)
      != (ssize_t) (size << GRUB_DISK_SECTOR_BITS))
    grub_error (GRUB_ERR_READ_ERROR, "cannot read from `%s'", map[disk->id].device);

  close (fd);
  return grub_errno;
}

static grub_err_t
grub_util_biosdisk_write (grub_disk_t disk, grub_disk_addr_t sector,
			  grub_size_t size, const char *buf)
{
  int fd;

  fd = open_device (disk, sector, O_WRONLY);
  if (fd < 0)
    return grub_errno;

  if (nwrite (fd, buf, size << GRUB_DISK_SECTOR_BITS)
      != (ssize_t) (size << GRUB_DISK_SECTOR_BITS))
    grub_error (GRUB_ERR_WRITE_ERROR, "cannot write to `%s'", map[disk->id].device);

  close (fd);
  return grub_errno;
}

static struct grub_disk_dev grub_util_biosdisk_dev =
  {
    .name = "biosdisk",
    .id = GRUB_DISK_DEVICE_BIOSDISK_ID,
    .iterate = grub_util_biosdisk_iterate,
    .open = grub_util_biosdisk_open,
    .close = 0,
    .read = grub_util_biosdisk_read,
    .write = grub_util_biosdisk_write,
    .next = 0
  };

static void
read_device_map (const char *dev_map)
{
  FILE *fp;
  char buf[1024];	/* XXX */
  int lineno = 0;
  struct stat st;

  auto void show_error (const char *msg);
  void show_error (const char *msg)
    {
      grub_util_error ("%s:%d: %s", dev_map, lineno, msg);
    }

  fp = fopen (dev_map, "r");
  if (! fp)
    grub_util_error ("Cannot open `%s'", dev_map);

  while (fgets (buf, sizeof (buf), fp))
    {
      char *p = buf;
      char *e;
      int drive;

      lineno++;

      /* Skip leading spaces.  */
      while (*p && isspace (*p))
	p++;

      /* If the first character is `#' or NUL, skip this line.  */
      if (*p == '\0' || *p == '#')
	continue;

      if (*p != '(')
	show_error ("No open parenthesis found");

      p++;
      /* Find a free slot.  */
      drive = find_free_slot ();
      if (drive < 0)
	show_error ("Map table size exceeded");

      e = p;
      p = strchr (p, ')');
      if (! p)
	show_error ("No close parenthesis found");

      map[drive].drive = xmalloc (p - e + sizeof ('\0'));
      strncpy (map[drive].drive, e, p - e + sizeof ('\0'));
      map[drive].drive[p - e] = '\0';

      p++;
      /* Skip leading spaces.  */
      while (*p && isspace (*p))
	p++;

      if (*p == '\0')
	show_error ("No filename found");

      /* NUL-terminate the filename.  */
      e = p;
      while (*e && ! isspace (*e))
	e++;
      *e = '\0';

#ifdef __MINGW32__
      (void) st;
      if (grub_util_get_disk_size (p) == -1LL)
#else
      if (stat (p, &st) == -1)
#endif
	{
	  free (map[drive].drive);
	  map[drive].drive = NULL;
	  grub_util_info ("Cannot stat `%s', skipping", p);
	  continue;
	}

#ifdef __linux__
      /* On Linux, the devfs uses symbolic links horribly, and that
	 confuses the interface very much, so use realpath to expand
	 symbolic links.  */
      map[drive].device = xmalloc (PATH_MAX);
      if (! realpath (p, map[drive].device))
	grub_util_error ("Cannot get the real path of `%s'", p);
#else
      map[drive].device = xstrdup (p);
#endif
    }

  fclose (fp);
}

void
grub_util_biosdisk_init (const char *dev_map)
{
  read_device_map (dev_map);
  grub_disk_dev_register (&grub_util_biosdisk_dev);
}

void
grub_util_biosdisk_fini (void)
{
  unsigned i;

  for (i = 0; i < sizeof (map) / sizeof (map[0]); i++)
    {
      if (map[i].drive)
	free (map[i].drive);
      if (map[i].device)
	free (map[i].device);
      map[i].drive = map[i].device = NULL;
    }

  grub_disk_dev_unregister (&grub_util_biosdisk_dev);
}

static char *
make_device_name (int drive, int dos_part, int bsd_part)
{
  int len = strlen(map[drive].drive);
  char *p;

  if (dos_part >= 0)
    {
      /* Add in char length of dos_part+1 */
      int tmp = dos_part + 1;
      len++;
      while ((tmp /= 10) != 0)
	len++;
    }
  if (bsd_part >= 0)
    len += 2;

  /* Length to alloc is: char length of map[drive].drive, plus
   *                     char length of (dos_part+1) or of bsd_part, plus
   *                     2 for the comma and a null/end of string (\0)
   */
  p = xmalloc (len + 2);
  sprintf (p, "%s", map[drive].drive);

  if (dos_part >= 0)
    sprintf (p + strlen (p), ",%d", dos_part + 1);

  if (bsd_part >= 0)
    sprintf (p + strlen (p), ",%c", bsd_part + 'a');

  return p;
}

static char *
convert_system_partition_to_system_disk (const char *os_dev)
{
#if defined(__linux__)
  char *path = xmalloc (PATH_MAX);
  if (! realpath (os_dev, path))
    return 0;

  if (strncmp ("/dev/", path, 5) == 0)
    {
      char *p = path + 5;

      /* If this is an IDE disk.  */
      if (strncmp ("ide/", p, 4) == 0)
	{
	  p = strstr (p, "part");
	  if (p)
	    strcpy (p, "disc");

	  return path;
	}

      /* If this is a SCSI disk.  */
      if (strncmp ("scsi/", p, 5) == 0)
	{
	  p = strstr (p, "part");
	  if (p)
	    strcpy (p, "disc");

	  return path;
	}

      /* If this is a DAC960 disk.  */
      if (strncmp ("rd/c", p, 4) == 0)
	{
	  /* /dev/rd/c[0-9]+d[0-9]+(p[0-9]+)? */
	  p = strchr (p, 'p');
	  if (p)
	    *p = '\0';

	  return path;
	}

      /* If this is a Mylex AcceleRAID Array.  */
      if (strncmp ("rs/c", p, 4) == 0)
	{
	  /* /dev/rd/c[0-9]+d[0-9]+(p[0-9]+)? */
	  p = strchr (p, 'p');
	  if (p)
	    *p = '\0';

	  return path;
	}
      /* If this is a CCISS disk.  */
      if (strncmp ("cciss/c", p, sizeof ("cciss/c") - 1) == 0)
	{
	  /* /dev/cciss/c[0-9]+d[0-9]+(p[0-9]+)? */
	  p = strchr (p, 'p');
	  if (p)
	    *p = '\0';

	  return path;
	}

      /* If this is a Compaq Intelligent Drive Array.  */
      if (strncmp ("ida/c", p, sizeof ("ida/c") - 1) == 0)
	{
	  /* /dev/ida/c[0-9]+d[0-9]+(p[0-9]+)? */
	  p = strchr (p, 'p');
	  if (p)
	    *p = '\0';

	  return path;
	}

      /* If this is an I2O disk.  */
      if (strncmp ("i2o/hd", p, sizeof ("i2o/hd") - 1) == 0)
      	{
	  /* /dev/i2o/hd[a-z]([0-9]+)? */
	  p[sizeof ("i2o/hda") - 1] = '\0';
	  return path;
	}

      /* If this is a MultiMediaCard (MMC).  */
      if (strncmp ("mmcblk", p, sizeof ("mmcblk") - 1) == 0)
	{
	  /* /dev/mmcblk[0-9]+(p[0-9]+)? */
	  p = strchr (p, 'p');
	  if (p)
	    *p = '\0';

	  return path;
	}

      /* If this is an IDE, SCSI or Virtio disk.  */
      if (strncmp ("vdisk", p, 5) == 0
	  && p[5] >= 'a' && p[5] <= 'z')
	{
	  /* /dev/vdisk[a-z][0-9]* */
	  p[6] = '\0';
	  return path;
	}
      if ((strncmp ("hd", p, 2) == 0
	   || strncmp ("vd", p, 2) == 0
	   || strncmp ("sd", p, 2) == 0)
	  && p[2] >= 'a' && p[2] <= 'z')
	{
	  /* /dev/[hsv]d[a-z][0-9]* */
	  p[3] = '\0';
	  return path;
	}

      /* If this is a Xen virtual block device.  */
      if ((strncmp ("xvd", p, 3) == 0) && p[3] >= 'a' && p[3] <= 'z')
	{
	  /* /dev/xvd[a-z][0-9]* */
	  p[4] = '\0';
	  return path;
	}
    }

  return path;

#elif defined(__GNU__)
  char *path = xstrdup (os_dev);
  if (strncmp ("/dev/sd", path, 7) == 0 || strncmp ("/dev/hd", path, 7) == 0)
    {
      char *p = strchr (path + 7, 's');
      if (p)
	*p = '\0';
    }
  return path;

#elif defined(__CYGWIN__)
  char *path = xstrdup (os_dev);
  if (strncmp ("/dev/sd", path, 7) == 0 && 'a' <= path[7] && path[7] <= 'z')
    path[8] = 0;
  return path;

#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__APPLE__)
  char *path = xstrdup (os_dev);
  if (strncmp ("/dev/", path, 5) == 0)
    {
      char *p;
      for (p = path + 5; *p; ++p)
        if (grub_isdigit(*p))
          {
            p = strchr (p, 's');
            if (p)
              *p = '\0';
            break;
          }
    }
  return path;

#else
# warning "The function `convert_system_partition_to_system_disk' might not work on your OS correctly."
  return xstrdup (os_dev);
#endif
}

#if defined(__linux__) || defined(__CYGWIN__)
static int
device_is_wholedisk (const char *os_dev)
{
  int len = strlen (os_dev);

  if (os_dev[len - 1] < '0' || os_dev[len - 1] > '9')
    return 1;
  return 0;
}
#endif

static int
find_system_device (const char *os_dev)
{
  int i;
  char *os_disk;

  os_disk = convert_system_partition_to_system_disk (os_dev);
  if (! os_disk)
    return -1;

  for (i = 0; i < (int) (sizeof (map) / sizeof (map[0])); i++)
    if (map[i].device && strcmp (map[i].device, os_disk) == 0)
      {
	free (os_disk);
	return i;
      }

  free (os_disk);
  return -1;
}

char *
grub_util_biosdisk_get_grub_dev (const char *os_dev)
{
  struct stat st;
  int drive;

  if (stat (os_dev, &st) < 0)
    {
      grub_error (GRUB_ERR_BAD_DEVICE, "cannot stat `%s'", os_dev);
      return 0;
    }

  drive = find_system_device (os_dev);
  if (drive < 0)
    {
      grub_error (GRUB_ERR_BAD_DEVICE,
		  "no mapping exists for `%s'", os_dev);
      return 0;
    }

  if (grub_strcmp (os_dev, convert_system_partition_to_system_disk (os_dev))
      == 0)
    return make_device_name (drive, -1, -1);

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__APPLE__)
  if (! S_ISCHR (st.st_mode))
#else
  if (! S_ISBLK (st.st_mode))
#endif
    return make_device_name (drive, -1, -1);

#if defined(__linux__) || defined(__CYGWIN__)
  /* Linux counts partitions uniformly, whether a BSD partition or a DOS
     partition, so mapping them to GRUB devices is not trivial.
     Here, get the start sector of a partition by HDIO_GETGEO, and
     compare it with each partition GRUB recognizes.

     Cygwin /dev/sdXN emulation uses Windows partition mapping. It
     does not count the extended partition and missing primary
     partitions.  Use same method as on Linux here.  */
  {
    char *name;
    grub_disk_t disk;
    int fd;
    struct hd_geometry hdg;
    int dos_part = -1;
    int bsd_part = -1;
    auto int find_partition (grub_disk_t disk,
			     const grub_partition_t partition);

    int find_partition (grub_disk_t disk __attribute__ ((unused)),
			const grub_partition_t partition)
      {
 	struct grub_msdos_partition *pcdata = NULL;

	if (strcmp (partition->partmap->name, "part_msdos") == 0)
	  pcdata = partition->data;

	if (pcdata)
	  {
	    if (pcdata->bsd_part < 0)
	      grub_util_info ("DOS partition %d starts from %lu",
			      pcdata->dos_part, partition->start);
	    else
	      grub_util_info ("BSD partition %d,%c starts from %lu",
			      pcdata->dos_part, pcdata->bsd_part + 'a',
			      partition->start);
	  }
	else
	  {
	      grub_util_info ("Partition %d starts from %lu",
			      partition->index, partition->start);
	  }

	if (hdg.start == partition->start)
	  {
	    if (pcdata)
	      {
		dos_part = pcdata->dos_part;
		bsd_part = pcdata->bsd_part;
	      }
	    else
	      {
		dos_part = partition->index;
		bsd_part = -1;
	      }
	    return 1;
	  }

	return 0;
      }

    name = make_device_name (drive, -1, -1);

    if (MAJOR (st.st_rdev) == FLOPPY_MAJOR)
      return name;

    fd = open (os_dev, O_RDONLY);
    if (fd == -1)
      {
	grub_error (GRUB_ERR_BAD_DEVICE, "cannot open `%s' while attempting to get disk geometry", os_dev);
	free (name);
	return 0;
      }

    if (ioctl (fd, HDIO_GETGEO, &hdg))
      {
	grub_error (GRUB_ERR_BAD_DEVICE,
		    "cannot get geometry of `%s'", os_dev);
	close (fd);
	free (name);
	return 0;
      }

    close (fd);

    grub_util_info ("%s starts from %lu", os_dev, hdg.start);

    if (hdg.start == 0 && device_is_wholedisk (os_dev))
      return name;

    grub_util_info ("opening the device %s", name);
    disk = grub_disk_open (name);
    free (name);

    if (! disk)
      return 0;

    grub_partition_iterate (disk, find_partition);
    if (grub_errno != GRUB_ERR_NONE)
      {
	grub_disk_close (disk);
	return 0;
      }

    if (dos_part < 0)
      {
	grub_disk_close (disk);
	grub_error (GRUB_ERR_BAD_DEVICE,
		    "cannot find the partition of `%s'", os_dev);
	return 0;
      }

    return make_device_name (drive, dos_part, bsd_part);
  }

#elif defined(__GNU__)
  /* GNU uses "/dev/[hs]d[0-9]+(s[0-9]+[a-z]?)?".  */
  {
    char *p;
    int dos_part = -1;
    int bsd_part = -1;

    p = strrchr (os_dev, 's');
    if (p)
      {
	long int n;
	char *q;

	p++;
	n = strtol (p, &q, 10);
	if (p != q && n != GRUB_LONG_MIN && n != GRUB_LONG_MAX)
	  {
	    dos_part = (int) n;

	    if (*q >= 'a' && *q <= 'g')
	      bsd_part = *q - 'a';
	  }
      }

    return make_device_name (drive, dos_part, bsd_part);
  }

#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__APPLE__)
  /* FreeBSD uses "/dev/[a-z]+[0-9]+(s[0-9]+[a-z]?)?".  */
  {
    int dos_part = -1;
    int bsd_part = -1;

    if (strncmp ("/dev/", os_dev, 5) == 0)
      {
        const char *p;
        char *q;
        long int n;

        for (p = os_dev + 5; *p; ++p)
          if (grub_isdigit(*p))
            {
              p = strchr (p, 's');
              if (p)
                {
                  p++;
                  n = strtol (p, &q, 10);
                  if (p != q && n != GRUB_LONG_MIN && n != GRUB_LONG_MAX)
                    {
                      dos_part = (int) n - 1;

                      if (*q >= 'a' && *q <= 'g')
                        bsd_part = *q - 'a';
                    }
                }
              break;
            }
      }

    return make_device_name (drive, dos_part, bsd_part);
  }

#else
# warning "The function `grub_util_biosdisk_get_grub_dev' might not work on your OS correctly."
  return make_device_name (drive, -1, -1);
#endif
}
