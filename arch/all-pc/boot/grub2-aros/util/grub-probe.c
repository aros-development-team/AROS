/* grub-probe.c - probe device information for a given path */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2006,2007,2008,2009,2010  Free Software Foundation, Inc.
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
#include <grub/emu/misc.h>
#include <grub/util/misc.h>
#include <grub/util/misc.h>
#include <grub/device.h>
#include <grub/disk.h>
#include <grub/file.h>
#include <grub/fs.h>
#include <grub/partition.h>
#include <grub/msdos_partition.h>
#include <grub/emu/hostdisk.h>
#include <grub/emu/getroot.h>
#include <grub/term.h>
#include <grub/env.h>
#include <grub/raid.h>
#include <grub/i18n.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#define _GNU_SOURCE	1
#include <getopt.h>

#include "progname.h"

enum {
  PRINT_FS,
  PRINT_FS_UUID,
  PRINT_FS_LABEL,
  PRINT_DRIVE,
  PRINT_DEVICE,
  PRINT_PARTMAP,
  PRINT_ABSTRACTION,
};

int print = PRINT_FS;
static unsigned int argument_is_device = 0;

static void
probe_partmap (grub_disk_t disk)
{
  grub_partition_t part;

  if (disk->partition == NULL)
    {
      grub_util_info ("no partition map found for %s", disk->name);
      return;
    }

  for (part = disk->partition; part; part = part->parent)
    printf ("%s\n", part->partmap->name);
}

static int
probe_raid_level (grub_disk_t disk)
{
  /* disk might be NULL in the case of a LVM physical volume with no LVM
     signature.  Ignore such cases here.  */
  if (!disk)
    return -1;

  if (disk->dev->id != GRUB_DISK_DEVICE_RAID_ID)
    return -1;

  return ((struct grub_raid_array *) disk->data)->level;
}

static void
probe (const char *path, char *device_name)
{
  char *drive_name = NULL;
  char *grub_path = NULL;
  char *filebuf_via_grub = NULL, *filebuf_via_sys = NULL;
  grub_device_t dev = NULL;
  grub_fs_t fs;

  if (path == NULL)
    {
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__)
      if (! grub_util_check_char_device (device_name))
        grub_util_error ("%s is not a character device", device_name);
#else
      if (! grub_util_check_block_device (device_name))
        grub_util_error ("%s is not a block device", device_name);
#endif
    }
  else
    device_name = grub_guess_root_device (path);

  if (! device_name)
    grub_util_error ("cannot find a device for %s (is /dev mounted?)", path);

  if (print == PRINT_DEVICE)
    {
      printf ("%s\n", device_name);
      goto end;
    }

  drive_name = grub_util_get_grub_dev (device_name);
  if (! drive_name)
    grub_util_error ("cannot find a GRUB drive for %s.  Check your device.map", device_name);

  if (print == PRINT_DRIVE)
    {
      printf ("(%s)\n", drive_name);
      goto end;
    }

  grub_util_info ("opening %s", drive_name);
  dev = grub_device_open (drive_name);
  if (! dev)
    grub_util_error ("%s", grub_errmsg);

  if (print == PRINT_ABSTRACTION)
    {
      grub_disk_memberlist_t list = NULL, tmp;
      const int is_lvm = (dev->disk->dev->id == GRUB_DISK_DEVICE_LVM_ID);
      int is_raid = 0;
      int is_raid5 = 0;
      int is_raid6 = 0;
      int raid_level;
      grub_disk_t raid_disk;

      raid_level = probe_raid_level (dev->disk);
      if (raid_level >= 0)
	{
	  is_raid = 1;
	  is_raid5 |= (raid_level == 5);
	  is_raid6 |= (raid_level == 6);
	  raid_disk = dev->disk;
	}

      if ((is_lvm) && (dev->disk->dev->memberlist))
	list = dev->disk->dev->memberlist (dev->disk);
      while (list)
	{
	  raid_level = probe_raid_level (list->disk);
	  if (raid_level >= 0)
	    {
	      is_raid = 1;
	      is_raid5 |= (raid_level == 5);
	      is_raid6 |= (raid_level == 6);
	      raid_disk = list->disk;
	    }

	  tmp = list->next;
	  free (list);
	  list = tmp;
	}

      if (is_raid)
	{
	  printf ("raid ");
	  if (is_raid5)
	    printf ("raid5rec ");
	  if (is_raid6)
	    printf ("raid6rec ");
	  if (raid_disk->dev->raidname)
	    printf ("%s ", raid_disk->dev->raidname (raid_disk));
	}

      if (is_lvm)
	printf ("lvm ");

      printf ("\n");

      goto end;
    }

  if (print == PRINT_PARTMAP)
    {
      grub_disk_memberlist_t list = NULL, tmp;

      /* Check if dev->disk itself is contained in a partmap.  */
      probe_partmap (dev->disk);

      /* In case of LVM/RAID, check the member devices as well.  */
      if (dev->disk->dev->memberlist)
	list = dev->disk->dev->memberlist (dev->disk);
      while (list)
	{
	  probe_partmap (list->disk);
	  /* LVM on RAID  */
	  if (list->disk->dev->memberlist)
	    {
	      grub_disk_memberlist_t sub_list;

	      sub_list = list->disk->dev->memberlist (list->disk);
	      while (sub_list)
		{
		  probe_partmap (sub_list->disk);
		  tmp = sub_list->next;
		  free (sub_list);
		  sub_list = tmp;
		}
	    }
	  tmp = list->next;
	  free (list);
	  list = tmp;
	}
      goto end;
    }

  fs = grub_fs_probe (dev);
  if (! fs)
    grub_util_error ("%s", grub_errmsg);

  if (print == PRINT_FS)
    {
      printf ("%s\n", fs->name);
    }
  else if (print == PRINT_FS_UUID)
    {
      char *uuid;
      if (! fs->uuid)
	grub_util_error ("%s does not support UUIDs", fs->name);

      if (fs->uuid (dev, &uuid) != GRUB_ERR_NONE)
	grub_util_error ("%s", grub_errmsg);

      printf ("%s\n", uuid);
    }
  else if (print == PRINT_FS_LABEL)
    {
      char *label;
      if (! fs->label)
	grub_util_error ("%s does not support labels", fs->name);

      if (fs->label (dev, &label) != GRUB_ERR_NONE)
	grub_util_error ("%s", grub_errmsg);

      printf ("%s\n", label);
    }

 end:
  if (dev)
    grub_device_close (dev);
  free (grub_path);
  free (filebuf_via_grub);
  free (filebuf_via_sys);
  free (drive_name);
}

static struct option options[] =
  {
    {"device", no_argument, 0, 'd'},
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
	     "Try `%s --help' for more information.\n", program_name);
  else
    printf ("\
Usage: %s [OPTION]... [PATH|DEVICE]\n\
\n\
Probe device information for a given path (or device, if the -d option is given).\n\
\n\
  -d, --device              given argument is a system device, not a path\n\
  -m, --device-map=FILE     use FILE as the device map [default=%s]\n\
  -t, --target=(fs|fs_uuid|fs_label|drive|device|partmap|abstraction)\n\
                            print filesystem module, GRUB drive, system device, partition map module or abstraction module [default=fs]\n\
  -h, --help                display this message and exit\n\
  -V, --version             print version information and exit\n\
  -v, --verbose             print verbose messages\n\
\n\
Report bugs to <%s>.\n\
", program_name,
	    DEFAULT_DEVICE_MAP, PACKAGE_BUGREPORT);

  exit (status);
}

int
main (int argc, char *argv[])
{
  char *dev_map = 0;
  char *argument;

  set_program_name (argv[0]);

  grub_util_init_nls ();

  /* Check for options.  */
  while (1)
    {
      int c = getopt_long (argc, argv, "dm:t:hVv", options, 0);

      if (c == -1)
	break;
      else
	switch (c)
	  {
	  case 'd':
	    argument_is_device = 1;
	    break;

	  case 'm':
	    if (dev_map)
	      free (dev_map);

	    dev_map = xstrdup (optarg);
	    break;

	  case 't':
	    if (!strcmp (optarg, "fs"))
	      print = PRINT_FS;
	    else if (!strcmp (optarg, "fs_uuid"))
	      print = PRINT_FS_UUID;
	    else if (!strcmp (optarg, "fs_label"))
	      print = PRINT_FS_LABEL;
	    else if (!strcmp (optarg, "drive"))
	      print = PRINT_DRIVE;
	    else if (!strcmp (optarg, "device"))
	      print = PRINT_DEVICE;
	    else if (!strcmp (optarg, "partmap"))
	      print = PRINT_PARTMAP;
	    else if (!strcmp (optarg, "abstraction"))
	      print = PRINT_ABSTRACTION;
	    else
	      usage (1);
	    break;

	  case 'h':
	    usage (0);
	    break;

	  case 'V':
	    printf ("%s (%s) %s\n", program_name, PACKAGE_NAME, PACKAGE_VERSION);
	    return 0;

	  case 'v':
	    verbosity++;
	    break;

	  default:
	    usage (1);
	    break;
	  }
    }

  if (verbosity > 1)
    grub_env_set ("debug", "all");

  /* Obtain ARGUMENT.  */
  if (optind >= argc)
    {
      fprintf (stderr, "No path or device is specified.\n");
      usage (1);
    }

  if (optind + 1 != argc)
    {
      fprintf (stderr, "Unknown extra argument `%s'.\n", argv[optind + 1]);
      usage (1);
    }

  argument = argv[optind];

  /* Initialize the emulated biosdisk driver.  */
  grub_util_biosdisk_init (dev_map ? : DEFAULT_DEVICE_MAP);

  /* Initialize all modules. */
  grub_init_all ();

  grub_lvm_fini ();
  grub_mdraid09_fini ();
  grub_mdraid1x_fini ();
  grub_raid_fini ();
  grub_raid_init ();
  grub_mdraid09_init ();
  grub_mdraid1x_init ();
  grub_lvm_init ();

  /* Do it.  */
  if (argument_is_device)
    probe (NULL, argument);
  else
    probe (argument, NULL);

  /* Free resources.  */
  grub_fini_all ();
  grub_util_biosdisk_fini ();

  free (dev_map);

  return 0;
}
