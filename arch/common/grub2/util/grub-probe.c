/* grub-probe.c - probe device information for a given path */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2006,2007 Free Software Foundation, Inc.
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
#include <grub/types.h>
#include <grub/util/misc.h>
#include <grub/device.h>
#include <grub/disk.h>
#include <grub/fs.h>
#include <grub/partition.h>
#include <grub/pc_partition.h>
#include <grub/util/biosdisk.h>
#include <grub/util/getroot.h>
#include <grub/term.h>

#include <grub_probe_init.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define _GNU_SOURCE	1
#include <getopt.h>

#define PRINT_FS	0
#define PRINT_DRIVE	1
#define PRINT_DEVICE	2
#define PRINT_PARTMAP	3

int print = PRINT_FS;

void
grub_putchar (int c)
{
  putchar (c);
}

int
grub_getkey (void)
{
  return -1;
}

grub_term_t
grub_term_get_current (void)
{
  return 0;
}

void
grub_refresh (void)
{
}

static void
probe (const char *path)
{
  char *device_name;
  char *drive_name = NULL;
  grub_device_t dev;
  grub_fs_t fs;
  
  device_name = grub_guess_root_device (path);
  if (! device_name)
    grub_util_error ("cannot find a device for %s.\n", path);

  if (print == PRINT_DEVICE)
    {
      printf ("%s\n", device_name);
      goto end;
    }

  drive_name = grub_util_get_grub_dev (device_name);
  if (! drive_name)
    grub_util_error ("cannot find a GRUB drive for %s.\n", device_name);
  
  if (print == PRINT_DRIVE)
    {
      printf ("(%s)\n", drive_name);
      goto end;
    }

  grub_util_info ("opening %s", drive_name);
  dev = grub_device_open (drive_name);
  if (! dev)
    grub_util_error ("%s", grub_errmsg);

  if (print == PRINT_PARTMAP)
    {
      if (dev->disk->partition == NULL)
        grub_util_error ("Cannot detect partition map for %s", drive_name);

      if (strcmp (dev->disk->partition->partmap->name, "amiga_partition_map") == 0)
        printf ("amiga\n");
      else if (strcmp (dev->disk->partition->partmap->name, "apple_partition_map") == 0)
        printf ("apple\n");
      else if (strcmp (dev->disk->partition->partmap->name, "gpt_partition_map") == 0)
        printf ("gpt\n");
      else if (strcmp (dev->disk->partition->partmap->name, "pc_partition_map") == 0)
        printf ("pc\n");
      else if (strcmp (dev->disk->partition->partmap->name, "sun_partition_map") == 0)
        printf ("sun\n");
      else
        grub_util_error ("Unknown partition map %s", dev->disk->partition->partmap->name);
      goto end;
    }

  fs = grub_fs_probe (dev);
  if (! fs)
    grub_util_error ("%s", grub_errmsg);

  printf ("%s\n", fs->name);
  
  grub_device_close (dev);

 end:
  
  free (device_name);
  free (drive_name);
}

static struct option options[] =
  {
    {"device-map", required_argument, 0, 'm'},
    {"target", required_argument, 0, 't'},
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
	     "Try ``grub-probe --help'' for more information.\n");
  else
    printf ("\
Usage: grub-probe [OPTION]... PATH\n\
\n\
Probe device information for a given path.\n\
\n\
  -m, --device-map=FILE     use FILE as the device map [default=%s]\n\
  -t, --target=(fs|drive|device|partmap)\n\
                            print filesystem module, GRUB drive, system device or partition map module [default=fs]\n\
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
  
  progname = "grub-probe";
  
  /* Check for options.  */
  while (1)
    {
      int c = getopt_long (argc, argv, "m:t:hVv", options, 0);
      
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

	  case 't':
	    if (!strcmp (optarg, "fs"))
	      print = PRINT_FS;
	    else if (!strcmp (optarg, "drive"))
	      print = PRINT_DRIVE;
	    else if (!strcmp (optarg, "device"))
	      print = PRINT_DEVICE;
	    else if (!strcmp (optarg, "partmap"))
	      print = PRINT_PARTMAP;
	    else
	      usage (1);
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
  
  /* Initialize all modules. */
  grub_init_all ();

  /* Do it.  */
  probe (path);
  
  /* Free resources.  */
  grub_fini_all ();
  grub_util_biosdisk_fini ();
  
  free (dev_map);
  
  return 0;
}
