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

#include <grub/machine/biosdisk.h>
#include <grub/machine/memory.h>
#include <grub/machine/kernel.h>
#include <grub/disk.h>
#include <grub/dl.h>
#include <grub/mm.h>
#include <grub/types.h>
#include <grub/misc.h>
#include <grub/err.h>
#include <grub/term.h>

static int cd_drive = 0;

static int
grub_biosdisk_get_drive (const char *name)
{
  unsigned long drive;

  if ((name[0] != 'f' && name[0] != 'h') || name[1] != 'd')
    goto fail;
    
  drive = grub_strtoul (name + 2, 0, 10);
  if (grub_errno != GRUB_ERR_NONE)
    goto fail;

  if (name[0] == 'h')
    drive += 0x80;
  
  return (int) drive ;

 fail:
  grub_error (GRUB_ERR_UNKNOWN_DEVICE, "not a biosdisk");
  return -1;
}

static int
grub_biosdisk_call_hook (int (*hook) (const char *name), int drive)
{
  char name[10];

    grub_sprintf (name, (drive & 0x80) ? "hd%d" : "fd%d", drive & (~0x80));
  return hook (name);
}

static int
grub_biosdisk_iterate (int (*hook) (const char *name))
{
  int drive;
  int num_floppies;

  /* For floppy disks, we can get the number safely.  */
  num_floppies = grub_biosdisk_get_num_floppies ();
  for (drive = 0; drive < num_floppies; drive++)
    if (grub_biosdisk_call_hook (hook, drive))
      return 1;
  
  /* For hard disks, attempt to read the MBR.  */
  for (drive = 0x80; drive < 0x90; drive++)
    {
      if (grub_biosdisk_rw_standard (0x02, drive, 0, 0, 1, 1,
				     GRUB_MEMORY_MACHINE_SCRATCH_SEG) != 0)
	{
	  grub_dprintf ("disk", "Read error when probing drive 0x%2x\n", drive);
	  break;
	}
      
      if (grub_biosdisk_call_hook (hook, drive))
	return 1;
    }

  if (cd_drive)
    {
      if (grub_biosdisk_call_hook (hook, cd_drive))
      return 1;
    }

  return 0;
}

static grub_err_t
grub_biosdisk_open (const char *name, grub_disk_t disk)
{
  grub_uint64_t total_sectors = 0;
  int drive;
  struct grub_biosdisk_data *data;

  drive = grub_biosdisk_get_drive (name);
  if (drive < 0)
    return grub_errno;

  disk->has_partitions = ((drive & 0x80) && (drive != cd_drive));
  disk->id = drive;
  
  data = (struct grub_biosdisk_data *) grub_malloc (sizeof (*data));
  if (! data)
    return grub_errno;
  
  data->drive = drive;
  data->flags = 0;

  if ((cd_drive) && (drive == cd_drive))
    {
      data->flags = GRUB_BIOSDISK_FLAG_LBA | GRUB_BIOSDISK_FLAG_CDROM;
      data->sectors = 32;
      total_sectors = 9000000;  /* TODO: get the correct size.  */
    }
  else if (drive & 0x80)
    {
      /* HDD */
      int version;
      
      version = grub_biosdisk_check_int13_extensions (drive);
      if (version)
	{
	  struct grub_biosdisk_drp *drp
	    = (struct grub_biosdisk_drp *) GRUB_MEMORY_MACHINE_SCRATCH_ADDR;

	  /* Clear out the DRP.  */
	  grub_memset (drp, 0, sizeof (*drp));
	  drp->size = sizeof (*drp);
	  if (! grub_biosdisk_get_diskinfo_int13_extensions (drive, drp))
	    {
	      data->flags = GRUB_BIOSDISK_FLAG_LBA;

	      if (drp->total_sectors)
		total_sectors = drp->total_sectors;
	      else
                /* Some buggy BIOSes doesn't return the total sectors
                   correctly but returns zero. So if it is zero, compute
                   it by C/H/S returned by the LBA BIOS call.  */
                total_sectors = drp->cylinders * drp->heads * drp->sectors;
	    }
	}
    }

  if (! (data->flags & GRUB_BIOSDISK_FLAG_CDROM))
    {
      if (grub_biosdisk_get_diskinfo_standard (drive,
					       &data->cylinders,
					       &data->heads,
					       &data->sectors) != 0)
        {
          grub_free (data);
          return grub_error (GRUB_ERR_BAD_DEVICE, "cannot get C/H/S values");
        }

      if (! total_sectors)
        total_sectors = data->cylinders * data->heads * data->sectors;
    }

  disk->total_sectors = total_sectors;
  disk->data = data;
  
  return GRUB_ERR_NONE;
}

static void
grub_biosdisk_close (grub_disk_t disk)
{
  grub_free (disk->data);
}

/* For readability.  */
#define GRUB_BIOSDISK_READ	0
#define GRUB_BIOSDISK_WRITE	1

#define GRUB_BIOSDISK_CDROM_RETRY_COUNT 3

static grub_err_t
grub_biosdisk_rw (int cmd, grub_disk_t disk,
		  grub_disk_addr_t sector, grub_size_t size,
		  unsigned segment)
{
  struct grub_biosdisk_data *data = disk->data;
  
  if (data->flags & GRUB_BIOSDISK_FLAG_LBA)
    {
      struct grub_biosdisk_dap *dap;
      
      dap = (struct grub_biosdisk_dap *) (GRUB_MEMORY_MACHINE_SCRATCH_ADDR
					  + (data->sectors
					     << GRUB_DISK_SECTOR_BITS));
      dap->length = sizeof (*dap);
      dap->reserved = 0;
      dap->blocks = size;
      dap->buffer = segment << 16;	/* The format SEGMENT:ADDRESS.  */
      dap->block = sector;

      if (data->flags & GRUB_BIOSDISK_FLAG_CDROM)
        {
	  int i;

	  if (cmd)
	    return grub_error (GRUB_ERR_WRITE_ERROR, "can\'t write to cdrom");

	  dap->blocks = (dap->blocks + 3) >> 2;
	  dap->block >>= 2;

	  for (i = 0; i < GRUB_BIOSDISK_CDROM_RETRY_COUNT; i++)
            if (! grub_biosdisk_rw_int13_extensions (0x42, data->drive, dap))
	      break;

	  if (i == GRUB_BIOSDISK_CDROM_RETRY_COUNT)
	    return grub_error (GRUB_ERR_READ_ERROR, "cdrom read error");
	}
      else
        if (grub_biosdisk_rw_int13_extensions (cmd + 0x42, data->drive, dap))
	  {
	    /* Fall back to the CHS mode.  */
	    data->flags &= ~GRUB_BIOSDISK_FLAG_LBA;
	    disk->total_sectors = data->cylinders * data->heads * data->sectors;
	    return grub_biosdisk_rw (cmd, disk, sector, size, segment);
	  }
    }
  else
    {
      unsigned coff, hoff, soff;
      unsigned head;
      unsigned real_sector = (unsigned) sector;
      
      /* It is impossible to reach over 2TB with the traditional
	 CHS access.  */
      if (sector > ~0UL)
	return grub_error (GRUB_ERR_OUT_OF_RANGE, "out of disk");

      soff = real_sector % data->sectors + 1;
      head = real_sector / data->sectors;
      hoff = head % data->heads;
      coff = head / data->heads;

      if (coff >= data->cylinders)
	return grub_error (GRUB_ERR_OUT_OF_RANGE, "out of disk");

      if (grub_biosdisk_rw_standard (cmd + 0x02, data->drive,
				     coff, hoff, soff, size, segment))
	{
	  switch (cmd)
	    {
	    case GRUB_BIOSDISK_READ:
	      return grub_error (GRUB_ERR_READ_ERROR, "biosdisk read error");
	    case GRUB_BIOSDISK_WRITE:
	      return grub_error (GRUB_ERR_WRITE_ERROR, "biosdisk write error");
	    }
	}
    }

  return GRUB_ERR_NONE;
}

/* Return the number of sectors which can be read safely at a time.  */
static grub_size_t
get_safe_sectors (grub_disk_addr_t sector, grub_uint32_t sectors)
{
  grub_size_t size;
  grub_uint32_t offset;

  /* OFFSET = SECTOR % SECTORS */
  grub_divmod64 (sector, sectors, &offset);

  size = sectors - offset;

  /* Limit the max to 0x7f because of Phoenix EDD.  */
  if (size > 0x7f)
    size = 0x7f;

  return size;
}

static grub_err_t
grub_biosdisk_read (grub_disk_t disk, grub_disk_addr_t sector,
		    grub_size_t size, char *buf)
{
  struct grub_biosdisk_data *data = disk->data;

  while (size)
    {
      grub_size_t len;

      len = get_safe_sectors (sector, data->sectors);
      if (len > size)
	len = size;

      if (grub_biosdisk_rw (GRUB_BIOSDISK_READ, disk, sector, len,
			    GRUB_MEMORY_MACHINE_SCRATCH_SEG))
	return grub_errno;

      grub_memcpy (buf, (void *) GRUB_MEMORY_MACHINE_SCRATCH_ADDR,
		   len << GRUB_DISK_SECTOR_BITS);
      buf += len << GRUB_DISK_SECTOR_BITS;
      sector += len;
      size -= len;
    }

  return grub_errno;
}

static grub_err_t
grub_biosdisk_write (grub_disk_t disk, grub_disk_addr_t sector,
		     grub_size_t size, const char *buf)
{
  struct grub_biosdisk_data *data = disk->data;

  while (size)
    {
      grub_size_t len;

      len = get_safe_sectors (sector, data->sectors);
      if (len > size)
	len = size;

      grub_memcpy ((void *) GRUB_MEMORY_MACHINE_SCRATCH_ADDR, buf,
		   len << GRUB_DISK_SECTOR_BITS);

      if (grub_biosdisk_rw (GRUB_BIOSDISK_WRITE, disk, sector, len,
			    GRUB_MEMORY_MACHINE_SCRATCH_SEG))
	return grub_errno;

      buf += len << GRUB_DISK_SECTOR_BITS;
      sector += len;
      size -= len;
    }

  return grub_errno;
}

static struct grub_disk_dev grub_biosdisk_dev =
  {
    .name = "biosdisk",
    .id = GRUB_DISK_DEVICE_BIOSDISK_ID,
    .iterate = grub_biosdisk_iterate,
    .open = grub_biosdisk_open,
    .close = grub_biosdisk_close,
    .read = grub_biosdisk_read,
    .write = grub_biosdisk_write,
    .next = 0
  };

static void
grub_disk_biosdisk_fini (void)
{
  grub_disk_dev_unregister (&grub_biosdisk_dev);
}

GRUB_MOD_INIT(biosdisk)
{
  struct grub_biosdisk_cdrp *cdrp
        = (struct grub_biosdisk_cdrp *) GRUB_MEMORY_MACHINE_SCRATCH_ADDR;

  if (grub_disk_firmware_is_tainted)
    {
      grub_printf ("Firmware is marked as tainted, refusing to initialize.\n");
      return;
    }
  grub_disk_firmware_fini = grub_disk_biosdisk_fini;

  grub_memset (cdrp, 0, sizeof (*cdrp));
  cdrp->size = sizeof (*cdrp);
  cdrp->media_type = 0xFF;
  if ((! grub_biosdisk_get_cdinfo_int13_extensions (grub_boot_drive, cdrp)) &&
      ((cdrp->media_type & GRUB_BIOSDISK_CDTYPE_MASK)
       == GRUB_BIOSDISK_CDTYPE_NO_EMUL))
    cd_drive = cdrp->drive_no;

  grub_disk_dev_register (&grub_biosdisk_dev);
}

GRUB_MOD_FINI(biosdisk)
{
  grub_disk_biosdisk_fini ();
}
