/* grub-setup.c - make GRUB usable */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004,2005,2006,2007,2008,2009  Free Software Foundation, Inc.
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
#include <grub/gpt_partition.h>
#include <grub/env.h>
#include <grub/util/hostdisk.h>
#include <grub/machine/boot.h>
#include <grub/machine/kernel.h>
#include <grub/term.h>
#include <grub/util/raid.h>
#include <grub/util/lvm.h>

#include <grub_setup_init.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <grub/util/getroot.h>

#define _GNU_SOURCE	1
#include <getopt.h>

/* This program fills in various fields inside of the 'boot' and 'core'
 * image files.
 *
 * The 'boot' image needs to know the OBP path name of the root
 * device.  It also needs to know the initial block number of
 * 'core' (which is 'diskboot' concatenated with 'kernel' and
 * all the modules, this is created by grub-mkimage).  This resulting
 * 'boot' image is 512 bytes in size and is placed in the second block
 * of a partition.
 *
 * The initial 'diskboot' block acts as a loader for the actual GRUB
 * kernel.  It contains the loading code and then a block list.
 *
 * The block list of 'core' starts at the end of the 'diskboot' image
 * and works it's way backwards towards the end of the code of 'diskboot'.
 *
 * We patch up the images with the necessary values and write out the
 * result.
 */

#define DEFAULT_BOOT_FILE	"boot.img"
#define DEFAULT_CORE_FILE	"core.img"

/* This is the blocklist used in the diskboot image.  */
struct boot_blocklist
{
  grub_uint64_t start;
  grub_uint32_t len;
} __attribute__ ((packed));

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

static char *compute_dest_ofpath (const char *dest)
{
  int len = strlen (dest);
  char *res, *p, c;

  res = xmalloc (len);
  p = res;
  while ((c = *dest++) != '\0')
    {
      if (c == '\\' && *dest == ',')
	{
	  *p++ = ',';
	  dest++;
	}
      else
	*p++ = c;
    }
  *p++ = '\0';

  return res;
}

static void
setup (const char *prefix, const char *dir,
       const char *boot_file, const char *core_file,
       const char *root, const char *dest)
{
  char *boot_path, *core_path;
  char *boot_img, *core_img;
  size_t boot_size, core_size;
  grub_uint16_t core_sectors;
  grub_device_t root_dev, dest_dev;
  char *boot_devpath, *dest_ofpath;
  grub_disk_addr_t *kernel_sector;
  struct boot_blocklist *first_block, *block;
  char *tmp_img;
  int i;
  grub_disk_addr_t first_sector;
  grub_uint16_t last_length = GRUB_DISK_SECTOR_SIZE;
  grub_file_t file;
  FILE *fp;
  struct { grub_uint64_t start; grub_uint64_t end; } embed_region;
  embed_region.start = embed_region.end = ~0UL;

  auto void NESTED_FUNC_ATTR save_first_sector (grub_disk_addr_t sector,
						unsigned int offset,
						unsigned int length);
  auto void NESTED_FUNC_ATTR save_blocklists (grub_disk_addr_t sector,
					      unsigned int offset,
					      unsigned int length);

  void NESTED_FUNC_ATTR save_first_sector (grub_disk_addr_t sector,
					   unsigned int offset,
					   unsigned int length)
    {
      grub_util_info ("first sector is <%llu,%u,%u>", sector, offset, length);

      if (offset != 0 || length != GRUB_DISK_SECTOR_SIZE)
	grub_util_error ("The first sector of the core file "
			 "is not sector-aligned");

      first_sector = sector;
    }

  void NESTED_FUNC_ATTR save_blocklists (grub_disk_addr_t sector,
					 unsigned int offset,
					 unsigned int length)
    {
      struct boot_blocklist *prev = block + 1;

      grub_util_info ("saving <%llu,%u,%u>", sector, offset, length);

      if (offset != 0 || last_length != GRUB_DISK_SECTOR_SIZE)
	grub_util_error ("Non-sector-aligned data is found in the core file");

      if (block != first_block
	  && (grub_be_to_cpu64 (prev->start)
	      + grub_be_to_cpu16 (prev->len)) == sector)
	prev->len = grub_cpu_to_be16 (grub_be_to_cpu16 (prev->len) + 1);
      else
	{
	  block->start = grub_cpu_to_be64 (sector);
	  block->len = grub_cpu_to_be16 (1);

	  block--;
	  if (block->len)
	    grub_util_error ("The sectors of the core file are too fragmented");
	}

      last_length = length;
    }

  dest_ofpath = compute_dest_ofpath (dest);

  /* Read the boot image by the OS service.  */
  boot_path = grub_util_get_path (dir, boot_file);
  boot_size = grub_util_get_image_size (boot_path);
  if (boot_size != GRUB_DISK_SECTOR_SIZE)
    grub_util_error ("The size of `%s' is not %d",
		     boot_path, GRUB_DISK_SECTOR_SIZE);
  boot_img = grub_util_read_image (boot_path);
  free (boot_path);

  /* Set the addresses of variables in the boot image.  */
  boot_devpath = (char *) (boot_img
			   + GRUB_BOOT_AOUT_HEADER_SIZE
			   + GRUB_BOOT_MACHINE_BOOT_DEVPATH);
  kernel_sector = (grub_disk_addr_t *) (boot_img
					+ GRUB_BOOT_AOUT_HEADER_SIZE
					+ GRUB_BOOT_MACHINE_KERNEL_SECTOR);

  core_path = grub_util_get_path (dir, core_file);
  core_size = grub_util_get_image_size (core_path);
  core_sectors = ((core_size + GRUB_DISK_SECTOR_SIZE - 1)
		  >> GRUB_DISK_SECTOR_BITS);
  if (core_size < GRUB_DISK_SECTOR_SIZE)
    grub_util_error ("The size of `%s' is too small", core_path);

  core_img = grub_util_read_image (core_path);
  free (core_path);

  /* Have FIRST_BLOCK to point to the first blocklist.  */
  first_block = (struct boot_blocklist *) (core_img
					   + GRUB_DISK_SECTOR_SIZE
					   - sizeof (*block));

  grub_util_info ("root is '%s', dest is '%s', and dest_ofpath is '%s'",
		  root, dest, dest_ofpath);

  /* Open the root device and the destination device.  */
  grub_util_info ("Opening root");
  root_dev = grub_device_open (root);
  if (! root_dev)
    grub_util_error ("%s", grub_errmsg);

  grub_util_info ("Opening dest");
  dest_dev = grub_device_open (dest);
  if (! dest_dev)
    grub_util_error ("%s", grub_errmsg);

  grub_util_info ("setting the root device to `%s'", root);
  if (grub_env_set ("root", root) != GRUB_ERR_NONE)
    grub_util_error ("%s", grub_errmsg);

  /* The core image must be put on a filesystem unfortunately.  */
  grub_util_info ("will leave the core image on the filesystem");

  /* Make sure that GRUB reads the identical image as the OS.  */
  tmp_img = xmalloc (core_size);
  core_path = grub_util_get_path (prefix, core_file);

  /* It is a Good Thing to sync two times.  */
  sync ();
  sync ();

#define MAX_TRIES	5

  for (i = 0; i < MAX_TRIES; i++)
    {
      grub_util_info ("attempting to read the core image `%s' from GRUB%s",
		      core_path, (i == 0) ? "" : " again");

      grub_disk_cache_invalidate_all ();

      file = grub_file_open (core_path);
      if (file)
	{
	  if (grub_file_size (file) != core_size)
	    grub_util_info ("succeeded in opening the core image but the size is different (%d != %d)",
			    (int) grub_file_size (file), (int) core_size);
	  else if (grub_file_read (file, tmp_img, core_size)
		   != (grub_ssize_t) core_size)
	    grub_util_info ("succeeded in opening the core image but cannot read %d bytes",
			    (int) core_size);
	  else if (memcmp (core_img, tmp_img, core_size) != 0)
	    {
#if 0
	      FILE *dump;
	      FILE *dump2;

	      dump = fopen ("dump.img", "wb");
	      if (dump)
		{
		  fwrite (tmp_img, 1, core_size, dump);
		  fclose (dump);
		}

	      dump2 = fopen ("dump2.img", "wb");
	      if (dump2)
		{
		  fwrite (core_img, 1, core_size, dump2);
		  fclose (dump2);
		}

#endif
	      grub_util_info ("succeeded in opening the core image but the data is different");
	    }
	  else
	    {
	      grub_file_close (file);
	      break;
	    }

	  grub_file_close (file);
	}
      else
	grub_util_info ("couldn't open the core image");

      if (grub_errno)
	grub_util_info ("error message = %s", grub_errmsg);

      grub_errno = GRUB_ERR_NONE;
      sync ();
      sleep (1);
    }

  if (i == MAX_TRIES)
    grub_util_error ("Cannot read `%s' correctly", core_path);

  /* Clean out the blocklists.  */
  block = first_block;
  while (block->len)
    {
      block->start = 0;
      block->len = 0;

      block--;

      if ((char *) block <= core_img)
	grub_util_error ("No terminator in the core image");
    }

  /* Now read the core image to determine where the sectors are.  */
  file = grub_file_open (core_path);
  if (! file)
    grub_util_error ("%s", grub_errmsg);

  file->read_hook = save_first_sector;
  if (grub_file_read (file, tmp_img, GRUB_DISK_SECTOR_SIZE)
      != GRUB_DISK_SECTOR_SIZE)
    grub_util_error ("Failed to read the first sector of the core image");

  block = first_block;
  file->read_hook = save_blocklists;
  if (grub_file_read (file, tmp_img, core_size - GRUB_DISK_SECTOR_SIZE)
      != (grub_ssize_t) core_size - GRUB_DISK_SECTOR_SIZE)
    grub_util_error ("Failed to read the rest sectors of the core image");

  grub_file_close (file);

  free (core_path);
  free (tmp_img);

  *kernel_sector = grub_cpu_to_be64 (first_sector);

  strcpy(boot_devpath, dest_ofpath);

  grub_util_info ("boot device path %s, prefix is %s, dest is %s",
		  boot_devpath, prefix, dest);

  /* Write the first two sectors of the core image onto the disk.  */
  core_path = grub_util_get_path (dir, core_file);
  grub_util_info ("opening the core image `%s'", core_path);
  fp = fopen (core_path, "r+b");
  if (! fp)
    grub_util_error ("Cannot open `%s'", core_path);

  grub_util_write_image (core_img, GRUB_DISK_SECTOR_SIZE, fp);
  fclose (fp);
  free (core_path);

  /* Write the boot image onto the disk.  */
  if (grub_disk_write (dest_dev->disk, 1, 0, GRUB_DISK_SECTOR_SIZE, boot_img))
    grub_util_error ("%s", grub_errmsg);

  /* Sync is a Good Thing.  */
  sync ();

  free (core_img);
  free (boot_img);
  grub_device_close (dest_dev);
  grub_device_close (root_dev);
}

static struct option options[] =
  {
    {"boot-image", required_argument, 0, 'b'},
    {"core-image", required_argument, 0, 'c'},
    {"directory", required_argument, 0, 'd'},
    {"device-map", required_argument, 0, 'm'},
    {"root-device", required_argument, 0, 'r'},
    {"help", no_argument, 0, 'h'},
    {"version", no_argument, 0, 'V'},
    {"verbose", no_argument, 0, 'v'},
    {0, 0, 0, 0}
  };

static void
usage (int status)
{
  if (status)
    fprintf (stderr, "Try ``grub-setup --help'' for more information.\n");
  else
    printf ("\
Usage: grub-setup [OPTION]... DEVICE\n\
\n\
Set up images to boot from DEVICE.\n\
DEVICE must be a GRUB device (e.g. ``(hd0,1)'').\n\
\n\
  -b, --boot-image=FILE   use FILE as the boot image [default=%s]\n\
  -c, --core-image=FILE   use FILE as the core image [default=%s]\n\
  -d, --directory=DIR     use GRUB files in the directory DIR [default=%s]\n\
  -m, --device-map=FILE   use FILE as the device map [default=%s]\n\
  -r, --root-device=DEV   use DEV as the root device [default=guessed]\n\
  -h, --help              display this message and exit\n\
  -V, --version           print version information and exit\n\
  -v, --verbose           print verbose messages\n\
\n\
Report bugs to <%s>.\n\
",
	    DEFAULT_BOOT_FILE, DEFAULT_CORE_FILE, DEFAULT_DIRECTORY,
	    DEFAULT_DEVICE_MAP, PACKAGE_BUGREPORT);

  exit (status);
}

struct grub_setup_info
{
  char *boot_file;
  char *core_file;
  char *dir;
  char *dev_map;
  char *root_dev;
  char *prefix;
  char *dest_dev;
};

static void
init_info (struct grub_setup_info *gp)
{
  gp->boot_file = NULL;
  gp->core_file = NULL;
  gp->dir = NULL;
  gp->dev_map = NULL;
  gp->root_dev = NULL;
  gp->prefix = NULL;
  gp->dest_dev = NULL;
}

static int
parse_options (struct grub_setup_info *gp, int argc, char *argv[])
{
  while (1)
    {
      int c = getopt_long (argc, argv, "b:c:d:m:r:hVv", options, 0);

      if (c == -1)
	break;
      else
	switch (c)
	  {
	  case 'b':
	    if (gp->boot_file)
	      free (gp->boot_file);

	    gp->boot_file = xstrdup (optarg);
	    break;

	  case 'c':
	    if (gp->core_file)
	      free (gp->core_file);

	    gp->core_file = xstrdup (optarg);
	    break;

	  case 'd':
	    if (gp->dir)
	      free (gp->dir);

	    gp->dir = xstrdup (optarg);
	    break;

	  case 'm':
	    if (gp->dev_map)
	      free (gp->dev_map);

	    gp->dev_map = xstrdup (optarg);
	    break;

	  case 'r':
	    if (gp->root_dev)
	      free (gp->root_dev);

	    gp->root_dev = xstrdup (optarg);
	    break;

	  case 'h':
	    usage (0);
	    break;

	  case 'V':
	    printf ("grub-setup (%s) %s\n", PACKAGE_NAME, PACKAGE_VERSION);
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

  if (optind >= argc)
    {
      fprintf (stderr, "No device is specified.\n");
      usage (1);
    }

  if (optind + 1 != argc)
    {
      fprintf (stderr, "Unknown extra argument `%s'.\n", argv[optind + 1]);
      usage (1);
    }
  return 1;
}

static char *
get_device_name (char *dev)
{
  size_t len = strlen (dev);

  if (dev[0] != '(' || dev[len - 1] != ')')
    return 0;

  dev[len - 1] = '\0';
  return dev + 1;
}

static void
find_dest_dev (struct grub_setup_info *gp, char *argv[])
{
  gp->dest_dev = get_device_name (argv[optind]);
  if (! gp->dest_dev)
    {
      /* Possibly, the user specified an OS device file.  */
      gp->dest_dev = grub_util_get_grub_dev (argv[optind]);
      if (! gp->dest_dev)
	{
	  fprintf (stderr, "Invalid device `%s'.\n", argv[optind]);
	  usage (1);
	}
      grub_util_info ("transformed OS device '%s' into GRUB device '%s'",
		      argv[optind], gp->dest_dev);
    }
  else
    {
      /* For simplicity.  */
      gp->dest_dev = xstrdup (gp->dest_dev);
      grub_util_info ("Using '%s' as GRUB device", gp->dest_dev);
    }
}

static void
check_root_dev (struct grub_setup_info *gp)
{
  if (gp->root_dev)
    {
      char *tmp = get_device_name (gp->root_dev);

      if (! tmp)
	grub_util_error ("Invalid root device `%s'", gp->root_dev);

      tmp = xstrdup (tmp);
      free (gp->root_dev);
      gp->root_dev = tmp;
    }
  else
    {
      char *dir = gp->dir ? gp->dir : DEFAULT_DIRECTORY;
      char *root_device = grub_guess_root_device (dir);

      gp->root_dev = grub_util_get_grub_dev (root_device);
      if (! gp->root_dev)
	{
	  grub_util_info ("guessing the root device failed, because of `%s'",
			  grub_errmsg);
	  grub_util_error ("Cannot guess the root device. "
			   "Specify the option ``--root-device''.");
	}
      grub_util_info ("Guessed root device '%s' and root_dev '%s' from "
		      "dir '%s'", root_device, gp->root_dev, dir);
    }
}

static void
free_memory (struct grub_setup_info *gp)
{
  free (gp->boot_file);
  free (gp->core_file);
  free (gp->dir);
  free (gp->dev_map);
  free (gp->root_dev);
  free (gp->prefix);
  free (gp->dest_dev);
}

int
main (int argc, char *argv[])
{
  struct grub_setup_info ginfo;

  progname = "grub-setup";

  init_info (&ginfo);
  if (!parse_options (&ginfo, argc, argv))
    return 0;

  /* Initialize the emulated biosdisk driver.  */
  grub_util_biosdisk_init (ginfo.dev_map ? ginfo.dev_map : DEFAULT_DEVICE_MAP);

  /* Initialize all modules. */
  grub_init_all ();

  find_dest_dev (&ginfo, argv);

  ginfo.prefix = grub_get_prefix (ginfo.dir ? : DEFAULT_DIRECTORY);

  check_root_dev (&ginfo);

  /* Do the real work.  */
  setup (ginfo.prefix,
	 ginfo.dir ? ginfo.dir : DEFAULT_DIRECTORY,
	 ginfo.boot_file ? ginfo.boot_file : DEFAULT_BOOT_FILE,
	 ginfo.core_file ? ginfo.core_file : DEFAULT_CORE_FILE,
	 ginfo.root_dev, ginfo.dest_dev);

  /* Free resources.  */
  grub_fini_all ();

  free_memory (&ginfo);

  return 0;
}
