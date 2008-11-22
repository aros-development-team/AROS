/* scsi.c - scsi support.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008  Free Software Foundation, Inc.
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

#include <grub/disk.h>
#include <grub/dl.h>
#include <grub/kernel.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/types.h>
#include <grub/machine/kernel.h>
#include <grub/scsi.h>
#include <grub/scsicmd.h>


static grub_scsi_dev_t grub_scsi_dev_list;

void
grub_scsi_dev_register (grub_scsi_dev_t dev)
{
  dev->next = grub_scsi_dev_list;
  grub_scsi_dev_list = dev;
}

void
grub_scsi_dev_unregister (grub_scsi_dev_t dev)
{
  grub_scsi_dev_t *p, q;
  
  for (p = &grub_scsi_dev_list, q = *p; q; p = &(q->next), q = q->next)
    if (q == dev)
      {
        *p = q->next;
	break;
      }
}


/* Determine the the device is removable and the type of the device
   SCSI.  */ 
static grub_err_t
grub_scsi_inquiry (grub_scsi_t scsi)
{
  struct grub_scsi_inquiry iq;
  struct grub_scsi_inquiry_data iqd;
  grub_err_t err;

  iq.opcode = grub_scsi_cmd_inquiry;
  iq.lun = scsi->lun << GRUB_SCSI_LUN_SHIFT;
  iq.reserved = 0;
  iq.alloc_length = 0x24; /* XXX: Hardcoded for now */
  iq.reserved2 = 0;

  err = scsi->dev->read (scsi, sizeof (iq), (char *) &iq,
			 sizeof (iqd), (char *) &iqd);
  if (err)
    return err;

  scsi->devtype = iqd.devtype & GRUB_SCSI_DEVTYPE_MASK;
  scsi->removable = iqd.rmb >> GRUB_SCSI_REMOVABLE_BIT;

  return GRUB_ERR_NONE;
}

/* Read the capacity and block size of SCSI.  */
static grub_err_t
grub_scsi_read_capacity (grub_scsi_t scsi)
{
  struct grub_scsi_read_capacity rc;
  struct grub_scsi_read_capacity_data rcd;
  grub_err_t err;

  rc.opcode = grub_scsi_cmd_read_capacity;
  rc.lun = scsi->lun << GRUB_SCSI_LUN_SHIFT;
  grub_memset (rc.reserved, 0, sizeof (rc.reserved));

  err = scsi->dev->read (scsi, sizeof (rc), (char *) &rc,
			 sizeof (rcd), (char *) &rcd);
  if (err)
    return err;

  scsi->size = grub_be_to_cpu32 (rcd.size);
  scsi->blocksize = grub_be_to_cpu32 (rcd.blocksize);

  return GRUB_ERR_NONE;
}

/* Send a SCSI request for DISK: read SIZE sectors starting with
   sector SECTOR to BUF.  */
static grub_err_t
grub_scsi_read10 (grub_disk_t disk, grub_disk_addr_t sector,
		  grub_size_t size, char *buf)
{
  grub_scsi_t scsi;
  struct grub_scsi_read10 rd;

  scsi = disk->data;

  rd.opcode = grub_scsi_cmd_read10;
  rd.lun = scsi->lun << GRUB_SCSI_LUN_SHIFT;
  rd.lba = grub_cpu_to_be32 (sector);
  rd.reserved = 0;
  rd.size = grub_cpu_to_be16 (size);
  rd.reserved2 = 0;
  rd.pad = 0;

  return scsi->dev->read (scsi, sizeof (rd), (char *) &rd, size * 512, buf);
}

/* Send a SCSI request for DISK: read SIZE sectors starting with
   sector SECTOR to BUF.  */
static grub_err_t
grub_scsi_read12 (grub_disk_t disk, grub_disk_addr_t sector,
		  grub_size_t size, char *buf)
{
  grub_scsi_t scsi;
  struct grub_scsi_read12 rd;

  scsi = disk->data;

  rd.opcode = grub_scsi_cmd_read12;
  rd.lun = scsi->lun << GRUB_SCSI_LUN_SHIFT;
  rd.lba = grub_cpu_to_be32 (sector);
  rd.size = grub_cpu_to_be32 (size);
  rd.reserved = 0;
  rd.control = 0;

  return scsi->dev->read (scsi, sizeof (rd), (char *) &rd, size * 512, buf);
}

#if 0
/* Send a SCSI request for DISK: write the data stored in BUF to SIZE
   sectors starting with SECTOR.  */
static grub_err_t
grub_scsi_write10 (grub_disk_t disk, grub_disk_addr_t sector,
		   grub_size_t size, char *buf)
{
  grub_scsi_t scsi;
  struct grub_scsi_write10 wr;

  scsi = disk->data;

  wr.opcode = grub_scsi_cmd_write10;
  wr.lun = scsi->lun << GRUB_SCSI_LUN_SHIFT;
  wr.lba = grub_cpu_to_be32 (sector);
  wr.reserved = 0;
  wr.size = grub_cpu_to_be16 (size);
  wr.reserved2 = 0;
  wr.pad = 0;

  return scsi->dev->write (scsi, sizeof (wr), (char *) &wr, size * 512, buf);
}

/* Send a SCSI request for DISK: write the data stored in BUF to SIZE
   sectors starting with SECTOR.  */
static grub_err_t
grub_scsi_write12 (grub_disk_t disk, grub_disk_addr_t sector,
		   grub_size_t size, char *buf)
{
  grub_scsi_t scsi;
  struct grub_scsi_write10 wr;

  scsi = disk->data;

  wr.opcode = grub_scsi_cmd_write12;
  wr.lun = scsi->lun << GRUB_SCSI_LUN_SHIFT;
  wr.lba = grub_cpu_to_be32 (sector);
  wr.size = grub_cpu_to_be32 (size);
  wr.reserved = 0;
  wr.pad = 0;

  return scsi->dev->write (scsi, sizeof (wr), (char *) &wr, size * 512, buf);
}
#endif


static int
grub_scsi_iterate (int (*hook) (const char *name))
{
  grub_scsi_dev_t p;

  auto int scsi_iterate (const char *name, int luns);

  int scsi_iterate (const char *name, int luns)
    {
      char sname[40];
      int i;

      /* In case of a single LUN, just return `usbX'.  */
      if (luns == 1)
	return hook (name);

      /* In case of multiple LUNs, every LUN will get a prefix to
	 distinguish it.  */
      for (i = 0; i < luns; i++)
	{
	  grub_sprintf (sname, "%s%c", name, 'a' + i);
	  if (hook (sname))
	    return 1;
	}
      return 0;
    }

  for (p = grub_scsi_dev_list; p; p = p->next)
    if (p->iterate && (p->iterate) (scsi_iterate))
      return 1;

  return 0;
}

static grub_err_t
grub_scsi_open (const char *name, grub_disk_t disk)
{
  grub_scsi_dev_t p;
  grub_scsi_t scsi;
  grub_err_t err;
  int len;
  int lun;
  
  scsi = grub_malloc (sizeof (*scsi));
  if (! scsi)
    return grub_errno;

  len = grub_strlen (name);
  lun = name[len - 1] - 'a';

  /* Try to detect a LUN ('a'-'z'), otherwise just use the first
     LUN.  */
  if (lun < 0 || lun > 26)
    lun = 0;

  for (p = grub_scsi_dev_list; p; p = p->next)
    {
      if (! p->open (name, scsi))
	{
	  disk->id = (unsigned long) "scsi"; /* XXX */
	  disk->data = scsi;
	  scsi->dev = p;
	  scsi->lun = lun;
	  scsi->name = grub_strdup (name);
	  if (! scsi->name)
	    {
	      return grub_errno;
	    }

	  grub_dprintf ("scsi", "dev opened\n");

	  err = grub_scsi_inquiry (scsi);
	  if (err)
	    {
	      grub_dprintf ("scsi", "inquiry failed\n");
	      return grub_errno;
	    }

	  grub_dprintf ("scsi", "inquiry: devtype=0x%02x removable=%d\n",
			scsi->devtype, scsi->removable);
	  
	  /* Try to be conservative about the device types
	     supported.  */
	  if (scsi->devtype != grub_scsi_devtype_direct
	      && scsi->devtype != grub_scsi_devtype_cdrom)
	    {
	      return grub_error (GRUB_ERR_UNKNOWN_DEVICE,
				 "unknown SCSI device");
	    }

	  if (scsi->devtype == grub_scsi_devtype_cdrom)
	    disk->has_partitions = 0;
	  else
	    disk->has_partitions = 1;

	  err = grub_scsi_read_capacity (scsi);
	  if (err)
	    {
	      grub_dprintf ("scsi", "READ CAPACITY failed\n");
	      return grub_errno;
	    }

	  /* SCSI blocks can be something else than 512, although GRUB
	     wants 512 byte blocks.  */
	  disk->total_sectors = ((scsi->size * scsi->blocksize)
				 << GRUB_DISK_SECTOR_BITS);

	  grub_dprintf ("scsi", "capacity=%llu, blksize=%d\n",
			disk->total_sectors, scsi->blocksize);

	  return GRUB_ERR_NONE;
	}
    }

  return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "not a SCSI disk");
}

static void
grub_scsi_close (grub_disk_t disk)
{
  grub_scsi_t scsi;

  scsi = disk->data;
  return scsi->dev->close (scsi);
}

static grub_err_t
grub_scsi_read (grub_disk_t disk, grub_disk_addr_t sector,
		grub_size_t size, char *buf)
{
  grub_scsi_t scsi;

  scsi = disk->data;

  /* SCSI sectors are variable in size.  GRUB uses 512 byte
     sectors.  */
  sector = grub_divmod64 (sector, scsi->blocksize >> GRUB_DISK_SECTOR_BITS,
			  NULL);

  /* Depending on the type, select a read function.  */
  switch (scsi->devtype)
    {
    case grub_scsi_devtype_direct:
      return grub_scsi_read10 (disk, sector, size, buf);

    case grub_scsi_devtype_cdrom:
      return grub_scsi_read12 (disk, sector, size, buf);
    }

  /* XXX: Never reached.  */
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_scsi_write (grub_disk_t disk __attribute((unused)),
		 grub_disk_addr_t sector __attribute((unused)),
		 grub_size_t size __attribute((unused)),
		 const char *buf __attribute((unused)))
{
#if 0
  /* XXX: Not tested yet!  */

  /* XXX: This should depend on the device type?  */
  return grub_scsi_write10 (disk, sector, size, buf);
#endif
  return GRUB_ERR_NOT_IMPLEMENTED_YET;
}


static struct grub_disk_dev grub_scsi_dev =
  {
    .name = "scsi",
    .id = GRUB_DISK_DEVICE_SCSI_ID,
    .iterate = grub_scsi_iterate,
    .open = grub_scsi_open,
    .close = grub_scsi_close,
    .read = grub_scsi_read,
    .write = grub_scsi_write,
    .next = 0
  };

GRUB_MOD_INIT(scsi)
{
  grub_disk_dev_register (&grub_scsi_dev);
}

GRUB_MOD_FINI(scsi)
{
  grub_disk_dev_unregister (&grub_scsi_dev);
}
