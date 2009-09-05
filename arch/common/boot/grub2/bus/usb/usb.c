/* usb.c - Generic USB interfaces.  */
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

static grub_usb_controller_dev_t grub_usb_list;

void
grub_usb_controller_dev_register (grub_usb_controller_dev_t usb)
{
  auto int iterate_hook (grub_usb_controller_t dev);

  /* Iterate over all controllers found by the driver.  */
  int iterate_hook (grub_usb_controller_t dev)
    {
      dev->dev = usb;

      /* Enable the ports of the USB Root Hub.  */
      grub_usb_root_hub (dev);

      return 0;
    }

  usb->next = grub_usb_list;
  grub_usb_list = usb;

  if (usb->iterate)
    usb->iterate (iterate_hook);
}

void
grub_usb_controller_dev_unregister (grub_usb_controller_dev_t usb)
{
  grub_usb_controller_dev_t *p, q;

  for (p = &grub_usb_list, q = *p; q; p = &(q->next), q = q->next)
    if (q == usb)
      {
	*p = q->next;
	break;
      }
}

#if 0
int
grub_usb_controller_iterate (int (*hook) (grub_usb_controller_t dev))
{
  grub_usb_controller_dev_t p;

  auto int iterate_hook (grub_usb_controller_t dev);

  int iterate_hook (grub_usb_controller_t dev)
    {
      dev->dev = p;
      if (hook (dev))
	return 1;
      return 0;
    }

  /* Iterate over all controller drivers.  */
  for (p = grub_usb_list; p; p = p->next)
    {
      /* Iterate over the busses of the controllers.  XXX: Actually, a
	 hub driver should do this.  */
      if (p->iterate (iterate_hook))
	return 1;
    }

  return 0;
}
#endif


grub_usb_err_t
grub_usb_clear_halt (grub_usb_device_t dev, int endpoint)
{
  dev->toggle[endpoint] = 0;
  return grub_usb_control_msg (dev, (GRUB_USB_REQTYPE_OUT
				     | GRUB_USB_REQTYPE_STANDARD
				     | GRUB_USB_REQTYPE_TARGET_ENDP),
			       GRUB_USB_REQ_CLEAR_FEATURE,
			       GRUB_USB_FEATURE_ENDP_HALT,
			       endpoint, 0, 0);
}

grub_usb_err_t
grub_usb_set_configuration (grub_usb_device_t dev, int configuration)
{
  int i;

  for (i = 0; i < 16; i++)
    dev->toggle[i] = 0;

  return grub_usb_control_msg (dev, (GRUB_USB_REQTYPE_OUT
				     | GRUB_USB_REQTYPE_STANDARD
				     | GRUB_USB_REQTYPE_TARGET_DEV),
			       GRUB_USB_REQ_SET_CONFIGURATION, configuration,
			       0, 0, NULL);
}

grub_usb_err_t
grub_usb_get_descriptor (grub_usb_device_t dev,
			 grub_uint8_t type, grub_uint8_t index,
			 grub_size_t size, char *data)
{
  return grub_usb_control_msg (dev, (GRUB_USB_REQTYPE_IN
				     | GRUB_USB_REQTYPE_STANDARD
				     | GRUB_USB_REQTYPE_TARGET_DEV),
			       GRUB_USB_REQ_GET_DESCRIPTOR,
			       (type << 8) | index,
			       0, size, data);
}

struct grub_usb_desc_endp *
grub_usb_get_endpdescriptor (grub_usb_device_t usbdev, int addr)
{
  int i;

  for (i = 0; i < usbdev->config[0].descconf->numif; i++)
    {
      struct grub_usb_desc_if *interf;
      int j;

      interf = usbdev->config[0].interf[i].descif;

      for (j = 0; j < interf->endpointcnt; j++)
	{
	  struct grub_usb_desc_endp *endp;
	  endp = &usbdev->config[0].interf[i].descendp[j];

	  if (endp->endp_addr == addr)
	    return endp;
	}
    }

  return NULL;
}

grub_usb_err_t
grub_usb_get_string (grub_usb_device_t dev, grub_uint8_t index, int langid,
		     char **string)
{
  struct grub_usb_desc_str descstr;
  struct grub_usb_desc_str *descstrp;
  grub_usb_err_t err;

  /* Only get the length.  */
  err = grub_usb_control_msg (dev, 1 << 7,
			      0x06, (3 << 8) | index,
			      langid, 1, (char *) &descstr);
  if (err)
    return err;

  descstrp = grub_malloc (descstr.length);
  if (! descstrp)
    return GRUB_USB_ERR_INTERNAL;
  err = grub_usb_control_msg (dev, 1 << 7,
			      0x06, (3 << 8) | index,
			      langid, descstr.length, (char *) descstrp);

  *string = grub_malloc (descstr.length / 2);
  if (! *string)
    {
      grub_free (descstrp);
      return GRUB_USB_ERR_INTERNAL;
    }

  grub_utf16_to_utf8 ((grub_uint8_t *) *string, descstrp->str, descstrp->length / 2 - 1);
  (*string)[descstr.length / 2 - 1] = '\0';
  grub_free (descstrp);

  return GRUB_USB_ERR_NONE;
}

grub_usb_err_t
grub_usb_device_initialize (grub_usb_device_t dev)
{
  struct grub_usb_desc_device *descdev;
  struct grub_usb_desc_config config;
  grub_usb_err_t err;
  int i;

  err = grub_usb_get_descriptor (dev, GRUB_USB_DESCRIPTOR_DEVICE,
				 0, sizeof (struct grub_usb_desc_device),
				 (char *) &dev->descdev);
  if (err)
    return err;
  descdev = &dev->descdev;

  for (i = 0; i < 8; i++)
    dev->config[i].descconf = NULL;

  for (i = 0; i < descdev->configcnt; i++)
    {
      int pos;
      int currif;
      char *data;

      /* First just read the first 4 bytes of the configuration
	 descriptor, after that it is known how many bytes really have
	 to be read.  */
      err = grub_usb_get_descriptor (dev, GRUB_USB_DESCRIPTOR_CONFIG, i, 4,
				     (char *) &config);

      data = grub_malloc (config.totallen);
      if (! data)
	{
	  err = GRUB_USB_ERR_INTERNAL;
	  goto fail;
	}

      dev->config[i].descconf = (struct grub_usb_desc_config *) data;
      err = grub_usb_get_descriptor (dev, GRUB_USB_DESCRIPTOR_CONFIG, i,
				     config.totallen, data);
      if (err)
	goto fail;

      /* Skip the configuration descriptor.  */
      pos = sizeof (struct grub_usb_desc_config);

      /* Read all interfaces.  */
      for (currif = 0; currif < dev->config[i].descconf->numif; currif++)
	{
	  dev->config[i].interf[currif].descif
	    = (struct grub_usb_desc_if *) &data[pos];
	  pos += sizeof (struct grub_usb_desc_if);

	  /* Point to the first endpoint.  */
	  dev->config[i].interf[currif].descendp
	    = (struct grub_usb_desc_endp *) &data[pos];
	  pos += (sizeof (struct grub_usb_desc_endp)
		  * dev->config[i].interf[currif].descif->endpointcnt);
	}
    }

  return GRUB_USB_ERR_NONE;

 fail:

  for (i = 0; i < 8; i++)
    grub_free (dev->config[i].descconf);

  return err;
}
