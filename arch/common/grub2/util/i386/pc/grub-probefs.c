/* grub-probefs.c - probe a filesystem module for a given path */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005 Free Software Foundation, Inc.
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
#include <grub/types.h>
#include <grub/util/misc.h>
#include <grub/device.h>
#include <grub/disk.h>
#include <grub/fs.h>
#include <grub/partition.h>
#include <grub/pc_partition.h>
#include <grub/machine/util/biosdisk.h>
#include <grub/util/getroot.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define _GNU_SOURCE	1
#include <getopt.h>

#ifdef __NetBSD__
/* NetBSD uses /boot for its boot block.  */
# define DEFAULT_DIRECTORY	"/grub"
#else
# define DEFAULT_DIRECTORY	"/boot/grub"
#endif

#define DEFAULT_DEVICE_MAP	DEFAULT_DIRECTORY "/device.map"

void
grub_putchar (int c)
{
  putchar (c);
}

void
grub_refresh (void)
{
}

static void
probe (const char *path)
{
  char *device_name;
  grub_device_t dev;
  grub_fs_t fs;
  
  device_name = grub_guess_root_device (path);
  if (! device_name)
    {
      fprintf (stderr, "cannot find a GRUB device for %s.\n", path);
      return;
    }

  grub_util_info ("opening %s", device_name);
  dev = grub_device_open (device_name);
  if (! dev)
    grub_util_error ("%s", grub_errmsg);

  fs = grub_fs_probe (dev);
  if (! fs)
    grub_util_error ("%s", grub_errmsg);

  printf ("%s\n", fs->name);
  
  grub_device_close (dev);
  free (device_name);
}

static struct option options[] =
  {
    {"device-map", required_argument, 0, 'm'},
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
	     "Try ``grub-probefs --help'' for more information.\n");
  else
    printf ("\
Usage: grub-probefs [OPTION]... PATH\n\
\n\
Probe a filesystem module for a given path.\n\
\n\
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
  char *path;
  
  progname = "grub-probefs";
  
  /* Check for options.  */
  while (1)
    {
      int c = getopt_long (argc, argv, "m:hVv", options, 0);
      
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

  /* Obtain PATH.  */
  if (optind >= argc)
    {
      fprintf (stderr, "No path is specified.\n");
      usage (1);
    }

  if (optind + 1 != argc)
    {
      fprintf (stderr, "Unknown extra argument `%s'.\n", argv[optind + 1]);
      usage (1);
    }

  path = argv[optind];
  
  /* Initialize the emulated biosdisk driver.  */
  grub_util_biosdisk_init (dev_map ? : DEFAULT_DEVICE_MAP);
  grub_pc_partition_map_init ();

  /* Initialize filesystems.  */
  grub_fat_init ();
  grub_ext2_init ();
  grub_ufs_init ();
  grub_minix_init ();
  grub_hfs_init ();
  grub_jfs_init ();

  /* Do it.  */
  probe (path);
  
  /* Free resources.  */
  grub_ext2_fini ();
  grub_fat_fini ();
  grub_ufs_fini ();
  grub_minix_fini ();
  grub_hfs_fini ();
  grub_jfs_fini ();
  
  grub_pc_partition_map_fini ();
  grub_util_biosdisk_fini ();
  
  free (dev_map);
  
  return 0;
}
