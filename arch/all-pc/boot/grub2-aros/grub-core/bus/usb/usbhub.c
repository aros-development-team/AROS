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
#include <grub/time.h>

#define GRUB_USBHUB_MAX_DEVICES 128

/* USB Supports 127 devices, with device 0 as special case.  */
static struct grub_usb_device *grub_usb_devs[GRUB_USBHUB_MAX_DEVICES];

static int rescan = 0;

struct grub_usb_hub
{
  struct grub_usb_hub *next;
  grub_usb_controller_t controller;
  int nports;
  struct grub_usb_device **devices;
  grub_usb_device_t dev;
};

static struct grub_usb_hub *hubs;

/* Add a device that currently has device number 0 and resides on
   CONTROLLER, the Hub reported that the device speed is SPEED.  */
static grub_usb_device_t
grub_usb_hub_add_dev (grub_usb_controller_t controller,
                      grub_usb_speed_t speed,
                      int port, int hubaddr)
{
  grub_usb_device_t dev;
  int i;
  grub_usb_err_t err;

  dev = grub_zalloc (sizeof (struct grub_usb_device));
  if (! dev)
    return NULL;

  dev->controller = *controller;
  dev->speed = speed;
  dev->port = port;
  dev->hubaddr = hubaddr;

  err = grub_usb_device_initialize (dev);
  if (err)
    {
      grub_free (dev);
      return NULL;
    }

  /* Assign a new address to the device.  */
  for (i = 1; i < GRUB_USBHUB_MAX_DEVICES; i++)
    {
      if (! grub_usb_devs[i])
	break;
    }
  if (i == GRUB_USBHUB_MAX_DEVICES)
    {
      grub_error (GRUB_ERR_IO, "can't assign address to USB device");
      for (i = 0; i < 8; i++)
        grub_free (dev->config[i].descconf);
      grub_free (dev);
      return NULL;
    }

  err = grub_usb_control_msg (dev,
			      (GRUB_USB_REQTYPE_OUT
			       | GRUB_USB_REQTYPE_STANDARD
			       | GRUB_USB_REQTYPE_TARGET_DEV),
			      GRUB_USB_REQ_SET_ADDRESS,
			      i, 0, 0, NULL);
  if (err)
    {
      for (i = 0; i < 8; i++)
        grub_free (dev->config[i].descconf);
      grub_free (dev);
      return NULL;
    }

  dev->addr = i;
  dev->initialized = 1;
  grub_usb_devs[i] = dev;

  grub_dprintf ("usb", "Added new usb device: %p, addr=%d\n",
		dev, i);
  grub_dprintf ("usb", "speed=%d, port=%d, hubaddr=%d\n",
		speed, port, hubaddr);

  /* Wait "recovery interval", spec. says 2ms */
  grub_millisleep (2);
  
  grub_usb_device_attach (dev);
  
  return dev;
}


static grub_usb_err_t
grub_usb_add_hub (grub_usb_device_t dev)
{
  struct grub_usb_usb_hubdesc hubdesc;
  grub_usb_err_t err;
  int i;
  
  err = grub_usb_control_msg (dev, (GRUB_USB_REQTYPE_IN
	  		            | GRUB_USB_REQTYPE_CLASS
			            | GRUB_USB_REQTYPE_TARGET_DEV),
                              GRUB_USB_REQ_GET_DESCRIPTOR,
			      (GRUB_USB_DESCRIPTOR_HUB << 8) | 0,
			      0, sizeof (hubdesc), (char *) &hubdesc);
  if (err)
    return err;
  grub_dprintf ("usb", "Hub descriptor:\n\t\t len:%d, typ:0x%02x, cnt:%d, char:0x%02x, pwg:%d, curr:%d\n",
                hubdesc.length, hubdesc.type, hubdesc.portcnt,
                hubdesc.characteristics, hubdesc.pwdgood,
                hubdesc.current);

  /* Activate the first configuration. Hubs should have only one conf. */
  grub_dprintf ("usb", "Hub set configuration\n");
  grub_usb_set_configuration (dev, 1);

  dev->children = grub_zalloc (hubdesc.portcnt * sizeof (dev->children[0]));
  if (!dev->children)
    return GRUB_USB_ERR_INTERNAL;
  dev->nports = hubdesc.portcnt;

  /* Power on all Hub ports.  */
  for (i = 1; i <= hubdesc.portcnt; i++)
    {
      grub_dprintf ("usb", "Power on - port %d\n", i);
      /* Power on the port and wait for possible device connect */
      grub_usb_control_msg (dev, (GRUB_USB_REQTYPE_OUT
				  | GRUB_USB_REQTYPE_CLASS
				  | GRUB_USB_REQTYPE_TARGET_OTHER),
			    GRUB_USB_REQ_SET_FEATURE,
			    GRUB_USB_HUB_FEATURE_PORT_POWER,
			    i, 0, NULL);
    }

  /* Rest will be done on next usb poll.  */
  for (i = 0; i < dev->config[0].interf[0].descif->endpointcnt;
       i++)
    {
      struct grub_usb_desc_endp *endp = NULL;
      endp = &dev->config[0].interf[0].descendp[i];

      if ((endp->endp_addr & 128) && grub_usb_get_ep_type(endp)
	  == GRUB_USB_EP_INTERRUPT)
	{
	  grub_size_t len;
	  dev->hub_endpoint = endp;
	  len = endp->maxpacket;
	  if (len > sizeof (dev->statuschange))
	    len = sizeof (dev->statuschange);
	  dev->hub_transfer
	    = grub_usb_bulk_read_background (dev, endp->endp_addr, len,
					     (char *) &dev->statuschange);
	  break;
	}
    }

  rescan = 1;

  return GRUB_ERR_NONE;
}

static void
attach_root_port (struct grub_usb_hub *hub, int portno,
		  grub_usb_speed_t speed)
{
  grub_usb_device_t dev;
  grub_err_t err;
  int total, i;
  grub_usb_speed_t current_speed = GRUB_USB_SPEED_NONE;
  int changed=0;

#if 0
/* Specification does not say about disabling of port when device
 * connected. If disabling is really necessary for some devices,
 * delete this #if 0 and related #endif */
  /* Disable the port. XXX: Why? */
  err = hub->controller->dev->portstatus (hub->controller, portno, 0);
  if (err)
    return;
#endif
  /* Wait for completion of insertion and stable power (USB spec.)
   * Should be at least 100ms, some devices requires more...
   * There is also another thing - some devices have worse contacts
   * and connected signal is unstable for some time - we should handle
   * it - but prevent deadlock in case when device is too faulty... */
  for (total = i = 0; (i < 250) && (total < 2000); i++, total++)
    {
      grub_millisleep (1);
      current_speed = hub->controller->dev->detect_dev
                        (hub->controller, portno, &changed);
      if (current_speed == GRUB_USB_SPEED_NONE)
        i = 0;
    }
  grub_dprintf ("usb", "total=%d\n", total);
  if (total >= 2000)
    return;

  /* Enable the port.  */
  err = hub->controller->dev->portstatus (hub->controller, portno, 1);
  if (err)
    return;
  hub->controller->dev->pending_reset = grub_get_time_ms () + 5000;

  grub_millisleep (10);

  /* Enable the port and create a device.  */
  dev = grub_usb_hub_add_dev (hub->controller, speed, portno, 0);
  hub->controller->dev->pending_reset = 0;
  if (! dev)
    return;

  hub->devices[portno] = dev;

  /* If the device is a Hub, scan it for more devices.  */
  if (dev->descdev.class == 0x09)
    grub_usb_add_hub (dev);
}

grub_usb_err_t
grub_usb_root_hub (grub_usb_controller_t controller)
{
  int i;
  struct grub_usb_hub *hub;
  int changed=0;

  hub = grub_malloc (sizeof (*hub));
  if (!hub)
    return GRUB_USB_ERR_INTERNAL;

  hub->next = hubs;
  hubs = hub;
  hub->controller = grub_malloc (sizeof (*controller));
  if (!hub->controller)
    {
      grub_free (hub);
      return GRUB_USB_ERR_INTERNAL;
    }

  grub_memcpy (hub->controller, controller, sizeof (*controller));
  hub->dev = 0;

  /* Query the number of ports the root Hub has.  */
  hub->nports = controller->dev->hubports (controller);
  hub->devices = grub_zalloc (sizeof (hub->devices[0]) * hub->nports);
  if (!hub->devices)
    {
      grub_free (hub->controller);
      grub_free (hub);
      return GRUB_USB_ERR_INTERNAL;
    }

  for (i = 0; i < hub->nports; i++)
    {
      grub_usb_speed_t speed;
      if (!controller->dev->pending_reset)
        {
          speed = controller->dev->detect_dev (hub->controller, i,
					       &changed);

          if (speed != GRUB_USB_SPEED_NONE)
	    attach_root_port (hub, i, speed);
        }
    }

  return GRUB_USB_ERR_NONE;
}

static void detach_device (grub_usb_device_t dev);

static void
detach_device (grub_usb_device_t dev)
{
  unsigned i;
  int k;
  if (!dev)
    return;
  if (dev->descdev.class == GRUB_USB_CLASS_HUB)
    {
      if (dev->hub_transfer)
	grub_usb_cancel_transfer (dev->hub_transfer);

      for (i = 0; i < dev->nports; i++)
	detach_device (dev->children[i]);
      grub_free (dev->children);
    }
  for (i = 0; i < ARRAY_SIZE (dev->config); i++)
    if (dev->config[i].descconf)
      for (k = 0; k < dev->config[i].descconf->numif; k++)
	{
	  struct grub_usb_interface *inter = &dev->config[i].interf[k];
	  if (inter && inter->detach_hook)
	    inter->detach_hook (dev, i, k);
	}
  grub_usb_devs[dev->addr] = 0;
}

static void
poll_nonroot_hub (grub_usb_device_t dev)
{
  grub_usb_err_t err;
  unsigned i;
  grub_uint8_t changed;
  grub_size_t actual, len;
  int j, total;

  if (!dev->hub_transfer)
    return;

  err = grub_usb_check_transfer (dev->hub_transfer, &actual);

  if (err == GRUB_USB_ERR_WAIT)
    return;

  changed = dev->statuschange;

  len = dev->hub_endpoint->maxpacket;
  if (len > sizeof (dev->statuschange))
    len = sizeof (dev->statuschange);
  dev->hub_transfer
    = grub_usb_bulk_read_background (dev, dev->hub_endpoint->endp_addr, len,
				     (char *) &dev->statuschange);

  if (err || actual == 0 || changed == 0)
    return;

  /* Iterate over the Hub ports.  */
  for (i = 1; i <= dev->nports; i++)
    {
      grub_uint32_t status;
      grub_uint32_t current_status = 0;

      if (!(changed & (1 << i)))
	continue;

      /* Get the port status.  */
      err = grub_usb_control_msg (dev, (GRUB_USB_REQTYPE_IN
					| GRUB_USB_REQTYPE_CLASS
					| GRUB_USB_REQTYPE_TARGET_OTHER),
				  GRUB_USB_REQ_GET_STATUS,
				  0, i, sizeof (status), (char *) &status);

      grub_dprintf ("usb", "dev = %p, i = %d, status = %08x\n",
                   dev, i, status);

      if (err)
	continue;

      /* FIXME: properly handle these conditions.  */
      if (status & GRUB_USB_HUB_STATUS_C_PORT_ENABLED)
	grub_usb_control_msg (dev, (GRUB_USB_REQTYPE_OUT
				    | GRUB_USB_REQTYPE_CLASS
				    | GRUB_USB_REQTYPE_TARGET_OTHER),
			      GRUB_USB_REQ_CLEAR_FEATURE,
			      GRUB_USB_HUB_FEATURE_C_PORT_ENABLED, i, 0, 0);

      if (status & GRUB_USB_HUB_STATUS_C_PORT_SUSPEND)
	grub_usb_control_msg (dev, (GRUB_USB_REQTYPE_OUT
				    | GRUB_USB_REQTYPE_CLASS
				    | GRUB_USB_REQTYPE_TARGET_OTHER),
			      GRUB_USB_REQ_CLEAR_FEATURE,
			      GRUB_USB_HUB_FEATURE_C_PORT_SUSPEND, i, 0, 0);

      if (status & GRUB_USB_HUB_STATUS_C_PORT_OVERCURRENT)
	grub_usb_control_msg (dev, (GRUB_USB_REQTYPE_OUT
				    | GRUB_USB_REQTYPE_CLASS
				    | GRUB_USB_REQTYPE_TARGET_OTHER),
			      GRUB_USB_REQ_CLEAR_FEATURE,
			      GRUB_USB_HUB_FEATURE_C_PORT_OVERCURRENT, i, 0, 0);

      if (!dev->controller.dev->pending_reset &&
          (status & GRUB_USB_HUB_STATUS_C_PORT_CONNECTED))
	{
	  grub_usb_control_msg (dev, (GRUB_USB_REQTYPE_OUT
				      | GRUB_USB_REQTYPE_CLASS
				      | GRUB_USB_REQTYPE_TARGET_OTHER),
				GRUB_USB_REQ_CLEAR_FEATURE,
				GRUB_USB_HUB_FEATURE_C_PORT_CONNECTED, i, 0, 0);

	  detach_device (dev->children[i - 1]);
	  dev->children[i - 1] = NULL;
      	    
	  /* Connected and status of connection changed ? */
	  if (status & GRUB_USB_HUB_STATUS_PORT_CONNECTED)
	    {
	      /* A device is actually connected to this port. */
  /* Wait for completion of insertion and stable power (USB spec.)
   * Should be at least 100ms, some devices requires more...
   * There is also another thing - some devices have worse contacts
   * and connected signal is unstable for some time - we should handle
   * it - but prevent deadlock in case when device is too faulty... */
              for (total = j = 0; (j < 250) && (total < 2000); j++, total++)
                {
                  grub_millisleep (1);
                  /* Get the port status.  */
                  err = grub_usb_control_msg (dev, (GRUB_USB_REQTYPE_IN
					       | GRUB_USB_REQTYPE_CLASS
					       | GRUB_USB_REQTYPE_TARGET_OTHER),
				              GRUB_USB_REQ_GET_STATUS,
				              0, i,
				              sizeof (current_status),
				              (char *) &current_status);
                  if (err)
                    {
                      total = 2000;
	              break;
                    }
                  if (!(current_status & GRUB_USB_HUB_STATUS_PORT_CONNECTED))
                    j = 0;
                }
              grub_dprintf ("usb", "(non-root) total=%d\n", total);
              if (total >= 2000)
                continue;

              /* Now do reset of port. */
	      grub_usb_control_msg (dev, (GRUB_USB_REQTYPE_OUT
					  | GRUB_USB_REQTYPE_CLASS
					  | GRUB_USB_REQTYPE_TARGET_OTHER),
				    GRUB_USB_REQ_SET_FEATURE,
				    GRUB_USB_HUB_FEATURE_PORT_RESET,
				    i, 0, 0);
	      rescan = 1;
	      /* We cannot reset more than one device at the same time !
	       * Resetting more devices together results in very bad
	       * situation: more than one device has default address 0
	       * at the same time !!!
	       * Additionaly, we cannot perform another reset
	       * anywhere on the same OHCI controller until
	       * we will finish addressing of reseted device ! */
              dev->controller.dev->pending_reset = grub_get_time_ms () + 5000;
              return;
	    }
	}

      if (status & GRUB_USB_HUB_STATUS_C_PORT_RESET)
	{
	  grub_usb_control_msg (dev, (GRUB_USB_REQTYPE_OUT
				      | GRUB_USB_REQTYPE_CLASS
				      | GRUB_USB_REQTYPE_TARGET_OTHER),
				GRUB_USB_REQ_CLEAR_FEATURE,
				GRUB_USB_HUB_FEATURE_C_PORT_RESET, i, 0, 0);

	  if (status & GRUB_USB_HUB_STATUS_PORT_CONNECTED)
	    {
	      grub_usb_speed_t speed;
	      grub_usb_device_t next_dev;

	      /* Determine the device speed.  */
	      if (status & GRUB_USB_HUB_STATUS_PORT_LOWSPEED)
		speed = GRUB_USB_SPEED_LOW;
	      else
		{
		  if (status & GRUB_USB_HUB_STATUS_PORT_HIGHSPEED)
		    speed = GRUB_USB_SPEED_HIGH;
		  else
		    speed = GRUB_USB_SPEED_FULL;
		}

	      /* Wait a recovery time after reset, spec. says 10ms */
	      grub_millisleep (10);

	      /* Add the device and assign a device address to it.  */
	      next_dev = grub_usb_hub_add_dev (&dev->controller, speed, i, dev->addr);
	      dev->controller.dev->pending_reset = 0;
	      if (! next_dev)
		continue;

	      dev->children[i - 1] = next_dev;

	      /* If the device is a Hub, scan it for more devices.  */
	      if (next_dev->descdev.class == 0x09)
		grub_usb_add_hub (next_dev);
	    }
	}
    }
}

void
grub_usb_poll_devices (void)
{
  struct grub_usb_hub *hub;
  int i;

  for (hub = hubs; hub; hub = hub->next)
    {
      /* Do we have to recheck number of ports?  */
      /* No, it should be never changed, it should be constant. */
      for (i = 0; i < hub->nports; i++)
	{
	  grub_usb_speed_t speed = GRUB_USB_SPEED_NONE;
	  int changed = 0;

          if (!hub->controller->dev->pending_reset)
            {
              /* Check for possible timeout */
              if (grub_get_time_ms () > hub->controller->dev->pending_reset)
                {
                  /* Something went wrong, reset device was not
                   * addressed properly, timeout happened */
	          hub->controller->dev->pending_reset = 0;
	          speed = hub->controller->dev->detect_dev (hub->controller,
                                                            i, &changed);
                }
            }
	  if (changed)
	    {
	      detach_device (hub->devices[i]);
	      hub->devices[i] = NULL;
	      if (speed != GRUB_USB_SPEED_NONE)
                attach_root_port (hub, i, speed);
	    }
	}
    }

  while (1)
    {
      rescan = 0;
      
      /* We should check changes of non-root hubs too. */
      for (i = 0; i < GRUB_USBHUB_MAX_DEVICES; i++)
	{
	  grub_usb_device_t dev = grub_usb_devs[i];
	  
	  if (dev && dev->descdev.class == 0x09)
	    poll_nonroot_hub (dev);
	}
      if (!rescan)
	break;
      grub_millisleep (50);
    }

}

int
grub_usb_iterate (int (*hook) (grub_usb_device_t dev))
{
  int i;

  for (i = 0; i < GRUB_USBHUB_MAX_DEVICES; i++)
    {
      if (grub_usb_devs[i])
	{
	  if (hook (grub_usb_devs[i]))
	      return 1;
	}
    }

  return 0;
}
