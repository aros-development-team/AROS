/* ata.c - ATA disk access.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2007  Free Software Foundation, Inc.
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

#include <grub/dl.h>
#include <grub/misc.h>
#include <grub/disk.h>
#include <grub/mm.h>
#include <grub/time.h>
/* XXX: For now this only works on i386.  */
#include <grub/cpu/io.h>

typedef enum
  {
    GRUB_ATA_CHS,
    GRUB_ATA_LBA,
    GRUB_ATA_LBA48
  } grub_ata_addressing_t;

/* At the moment, only two IDE ports are supported.  */
static const int grub_ata_ioaddress[] = { 0x1f0, 0x170 };
static const int grub_ata_ioaddress2[] = { 0x3f6, 0x376 };

#define GRUB_CDROM_SECTOR_SIZE	2048

#define GRUB_ATA_REG_DATA	0
#define GRUB_ATA_REG_ERROR	1
#define GRUB_ATA_REG_FEATURES	1
#define GRUB_ATA_REG_SECTORS	2
#define GRUB_ATA_REG_SECTNUM	3
#define GRUB_ATA_REG_CYLLSB	4
#define GRUB_ATA_REG_CYLMSB	5
#define GRUB_ATA_REG_LBALOW	3
#define GRUB_ATA_REG_LBAMID	4
#define GRUB_ATA_REG_LBAHIGH	5
#define GRUB_ATA_REG_DISK	6
#define GRUB_ATA_REG_CMD	7
#define GRUB_ATA_REG_STATUS	7

#define GRUB_ATA_REG2_CONTROL	0

enum grub_ata_commands
  {
    GRUB_ATA_CMD_READ_SECTORS = 0x20,
    GRUB_ATA_CMD_READ_SECTORS_EXT = 0x24,
    GRUB_ATA_CMD_WRITE_SECTORS = 0x30,
    GRUB_ATA_CMD_WRITE_SECTORS_EXT = 0x34,
    GRUB_ATA_CMD_IDENTIFY_DEVICE = 0xEC,
    GRUB_ATA_CMD_IDENTIFY_PACKET_DEVICE = 0xA1,
    GRUB_ATA_CMD_PACKET = 0xA0
  };

struct grub_ata_device
{
  /* IDE port to use.  */
  int port;

  /* IO addresses on which the registers for this device can be
     found.  */
  int ioaddress;
  int ioaddress2;

  /* Two devices can be connected to a single cable.  Use this field
     to select device 0 (commonly known as "master") or device 1
     (commonly known as "slave").  */
  int device;

  /* Addressing methods available for accessing this device.  If CHS
     is only available, use that.  Otherwise use LBA, except for the
     high sectors.  In that case use LBA48.  */
  grub_ata_addressing_t addr;

  /* Sector count.  */
  grub_uint64_t size;

  /* CHS maximums.  */
  grub_uint16_t cylinders;
  grub_uint16_t heads;
  grub_uint16_t sectors_per_track;

  /* Set to 0 for ATA, set to 1 for ATAPI.  */
  int atapi;

  struct grub_ata_device *next;
};

static struct grub_ata_device *grub_ata_devices;

static inline void
grub_ata_regset (struct grub_ata_device *dev, int reg, int val)
{
  grub_outb (val, dev->ioaddress + reg);
}

static inline int
grub_ata_regget (struct grub_ata_device *dev, int reg)
{
  return grub_inb (dev->ioaddress + reg);
}

static inline void
grub_ata_regset2 (struct grub_ata_device *dev, int reg, int val)
{
  grub_outb (val, dev->ioaddress2 + reg);
}

static inline int
grub_ata_regget2 (struct grub_ata_device *dev, int reg)
{
  return grub_inb (dev->ioaddress2 + reg);
}

/* Wait until the device DEV has the status set to ready.  */
static inline void
grub_ata_wait_busy (struct grub_ata_device *dev)
{
  while ((grub_ata_regget (dev, GRUB_ATA_REG_STATUS) & 0x80));
}

static inline void
grub_ata_wait_drq (struct grub_ata_device *dev)
{
  while (! (grub_ata_regget (dev, GRUB_ATA_REG_STATUS) & 0x08));
}

static inline void
grub_ata_wait (void)
{
  grub_millisleep (50);
}

/* Byteorder has to be changed before strings can be read.  */
static inline void
grub_ata_strncpy (char *dst, char *src, grub_size_t len)
{
  grub_uint16_t *src16 = (grub_uint16_t *) src;
  grub_uint16_t *dst16 = (grub_uint16_t *) dst;
  unsigned int i;

  for (i = 0; i < len / 2; i++)
    *(dst16++) = grub_be_to_cpu16(*(src16++));
  dst[len] = '\0';
}

static int
grub_ata_pio_read (struct grub_ata_device *dev, char *buf,
		   grub_size_t size)
{
  grub_uint16_t *buf16 = (grub_uint16_t *) buf;
  unsigned int i;

  if (grub_ata_regget (dev, GRUB_ATA_REG_STATUS) & 1)
    return grub_ata_regget (dev, GRUB_ATA_REG_ERROR);

  /* Wait until the data is available.  */
  grub_ata_wait_drq (dev);

  /* Read in the data, word by word.  */
  for (i = 0; i < size / 2; i++)
    buf16[i] = grub_le_to_cpu16 (grub_inw(dev->ioaddress + GRUB_ATA_REG_DATA));

  if (grub_ata_regget (dev, GRUB_ATA_REG_STATUS) & 1)
    return grub_ata_regget (dev, GRUB_ATA_REG_ERROR);

  return 0;
}

static grub_err_t
grub_ata_pio_write (struct grub_ata_device *dev, char *buf,
		    grub_size_t size)
{
  grub_uint16_t *buf16 = (grub_uint16_t *) buf;
  unsigned int i;

  /* Wait until the device is ready to write.  */
  grub_ata_wait_drq (dev);

  /* Write the data, word by word.  */
  for (i = 0; i < size / 2; i++)
    grub_outw(grub_cpu_to_le16 (buf16[i]), dev->ioaddress + GRUB_ATA_REG_DATA);

  if (grub_ata_regget (dev, GRUB_ATA_REG_STATUS) & 1)
    return grub_ata_regget (dev, GRUB_ATA_REG_ERROR);

  return 0;
}

static void
grub_ata_dumpinfo (struct grub_ata_device *dev, char *info)
{
  char text[41];

  /* The device information was read, dump it for debugging.  */
  grub_ata_strncpy (text, info + 20, 20);
  grub_printf ("Serial: %s\n", text);
  grub_ata_strncpy (text, info + 46, 8);
  grub_printf ("Firmware: %s\n", text);
  grub_ata_strncpy (text, info + 54, 40);
  grub_printf ("Model: %s\n", text);

  grub_printf ("Addressing: %d\n", dev->addr);
  grub_printf ("#sectors: 0x%llx\n", dev->size);
}

static grub_err_t
grub_atapi_identify (struct grub_ata_device *dev)
{
  char *info;

  info = grub_malloc (256);
  if (! info)
    return grub_errno;

  grub_ata_wait_busy (dev);

  grub_ata_regset (dev, GRUB_ATA_REG_DISK, 0xE0 | dev->device << 4);
  grub_ata_regset (dev, GRUB_ATA_REG_CMD,
		   GRUB_ATA_CMD_IDENTIFY_PACKET_DEVICE);
  grub_ata_wait ();

  grub_ata_pio_read (dev, info, 256);

  dev->atapi = 1;

  grub_ata_dumpinfo (dev, info);

  grub_free (info);

  return 0;
}

static grub_err_t
grub_atapi_packet (struct grub_ata_device *dev, char *packet)
{
  grub_ata_regset (dev, GRUB_ATA_REG_DISK, dev->device << 4);
  grub_ata_regset (dev, GRUB_ATA_REG_FEATURES, 0);
  grub_ata_regset (dev, GRUB_ATA_REG_SECTORS, 0);
  grub_ata_regset (dev, GRUB_ATA_REG_LBAHIGH, 0xFF);
  grub_ata_regset (dev, GRUB_ATA_REG_LBAMID, 0xFF);
  grub_ata_regset (dev, GRUB_ATA_REG_CMD, GRUB_ATA_CMD_PACKET);
  grub_ata_wait ();

  grub_ata_pio_write (dev, packet, 12);

  return 0;
}

static grub_err_t
grub_ata_identify (struct grub_ata_device *dev)
{
  char *info;
  grub_uint16_t *info16;
  int ataerr;

  info = grub_malloc (GRUB_DISK_SECTOR_SIZE);
  if (! info)
    return grub_errno;

  info16 = (grub_uint16_t *) info;

  grub_ata_wait_busy (dev);

  grub_ata_regset (dev, GRUB_ATA_REG_DISK, 0xE0 | dev->device << 4);
  grub_ata_regset (dev, GRUB_ATA_REG_CMD, GRUB_ATA_CMD_IDENTIFY_DEVICE);
  grub_ata_wait ();

  ataerr = grub_ata_pio_read (dev, info, GRUB_DISK_SECTOR_SIZE);
  if (ataerr & 4)
    {
      /* ATAPI device detected.  */
      grub_free(info);
      return grub_atapi_identify (dev);
    }
  else if (ataerr)
    {
      /* Error.  */
      grub_free(info);
      return grub_error (GRUB_ERR_UNKNOWN_DEVICE,
			 "device can not be identified");
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
grub_ata_initialize (void)
{
  struct grub_ata_device *dev;
  struct grub_ata_device **devp;
  int port;
  int device;

  for (port = 0; port <= 1; port++)
    {
      for (device = 0; device <= 1; device++)
	{
	  dev = grub_malloc (sizeof(*dev));
	  if (! dev)
	    return grub_errno;

	  /* Setup the device information.  */
	  dev->port = port;
	  dev->device = device;
	  dev->ioaddress = grub_ata_ioaddress[dev->port];
	  dev->ioaddress2 = grub_ata_ioaddress2[dev->port];
	  dev->next = NULL;

	  /* Try to detect if the port is in use by writing to it,
	     waiting for a while and reading it again.  If the value
	     was preserved, there is a device connected.  */
	  grub_ata_regset (dev, GRUB_ATA_REG_DISK, dev->device << 4);
	  grub_ata_wait ();
	  grub_ata_regset (dev, GRUB_ATA_REG_SECTORS, 0x5A);  
	  grub_ata_wait ();
	  if (grub_ata_regget (dev, GRUB_ATA_REG_SECTORS) != 0x5A)
	    {
	      grub_free(dev);
	      continue;
	    }

	  /* Detect if the device is present by issuing a reset.  */
	  grub_ata_regset2 (dev, GRUB_ATA_REG2_CONTROL, 6);
	  grub_ata_wait ();
	  grub_ata_regset2 (dev, GRUB_ATA_REG2_CONTROL, 2);
	  grub_ata_wait ();
	  grub_ata_regset (dev, GRUB_ATA_REG_DISK, dev->device << 4);
	  grub_ata_wait ();

	  /* XXX: Check some registers to see if the reset worked as
	     expected for this device.  */
#if 1
	  /* Enable for ATAPI .  */
	  if (grub_ata_regget (dev, GRUB_ATA_REG_CYLLSB) != 0x14
	      || grub_ata_regget (dev, GRUB_ATA_REG_CYLMSB) != 0xeb)
#endif
	  if (grub_ata_regget (dev, GRUB_ATA_REG_STATUS) == 0
	      || (grub_ata_regget (dev, GRUB_ATA_REG_CYLLSB) != 0
		  && grub_ata_regget (dev, GRUB_ATA_REG_CYLMSB) != 0
		  && grub_ata_regget (dev, GRUB_ATA_REG_CYLLSB) != 0x3c
		  && grub_ata_regget (dev, GRUB_ATA_REG_CYLLSB) != 0xc3))
	    {
	      grub_free (dev);
	      continue;
	    }

	  /* Use the IDENTIFY DEVICE command to query the device.  */
	  if (grub_ata_identify (dev))
	    {
	      grub_free (dev);
	      continue;
	    }

	  /* Register the device.  */
	  for (devp = &grub_ata_devices; *devp; devp = &(*devp)->next);
	  *devp = dev;
	}
    }

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
  grub_ata_wait_busy (dev);

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

	grub_ata_regset (dev, GRUB_ATA_REG_SECTNUM, sect);
	grub_ata_regset (dev, GRUB_ATA_REG_CYLLSB, cylinder & 0xFF);
	grub_ata_regset (dev, GRUB_ATA_REG_CYLMSB, cylinder >> 8);
	grub_ata_regset (dev, GRUB_ATA_REG_DISK, (dev->device << 4) | head);

	break;
      }

    case GRUB_ATA_LBA:
      if (size == 256)
	size = 0;
      grub_ata_setlba (dev, sector, size);
      grub_ata_regset (dev, GRUB_ATA_REG_DISK,
		       0xE0 | (dev->device << 4) | ((sector >> 24) & 0x0F));
      break;

    case GRUB_ATA_LBA48:
      if (size == 65536)
	size = 0;

      /* Set "Previous".  */
      grub_ata_setlba (dev, sector >> 24, size >> 8);
      /* Set "Current".  */
      grub_ata_setlba (dev, sector, size);
      grub_ata_regset (dev, GRUB_ATA_REG_DISK, 0xE0 | (dev->device << 4));

      break;
    }

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_ata_readwrite (grub_disk_t disk, grub_disk_addr_t sector,
		    grub_size_t size, char *buf, int rw)
{
  struct grub_ata_device *dev = (struct grub_ata_device *) disk->data;
  grub_size_t cnt;
  grub_size_t batch;
  grub_ata_addressing_t addressing;
  int cmd;
  int cmd_write;
  unsigned int sect;

  addressing = dev->addr;

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

  cnt = size / batch;

  /* Read/write batches of 256/65536 sectors, when more than 256/65536
     sectors should be read/written.  */
  for (; cnt; cnt--)
    {
      if (grub_ata_setaddress (dev, addressing, sector, batch))
	return grub_errno;

      if (rw == 0)
	{
	  /* Read 256/65536 sectors.  */
	  grub_ata_regset (dev, GRUB_ATA_REG_CMD, cmd);
	  grub_ata_wait ();
	  for (sect = 0; sect < batch; sect++)
	    {
	      if (grub_ata_pio_read (dev, buf,
				     GRUB_DISK_SECTOR_SIZE))
		return grub_error (GRUB_ERR_READ_ERROR, "ATA read error");
	      buf += GRUB_DISK_SECTOR_SIZE;
	      sector++;
	    }
	}
      else
	{
	  /* Write 256/65536 sectors.  */
	  grub_ata_regset (dev, GRUB_ATA_REG_CMD, cmd_write);
	  grub_ata_wait ();
	  for (sect = 0; sect < batch; sect++)
	    {
	      if (grub_ata_pio_write (dev, buf,
				      GRUB_DISK_SECTOR_SIZE))
		return grub_error (GRUB_ERR_WRITE_ERROR, "ATA write error");
	      buf += GRUB_DISK_SECTOR_SIZE;
	    }
	}
      sector += batch;
    }

  /* Read/write just a "few" sectors.  */
  if (grub_ata_setaddress (dev, addressing, sector, size % batch))
    return grub_errno;

  if (rw == 0)
    {
      /* Read sectors.  */
      grub_ata_regset (dev, GRUB_ATA_REG_CMD, cmd);
      grub_ata_wait ();
      for (sect = 0; sect < (size % batch); sect++)
	{
	  if (grub_ata_pio_read (dev, buf, GRUB_DISK_SECTOR_SIZE))
	    return grub_error (GRUB_ERR_READ_ERROR, "ATA read error");
	  buf += GRUB_DISK_SECTOR_SIZE;
	}
    } else {
      /* Write sectors.  */
      grub_ata_regset (dev, GRUB_ATA_REG_CMD, cmd_write);
      grub_ata_wait ();
      for (sect = 0; sect < batch; sect++)
	{
	  if (grub_ata_pio_write (dev, buf,
				  (size % batch) * GRUB_DISK_SECTOR_SIZE))
	    return grub_error (GRUB_ERR_WRITE_ERROR, "ATA write error");
	  buf += GRUB_DISK_SECTOR_SIZE;
	}
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
    disk->total_sectors = 9000000; /* XXX */
  else
    disk->total_sectors = dev->size;

  disk->id = (int) dev;
  
  disk->has_partitions = !dev->atapi;
  disk->data = dev;

  return 0;
}

static void
grub_ata_close (grub_disk_t disk __attribute__((unused)))
{
  
}

struct grub_atapi_read
{
  grub_uint8_t code;
  grub_uint8_t reserved1;
  grub_uint32_t lba;
  grub_uint32_t length;
  grub_uint8_t reserved2[2];
} __attribute__((packed));

static grub_err_t
grub_atapi_readsector (struct grub_ata_device *dev,
		       char *buf, grub_disk_addr_t sector)
{
  struct grub_atapi_read readcmd;

  readcmd.code = 0xA8;
  readcmd.lba = grub_cpu_to_be32 (sector);
  readcmd.length = grub_cpu_to_be32 (1);

  grub_atapi_packet (dev, (char *) &readcmd);
  grub_ata_wait ();
  grub_ata_pio_read (dev, buf, GRUB_CDROM_SECTOR_SIZE);

  return 0;
}

static grub_err_t
grub_ata_read (grub_disk_t disk, grub_disk_addr_t sector,
	       grub_size_t size, char *buf)
{
  struct grub_ata_device *dev = (struct grub_ata_device *) disk->data;
  int cdsector;
  char *sbuf;

  if (! dev->atapi)
    return grub_ata_readwrite (disk, sector, size, buf, 0);

  /* ATAPI is being used, so try to read from CDROM using ATAPI.  */

  sbuf = grub_malloc (GRUB_CDROM_SECTOR_SIZE);
  if (! sbuf)
    return grub_errno;

  /* CDROMs have sectors of 2048 bytes, so chop them into pieces of
     512 bytes.  */
  while (size > 0)
    {
      int rsize;
      int offset;
      int max;

      cdsector = sector >> 2;
      rsize = ((size * GRUB_DISK_SECTOR_SIZE > GRUB_CDROM_SECTOR_SIZE)
	       ? GRUB_CDROM_SECTOR_SIZE : size * GRUB_DISK_SECTOR_SIZE);
      offset = (sector & 3) * GRUB_DISK_SECTOR_SIZE;
      max = GRUB_CDROM_SECTOR_SIZE - offset;
      rsize = (rsize > max) ? max : rsize;

      grub_atapi_readsector (dev, sbuf, cdsector);
      grub_memcpy (buf + offset, sbuf, rsize);

      buf += rsize;
      size -= rsize / GRUB_DISK_SECTOR_SIZE;
      sector += rsize / GRUB_DISK_SECTOR_SIZE;
    }

  grub_free (sbuf);

  return 0;
}

static grub_err_t
grub_ata_write (grub_disk_t disk,
		grub_disk_addr_t sector,
		grub_size_t size,
		const char *buf)
{
#if 1
  return GRUB_ERR_NOT_IMPLEMENTED_YET;
#else
  return grub_ata_readwrite (disk, sector, size, (char *) buf, 1);
#endif
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



GRUB_MOD_INIT(ata)
{
  (void) mod;			/* To stop warning. */

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
}

GRUB_MOD_FINI(ata)
{
  grub_disk_dev_unregister (&grub_atadisk_dev);
}
