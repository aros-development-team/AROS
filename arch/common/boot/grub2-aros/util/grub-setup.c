/* grub-setup.c - make GRUB usable */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004,2005,2006,2007,2008,2009,2010  Free Software Foundation, Inc.
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
#include <grub/device.h>
#include <grub/disk.h>
#include <grub/file.h>
#include <grub/fs.h>
#include <grub/partition.h>
#include <grub/env.h>
#include <grub/emu/hostdisk.h>
#include <grub/machine/boot.h>
#include <grub/machine/kernel.h>
#include <grub/term.h>
#include <grub/i18n.h>
#include <grub/util/raid.h>
#include <grub/util/lvm.h>
#ifdef GRUB_MACHINE_IEEE1275
#include <grub/util/ofpath.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <assert.h>
#include <grub/emu/getroot.h>
#include "progname.h"
#include <grub/reed_solomon.h>
#include <grub/msdos_partition.h>

#define _GNU_SOURCE	1
#include <argp.h>

/* On SPARC this program fills in various fields inside of the 'boot' and 'core'
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

#ifdef GRUB_MACHINE_SPARC64
#define grub_target_to_host16(x)	grub_be_to_cpu16(x)
#define grub_target_to_host32(x)	grub_be_to_cpu32(x)
#define grub_target_to_host64(x)	grub_be_to_cpu64(x)
#define grub_host_to_target16(x)	grub_cpu_to_be16(x)
#define grub_host_to_target32(x)	grub_cpu_to_be32(x)
#define grub_host_to_target64(x)	grub_cpu_to_be64(x)
#elif defined (GRUB_MACHINE_PCBIOS)
#define grub_target_to_host16(x)	grub_le_to_cpu16(x)
#define grub_target_to_host32(x)	grub_le_to_cpu32(x)
#define grub_target_to_host64(x)	grub_le_to_cpu64(x)
#define grub_host_to_target16(x)	grub_cpu_to_le16(x)
#define grub_host_to_target32(x)	grub_cpu_to_le32(x)
#define grub_host_to_target64(x)	grub_cpu_to_le64(x)
#else
#error Complete this
#endif

static void
write_rootdev (char *core_img, grub_device_t root_dev,
	       char *boot_img, grub_uint64_t first_sector)
{
#ifdef GRUB_MACHINE_PCBIOS
  {
    grub_int32_t *install_dos_part, *install_bsd_part;
    grub_int32_t dos_part, bsd_part;
    grub_uint8_t *boot_drive;
    grub_disk_addr_t *kernel_sector;
    boot_drive = (grub_uint8_t *) (boot_img + GRUB_BOOT_MACHINE_BOOT_DRIVE);
    kernel_sector = (grub_disk_addr_t *) (boot_img
					  + GRUB_BOOT_MACHINE_KERNEL_SECTOR);


    install_dos_part = (grub_int32_t *) (core_img + GRUB_DISK_SECTOR_SIZE
					 + GRUB_KERNEL_MACHINE_INSTALL_DOS_PART);
    install_bsd_part = (grub_int32_t *) (core_img + GRUB_DISK_SECTOR_SIZE
					 + GRUB_KERNEL_MACHINE_INSTALL_BSD_PART);

    /* If we hardcoded drive as part of prefix, we don't want to
       override the current setting.  */
    if (*install_dos_part != -2)
      {
	/* Embed information about the installed location.  */
	if (root_dev->disk->partition)
	  {
	    if (root_dev->disk->partition->parent)
	      {
		if (root_dev->disk->partition->parent->parent)
		  grub_util_error ("Installing on doubly nested partitions is "
				   "not supported");
		dos_part = root_dev->disk->partition->parent->number;
		bsd_part = root_dev->disk->partition->number;
	      }
	    else
	      {
		dos_part = root_dev->disk->partition->number;
		bsd_part = -1;
	      }
	  }
	else
	  dos_part = bsd_part = -1;
      }
    else
      {
	dos_part = grub_le_to_cpu32 (*install_dos_part);
	bsd_part = grub_le_to_cpu32 (*install_bsd_part);
      }

    grub_util_info ("dos partition is %d, bsd partition is %d",
		    dos_part, bsd_part);

    *install_dos_part = grub_cpu_to_le32 (dos_part);
    *install_bsd_part = grub_cpu_to_le32 (bsd_part);

    /* FIXME: can this be skipped?  */
    *boot_drive = 0xFF;

    *kernel_sector = grub_cpu_to_le64 (first_sector);
  }
#endif
#ifdef GRUB_MACHINE_IEEE1275
  {
    grub_disk_addr_t *kernel_byte;
    kernel_byte = (grub_disk_addr_t *) (boot_img
					+ GRUB_BOOT_AOUT_HEADER_SIZE
					+ GRUB_BOOT_MACHINE_KERNEL_BYTE);
    *kernel_byte = grub_cpu_to_be64 (first_sector << GRUB_DISK_SECTOR_BITS);
  }
#endif
}

#ifdef GRUB_MACHINE_IEEE1275
#define BOOT_SECTOR 1
#else
#define BOOT_SECTOR 0
#endif

static void
setup (const char *dir,
       const char *boot_file, const char *core_file,
       const char *root, const char *dest, int must_embed, int force,
       int fs_probe, int allow_floppy)
{
  char *boot_path, *core_path, *core_path_dev, *core_path_dev_full;
  char *boot_img, *core_img;
  size_t boot_size, core_size;
  grub_uint16_t core_sectors;
  grub_device_t root_dev, dest_dev;
  struct grub_boot_blocklist *first_block, *block;
  char *tmp_img;
  int i;
  grub_disk_addr_t first_sector;
#ifdef GRUB_MACHINE_PCBIOS
  grub_uint16_t current_segment
    = GRUB_BOOT_MACHINE_KERNEL_SEG + (GRUB_DISK_SECTOR_SIZE >> 4);
#endif
  grub_uint16_t last_length = GRUB_DISK_SECTOR_SIZE;
  grub_file_t file;
  FILE *fp;

  auto void NESTED_FUNC_ATTR save_first_sector (grub_disk_addr_t sector,
						unsigned offset,
						unsigned length);
  auto void NESTED_FUNC_ATTR save_blocklists (grub_disk_addr_t sector,
					      unsigned offset,
					      unsigned length);

  void NESTED_FUNC_ATTR save_first_sector (grub_disk_addr_t sector,
					   unsigned offset,
					   unsigned length)
    {
      grub_util_info ("the first sector is <%llu,%u,%u>",
		      sector, offset, length);

      if (offset != 0 || length != GRUB_DISK_SECTOR_SIZE)
	grub_util_error (_("the first sector of the core file is not sector-aligned"));

      first_sector = sector;
    }

  void NESTED_FUNC_ATTR save_blocklists (grub_disk_addr_t sector,
					 unsigned offset,
					 unsigned length)
    {
      struct grub_boot_blocklist *prev = block + 1;

      grub_util_info ("saving <%llu,%u,%u>", sector, offset, length);

      if (offset != 0 || last_length != GRUB_DISK_SECTOR_SIZE)
	grub_util_error (_("non-sector-aligned data is found in the core file"));

      if (block != first_block
	  && (grub_target_to_host64 (prev->start)
	      + grub_target_to_host16 (prev->len)) == sector)
	prev->len = grub_host_to_target16 (grub_target_to_host16 (prev->len) + 1);
      else
	{
	  block->start = grub_host_to_target64 (sector);
	  block->len = grub_host_to_target16 (1);
#ifdef GRUB_MACHINE_PCBIOS
	  block->segment = grub_host_to_target16 (current_segment);
#endif

	  block--;
	  if (block->len)
	    grub_util_error (_("the sectors of the core file are too fragmented"));
	}

      last_length = length;
#ifdef GRUB_MACHINE_PCBIOS
      current_segment += GRUB_DISK_SECTOR_SIZE >> 4;
#endif
    }

  /* Read the boot image by the OS service.  */
  boot_path = grub_util_get_path (dir, boot_file);
  boot_size = grub_util_get_image_size (boot_path);
  if (boot_size != GRUB_DISK_SECTOR_SIZE)
    grub_util_error (_("the size of `%s' is not %u"),
		     boot_path, GRUB_DISK_SECTOR_SIZE);
  boot_img = grub_util_read_image (boot_path);
  free (boot_path);

  core_path = grub_util_get_path (dir, core_file);
  core_size = grub_util_get_image_size (core_path);
  core_sectors = ((core_size + GRUB_DISK_SECTOR_SIZE - 1)
		  >> GRUB_DISK_SECTOR_BITS);
  if (core_size < GRUB_DISK_SECTOR_SIZE)
    grub_util_error (_("the size of `%s' is too small"), core_path);
#ifdef GRUB_MACHINE_PCBIOS
  if (core_size > 0xFFFF * GRUB_DISK_SECTOR_SIZE)
    grub_util_error (_("the size of `%s' is too large"), core_path);
#endif

  core_img = grub_util_read_image (core_path);

  /* Have FIRST_BLOCK to point to the first blocklist.  */
  first_block = (struct grub_boot_blocklist *) (core_img
						+ GRUB_DISK_SECTOR_SIZE
						- sizeof (*block));
  grub_util_info ("root is `%s', dest is `%s'", root, dest);

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

#ifdef GRUB_MACHINE_PCBIOS
  /* Read the original sector from the disk.  */
  tmp_img = xmalloc (GRUB_DISK_SECTOR_SIZE);
  if (grub_disk_read (dest_dev->disk, 0, 0, GRUB_DISK_SECTOR_SIZE, tmp_img))
    grub_util_error ("%s", grub_errmsg);
#endif

#ifdef GRUB_MACHINE_PCBIOS
  {
    grub_uint16_t *boot_drive_check;
    boot_drive_check = (grub_uint16_t *) (boot_img
					  + GRUB_BOOT_MACHINE_DRIVE_CHECK);
    /* Copy the possible DOS BPB.  */
    memcpy (boot_img + GRUB_BOOT_MACHINE_BPB_START,
	    tmp_img + GRUB_BOOT_MACHINE_BPB_START,
	    GRUB_BOOT_MACHINE_BPB_END - GRUB_BOOT_MACHINE_BPB_START);

    /* If DEST_DRIVE is a hard disk, enable the workaround, which is
       for buggy BIOSes which don't pass boot drive correctly. Instead,
       they pass 0x00 or 0x01 even when booted from 0x80.  */
    if (!allow_floppy && !grub_util_biosdisk_is_floppy (dest_dev->disk))
      /* Replace the jmp (2 bytes) with double nop's.  */
      *boot_drive_check = 0x9090;
  }
#endif

#ifdef GRUB_MACHINE_PCBIOS
  {
    grub_partition_map_t dest_partmap = NULL;
    grub_partition_t container = dest_dev->disk->partition;
    int multiple_partmaps = 0;
    grub_err_t err;
    grub_disk_addr_t *sectors;
    int i;
    grub_fs_t fs;
    unsigned int nsec;

    /* Unlike root_dev, with dest_dev we're interested in the partition map even
       if dest_dev itself is a whole disk.  */
    auto int NESTED_FUNC_ATTR identify_partmap (grub_disk_t disk,
						const grub_partition_t p);
    int NESTED_FUNC_ATTR identify_partmap (grub_disk_t disk __attribute__ ((unused)),
					   const grub_partition_t p)
    {
      if (p->parent != container)
	return 0;
      /* NetBSD and OpenBSD subpartitions have metadata inside a partition,
	 so they are safe to ignore.
       */
      if (grub_strcmp (p->partmap->name, "netbsd") == 0
	  || grub_strcmp (p->partmap->name, "openbsd") == 0)
	return 0;
      if (dest_partmap == NULL)
	{
	  dest_partmap = p->partmap;
	  return 0;
	}
      if (dest_partmap == p->partmap)
	return 0;
      multiple_partmaps = 1;
      return 1;
    }

    grub_partition_iterate (dest_dev->disk, identify_partmap);

    if (container && grub_strcmp (container->partmap->name, "msdos") == 0
	&& dest_partmap
	&& (container->msdostype == GRUB_PC_PARTITION_TYPE_NETBSD
	    || container->msdostype == GRUB_PC_PARTITION_TYPE_OPENBSD))
      {
	grub_util_warn (_("Attempting to install GRUB to a disk with multiple partition labels or both partition label and filesystem.  This is not supported yet."));
	goto unable_to_embed;
      }

    fs = grub_fs_probe (dest_dev);
    if (!fs)
      grub_errno = GRUB_ERR_NONE;

#ifdef GRUB_MACHINE_PCBIOS
    if (fs_probe)
      {
	if (!fs && !dest_partmap)
	  grub_util_error (_("unable to identify a filesystem in %s; safety check can't be performed"),
			   dest_dev->disk->name);
	if (fs && !fs->reserved_first_sector)
	  grub_util_error (_("%s appears to contain a %s filesystem which isn't known to "
			     "reserve space for DOS-style boot.  Installing GRUB there could "
			     "result in FILESYSTEM DESTRUCTION if valuable data is overwritten "
			     "by grub-setup (--skip-fs-probe disables this "
			     "check, use at your own risk)"), dest_dev->disk->name, fs->name);

	if (dest_partmap && strcmp (dest_partmap->name, "msdos") != 0
	    && strcmp (dest_partmap->name, "gpt") != 0
	    && strcmp (dest_partmap->name, "bsd") != 0
	    && strcmp (dest_partmap->name, "netbsd") != 0
	    && strcmp (dest_partmap->name, "openbsd") != 0
	    && strcmp (dest_partmap->name, "sunpc") != 0)
	  grub_util_error (_("%s appears to contain a %s partition map which isn't known to "
			     "reserve space for DOS-style boot.  Installing GRUB there could "
			     "result in FILESYSTEM DESTRUCTION if valuable data is overwritten "
			     "by grub-setup (--skip-fs-probe disables this "
			     "check, use at your own risk)"), dest_dev->disk->name, dest_partmap->name);
      }
#endif

    if (! dest_partmap)
      {
	grub_util_warn (_("Attempting to install GRUB to a partitionless disk or to a partition.  This is a BAD idea."));
	goto unable_to_embed;
      }
    if (multiple_partmaps || fs)
      {
	grub_util_warn (_("Attempting to install GRUB to a disk with multiple partition labels or both partition label and filesystem.  This is not supported yet."));
	goto unable_to_embed;
      }

    /* Copy the partition table.  */
    if (dest_partmap)
      memcpy (boot_img + GRUB_BOOT_MACHINE_WINDOWS_NT_MAGIC,
	      tmp_img + GRUB_BOOT_MACHINE_WINDOWS_NT_MAGIC,
	      GRUB_BOOT_MACHINE_PART_END - GRUB_BOOT_MACHINE_WINDOWS_NT_MAGIC);

    free (tmp_img);
    
    if (!dest_partmap->embed)
      {
	grub_util_warn ("Partition style '%s' doesn't support embeding",
			dest_partmap->name);
	goto unable_to_embed;
      }

    nsec = core_sectors;
    err = dest_partmap->embed (dest_dev->disk, &nsec,
			       GRUB_EMBED_PCBIOS, &sectors);
    if (nsec > 2 * core_sectors)
      nsec = 2 * core_sectors;
    
    if (err)
      {
	grub_util_warn ("%s", grub_errmsg);
	grub_errno = GRUB_ERR_NONE;
	goto unable_to_embed;
      }

    /* Clean out the blocklists.  */
    block = first_block;
    while (block->len)
      {
	grub_memset (block, 0, sizeof (block));
      
	block--;

	if ((char *) block <= core_img)
	  grub_util_error ("No terminator in the core image");
      }

    save_first_sector (sectors[0] + grub_partition_get_start (container),
		       0, GRUB_DISK_SECTOR_SIZE);

    block = first_block;
    for (i = 1; i < nsec; i++)
      save_blocklists (sectors[i] + grub_partition_get_start (container),
		       0, GRUB_DISK_SECTOR_SIZE);

    write_rootdev (core_img, root_dev, boot_img, first_sector);

    core_img = realloc (core_img, nsec * GRUB_DISK_SECTOR_SIZE);
    first_block = (struct grub_boot_blocklist *) (core_img
						  + GRUB_DISK_SECTOR_SIZE
						  - sizeof (*block));

    *(grub_uint32_t *) (core_img + GRUB_DISK_SECTOR_SIZE
			+ GRUB_KERNEL_I386_PC_REED_SOLOMON_REDUNDANCY)
      = grub_host_to_target32 (nsec * GRUB_DISK_SECTOR_SIZE - core_size);

    grub_reed_solomon_add_redundancy (core_img + GRUB_KERNEL_I386_PC_NO_REED_SOLOMON_PART + GRUB_DISK_SECTOR_SIZE,
				      core_size - GRUB_KERNEL_I386_PC_NO_REED_SOLOMON_PART - GRUB_DISK_SECTOR_SIZE,
				      nsec * GRUB_DISK_SECTOR_SIZE
				      - core_size);

    /* Make sure that the second blocklist is a terminator.  */
    block = first_block - 1;
    block->start = 0;
    block->len = 0;
    block->segment = 0;

    /* Write the core image onto the disk.  */
    for (i = 0; i < nsec; i++)
      grub_disk_write (dest_dev->disk, sectors[i], 0,
		       GRUB_DISK_SECTOR_SIZE,
		       core_img + i * GRUB_DISK_SECTOR_SIZE);

    grub_free (sectors);

    goto finish;
  }
#endif

unable_to_embed:

  if (must_embed)
    grub_util_error (_("embedding is not possible, but this is required when "
		       "the root device is on a RAID array or LVM volume"));

#ifdef GRUB_MACHINE_PCBIOS
  if (dest_dev->disk->id != root_dev->disk->id)
    grub_util_error (_("embedding is not possible, but this is required for "
		       "cross-disk install"));
#endif

  grub_util_warn (_("Embedding is not possible.  GRUB can only be installed in this "
		    "setup by using blocklists.  However, blocklists are UNRELIABLE and "
		    "their use is discouraged."));
  if (! force)
    grub_util_error (_("will not proceed with blocklists"));

  /* The core image must be put on a filesystem unfortunately.  */
  grub_util_info ("will leave the core image on the filesystem");

  /* Make sure that GRUB reads the identical image as the OS.  */
  tmp_img = xmalloc (core_size);
  core_path_dev_full = grub_util_get_path (dir, core_file);
  core_path_dev = grub_make_system_path_relative_to_its_root (core_path_dev_full);
  free (core_path_dev_full);

  /* It is a Good Thing to sync two times.  */
  sync ();
  sync ();

#define MAX_TRIES	5

  for (i = 0; i < MAX_TRIES; i++)
    {
      grub_util_info ((i == 0) ? _("attempting to read the core image `%s' from GRUB")
		      : _("attempting to read the core image `%s' from GRUB again"),
		      core_path_dev);

      grub_disk_cache_invalidate_all ();

      grub_file_filter_disable_compression ();
      file = grub_file_open (core_path_dev);
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
    grub_util_error (_("cannot read `%s' correctly"), core_path_dev);

  /* Clean out the blocklists.  */
  block = first_block;
  while (block->len)
    {
      block->start = 0;
      block->len = 0;
#ifdef GRUB_MACHINE_PCBIOS
      block->segment = 0;
#endif

      block--;

      if ((char *) block <= core_img)
	grub_util_error (_("no terminator in the core image"));
    }

  /* Now read the core image to determine where the sectors are.  */
  grub_file_filter_disable_compression ();
  file = grub_file_open (core_path_dev);
  if (! file)
    grub_util_error ("%s", grub_errmsg);

  file->read_hook = save_first_sector;
  if (grub_file_read (file, tmp_img, GRUB_DISK_SECTOR_SIZE)
      != GRUB_DISK_SECTOR_SIZE)
    grub_util_error (_("failed to read the first sector of the core image"));

  block = first_block;
  file->read_hook = save_blocklists;
  if (grub_file_read (file, tmp_img, core_size - GRUB_DISK_SECTOR_SIZE)
      != (grub_ssize_t) core_size - GRUB_DISK_SECTOR_SIZE)
    grub_util_error (_("failed to read the rest sectors of the core image"));

#ifdef GRUB_MACHINE_IEEE1275
  {
    char *boot_devpath;
    boot_devpath = (char *) (boot_img
			     + GRUB_BOOT_AOUT_HEADER_SIZE
			     + GRUB_BOOT_MACHINE_BOOT_DEVPATH);
    if (file->device->disk->id != dest_dev->disk->id)
      {
	const char *dest_ofpath;
	dest_ofpath
	  = grub_util_devname_to_ofpath (grub_util_biosdisk_get_osdev (file->device->disk));
	grub_util_info ("dest_ofpath is `%s'", dest_ofpath);
	strncpy (boot_devpath, dest_ofpath, GRUB_BOOT_MACHINE_BOOT_DEVPATH_END
		 - GRUB_BOOT_MACHINE_BOOT_DEVPATH - 1);
	boot_devpath[GRUB_BOOT_MACHINE_BOOT_DEVPATH_END
		   - GRUB_BOOT_MACHINE_BOOT_DEVPATH - 1] = 0;
      }
    else
      {
	grub_util_info ("non cross-disk install");
	memset (boot_devpath, 0, GRUB_BOOT_MACHINE_BOOT_DEVPATH_END
		- GRUB_BOOT_MACHINE_BOOT_DEVPATH);
      }
    grub_util_info ("boot device path %s", boot_devpath);
  }
#endif

  grub_file_close (file);

  free (core_path_dev);
  free (tmp_img);

  write_rootdev (core_img, root_dev, boot_img, first_sector);

  /* Write the first two sectors of the core image onto the disk.  */
  grub_util_info ("opening the core image `%s'", core_path);
  fp = fopen (core_path, "r+b");
  if (! fp)
    grub_util_error (_("cannot open `%s'"), core_path);

  grub_util_write_image (core_img, GRUB_DISK_SECTOR_SIZE * 2, fp);
  fclose (fp);

 finish:

  /* Write the boot image onto the disk.  */
  if (grub_disk_write (dest_dev->disk, BOOT_SECTOR,
		       0, GRUB_DISK_SECTOR_SIZE, boot_img))
    grub_util_error ("%s", grub_errmsg);


  /* Sync is a Good Thing.  */
  sync ();

  free (core_path);
  free (core_img);
  free (boot_img);
  grub_device_close (dest_dev);
  grub_device_close (root_dev);
}

static struct argp_option options[] = {
  {"boot-image",  'b', N_("FILE"), 0,
   N_("Use FILE as the boot image [default=%s]"), 0},
  {"core-image",  'c', N_("FILE"), 0,
   N_("Use FILE as the core image [default=%s]"), 0},
  {"directory",   'd', N_("DIR"),  0,
   N_("Use GRUB files in the directory DIR [default=%s]"), 0},
  {"device-map",  'm', N_("FILE"), 0,
   N_("Use FILE as the device map [default=%s]"), 0},
  {"root-device", 'r', N_("DEV"),  0,
   N_("Use DEV as the root device [default=guessed]"), 0},
  {"force",       'f', 0,      0,
   N_("Install even if problems are detected"), 0},
  {"skip-fs-probe",'s',0,      0,
   N_("Do not probe for filesystems in DEVICE"), 0},
  {"verbose",     'v', 0,      0,
   N_("Print verbose messages."), 0},
  {"allow-floppy", 'a', 0,      0,
   N_("Make the drive also bootable as floppy (default for fdX devices). May break on some BIOSes."), 0},

  { 0, 0, 0, 0, 0, 0 }
};

static char *
help_filter (int key, const char *text, void *input __attribute__ ((unused)))
{
  switch (key)
    {
      case 'b':
        return xasprintf (text, DEFAULT_BOOT_FILE);

      case 'c':
        return xasprintf (text, DEFAULT_CORE_FILE);

      case 'd':
        return xasprintf (text, DEFAULT_DIRECTORY);

      case 'm':
        return xasprintf (text, DEFAULT_DEVICE_MAP);

      default:
        return (char *) text;
    }
}

struct arguments
{
  char *boot_file;
  char *core_file;
  char *dir;
  char *dev_map;
  char *root_dev;
  int  force;
  int  fs_probe;
  int allow_floppy;
  char *device;
};

/* Print the version information.  */
static void
print_version (FILE *stream, struct argp_state *state)
{
  fprintf (stream, "%s (%s) %s\n", program_name, PACKAGE_NAME, PACKAGE_VERSION);
}
void (*argp_program_version_hook) (FILE *, struct argp_state *) = print_version;

/* Set the bug report address */
const char *argp_program_bug_address = "<"PACKAGE_BUGREPORT">";

static error_t
argp_parser (int key, char *arg, struct argp_state *state)
{
  /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
  struct arguments *arguments = state->input;

  char *p;

  switch (key)
    {
      case 'a':
        arguments->allow_floppy = 1;
        break;

      case 'b':
        if (arguments->boot_file)
          free (arguments->boot_file);

        arguments->boot_file = xstrdup (arg);
        break;

      case 'c':
        if (arguments->core_file)
          free (arguments->core_file);

        arguments->core_file = xstrdup (arg);
        break;

      case 'd':
        if (arguments->dir)
          free (arguments->dir);

        arguments->dir = xstrdup (arg);
        break;

      case 'm':
        if (arguments->dev_map)
          free (arguments->dev_map);

        arguments->dev_map = xstrdup (arg);
        break;

      case 'r':
        if (arguments->root_dev)
          free (arguments->root_dev);

        arguments->root_dev = xstrdup (arg);
        break;

      case 'f':
        arguments->force = 1;
        break;

      case 's':
        arguments->fs_probe = 0;
        break;

      case 'v':
        verbosity++;
        break;

      case ARGP_KEY_ARG:
        if (state->arg_num == 0)
          arguments->device = xstrdup(arg);
        else
          {
            /* Too many arguments. */
            fprintf (stderr, _("Unknown extra argument `%s'.\n"), arg);
            argp_usage (state);
          }
        break;

      case ARGP_KEY_NO_ARGS:
          fprintf (stderr, "%s", _("No device is specified.\n"));
          argp_usage (state);
          break;

      default:
        return ARGP_ERR_UNKNOWN;
    }

  return 0;
}

static struct argp argp = {
  options, argp_parser, N_("DEVICE"),
  "\n"N_("\
Set up images to boot from DEVICE.\n\
\n\
You should not normally run this program directly.  Use grub-install instead.")
"\v"N_("\
DEVICE must be an OS device (e.g. /dev/sda)."),
  NULL, help_filter, NULL
};

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
  char *root_dev = NULL;
  char *dest_dev = NULL;
  int must_embed = 0;
  struct arguments arguments;

  set_program_name (argv[0]);

  grub_util_init_nls ();

  /* Default option values. */
  memset (&arguments, 0, sizeof (struct arguments));
  arguments.fs_probe  = 1;

  /* Parse our arguments */
  if (argp_parse (&argp, argc, argv, 0, 0, &arguments) != 0)
    {
      fprintf (stderr, "%s", _("Error in parsing command line arguments\n"));
      exit(1);
    }

#ifdef GRUB_MACHINE_IEEE1275
  arguments.force = 1;
#endif

  if (verbosity > 1)
    grub_env_set ("debug", "all");

  /* Initialize the emulated biosdisk driver.  */
  grub_util_biosdisk_init (arguments.dev_map ? : DEFAULT_DEVICE_MAP);

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

  dest_dev = get_device_name (arguments.device);
  if (! dest_dev)
    {
      /* Possibly, the user specified an OS device file.  */
      dest_dev = grub_util_get_grub_dev (arguments.device);
      if (! dest_dev)
        {
          char *program = xstrdup(program_name);
          fprintf (stderr, _("Invalid device `%s'.\n"), arguments.device);
          argp_help (&argp, stderr, ARGP_HELP_STD_USAGE, program);
          free(program);
          exit(1);
        }
      grub_util_info ("transformed OS device `%s' into GRUB device `%s'",
                      arguments.device, dest_dev);
    }
  else
    {
      /* For simplicity.  */
      dest_dev = xstrdup (dest_dev);
      grub_util_info ("Using `%s' as GRUB device", dest_dev);
    }

  if (arguments.root_dev)
    {
      root_dev = get_device_name (arguments.root_dev);

      if (! root_dev)
        grub_util_error (_("invalid root device `%s'"), arguments.root_dev);

      root_dev = xstrdup (root_dev);
    }
  else
    {
      char *root_device =
        grub_guess_root_device (arguments.dir ? : DEFAULT_DIRECTORY);

      root_dev = grub_util_get_grub_dev (root_device);
      if (! root_dev)
	{
	  grub_util_info ("guessing the root device failed, because of `%s'",
			  grub_errmsg);
          grub_util_error (_("cannot guess the root device. Specify the option "
                             "`--root-device'"));
	}
      grub_util_info ("guessed root device `%s' and root_dev `%s' from "
                      "dir `%s'", root_device, root_dev,
                      arguments.dir ? : DEFAULT_DIRECTORY);
    }

#ifdef __linux__
  if (grub_util_lvm_isvolume (root_dev))
    must_embed = 1;

  if (root_dev[0] == 'm' && root_dev[1] == 'd'
      && ((root_dev[2] >= '0' && root_dev[2] <= '9') || root_dev[2] == '/'))
    {
      /* FIXME: we can avoid this on RAID1.  */
      must_embed = 1;
    }

  if (dest_dev[0] == 'm' && dest_dev[1] == 'd'
      && ((dest_dev[2] >= '0' && dest_dev[2] <= '9') || dest_dev[2] == '/'))
    {
      char **devicelist;
      int i;

      devicelist = grub_util_raid_getmembers (dest_dev);

      for (i = 0; devicelist[i]; i++)
        {
          setup (arguments.dir ? : DEFAULT_DIRECTORY,
                 arguments.boot_file ? : DEFAULT_BOOT_FILE,
                 arguments.core_file ? : DEFAULT_CORE_FILE,
                 root_dev, grub_util_get_grub_dev (devicelist[i]), 1,
                 arguments.force, arguments.fs_probe,
		 arguments.allow_floppy);
        }
    }
  else
#endif
    /* Do the real work.  */
    setup (arguments.dir ? : DEFAULT_DIRECTORY,
           arguments.boot_file ? : DEFAULT_BOOT_FILE,
           arguments.core_file ? : DEFAULT_CORE_FILE,
           root_dev, dest_dev, must_embed, arguments.force,
	   arguments.fs_probe, arguments.allow_floppy);

  /* Free resources.  */
  grub_fini_all ();
  grub_util_biosdisk_fini ();

  free (arguments.boot_file);
  free (arguments.core_file);
  free (arguments.dir);
  free (arguments.root_dev);
  free (arguments.dev_map);
  free (arguments.device);
  free (root_dev);
  free (dest_dev);

  return 0;
}
