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

#include <grub/machine/biosdisk.h>
#include <grub/machine/kernel.h>
#include <grub/machine/memory.h>
#include <grub/machine/int.h>
#include <grub/disk.h>
#include <grub/dl.h>
#include <grub/mm.h>
#include <grub/types.h>
#include <grub/misc.h>
#include <grub/err.h>
#include <grub/term.h>
#include <grub/i18n.h>

GRUB_MOD_LICENSE ("GPLv3+");

static int cd_drive = 0;
static int grub_biosdisk_rw_int13_extensions (int ah, int drive, void *dap);

static int grub_biosdisk_get_num_floppies (void)
{
  struct grub_bios_int_registers regs;
  int drive;

  /* reset the disk system first */
  regs.eax = 0;
  regs.edx = 0;
  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;

  grub_bios_interrupt (0x13, &regs);

  for (drive = 0; drive < 2; drive++)
    {
      regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT | GRUB_CPU_INT_FLAGS_CARRY;
      regs.edx = drive;

      /* call GET DISK TYPE */
      regs.eax = 0x1500;
      grub_bios_interrupt (0x13, &regs);
      if (regs.flags & GRUB_CPU_INT_FLAGS_CARRY)
	break;

      /* check if this drive exists */
      if (!(regs.eax & 0x300))
	break;
    }

  return drive;
}

/*
 *   Call IBM/MS INT13 Extensions (int 13 %ah=AH) for DRIVE. DAP
 *   is passed for disk address packet. If an error occurs, return
 *   non-zero, otherwise zero.
 */

static int 
grub_biosdisk_rw_int13_extensions (int ah, int drive, void *dap)
{
  struct grub_bios_int_registers regs;
  regs.eax = ah << 8;
  /* compute the address of disk_address_packet */
  regs.ds = (((grub_addr_t) dap) & 0xffff0000) >> 4;
  regs.esi = (((grub_addr_t) dap) & 0xffff);
  regs.edx = drive;
  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;

  grub_bios_interrupt (0x13, &regs);
  return (regs.eax >> 8) & 0xff;
}

/*
 *   Call standard and old INT13 (int 13 %ah=AH) for DRIVE. Read/write
 *   NSEC sectors from COFF/HOFF/SOFF into SEGMENT. If an error occurs,
 *   return non-zero, otherwise zero.
 */
static int 
grub_biosdisk_rw_standard (int ah, int drive, int coff, int hoff,
			   int soff, int nsec, int segment)
{
  int ret, i;

  /* Try 3 times.  */
  for (i = 0; i < 3; i++)
    {
      struct grub_bios_int_registers regs;

      /* set up CHS information */
      /* set %ch to low eight bits of cylinder */
      regs.ecx = (coff << 8) & 0xff00;
      /* set bits 6-7 of %cl to high two bits of cylinder */
      regs.ecx |= (coff >> 2) & 0xc0;
      /* set bits 0-5 of %cl to sector */
      regs.ecx |= soff & 0x3f;

      /* set %dh to head and %dl to drive */  
      regs.edx = (drive & 0xff) | ((hoff << 8) & 0xff00);
      /* set %ah to AH */
      regs.eax = (ah << 8) & 0xff00;
      /* set %al to NSEC */
      regs.eax |= nsec & 0xff;

      regs.ebx = 0;
      regs.es = segment;

      regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;

      grub_bios_interrupt (0x13, &regs);
      /* check if successful */
      if (!(regs.flags & GRUB_CPU_INT_FLAGS_CARRY))
	return 0;

      /* save return value */
      ret = regs.eax >> 8;

      /* if fail, reset the disk system */
      regs.eax = 0;
      regs.edx = (drive & 0xff);
      regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;
      grub_bios_interrupt (0x13, &regs);
    }
  return ret;
}

/*
 *   Check if LBA is supported for DRIVE. If it is supported, then return
 *   the major version of extensions, otherwise zero.
 */
static int
grub_biosdisk_check_int13_extensions (int drive)
{
  struct grub_bios_int_registers regs;

  regs.edx = drive & 0xff;
  regs.eax = 0x4100;
  regs.ebx = 0x55aa;
  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;
  grub_bios_interrupt (0x13, &regs);
  
  if (regs.flags & GRUB_CPU_INT_FLAGS_CARRY)
    return 0;

  if ((regs.ebx & 0xffff) != 0xaa55)
    return 0;

  /* check if AH=0x42 is supported */
  if (!(regs.ecx & 1))
    return 0;

  return (regs.eax >> 8) & 0xff;
}

/*
 *   Return the geometry of DRIVE in CYLINDERS, HEADS and SECTORS. If an
 *   error occurs, then return non-zero, otherwise zero.
 */
static int 
grub_biosdisk_get_diskinfo_standard (int drive,
				     unsigned long *cylinders,
				     unsigned long *heads,
				     unsigned long *sectors)
{
  struct grub_bios_int_registers regs;

  regs.eax = 0x0800;
  regs.edx = drive & 0xff;

  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;
  grub_bios_interrupt (0x13, &regs);

  /* Check if unsuccessful. Ignore return value if carry isn't set to 
     workaround some buggy BIOSes. */
  if ((regs.flags & GRUB_CPU_INT_FLAGS_CARRY) && ((regs.eax & 0xff00) != 0))
    return (regs.eax & 0xff00) >> 8;

  /* bogus BIOSes may not return an error number */  
  /* 0 sectors means no disk */
  if (!(regs.ecx & 0x3f))
    /* XXX 0x60 is one of the unused error numbers */
    return 0x60;

  /* the number of heads is counted from zero */
  *heads = ((regs.edx >> 8) & 0xff) + 1;
  *cylinders = (((regs.ecx >> 8) & 0xff) | ((regs.ecx << 2) & 0x0300)) + 1;
  *sectors = regs.ecx & 0x3f;
  return 0;
}

static int
grub_biosdisk_get_diskinfo_real (int drive, void *drp, grub_uint16_t ax)
{
  struct grub_bios_int_registers regs;

  regs.eax = ax;

  /* compute the address of drive parameters */
  regs.esi = ((grub_addr_t) drp) & 0xf;
  regs.ds = ((grub_addr_t) drp) >> 4;
  regs.edx = drive & 0xff;

  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;
  grub_bios_interrupt (0x13, &regs);

  /* Check if unsuccessful. Ignore return value if carry isn't set to 
     workaround some buggy BIOSes. */
  if ((regs.flags & GRUB_CPU_INT_FLAGS_CARRY) && ((regs.eax & 0xff00) != 0))
    return (regs.eax & 0xff00) >> 8;

  return 0;
}

/*
 *   Return the cdrom information of DRIVE in CDRP. If an error occurs,
 *   then return non-zero, otherwise zero.
 */
static int
grub_biosdisk_get_cdinfo_int13_extensions (int drive, void *cdrp)
{
  return grub_biosdisk_get_diskinfo_real (drive, cdrp, 0x4b01);
}

/*
 *   Return the geometry of DRIVE in a drive parameters, DRP. If an error
 *   occurs, then return non-zero, otherwise zero.
 */
static int
grub_biosdisk_get_diskinfo_int13_extensions (int drive, void *drp)
{
  return grub_biosdisk_get_diskinfo_real (drive, drp, 0x4800);
}

static int
grub_biosdisk_get_drive (const char *name)
{
  unsigned long drive;

  if (name[0] == 'c' && name[1] == 'd' && name[2] == 0 && cd_drive)
    return cd_drive;

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

  if (cd_drive && drive == cd_drive)
    return hook ("cd");

  grub_snprintf (name, sizeof (name),
		 (drive & 0x80) ? "hd%d" : "fd%d", drive & (~0x80));
  return hook (name);
}

static int
grub_biosdisk_iterate (int (*hook) (const char *name),
		       grub_disk_pull_t pull __attribute__ ((unused)))
{
  int num_floppies;
  int drive;

  /* For hard disks, attempt to read the MBR.  */
  switch (pull)
    {
    case GRUB_DISK_PULL_NONE:
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
      return 0;

    case GRUB_DISK_PULL_REMOVABLE:
      if (cd_drive)
	{
	  if (grub_biosdisk_call_hook (hook, cd_drive))
	    return 1;
	}

      /* For floppy disks, we can get the number safely.  */
      num_floppies = grub_biosdisk_get_num_floppies ();
      for (drive = 0; drive < num_floppies; drive++)
	if (grub_biosdisk_call_hook (hook, drive))
	  return 1;
      return 0;
    default:
      return 0;
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

  disk->id = drive;

  data = (struct grub_biosdisk_data *) grub_zalloc (sizeof (*data));
  if (! data)
    return grub_errno;

  data->drive = drive;

  if ((cd_drive) && (drive == cd_drive))
    {
      data->flags = GRUB_BIOSDISK_FLAG_LBA | GRUB_BIOSDISK_FLAG_CDROM;
      data->sectors = 8;
      disk->log_sector_size = 11;
      /* TODO: get the correct size.  */
      total_sectors = GRUB_DISK_SIZE_UNKNOWN;
    }
  else
    {
      /* HDD */
      int version;

      disk->log_sector_size = 9;

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
	      if (drp->bytes_per_sector
		  && !(drp->bytes_per_sector & (drp->bytes_per_sector - 1))
		  && drp->bytes_per_sector >= 512
		  && drp->bytes_per_sector <= 16384)
		{
		  for (disk->log_sector_size = 0;
		       (1 << disk->log_sector_size) < drp->bytes_per_sector;
		       disk->log_sector_size++);
		}
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
	  if (total_sectors && (data->flags & GRUB_BIOSDISK_FLAG_LBA))
	    {
	      data->sectors = 63;
	      data->heads = 255;
	      data->cylinders
		= grub_divmod64 (total_sectors
				 + data->heads * data->sectors - 1,
				 data->heads * data->sectors, 0);
	    }
	  else
	    {
	      grub_free (data);
	      return grub_error (GRUB_ERR_BAD_DEVICE, "%s cannot get C/H/S values", disk->name);
	    }
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
					     << disk->log_sector_size));
      dap->length = sizeof (*dap);
      dap->reserved = 0;
      dap->blocks = size;
      dap->buffer = segment << 16;	/* The format SEGMENT:ADDRESS.  */
      dap->block = sector;

      if (data->flags & GRUB_BIOSDISK_FLAG_CDROM)
        {
	  int i;

	  if (cmd)
	    return grub_error (GRUB_ERR_WRITE_ERROR, N_("cannot write to CD-ROM"));

	  for (i = 0; i < GRUB_BIOSDISK_CDROM_RETRY_COUNT; i++)
            if (! grub_biosdisk_rw_int13_extensions (0x42, data->drive, dap))
	      break;

	  if (i == GRUB_BIOSDISK_CDROM_RETRY_COUNT)
	    return grub_error (GRUB_ERR_READ_ERROR, N_("failure reading sector 0x%llx "
						       "from `%s'"),
			       (unsigned long long) sector,
			       disk->name);
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

      /* It is impossible to reach over 8064 MiB (a bit less than LBA24) with
	 the traditional CHS access.  */
      if (sector >
	  1024 /* cylinders */ *
	  256 /* heads */ *
	  63 /* spt */)
	return grub_error (GRUB_ERR_OUT_OF_RANGE,
			   N_("attempt to read or write outside of disk `%s'"),
			   disk->name);

      soff = ((grub_uint32_t) sector) % data->sectors + 1;
      head = ((grub_uint32_t) sector) / data->sectors;
      hoff = head % data->heads;
      coff = head / data->heads;

      if (coff >= data->cylinders)
	return grub_error (GRUB_ERR_OUT_OF_RANGE,
			   N_("attempt to read or write outside of disk `%s'"),
			   disk->name);

      if (grub_biosdisk_rw_standard (cmd + 0x02, data->drive,
				     coff, hoff, soff, size, segment))
	{
	  switch (cmd)
	    {
	    case GRUB_BIOSDISK_READ:
	      return grub_error (GRUB_ERR_READ_ERROR, N_("failure reading sector 0x%llx "
							 "from `%s'"),
				 (unsigned long long) sector,
				 disk->name);
	    case GRUB_BIOSDISK_WRITE:
	      return grub_error (GRUB_ERR_WRITE_ERROR, N_("failure writing sector 0x%llx "
							  "to `%s'"),
				 (unsigned long long) sector,
				 disk->name);
	    }
	}
    }

  return GRUB_ERR_NONE;
}

/* Return the number of sectors which can be read safely at a time.  */
static grub_size_t
get_safe_sectors (grub_disk_t disk, grub_disk_addr_t sector)
{
  grub_size_t size;
  grub_uint64_t offset;
  struct grub_biosdisk_data *data = disk->data;
  grub_uint32_t sectors = data->sectors;

  /* OFFSET = SECTOR % SECTORS */
  grub_divmod64 (sector, sectors, &offset);

  size = sectors - offset;

  /* Limit the max to 0x7f because of Phoenix EDD.  */
  if (size > ((0x7fU << GRUB_DISK_SECTOR_BITS) >> disk->log_sector_size))
    size = ((0x7fU << GRUB_DISK_SECTOR_BITS) >> disk->log_sector_size);

  return size;
}

static grub_err_t
grub_biosdisk_read (grub_disk_t disk, grub_disk_addr_t sector,
		    grub_size_t size, char *buf)
{
  while (size)
    {
      grub_size_t len;

      len = get_safe_sectors (disk, sector);

      if (len > size)
	len = size;

      if (grub_biosdisk_rw (GRUB_BIOSDISK_READ, disk, sector, len,
			    GRUB_MEMORY_MACHINE_SCRATCH_SEG))
	return grub_errno;

      grub_memcpy (buf, (void *) GRUB_MEMORY_MACHINE_SCRATCH_ADDR,
		   len << disk->log_sector_size);

      buf += len << disk->log_sector_size;
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

  if (data->flags & GRUB_BIOSDISK_FLAG_CDROM)
    return grub_error (GRUB_ERR_IO, N_("cannot write to CD-ROM"));

  while (size)
    {
      grub_size_t len;

      len = get_safe_sectors (disk, sector);
      if (len > size)
	len = size;

      grub_memcpy ((void *) GRUB_MEMORY_MACHINE_SCRATCH_ADDR, buf,
		   len << disk->log_sector_size);

      if (grub_biosdisk_rw (GRUB_BIOSDISK_WRITE, disk, sector, len,
			    GRUB_MEMORY_MACHINE_SCRATCH_SEG))
	return grub_errno;

      buf += len << disk->log_sector_size;
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
  grub_uint8_t boot_drive;

  if (grub_disk_firmware_is_tainted)
    {
      grub_puts_ (N_("Native disk drivers are in use. "
		     "Refusing to use firmware disk interface."));
      return;
    }
  grub_disk_firmware_fini = grub_disk_biosdisk_fini;

  grub_memset (cdrp, 0, sizeof (*cdrp));
  cdrp->size = sizeof (*cdrp);
  cdrp->media_type = 0xFF;
  boot_drive = (grub_boot_device >> 24);
  if ((! grub_biosdisk_get_cdinfo_int13_extensions (boot_drive, cdrp))
      && ((cdrp->media_type & GRUB_BIOSDISK_CDTYPE_MASK)
	  == GRUB_BIOSDISK_CDTYPE_NO_EMUL))
    cd_drive = cdrp->drive_no;
  /* Since diskboot.S rejects devices over 0x90 it must be a CD booted with
     cdboot.S
   */
  if (boot_drive >= 0x90)
    cd_drive = boot_drive;

  grub_disk_dev_register (&grub_biosdisk_dev);
}

GRUB_MOD_FINI(biosdisk)
{
  grub_disk_biosdisk_fini ();
}
