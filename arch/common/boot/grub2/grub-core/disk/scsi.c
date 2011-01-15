/* scsi.c - scsi support.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008,2009  Free Software Foundation, Inc.
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
#include <grub/scsi.h>
#include <grub/scsicmd.h>
#include <grub/time.h>


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


/* Check result of previous operation.  */
static grub_err_t
grub_scsi_request_sense (grub_scsi_t scsi)
{
  struct grub_scsi_request_sense rs;
  struct grub_scsi_request_sense_data rsd;
  grub_err_t err;

  rs.opcode = grub_scsi_cmd_request_sense;
  rs.lun = scsi->lun << GRUB_SCSI_LUN_SHIFT;
  rs.reserved1 = 0;
  rs.reserved2 = 0;
  rs.alloc_length = 0x12; /* XXX: Hardcoded for now */
  rs.control = 0;
  grub_memset (rs.pad, 0, sizeof(rs.pad));

  err = scsi->dev->read (scsi, sizeof (rs), (char *) &rs,
			 sizeof (rsd), (char *) &rsd);
  if (err)
    return err;

  return GRUB_ERR_NONE;
}
/* Self commenting... */
static grub_err_t
grub_scsi_test_unit_ready (grub_scsi_t scsi)
{
  struct grub_scsi_test_unit_ready tur;
  grub_err_t err;
  grub_err_t err_sense;
  
  tur.opcode = grub_scsi_cmd_test_unit_ready;
  tur.lun = scsi->lun << GRUB_SCSI_LUN_SHIFT;
  tur.reserved1 = 0;
  tur.reserved2 = 0;
  tur.reserved3 = 0;
  tur.control = 0;
  grub_memset (tur.pad, 0, sizeof(tur.pad));

  err = scsi->dev->read (scsi, sizeof (tur), (char *) &tur,
			 0, NULL);

  /* Each SCSI command should be followed by Request Sense.
     If not so, many devices STALLs or definitely freezes. */
  err_sense = grub_scsi_request_sense (scsi);
  if (err_sense != GRUB_ERR_NONE)
  	grub_errno = err;
  /* err_sense is ignored for now and Request Sense Data also... */
  
  if (err)
    return err;

  return GRUB_ERR_NONE;
}

/* Determine if the device is removable and the type of the device
   SCSI.  */
static grub_err_t
grub_scsi_inquiry (grub_scsi_t scsi)
{
  struct grub_scsi_inquiry iq;
  struct grub_scsi_inquiry_data iqd;
  grub_err_t err;
  grub_err_t err_sense;

  iq.opcode = grub_scsi_cmd_inquiry;
  iq.lun = scsi->lun << GRUB_SCSI_LUN_SHIFT;
  iq.page = 0;
  iq.reserved = 0;
  iq.alloc_length = 0x24; /* XXX: Hardcoded for now */
  iq.control = 0;
  grub_memset (iq.pad, 0, sizeof(iq.pad));

  err = scsi->dev->read (scsi, sizeof (iq), (char *) &iq,
			 sizeof (iqd), (char *) &iqd);

  /* Each SCSI command should be followed by Request Sense.
     If not so, many devices STALLs or definitely freezes. */
  err_sense = grub_scsi_request_sense (scsi);
  if (err_sense != GRUB_ERR_NONE)
  	grub_errno = err;
  /* err_sense is ignored for now and Request Sense Data also... */

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
  grub_err_t err_sense;

  rc.opcode = grub_scsi_cmd_read_capacity;
  rc.lun = scsi->lun << GRUB_SCSI_LUN_SHIFT;
  rc.logical_block_addr = 0;
  rc.reserved1 = 0;
  rc.reserved2 = 0;
  rc.PMI = 0;
  rc.control = 0;
  rc.pad = 0;
	
  err = scsi->dev->read (scsi, sizeof (rc), (char *) &rc,
			 sizeof (rcd), (char *) &rcd);

  /* Each SCSI command should be followed by Request Sense.
     If not so, many devices STALLs or definitely freezes. */
  err_sense = grub_scsi_request_sense (scsi);
  if (err_sense != GRUB_ERR_NONE)
  	grub_errno = err;
/* err_sense is ignored for now and Request Sense Data also... */

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
  grub_err_t err;
  grub_err_t err_sense;

  scsi = disk->data;

  rd.opcode = grub_scsi_cmd_read10;
  rd.lun = scsi->lun << GRUB_SCSI_LUN_SHIFT;
  rd.lba = grub_cpu_to_be32 (sector);
  rd.reserved = 0;
  rd.size = grub_cpu_to_be16 (size);
  rd.reserved2 = 0;
  rd.pad = 0;

  err = scsi->dev->read (scsi, sizeof (rd), (char *) &rd, size * scsi->blocksize, buf);

  /* Each SCSI command should be followed by Request Sense.
     If not so, many devices STALLs or definitely freezes. */
  err_sense = grub_scsi_request_sense (scsi);
  if (err_sense != GRUB_ERR_NONE)
  	grub_errno = err;
  /* err_sense is ignored for now and Request Sense Data also... */

  return err;
}

/* Send a SCSI request for DISK: read SIZE sectors starting with
   sector SECTOR to BUF.  */
static grub_err_t
grub_scsi_read12 (grub_disk_t disk, grub_disk_addr_t sector,
		  grub_size_t size, char *buf)
{
  grub_scsi_t scsi;
  struct grub_scsi_read12 rd;
  grub_err_t err;
  grub_err_t err_sense;

  scsi = disk->data;

  rd.opcode = grub_scsi_cmd_read12;
  rd.lun = scsi->lun << GRUB_SCSI_LUN_SHIFT;
  rd.lba = grub_cpu_to_be32 (sector);
  rd.size = grub_cpu_to_be32 (size);
  rd.reserved = 0;
  rd.control = 0;

  err = scsi->dev->read (scsi, sizeof (rd), (char *) &rd, size * scsi->blocksize, buf);

  /* Each SCSI command should be followed by Request Sense.
     If not so, many devices STALLs or definitely freezes. */
  err_sense = grub_scsi_request_sense (scsi);
  if (err_sense != GRUB_ERR_NONE)
  	grub_errno = err;
  /* err_sense is ignored for now and Request Sense Data also... */

  return err;
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
  grub_err_t err;
  grub_err_t err_sense;

  scsi = disk->data;

  wr.opcode = grub_scsi_cmd_write10;
  wr.lun = scsi->lun << GRUB_SCSI_LUN_SHIFT;
  wr.lba = grub_cpu_to_be32 (sector);
  wr.reserved = 0;
  wr.size = grub_cpu_to_be16 (size);
  wr.reserved2 = 0;
  wr.pad = 0;

  err = scsi->dev->write (scsi, sizeof (wr), (char *) &wr, size * scsi->blocksize, buf);

  /* Each SCSI command should be followed by Request Sense.
     If not so, many devices STALLs or definitely freezes. */
  err_sense = grub_scsi_request_sense (scsi);
  if (err_sense != GRUB_ERR_NONE)
  	grub_errno = err;
  /* err_sense is ignored for now and Request Sense Data also... */

  return err;
}

/* Send a SCSI request for DISK: write the data stored in BUF to SIZE
   sectors starting with SECTOR.  */
static grub_err_t
grub_scsi_write12 (grub_disk_t disk, grub_disk_addr_t sector,
		   grub_size_t size, char *buf)
{
  grub_scsi_t scsi;
  struct grub_scsi_write12 wr;
  grub_err_t err;
  grub_err_t err_sense;

  scsi = disk->data;

  wr.opcode = grub_scsi_cmd_write12;
  wr.lun = scsi->lun << GRUB_SCSI_LUN_SHIFT;
  wr.lba = grub_cpu_to_be32 (sector);
  wr.size = grub_cpu_to_be32 (size);
  wr.reserved = 0;
  wr.control = 0;

  err = scsi->dev->write (scsi, sizeof (wr), (char *) &wr, size * scsi->blocksize, buf);

  /* Each SCSI command should be followed by Request Sense.
     If not so, many devices STALLs or definitely freezes. */
  err_sense = grub_scsi_request_sense (scsi);
  if (err_sense != GRUB_ERR_NONE)
  	grub_errno = err;
  /* err_sense is ignored for now and Request Sense Data also... */

  return err;
}
#endif


static int
grub_scsi_iterate (int (*hook) (const char *name))
{
  grub_scsi_dev_t p;

  auto int scsi_iterate (int bus, int luns);

  int scsi_iterate (int bus, int luns)
    {
      int i;

      /* In case of a single LUN, just return `usbX'.  */
      if (luns == 1)
	{
	  char *sname;
	  int ret;
	  sname = grub_xasprintf ("%s%d", p->name, bus);
	  if (!sname)
	    return 1;
	  ret = hook (sname);
	  grub_free (sname);
	  return ret;
	}

      /* In case of multiple LUNs, every LUN will get a prefix to
	 distinguish it.  */
      for (i = 0; i < luns; i++)
	{
	  char *sname;
	  int ret;
	  sname = grub_xasprintf ("%s%d%c", p->name, bus, 'a' + i);
	  if (!sname)
	    return 1;
	  ret = hook (sname);
	  grub_free (sname);
	  if (ret)
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
  int lun, bus;
  grub_uint64_t maxtime;
  const char *nameend;

  nameend = name + grub_strlen (name) - 1;
  /* Try to detect a LUN ('a'-'z'), otherwise just use the first
     LUN.  */
  if (nameend >= name && *nameend >= 'a' && *nameend <= 'z')
    {
      lun = *nameend - 'a';
      nameend--;
    }
  else
    lun = 0;

  while (nameend >= name && grub_isdigit (*nameend))
    nameend--;

  if (!nameend[1] || !grub_isdigit (nameend[1]))
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "not a SCSI disk");

  bus = grub_strtoul (nameend + 1, 0, 0);

  scsi = grub_malloc (sizeof (*scsi));
  if (! scsi)
    return grub_errno;

  for (p = grub_scsi_dev_list; p; p = p->next)
    {
      if (grub_strncmp (p->name, name, nameend - name) != 0)
	continue;

      if (p->open (bus, scsi))
	continue;

      disk->id = grub_make_scsi_id (p->id, bus, lun);
      disk->data = scsi;
      scsi->dev = p;
      scsi->lun = lun;
      scsi->bus = bus;

      grub_dprintf ("scsi", "dev opened\n");

      err = grub_scsi_inquiry (scsi);
      if (err)
	{
	  grub_free (scsi);
	  grub_dprintf ("scsi", "inquiry failed\n");
	  return err;
	}

      grub_dprintf ("scsi", "inquiry: devtype=0x%02x removable=%d\n",
		    scsi->devtype, scsi->removable);

      /* Try to be conservative about the device types
	 supported.  */
      if (scsi->devtype != grub_scsi_devtype_direct
	  && scsi->devtype != grub_scsi_devtype_cdrom)
	{
	  grub_free (scsi);
	  return grub_error (GRUB_ERR_UNKNOWN_DEVICE,
			     "unknown SCSI device");
	}

      /* According to USB MS tests specification, issue Test Unit Ready
       * until OK */
      maxtime = grub_get_time_ms () + 5000; /* It is safer value */
      do
        {
	  /* Timeout is necessary - for example in case when we have
	   * universal card reader with more LUNs and we have only
	   * one card inserted (or none), so only one LUN (or none)
	   * will be ready - and we want not to hang... */
	  if (grub_get_time_ms () > maxtime)
            {
              err = GRUB_ERR_READ_ERROR;
              grub_free (scsi);
              grub_dprintf ("scsi", "LUN is not ready - timeout\n");
              return err;
            }
          err = grub_scsi_test_unit_ready (scsi);
        }
      while (err == GRUB_ERR_READ_ERROR);
      /* Reset grub_errno !
       * It is set to some error code in loop before... */
      grub_errno = GRUB_ERR_NONE;

      /* Read capacity of media */
      err = grub_scsi_read_capacity (scsi);
      if (err)
	{
	  grub_free (scsi);
	  grub_dprintf ("scsi", "READ CAPACITY failed\n");
	  return err;
	}

      /* SCSI blocks can be something else than 512, although GRUB
	 wants 512 byte blocks.  */
      disk->total_sectors = ((grub_uint64_t)scsi->size
                             * (grub_uint64_t)scsi->blocksize)
			    >> GRUB_DISK_SECTOR_BITS;

      grub_dprintf ("scsi", "blocks=%u, blocksize=%u\n",
		    scsi->size, scsi->blocksize);
      grub_dprintf ("scsi", "Disk total 512 sectors = %llu\n",
		    (unsigned long long) disk->total_sectors);

      return GRUB_ERR_NONE;
    }

  grub_free (scsi);

  return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "not a SCSI disk");
}

static void
grub_scsi_close (grub_disk_t disk)
{
  grub_scsi_t scsi;

  scsi = disk->data;
  if (scsi->dev->close)
    scsi->dev->close (scsi);
  grub_free (scsi);
}

static grub_err_t
grub_scsi_read (grub_disk_t disk, grub_disk_addr_t sector,
		grub_size_t size, char *buf)
{
  grub_scsi_t scsi;

  scsi = disk->data;

  /* SCSI sectors are variable in size.  GRUB uses 512 byte
     sectors.  */
  if (scsi->blocksize != GRUB_DISK_SECTOR_SIZE)
    {
      unsigned spb = scsi->blocksize >> GRUB_DISK_SECTOR_BITS;
      if (! (spb != 0 && (scsi->blocksize & GRUB_DISK_SECTOR_SIZE) == 0))
	return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
			   "unsupported SCSI block size");

      grub_uint32_t sector_mod = 0;
      sector = grub_divmod64 (sector, spb, &sector_mod);

      if (! (sector_mod == 0 && size % spb == 0))
	return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
			   "unaligned SCSI read not supported");

      size /= spb;
    }

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

#if 0 /* Workaround - it works - but very slowly, from some reason
       * unknown to me (specially on OHCI). Do not use it. */
  /* Split transfer requests to device sector size because */
  /* some devices are not able to transfer more than 512-1024 bytes */
  grub_err_t err = GRUB_ERR_NONE;

  for ( ; size; size--)
    {
      /* Depending on the type, select a read function.  */
      switch (scsi->devtype)
        {
          case grub_scsi_devtype_direct:
            err = grub_scsi_read10 (disk, sector, 1, buf);
            break;

          case grub_scsi_devtype_cdrom:
            err = grub_scsi_read12 (disk, sector, 1, buf);
            break;

          default: /* This should not happen */
            return GRUB_ERR_READ_ERROR;
        }
      if (err)
        return err;
      sector++;
      buf += scsi->blocksize;
    }

  return err;
#endif
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
