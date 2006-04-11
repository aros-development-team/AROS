/* biosdisk.c - emulate biosdisk */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004  Free Software Foundation, Inc.
 *
 *  GRUB is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <grub/machine/biosdisk.h>
#include <grub/disk.h>
#include <grub/partition.h>
#include <grub/pc_partition.h>
#include <grub/types.h>
#include <grub/err.h>
#include <grub/util/misc.h>

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
# ifndef BLKGETSIZE
#  define BLKGETSIZE    _IO(0x12,96)    /* return device size */
# endif /* ! BLKGETSIZE */
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

static char *map[256];

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
get_drive (const char *name)
{
  unsigned long drive;
  char *p;
  
  if ((name[0] != 'f' && name[0] != 'h') || name[1] != 'd')
    goto fail;

  drive = strtoul (name + 2, &p, 10);
  if (p == name + 2)
    goto fail;

  if (name[0] == 'h')
    drive += 0x80;

  if (drive > sizeof (map) / sizeof (map[0]))
    goto fail;
  
  return (int) drive;

 fail:
  grub_error (GRUB_ERR_UNKNOWN_DEVICE, "not a biosdisk");
  return -1;
}

static int
call_hook (int (*hook) (const char *name), int drive)
{
  char name[10];

  sprintf (name, (drive & 0x80) ? "hd%d" : "fd%d", drive & (~0x80));
  return hook (name);
}

static int
grub_util_biosdisk_iterate (int (*hook) (const char *name))
{
  unsigned i;

  for (i = 0; i < sizeof (map) / sizeof (map[0]); i++)
    if (map[i] && call_hook (hook, i))
      return 1;

  return 0;
}

static grub_err_t
grub_util_biosdisk_open (const char *name, grub_disk_t disk)
{
  int drive;
  struct stat st;
  
  drive = get_drive (name);
  if (drive < 0)
    return grub_errno;

  if (! map[drive])
    return grub_error (GRUB_ERR_BAD_DEVICE,
		       "no mapping exists for `%s'", name);
  
  disk->has_partitions = (drive & 0x80);
  disk->id = drive;

  /* Get the size.  */
#ifdef __linux__
  {
    unsigned long nr;
    int fd;

    fd = open (map[drive], O_RDONLY);
    if (! fd)
      return grub_error (GRUB_ERR_BAD_DEVICE, "cannot open `%s'", map[drive]);

    if (fstat (fd, &st) < 0 || ! S_ISBLK (st.st_mode))
      {
	close (fd);
	goto fail;
      }
    
    if (ioctl (fd, BLKGETSIZE, &nr))
      {
	close (fd);
	goto fail;
      }

    close (fd);
    disk->total_sectors = nr;
    
    grub_util_info ("the size of %s is %lu", name, disk->total_sectors);
    
    return GRUB_ERR_NONE;
  }

 fail:
  /* In GNU/Hurd, stat() will return the right size.  */
#elif !defined (__GNU__)
# warning "No special routine to get the size of a block device is implemented for your OS. This is not possibly fatal."
#endif
  if (stat (map[drive], &st) < 0)
    return grub_error (GRUB_ERR_BAD_DEVICE, "cannot stat `%s'", map[drive]);

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
  char *real_dev;

  real_dev = xstrdup (dev);
  
  if (have_devfs () && strcmp (real_dev + len - 5, "/disc") == 0)
    {
      p = real_dev + len - 4;
      format = "part%d";
    }
  else if ((strncmp (real_dev + 5, "hd", 2) == 0
	    || strncmp (real_dev + 5, "sd", 2) == 0)
	   && real_dev[7] >= 'a' && real_dev[7] <= 'z')
    {
      p = real_dev + 8;
      format = "%d";
    }
  else if (strncmp (real_dev + 5, "rd/c", 4) == 0)
    {
      p = strchr (real_dev + 9, 'd');
      if (! p)
	return 0;

      p++;
      while (*p && isdigit (*p))
	p++;

      format = "p%d";
    }
  else
    {
      free (real_dev);
      return 0;
    }

  for (i = 1; i < 10000; i++)
    {
      int fd;
      struct hd_geometry hdg;
      
      sprintf (p, format, i);
      fd = open (real_dev, O_RDONLY);
      if (! fd)
	{
	  free (real_dev);
	  return 0;
	}

      if (ioctl (fd, HDIO_GETGEO, &hdg))
	{
	  close (fd);
	  free (real_dev);
	  return 0;
	}

      close (fd);
      
      if (hdg.start == sector)
	{
	  strcpy (dev, real_dev);
	  free (real_dev);
	  return 1;
	}
    }

  free (real_dev);
  return 0;
}
#endif /* __linux__ */

static int
open_device (const grub_disk_t disk, unsigned long sector, int flags)
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
  
#ifdef __linux__
  /* Linux has a bug that the disk cache for a whole disk is not consistent
     with the one for a partition of the disk.  */
  {
    int is_partition = 0;
    char dev[PATH_MAX];
    
    strcpy (dev, map[disk->id]);
    if (disk->partition && strncmp (map[disk->id], "/dev/", 5) == 0)
      is_partition = linux_find_partition (dev, disk->partition->start);
    
    /* Open the partition.  */
    grub_util_info ("opening the device `%s'", dev);
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
  fd = open (map[disk->id], flags);
  if (fd < 0)
    {
      grub_error (GRUB_ERR_BAD_DEVICE, "cannot open `%s'", map[disk->id]);
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
	grub_error (GRUB_ERR_BAD_DEVICE, "cannot seek `%s'", map[disk->id]);
	close (fd);
	return -1;
      }
  }
#else
  {
    off_t offset = (off_t) sector << GRUB_DISK_SECTOR_BITS;

    if (lseek (fd, offset, SEEK_SET) != offset)
      {
	grub_error (GRUB_ERR_BAD_DEVICE, "cannot seek `%s'", map[disk->id]);
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
grub_util_biosdisk_read (grub_disk_t disk, unsigned long sector,
			 unsigned long size, char *buf)
{
  int fd;

  fd = open_device (disk, sector, O_RDONLY);
  if (fd < 0)
    return grub_errno;
  
#ifdef __linux__
  if (sector == 0 && size > 1)
    {
      /* Work around a bug in linux's ez remapping.  Linux remaps all
	 sectors that are read together with the MBR in one read.  It
	 should only remap the MBR, so we split the read in two 
	 parts. -jochen  */
      if (nread (fd, buf, GRUB_DISK_SECTOR_SIZE) != GRUB_DISK_SECTOR_SIZE)
	{
	  grub_error (GRUB_ERR_READ_ERROR, "cannot read `%s'", map[disk->id]);
	  close (fd);
	  return grub_errno;
	}
      
      buf += GRUB_DISK_SECTOR_SIZE;
      size--;
    }
#endif /* __linux__ */
  
  if (nread (fd, buf, size << GRUB_DISK_SECTOR_BITS)
      != (ssize_t) (size << GRUB_DISK_SECTOR_BITS))
    grub_error (GRUB_ERR_READ_ERROR, "cannot read from `%s'", map[disk->id]);

  close (fd);
  return grub_errno;
}

static grub_err_t
grub_util_biosdisk_write (grub_disk_t disk, unsigned long sector,
			  unsigned long size, const char *buf)
{
  int fd;

  fd = open_device (disk, sector, O_WRONLY);
  if (fd < 0)
    return grub_errno;
  
  if (nwrite (fd, buf, size << GRUB_DISK_SECTOR_BITS)
      != (ssize_t) (size << GRUB_DISK_SECTOR_BITS))
    grub_error (GRUB_ERR_WRITE_ERROR, "cannot write to `%s'", map[disk->id]);

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
      drive = get_drive (p);
      if (drive < 0 || drive >= (int) (sizeof (map) / sizeof (map[0])))
	show_error ("Bad device name");

      p = strchr (p, ')');
      if (! p)
	show_error ("No close parenthesis found");

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

      /* Multiple entries for a given drive is not allowed.  */
      if (map[drive])
	show_error ("Duplicated entry found");

#ifdef __linux__
      /* On Linux, the devfs uses symbolic links horribly, and that
	 confuses the interface very much, so use realpath to expand
	 symbolic links.  */
      map[drive] = xmalloc (PATH_MAX);
      if (! realpath (p, map[drive]))
	grub_util_error ("Cannot get the real path of `%s'", p);
#else
      map[drive] = xstrdup (p);
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
    free (map[i]);
  
  grub_disk_dev_unregister (&grub_util_biosdisk_dev);
}

static char *
make_device_name (int drive, int dos_part, int bsd_part)
{
  char *p;

  p = xmalloc (30);
  sprintf (p, (drive & 0x80) ? "hd%d" : "fd%d", drive & ~0x80);
  
  if (dos_part >= 0)
    sprintf (p + strlen (p), ",%d", dos_part);
  
  if (bsd_part >= 0)
    sprintf (p + strlen (p), ",%c", bsd_part + 'a');
  
  return p;
}

static char *
get_os_disk (const char *os_dev)
{
  char *path, *p;
  
#if defined(__linux__)
  path = xmalloc (PATH_MAX);
  if (! realpath (os_dev, path))
    return 0;
  
  if (strncmp ("/dev/", path, 5) == 0)
    {
      p = path + 5;

      if (have_devfs ())
	{
	  /* If this is an IDE disk.  */
	  if (strncmp ("/dev/ide/", p, 9) == 0)
	    {
	      p = strstr (p, "part");
	      if (p)
		strcpy (p, "disc");

	      return path;
	    }

	  /* If this is a SCSI disk.  */
	  if (strncmp ("/dev/scsi/", p, 10) == 0)
	    {
	      p = strstr (p, "part");
	      if (p)
		strcpy (p, "disc");

	      return path;
	    }
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
      
      /* If this is an IDE disk or a SCSI disk.  */
      if ((strncmp ("hd", p, 2) == 0
	   || strncmp ("sd", p, 2) == 0)
	  && p[2] >= 'a' && p[2] <= 'z')
	{
	  /* /dev/[hs]d[a-z][0-9]* */
	  p[3] = '\0';
	  return path;
	}
    }

  return path;
  
#elif defined(__GNU__)
  path = xstrdup (os_dev);
  if (strncmp ("/dev/sd", path, 7) == 0 || strncmp ("/dev/hd", path, 7) == 0)
    {
      p = strchr (path, 's');
      if (p)
	*p = '\0';
    }
  return path;

#else
# warning "The function `get_os_disk' might not work on your OS correctly."
  return xstrdup (os_dev);
#endif
}

static int
find_drive (const char *os_dev)
{
  int i;
  char *os_disk;

  os_disk = get_os_disk (os_dev);
  if (! os_disk)
    return -1;
  
  for (i = 0; i < (int) (sizeof (map) / sizeof (map[0])); i++)
    if (map[i] && strcmp (map[i], os_disk) == 0)
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

  drive = find_drive (os_dev);
  if (drive < 0)
    {
      grub_error (GRUB_ERR_BAD_DEVICE,
		  "no mapping exists for `%s'", os_dev);
      return 0;
    }
  
  if (! S_ISBLK (st.st_mode))
    return make_device_name (drive, -1, -1);
  
#if defined(__linux__)
  /* Linux counts partitions uniformly, whether a BSD partition or a DOS
     partition, so mapping them to GRUB devices is not trivial.
     Here, get the start sector of a partition by HDIO_GETGEO, and
     compare it with each partition GRUB recognizes.  */
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
 	struct grub_pc_partition *pcdata = 0;
	
	if (strcmp (partition->partmap->name, "pc_partition_map") == 0)
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
	
	if (hdg.start == partition->start)
	  {
	    if (pcdata)
	      {
		dos_part = pcdata->dos_part;
		bsd_part = pcdata->bsd_part;
	      }
	    else
	      {
		dos_part = 0;
		bsd_part = 0;
	      }
	    return 1;
	  }
	
	return 0;
      }
    
    name = make_device_name (drive, -1, -1);
    
    if (MAJOR (st.st_rdev) == FLOPPY_MAJOR)
      return name;
    
    fd = open (os_dev, O_RDONLY);
    if (! fd)
      {
	grub_error (GRUB_ERR_BAD_DEVICE, "cannot open `%s'", os_dev);
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
    
    if (hdg.start == 0)
      return name;

    grub_util_info ("opening the device %s", name);
    disk = grub_disk_open (name);
    free (name);
    
    if (! disk)
      return 0;
    
    if (grub_partition_iterate (disk, find_partition) != GRUB_ERR_NONE)
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
	if (p != q && n != LONG_MIN && n != LONG_MAX)
	  {
	    dos_part = (int) n;
	    
	    if (*q >= 'a' && *q <= 'g')
	      bsd_part = *q - 'a';
	  }
      }
    
    return make_device_name (drive, dos_part, bsd_part);
  }
  
#else
# warning "The function `grub_util_biosdisk_get_grub_dev' might not work on your OS correctly."
  return make_device_name (drive, -1, -1);
#endif
}
