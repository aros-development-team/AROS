/* deviceiter.c - iterate over system devices */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004,2005,2007,2008 Free Software Foundation, Inc.
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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <dirent.h>

#include <grub/util/misc.h>
#include <grub/util/deviceiter.h>
#include <grub/list.h>
#include <grub/misc.h>
#include <grub/emu/misc.h>

#ifdef __linux__
# if !defined(__GLIBC__) || \
        ((__GLIBC__ < 2) || ((__GLIBC__ == 2) && (__GLIBC_MINOR__ < 1)))
/* Maybe libc doesn't have large file support.  */
#  include <linux/unistd.h>     /* _llseek */
# endif /* (GLIBC < 2) || ((__GLIBC__ == 2) && (__GLIBC_MINOR < 1)) */
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
# ifndef FLOPPY_MAJOR
#  define FLOPPY_MAJOR	2	/* the major number for floppy */
# endif /* ! FLOPPY_MAJOR */
# ifndef MAJOR
#  define MAJOR(dev)	\
  ({ \
     unsigned long long __dev = (dev); \
     (unsigned) ((__dev >> 8) & 0xfff) \
                 | ((unsigned int) (__dev >> 32) & ~0xfff); \
  })
# endif /* ! MAJOR */
# ifndef MINOR
#  define MINOR(dev)	\
  ({ \
     unsigned long long __dev = (dev); \
     (unsigned) (__dev & 0xff) | ((unsigned int) (__dev >> 12) & ~0xff); \
  })
# endif /* ! MINOR */
# ifndef CDROM_GET_CAPABILITY
#  define CDROM_GET_CAPABILITY	0x5331	/* get capabilities */
# endif /* ! CDROM_GET_CAPABILITY */
# ifndef BLKGETSIZE
#  define BLKGETSIZE	_IO(0x12,96)	/* return device size */
# endif /* ! BLKGETSIZE */

#ifdef HAVE_DEVICE_MAPPER
# include <libdevmapper.h>
#endif
#endif /* __linux__ */

/* Use __FreeBSD_kernel__ instead of __FreeBSD__ for compatibility with
   kFreeBSD-based non-FreeBSD systems (e.g. GNU/kFreeBSD) */
#if defined(__FreeBSD__) && ! defined(__FreeBSD_kernel__)
# define __FreeBSD_kernel__
#endif
#ifdef __FreeBSD_kernel__
  /* Obtain version of kFreeBSD headers */
# include <osreldate.h>
# ifndef __FreeBSD_kernel_version
#  define __FreeBSD_kernel_version __FreeBSD_version
# endif

  /* Runtime detection of kernel */
# include <sys/utsname.h>
int
get_kfreebsd_version (void)
{
  struct utsname uts;
  int major;
  int minor;
  int v[2];

  uname (&uts);
  sscanf (uts.release, "%d.%d", &major, &minor);

  if (major >= 9)
    major = 9;
  if (major >= 5)
    {
      v[0] = minor/10; v[1] = minor%10;
    }
  else
    {
      v[0] = minor%10; v[1] = minor/10;
    }
  return major*100000+v[0]*10000+v[1]*1000;
}
#endif /* __FreeBSD_kernel__ */

#if defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__OpenBSD__)
# include <sys/ioctl.h>		/* ioctl */
# include <sys/disklabel.h>
# include <sys/cdio.h>		/* CDIOCCLRDEBUG */
# if defined(__FreeBSD_kernel__)
#  include <sys/param.h>
#  if __FreeBSD_kernel_version >= 500040
#   include <sys/disk.h>
#  endif
# endif /* __FreeBSD_kernel__ */
#endif /* __FreeBSD_kernel__ || __NetBSD__ || __OpenBSD__ */

#ifdef HAVE_OPENDISK
# include <util.h>
#endif /* HAVE_OPENDISK */

#ifdef __linux__
/* Check if we have devfs support.  */
static int
have_devfs (void)
{
  struct stat st;
  return stat ("/dev/.devfsd", &st) == 0;
}
#endif /* __linux__ */

/* These three functions are quite different among OSes.  */
static void
get_floppy_disk_name (char *name, int unit)
{
#if defined(__linux__)
  /* GNU/Linux */
  if (have_devfs ())
    sprintf (name, "/dev/floppy/%d", unit);
  else
    sprintf (name, "/dev/fd%d", unit);
#elif defined(__GNU__)
  /* GNU/Hurd */
  sprintf (name, "/dev/fd%d", unit);
#elif defined(__FreeBSD_kernel__)
  /* kFreeBSD */
  if (get_kfreebsd_version () >= 400000)
    sprintf (name, "/dev/fd%d", unit);
  else
    sprintf (name, "/dev/rfd%d", unit);
#elif defined(__NetBSD__)
  /* NetBSD */
  /* opendisk() doesn't work for floppies.  */
  sprintf (name, "/dev/rfd%da", unit);
#elif defined(__OpenBSD__)
  /* OpenBSD */
  sprintf (name, "/dev/rfd%dc", unit);
#elif defined(__QNXNTO__)
  /* QNX RTP */
  sprintf (name, "/dev/fd%d", unit);
#elif defined(__CYGWIN__)
  /* Cygwin */
  sprintf (name, "/dev/fd%d", unit);
#elif defined(__MINGW32__)
  (void) unit;
  *name = 0;
#else
# warning "BIOS floppy drives cannot be guessed in your operating system."
  /* Set NAME to a bogus string.  */
  *name = 0;
#endif
}

static void
get_ide_disk_name (char *name, int unit)
{
#if defined(__linux__)
  /* GNU/Linux */
  sprintf (name, "/dev/hd%c", unit + 'a');
#elif defined(__GNU__)
  /* GNU/Hurd */
  sprintf (name, "/dev/hd%d", unit);
#elif defined(__FreeBSD_kernel__)
  /* kFreeBSD */
  if (get_kfreebsd_version () >= 400000)
    sprintf (name, "/dev/ad%d", unit);
  else
    sprintf (name, "/dev/rwd%d", unit);
#elif defined(__NetBSD__) && defined(HAVE_OPENDISK)
  /* NetBSD */
  char shortname[16];
  int fd;

  sprintf (shortname, "wd%d", unit);
  fd = opendisk (shortname, O_RDONLY, name,
		 16,	/* length of NAME */
		 0	/* char device */
		 );
  close (fd);
#elif defined(__OpenBSD__)
  /* OpenBSD */
  sprintf (name, "/dev/rwd%dc", unit);
#elif defined(__QNXNTO__)
  /* QNX RTP */
  /* Actually, QNX RTP doesn't distinguish IDE from SCSI, so this could
     contain SCSI disks.  */
  sprintf (name, "/dev/hd%d", unit);
#elif defined(__CYGWIN__)
  /* Cygwin emulates all disks as /dev/sdX.  */
  (void) unit;
  *name = 0;
#elif defined(__MINGW32__)
  sprintf (name, "//./PHYSICALDRIVE%d", unit);
#else
# warning "BIOS IDE drives cannot be guessed in your operating system."
  /* Set NAME to a bogus string.  */
  *name = 0;
#endif
}

static void
get_scsi_disk_name (char *name, int unit)
{
#if defined(__linux__)
  /* GNU/Linux */
  sprintf (name, "/dev/sd%c", unit + 'a');
#elif defined(__GNU__)
  /* GNU/Hurd */
  sprintf (name, "/dev/sd%d", unit);
#elif defined(__FreeBSD_kernel__)
  /* kFreeBSD */
  if (get_kfreebsd_version () >= 400000)
    sprintf (name, "/dev/da%d", unit);
  else
    sprintf (name, "/dev/rda%d", unit);
#elif defined(__NetBSD__) && defined(HAVE_OPENDISK)
  /* NetBSD */
  char shortname[16];
  int fd;

  sprintf (shortname, "sd%d", unit);
  fd = opendisk (shortname, O_RDONLY, name,
		 16,	/* length of NAME */
		 0	/* char device */
		 );
  close (fd);
#elif defined(__OpenBSD__)
  /* OpenBSD */
  sprintf (name, "/dev/rsd%dc", unit);
#elif defined(__QNXNTO__)
  /* QNX RTP */
  /* QNX RTP doesn't distinguish SCSI from IDE, so it is better to
     disable the detection of SCSI disks here.  */
  *name = 0;
#elif defined(__CYGWIN__)
  /* Cygwin emulates all disks as /dev/sdX.  */
  sprintf (name, "/dev/sd%c", unit + 'a');
#elif defined(__MINGW32__)
  (void) unit;
  *name = 0;
#else
# warning "BIOS SCSI drives cannot be guessed in your operating system."
  /* Set NAME to a bogus string.  */
  *name = 0;
#endif
}

#ifdef __linux__
static void
get_virtio_disk_name (char *name, int unit)
{
#ifdef __sparc__
  sprintf (name, "/dev/vdisk%c", unit + 'a');
#else
  sprintf (name, "/dev/vd%c", unit + 'a');
#endif
}

static void
get_dac960_disk_name (char *name, int controller, int drive)
{
  sprintf (name, "/dev/rd/c%dd%d", controller, drive);
}

static void
get_acceleraid_disk_name (char *name, int controller, int drive)
{
  sprintf (name, "/dev/rs/c%dd%d", controller, drive);
}

static void
get_ataraid_disk_name (char *name, int unit)
{
  sprintf (name, "/dev/ataraid/d%c", unit + '0');
}

static void
get_i2o_disk_name (char *name, char unit)
{
  sprintf (name, "/dev/i2o/hd%c", unit);
}

static void
get_cciss_disk_name (char *name, int controller, int drive)
{
  sprintf (name, "/dev/cciss/c%dd%d", controller, drive);
}

static void
get_ida_disk_name (char *name, int controller, int drive)
{
  sprintf (name, "/dev/ida/c%dd%d", controller, drive);
}

static void
get_mmc_disk_name (char *name, int unit)
{
  sprintf (name, "/dev/mmcblk%d", unit);
}

static void
get_xvd_disk_name (char *name, int unit)
{
  sprintf (name, "/dev/xvd%c", unit + 'a');
}
#endif

static struct seen_device
{
  struct seen_device *next;
  const char *name;
} *seen;

/* Check if DEVICE can be read.  Skip any DEVICE that we have already seen.
   If an error occurs, return zero, otherwise return non-zero.  */
static int
check_device_readable_unique (const char *device)
{
  char *real_device;
  char buf[512];
  FILE *fp;
  struct seen_device *seen_elt;

  /* If DEVICE is empty, just return error.  */
  if (*device == 0)
    return 0;

  /* Have we seen this device already?  */
  real_device = canonicalize_file_name (device);
  if (! real_device)
    return 0;
  if (grub_named_list_find (GRUB_AS_NAMED_LIST (seen), real_device))
    {
      grub_dprintf ("deviceiter", "Already seen %s (%s)\n",
		    device, real_device);
      goto fail;
    }

  fp = fopen (device, "r");
  if (! fp)
    {
      switch (errno)
	{
#ifdef ENOMEDIUM
	case ENOMEDIUM:
# if 0
	  /* At the moment, this finds only CDROMs, which can't be
	     read anyway, so leave it out. Code should be
	     reactivated if `removable disks' and CDROMs are
	     supported.  */
	  /* Accept it, it may be inserted.  */
	  return 1;
# endif
	  break;
#endif /* ENOMEDIUM */
	default:
	  /* Break case and leave.  */
	  break;
	}
      /* Error opening the device.  */
      goto fail;
    }

  /* Make sure CD-ROMs don't get assigned a BIOS disk number
     before SCSI disks!  */
#ifdef __linux__
# ifdef CDROM_GET_CAPABILITY
  if (ioctl (fileno (fp), CDROM_GET_CAPABILITY, 0) >= 0)
    goto fail;
# else /* ! CDROM_GET_CAPABILITY */
  /* Check if DEVICE is a CD-ROM drive by the HDIO_GETGEO ioctl.  */
  {
    struct hd_geometry hdg;
    struct stat st;

    if (fstat (fileno (fp), &st))
      goto fail;

    /* If it is a block device and isn't a floppy, check if HDIO_GETGEO
       succeeds.  */
    if (S_ISBLK (st.st_mode)
	&& MAJOR (st.st_rdev) != FLOPPY_MAJOR
	&& ioctl (fileno (fp), HDIO_GETGEO, &hdg))
      goto fail;
  }
# endif /* ! CDROM_GET_CAPABILITY */
#endif /* __linux__ */

#if defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__OpenBSD__)
# ifdef CDIOCCLRDEBUG
  if (ioctl (fileno (fp), CDIOCCLRDEBUG, 0) >= 0)
    goto fail;
# endif /* CDIOCCLRDEBUG */
#endif /* __FreeBSD_kernel__ || __NetBSD__ || __OpenBSD__ */

  /* Attempt to read the first sector.  */
  if (fread (buf, 1, 512, fp) != 512)
    {
      fclose (fp);
      goto fail;
    }

  /* Remember that we've seen this device.  */
  seen_elt = xmalloc (sizeof (*seen_elt));
  seen_elt->name = real_device; /* steal memory */
  grub_list_push (GRUB_AS_LIST_P (&seen), GRUB_AS_LIST (seen_elt));

  fclose (fp);
  return 1;

fail:
  free (real_device);
  return 0;
}

static void
clear_seen_devices (void)
{
  while (seen)
    {
      struct seen_device *seen_elt = seen;
      seen = seen->next;
      free (seen_elt);
    }
  seen = NULL;
}

#ifdef __linux__
struct device
{
	char *stable;
	char *kernel;
};

/* Sort by the kernel name for preference since that most closely matches
   older device.map files, but sort by stable by-id names as a fallback.
   This is because /dev/disk/by-id/ usually has a few alternative
   identifications of devices (e.g. ATA vs. SATA).
   check_device_readable_unique will ensure that we only get one for any
   given disk, but sort the list so that the choice of which one we get is
   stable.  */
static int
compare_devices (const void *a, const void *b)
{
  const struct device *left = (const struct device *) a;
  const struct device *right = (const struct device *) b;

  if (left->kernel && right->kernel)
    {
      int ret = strcmp (left->kernel, right->kernel);
      if (ret)
	return ret;
    }

  return strcmp (left->stable, right->stable);
}
#endif /* __linux__ */

void
grub_util_iterate_devices (int NESTED_FUNC_ATTR (*hook) (const char *, int),
			   int floppy_disks)
{
  int i;

  clear_seen_devices ();

  /* Floppies.  */
  for (i = 0; i < floppy_disks; i++)
    {
      char name[16];
      struct stat st;

      get_floppy_disk_name (name, i);
      if (stat (name, &st) < 0)
	break;
      /* In floppies, write the map, whether check_device_readable_unique
         succeeds or not, because the user just may not insert floppies.  */
      if (hook (name, 1))
	goto out;
    }

#ifdef __linux__
  {
    DIR *dir = opendir ("/dev/disk/by-id");

    if (dir)
      {
	struct dirent *entry;
	struct device *devs;
	size_t devs_len = 0, devs_max = 1024, i;

	devs = xmalloc (devs_max * sizeof (*devs));

	/* Dump all the directory entries into names, resizing if
	   necessary.  */
	for (entry = readdir (dir); entry; entry = readdir (dir))
	  {
	    /* Skip current and parent directory entries.  */
	    if (strcmp (entry->d_name, ".") == 0 ||
		strcmp (entry->d_name, "..") == 0)
	      continue;
	    /* Skip partition entries.  */
	    if (strstr (entry->d_name, "-part"))
	      continue;
	    /* Skip device-mapper entries; we'll handle the ones we want
	       later.  */
	    if (strncmp (entry->d_name, "dm-", sizeof ("dm-") - 1) == 0)
	      continue;
	    /* Skip RAID entries; they are handled by upper layers.  */
	    if (strncmp (entry->d_name, "md-", sizeof ("md-") - 1) == 0)
	      continue;
	    if (devs_len >= devs_max)
	      {
		devs_max *= 2;
		devs = xrealloc (devs, devs_max * sizeof (*devs));
	      }
	    devs[devs_len].stable =
	      xasprintf ("/dev/disk/by-id/%s", entry->d_name);
	    devs[devs_len].kernel =
	      canonicalize_file_name (devs[devs_len].stable);
	    devs_len++;
	  }

	qsort (devs, devs_len, sizeof (*devs), &compare_devices);

	closedir (dir);

	/* Now add all the devices in sorted order.  */
	for (i = 0; i < devs_len; ++i)
	  {
	    if (check_device_readable_unique (devs[i].stable))
	      {
		if (hook (devs[i].stable, 0))
		  goto out;
	      }
	    free (devs[i].stable);
	    free (devs[i].kernel);
	  }
	free (devs);
      }
  }

  if (have_devfs ())
    {
      i = 0;
      while (1)
	{
	  char discn[32];
	  char name[PATH_MAX];
	  struct stat st;

	  /* Linux creates symlinks "/dev/discs/discN" for convenience.
	     The way to number disks is the same as GRUB's.  */
	  sprintf (discn, "/dev/discs/disc%d", i++);
	  if (stat (discn, &st) < 0)
	    break;

	  if (realpath (discn, name))
	    {
	      strcat (name, "/disc");
	      if (hook (name, 0))
		goto out;
	    }
	}
      goto out;
    }
#endif /* __linux__ */

  /* IDE disks.  */
  for (i = 0; i < 96; i++)
    {
      char name[16];

      get_ide_disk_name (name, i);
      if (check_device_readable_unique (name))
	{
	  if (hook (name, 0))
	    goto out;
	}
    }

#ifdef __linux__
  /* Virtio disks.  */
  for (i = 0; i < 26; i++)
    {
      char name[16];

      get_virtio_disk_name (name, i);
      if (check_device_readable_unique (name))
	{
	  if (hook (name, 0))
	    goto out;
	}
    }

  /* ATARAID disks.  */
  for (i = 0; i < 8; i++)
    {
      char name[20];

      get_ataraid_disk_name (name, i);
      if (check_device_readable_unique (name))
	{
	  if (hook (name, 0))
	    goto out;
        }
    }

  /* Xen virtual block devices.  */
  for (i = 0; i < 26; i++)
    {
      char name[16];

      get_xvd_disk_name (name, i);
      if (check_device_readable_unique (name))
	{
	  if (hook (name, 0))
	    goto out;
	}
    }
#endif /* __linux__ */

  /* The rest is SCSI disks.  */
  for (i = 0; i < 48; i++)
    {
      char name[16];

      get_scsi_disk_name (name, i);
      if (check_device_readable_unique (name))
	{
	  if (hook (name, 0))
	    goto out;
	}
    }

#ifdef __linux__
  /* This is for DAC960 - we have
     /dev/rd/c<controller>d<logical drive>p<partition>.

     DAC960 driver currently supports up to 8 controllers, 32 logical
     drives, and 7 partitions.  */
  {
    int controller, drive;

    for (controller = 0; controller < 8; controller++)
      {
	for (drive = 0; drive < 15; drive++)
	  {
	    char name[24];

	    get_dac960_disk_name (name, controller, drive);
	    if (check_device_readable_unique (name))
	      {
		if (hook (name, 0))
		  goto out;
	      }
	  }
      }
  }

  /* This is for Mylex Acceleraid - we have
     /dev/rd/c<controller>d<logical drive>p<partition>.  */
  {
    int controller, drive;

    for (controller = 0; controller < 8; controller++)
      {
	for (drive = 0; drive < 15; drive++)
	  {
	    char name[24];

	    get_acceleraid_disk_name (name, controller, drive);
	    if (check_device_readable_unique (name))
	      {
		if (hook (name, 0))
		  goto out;
	      }
	  }
      }
  }

  /* This is for CCISS - we have
     /dev/cciss/c<controller>d<logical drive>p<partition>.  */
  {
    int controller, drive;

    for (controller = 0; controller < 3; controller++)
      {
	for (drive = 0; drive < 16; drive++)
	  {
	    char name[24];

	    get_cciss_disk_name (name, controller, drive);
	    if (check_device_readable_unique (name))
	      {
		if (hook (name, 0))
		  goto out;
	      }
	  }
      }
  }

  /* This is for Compaq Intelligent Drive Array - we have
     /dev/ida/c<controller>d<logical drive>p<partition>.  */
  {
    int controller, drive;

    for (controller = 0; controller < 3; controller++)
      {
	for (drive = 0; drive < 16; drive++)
	  {
	    char name[24];

	    get_ida_disk_name (name, controller, drive);
	    if (check_device_readable_unique (name))
	      {
		if (hook (name, 0))
		  goto out;
	      }
	  }
      }
  }

  /* This is for I2O - we have /dev/i2o/hd<logical drive><partition> */
  {
    char unit;

    for (unit = 'a'; unit < 'f'; unit++)
      {
	char name[24];

	get_i2o_disk_name (name, unit);
	if (check_device_readable_unique (name))
	  {
	    if (hook (name, 0))
	      goto out;
	  }
      }
  }

  /* MultiMediaCard (MMC).  */
  for (i = 0; i < 10; i++)
    {
      char name[16];

      get_mmc_disk_name (name, i);
      if (check_device_readable_unique (name))
	{
	  if (hook (name, 0))
	    goto out;
	}
    }

# ifdef HAVE_DEVICE_MAPPER
#  define dmraid_check(cond, ...) \
  if (! (cond)) \
    { \
      grub_dprintf ("deviceiter", __VA_ARGS__); \
      goto dmraid_end; \
    }

  /* DM-RAID.  */
  if (grub_device_mapper_supported ())
    {
      struct dm_tree *tree = NULL;
      struct dm_task *task = NULL;
      struct dm_names *names = NULL;
      unsigned int next = 0;
      void *top_handle, *second_handle;
      struct dm_tree_node *root, *top, *second;

      /* Build DM tree for all devices.  */
      tree = dm_tree_create ();
      dmraid_check (tree, "dm_tree_create failed\n");
      task = dm_task_create (DM_DEVICE_LIST);
      dmraid_check (task, "dm_task_create failed\n");
      dmraid_check (dm_task_run (task), "dm_task_run failed\n");
      names = dm_task_get_names (task);
      dmraid_check (names, "dm_task_get_names failed\n");
      dmraid_check (names->dev, "No DM devices found\n");
      do
	{
	  names = (void *) names + next;
	  dmraid_check (dm_tree_add_dev (tree, MAJOR (names->dev),
					 MINOR (names->dev)),
			   "dm_tree_add_dev (%s) failed\n", names->name);
	  next = names->next;
	}
      while (next);

      /* Walk the second-level children of the inverted tree; that is, devices
	 which are directly composed of non-DM devices such as hard disks.
	 This class includes all DM-RAID disks and excludes all DM-RAID
	 partitions.  */
      root = dm_tree_find_node (tree, 0, 0);
      top_handle = NULL;
      top = dm_tree_next_child (&top_handle, root, 1);
      while (top)
	{
	  second_handle = NULL;
	  second = dm_tree_next_child (&second_handle, top, 1);
	  while (second)
	    {
	      const char *node_name, *node_uuid;
	      char *name;

	      node_name = dm_tree_node_get_name (second);
	      dmraid_check (node_name, "dm_tree_node_get_name failed\n");
	      node_uuid = dm_tree_node_get_uuid (second);
	      dmraid_check (node_uuid, "dm_tree_node_get_uuid failed\n");
	      if (strncmp (node_uuid, "DMRAID-", 7) != 0)
		{
		  grub_dprintf ("deviceiter", "%s is not DM-RAID\n", node_name);
		  goto dmraid_next_child;
		}

	      name = xasprintf ("/dev/mapper/%s", node_name);
	      if (check_device_readable_unique (name))
		{
		  if (hook (name, 0))
		    {
		      free (name);
		      if (task)
			dm_task_destroy (task);
		      if (tree)
			dm_tree_free (tree);
		      goto out;
		    }
		}
	      free (name);

dmraid_next_child:
	      second = dm_tree_next_child (&second_handle, top, 1);
	    }
	  top = dm_tree_next_child (&top_handle, root, 1);
	}

dmraid_end:
      if (task)
	dm_task_destroy (task);
      if (tree)
	dm_tree_free (tree);
    }
# endif /* HAVE_DEVICE_MAPPER */
#endif /* __linux__ */

out:
  clear_seen_devices ();
}

