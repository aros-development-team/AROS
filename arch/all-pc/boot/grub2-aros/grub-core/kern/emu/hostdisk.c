/* hostdisk.c - emulate biosdisk */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004,2006,2007,2008,2009,2010  Free Software Foundation, Inc.
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
#include <grub/emu/misc.h>
#include <grub/emu/hostdisk.h>
#include <grub/misc.h>
#include <grub/i18n.h>
#include <grub/list.h>

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

#ifdef HAVE_DEVICE_MAPPER
# include <libdevmapper.h>
#endif

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
# define HAVE_DIOCGDINFO
# include <sys/ioctl.h>
# include <sys/disklabel.h>    /* struct disklabel */
#else /* !defined(__NetBSD__) && !defined(__FreeBSD__) && !defined(__FreeBSD_kernel__) */
# undef HAVE_DIOCGDINFO
#endif /* defined(__NetBSD__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) */

#if defined(__NetBSD__)
# ifdef HAVE_GETRAWPARTITION
#  include <util.h>    /* getrawpartition */
# endif /* HAVE_GETRAWPARTITION */
# include <sys/fdio.h>
# ifndef FLOPPY_MAJOR
#  define FLOPPY_MAJOR	2
# endif /* ! FLOPPY_MAJOR */
# ifndef RAW_FLOPPY_MAJOR
#  define RAW_FLOPPY_MAJOR	9
# endif /* ! RAW_FLOPPY_MAJOR */
#endif /* defined(__NetBSD__) */

struct
{
  char *drive;
  char *device;
} map[256];

struct grub_util_biosdisk_data
{
  char *dev;
  int access_mode;
  int fd;
};

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

#if defined(__NetBSD__)
/* Adjust device driver parameters.  This function should be called just
   after successfully opening the device.  For now, it simply prevents the
   floppy driver from retrying operations on failure, as otherwise the
   driver takes a while to abort when there is no floppy in the drive.  */
static void
configure_device_driver (int fd)
{
  struct stat st;

  if (fstat (fd, &st) < 0 || ! S_ISCHR (st.st_mode))
    return;
  if (major(st.st_rdev) == RAW_FLOPPY_MAJOR)
    {
      int floppy_opts;

      if (ioctl (fd, FDIOCGETOPTS, &floppy_opts) == -1)
	return;
      floppy_opts |= FDOPT_NORETRY;
      if (ioctl (fd, FDIOCSETOPTS, &floppy_opts) == -1)
	return;
    }
}
#endif /* defined(__NetBSD__) */

static int
find_grub_drive (const char *name)
{
  unsigned int i;

  if (name)
    {
      for (i = 0; i < ARRAY_SIZE (map); i++)
	if (map[i].drive && ! strcmp (map[i].drive, name))
	  return i;
    }

  return -1;
}

static int
find_free_slot (void)
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
  struct grub_util_biosdisk_data *data;

  drive = find_grub_drive (name);
  if (drive < 0)
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE,
		       "no mapping exists for `%s'", name);

  disk->id = drive;
  disk->data = data = xmalloc (sizeof (struct grub_util_biosdisk_data));
  data->dev = NULL;
  data->access_mode = 0;
  data->fd = -1;

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
      defined(__FreeBSD_kernel__) || defined(__APPLE__) || defined(__NetBSD__)
  {
# if defined(__NetBSD__)
    struct disklabel label;
# else
    unsigned long long nr;
# endif
    int fd;

    fd = open (map[drive].device, O_RDONLY);
    if (fd == -1)
      return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "cannot open `%s' while attempting to get disk size", map[drive].device);

# if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__APPLE__) || defined(__NetBSD__)
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
# elif defined(__NetBSD__)
    configure_device_driver (fd);
    if (ioctl (fd, DIOCGDINFO, &label) == -1)
# else
    if (ioctl (fd, BLKGETSIZE64, &nr))
# endif
      {
	close (fd);
	goto fail;
      }

    close (fd);

# if defined (__APPLE__)
    disk->total_sectors = nr;
# elif defined(__NetBSD__)
    disk->total_sectors = label.d_secperunit;
# else
    disk->total_sectors = nr / 512;

    if (nr % 512)
      grub_util_error ("unaligned device size");
# endif

    grub_util_info ("the size of %s is %llu", name, disk->total_sectors);

    return GRUB_ERR_NONE;
  }

 fail:
  /* In GNU/Hurd, stat() will return the right size.  */
#elif !defined (__GNU__)
# warning "No special routine to get the size of a block device is implemented for your OS. This is not possibly fatal."
#endif
  if (stat (map[drive].device, &st) < 0)
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "cannot stat `%s'", map[drive].device);

  disk->total_sectors = st.st_size >> GRUB_DISK_SECTOR_BITS;

  grub_util_info ("the size of %s is %lu", name, disk->total_sectors);

  return GRUB_ERR_NONE;
}

#ifdef HAVE_DEVICE_MAPPER
static int
device_is_mapped (const char *dev)
{
  struct stat st;

  if (stat (dev, &st) < 0)
    return 0;

  return dm_is_dm_major (major (st.st_rdev));
}
#endif /* HAVE_DEVICE_MAPPER */

#if defined(__linux__) || defined(__CYGWIN__) || defined(HAVE_DIOCGDINFO)
static grub_disk_addr_t
find_partition_start (const char *dev)
{
  int fd;
# if !defined(HAVE_DIOCGDINFO)
  struct hd_geometry hdg;
# else /* defined(HAVE_DIOCGDINFO) */
  struct disklabel label;
  int p_index;
# endif /* !defined(HAVE_DIOCGDINFO) */

# ifdef HAVE_DEVICE_MAPPER
  if (grub_device_mapper_supported () && device_is_mapped (dev)) {
    struct dm_task *task = NULL;
    grub_uint64_t start, length;
    char *target_type, *params, *space;
    grub_disk_addr_t partition_start;

    /* If any device-mapper operation fails, we fall back silently to
       HDIO_GETGEO.  */
    task = dm_task_create (DM_DEVICE_TABLE);
    if (! task)
      {
	grub_dprintf ("hostdisk", "dm_task_create failed\n");
	goto devmapper_fail;
      }

    if (! dm_task_set_name (task, dev))
      {
	grub_dprintf ("hostdisk", "dm_task_set_name failed\n");
	goto devmapper_fail;
      }

    if (! dm_task_run (task))
      {
	grub_dprintf ("hostdisk", "dm_task_run failed\n");
	goto devmapper_fail;
      }

    dm_get_next_target (task, NULL, &start, &length, &target_type, &params);
    if (! target_type)
      {
	grub_dprintf ("hostdisk", "no dm target\n");
	goto devmapper_fail;
      }
    if (strcmp (target_type, "linear") != 0)
      {
	grub_dprintf ("hostdisk", "ignoring dm target %s (not linear)\n",
		      target_type);
	goto devmapper_fail;
      }
    if (! params)
      {
	grub_dprintf ("hostdisk", "no dm params\n");
	goto devmapper_fail;
      }

    /* The params string for a linear target looks like this:
         DEVICE-NAME START-SECTOR
       Parse this out.  */
    space = strchr (params, ' ');
    if (! space)
      goto devmapper_fail;
    errno = 0;
    partition_start = strtoull (space + 1, NULL, 10);
    if (errno == 0)
      {
	grub_dprintf ("hostdisk", "dm %s starts at %llu\n",
		      dev, (unsigned long long) partition_start);
	dm_task_destroy (task);
	return partition_start;
      }

devmapper_fail:
    if (task)
      dm_task_destroy (task);
  }
# endif /* HAVE_DEVICE_MAPPER */

  fd = open (dev, O_RDONLY);
  if (fd == -1)
    {
      grub_error (GRUB_ERR_BAD_DEVICE,
# if !defined(HAVE_DIOCGDINFO)
		  "cannot open `%s' while attempting to get disk geometry", dev);
# else /* defined(HAVE_DIOCGDINFO) */
		  "cannot open `%s' while attempting to get disk label", dev);
# endif /* !defined(HAVE_DIOCGDINFO) */
      return 0;
    }

# if !defined(HAVE_DIOCGDINFO)
  if (ioctl (fd, HDIO_GETGEO, &hdg))
# else /* defined(HAVE_DIOCGDINFO) */
#  if defined(__NetBSD__)
  configure_device_driver (fd);
#  endif /* defined(__NetBSD__) */
  if (ioctl (fd, DIOCGDINFO, &label) == -1)
# endif /* !defined(HAVE_DIOCGDINFO) */
    {
      grub_error (GRUB_ERR_BAD_DEVICE,
# if !defined(HAVE_DIOCGDINFO)
		  "cannot get disk geometry of `%s'", dev);
# else /* defined(HAVE_DIOCGDINFO) */
		  "cannot get disk label of `%s'", dev);
# endif /* !defined(HAVE_DIOCGDINFO) */
      close (fd);
      return 0;
    }

  close (fd);

# if !defined(HAVE_DIOCGDINFO)
  return hdg.start;
# else /* defined(HAVE_DIOCGDINFO) */
  p_index = dev[strlen(dev) - 1] - 'a';

  if (p_index >= label.d_npartitions)
    {
      grub_error (GRUB_ERR_BAD_DEVICE,
		  "no disk label entry for `%s'", dev);
      return 0;
    }
  return (grub_disk_addr_t) label.d_partitions[p_index].p_offset;
# endif /* !defined(HAVE_DIOCGDINFO) */
}
#endif /* __linux__ || __CYGWIN__ || HAVE_DIOCGDINFO */

#ifdef __linux__
/* Cache of partition start sectors for each disk.  */
struct linux_partition_cache
{
  struct linux_partition_cache *next;
  char *dev;
  unsigned long start;
  int partno;
};

struct linux_partition_cache *linux_partition_cache_list;

static int
linux_find_partition (char *dev, unsigned long sector)
{
  size_t len = strlen (dev);
  const char *format;
  char *p;
  int i;
  char real_dev[PATH_MAX];
  struct linux_partition_cache *cache;

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

  for (cache = linux_partition_cache_list; cache; cache = cache->next)
    {
      if (strcmp (cache->dev, dev) == 0 && cache->start == sector)
	{
	  sprintf (p, format, cache->partno);
	  strcpy (dev, real_dev);
	  return 1;
	}
    }

  for (i = 1; i < 10000; i++)
    {
      int fd;
      grub_disk_addr_t start;

      sprintf (p, format, i);

      fd = open (real_dev, O_RDONLY);
      if (fd == -1)
	return 0;
      close (fd);

      start = find_partition_start (real_dev);
      /* We don't care about errors here.  */
      grub_errno = GRUB_ERR_NONE;

      if (start == sector)
	{
	  struct linux_partition_cache *new_cache_item;

	  new_cache_item = xmalloc (sizeof *new_cache_item);
	  new_cache_item->dev = xstrdup (dev);
	  new_cache_item->start = start;
	  new_cache_item->partno = i;
	  grub_list_push (GRUB_AS_LIST_P (&linux_partition_cache_list),
			  GRUB_AS_LIST (new_cache_item));

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
  struct grub_util_biosdisk_data *data = disk->data;

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
    grub_disk_addr_t part_start = 0;

    part_start = grub_partition_get_start (disk->partition);

    strcpy (dev, map[disk->id].device);
    if (disk->partition && sector >= part_start
	&& strncmp (map[disk->id].device, "/dev/", 5) == 0)
      is_partition = linux_find_partition (dev, part_start);

    if (data->dev && strcmp (data->dev, dev) == 0 &&
	data->access_mode == (flags & O_ACCMODE))
      {
	grub_dprintf ("hostdisk", "reusing open device `%s'\n", dev);
	fd = data->fd;
      }
    else
      {
	free (data->dev);
	if (data->fd != -1)
	  close (data->fd);

	/* Open the partition.  */
	grub_dprintf ("hostdisk", "opening the device `%s' in open_device()\n", dev);
	fd = open (dev, flags);
	if (fd < 0)
	  {
	    grub_error (GRUB_ERR_BAD_DEVICE, "cannot open `%s'", dev);
	    return -1;
	  }

	/* Flush the buffer cache to the physical disk.
	   XXX: This also empties the buffer cache.  */
	ioctl (fd, BLKFLSBUF, 0);

	data->dev = xstrdup (dev);
	data->access_mode = (flags & O_ACCMODE);
	data->fd = fd;
      }

    if (is_partition)
      sector -= part_start;
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

  if (data->dev && strcmp (data->dev, map[disk->id].device) == 0 &&
      data->access_mode == (flags & O_ACCMODE))
    {
      grub_dprintf ("hostdisk", "reusing open device `%s'\n", data->dev);
      fd = data->fd;
    }
  else
    {
      free (data->dev);
      if (data->fd != -1)
	close (data->fd);

      fd = open (map[disk->id].device, flags);
      if (fd >= 0)
	{
	  data->dev = xstrdup (map[disk->id].device);
	  data->access_mode = (flags & O_ACCMODE);
	  data->fd = fd;
	}
    }

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

#if defined(__NetBSD__)
  configure_device_driver (fd);
#endif /* defined(__NetBSD__) */

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

  /* Split pre-partition and partition reads.  */
  if (disk->partition && sector < disk->partition->start
      && sector + size > disk->partition->start)
    {
      grub_err_t err;
      err = grub_util_biosdisk_read (disk, sector,
				     disk->partition->start - sector,
				     buf);
      if (err)
	return err;

      return grub_util_biosdisk_read (disk, disk->partition->start,
				      size - (disk->partition->start - sector),
				      buf + ((disk->partition->start - sector)
					     << GRUB_DISK_SECTOR_BITS));
    }

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

  return grub_errno;
}

static grub_err_t
grub_util_biosdisk_write (grub_disk_t disk, grub_disk_addr_t sector,
			  grub_size_t size, const char *buf)
{
  int fd;

  /* Split pre-partition and partition writes.  */
  if (disk->partition && sector < disk->partition->start
      && sector + size > disk->partition->start)
    {
      grub_err_t err;
      err = grub_util_biosdisk_write (disk, sector,
				      disk->partition->start - sector,
				      buf);
      if (err)
	return err;

      return grub_util_biosdisk_write (disk, disk->partition->start,
				       size - (disk->partition->start - sector),
				       buf + ((disk->partition->start - sector)
					      << GRUB_DISK_SECTOR_BITS));
    }

  fd = open_device (disk, sector, O_WRONLY);
  if (fd < 0)
    return grub_errno;

  if (nwrite (fd, buf, size << GRUB_DISK_SECTOR_BITS)
      != (ssize_t) (size << GRUB_DISK_SECTOR_BITS))
    grub_error (GRUB_ERR_WRITE_ERROR, "cannot write to `%s'", map[disk->id].device);

  return grub_errno;
}

static void
grub_util_biosdisk_close (struct grub_disk *disk)
{
  struct grub_util_biosdisk_data *data = disk->data;

  free (data->dev);
  if (data->fd != -1)
    close (data->fd);
  free (data);
}

static struct grub_disk_dev grub_util_biosdisk_dev =
  {
    .name = "biosdisk",
    .id = GRUB_DISK_DEVICE_BIOSDISK_ID,
    .iterate = grub_util_biosdisk_iterate,
    .open = grub_util_biosdisk_open,
    .close = grub_util_biosdisk_close,
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
    {
      grub_util_info (_("cannot open `%s'"), dev_map);
      return;
    }

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
	 symbolic links.  Leave /dev/mapper/ alone, though.  */
      if (strncmp (p, "/dev/mapper/", 12) != 0)
	{
	  map[drive].device = xmalloc (PATH_MAX);
	  if (! realpath (p, map[drive].device))
	    grub_util_error ("cannot get the real path of `%s'", p);
	}
      else
#endif
      map[drive].device = xstrdup (p);
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

/*
 * Note: we do not use the new partition naming scheme as dos_part does not
 * necessarily correspond to an msdos partition.
 */
static char *
make_device_name (int drive, int dos_part, int bsd_part)
{
  char *ret;
  char *dos_part_str = NULL;
  char *bsd_part_str = NULL;

  if (dos_part >= 0)
    dos_part_str = xasprintf (",%d", dos_part + 1);

  if (bsd_part >= 0)
    bsd_part_str = xasprintf (",%d", bsd_part + 1);

  ret = xasprintf ("%s%s%s", map[drive].drive,
                   dos_part_str ? : "",
                   bsd_part_str ? : "");

  if (dos_part_str)
    free (dos_part_str);

  if (bsd_part_str)
    free (bsd_part_str);

  return ret;
}

static char *
convert_system_partition_to_system_disk (const char *os_dev, struct stat *st)
{
#if defined(__linux__)
  char *path = xmalloc (PATH_MAX);
  if (! realpath (os_dev, path))
    return NULL;

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

      if (strncmp ("md", p, 2) == 0
	  && p[2] >= '0' && p[2] <= '9')
	{
	  char *ptr = p + 2;
	  while (*ptr >= '0' && *ptr <= '9')
	    ptr++;
	  *ptr = 0;
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
	  char *pp = p + 2;
	  while (*pp >= 'a' && *pp <= 'z')
	    pp++;
	  /* /dev/[hsv]d[a-z]+[0-9]* */
	  *pp = '\0';
	  return path;
	}

      /* If this is a Xen virtual block device.  */
      if ((strncmp ("xvd", p, 3) == 0) && p[3] >= 'a' && p[3] <= 'z')
	{
	  char *pp = p + 3;
	  while (*pp >= 'a' && *pp <= 'z')
	    pp++;
	  /* /dev/xvd[a-z]+[0-9]* */
	  *pp = '\0';
	  return path;
	}

#ifdef HAVE_DEVICE_MAPPER
      /* If this is a DM-RAID device.
         Compare os_dev rather than path here, since nodes under
         /dev/mapper/ are often symlinks.  */
      if ((strncmp ("/dev/mapper/", os_dev, 12) == 0))
	{
	  struct dm_tree *tree;
	  uint32_t maj, min;
	  struct dm_tree_node *node = NULL, *child;
	  void *handle;
	  const char *node_uuid, *mapper_name = NULL, *child_uuid, *child_name;

	  tree = dm_tree_create ();
	  if (! tree)
	    {
	      grub_dprintf ("hostdisk", "dm_tree_create failed\n");
	      goto devmapper_out;
	    }

	  maj = major (st->st_rdev);
	  min = minor (st->st_rdev);
	  if (! dm_tree_add_dev (tree, maj, min))
	    {
	      grub_dprintf ("hostdisk", "dm_tree_add_dev failed\n");
	      goto devmapper_out;
	    }

	  node = dm_tree_find_node (tree, maj, min);
	  if (! node)
	    {
	      grub_dprintf ("hostdisk", "dm_tree_find_node failed\n");
	      goto devmapper_out;
	    }
	  node_uuid = dm_tree_node_get_uuid (node);
	  if (! node_uuid)
	    {
	      grub_dprintf ("hostdisk", "%s has no DM uuid\n", path);
	      node = NULL;
	      goto devmapper_out;
	    }
	  else if (strncmp (node_uuid, "DMRAID-", 7) != 0)
	    {
	      grub_dprintf ("hostdisk", "%s is not DM-RAID\n", path);
	      node = NULL;
	      goto devmapper_out;
	    }

	  handle = NULL;
	  /* Counter-intuitively, device-mapper refers to the disk-like
	     device containing a DM-RAID partition device as a "child" of
	     the partition device.  */
	  child = dm_tree_next_child (&handle, node, 0);
	  if (! child)
	    {
	      grub_dprintf ("hostdisk", "%s has no DM children\n", path);
	      goto devmapper_out;
	    }
	  child_uuid = dm_tree_node_get_uuid (child);
	  if (! child_uuid)
	    {
	      grub_dprintf ("hostdisk", "%s child has no DM uuid\n", path);
	      goto devmapper_out;
	    }
	  else if (strncmp (child_uuid, "DMRAID-", 7) != 0)
	    {
	      grub_dprintf ("hostdisk", "%s child is not DM-RAID\n", path);
	      goto devmapper_out;
	    }
	  child_name = dm_tree_node_get_name (child);
	  if (! child_name)
	    {
	      grub_dprintf ("hostdisk", "%s child has no DM name\n", path);
	      goto devmapper_out;
	    }
	  mapper_name = child_name;

devmapper_out:
	  if (! mapper_name && node)
	    {
	      /* This is a DM-RAID disk, not a partition.  */
	      mapper_name = dm_tree_node_get_name (node);
	      if (! mapper_name)
		grub_dprintf ("hostdisk", "%s has no DM name\n", path);
	    }
	  if (tree)
	    dm_tree_free (tree);
	  free (path);
	  if (mapper_name)
	    return xasprintf ("/dev/mapper/%s", mapper_name);
	  else
	    return NULL;
	}
#endif /* HAVE_DEVICE_MAPPER */
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
            p = strpbrk (p, "sp");
            if (p)
              *p = '\0';
            break;
          }
    }
  return path;

#elif defined(__NetBSD__)
  /* NetBSD uses "/dev/r[a-z]+[0-9][a-z]".  */
  char *path = xstrdup (os_dev);
  if (strncmp ("/dev/r", path, sizeof("/dev/r") - 1) == 0 &&
      (path[sizeof("/dev/r") - 1] >= 'a' && path[sizeof("/dev/r") - 1] <= 'z') &&
      strncmp ("fd", path + sizeof("/dev/r") - 1, sizeof("fd") - 1) != 0)    /* not a floppy device name */
    {
      char *p;
      for (p = path + sizeof("/dev/r"); *p >= 'a' && *p <= 'z'; p++);
      if (grub_isdigit(*p))
	{
	  p++;
	  if ((*p >= 'a' && *p <= 'z') && (*(p+1) == '\0'))
	    {
	      /* path matches the required regular expression and
		 p points to its last character.  */
	      int rawpart = -1;
# ifdef HAVE_GETRAWPARTITION
	      rawpart = getrawpartition();
# endif /* HAVE_GETRAWPARTITION */
	      if (rawpart >= 0)
		*p = 'a' + rawpart;
	    }
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

#if defined(__NetBSD__)
/* Try to determine whether a given device name corresponds to a whole disk.
   This function should give in most cases a definite answer, but it may
   actually give an approximate one in the following sense: if the return
   value is 0 then the device name does not correspond to a whole disk.  */
static int
device_is_wholedisk (const char *os_dev)
{
  int len = strlen (os_dev);
  int rawpart = -1;

# ifdef HAVE_GETRAWPARTITION
  rawpart = getrawpartition();
# endif /* HAVE_GETRAWPARTITION */
  if (rawpart < 0)
    return 1;
  return (os_dev[len - 1] == ('a' + rawpart));
}
#endif /* defined(__NetBSD__) */

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
static int
device_is_wholedisk (const char *os_dev)
{
  const char *p;

  if (strncmp (os_dev, "/dev/", sizeof ("/dev/") - 1) != 0)
    return 0;

  for (p = os_dev + sizeof ("/dev/") - 1; *p; ++p)
    if (grub_isdigit (*p))
      {
	if (strchr (p, 's'))
	  return 0;
	break;
      }

  return 1;
}
#endif /* defined(__FreeBSD__) || defined(__FreeBSD_kernel__) */

static int
find_system_device (const char *os_dev, struct stat *st, int convert, int add)
{
  unsigned int i;
  char *os_disk;

  if (convert)
    os_disk = convert_system_partition_to_system_disk (os_dev, st);
  else
    os_disk = xstrdup (os_dev);
  if (! os_disk)
    return -1;

  for (i = 0; i < ARRAY_SIZE (map); i++)
    if (! map[i].device)
      break;
    else if (strcmp (map[i].device, os_disk) == 0)
      {
	free (os_disk);
	return i;
      }

  if (!add)
    return -1;

  if (i == ARRAY_SIZE (map))
    grub_util_error (_("device count exceeds limit"));

  map[i].device = os_disk;
  map[i].drive = xstrdup (os_disk);

  return i;
}

int
grub_util_biosdisk_is_present (const char *os_dev)
{
  struct stat st;

  if (stat (os_dev, &st) < 0)
    return 0;

  return find_system_device (os_dev, &st, 1, 0) != -1;
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

  drive = find_system_device (os_dev, &st, 1, 1);
  if (drive < 0)
    {
      grub_error (GRUB_ERR_UNKNOWN_DEVICE,
		  "no mapping exists for `%s'", os_dev);
      return 0;
    }

  if (grub_strcmp (os_dev,
		   convert_system_partition_to_system_disk (os_dev, &st)) == 0)
    return make_device_name (drive, -1, -1);

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__APPLE__) || defined(__NetBSD__)
  if (! S_ISCHR (st.st_mode))
#else
  if (! S_ISBLK (st.st_mode))
#endif
    return make_device_name (drive, -1, -1);

#if defined(__linux__) || defined(__CYGWIN__) || defined(HAVE_DIOCGDINFO)
  /* Linux counts partitions uniformly, whether a BSD partition or a DOS
     partition, so mapping them to GRUB devices is not trivial.
     Here, get the start sector of a partition by HDIO_GETGEO, and
     compare it with each partition GRUB recognizes.

     Cygwin /dev/sdXN emulation uses Windows partition mapping. It
     does not count the extended partition and missing primary
     partitions.  Use same method as on Linux here.

     For NetBSD and FreeBSD, proceed as for Linux, except that the start
     sector is obtained from the disk label.  */
  {
    char *name, *partname;
    grub_disk_t disk;
    grub_disk_addr_t start;
    auto int find_partition (grub_disk_t dsk,
			     const grub_partition_t partition);

    int find_partition (grub_disk_t dsk __attribute__ ((unused)),
			const grub_partition_t partition)
      {
	grub_disk_addr_t part_start = 0;
	grub_util_info ("Partition %d starts from %lu",
			partition->number, partition->start);

	part_start = grub_partition_get_start (partition);

	if (start == part_start)
	  {
	    partname = grub_partition_get_name (partition);
	    return 1;
	  }

	return 0;
      }

    name = make_device_name (drive, -1, -1);

# if !defined(HAVE_DIOCGDINFO)
    if (MAJOR (st.st_rdev) == FLOPPY_MAJOR)
      return name;
# else /* defined(HAVE_DIOCGDINFO) */
    /* Since os_dev and convert_system_partition_to_system_disk (os_dev) are
     * different, we know that os_dev cannot be a floppy device.  */
# endif /* !defined(HAVE_DIOCGDINFO) */

    start = find_partition_start (os_dev);
    if (grub_errno != GRUB_ERR_NONE)
      {
	free (name);
	return 0;
      }

    grub_util_info ("%s starts from %lu", os_dev, start);

    if (start == 0 && device_is_wholedisk (os_dev))
      return name;

    grub_util_info ("opening the device %s", name);
    disk = grub_disk_open (name);
    free (name);

    if (! disk)
      {
	/* We already know that the partition exists.  Given that we already
	   checked the device map above, we can only get
	   GRUB_ERR_UNKNOWN_DEVICE at this point if the disk does not exist.
	   This can happen on Xen, where disk images in the host can be
	   assigned to devices that have partition-like names in the guest
	   but are really more like disks.  */
	if (grub_errno == GRUB_ERR_UNKNOWN_DEVICE)
	  {
	    grub_util_warn
	      ("disk does not exist, so falling back to partition device %s",
	       os_dev);

	    drive = find_system_device (os_dev, &st, 0, 1);
	    if (drive < 0)
	      {
		grub_error (GRUB_ERR_UNKNOWN_DEVICE,
			    "no mapping exists for `%s'", os_dev);
		return 0;
	      }

	    return make_device_name (drive, -1, -1);
	  }
	else
	  return 0;
      }

    partname = NULL;
    grub_partition_iterate (disk, find_partition);
    if (grub_errno != GRUB_ERR_NONE)
      {
	grub_disk_close (disk);
	return 0;
      }

    if (partname == NULL)
      {
	grub_disk_close (disk);
	grub_error (GRUB_ERR_BAD_DEVICE,
		    "cannot find the partition of `%s'", os_dev);
	return 0;
      }

    name = grub_xasprintf ("%s,%s", disk->name, partname);
    free (partname);
    return name;
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
	    dos_part = (int) n - 1;

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

const char *
grub_util_biosdisk_get_osdev (grub_disk_t disk)
{
  return map[disk->id].device;
}

int
grub_util_biosdisk_is_floppy (grub_disk_t disk)
{
  struct stat st;
  int fd;

  fd = open (map[disk->id].device, O_RDONLY);
  /* Shouldn't happen.  */
  if (fd == -1)
    return 0;

  /* Shouldn't happen either.  */
  if (fstat (fd, &st) < 0)
    return 0;

#if defined(__NetBSD__)
  if (major(st.st_rdev) == RAW_FLOPPY_MAJOR)
    return 1;
#endif

#if defined(FLOPPY_MAJOR)
  if (major(st.st_rdev) == FLOPPY_MAJOR)
#else
  /* Some kernels (e.g. kFreeBSD) don't have a static major number
     for floppies, but they still use a "fd[0-9]" pathname.  */
  if (map[disk->id].device[5] == 'f'
      && map[disk->id].device[6] == 'd'
      && map[disk->id].device[7] >= '0'
      && map[disk->id].device[7] <= '9')
#endif
    return 1;

  return 0;
}
