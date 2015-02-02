/*  usb.c -- libusb USB support for GRUB.  */
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

#include <config.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <usb.h>
#include <grub/usb.h>
#include <grub/dl.h>

GRUB_MOD_LICENSE ("GPLv3+");


static struct grub_usb_controller_dev usb_controller =
{
  .name = "libusb"
};

static struct grub_usb_device *grub_usb_devs[128];

struct usb_bus *busses;

static grub_err_t
grub_libusb_devices (void)

{
  struct usb_bus *bus;
  int last = 0;

  busses = usb_get_busses();

  for (bus = busses; bus; bus = bus->next)
    {
      struct usb_device *usbdev;
      struct grub_usb_device *dev;

      for (usbdev = bus->devices; usbdev; usbdev = usbdev->next)
	{
	  struct usb_device_descriptor *desc = &usbdev->descriptor;
	  grub_err_t err;

	  if (! desc->bcdUSB)
	    continue;

	  dev = grub_malloc (sizeof (*dev));
	  if (! dev)
	    return grub_errno;

	  dev->data = usbdev;

	  /* Fill in all descriptors.  */
	  err = grub_usb_device_initialize (dev);
	  if (err)
	    {
	      grub_errno = GRUB_ERR_NONE;
	      continue;
	    }

	  /* Register the device.  */
	  grub_usb_devs[last++] = dev;
	}
    }

  return GRUB_USB_ERR_NONE;
}

void
grub_usb_poll_devices (void)
{
  /* TODO: recheck grub_usb_devs */
}


int
grub_usb_iterate (grub_usb_iterate_hook_t hook, void *hook_data)
{
  int i;

  for (i = 0; i < 128; i++)
    {
      if (grub_usb_devs[i])
	{
	  if (hook (grub_usb_devs[i], hook_data))
	      return 1;
	}
    }

  return 0;
}

grub_usb_err_t
grub_usb_root_hub (grub_usb_controller_t controller __attribute__((unused)))
{
  return GRUB_USB_ERR_NONE;
}

grub_usb_err_t
grub_usb_control_msg (grub_usb_device_t dev, grub_uint8_t reqtype,
		      grub_uint8_t request, grub_uint16_t value,
		      grub_uint16_t idx, grub_size_t size, char *data)
{
  usb_dev_handle *devh;
  struct usb_device *d = dev->data;

  devh = usb_open (d);
  if (usb_control_msg (devh, reqtype, request,
		       value, idx, data, size, 20) < 0)
    {
      usb_close (devh);
      return GRUB_USB_ERR_STALL;
    }

  usb_close (devh);

  return GRUB_USB_ERR_NONE;
}

grub_usb_err_t
grub_usb_bulk_read (grub_usb_device_t dev,
		    int endpoint, grub_size_t size, char *data)
{
  usb_dev_handle *devh;
  struct usb_device *d = dev->data;

  devh = usb_open (d);
  if (usb_claim_interface (devh, 0) < 1)
    {
      usb_close (devh);
      return GRUB_USB_ERR_STALL;
    }

  if (usb_bulk_read (devh, endpoint, data, size, 20) < 1)
    {
      usb_close (devh);
      return GRUB_USB_ERR_STALL;
    }

  usb_release_interface (devh, 0);
  usb_close (devh);

  return GRUB_USB_ERR_NONE;
}

grub_usb_err_t
grub_usb_bulk_write (grub_usb_device_t dev,
		     int endpoint, grub_size_t size, char *data)
{
  usb_dev_handle *devh;
  struct usb_device *d = dev->data;

  devh = usb_open (d);
  if (usb_claim_interface (devh, 0) < 0)
    goto fail;

  if (usb_bulk_write (devh, endpoint, data, size, 20) < 0)
    goto fail;

  if (usb_release_interface (devh, 0) < 0)
    goto fail;

  usb_close (devh);

  return GRUB_USB_ERR_NONE;

 fail:
  usb_close (devh);
  return GRUB_USB_ERR_STALL;
}

GRUB_MOD_INIT (libusb)
{
  usb_init();
  usb_find_busses();
  usb_find_devices();

  if (grub_libusb_devices ())
    return;

  grub_usb_controller_dev_register (&usb_controller);

  return;
}

GRUB_MOD_FINI (libusb)
{
  return;
}
