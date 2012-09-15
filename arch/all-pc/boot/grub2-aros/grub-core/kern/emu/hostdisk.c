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
#include <grub/emu/getroot.h>
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
# include <sys/mount.h>
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
#endif /* __linux__ */

#ifdef __CYGWIN__
# include <sys/ioctl.h>
# include <cygwin/fs.h> /* BLKGETSIZE64 */
# include <cygwin/hdreg.h> /* HDIO_GETGEO */
#endif

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
# include <sys/disk.h> /* DIOCGMEDIASIZE */
# include <sys/param.h>
# include <sys/sysctl.h>
# include <sys/mount.h>
#include <libgeom.h>
#endif

#if defined (__sun__)
# include <sys/dkio.h>
#endif

#if defined(__APPLE__)
# include <sys/disk.h>
#endif

#ifdef HAVE_DEVICE_MAPPER
# include <libdevmapper.h>
#endif

#if defined(__NetBSD__)
# define HAVE_DIOCGDINFO
# include <sys/ioctl.h>
# include <sys/disklabel.h>    /* struct disklabel */
# include <sys/disk.h>    /* struct dkwedge_info */
#else /* !defined(__NetBSD__) && !defined(__FreeBSD__) && !defined(__FreeBSD_kernel__) */
# undef HAVE_DIOCGDINFO
#endif /* defined(__NetBSD__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) */

#if defined(__NetBSD__)
# ifdef HAVE_GETRAWPARTITION
#  include <util.h>    /* getrawpartition */
# endif /* HAVE_GETRAWPARTITION */
# include <sys/fdio.h>
# ifndef RAW_FLOPPY_MAJOR
#  define RAW_FLOPPY_MAJOR	9
# endif /* ! RAW_FLOPPY_MAJOR */
#endif /* defined(__NetBSD__) */

static struct
{
  char *drive;
  char *device;
  int device_map;
} map[256];

struct grub_util_biosdisk_data
{
  char *dev;
  int access_mode;
  int fd;
  int is_disk;
  int device_map;
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
unescape_cmp (const char *a, const char *b_escaped)
{
  while (*a || *b_escaped)
    {
      if (*b_escaped == '\\' && b_escaped[1] != 0)
	b_escaped++;
      if (*a < *b_escaped)
	return -1;
      if (*a > *b_escaped)
	return +1;
      a++;
      b_escaped++;
    }
  if (*a)
    return +1;
  if (*b_escaped)
    return -1;
  return 0;
}

static int
find_grub_drive (const char *name)
{
  unsigned int i;

  if (name)
    {
      for (i = 0; i < ARRAY_SIZE (map); i++)
	if (map[i].drive && unescape_cmp (map[i].drive, name) == 0)
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
grub_util_biosdisk_iterate (int (*hook) (const char *name),
			    grub_disk_pull_t pull)
{
  unsigned i;

  if (pull != GRUB_DISK_PULL_NONE)
    return 0;

  for (i = 0; i < sizeof (map) / sizeof (map[0]); i++)
    if (map[i].drive && hook (map[i].drive))
      return 1;

  return 0;
}

#if !defined(__MINGW32__)
grub_uint64_t
grub_util_get_fd_size (int fd, const char *name, unsigned *log_secsize)
{
#if !defined (__GNU__)
# if defined(__NetBSD__)
  struct disklabel label;
# elif defined (__sun__)
  struct dk_minfo minfo;
# else
  unsigned long long nr;
# endif
#endif
  unsigned sector_size, log_sector_size;
  struct stat st;

  if (fstat (fd, &st) < 0)
    /* TRANSLATORS: "stat" comes from the name of POSIX function.  */
    grub_util_error (_("cannot stat `%s': %s"), name, strerror (errno));

#if defined(__linux__) || defined(__CYGWIN__) || defined(__FreeBSD__) || \
  defined(__FreeBSD_kernel__) || defined(__APPLE__) || defined(__NetBSD__) \
  || defined (__sun__)

# if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__APPLE__) || defined(__NetBSD__) || defined (__sun__)
  if (! S_ISCHR (st.st_mode))
# else
  if (! S_ISBLK (st.st_mode))
# endif
    goto fail;

# if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
    if (ioctl (fd, DIOCGMEDIASIZE, &nr))
# elif defined(__APPLE__)
    if (ioctl (fd, DKIOCGETBLOCKCOUNT, &nr))
# elif defined(__NetBSD__)
    configure_device_driver (fd);
    if (ioctl (fd, DIOCGDINFO, &label) == -1)
# elif defined (__sun__)
    if (!ioctl (fd, DKIOCGMEDIAINFO, &minfo))
# else
    if (ioctl (fd, BLKGETSIZE64, &nr))
# endif
      goto fail;

# if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
    if (ioctl (fd, DIOCGSECTORSIZE, &sector_size))
      goto fail;
# elif defined(__APPLE__)
    if (ioctl (fd, DKIOCGETBLOCKSIZE, &sector_size))
      goto fail;
# elif defined(__sun__)
    sector_size = minfo.dki_lbsize;
# elif defined(__NetBSD__)
    sector_size = label.d_secsize;
# else
    if (ioctl (fd, BLKSSZGET, &sector_size))
      goto fail;
# endif
    if (sector_size & (sector_size - 1) || !sector_size)
      goto fail;
    for (log_sector_size = 0;
	 (1 << log_sector_size) < sector_size;
	 log_sector_size++);

    if (log_secsize)
      *log_secsize = log_sector_size;

# if defined (__APPLE__)
    return nr << log_sector_size;
# elif defined(__NetBSD__)
    return (grub_uint64_t) label.d_secperunit << log_sector_size;
# elif defined (__sun__)
    return minfo.dki_capacity << log_sector_size;
# else
    if (nr & ((1 << log_sector_size) - 1))
      grub_util_error ("%s", _("unaligned device size"));

    return nr;
# endif

 fail:

  /* In GNU/Hurd, stat() will return the right size.  */
#elif !defined (__GNU__)
# warning "No special routine to get the size of a block device is implemented for your OS. This is not possibly fatal."
#endif

  sector_size = 512;
  log_sector_size = 9;

  if (log_secsize)
   *log_secsize = 9;

  return st.st_size;
}
#endif

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
  data->is_disk = 0;
  data->device_map = map[drive].device_map;

  /* Get the size.  */
#if defined(__MINGW32__)
  {
    grub_uint64_t size;

    size = grub_util_get_disk_size (map[drive].device);

    if (size % 512)
      grub_util_error (_("unaligned device size"));

    disk->total_sectors = size >> 9;

    grub_util_info ("the size of %s is %llu", name, disk->total_sectors);

    return GRUB_ERR_NONE;
  }
#else
  {
    int fd;

    fd = open (map[drive].device, O_RDONLY);
    if (fd == -1)
      return grub_error (GRUB_ERR_UNKNOWN_DEVICE, N_("cannot open `%s': %s"),
			 map[drive].device, strerror (errno));

    disk->total_sectors = grub_util_get_fd_size (fd, map[drive].device,
						 &disk->log_sector_size);
    disk->total_sectors >>= disk->log_sector_size;

# if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__APPLE__) || defined(__NetBSD__)
    if (fstat (fd, &st) < 0 || ! S_ISCHR (st.st_mode))
# else
    if (fstat (fd, &st) < 0 || ! S_ISBLK (st.st_mode))
# endif
      data->is_disk = 1;

    close (fd);

    grub_util_info ("the size of %s is %" PRIuGRUB_UINT64_T,
		    name, disk->total_sectors);

    return GRUB_ERR_NONE;
  }
#endif
}

int
grub_util_device_is_mapped (const char *dev)
{
#ifdef HAVE_DEVICE_MAPPER
  struct stat st;

  if (!grub_device_mapper_supported ())
    return 0;

  if (stat (dev, &st) < 0)
    return 0;

  return dm_is_dm_major (major (st.st_rdev));
#else
  return 0;
#endif /* HAVE_DEVICE_MAPPER */
}

#ifdef HAVE_DEVICE_MAPPER
int
grub_util_get_dm_node_linear_info (const char *dev,
				   int *maj, int *min,
				   grub_disk_addr_t *st)
{
  struct dm_task *dmt;
  void *next = NULL;
  uint64_t length, start;
  char *target, *params;
  char *ptr;
  int major, minor;
  int first = 1;
  grub_disk_addr_t partstart = 0;

  while (1)
    {
      dmt = dm_task_create(DM_DEVICE_TABLE);
      if (!dmt)
	break;
      
      if (! (first ? dm_task_set_name (dmt, dev)
	     : dm_task_set_major_minor (dmt, major, minor, 0)))
	{
	  dm_task_destroy (dmt);
	  break;
	}
      dm_task_no_open_count(dmt);
      if (!dm_task_run(dmt))
	{
	  dm_task_destroy (dmt);
	  break;
	}
      next = dm_get_next_target(dmt, next, &start, &length,
				&target, &params);
      if (grub_strcmp (target, "linear") != 0)
	{
	  dm_task_destroy (dmt);
	  break;
	}
      major = grub_strtoul (params, &ptr, 10);
      if (grub_errno)
	{
	  dm_task_destroy (dmt);
	  grub_errno = GRUB_ERR_NONE;
	  return 0;
	}
      if (*ptr != ':')
	{
	  dm_task_destroy (dmt);
	  return 0;
	}
      ptr++;
      minor = grub_strtoul (ptr, &ptr, 10);
      if (grub_errno)
	{
	  grub_errno = GRUB_ERR_NONE;
	  dm_task_destroy (dmt);
	  return 0;
	}

      if (*ptr != ' ')
	{
	  dm_task_destroy (dmt);
	  return 0;
	}
      ptr++;
      partstart += grub_strtoull (ptr, &ptr, 10);
      if (grub_errno)
	{
	  grub_errno = GRUB_ERR_NONE;
	  dm_task_destroy (dmt);
	  return 0;
	}

      dm_task_destroy (dmt);
      first = 0;
    }
  if (first)
    return 0;
  if (maj)
    *maj = major;
  if (min)
    *min = minor;
  if (st)
    *st = partstart;
  return 1;
}
#endif

#if defined (__FreeBSD__) || defined(__FreeBSD_kernel__)

/* FIXME: geom actually gives us the whole container hierarchy.
   It can be used more efficiently than this.  */
void
grub_util_follow_gpart_up (const char *name, grub_disk_addr_t *off_out, char **name_out)
{
  struct gmesh mesh;
  struct gclass *class;
  int error;
  struct ggeom *geom;

  grub_util_info ("following geom '%s'", name);

  error = geom_gettree (&mesh);
  if (error != 0)
    /* TRANSLATORS: geom is the name of (k)FreeBSD device framework.
       Usually left untranslated.
     */
    grub_util_error ("%s", _("couldn't open geom"));

  LIST_FOREACH (class, &mesh.lg_class, lg_class)
    if (strcasecmp (class->lg_name, "part") == 0)
      break;
  if (!class)
    /* TRANSLATORS: geom is the name of (k)FreeBSD device framework.
       Usually left untranslated. "part" is the identifier of one of its
       classes.  */
    grub_util_error ("%s", _("couldn't find geom `part' class"));

  LIST_FOREACH (geom, &class->lg_geom, lg_geom)
    { 
      struct gprovider *provider;
      LIST_FOREACH (provider, &geom->lg_provider, lg_provider)
	if (strcmp (provider->lg_name, name) == 0)
	  {
	    char *name_tmp = xstrdup (geom->lg_name);
	    grub_disk_addr_t off = 0;
	    struct gconfig *config;
	    grub_util_info ("geom '%s' has parent '%s'", name, geom->lg_name);

	    grub_util_follow_gpart_up (name_tmp, &off, name_out);
	    free (name_tmp);
	    LIST_FOREACH (config, &provider->lg_config, lg_config)
	      if (strcasecmp (config->lg_name, "start") == 0)
		off += strtoull (config->lg_val, 0, 10);
	    if (off_out)
	      *off_out = off;
	    return;
	  }
    }
  grub_util_info ("geom '%s' has no parent", name);
  if (name_out)
    *name_out = xstrdup (name);
  if (off_out)
    *off_out = 0;
}

grub_disk_addr_t
grub_hostdisk_find_partition_start (const char *dev)
{
  grub_disk_addr_t out;
  if (strncmp (dev, "/dev/", sizeof ("/dev/") - 1) != 0)
    return 0;
  grub_util_follow_gpart_up (dev + sizeof ("/dev/") - 1, &out, NULL);

  return out;
}

#elif defined(__linux__) || defined(__CYGWIN__) || defined(HAVE_DIOCGDINFO) || defined (__sun__)
grub_disk_addr_t
grub_hostdisk_find_partition_start (const char *dev)
{
  int fd;
#ifdef __sun__
  struct extpart_info pinfo;
# elif !defined(HAVE_DIOCGDINFO)
  struct hd_geometry hdg;
# else /* defined(HAVE_DIOCGDINFO) */
#  if defined(__NetBSD__)
  struct dkwedge_info dkw;
#  endif /* defined(__NetBSD__) */
  struct disklabel label;
  int p_index;
# endif /* !defined(HAVE_DIOCGDINFO) */

# ifdef HAVE_DEVICE_MAPPER
  grub_disk_addr_t partition_start;
  if (grub_util_device_is_mapped (dev)
      && grub_util_get_dm_node_linear_info (dev, 0, 0, &partition_start))
    return partition_start;
# endif /* HAVE_DEVICE_MAPPER */

  fd = open (dev, O_RDONLY);
  if (fd == -1)
    {
      grub_error (GRUB_ERR_BAD_DEVICE, N_("cannot open `%s': %s"),
		  dev, strerror (errno));
      return 0;
    }

#if defined(__sun__)
  if (ioctl (fd, DKIOCEXTPARTINFO, &pinfo))
# elif !defined(HAVE_DIOCGDINFO)
  if (ioctl (fd, HDIO_GETGEO, &hdg))
# else /* defined(HAVE_DIOCGDINFO) */
#  if defined(__NetBSD__)
  configure_device_driver (fd);
  /* First handle the case of disk wedges.  */
  if (ioctl (fd, DIOCGWEDGEINFO, &dkw) == 0)
    {
      close (fd);
      return (grub_disk_addr_t) dkw.dkw_offset;
    }
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

#ifdef __sun__
  return pinfo.p_start;
# elif !defined(HAVE_DIOCGDINFO)
  return hdg.start;
# else /* defined(HAVE_DIOCGDINFO) */
  if (dev[0])
    p_index = dev[strlen(dev) - 1] - 'a';
  else
    p_index = -1;
  
  if (p_index >= label.d_npartitions || p_index < 0)
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
  struct linux_partition_cache **prev;
  char *dev;
  unsigned long start;
  int partno;
};

struct linux_partition_cache *linux_partition_cache_list;

static int
linux_find_partition (char *dev, grub_disk_addr_t sector)
{
  size_t len = strlen (dev);
  const char *format;
  char *p;
  int i;
  char real_dev[PATH_MAX];
  struct linux_partition_cache *cache;
  int missing = 0;

  strcpy(real_dev, dev);

  if (have_devfs () && strcmp (real_dev + len - 5, "/disc") == 0)
    {
      p = real_dev + len - 4;
      format = "part%d";
    }
  else if (strncmp (real_dev, "/dev/disk/by-id/",
		    sizeof ("/dev/disk/by-id/") - 1) == 0)
    {
      p = real_dev + len;
      format = "-part%d";
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
	{
	  if (missing++ < 10)
	    continue;
	  else
	    return 0;
	}
      missing = 0;
      close (fd);

      start = grub_hostdisk_find_partition_start (real_dev);
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

#if defined(__linux__) && (!defined(__GLIBC__) || \
        ((__GLIBC__ < 2) || ((__GLIBC__ == 2) && (__GLIBC_MINOR__ < 1))))
  /* Maybe libc doesn't have large file support.  */
grub_err_t
grub_util_fd_seek (int fd, const char *name, grub_uint64_t off)
{
  loff_t offset, result;
  static int _llseek (uint filedes, ulong hi, ulong lo,
		      loff_t *res, uint wh);
  _syscall5 (int, _llseek, uint, filedes, ulong, hi, ulong, lo,
	     loff_t *, res, uint, wh);

  offset = (loff_t) off;
  if (_llseek (fd, offset >> 32, offset & 0xffffffff, &result, SEEK_SET))
    return grub_error (GRUB_ERR_BAD_DEVICE, N_("cannot seek `%s': %s"),
		       name, strerror (errno));
  return GRUB_ERR_NONE;
}
#else
grub_err_t
grub_util_fd_seek (int fd, const char *name, grub_uint64_t off)
{
  off_t offset = (off_t) off;

  if (lseek (fd, offset, SEEK_SET) != offset)
    return grub_error (GRUB_ERR_BAD_DEVICE, N_("cannot seek `%s': %s"),
		       name, strerror (errno));
  return 0;
}
#endif

static void
flush_initial_buffer (const char *os_dev __attribute__ ((unused)))
{
#ifdef __linux__
  int fd;
  struct stat st;

  fd = open (os_dev, O_RDONLY);
  if (fd >= 0 && fstat (fd, &st) >= 0 && S_ISBLK (st.st_mode))
    ioctl (fd, BLKFLSBUF, 0);
  if (fd >= 0)
    close (fd);
#endif
}

const char *
grub_hostdisk_os_dev_to_grub_drive (const char *os_disk, int add)
{
  unsigned int i;

  for (i = 0; i < ARRAY_SIZE (map); i++)
    if (! map[i].device)
      break;
    else if (strcmp (map[i].device, os_disk) == 0)
      return map[i].drive;

  if (!add)
    return NULL;

  if (i == ARRAY_SIZE (map))
    /* TRANSLATORS: it refers to the lack of free slots.  */
    grub_util_error ("%s", _("device count exceeds limit"));

  map[i].device = xstrdup (os_disk);
  map[i].drive = xmalloc (sizeof ("hostdisk/") + strlen (os_disk));
  strcpy (map[i].drive, "hostdisk/");
  strcpy (map[i].drive + sizeof ("hostdisk/") - 1, os_disk);
  map[i].device_map = 0;

  flush_initial_buffer (os_disk);

  return map[i].drive;
}

static int
open_device (const grub_disk_t disk, grub_disk_addr_t sector, int flags,
	     grub_disk_addr_t *max)
{
  int fd;
  struct grub_util_biosdisk_data *data = disk->data;

  *max = ~0ULL;

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
    if (disk->partition
	&& strncmp (map[disk->id].device, "/dev/", 5) == 0)
      {
	if (sector >= part_start)
	  is_partition = linux_find_partition (dev, part_start);
	else
	  *max = part_start - sector;
      }

  reopen:

    if (data->dev && strcmp (data->dev, dev) == 0 &&
	data->access_mode == (flags & O_ACCMODE))
      {
	grub_dprintf ("hostdisk", "reusing open device `%s'\n", dev);
	fd = data->fd;
      }
    else
      {
	free (data->dev);
	data->dev = 0;
	if (data->fd != -1)
	  {
	    if (data->access_mode == O_RDWR || data->access_mode == O_WRONLY)
	      {
		fsync (data->fd);
#ifdef __linux__
		if (data->is_disk)
		  ioctl (data->fd, BLKFLSBUF, 0);
#endif
	      }

	    close (data->fd);
	    data->fd = -1;
	  }

	/* Open the partition.  */
	grub_dprintf ("hostdisk", "opening the device `%s' in open_device()\n", dev);
	fd = open (dev, flags);
	if (fd < 0)
	  {
	    grub_error (GRUB_ERR_BAD_DEVICE, N_("cannot open `%s': %s"),
			dev, strerror (errno));
	    return -1;
	  }

	data->dev = xstrdup (dev);
	data->access_mode = (flags & O_ACCMODE);
	data->fd = fd;

#ifdef __linux__
	if (data->is_disk)
	  ioctl (data->fd, BLKFLSBUF, 0);
#endif
      }

    if (is_partition)
      {
	*max = grub_util_get_fd_size (fd, dev, 0);
	*max >>= disk->log_sector_size;
	if (sector - part_start >= *max)
	  {
	    *max = disk->partition->len - (sector - part_start);
	    if (*max == 0)
	      *max = ~0ULL;
	    is_partition = 0;
	    strcpy (dev, map[disk->id].device);
	    goto reopen;
	  }
	sector -= part_start;
	*max -= sector;
      }
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
      data->dev = 0;
      if (data->fd != -1)
	{
	    if (data->access_mode == O_RDWR || data->access_mode == O_WRONLY)
	      {
		fsync (data->fd);
#ifdef __linux__
		if (data->is_disk)
		  ioctl (data->fd, BLKFLSBUF, 0);
#endif
	      }
	    close (data->fd);
	    data->fd = -1;
	}

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
      grub_error (GRUB_ERR_BAD_DEVICE, N_("cannot open `%s': %s"),
		  map[disk->id].device, strerror (errno));
      return -1;
    }
#endif /* ! __linux__ */

#if defined(__NetBSD__)
  configure_device_driver (fd);
#endif /* defined(__NetBSD__) */

  if (grub_util_fd_seek (fd, map[disk->id].device,
			 sector << disk->log_sector_size))
    {
      close (fd);
      return -1;
    }

  return fd;
}

/* Read LEN bytes from FD in BUF. Return less than or equal to zero if an
   error occurs, otherwise return LEN.  */
ssize_t
grub_util_fd_read (int fd, char *buf, size_t len)
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
ssize_t
grub_util_fd_write (int fd, const char *buf, size_t len)
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
  while (size)
    {
      int fd;
      grub_disk_addr_t max = ~0ULL;
      fd = open_device (disk, sector, O_RDONLY, &max);
      if (fd < 0)
	return grub_errno;

#ifdef __linux__
      if (sector == 0)
	/* Work around a bug in Linux ez remapping.  Linux remaps all
	   sectors that are read together with the MBR in one read.  It
	   should only remap the MBR, so we split the read in two
	   parts. -jochen  */
	max = 1;
#endif /* __linux__ */

      if (max > size)
	max = size;

      if (grub_util_fd_read (fd, buf, max << disk->log_sector_size)
	  != (ssize_t) (max << disk->log_sector_size))
	return grub_error (GRUB_ERR_READ_ERROR, N_("cannot read `%s': %s"),
			   map[disk->id].device, strerror (errno));
      size -= max;
      buf += (max << disk->log_sector_size);
      sector += max;
    }
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_util_biosdisk_write (grub_disk_t disk, grub_disk_addr_t sector,
			  grub_size_t size, const char *buf)
{
  while (size)
    {
      int fd;
      grub_disk_addr_t max = ~0ULL;
      fd = open_device (disk, sector, O_WRONLY, &max);
      if (fd < 0)
	return grub_errno;

#ifdef __linux__
      if (sector == 0)
	/* Work around a bug in Linux ez remapping.  Linux remaps all
	   sectors that are write together with the MBR in one write.  It
	   should only remap the MBR, so we split the write in two
	   parts. -jochen  */
	max = 1;
#endif /* __linux__ */

      if (max > size)
	max = size;

      if (grub_util_fd_write (fd, buf, max << disk->log_sector_size)
	  != (ssize_t) (max << disk->log_sector_size))
	return grub_error (GRUB_ERR_WRITE_ERROR, N_("cannot write to `%s': %s"),
			   map[disk->id].device, strerror (errno));
      size -= max;
      buf += (max << disk->log_sector_size);
    }
  return GRUB_ERR_NONE;
}

grub_err_t
grub_util_biosdisk_flush (struct grub_disk *disk)
{
  struct grub_util_biosdisk_data *data = disk->data;

  if (disk->dev->id != GRUB_DISK_DEVICE_BIOSDISK_ID)
    return GRUB_ERR_NONE;
  if (data->fd == -1)
    {
      grub_disk_addr_t max;
      data->fd = open_device (disk, 0, O_RDONLY, &max);
      if (data->fd < 0)
	return grub_errno;
    }
  fsync (data->fd);
#ifdef __linux__
  if (data->is_disk)
    ioctl (data->fd, BLKFLSBUF, 0);
#endif
  return GRUB_ERR_NONE;
}

static void
grub_util_biosdisk_close (struct grub_disk *disk)
{
  struct grub_util_biosdisk_data *data = disk->data;

  free (data->dev);
  if (data->fd != -1)
    {
      if (data->access_mode == O_RDWR || data->access_mode == O_WRONLY)
	grub_util_biosdisk_flush (disk);
      close (data->fd);
    }
  free (data);
}

static struct grub_disk_dev grub_util_biosdisk_dev =
  {
    .name = "hostdisk",
    .id = GRUB_DISK_DEVICE_HOSTDISK_ID,
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

  auto void show_error (const char *msg)
    __attribute__ ((noreturn));
  void __attribute__ ((noreturn)) show_error (const char *msg)
    {
      grub_util_error ("%s:%d: %s", dev_map, lineno, msg);
    }

  if (dev_map[0] == '\0')
    {
      grub_util_info ("no device.map");
      return;
    }

  fp = fopen (dev_map, "r");
  if (! fp)
    {
      grub_util_info (_("cannot open `%s': %s"), dev_map, strerror (errno));
      return;
    }

  while (fgets (buf, sizeof (buf), fp))
    {
      char *p = buf;
      char *e;
      char *drive_e, *drive_p;
      int drive;

      lineno++;

      /* Skip leading spaces.  */
      while (*p && grub_isspace (*p))
	p++;

      /* If the first character is `#' or NUL, skip this line.  */
      if (*p == '\0' || *p == '#')
	continue;

      if (*p != '(')
	{
	  char *tmp;
	  tmp = xasprintf (_("missing `%c' symbol"), '(');
	  show_error (tmp);
	}

      p++;
      /* Find a free slot.  */
      drive = find_free_slot ();
      if (drive < 0)
	show_error (_("device count exceeds limit"));

      e = p;
      p = strchr (p, ')');
      if (! p)
	{
	  char *tmp;
	  tmp = xasprintf (_("missing `%c' symbol"), ')');
	  show_error (tmp);
	}

      map[drive].drive = 0;
      if ((e[0] == 'f' || e[0] == 'h' || e[0] == 'c') && e[1] == 'd')
	{
	  char *ptr;
	  for (ptr = e + 2; ptr < p; ptr++)
	    if (!grub_isdigit (*ptr))
	      break;
	  if (ptr == p)
	    {
	      map[drive].drive = xmalloc (p - e + sizeof ('\0'));
	      strncpy (map[drive].drive, e, p - e + sizeof ('\0'));
	      map[drive].drive[p - e] = '\0';
	    }
	  if (*ptr == ',')
	    {
	      *p = 0;

	      /* TRANSLATORS: Only one entry is ignored. However the suggestion
		 is to correct/delete the whole file.
		 device.map is a file indicating which
		 devices are available at boot time. Fedora populated it with
		 entries like (hd0,1) /dev/sda1 which would mean that every
		 partition is a separate disk for BIOS. Such entries were
		 inactive in GRUB due to its bug which is now gone. Without
		 this additional check these entries would be harmful now.
	      */
	      grub_util_warn (_("the device.map entry `%s' is invalid. "
				"Ignoring it. Please correct or "
				"delete your device.map"), e);
	      continue;
	    }
	}
      drive_e = e;
      drive_p = p;
      map[drive].device_map = 1;

      p++;
      /* Skip leading spaces.  */
      while (*p && grub_isspace (*p))
	p++;

      if (*p == '\0')
	show_error (_("filename expected"));

      /* NUL-terminate the filename.  */
      e = p;
      while (*e && ! grub_isspace (*e))
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
	    grub_util_error (_("failed to get canonical path of %s"), p);
	}
      else
#endif
      map[drive].device = xstrdup (p);
      if (!map[drive].drive)
	{
	  char c;
	  map[drive].drive = xmalloc (sizeof ("hostdisk/") + strlen (p));
	  memcpy (map[drive].drive, "hostdisk/", sizeof ("hostdisk/") - 1);
	  strcpy (map[drive].drive + sizeof ("hostdisk/") - 1, p);
	  c = *drive_p;
	  *drive_p = 0;
	  /* TRANSLATORS: device.map is a filename. Not to be translated.
	     device.map specifies disk correspondance overrides. Previously
	     one could create any kind of device name with this. Due to
	     some problems we decided to limit it to just a handful
	     possibilities.  */
	  grub_util_warn (_("the drive name `%s' in device.map is incorrect. "
			    "Using %s instead. "
			    "Please use the form [hfc]d[0-9]* "
			    "(E.g. `hd0' or `cd')"),
			  drive_e, map[drive].drive);
	  *drive_p = c;
	}

      flush_initial_buffer (map[drive].device);
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

const char *
grub_util_biosdisk_get_compatibility_hint (grub_disk_t disk)
{
  if (disk->dev != &grub_util_biosdisk_dev || map[disk->id].device_map)
    return disk->name;
  return 0;
}

const char *
grub_util_biosdisk_get_osdev (grub_disk_t disk)
{
  if (disk->dev != &grub_util_biosdisk_dev)
    return 0;

  return map[disk->id].device;
}
