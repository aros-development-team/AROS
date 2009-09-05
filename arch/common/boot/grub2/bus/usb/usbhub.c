/* usb.c - USB Hub Support.  */
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
#include <grub/misc.h>

/* USB Supports 127 devices, with device 0 as special case.  */
static struct grub_usb_device *grub_usb_devs[128];

/* Add a device that currently has device number 0 and resides on
   CONTROLLER, the Hub reported that the device speed is SPEED.  */
static grub_usb_device_t
grub_usb_hub_add_dev (grub_usb_controller_t controller, grub_usb_speed_t speed)
{
  grub_usb_device_t dev;
  int i;

  dev = grub_zalloc (sizeof (struct grub_usb_device));
  if (! dev)
    return NULL;

  dev->controller = *controller;
  dev->speed = speed;

  grub_usb_device_initialize (dev);

  /* Assign a new address to the device.  */
  for (i = 1; i < 128; i++)
    {
      if (! grub_usb_devs[i])
	break;
    }
  if (grub_usb_devs[i])
    {
      grub_error (GRUB_ERR_IO, "Can't assign address to USB device");
      return NULL;
    }

  grub_usb_control_msg (dev,
			(GRUB_USB_REQTYPE_OUT
			 | GRUB_USB_REQTYPE_STANDARD
			 | GRUB_USB_REQTYPE_TARGET_DEV),
			GRUB_USB_REQ_SET_ADDRESS,
			i, 0, 0, NULL);
  dev->addr = i;
  dev->initialized = 1;
  grub_usb_devs[i] = dev;

  return dev;
}


static grub_err_t
grub_usb_add_hub (grub_usb_device_t dev)
{
  struct grub_usb_usb_hubdesc hubdesc;
  grub_err_t err;
  int i;

  grub_usb_control_msg (dev, (GRUB_USB_REQTYPE_IN
			      | GRUB_USB_REQTYPE_CLASS
			      | GRUB_USB_REQTYPE_TARGET_DEV),
			GRUB_USB_REQ_GET_DESCRIPTOR,
			(GRUB_USB_DESCRIPTOR_HUB << 8) | 0,
			0, sizeof (hubdesc), (char *) &hubdesc);

  /* Iterate over the Hub ports.  */
  for (i = 1; i <= hubdesc.portcnt; i++)
    {
      grub_uint32_t status;

      /* Get the port status.  */
      err = grub_usb_control_msg (dev, (GRUB_USB_REQTYPE_IN
					| GRUB_USB_REQTYPE_CLASS
					| GRUB_USB_REQTYPE_TARGET_OTHER),
				  GRUB_USB_REQ_HUB_GET_PORT_STATUS,
				  0, i, sizeof (status), (char *) &status);

      /* Just ignore the device if the Hub does not report the
	 status.  */
      if (err)
	continue;

      /* If connected, reset and enable the port.  */
      if (status & GRUB_USB_HUB_STATUS_CONNECTED)
	{
	  grub_usb_speed_t speed;

	  /* Determine the device speed.  */
	  if (status & GRUB_USB_HUB_STATUS_LOWSPEED)
	    speed = GRUB_USB_SPEED_LOW;
	  else
	    {
	      if (status & GRUB_USB_HUB_STATUS_HIGHSPEED)
		speed = GRUB_USB_SPEED_HIGH;
	      else
		speed = GRUB_USB_SPEED_FULL;
	    }

	  /* A device is actually connected to this port, not enable
	     the port.  XXX: Why 0x03?  According to some docs it
	     should be 0x0.  Check the specification!  */
	  err = grub_usb_control_msg (dev, (GRUB_USB_REQTYPE_OUT
					    | GRUB_USB_REQTYPE_CLASS
					    | GRUB_USB_REQTYPE_TARGET_OTHER),
				      0x3, 0x4, i, 0, 0);

	  /* If the Hub does not cooperate for this port, just skip
	     the port.  */
	  if (err)
	    continue;

	  /* Add the device and assign a device address to it.  */
	  grub_usb_hub_add_dev (&dev->controller, speed);
	}
    }

  return GRUB_ERR_NONE;
}

grub_usb_err_t
grub_usb_root_hub (grub_usb_controller_t controller)
{
  grub_err_t err;
  int ports;
  int i;

  /* Query the number of ports the root Hub has.  */
  ports = controller->dev->hubports (controller);

  for (i = 0; i < ports; i++)
    {
      grub_usb_speed_t speed = controller->dev->detect_dev (controller, i);

      if (speed != GRUB_USB_SPEED_NONE)
	{
	  grub_usb_device_t dev;

	  /* Enable the port.  */
	  err = controller->dev->portstatus (controller, i, 1);
	  if (err)
	    continue;

	  /* Enable the port and create a device.  */
	  dev = grub_usb_hub_add_dev (controller, speed);
	  if (! dev)
	    continue;

	  /* If the device is a Hub, scan it for more devices.  */
	  if (dev->descdev.class == 0x09)
	    grub_usb_add_hub (dev);
	}
    }

  return GRUB_USB_ERR_NONE;
}

int
grub_usb_iterate (int (*hook) (grub_usb_device_t dev))
{
  int i;

  for (i = 0; i < 128; i++)
    {
      if (grub_usb_devs[i])
	{
	  if (hook (grub_usb_devs[i]))
	      return 1;
	}
    }

  return 0;
}
