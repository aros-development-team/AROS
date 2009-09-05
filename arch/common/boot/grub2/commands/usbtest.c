/* usbtest.c - test module for USB */
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

#include <grub/types.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/err.h>
#include <grub/dl.h>
#include <grub/usb.h>
#include <grub/command.h>

static const char *usb_classes[] =
  {
    "",
    "Audio",
    "Communication Interface",
    "HID",
    "",
    "Physical",
    "Image",
    "Printer",
    "Mass Storage",
    "Hub",
    "Data Interface",
    "Smart Card",
    "Content Security",
    "Video"
  };

static const char *usb_endp_type[] =
  {
    "Control",
    "Isochronous",
    "Bulk",
    "Interrupt"
  };

static const char *usb_devspeed[] =
  {
    "",
    "Low",
    "Full",
    "High"
  };

static void
usb_print_str (const char *description, grub_usb_device_t dev, int idx)
{
  char *name;
  /* XXX: LANGID  */

  if (! idx)
    return;

  grub_usb_get_string (dev, idx, 0x0409, &name);
  grub_printf ("%s: `%s'\n", description, name);
  grub_free (name);
}

static int
usb_iterate (grub_usb_device_t dev)
{
  struct grub_usb_desc_device *descdev;
  int i;

  descdev = &dev->descdev;

  usb_print_str ("Product", dev, descdev->strprod);
  usb_print_str ("Vendor", dev, descdev->strvendor);
  usb_print_str ("Serial", dev, descdev->strserial);

  if (descdev->class > 0 && descdev->class <= 0x0E)
    grub_printf ("Class: (0x%02x) %s, Subclass: 0x%02x, Protocol: 0x%02x\n",
		 descdev->class, usb_classes[descdev->class],
		 descdev->subclass, descdev->protocol);
  grub_printf ("USB version %d.%d, VendorID: 0x%02x, ProductID: 0x%02x, #conf: %d\n",
	       descdev->usbrel >> 8, (descdev->usbrel >> 4) & 0x0F,
	       descdev->vendorid, descdev->prodid, descdev->configcnt);

  grub_printf ("%s speed device\n", usb_devspeed[dev->speed]);

  for (i = 0; i < descdev->configcnt; i++)
    {
      struct grub_usb_desc_config *config;

      config = dev->config[i].descconf;
      usb_print_str ("Configuration:", dev, config->strconfig);
    }

  for (i = 0; i < dev->config[0].descconf->numif; i++)
    {
      int j;
      struct grub_usb_desc_if *interf;
      interf = dev->config[0].interf[i].descif;

      grub_printf ("Interface #%d: #Endpoints: %d   ",
		   i, interf->endpointcnt);
      if (interf->class > 0 && interf->class <= 0x0E)
	grub_printf ("Class: (0x%02x) %s, Subclass: 0x%02x, Protocol: 0x%02x\n",
		     interf->class, usb_classes[interf->class],
		     interf->subclass, interf->protocol);

      usb_print_str ("Interface", dev, interf->strif);

      for (j = 0; j < interf->endpointcnt; j++)
	{
	  struct grub_usb_desc_endp *endp;
	  endp = &dev->config[0].interf[i].descendp[j];

	  grub_printf ("Endpoint #%d: %s, max packed size: %d, transfer type: %s, latency: %d\n",
		       endp->endp_addr & 15,
		       (endp->endp_addr & 128) ? "IN" : "OUT",
		       endp->maxpacket, usb_endp_type[endp->attrib & 3],
		       endp->interval);
	}
    }

  grub_printf("\n");

  return 0;
}

static grub_err_t
grub_cmd_usbtest (grub_command_t cmd __attribute__ ((unused)),
		  int argc __attribute__ ((unused)),
		  char **args __attribute__ ((unused)))
{
  grub_printf ("USB devices:\n\n");
  grub_usb_iterate (usb_iterate);

  return 0;
}

static grub_command_t cmd;

GRUB_MOD_INIT(usbtest)
{
  cmd = grub_register_command ("usb", grub_cmd_usbtest,
			       0, "Test USB support");
}

GRUB_MOD_FINI(usbtest)
{
  grub_unregister_command (cmd);
}
