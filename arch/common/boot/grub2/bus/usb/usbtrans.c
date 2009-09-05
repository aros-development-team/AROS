/* usbtrans.c - USB Transfers and Transactions.  */
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
#include <grub/misc.h>
#include <grub/usb.h>
#include <grub/usbtrans.h>

grub_usb_err_t
grub_usb_control_msg (grub_usb_device_t dev,
		      grub_uint8_t reqtype,
		      grub_uint8_t request,
		      grub_uint16_t value,
		      grub_uint16_t index,
		      grub_size_t size, char *data)
{
  int i;
  grub_usb_transfer_t transfer;
  int datablocks;
  struct grub_usb_packet_setup setupdata;
  grub_usb_err_t err;
  unsigned int max;

  grub_dprintf ("usb",
		"control: reqtype=0x%02x req=0x%02x val=0x%02x idx=0x%02x size=%d\n",
		reqtype, request,  value, index, size);

  /* Create a transfer.  */
  transfer = grub_malloc (sizeof (struct grub_usb_transfer));
  if (! transfer)
    return grub_errno;

  /* Determine the maximum packet size.  */
  if (dev->initialized)
    max = dev->descdev.maxsize0;
  else
    max = 64;

  datablocks = (size + max - 1) / max;

  /* XXX: Discriminate between different types of control
     messages.  */
  transfer->transcnt = datablocks + 2;
  transfer->size = size; /* XXX ? */
  transfer->endpoint = 0;
  transfer->devaddr = dev->addr;
  transfer->type = GRUB_USB_TRANSACTION_TYPE_CONTROL;
  transfer->max = max;
  transfer->dev = dev;

  /* Allocate an array of transfer data structures.  */
  transfer->transactions = grub_malloc (transfer->transcnt
					* sizeof (struct grub_usb_transfer));
  if (! transfer->transactions)
    {
      grub_free (transfer);
      return grub_errno;
    }

  /* Build a Setup packet.  XXX: Endianess.  */
  setupdata.reqtype = reqtype;
  setupdata.request = request;
  setupdata.value = value;
  setupdata.index = index;
  setupdata.length = size;
  transfer->transactions[0].size = sizeof (setupdata);
  transfer->transactions[0].pid = GRUB_USB_TRANSFER_TYPE_SETUP;
  transfer->transactions[0].data = (char *) &setupdata;
  transfer->transactions[0].toggle = 0;

  /* Now the data...  XXX: Is this the right way to transfer control
     transfers?  */
  for (i = 0; i < datablocks; i++)
    {
      grub_usb_transaction_t tr = &transfer->transactions[i + 1];

      tr->size = (size > max) ? max : size;
      /* Use the right most bit as the data toggle.  Simple and
	 effective.  */
      tr->toggle = !(i & 1);
      if (reqtype & 128)
	tr->pid = GRUB_USB_TRANSFER_TYPE_IN;
      else
	tr->pid = GRUB_USB_TRANSFER_TYPE_OUT;
      tr->data = &data[i * max];
      size -= max;
    }

  /* End with an empty OUT transaction.  */
  transfer->transactions[datablocks + 1].size = 0;
  transfer->transactions[datablocks + 1].data = NULL;
  if (reqtype & 128)
    transfer->transactions[datablocks + 1].pid = GRUB_USB_TRANSFER_TYPE_OUT;
  else
    transfer->transactions[datablocks + 1].pid = GRUB_USB_TRANSFER_TYPE_IN;

  transfer->transactions[datablocks + 1].toggle = 1;

  err = dev->controller.dev->transfer (&dev->controller, transfer);

  grub_free (transfer->transactions);
  grub_free (transfer);

  return err;
}

static grub_usb_err_t
grub_usb_bulk_readwrite (grub_usb_device_t dev,
			 int endpoint, grub_size_t size, char *data,
			 grub_transfer_type_t type)
{
  int i;
  grub_usb_transfer_t transfer;
  int datablocks;
  unsigned int max;
  grub_usb_err_t err;
  int toggle = dev->toggle[endpoint];

  /* Use the maximum packet size given in the endpoint descriptor.  */
  if (dev->initialized)
    {
      struct grub_usb_desc_endp *endpdesc;
      endpdesc = grub_usb_get_endpdescriptor (dev, 0);

      if (endpdesc)
	max = endpdesc->maxpacket;
      else
	max = 64;
    }
  else
    max = 64;

  /* Create a transfer.  */
  transfer = grub_malloc (sizeof (struct grub_usb_transfer));
  if (! transfer)
    return grub_errno;

  datablocks = ((size + max - 1) / max);
  transfer->transcnt = datablocks;
  transfer->size = size - 1;
  transfer->endpoint = endpoint;
  transfer->devaddr = dev->addr;
  transfer->type = GRUB_USB_TRANSACTION_TYPE_BULK;
  transfer->max = max;
  transfer->dev = dev;

  /* Allocate an array of transfer data structures.  */
  transfer->transactions = grub_malloc (transfer->transcnt
					* sizeof (struct grub_usb_transfer));
  if (! transfer->transactions)
    {
      grub_free (transfer);
      return grub_errno;
    }

  /* Set up all transfers.  */
  for (i = 0; i < datablocks; i++)
    {
      grub_usb_transaction_t tr = &transfer->transactions[i];

      tr->size = (size > max) ? max : size;
      /* XXX: Use the right most bit as the data toggle.  Simple and
	 effective.  */
      tr->toggle = toggle;
      toggle = toggle ? 0 : 1;
      tr->pid = type;
      tr->data = &data[i * max];
      size -= tr->size;
    }

  err = dev->controller.dev->transfer (&dev->controller, transfer);
  grub_dprintf ("usb", "toggle=%d\n", toggle);
  dev->toggle[endpoint] = toggle;

  grub_free (transfer->transactions);
  grub_free (transfer);

  return err;
}

grub_usb_err_t
grub_usb_bulk_write (grub_usb_device_t dev,
		     int endpoint, grub_size_t size, char *data)
{
  return grub_usb_bulk_readwrite (dev, endpoint, size, data,
				  GRUB_USB_TRANSFER_TYPE_OUT);
}

grub_usb_err_t
grub_usb_bulk_read (grub_usb_device_t dev,
		    int endpoint, grub_size_t size, char *data)
{
  return grub_usb_bulk_readwrite (dev, endpoint, size, data,
				  GRUB_USB_TRANSFER_TYPE_IN);
}
