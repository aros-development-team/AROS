/* grub-mkdevicemap.c - make a device map file automatically */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004,2005 Free Software Foundation, Inc.
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

#include <grub/util/misc.h>

#define _GNU_SOURCE	1
#include <getopt.h>

#ifdef __NetBSD__
/* NetBSD uses /boot for its boot block.  */
# define DEFAULT_DIRECTORY	"/grub"
#else
# define DEFAULT_DIRECTORY	"/boot/grub"
#endif

#define DEFAULT_DEVICE_MAP	DEFAULT_DIRECTORY "/device.map"

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
# ifndef CDROM_GET_CAPABILITY
#  define CDROM_GET_CAPABILITY	0x5331	/* get capabilities */
# endif /* ! CDROM_GET_CAPABILITY */
# ifndef BLKGETSIZE
#  define BLKGETSIZE	_IO(0x12,96)	/* return device size */
# endif /* ! BLKGETSIZE */
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
get_kfreebsd_version ()
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
#else
# warning "BIOS SCSI drives cannot be guessed in your operating system."
  /* Set NAME to a bogus string.  */
  *name = 0;
#endif
}

#ifdef __linux__
static void
get_dac960_disk_name (char *name, int controller, int drive)
{
  sprintf (name, "/dev/rd/c%dd%d", controller, drive);
}

static void
get_ataraid_disk_name (char *name, int unit)
{
  sprintf (name, "/dev/ataraid/d%c", unit + '0');
}
#endif

/* Check if DEVICE can be read. If an error occurs, return zero,
   otherwise return non-zero.  */
int
check_device (const char *device)
{
  char buf[512];
  FILE *fp;

  /* If DEVICE is empty, just return 1.  */
  if (*device == 0)
    return 1;
  
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
      return 0;
    }
  
  /* Make sure CD-ROMs don't get assigned a BIOS disk number 
     before SCSI disks!  */
#ifdef __linux__
# ifdef CDROM_GET_CAPABILITY
  if (ioctl (fileno (fp), CDROM_GET_CAPABILITY, 0) >= 0)
    return 0;
# else /* ! CDROM_GET_CAPABILITY */
  /* Check if DEVICE is a CD-ROM drive by the HDIO_GETGEO ioctl.  */
  {
    struct hd_geometry hdg;
    struct stat st;

    if (fstat (fileno (fp), &st))
      return 0;

    /* If it is a block device and isn't a floppy, check if HDIO_GETGEO
       succeeds.  */
    if (S_ISBLK (st.st_mode)
	&& MAJOR (st.st_rdev) != FLOPPY_MAJOR
	&& ioctl (fileno (fp), HDIO_GETGEO, &hdg))
      return 0;
  }
# endif /* ! CDROM_GET_CAPABILITY */
#endif /* __linux__ */

#if defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__OpenBSD__)
# ifdef CDIOCCLRDEBUG
  if (ioctl (fileno (fp), CDIOCCLRDEBUG, 0) >= 0)
    return 0;
# endif /* CDIOCCLRDEBUG */
#endif /* __FreeBSD_kernel__ || __NetBSD__ || __OpenBSD__ */
  
  /* Attempt to read the first sector.  */
  if (fread (buf, 1, 512, fp) != 512)
    {
      fclose (fp);
      return 0;
    }
  
  fclose (fp);
  return 1;
}

static void
make_device_map (const char *device_map, int floppy_disks)
{
  FILE *fp;
  int num_hd = 0;
  int i;

  if (strcmp (device_map, "-") == 0)
    fp = stdout;
  else
    fp = fopen (device_map, "w");
  
  if (! fp)
    grub_util_error ("cannot open %s", device_map);

  /* Floppies.  */
  for (i = 0; i < floppy_disks; i++)
    {
      char name[16];
      
      get_floppy_disk_name (name, i);
      /* In floppies, write the map, whether check_device succeeds
	 or not, because the user just may not insert floppies.  */
      if (fp)
	fprintf (fp, "(fd%d)\t%s\n", i, name);
    }
  
#ifdef __linux__
  if (have_devfs ())
    {
      while (1)
	{
	  char discn[32];
	  char name[PATH_MAX];
	  struct stat st;

	  /* Linux creates symlinks "/dev/discs/discN" for convenience.
	     The way to number disks is the same as GRUB's.  */
	  sprintf (discn, "/dev/discs/disc%d", num_hd);
	  if (stat (discn, &st) < 0)
	    break;
	  
	  if (realpath (discn, name))
	    {
	      strcat (name, "/disc");
	      fprintf (fp, "(hd%d)\t%s\n", num_hd, name);
	    }
	  
	  num_hd++;
	}
      
      goto finish;
    }
#endif /* __linux__ */
    
  /* IDE disks.  */
  for (i = 0; i < 8; i++)
    {
      char name[16];
      
      get_ide_disk_name (name, i);
      if (check_device (name))
	{
	  fprintf (fp, "(hd%d)\t%s\n", num_hd, name);
	  num_hd++;
	}
    }
  
#ifdef __linux__
  /* ATARAID disks.  */
  for (i = 0; i < 8; i++)
    {
      char name[20];

      get_ataraid_disk_name (name, i);
      if (check_device (name))
	{
	  fprintf (fp, "(hd%d)\t%s\n", num_hd, name);
          num_hd++;
        }
    }
#endif /* __linux__ */

  /* The rest is SCSI disks.  */
  for (i = 0; i < 16; i++)
    {
      char name[16];
      
      get_scsi_disk_name (name, i);
      if (check_device (name))
	{
	  fprintf (fp, "(hd%d)\t%s\n", num_hd, name);
	  num_hd++;
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
	    if (check_device (name))
	      {
		fprintf (fp, "(hd%d)\t%s\n", num_hd, name);
		num_hd++;
	      }
	  }
      }
  }
#endif /* __linux__ */

 finish:
  if (fp != stdout)
    fclose (fp);
}

static struct option options[] =
  {
    {"device-map", required_argument, 0, 'm'},
    {"probe-second-floppy", no_argument, 0, 's'},
    {"no-floppy", no_argument, 0, 'n'},
    {"help", no_argument, 0, 'h'},
    {"version", no_argument, 0, 'V'},
    {"verbose", no_argument, 0, 'v'},
    {0, 0, 0, 0}
  };

static void
usage (int status)
{
  if (status)
    fprintf (stderr,
	     "Try ``grub-mkdevicemap --help'' for more information.\n");
  else
    printf ("\
Usage: grub-mkdevicemap [OPTION]...\n\
\n\
Generate a device map file automatically.\n\
\n\
  -n, --no-floppy           do not probe any floppy drive\n\
  -s, --probe-second-floppy probe the second floppy drive\n\
  -m, --device-map=FILE     use FILE as the device map [default=%s]\n\
  -h, --help                display this message and exit\n\
  -V, --version             print version information and exit\n\
  -v, --verbose             print verbose messages\n\
\n\
Report bugs to <%s>.\n\
",
	    DEFAULT_DEVICE_MAP, PACKAGE_BUGREPORT);
  
  exit (status);
}

int
main (int argc, char *argv[])
{
  char *dev_map = 0;
  int floppy_disks = 1;
  
  progname = "grub-mkdevicemap";
  
  /* Check for options.  */
  while (1)
    {
      int c = getopt_long (argc, argv, "snm:r:hVv", options, 0);
      
      if (c == -1)
	break;
      else
	switch (c)
	  {
	  case 'm':
	    if (dev_map)
	      free (dev_map);

	    dev_map = xstrdup (optarg);
	    break;

	  case 'n':
	    floppy_disks = 0;
	    break;

	  case 's':
	    floppy_disks = 2;
	    break;
	    
	  case 'h':
	    usage (0);
	    break;

	  case 'V':
	    printf ("%s (%s) %s\n", progname, PACKAGE_NAME, PACKAGE_VERSION);
	    return 0;

	  case 'v':
	    verbosity++;
	    break;

	  default:
	    usage (1);
	    break;
	  }
    }

  make_device_map (dev_map ? : DEFAULT_DEVICE_MAP, floppy_disks);

  free (dev_map);
  
  return 0;
}
