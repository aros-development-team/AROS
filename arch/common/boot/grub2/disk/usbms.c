/* usbms.c - USB Mass Storage Support.  */
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

#include <grub/dl.h>
#include <grub/mm.h>
#include <grub/usb.h>
#include <grub/scsi.h>
#include <grub/scsicmd.h>
#include <grub/misc.h>

#define GRUB_USBMS_DIRECTION_BIT	7

/* The USB Mass Storage Command Block Wrapper.  */
struct grub_usbms_cbw
{
  grub_uint32_t signature;
  grub_uint32_t tag;
  grub_uint32_t transfer_length;
  grub_uint8_t flags;
  grub_uint8_t lun;
  grub_uint8_t length;
  grub_uint8_t cbwcb[16];
} __attribute__ ((packed));

struct grub_usbms_csw
{
  grub_uint32_t signature;
  grub_uint32_t tag;
  grub_uint32_t residue;
  grub_uint8_t status;
} __attribute__ ((packed));

struct grub_usbms_dev
{
  struct grub_usb_device *dev;

  int luns;

  int interface;
  struct grub_usb_desc_endp *in;
  struct grub_usb_desc_endp *out;

  int in_maxsz;
  int out_maxsz;

  struct grub_usbms_dev *next;
};
typedef struct grub_usbms_dev *grub_usbms_dev_t;

static grub_usbms_dev_t grub_usbms_dev_list;

static int devcnt;

static grub_err_t
grub_usbms_reset (grub_usb_device_t dev, int interface)
{
  return grub_usb_control_msg (dev, 0x21, 255, 0, interface, 0, 0);
}

static void
grub_usbms_finddevs (void)
{
  auto int usb_iterate (grub_usb_device_t dev);

  int usb_iterate (grub_usb_device_t usbdev)
    {
      grub_usb_err_t err;
      struct grub_usb_desc_device *descdev = &usbdev->descdev;
      int i;

      if (descdev->class != 0 || descdev->subclass || descdev->protocol != 0)
	return 0;

      /* XXX: Just check configuration 0 for now.  */
      for (i = 0; i < usbdev->config[0].descconf->numif; i++)
	{
	  struct grub_usbms_dev *usbms;
	  struct grub_usb_desc_if *interf;
	  int j;
	  grub_uint8_t luns;

	  interf = usbdev->config[0].interf[i].descif;

	  /* If this is not a USB Mass Storage device with a supported
	     protocol, just skip it.  */
	  if (interf->class != GRUB_USB_CLASS_MASS_STORAGE
	      || interf->subclass != GRUB_USBMS_SUBCLASS_BULK
	      || interf->protocol != GRUB_USBMS_PROTOCOL_BULK)
	    {
	      continue;
	    }

	  devcnt++;
	  usbms = grub_zalloc (sizeof (struct grub_usbms_dev));
	  if (! usbms)
	    return 1;

	  usbms->dev = usbdev;
	  usbms->interface = i;

	  /* Iterate over all endpoints of this interface, at least a
	     IN and OUT bulk endpoint are required.  */
	  for (j = 0; j < interf->endpointcnt; j++)
	    {
	      struct grub_usb_desc_endp *endp;
	      endp = &usbdev->config[0].interf[i].descendp[j];

	      if ((endp->endp_addr & 128) && (endp->attrib & 3) == 2)
		{
		  /* Bulk IN endpoint.  */
		  usbms->in = endp;
		  grub_usb_clear_halt (usbdev, endp->endp_addr & 128);
		  usbms->in_maxsz = endp->maxpacket;
		}
	      else if (!(endp->endp_addr & 128) && (endp->attrib & 3) == 2)
		{
		  /* Bulk OUT endpoint.  */
		  usbms->out = endp;
		  grub_usb_clear_halt (usbdev, endp->endp_addr & 128);
		  usbms->out_maxsz = endp->maxpacket;
		}
	    }

	  if (!usbms->in || !usbms->out)
	    {
	      grub_free (usbms);
	      return 0;
	    }

	  /* Query the amount of LUNs.  */
	  err = grub_usb_control_msg (usbdev, 0xA1, 254,
				      0, i, 1, (char *) &luns);
	  if (err)
	    {
	      /* In case of a stall, clear the stall.  */
	      if (err == GRUB_USB_ERR_STALL)
		{
		  grub_usb_clear_halt (usbdev, usbms->in->endp_addr & 3);
		  grub_usb_clear_halt (usbdev, usbms->out->endp_addr & 3);
		}

	      /* Just set the amount of LUNs to one.  */
	      grub_errno = GRUB_ERR_NONE;
	      usbms->luns = 1;
	    }
	  else
	    usbms->luns = luns;

	  /* XXX: Check the magic values, does this really make
	     sense?  */
	  grub_usb_control_msg (usbdev, (1 << 6) | 1, 255,
				0, i, 0, 0);

	  /* XXX: To make Qemu work?  */
	  if (usbms->luns == 0)
	    usbms->luns = 1;

	  usbms->next = grub_usbms_dev_list;
	  grub_usbms_dev_list = usbms;

	  /* XXX: Activate the first configuration.  */
	  grub_usb_set_configuration (usbdev, 1);

	  /* Bulk-Only Mass Storage Reset, after the reset commands
	     will be accepted.  */
	  grub_usbms_reset (usbdev, i);

	  return 0;
	}

      return 0;
    }

  grub_usb_iterate (usb_iterate);
}



static int
grub_usbms_iterate (int (*hook) (const char *name, int luns))
{
  grub_usbms_dev_t p;
  int cnt = 0;

  for (p = grub_usbms_dev_list; p; p = p->next)
    {
      char devname[20];
      grub_sprintf (devname, "usb%d", cnt);

      if (hook (devname, p->luns))
	return 1;
      cnt++;
    }

  return 0;
}

static grub_err_t
grub_usbms_transfer (struct grub_scsi *scsi, grub_size_t cmdsize, char *cmd,
		     grub_size_t size, char *buf, int read_write)
{
  struct grub_usbms_cbw cbw;
  grub_usbms_dev_t dev = (grub_usbms_dev_t) scsi->data;
  struct grub_usbms_csw status;
  static grub_uint32_t tag = 0;
  grub_usb_err_t err = GRUB_USB_ERR_NONE;
  int retrycnt = 3 + 1;

 retry:
  retrycnt--;
  if (retrycnt == 0)
    return grub_error (GRUB_ERR_IO, "USB Mass Storage stalled");

  /* Setup the request.  */
  grub_memset (&cbw, 0, sizeof (cbw));
  cbw.signature = grub_cpu_to_le32 (0x43425355);
  cbw.tag = tag++;
  cbw.transfer_length = grub_cpu_to_le32 (size);
  cbw.flags = (!read_write) << GRUB_USBMS_DIRECTION_BIT;
  cbw.lun = scsi->lun << GRUB_SCSI_LUN_SHIFT;
  cbw.length = cmdsize;
  grub_memcpy (cbw.cbwcb, cmd, cmdsize);

  /* Write the request.  */
  err = grub_usb_bulk_write (dev->dev, dev->out->endp_addr & 15,
			     sizeof (cbw), (char *) &cbw);
  if (err)
    {
      if (err == GRUB_USB_ERR_STALL)
	{
	  grub_usb_clear_halt (dev->dev, dev->out->endp_addr);
	  goto retry;
	}
      return grub_error (GRUB_ERR_IO, "USB Mass Storage request failed");
    }

  /* Read/write the data.  */
  if (read_write == 0)
    {
      err = grub_usb_bulk_read (dev->dev, dev->in->endp_addr & 15, size, buf);
      grub_dprintf ("usb", "read: %d %d\n", err, GRUB_USB_ERR_STALL);
      if (err)
	{
	  if (err == GRUB_USB_ERR_STALL)
	    {
	      grub_usb_clear_halt (dev->dev, dev->in->endp_addr);
	      goto retry;
	    }
	  return grub_error (GRUB_ERR_READ_ERROR,
			     "can't read from USB Mass Storage device");
	}
    }
  else
    {
      err = grub_usb_bulk_write (dev->dev, dev->in->endp_addr & 15, size, buf);
      grub_dprintf ("usb", "write: %d %d\n", err, GRUB_USB_ERR_STALL);
      if (err)
	{
	  if (err == GRUB_USB_ERR_STALL)
	    {
	      grub_usb_clear_halt (dev->dev, dev->out->endp_addr);
	      goto retry;
	    }
	  return grub_error (GRUB_ERR_WRITE_ERROR,
			     "can't write to USB Mass Storage device");
	}
    }

  /* Read the status.  */
  err = grub_usb_bulk_read (dev->dev, dev->in->endp_addr & 15,
			    sizeof (status), (char *) &status);
  if (err)
    {
      if (err == GRUB_USB_ERR_STALL)
	{
	  grub_usb_clear_halt (dev->dev, dev->in->endp_addr);
	  goto retry;
	}
      return grub_error (GRUB_ERR_READ_ERROR,
			 "can't read status from USB Mass Storage device");
    }

  /* XXX: Magic and check this code.  */
  if (status.status == 2)
    {
      /* XXX: Phase error, reset device.  */
      grub_usbms_reset (dev->dev, dev->interface);
      grub_usb_clear_halt (dev->dev, dev->in->endp_addr);
      grub_usb_clear_halt (dev->dev, dev->out->endp_addr);

      goto retry;
    }

  if (status.status)
    return grub_error (GRUB_ERR_READ_ERROR,
		       "error communication with USB Mass Storage device");

  return GRUB_ERR_NONE;
}


static grub_err_t
grub_usbms_read (struct grub_scsi *scsi, grub_size_t cmdsize, char *cmd,
		 grub_size_t size, char *buf)
{
  return grub_usbms_transfer (scsi, cmdsize, cmd, size, buf, 0);
}

static grub_err_t
grub_usbms_write (struct grub_scsi *scsi, grub_size_t cmdsize, char *cmd,
		  grub_size_t size, char *buf)
{
  return grub_usbms_transfer (scsi, cmdsize, cmd, size, buf, 1);
}

static grub_err_t
grub_usbms_open (const char *name, struct grub_scsi *scsi)
{
  grub_usbms_dev_t p;
  int devnum;
  int i = 0;

  if (grub_strncmp (name, "usb", 3))
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE,
		       "not a USB Mass Storage device");

  devnum = grub_strtoul (name + 3, NULL, 10);
  for (p = grub_usbms_dev_list; p; p = p->next)
    {
      /* Check if this is the devnumth device.  */
      if (devnum == i)
	{
	  scsi->data = p;
	  scsi->name = grub_strdup (name);
	  scsi->luns = p->luns;
	  if (! scsi->name)
	    return grub_errno;

	  return GRUB_ERR_NONE;
	}

      i++;
    }

  return grub_error (GRUB_ERR_UNKNOWN_DEVICE,
		     "not a USB Mass Storage device");
}

static void
grub_usbms_close (struct grub_scsi *scsi)
{
  grub_free (scsi->name);
}

static struct grub_scsi_dev grub_usbms_dev =
  {
    .name = "usb",
    .iterate = grub_usbms_iterate,
    .open = grub_usbms_open,
    .close = grub_usbms_close,
    .read = grub_usbms_read,
    .write = grub_usbms_write
  };

GRUB_MOD_INIT(usbms)
{
  grub_usbms_finddevs ();
  grub_scsi_dev_register (&grub_usbms_dev);
}

GRUB_MOD_FINI(usbms)
{
  grub_scsi_dev_unregister (&grub_usbms_dev);
}
