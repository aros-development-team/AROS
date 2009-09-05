/* grub-probe.c - probe device information for a given path */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2006,2007,2008,2009  Free Software Foundation, Inc.
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
#include <grub/file.h>
#include <grub/fs.h>
#include <grub/partition.h>
#include <grub/msdos_partition.h>
#include <grub/util/hostdisk.h>
#include <grub/util/getroot.h>
#include <grub/term.h>
#include <grub/env.h>
#include <grub/raid.h>

#include <grub_probe_init.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#define _GNU_SOURCE	1
#include <getopt.h>

enum {
  PRINT_FS,
  PRINT_FS_UUID,
  PRINT_DRIVE,
  PRINT_DEVICE,
  PRINT_PARTMAP,
  PRINT_ABSTRACTION,
};

int print = PRINT_FS;
static unsigned int argument_is_device = 0;

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

struct grub_handler_class grub_term_input_class;
struct grub_handler_class grub_term_output_class;

void
grub_refresh (void)
{
  fflush (stdout);
}

static void
probe_partmap (grub_disk_t disk)
{
  if (disk->partition == NULL)
    {
      grub_util_info ("No partition map found for %s", disk->name);
      return;
    }

  printf ("%s\n", disk->partition->partmap->name);
}

static int
probe_raid_level (grub_disk_t disk)
{
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
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
      if (! grub_util_check_char_device (device_name))
        grub_util_error ("%s is not a character device.\n", device_name);
#else
      if (! grub_util_check_block_device (device_name))
        grub_util_error ("%s is not a block device.\n", device_name);
#endif
    }
  else
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
    grub_util_error ("Cannot find a GRUB drive for %s.  Check your device.map.\n", device_name);

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

      raid_level = probe_raid_level (dev->disk);
      if (raid_level >= 0)
	{
	  is_raid = 1;
	  is_raid5 |= (raid_level == 5);
	  is_raid6 |= (raid_level == 6);
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
	  printf ("mdraid ");
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
      struct stat st;

      stat (path, &st);

      if (S_ISREG (st.st_mode))
	{
	  /* Regular file.  Verify that we can read it properly.  */

	  grub_file_t file;
	  grub_util_info ("reading %s via OS facilities", path);
	  filebuf_via_sys = grub_util_read_image (path);

	  grub_util_info ("reading %s via GRUB facilities", path);
	  asprintf (&grub_path, "(%s)%s", drive_name, path);
	  file = grub_file_open (grub_path);
	  filebuf_via_grub = xmalloc (file->size);
	  grub_file_read (file, filebuf_via_grub, file->size);

	  grub_util_info ("comparing");

	  if (memcmp (filebuf_via_grub, filebuf_via_sys, file->size))
	    grub_util_error ("files differ");
	}
      printf ("%s\n", fs->name);
    }

  if (print == PRINT_FS_UUID)
    {
      char *uuid;
      if (! fs->uuid)
	grub_util_error ("%s does not support UUIDs", fs->name);

      fs->uuid (dev, &uuid);

      printf ("%s\n", uuid);
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
	     "Try ``grub-probe --help'' for more information.\n");
  else
    printf ("\
Usage: grub-probe [OPTION]... [PATH|DEVICE]\n\
\n\
Probe device information for a given path (or device, if the -d option is given).\n\
\n\
  -d, --device              given argument is a system device, not a path\n\
  -m, --device-map=FILE     use FILE as the device map [default=%s]\n\
  -t, --target=(fs|fs_uuid|drive|device|partmap|abstraction)\n\
                            print filesystem module, GRUB drive, system device, partition map module or abstraction module [default=fs]\n\
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
  char *argument;

  progname = "grub-probe";

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
