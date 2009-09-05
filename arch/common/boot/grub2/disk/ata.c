/* ata.c - ATA disk access.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2007, 2008, 2009  Free Software Foundation, Inc.
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

#include <grub/ata.h>
#include <grub/dl.h>
#include <grub/disk.h>
#include <grub/mm.h>
#include <grub/time.h>
#include <grub/pci.h>
#include <grub/scsi.h>

/* At the moment, only two IDE ports are supported.  */
static const int grub_ata_ioaddress[] = { 0x1f0, 0x170 };
static const int grub_ata_ioaddress2[] = { 0x3f6, 0x376 };

static struct grub_ata_device *grub_ata_devices;

/* Wait for !BSY.  */
grub_err_t
grub_ata_wait_not_busy (struct grub_ata_device *dev, int milliseconds)
{
  /* ATA requires 400ns (after a write to CMD register) or
     1 PIO cycle (after a DRQ block transfer) before
     first check of BSY.  */
  grub_millisleep (1);

  int i = 1;
  grub_uint8_t sts;
  while ((sts = grub_ata_regget (dev, GRUB_ATA_REG_STATUS))
	 & GRUB_ATA_STATUS_BUSY)
    {
      if (i >= milliseconds)
        {
	  grub_dprintf ("ata", "timeout: %dms, status=0x%x\n",
			milliseconds, sts);
	  return grub_error (GRUB_ERR_TIMEOUT, "ATA timeout");
	}

      grub_millisleep (1);
      i++;
    }

  return GRUB_ERR_NONE;
}

static inline void
grub_ata_wait (void)
{
  grub_millisleep (50);
}

/* Wait for !BSY, DRQ.  */
grub_err_t
grub_ata_wait_drq (struct grub_ata_device *dev, int rw,
		   int milliseconds)
{
  if (grub_ata_wait_not_busy (dev, milliseconds))
    return grub_errno;

  /* !DRQ implies error condition.  */
  grub_uint8_t sts = grub_ata_regget (dev, GRUB_ATA_REG_STATUS);
  if ((sts & (GRUB_ATA_STATUS_DRQ | GRUB_ATA_STATUS_ERR))
      != GRUB_ATA_STATUS_DRQ)
    {
      grub_dprintf ("ata", "ata error: status=0x%x, error=0x%x\n",
		    sts, grub_ata_regget (dev, GRUB_ATA_REG_ERROR));
      if (! rw)
        return grub_error (GRUB_ERR_READ_ERROR, "ATA read error");
      else
        return grub_error (GRUB_ERR_WRITE_ERROR, "ATA write error");
    }

  return GRUB_ERR_NONE;
}

/* Byteorder has to be changed before strings can be read.  */
static void
grub_ata_strncpy (char *dst, char *src, grub_size_t len)
{
  grub_uint16_t *src16 = (grub_uint16_t *) src;
  grub_uint16_t *dst16 = (grub_uint16_t *) dst;
  unsigned int i;

  for (i = 0; i < len / 2; i++)
    *(dst16++) = grub_be_to_cpu16 (*(src16++));
  dst[len] = '\0';
}

void
grub_ata_pio_read (struct grub_ata_device *dev, char *buf, grub_size_t size)
{
  grub_uint16_t *buf16 = (grub_uint16_t *) buf;
  unsigned int i;

  /* Read in the data, word by word.  */
  for (i = 0; i < size / 2; i++)
    buf16[i] = grub_le_to_cpu16 (grub_inw(dev->ioaddress + GRUB_ATA_REG_DATA));
}

static void
grub_ata_pio_write (struct grub_ata_device *dev, char *buf, grub_size_t size)
{
  grub_uint16_t *buf16 = (grub_uint16_t *) buf;
  unsigned int i;

  /* Write the data, word by word.  */
  for (i = 0; i < size / 2; i++)
    grub_outw(grub_cpu_to_le16 (buf16[i]), dev->ioaddress + GRUB_ATA_REG_DATA);
}

static void
grub_ata_dumpinfo (struct grub_ata_device *dev, char *info)
{
  char text[41];

  /* The device information was read, dump it for debugging.  */
  grub_ata_strncpy (text, info + 20, 20);
  grub_dprintf ("ata", "Serial: %s\n", text);
  grub_ata_strncpy (text, info + 46, 8);
  grub_dprintf ("ata", "Firmware: %s\n", text);
  grub_ata_strncpy (text, info + 54, 40);
  grub_dprintf ("ata", "Model: %s\n", text);

  if (! dev->atapi)
    {
      grub_dprintf ("ata", "Addressing: %d\n", dev->addr);
      grub_dprintf ("ata", "Sectors: %lld\n", dev->size);
    }
}

static grub_err_t
grub_atapi_identify (struct grub_ata_device *dev)
{
  char *info;

  info = grub_malloc (GRUB_DISK_SECTOR_SIZE);
  if (! info)
    return grub_errno;

  grub_ata_regset (dev, GRUB_ATA_REG_DISK, 0xE0 | dev->device << 4);
  grub_ata_wait ();
  if (grub_ata_check_ready (dev))
    {
      grub_free (info);
      return grub_errno;
    }

  grub_ata_regset (dev, GRUB_ATA_REG_CMD, GRUB_ATA_CMD_IDENTIFY_PACKET_DEVICE);
  grub_ata_wait ();

  if (grub_ata_wait_drq (dev, 0, GRUB_ATA_TOUT_STD))
    {
      grub_free (info);
      return grub_errno;
    }
  grub_ata_pio_read (dev, info, GRUB_DISK_SECTOR_SIZE);

  dev->atapi = 1;

  grub_ata_dumpinfo (dev, info);

  grub_free (info);

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_atapi_wait_drq (struct grub_ata_device *dev,
		     grub_uint8_t ireason,
		     int milliseconds)
{
  /* Wait for !BSY, DRQ, ireason */
  if (grub_ata_wait_not_busy (dev, milliseconds))
    return grub_errno;

  grub_uint8_t sts = grub_ata_regget (dev, GRUB_ATA_REG_STATUS);
  grub_uint8_t irs = grub_ata_regget (dev, GRUB_ATAPI_REG_IREASON);

  /* OK if DRQ is asserted and interrupt reason is as expected.  */
  if ((sts & GRUB_ATA_STATUS_DRQ)
      && (irs & GRUB_ATAPI_IREASON_MASK) == ireason)
    return GRUB_ERR_NONE;

  /* !DRQ implies error condition.  */
  grub_dprintf ("ata", "atapi error: status=0x%x, ireason=0x%x, error=0x%x\n",
	        sts, irs, grub_ata_regget (dev, GRUB_ATA_REG_ERROR));

  if (! (sts & GRUB_ATA_STATUS_DRQ)
      && (irs & GRUB_ATAPI_IREASON_MASK) == GRUB_ATAPI_IREASON_ERROR)
    {
      if (ireason == GRUB_ATAPI_IREASON_CMD_OUT)
	return grub_error (GRUB_ERR_READ_ERROR, "ATA PACKET command error");
      else
	return grub_error (GRUB_ERR_READ_ERROR, "ATAPI read error");
    }

  return grub_error (GRUB_ERR_READ_ERROR, "ATAPI protocol error");
}

static grub_err_t
grub_atapi_packet (struct grub_ata_device *dev, char *packet,
		   grub_size_t size)
{
  grub_ata_regset (dev, GRUB_ATA_REG_DISK, dev->device << 4);
  if (grub_ata_check_ready (dev))
    return grub_errno;

  /* Send ATA PACKET command.  */
  grub_ata_regset (dev, GRUB_ATA_REG_FEATURES, 0);
  grub_ata_regset (dev, GRUB_ATAPI_REG_IREASON, 0);
  grub_ata_regset (dev, GRUB_ATAPI_REG_CNTHIGH, size >> 8);
  grub_ata_regset (dev, GRUB_ATAPI_REG_CNTLOW, size & 0xFF);

  grub_ata_regset (dev, GRUB_ATA_REG_CMD, GRUB_ATA_CMD_PACKET);

  /* Wait for !BSY, DRQ, !I/O, C/D.  */
  if (grub_atapi_wait_drq (dev, GRUB_ATAPI_IREASON_CMD_OUT, GRUB_ATA_TOUT_STD))
    return grub_errno;

  /* Write the packet.  */
  grub_ata_pio_write (dev, packet, 12);

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_ata_identify (struct grub_ata_device *dev)
{
  char *info;
  grub_uint16_t *info16;

  info = grub_malloc (GRUB_DISK_SECTOR_SIZE);
  if (! info)
    return grub_errno;

  info16 = (grub_uint16_t *) info;

  grub_ata_regset (dev, GRUB_ATA_REG_DISK, 0xE0 | dev->device << 4);
  grub_ata_wait ();
  if (grub_ata_check_ready (dev))
    {
      grub_free (info);
      return grub_errno;
    }

  grub_ata_regset (dev, GRUB_ATA_REG_CMD, GRUB_ATA_CMD_IDENTIFY_DEVICE);
  grub_ata_wait ();

  if (grub_ata_wait_drq (dev, 0, GRUB_ATA_TOUT_STD))
    {
      grub_free (info);
      grub_errno = GRUB_ERR_NONE;
      grub_uint8_t sts = grub_ata_regget (dev, GRUB_ATA_REG_STATUS);

      if ((sts & (GRUB_ATA_STATUS_BUSY | GRUB_ATA_STATUS_DRQ
	  | GRUB_ATA_STATUS_ERR)) == GRUB_ATA_STATUS_ERR
	  && (grub_ata_regget (dev, GRUB_ATA_REG_ERROR) & 0x04 /* ABRT */))
	/* Device without ATA IDENTIFY, try ATAPI.  */
	return grub_atapi_identify (dev);

      else if (sts == 0x00)
	/* No device, return error but don't print message.  */
	return GRUB_ERR_UNKNOWN_DEVICE;

      else
	/* Other Error.  */
	return grub_error (GRUB_ERR_UNKNOWN_DEVICE,
			   "device can not be identified");
    }

  grub_ata_pio_read (dev, info, GRUB_DISK_SECTOR_SIZE);

  /* Re-check status to avoid bogus identify data due to stuck DRQ.  */
  grub_uint8_t sts = grub_ata_regget (dev, GRUB_ATA_REG_STATUS);
  if (sts & (GRUB_ATA_STATUS_BUSY | GRUB_ATA_STATUS_DRQ | GRUB_ATA_STATUS_ERR))
    {
      grub_dprintf ("ata", "bad status=0x%x\n", sts);
      grub_free (info);
      /* No device, return error but don't print message.  */
      grub_errno = GRUB_ERR_NONE;
      return GRUB_ERR_UNKNOWN_DEVICE;
    }

  /* Now it is certain that this is not an ATAPI device.  */
  dev->atapi = 0;

  /* CHS is always supported.  */
  dev->addr = GRUB_ATA_CHS;

  /* Check if LBA is supported.  */
  if (info16[49] & (1 << 9))
    {
      /* Check if LBA48 is supported.  */
      if (info16[83] & (1 << 10))
	dev->addr = GRUB_ATA_LBA48;
      else
	dev->addr = GRUB_ATA_LBA;
    }

  /* Determine the amount of sectors.  */
  if (dev->addr != GRUB_ATA_LBA48)
    dev->size = grub_le_to_cpu32(*((grub_uint32_t *) &info16[60]));
  else
    dev->size = grub_le_to_cpu64(*((grub_uint64_t *) &info16[100]));

  /* Read CHS information.  */
  dev->cylinders = info16[1];
  dev->heads = info16[3];
  dev->sectors_per_track = info16[6];

  grub_ata_dumpinfo (dev, info);

  grub_free(info);

  return 0;
}

static grub_err_t
grub_ata_device_initialize (int port, int device, int addr, int addr2)
{
  struct grub_ata_device *dev;
  struct grub_ata_device **devp;

  grub_dprintf ("ata", "detecting device %d,%d (0x%x, 0x%x)\n",
		port, device, addr, addr2);

  dev = grub_malloc (sizeof(*dev));
  if (! dev)
    return grub_errno;

  /* Setup the device information.  */
  dev->port = port;
  dev->device = device;
  dev->ioaddress = addr;
  dev->ioaddress2 = addr2;
  dev->next = NULL;

  grub_ata_regset (dev, GRUB_ATA_REG_DISK, dev->device << 4);
  grub_ata_wait ();

  /* Try to detect if the port is in use by writing to it,
     waiting for a while and reading it again.  If the value
     was preserved, there is a device connected.  */
  grub_ata_regset (dev, GRUB_ATA_REG_SECTORS, 0x5A);
  grub_ata_wait ();
  grub_uint8_t sec = grub_ata_regget (dev, GRUB_ATA_REG_SECTORS);
  grub_dprintf ("ata", "sectors=0x%x\n", sec);
  if (sec != 0x5A)
    {
      grub_free(dev);
      return 0;
    }

  /* The above test may detect a second (slave) device
     connected to a SATA controller which supports only one
     (master) device.  It is not safe to use the status register
     READY bit to check for controller channel existence.  Some
     ATAPI commands (RESET, DIAGNOSTIC) may clear this bit.  */

  /* Use the IDENTIFY DEVICE command to query the device.  */
  if (grub_ata_identify (dev))
    {
      grub_free (dev);
      return 0;
    }

  /* Register the device.  */
  for (devp = &grub_ata_devices; *devp; devp = &(*devp)->next);
  *devp = dev;

  return 0;
}

static int NESTED_FUNC_ATTR
grub_ata_pciinit (int bus, int device, int func,
		  grub_pci_id_t pciid __attribute__((unused)))
{
  static int compat_use[2] = { 0 };
  grub_pci_address_t addr;
  grub_uint32_t class;
  grub_uint32_t bar1;
  grub_uint32_t bar2;
  int rega;
  int regb;
  int i;
  static int controller = 0;

  /* Read class.  */
  addr = grub_pci_make_address (bus, device, func, 2);
  class = grub_pci_read (addr);

  /* Check if this class ID matches that of a PCI IDE Controller.  */
  if (class >> 16 != 0x0101)
    return 0;

  for (i = 0; i < 2; i++)
    {
      /* Set to 0 when the channel operated in compatibility mode.  */
      int compat = (class >> (8 + 2 * i)) & 1;

      rega = 0;
      regb = 0;

      /* If the channel is in compatibility mode, just assign the
	 default registers.  */
      if (compat == 0 && !compat_use[i])
	{
	  rega = grub_ata_ioaddress[i];
	  regb = grub_ata_ioaddress2[i];
	  compat_use[i] = 1;
	}
      else if (compat)
	{
	  /* Read the BARs, which either contain a mmapped IO address
	     or the IO port address.  */
	  addr = grub_pci_make_address (bus, device, func, 4 + 2 * i);
	  bar1 = grub_pci_read (addr);
	  addr = grub_pci_make_address (bus, device, func, 5 + 2 * i);
	  bar2 = grub_pci_read (addr);

	  /* Check if the BARs describe an IO region.  */
	  if ((bar1 & 1) && (bar2 & 1))
	    {
	      rega = bar1 & ~3;
	      regb = bar2 & ~3;
	    }
	}

      grub_dprintf ("ata",
		    "PCI dev (%d,%d,%d) compat=%d rega=0x%x regb=0x%x\n",
		    bus, device, func, compat, rega, regb);

      if (rega && regb)
	{
	  grub_errno = GRUB_ERR_NONE;
	  grub_ata_device_initialize (controller * 2 + i, 0, rega, regb);

	  /* Most errors raised by grub_ata_device_initialize() are harmless.
	     They just indicate this particular drive is not responding, most
	     likely because it doesn't exist.  We might want to ignore specific
	     error types here, instead of printing them.  */
	  if (grub_errno)
	    {
	      grub_print_error ();
	      grub_errno = GRUB_ERR_NONE;
	    }

	  grub_ata_device_initialize (controller * 2 + i, 1, rega, regb);

	  /* Likewise.  */
	  if (grub_errno)
	    {
	      grub_print_error ();
	      grub_errno = GRUB_ERR_NONE;
	    }
	}
    }

  controller++;

  return 0;
}

static grub_err_t
grub_ata_initialize (void)
{
  grub_pci_iterate (grub_ata_pciinit);
  return 0;
}


static void
grub_ata_setlba (struct grub_ata_device *dev, grub_disk_addr_t sector,
		 grub_size_t size)
{
  grub_ata_regset (dev, GRUB_ATA_REG_SECTORS, size);
  grub_ata_regset (dev, GRUB_ATA_REG_LBALOW, sector & 0xFF);
  grub_ata_regset (dev, GRUB_ATA_REG_LBAMID, (sector >> 8) & 0xFF);
  grub_ata_regset (dev, GRUB_ATA_REG_LBAHIGH, (sector >> 16) & 0xFF);
}

static grub_err_t
grub_ata_setaddress (struct grub_ata_device *dev,
		     grub_ata_addressing_t addressing,
		     grub_disk_addr_t sector,
		     grub_size_t size)
{
  switch (addressing)
    {
    case GRUB_ATA_CHS:
      {
	unsigned int cylinder;
	unsigned int head;
	unsigned int sect;

	/* Calculate the sector, cylinder and head to use.  */
	sect = ((grub_uint32_t) sector % dev->sectors_per_track) + 1;
	cylinder = (((grub_uint32_t) sector / dev->sectors_per_track)
		    / dev->heads);
	head = ((grub_uint32_t) sector / dev->sectors_per_track) % dev->heads;

	if (sect > dev->sectors_per_track
	    || cylinder > dev->cylinders
	    || head > dev->heads)
	  return grub_error (GRUB_ERR_OUT_OF_RANGE,
			     "sector %d can not be addressed "
			     "using CHS addressing", sector);

	grub_ata_regset (dev, GRUB_ATA_REG_DISK, (dev->device << 4) | head);
	if (grub_ata_check_ready (dev))
	  return grub_errno;

	grub_ata_regset (dev, GRUB_ATA_REG_SECTNUM, sect);
	grub_ata_regset (dev, GRUB_ATA_REG_CYLLSB, cylinder & 0xFF);
	grub_ata_regset (dev, GRUB_ATA_REG_CYLMSB, cylinder >> 8);

	break;
      }

    case GRUB_ATA_LBA:
      if (size == 256)
	size = 0;
      grub_ata_regset (dev, GRUB_ATA_REG_DISK,
		       0xE0 | (dev->device << 4) | ((sector >> 24) & 0x0F));
      if (grub_ata_check_ready (dev))
	return grub_errno;

      grub_ata_setlba (dev, sector, size);
      break;

    case GRUB_ATA_LBA48:
      if (size == 65536)
	size = 0;

      grub_ata_regset (dev, GRUB_ATA_REG_DISK, 0xE0 | (dev->device << 4));
      if (grub_ata_check_ready (dev))
	return grub_errno;

      /* Set "Previous".  */
      grub_ata_setlba (dev, sector >> 24, size >> 8);
      /* Set "Current".  */
      grub_ata_setlba (dev, sector, size);

      break;
    }

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_ata_readwrite (grub_disk_t disk, grub_disk_addr_t sector,
		    grub_size_t size, char *buf, int rw)
{
  struct grub_ata_device *dev = (struct grub_ata_device *) disk->data;

  grub_dprintf("ata", "grub_ata_readwrite (size=%u, rw=%d)\n", size, rw);

  grub_ata_addressing_t addressing = dev->addr;
  grub_size_t batch;
  int cmd, cmd_write;

  if (addressing == GRUB_ATA_LBA48 && ((sector + size) >> 28) != 0)
    {
      batch = 65536;
      cmd = GRUB_ATA_CMD_READ_SECTORS_EXT;
      cmd_write = GRUB_ATA_CMD_WRITE_SECTORS_EXT;
    }
  else
    {
      if (addressing == GRUB_ATA_LBA48)
	addressing = GRUB_ATA_LBA;
      batch = 256;
      cmd = GRUB_ATA_CMD_READ_SECTORS;
      cmd_write = GRUB_ATA_CMD_WRITE_SECTORS;
    }

  grub_size_t nsectors = 0;
  while (nsectors < size)
    {
      if (size - nsectors < batch)
	batch = size - nsectors;

      grub_dprintf("ata", "rw=%d, sector=%llu, batch=%u\n", rw, sector, batch);

      /* Send read/write command.  */
      if (grub_ata_setaddress (dev, addressing, sector, batch))
	return grub_errno;

      grub_ata_regset (dev, GRUB_ATA_REG_CMD, (! rw ? cmd : cmd_write));

      unsigned sect;
      for (sect = 0; sect < batch; sect++)
	{
	  /* Wait for !BSY, DRQ.  */
	  if (grub_ata_wait_drq (dev, rw, GRUB_ATA_TOUT_DATA))
	    return grub_errno;

	  /* Transfer data.  */
	  if (! rw)
	    grub_ata_pio_read (dev, buf, GRUB_DISK_SECTOR_SIZE);
	  else
	    grub_ata_pio_write (dev, buf, GRUB_DISK_SECTOR_SIZE);

	  buf += GRUB_DISK_SECTOR_SIZE;
	}

      if (rw)
        {
	  /* Check for write error.  */
	  if (grub_ata_wait_not_busy (dev, GRUB_ATA_TOUT_DATA))
	    return grub_errno;

	  if (grub_ata_regget (dev, GRUB_ATA_REG_STATUS)
	      & (GRUB_ATA_STATUS_DRQ | GRUB_ATA_STATUS_ERR))
	    return grub_error (GRUB_ERR_WRITE_ERROR, "ATA write error");
	}

      sector += batch;
      nsectors += batch;
    }

  return GRUB_ERR_NONE;
}



static int
grub_ata_iterate (int (*hook) (const char *name))
{
  struct grub_ata_device *dev;

  for (dev = grub_ata_devices; dev; dev = dev->next)
    {
      char devname[5];
      grub_sprintf (devname, "ata%d", dev->port * 2 + dev->device);

      if (dev->atapi)
	continue;

      if (hook (devname))
	return 1;
    }

  return 0;
}

static grub_err_t
grub_ata_open (const char *name, grub_disk_t disk)
{
  struct grub_ata_device *dev;

  for (dev = grub_ata_devices; dev; dev = dev->next)
    {
      char devname[5];
      grub_sprintf (devname, "ata%d", dev->port * 2 + dev->device);
      if (grub_strcmp (name, devname) == 0)
	break;
    }

  if (! dev)
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "Can't open device");

  if (dev->atapi)
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "not an ATA harddisk");

  disk->total_sectors = dev->size;

  disk->id = (unsigned long) dev;

  disk->has_partitions = 1;
  disk->data = dev;

  return 0;
}

static void
grub_ata_close (grub_disk_t disk __attribute__((unused)))
{

}

static grub_err_t
grub_ata_read (grub_disk_t disk, grub_disk_addr_t sector,
	       grub_size_t size, char *buf)
{
  return grub_ata_readwrite (disk, sector, size, buf, 0);
}

static grub_err_t
grub_ata_write (grub_disk_t disk,
		grub_disk_addr_t sector,
		grub_size_t size,
		const char *buf)
{
  return grub_ata_readwrite (disk, sector, size, (char *) buf, 1);
}

static struct grub_disk_dev grub_atadisk_dev =
  {
    .name = "ATA",
    .id = GRUB_DISK_DEVICE_ATA_ID,
    .iterate = grub_ata_iterate,
    .open = grub_ata_open,
    .close = grub_ata_close,
    .read = grub_ata_read,
    .write = grub_ata_write,
    .next = 0
  };



/* ATAPI code.  */

static int
grub_atapi_iterate (int (*hook) (const char *name, int luns))
{
  struct grub_ata_device *dev;

  for (dev = grub_ata_devices; dev; dev = dev->next)
    {
      char devname[7];
      grub_sprintf (devname, "ata%d", dev->port * 2 + dev->device);

      if (! dev->atapi)
	continue;

      if (hook (devname, 1))
	return 1;
    }

  return 0;

}

static grub_err_t
grub_atapi_read (struct grub_scsi *scsi,
		 grub_size_t cmdsize __attribute__((unused)),
		 char *cmd, grub_size_t size, char *buf)
{
  struct grub_ata_device *dev = (struct grub_ata_device *) scsi->data;

  grub_dprintf("ata", "grub_atapi_read (size=%u)\n", size);

  if (grub_atapi_packet (dev, cmd, size))
    return grub_errno;

  grub_size_t nread = 0;
  while (nread < size)
    {
      /* Wait for !BSY, DRQ, I/O, !C/D.  */
      if (grub_atapi_wait_drq (dev, GRUB_ATAPI_IREASON_DATA_IN, GRUB_ATA_TOUT_DATA))
	return grub_errno;

      /* Get byte count for this DRQ assertion.  */
      unsigned cnt = grub_ata_regget (dev, GRUB_ATAPI_REG_CNTHIGH) << 8
		   | grub_ata_regget (dev, GRUB_ATAPI_REG_CNTLOW);
      grub_dprintf("ata", "DRQ count=%u\n", cnt);

      /* Count of last transfer may be uneven.  */
      if (! (0 < cnt && cnt <= size - nread && (! (cnt & 1) || cnt == size - nread)))
	return grub_error (GRUB_ERR_READ_ERROR, "Invalid ATAPI transfer count");

      /* Read the data.  */
      grub_ata_pio_read (dev, buf + nread, cnt);

      if (cnt & 1)
	buf[nread + cnt - 1] = (char) grub_le_to_cpu16 (grub_inw (dev->ioaddress + GRUB_ATA_REG_DATA));

      nread += cnt;
    }

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_atapi_write (struct grub_scsi *scsi __attribute__((unused)),
		  grub_size_t cmdsize __attribute__((unused)),
		  char *cmd __attribute__((unused)),
		  grub_size_t size __attribute__((unused)),
		  char *buf __attribute__((unused)))
{
  // XXX: scsi.mod does not use write yet.
  return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET, "ATAPI write not implemented");
}

static grub_err_t
grub_atapi_open (const char *name, struct grub_scsi *scsi)
{
  struct grub_ata_device *dev;
  struct grub_ata_device *devfnd = 0;

  for (dev = grub_ata_devices; dev; dev = dev->next)
    {
      char devname[7];
      grub_sprintf (devname, "ata%d", dev->port * 2 + dev->device);

      if (!grub_strcmp (devname, name))
	{
	  devfnd = dev;
	  break;
	}
    }

  grub_dprintf ("ata", "opening ATAPI dev `%s'\n", name);

  if (! devfnd)
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "No such ATAPI device");

  scsi->data = devfnd;

  return GRUB_ERR_NONE;
}

static void
grub_atapi_close (struct grub_scsi *scsi)
{
  grub_free (scsi->name);
}

static struct grub_scsi_dev grub_atapi_dev =
  {
    .name = "ATAPI",
    .iterate = grub_atapi_iterate,
    .open = grub_atapi_open,
    .close = grub_atapi_close,
    .read = grub_atapi_read,
    .write = grub_atapi_write
  };



GRUB_MOD_INIT(ata)
{
  /* To prevent two drivers operating on the same disks.  */
  grub_disk_firmware_is_tainted = 1;
  if (grub_disk_firmware_fini)
    {
      grub_disk_firmware_fini ();
      grub_disk_firmware_fini = NULL;
    }

  /* ATA initialization.  */
  grub_ata_initialize ();

  grub_disk_dev_register (&grub_atadisk_dev);

  /* ATAPI devices are handled by scsi.mod.  */
  grub_scsi_dev_register (&grub_atapi_dev);
}

GRUB_MOD_FINI(ata)
{
  grub_scsi_dev_unregister (&grub_atapi_dev);
  grub_disk_dev_unregister (&grub_atadisk_dev);
}
