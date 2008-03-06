/* grub-setup.c - make GRUB usable */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004,2005,2006,2007,2008  Free Software Foundation, Inc.
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
#include <grub/pc_partition.h>
#include <grub/gpt_partition.h>
#include <grub/env.h>
#include <grub/util/biosdisk.h>
#include <grub/machine/boot.h>
#include <grub/machine/kernel.h>
#include <grub/term.h>
#include <grub/util/raid.h>
#include <grub/util/lvm.h>

static const grub_gpt_part_type_t grub_gpt_partition_type_bios_boot = GRUB_GPT_PARTITION_TYPE_BIOS_BOOT;

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

#define DEFAULT_BOOT_FILE	"boot.img"
#define DEFAULT_CORE_FILE	"core.img"

/* This is the blocklist used in the diskboot image.  */
struct boot_blocklist
{
  grub_uint64_t start;
  grub_uint16_t len;
  grub_uint16_t segment;
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
setup (const char *prefix, const char *dir,
       const char *boot_file, const char *core_file,
       const char *root, const char *dest, int must_embed)
{
  char *boot_path, *core_path;
  char *boot_img, *core_img;
  size_t boot_size, core_size;
  grub_uint16_t core_sectors;
  grub_device_t root_dev, dest_dev;
  grub_uint8_t *boot_drive, *root_drive;
  grub_disk_addr_t *kernel_sector;
  grub_uint16_t *boot_drive_check;
  struct boot_blocklist *first_block, *block;
  grub_int32_t *install_dos_part, *install_bsd_part;
  char *install_prefix;
  char *tmp_img;
  int i;
  grub_disk_addr_t first_sector;
  grub_uint16_t current_segment
    = GRUB_BOOT_MACHINE_KERNEL_SEG + (GRUB_DISK_SECTOR_SIZE >> 4);
  grub_uint16_t last_length = GRUB_DISK_SECTOR_SIZE;
  grub_file_t file;
  FILE *fp;
  struct { grub_uint64_t start; grub_uint64_t end; } embed_region;
  embed_region.start = embed_region.end = ~0UL;
  int able_to_embed = 1;
  
  auto void NESTED_FUNC_ATTR save_first_sector (grub_disk_addr_t sector, unsigned offset,
			       unsigned length);
  auto void NESTED_FUNC_ATTR save_blocklists (grub_disk_addr_t sector, unsigned offset,
			     unsigned length);

  auto int find_usable_region (grub_disk_t disk,
			       const grub_partition_t p);
  int find_usable_region (grub_disk_t disk __attribute__ ((unused)),
			  const grub_partition_t p)
    {
      if (! strcmp (p->partmap->name, "pc_partition_map"))
	{
	  struct grub_pc_partition *pcdata = p->data;
	  
	  /* There's always an embed region, and it starts right after the MBR.  */
	  embed_region.start = 1;
	  
	  /* For its end offset, include as many dummy partitions as we can.  */
	  if (! grub_pc_partition_is_empty (pcdata->dos_type)
	      && ! grub_pc_partition_is_bsd (pcdata->dos_type)
	      && embed_region.end > p->start)
	    embed_region.end = p->start;
	}
      else
	{
	  struct grub_gpt_partentry *gptdata = p->data;
	  
	  /* If there's an embed region, it is in a dedicated partition.  */
	  if (! memcmp (&gptdata->type, &grub_gpt_partition_type_bios_boot, 16))
	    {
	      embed_region.start = p->start;
	      embed_region.end = p->start + p->len;
	      
	      return 1;
	    }
	}

      return 0;
    }
  
  void NESTED_FUNC_ATTR save_first_sector (grub_disk_addr_t sector, unsigned offset,
			  unsigned length)
    {
      grub_util_info ("the first sector is <%llu,%u,%u>",
		      sector, offset, length);
      
      if (offset != 0 || length != GRUB_DISK_SECTOR_SIZE)
	grub_util_error ("The first sector of the core file is not sector-aligned");

      first_sector = sector;
    }

  void NESTED_FUNC_ATTR save_blocklists (grub_disk_addr_t sector, unsigned offset,
			unsigned length)
    {
      struct boot_blocklist *prev = block + 1;

      grub_util_info ("saving <%llu,%u,%u> with the segment 0x%x",
		      sector, offset, length, (unsigned) current_segment);
      
      if (offset != 0 || last_length != GRUB_DISK_SECTOR_SIZE)
	grub_util_error ("Non-sector-aligned data is found in the core file");

      if (block != first_block
	  && (grub_le_to_cpu64 (prev->start)
	      + grub_le_to_cpu16 (prev->len)) == sector)
	prev->len = grub_cpu_to_le16 (grub_le_to_cpu16 (prev->len) + 1);
      else
	{
	  block->start = grub_cpu_to_le64 (sector);
	  block->len = grub_cpu_to_le16 (1);
	  block->segment = grub_cpu_to_le16 (current_segment);

	  block--;
	  if (block->len)
	    grub_util_error ("The sectors of the core file are too fragmented");
	}
      
      last_length = length;
      current_segment += GRUB_DISK_SECTOR_SIZE >> 4;
    }
  
  /* Read the boot image by the OS service.  */
  boot_path = grub_util_get_path (dir, boot_file);
  boot_size = grub_util_get_image_size (boot_path);
  if (boot_size != GRUB_DISK_SECTOR_SIZE)
    grub_util_error ("The size of `%s' is not %d",
		     boot_path, GRUB_DISK_SECTOR_SIZE);
  boot_img = grub_util_read_image (boot_path);
  free (boot_path);

  /* Set the addresses of variables in the boot image.  */
  boot_drive = (grub_uint8_t *) (boot_img + GRUB_BOOT_MACHINE_BOOT_DRIVE);
  root_drive = (grub_uint8_t *) (boot_img + GRUB_BOOT_MACHINE_ROOT_DRIVE);
  kernel_sector = (grub_disk_addr_t *) (boot_img
				     + GRUB_BOOT_MACHINE_KERNEL_SECTOR);
  boot_drive_check = (grub_uint16_t *) (boot_img
					+ GRUB_BOOT_MACHINE_DRIVE_CHECK);
  
  core_path = grub_util_get_path (dir, core_file);
  core_size = grub_util_get_image_size (core_path);
  core_sectors = ((core_size + GRUB_DISK_SECTOR_SIZE - 1)
		  >> GRUB_DISK_SECTOR_BITS);
  if (core_size < GRUB_DISK_SECTOR_SIZE)
    grub_util_error ("The size of `%s' is too small", core_path);
  else if (core_size > 0xFFFF * GRUB_DISK_SECTOR_SIZE)
    grub_util_error ("The size of `%s' is too large", core_path);
  
  core_img = grub_util_read_image (core_path);
  free (core_path);

  /* Have FIRST_BLOCK to point to the first blocklist.  */
  first_block = (struct boot_blocklist *) (core_img
					   + GRUB_DISK_SECTOR_SIZE
					   - sizeof (*block));

  install_dos_part = (grub_int32_t *) (core_img + GRUB_DISK_SECTOR_SIZE
				       + GRUB_KERNEL_MACHINE_INSTALL_DOS_PART);
  install_bsd_part = (grub_int32_t *) (core_img + GRUB_DISK_SECTOR_SIZE
				       + GRUB_KERNEL_MACHINE_INSTALL_BSD_PART);
  install_prefix = (core_img + GRUB_DISK_SECTOR_SIZE
		    + GRUB_KERNEL_MACHINE_PREFIX);

  /* Open the root device and the destination device.  */
  root_dev = grub_device_open (root);
  if (! root_dev)
    grub_util_error ("%s", grub_errmsg);

  dest_dev = grub_device_open (dest);
  if (! dest_dev)
    grub_util_error ("%s", grub_errmsg);

  grub_util_info ("setting the root device to `%s'", root);
  if (grub_env_set ("root", root) != GRUB_ERR_NONE)
    grub_util_error ("%s", grub_errmsg);

  /* Read the original sector from the disk.  */
  tmp_img = xmalloc (GRUB_DISK_SECTOR_SIZE);
  if (grub_disk_read (dest_dev->disk, 0, 0, GRUB_DISK_SECTOR_SIZE, tmp_img))
    grub_util_error ("%s", grub_errmsg);

  /* Copy the possible DOS BPB.  */
  memcpy (boot_img + GRUB_BOOT_MACHINE_BPB_START,
	  tmp_img + GRUB_BOOT_MACHINE_BPB_START,
	  GRUB_BOOT_MACHINE_BPB_END - GRUB_BOOT_MACHINE_BPB_START);

  /* Copy the possible partition table.  */
  if (dest_dev->disk->has_partitions)
    memcpy (boot_img + GRUB_BOOT_MACHINE_WINDOWS_NT_MAGIC,
	    tmp_img + GRUB_BOOT_MACHINE_WINDOWS_NT_MAGIC,
	    GRUB_BOOT_MACHINE_PART_END - GRUB_BOOT_MACHINE_WINDOWS_NT_MAGIC);

  free (tmp_img);
  
  /* If DEST_DRIVE is a hard disk, enable the workaround, which is
     for buggy BIOSes which don't pass boot drive correctly. Instead,
     they pass 0x00 or 0x01 even when booted from 0x80.  */
  if (dest_dev->disk->id & 0x80)
    /* Replace the jmp (2 bytes) with double nop's.  */
    *boot_drive_check = 0x9090;
  
  /* If the destination device can have partitions and it is the MBR,
     try to embed the core image into after the MBR.  */
  if (dest_dev->disk->has_partitions && ! dest_dev->disk->partition)
    {
      grub_partition_iterate (dest_dev->disk, find_usable_region);

      /* If there is enough space...  */
      if ((unsigned long) core_sectors <= embed_region.end - embed_region.start)
	{
	  grub_util_info ("will embed the core image at sector 0x%llx", embed_region.start);

	  /* The first blocklist contains the whole sectors.  */
	  first_block->start = grub_cpu_to_le64 (embed_region.start + 1);
	  first_block->len = grub_cpu_to_le16 (core_sectors - 1);
	  first_block->segment
	    = grub_cpu_to_le16 (GRUB_BOOT_MACHINE_KERNEL_SEG
				+ (GRUB_DISK_SECTOR_SIZE >> 4));

	  /* Make sure that the second blocklist is a terminator.  */
	  block = first_block - 1;
	  block->start = 0;
	  block->len = 0;
	  block->segment = 0;

	  /* Embed information about the installed location.  */
	  if (must_embed)
	    *install_dos_part = *install_bsd_part = grub_cpu_to_le32 (-2);
	  else if (root_dev->disk->partition)
	    {
	      if (strcmp (root_dev->disk->partition->partmap->name,
			  "pc_partition_map") == 0)
		{
		  struct grub_pc_partition *pcdata =
		    root_dev->disk->partition->data;
		  *install_dos_part
		    = grub_cpu_to_le32 (pcdata->dos_part);
		  *install_bsd_part
		    = grub_cpu_to_le32 (pcdata->bsd_part);
		}
	      else if (strcmp (root_dev->disk->partition->partmap->name,
			       "gpt_partition_map") == 0)
		{
		  *install_dos_part = grub_cpu_to_le32 (root_dev->disk->partition->index);
		  *install_bsd_part = grub_cpu_to_le32 (-1);
		}
	      else
		grub_util_error ("No PC style partitions found");
	    }
	  else
	    *install_dos_part = *install_bsd_part = grub_cpu_to_le32 (-1);

	  grub_util_info ("dos partition is %d, bsd partition is %d, prefix is %s",
		  grub_le_to_cpu32 (*install_dos_part),
		  grub_le_to_cpu32 (*install_bsd_part),
		  prefix);
	  strcpy (install_prefix, prefix);
	  
	  /* Write the core image onto the disk.  */
	  if (grub_disk_write (dest_dev->disk, embed_region.start, 0, core_size, core_img))
	    grub_util_error ("%s", grub_errmsg);

	  /* The boot image and the core image are on the same drive,
	     so there is no need to specify the boot drive explicitly.  */
	  *boot_drive = 0xff;
	  *kernel_sector = grub_cpu_to_le64 (embed_region.start);

          /* If the root device is different from the destination device,
             it is necessary to embed the root drive explicitly.  */
          if (root_dev->disk->id != dest_dev->disk->id)
            *root_drive = (grub_uint8_t) root_dev->disk->id;
          else
            *root_drive = 0xFF;

	  /* Write the boot image onto the disk.  */
	  if (grub_disk_write (dest_dev->disk, 0, 0, GRUB_DISK_SECTOR_SIZE,
			       boot_img))
	    grub_util_error ("%s", grub_errmsg);

	  goto finish;
	}
      else
        able_to_embed = 0;
    }
  else
    able_to_embed = 0;

  if (must_embed && ! able_to_embed)
    grub_util_error ("Can't embed the core image, but this is required when\n"
		     "the root device is on a RAID array or LVM volume.");
  
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
      block->segment = 0;

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
  
  *kernel_sector = grub_cpu_to_le64 (first_sector);

  /* If the destination device is different from the root device,
     it is necessary to embed the boot drive explicitly.  */
  if (root_dev->disk->id != dest_dev->disk->id)
    *boot_drive = (grub_uint8_t) root_dev->disk->id;
  else
    *boot_drive = 0xFF;

  /* When the core image is not embedded, the root device always follows
     the boot device.  */
  *root_drive = 0xFF;

  /* Embed information about the installed location.  */
  if (root_dev->disk->partition)
    {
      struct grub_pc_partition *pcdata =
	root_dev->disk->partition->data;

	if (strcmp (root_dev->disk->partition->partmap->name,
		  "pc_partition_map") == 0)
	  {
	    *install_dos_part
	      = grub_cpu_to_le32 (pcdata->dos_part);
	    *install_bsd_part
	      = grub_cpu_to_le32 (pcdata->bsd_part);
	  }
	else if (strcmp (root_dev->disk->partition->partmap->name,
		  "gpt_partition_map") == 0)
	  {
	    *install_dos_part = grub_cpu_to_le32 (root_dev->disk->partition->index);
	    *install_bsd_part = grub_cpu_to_le32 (-1);
	  }
	else
	  grub_util_error ("No PC style partitions found");
    }
  else
    *install_dos_part = *install_bsd_part = grub_cpu_to_le32 (-1);

  grub_util_info ("dos partition is %d, bsd partition is %d, prefix is %s",
		  grub_le_to_cpu32 (*install_dos_part),
		  grub_le_to_cpu32 (*install_bsd_part),
		  prefix);
  strcpy (install_prefix, prefix);
  
  /* Write the first two sectors of the core image onto the disk.  */
  core_path = grub_util_get_path (dir, core_file);
  grub_util_info ("opening the core image `%s'", core_path);
  fp = fopen (core_path, "r+b");
  if (! fp)
    grub_util_error ("Cannot open `%s'", core_path);

  grub_util_write_image (core_img, GRUB_DISK_SECTOR_SIZE * 2, fp);
  fclose (fp);
  free (core_path);

  /* Write the boot image onto the disk.  */
  if (grub_disk_write (dest_dev->disk, 0, 0, GRUB_DISK_SECTOR_SIZE, boot_img))
    grub_util_error ("%s", grub_errmsg);

 finish:

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

static char *
get_device_name (char *dev)
{
  size_t len = strlen (dev);
  
  if (dev[0] != '(' || dev[len - 1] != ')')
    return 0;

  dev[len - 1] = '\0';
  return dev + 1;
}

int
main (int argc, char *argv[])
{
  char *boot_file = 0;
  char *core_file = 0;
  char *dir = 0;
  char *dev_map = 0;
  char *root_dev = 0;
  char *prefix;
  char *dest_dev;
  int must_embed = 0;
  
  progname = "grub-setup";

  /* Check for options.  */
  while (1)
    {
      int c = getopt_long (argc, argv, "b:c:d:m:r:hVv", options, 0);

      if (c == -1)
	break;
      else
	switch (c)
	  {
	  case 'b':
	    if (boot_file)
	      free (boot_file);

	    boot_file = xstrdup (optarg);
	    break;

	  case 'c':
	    if (core_file)
	      free (core_file);

	    core_file = xstrdup (optarg);
	    break;

	  case 'd':
	    if (dir)
	      free (dir);

	    dir = xstrdup (optarg);
	    break;
	    
	  case 'm':
	    if (dev_map)
	      free (dev_map);

	    dev_map = xstrdup (optarg);
	    break;

	  case 'r':
	    if (root_dev)
	      free (root_dev);

	    root_dev = xstrdup (optarg);
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

  /* Obtain DEST_DEV.  */
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

  /* Initialize the emulated biosdisk driver.  */
  grub_util_biosdisk_init (dev_map ? : DEFAULT_DEVICE_MAP);

  /* Initialize all modules. */
  grub_init_all ();
  
  dest_dev = get_device_name (argv[optind]);
  if (! dest_dev)
    {
      /* Possibly, the user specified an OS device file.  */
      dest_dev = grub_util_get_grub_dev (argv[optind]);
      if (! dest_dev)
	{
	  fprintf (stderr, "Invalid device `%s'.\n", argv[optind]);
	  usage (1);
	}
    }
  else
    /* For simplicity.  */
    dest_dev = xstrdup (dest_dev);

  prefix = grub_get_prefix (dir ? : DEFAULT_DIRECTORY);
  
  if (root_dev)
    {
      char *tmp = get_device_name (root_dev);

      if (! tmp)
	grub_util_error ("Invalid root device `%s'", root_dev);
      
      tmp = xstrdup (tmp);
      free (root_dev);
      root_dev = tmp;
    }
  else
    {
      root_dev = grub_util_get_grub_dev (grub_guess_root_device (dir ? : DEFAULT_DIRECTORY));
      if (! root_dev)
	{
	  grub_util_info ("guessing the root device failed, because of `%s'",
			  grub_errmsg);
	  grub_util_error ("Cannot guess the root device. Specify the option ``--root-device''.");
	}
    }

#ifdef __linux__
  if (grub_util_lvm_isvolume (root_dev))
    {
      char *newprefix;
      must_embed = 1;

      newprefix = xmalloc (1 + strlen (root_dev) + 1 + strlen (prefix) + 1);
      sprintf (newprefix, "(%s)%s", root_dev, prefix);
      free (prefix);
      prefix = newprefix;
    }
    
  if (dest_dev[0] == 'm' && dest_dev[1] == 'd'
      && dest_dev[2] >= '0' && dest_dev[2] <= '9')
    {
      char **devicelist;
      char *raid_prefix;
      int i;

      raid_prefix = xmalloc (1 + strlen (dest_dev) + 1 + strlen (prefix) + 1);

      sprintf (raid_prefix, "(%s)%s", dest_dev, prefix);
      
      devicelist = grub_util_raid_getmembers (dest_dev);

      for (i = 0; devicelist[i]; i++)
	{
	  setup (raid_prefix,
		 dir ? : DEFAULT_DIRECTORY,
		 boot_file ? : DEFAULT_BOOT_FILE,
		 core_file ? : DEFAULT_CORE_FILE,
		 root_dev, grub_util_get_grub_dev (devicelist[i]), 1);
	}

      free (raid_prefix);
    }
  else
#endif
  /* Do the real work.  */
    setup (prefix,
	   dir ? : DEFAULT_DIRECTORY,
	   boot_file ? : DEFAULT_BOOT_FILE,
	   core_file ? : DEFAULT_CORE_FILE,
	   root_dev, dest_dev, must_embed);

  /* Free resources.  */
  grub_fini_all ();
  grub_util_biosdisk_fini ();
  
  free (boot_file);
  free (core_file);
  free (dir);
  free (dev_map);
  free (root_dev);
  free (prefix);
  free (dest_dev);
  
  return 0;
}
