/* uhci.c - UHCI Support.  */
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
#include <grub/pci.h>
#include <grub/cpu/pci.h>
#include <grub/i386/io.h>
#include <grub/time.h>

#define GRUB_UHCI_IOMASK	(0x7FF << 5)

typedef enum
  {
    GRUB_UHCI_REG_USBCMD = 0x00,
    GRUB_UHCI_REG_FLBASEADD = 0x08,
    GRUB_UHCI_REG_PORTSC1 = 0x10,
    GRUB_UHCI_REG_PORTSC2 = 0x12
  } grub_uhci_reg_t;

#define GRUB_UHCI_LINK_TERMINATE	1
#define GRUB_UHCI_LINK_QUEUE_HEAD	2


/* UHCI Queue Head.  */
struct grub_uhci_qh
{
  /* Queue head link pointer which points to the next queue head.  */
  grub_uint32_t linkptr;

  /* Queue element link pointer which points to the first data object
     within the queue.  */
  grub_uint32_t elinkptr;

  /* Queue heads are aligned on 16 bytes, pad so a queue head is 16
     bytes so we can store many in a 4K page.  */
  grub_uint8_t pad[8];
} __attribute__ ((packed));

/* UHCI Transfer Descriptor.  */
struct grub_uhci_td
{
  /* Pointer to the next TD in the list.  */
  grub_uint32_t linkptr;

  /* Control and status bits.  */
  grub_uint32_t ctrl_status;

  /* All information required to transfer the Token packet.  */
  grub_uint32_t token;

  /* A pointer to the data buffer, UHCI requires this pointer to be 32
     bits.  */
  grub_uint32_t buffer;

  /* Another linkptr that is not overwritten by the Host Controller.
     This is GRUB specific.  */
  grub_uint32_t linkptr2;

  /* 3 additional 32 bits words reserved for the Host Controller Driver.  */
  grub_uint32_t data[3];
} __attribute__ ((packed));

typedef volatile struct grub_uhci_td *grub_uhci_td_t;
typedef volatile struct grub_uhci_qh *grub_uhci_qh_t;

struct grub_uhci
{
  int iobase;
  grub_uint32_t *framelist;

  /* 256 Queue Heads.  */
  grub_uhci_qh_t qh;

  /* 256 Transfer Descriptors.  */
  grub_uhci_td_t td;

  /* Free Transfer Descriptors.  */
  grub_uhci_td_t tdfree;

  struct grub_uhci *next;
};

static struct grub_uhci *uhci;

static grub_uint16_t
grub_uhci_readreg16 (struct grub_uhci *u, grub_uhci_reg_t reg)
{
  return grub_inw (u->iobase + reg);
}

#if 0
static grub_uint32_t
grub_uhci_readreg32 (struct grub_uhci *u, grub_uhci_reg_t reg)
{
  return grub_inl (u->iobase + reg);
}
#endif

static void
grub_uhci_writereg16 (struct grub_uhci *u,
		      grub_uhci_reg_t reg, grub_uint16_t val)
{
  grub_outw (val, u->iobase + reg);
}

static void
grub_uhci_writereg32 (struct grub_uhci *u,
		    grub_uhci_reg_t reg, grub_uint32_t val)
{
  grub_outl (val, u->iobase + reg);
}

static grub_err_t
grub_uhci_portstatus (grub_usb_controller_t dev,
		      unsigned int port, unsigned int enable);


/* Iterate over all PCI devices.  Determine if a device is an UHCI
   controller.  If this is the case, initialize it.  */
static int NESTED_FUNC_ATTR
grub_uhci_pci_iter (int bus, int device, int func,
		    grub_pci_id_t pciid __attribute__((unused)))
{
  grub_uint32_t class_code;
  grub_uint32_t class;
  grub_uint32_t subclass;
  grub_uint32_t interf;
  grub_uint32_t base;
  grub_uint32_t fp;
  grub_pci_address_t addr;
  struct grub_uhci *u;
  int i;

  addr = grub_pci_make_address (bus, device, func, 2);
  class_code = grub_pci_read (addr) >> 8;

  interf = class_code & 0xFF;
  subclass = (class_code >> 8) & 0xFF;
  class = class_code >> 16;

  /* If this is not an UHCI controller, just return.  */
  if (class != 0x0c || subclass != 0x03 || interf != 0x00)
    return 0;

  /* Determine IO base address.  */
  addr = grub_pci_make_address (bus, device, func, 8);
  base = grub_pci_read (addr);
  /* Stop if there is no IO space base address defined.  */
  if (! (base & 1))
    return 0;

  /* Allocate memory for the controller and register it.  */
  u = grub_zalloc (sizeof (*u));
  if (! u)
    return 1;

  u->iobase = base & GRUB_UHCI_IOMASK;
  grub_dprintf ("uhci", "class=0x%02x 0x%02x interface 0x%02x base=0x%x\n",
		class, subclass, interf, u->iobase);

  /* Reserve a page for the frame list.  */
  u->framelist = grub_memalign (4096, 4096);
  if (! u->framelist)
    goto fail;

  /* The framelist pointer of UHCI is only 32 bits, make sure this
     code works on on 64 bits architectures.  */
#if GRUB_CPU_SIZEOF_VOID_P == 8
  if ((grub_uint64_t) u->framelist >> 32)
    {
      grub_error (GRUB_ERR_OUT_OF_MEMORY,
		  "allocated frame list memory not <4GB");
      goto fail;
    }
#endif

  /* The QH pointer of UHCI is only 32 bits, make sure this
     code works on on 64 bits architectures.  */
  u->qh = (grub_uhci_qh_t) grub_memalign (4096, 4096);
  if (! u->qh)
    goto fail;

#if GRUB_CPU_SIZEOF_VOID_P == 8
  if ((grub_uint64_t) u->qh >> 32)
    {
      grub_error (GRUB_ERR_OUT_OF_MEMORY, "allocated QH memory not <4GB");
      goto fail;
    }
#endif

  /* The TD pointer of UHCI is only 32 bits, make sure this
     code works on on 64 bits architectures.  */
  u->td = (grub_uhci_td_t) grub_memalign (4096, 4096*2);
  if (! u->td)
    goto fail;

#if GRUB_CPU_SIZEOF_VOID_P == 8
  if ((grub_uint64_t) u->td >> 32)
    {
      grub_error (GRUB_ERR_OUT_OF_MEMORY, "allocated TD memory not <4GB");
      goto fail;
    }
#endif

  /* Link all Transfer Descriptors in a list of available Transfer
     Descriptors.  */
  for (i = 0; i < 256; i++)
    u->td[i].linkptr = (grub_uint32_t) &u->td[i + 1];
  u->td[255 - 1].linkptr = 0;
  u->tdfree = u->td;

  /* Make sure UHCI is disabled!  */
  grub_uhci_writereg16 (u, GRUB_UHCI_REG_USBCMD, 0);

  /* Setup the frame list pointers.  Since no isochronous transfers
     are and will be supported, they all point to the (same!) queue
     head.  */
  fp = (grub_uint32_t) u->qh & (~15);
  /* Mark this as a queue head.  */
  fp |= 2;
  for (i = 0; i < 1024; i++)
    u->framelist[i] = fp;
  /* Program the framelist address into the UHCI controller.  */
  grub_uhci_writereg32 (u, GRUB_UHCI_REG_FLBASEADD,
			(grub_uint32_t) u->framelist);

  /* Make the Queue Heads point to each other.  */
  for (i = 0; i < 256; i++)
    {
      /* Point to the next QH.  */
      u->qh[i].linkptr = (grub_uint32_t) (&u->qh[i + 1]) & (~15);

      /* This is a QH.  */
      u->qh[i].linkptr |= GRUB_UHCI_LINK_QUEUE_HEAD;

      /* For the moment, do not point to a Transfer Descriptor.  These
	 are set at transfer time, so just terminate it.  */
      u->qh[i].elinkptr = 1;
    }

  /* The last Queue Head should terminate.  256 are too many QHs so
     just use 50.  */
  u->qh[50 - 1].linkptr = 1;

  /* Enable UHCI again.  */
  grub_uhci_writereg16 (u, GRUB_UHCI_REG_USBCMD, 1 | (1 << 7));

  /* UHCI is initialized and ready for transfers.  */
  grub_dprintf ("uhci", "UHCI initialized\n");


#if 0
  {
    int i;
    for (i = 0; i < 10; i++)
      {
	grub_uint16_t frnum;

	frnum = grub_uhci_readreg16 (u, 6);
	grub_dprintf ("uhci", "Framenum=%d\n", frnum);
	grub_millisleep (100);
      }
  }
#endif

  /* Link to uhci now that initialisation is successful.  */
  u->next = uhci;
  uhci = u;

  return 0;

 fail:
  if (u)
    {
      grub_free ((void *) u->qh);
      grub_free (u->framelist);
    }
  grub_free (u);

  return 1;
}

static void
grub_uhci_inithw (void)
{
  grub_pci_iterate (grub_uhci_pci_iter);
}

static grub_uhci_td_t
grub_alloc_td (struct grub_uhci *u)
{
  grub_uhci_td_t ret;

  /* Check if there is a Transfer Descriptor available.  */
  if (! u->tdfree)
    return NULL;

  ret = u->tdfree;
  u->tdfree = (grub_uhci_td_t) u->tdfree->linkptr;

  return ret;
}

static void
grub_free_td (struct grub_uhci *u, grub_uhci_td_t td)
{
  td->linkptr = (grub_uint32_t) u->tdfree;
  u->tdfree = td;
}

static void
grub_free_queue (struct grub_uhci *u, grub_uhci_td_t td)
{
  /* Free the TDs in this queue.  */
  while (td)
    {
      grub_uhci_td_t tdprev;

      /* Unlink the queue.  */
      tdprev = td;
      td = (grub_uhci_td_t) td->linkptr2;

      /* Free the TD.  */
      grub_free_td (u, tdprev);
    }
}

static grub_uhci_qh_t
grub_alloc_qh (struct grub_uhci *u,
	       grub_transaction_type_t tr __attribute__((unused)))
{
  int i;
  grub_uhci_qh_t qh;

  /* Look for a Queue Head for this transfer.  Skip the first QH if
     this is a Interrupt Transfer.  */
#if 0
  if (tr == GRUB_USB_TRANSACTION_TYPE_INTERRUPT)
    i = 0;
  else
#endif
    i = 1;

  for (; i < 255; i++)
    {
      if (u->qh[i].elinkptr & 1)
	break;
    }
  qh = &u->qh[i];
  if (! (qh->elinkptr & 1))
    {
      grub_error (GRUB_ERR_OUT_OF_MEMORY,
		  "no free queue heads available");
      return NULL;
    }

  return qh;
}

static grub_uhci_td_t
grub_uhci_transaction (struct grub_uhci *u, unsigned int endp,
		       grub_transfer_type_t type, unsigned int addr,
		       unsigned int toggle, grub_size_t size,
		       char *data)
{
  grub_uhci_td_t td;
  static const unsigned int tf[] = { 0x69, 0xE1, 0x2D };

  /* XXX: Check if data is <4GB.  If it isn't, just copy stuff around.
     This is only relevant for 64 bits architectures.  */

  /* Grab a free Transfer Descriptor and initialize it.  */
  td = grub_alloc_td (u);
  if (! td)
    {
      grub_error (GRUB_ERR_OUT_OF_MEMORY,
		  "no transfer descriptors available for UHCI transfer");
      return 0;
    }

  grub_dprintf ("uhci",
		"transaction: endp=%d, type=%d, addr=%d, toggle=%d, size=%d data=%p td=%p\n",
		endp, type, addr, toggle, size, data, td);

  /* Don't point to any TD, just terminate.  */
  td->linkptr = 1;

  /* Active!  Only retry a transfer 3 times.  */
  td->ctrl_status = (1 << 23) | (3 << 27);

  /* If zero bytes are transmitted, size is 0x7FF.  Otherwise size is
     size-1.  */
  if (size == 0)
    size = 0x7FF;
  else
    size = size - 1;

  /* Setup whatever is required for the token packet.  */
  td->token = ((size << 21) | (toggle << 19) | (endp << 15)
	       | (addr << 8) | tf[type]);

  td->buffer = (grub_uint32_t) data;

  return td;
}

static grub_usb_err_t
grub_uhci_transfer (grub_usb_controller_t dev,
		    grub_usb_transfer_t transfer)
{
  struct grub_uhci *u = (struct grub_uhci *) dev->data;
  grub_uhci_qh_t qh;
  grub_uhci_td_t td;
  grub_uhci_td_t td_first = NULL;
  grub_uhci_td_t td_prev = NULL;
  grub_usb_err_t err = GRUB_USB_ERR_NONE;
  int i;

  /* Allocate a queue head for the transfer queue.  */
  qh = grub_alloc_qh (u, GRUB_USB_TRANSACTION_TYPE_CONTROL);
  if (! qh)
    return grub_errno;

  for (i = 0; i < transfer->transcnt; i++)
    {
      grub_usb_transaction_t tr = &transfer->transactions[i];

      td = grub_uhci_transaction (u, transfer->endpoint, tr->pid,
				  transfer->devaddr, tr->toggle,
				  tr->size, tr->data);
      if (! td)
	{
	  /* Terminate and free.  */
	  td_prev->linkptr2 = 0;
	  td_prev->linkptr = 1;

	  if (td_first)
	    grub_free_queue (u, td_first);

	  return GRUB_USB_ERR_INTERNAL;
	}

      if (! td_first)
	td_first = td;
      else
	{
	  td_prev->linkptr2 = (grub_uint32_t) td;
	  td_prev->linkptr = (grub_uint32_t) td;
	  td_prev->linkptr |= 4;
	}
      td_prev = td;
    }
  td_prev->linkptr2 = 0;
  td_prev->linkptr = 1;

  grub_dprintf ("uhci", "setup transaction %d\n", transfer->type);

  /* Link it into the queue and terminate.  Now the transaction can
     take place.  */
  qh->elinkptr = (grub_uint32_t) td_first;

  grub_dprintf ("uhci", "initiate transaction\n");

  /* Wait until either the transaction completed or an error
     occurred.  */
  for (;;)
    {
      grub_uhci_td_t errtd;

      errtd = (grub_uhci_td_t) (qh->elinkptr & ~0x0f);

      grub_dprintf ("uhci", ">t status=0x%02x data=0x%02x td=%p\n",
		    errtd->ctrl_status, errtd->buffer & (~15), errtd);

      /* Check if the transaction completed.  */
      if (qh->elinkptr & 1)
	break;

      grub_dprintf ("uhci", "t status=0x%02x\n", errtd->ctrl_status);

      /* Check if the TD is not longer active.  */
      if (! (errtd->ctrl_status & (1 << 23)))
	{
	  grub_dprintf ("uhci", ">>t status=0x%02x\n", errtd->ctrl_status);

	  /* Check if the endpoint is stalled.  */
	  if (errtd->ctrl_status & (1 << 22))
	    err = GRUB_USB_ERR_STALL;

	  /* Check if an error related to the data buffer occurred.  */
	  if (errtd->ctrl_status & (1 << 21))
	    err = GRUB_USB_ERR_DATA;

	  /* Check if a babble error occurred.  */
	  if (errtd->ctrl_status & (1 << 20))
	    err = GRUB_USB_ERR_BABBLE;

	  /* Check if a NAK occurred.  */
	  if (errtd->ctrl_status & (1 << 19))
	    err = GRUB_USB_ERR_NAK;

	  /* Check if a timeout occurred.  */
	  if (errtd->ctrl_status & (1 << 18))
	    err = GRUB_USB_ERR_TIMEOUT;

	  /* Check if a bitstuff error occurred.  */
	  if (errtd->ctrl_status & (1 << 17))
	    err = GRUB_USB_ERR_BITSTUFF;

	  if (err)
	    goto fail;

	  /* Fall through, no errors occurred, so the QH might be
	     updated.  */
	  grub_dprintf ("uhci", "transaction fallthrough\n");
	}
    }

  grub_dprintf ("uhci", "transaction complete\n");

 fail:

  grub_dprintf ("uhci", "transaction failed\n");

  /* Place the QH back in the free list and deallocate the associated
     TDs.  */
  qh->elinkptr = 1;
  grub_free_queue (u, td_first);

  return err;
}

static int
grub_uhci_iterate (int (*hook) (grub_usb_controller_t dev))
{
  struct grub_uhci *u;
  struct grub_usb_controller dev;

  for (u = uhci; u; u = u->next)
    {
      dev.data = u;
      if (hook (&dev))
	return 1;
    }

  return 0;
}

static grub_err_t
grub_uhci_portstatus (grub_usb_controller_t dev,
		      unsigned int port, unsigned int enable)
{
  struct grub_uhci *u = (struct grub_uhci *) dev->data;
  int reg;
  unsigned int status;

  grub_dprintf ("uhci", "enable=%d port=%d\n", enable, port);

  if (port == 0)
    reg = GRUB_UHCI_REG_PORTSC1;
  else if (port == 1)
    reg = GRUB_UHCI_REG_PORTSC2;
  else
    return grub_error (GRUB_ERR_OUT_OF_RANGE,
		       "UHCI Root Hub port does not exist");

  status = grub_uhci_readreg16 (u, reg);
  grub_dprintf ("uhci", "detect=0x%02x\n", status);

  /* Reset the port.  */
  grub_uhci_writereg16 (u, reg, enable << 9);

  /* Wait for the reset to complete.  XXX: How long exactly?  */
  grub_millisleep (10);
  status = grub_uhci_readreg16 (u, reg);
  grub_uhci_writereg16 (u, reg, status & ~(1 << 9));
  grub_dprintf ("uhci", "reset completed\n");

  /* Enable the port.  */
  grub_uhci_writereg16 (u, reg, enable << 2);
  grub_millisleep (10);

  grub_dprintf ("uhci", "waiting for the port to be enabled\n");

  while (! (grub_uhci_readreg16 (u, reg) & (1 << 2)));

  status = grub_uhci_readreg16 (u, reg);
  grub_dprintf ("uhci", ">3detect=0x%02x\n", status);


  return GRUB_ERR_NONE;
}

static grub_usb_speed_t
grub_uhci_detect_dev (grub_usb_controller_t dev, int port)
{
  struct grub_uhci *u = (struct grub_uhci *) dev->data;
  int reg;
  unsigned int status;

  if (port == 0)
    reg = GRUB_UHCI_REG_PORTSC1;
  else if (port == 1)
    reg = GRUB_UHCI_REG_PORTSC2;
  else
    return grub_error (GRUB_ERR_OUT_OF_RANGE,
		       "UHCI Root Hub port does not exist");

  status = grub_uhci_readreg16 (u, reg);

  grub_dprintf ("uhci", "detect=0x%02x port=%d\n", status, port);

  if (! (status & 1))
    return GRUB_USB_SPEED_NONE;
  else if (status & (1 << 8))
    return GRUB_USB_SPEED_LOW;
  else
    return GRUB_USB_SPEED_FULL;
}

static int
grub_uhci_hubports (grub_usb_controller_t dev __attribute__((unused)))
{
  /* The root hub has exactly two ports.  */
  return 2;
}


static struct grub_usb_controller_dev usb_controller =
{
  .name = "uhci",
  .iterate = grub_uhci_iterate,
  .transfer = grub_uhci_transfer,
  .hubports = grub_uhci_hubports,
  .portstatus = grub_uhci_portstatus,
  .detect_dev = grub_uhci_detect_dev
};

GRUB_MOD_INIT(uhci)
{
  grub_uhci_inithw ();
  grub_usb_controller_dev_register (&usb_controller);
  grub_dprintf ("uhci", "registered\n");
}

GRUB_MOD_FINI(uhci)
{
  struct grub_uhci *u;

  /* Disable all UHCI controllers.  */
  for (u = uhci; u; u = u->next)
    grub_uhci_writereg16 (u, GRUB_UHCI_REG_USBCMD, 0);

  /* Unregister the controller.  */
  grub_usb_controller_dev_unregister (&usb_controller);
}
