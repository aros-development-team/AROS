/* ohci.c - OHCI Support.  */
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
#include <grub/usbtrans.h>
#include <grub/misc.h>
#include <grub/pci.h>
#include <grub/cpu/pci.h>
#include <grub/i386/io.h>
#include <grub/time.h>

struct grub_ohci_hcca
{
  /* Pointers to Interrupt Endpoint Descriptors.  Not used by
     GRUB.  */
  grub_uint32_t inttable[32];

  /* Current frame number.  */
  grub_uint16_t framenumber;

  grub_uint16_t pad;

  /* List of completed TDs.  */
  grub_uint32_t donehead;

  grub_uint8_t reserved[116];
} __attribute__((packed));

/* OHCI Endpoint Descriptor.  */
struct grub_ohci_ed
{
  grub_uint32_t target;
  grub_uint32_t td_tail;
  grub_uint32_t td_head;
  grub_uint32_t next_ed;
} __attribute__((packed));

struct grub_ohci_td
{
  /* Information used to construct the TOKEN packet.  */
  grub_uint32_t token;

  grub_uint32_t buffer;
  grub_uint32_t next_td;
  grub_uint32_t buffer_end;
} __attribute__((packed));

typedef struct grub_ohci_td *grub_ohci_td_t;
typedef struct grub_ohci_ed *grub_ohci_ed_t;

struct grub_ohci
{
  volatile grub_uint32_t *iobase;
  volatile struct grub_ohci_hcca *hcca;
  struct grub_ohci *next;
};

static struct grub_ohci *ohci;

typedef enum
{
  GRUB_OHCI_REG_REVISION = 0x00,
  GRUB_OHCI_REG_CONTROL,
  GRUB_OHCI_REG_CMDSTATUS,
  GRUB_OHCI_REG_INTSTATUS,
  GRUB_OHCI_REG_INTENA,
  GRUB_OHCI_REG_INTDIS,
  GRUB_OHCI_REG_HCCA,
  GRUB_OHCI_REG_PERIODIC,
  GRUB_OHCI_REG_CONTROLHEAD,
  GRUB_OHCI_REG_CONTROLCURR,
  GRUB_OHCI_REG_BULKHEAD,
  GRUB_OHCI_REG_BULKCURR,
  GRUB_OHCI_REG_DONEHEAD,
  GRUB_OHCI_REG_FRAME_INTERVAL,
  GRUB_OHCI_REG_RHUBA = 18,
  GRUB_OHCI_REG_RHUBPORT = 21
} grub_ohci_reg_t;

static grub_uint32_t
grub_ohci_readreg32 (struct grub_ohci *o, grub_ohci_reg_t reg)
{
  return grub_le_to_cpu32 (*(o->iobase + reg));
}

static void
grub_ohci_writereg32 (struct grub_ohci *o,
		      grub_ohci_reg_t reg, grub_uint32_t val)
{
  *(o->iobase + reg) = grub_cpu_to_le32 (val);
}



/* Iterate over all PCI devices.  Determine if a device is an OHCI
   controller.  If this is the case, initialize it.  */
static int NESTED_FUNC_ATTR
grub_ohci_pci_iter (int bus, int device, int func,
		    grub_pci_id_t pciid __attribute__((unused)))
{
  grub_uint32_t class_code;
  grub_uint32_t class;
  grub_uint32_t subclass;
  grub_uint32_t interf;
  grub_uint32_t base;
  grub_pci_address_t addr;
  struct grub_ohci *o;
  grub_uint32_t revision;
  grub_uint32_t frame_interval;

  addr = grub_pci_make_address (bus, device, func, 2);
  class_code = grub_pci_read (addr) >> 8;

  interf = class_code & 0xFF;
  subclass = (class_code >> 8) & 0xFF;
  class = class_code >> 16;

  /* If this is not an OHCI controller, just return.  */
  if (class != 0x0c || subclass != 0x03 || interf != 0x10)
    return 0;

  /* Determine IO base address.  */
  addr = grub_pci_make_address (bus, device, func, 4);
  base = grub_pci_read (addr);

#if 0
  /* Stop if there is no IO space base address defined.  */
  if (! (base & 1))
    return 0;
#endif

  /* Allocate memory for the controller and register it.  */
  o = grub_malloc (sizeof (*o));
  if (! o)
    return 1;

  o->iobase = (grub_uint32_t *) base;

  /* Reserve memory for the HCCA.  */
  o->hcca = (struct grub_ohci_hcca *) grub_memalign (256, 256);

  grub_dprintf ("ohci", "class=0x%02x 0x%02x interface 0x%02x base=%p\n",
 		class, subclass, interf, o->iobase);

  /* Check if the OHCI revision is actually 1.0 as supported.  */
  revision = grub_ohci_readreg32 (o, GRUB_OHCI_REG_REVISION);
  grub_dprintf ("ohci", "OHCI revision=0x%02x\n", revision & 0xFF);
  if ((revision & 0xFF) != 0x10)
    goto fail;

  /* Backup the frame interval register.  */
  frame_interval = grub_ohci_readreg32 (o, GRUB_OHCI_REG_FRAME_INTERVAL);

  /* Suspend the OHCI by issuing a reset.  */
  grub_ohci_writereg32 (o, GRUB_OHCI_REG_CMDSTATUS, 1); /* XXX: Magic.  */
  grub_millisleep (1);
  grub_dprintf ("ohci", "OHCI reset\n");

  /* Restore the frame interval register.  */
  grub_ohci_writereg32 (o, GRUB_OHCI_REG_FRAME_INTERVAL, frame_interval);

  /* Setup the HCCA.  */
  grub_ohci_writereg32 (o, GRUB_OHCI_REG_HCCA, (grub_uint32_t) o->hcca);
  grub_dprintf ("ohci", "OHCI HCCA\n");

  /* Enable the OHCI.  */
  grub_ohci_writereg32 (o, GRUB_OHCI_REG_CONTROL,
			(2 << 6));
  grub_dprintf ("ohci", "OHCI enable: 0x%02x\n",
		(grub_ohci_readreg32 (o, GRUB_OHCI_REG_CONTROL) >> 6) & 3);

  /* Link to ohci now that initialisation is successful.  */
  o->next = ohci;
  ohci = o;

  return 0;

 fail:
  if (o)
    grub_free ((void *) o->hcca);
  grub_free (o);

  return 1;
}


static void
grub_ohci_inithw (void)
{
  grub_pci_iterate (grub_ohci_pci_iter);
}



static int
grub_ohci_iterate (int (*hook) (grub_usb_controller_t dev))
{
  struct grub_ohci *o;
  struct grub_usb_controller dev;

  for (o = ohci; o; o = o->next)
    {
      dev.data = o;
      if (hook (&dev))
	return 1;
    }

  return 0;
}

static void
grub_ohci_transaction (grub_ohci_td_t td,
		       grub_transfer_type_t type, unsigned int toggle,
		       grub_size_t size, char *data)
{
  grub_uint32_t token;
  grub_uint32_t buffer;
  grub_uint32_t buffer_end;

  grub_dprintf ("ohci", "OHCI transaction td=%p type=%d, toggle=%d, size=%d\n",
		td, type, toggle, size);

  switch (type)
    {
    case GRUB_USB_TRANSFER_TYPE_SETUP:
      token = 0 << 19;
      break;
    case GRUB_USB_TRANSFER_TYPE_IN:
      token = 2 << 19;
      break;
    case GRUB_USB_TRANSFER_TYPE_OUT:
      token = 1 << 19;
      break;
    default:
      token = 0;
      break;
    }

  /* Generate no interrupts.  */
  token |= 7 << 21;

  /* Set the token.  */
  token |= toggle << 24;
  token |= 1 << 25;

  buffer = (grub_uint32_t) data;
  buffer_end = buffer + size - 1;

  td->token = grub_cpu_to_le32 (token);
  td->buffer = grub_cpu_to_le32 (buffer);
  td->next_td = 0;
  td->buffer_end = grub_cpu_to_le32 (buffer_end);
}

static grub_usb_err_t
grub_ohci_transfer (grub_usb_controller_t dev,
		    grub_usb_transfer_t transfer)
{
  struct grub_ohci *o = (struct grub_ohci *) dev->data;
  grub_ohci_ed_t ed;
  grub_ohci_td_t td_list;
  grub_uint32_t target;
  grub_uint32_t td_tail;
  grub_uint32_t td_head;
  grub_uint32_t status;
  grub_uint32_t control;
  grub_usb_err_t err;
  int i;

  /* Allocate an Endpoint Descriptor.  */
  ed = grub_memalign (16, sizeof (*ed));
  if (! ed)
    return GRUB_USB_ERR_INTERNAL;

  td_list = grub_memalign (16, sizeof (*td_list) * (transfer->transcnt + 1));
  if (! td_list)
    {
      grub_free ((void *) ed);
      return GRUB_USB_ERR_INTERNAL;
    }

  grub_dprintf ("ohci", "alloc=%p\n", td_list);

  /* Setup all Transfer Descriptors.  */
  for (i = 0; i < transfer->transcnt; i++)
    {
      grub_usb_transaction_t tr = &transfer->transactions[i];

      grub_ohci_transaction (&td_list[i], tr->pid, tr->toggle,
			     tr->size, tr->data);

      td_list[i].next_td = grub_cpu_to_le32 (&td_list[i + 1]);
    }

  /* Setup the Endpoint Descriptor.  */

  /* Set the device address.  */
  target = transfer->devaddr;

  /* Set the endpoint.  */
  target |= transfer->endpoint << 7;

  /* Set the device speed.  */
  target |= (transfer->dev->speed == GRUB_USB_SPEED_LOW) << 13;

  /* Set the maximum packet size.  */
  target |= transfer->max << 16;

  td_head = (grub_uint32_t) td_list;

  td_tail = (grub_uint32_t) &td_list[transfer->transcnt];

  ed->target = grub_cpu_to_le32 (target);
  ed->td_head = grub_cpu_to_le32 (td_head);
  ed->td_tail = grub_cpu_to_le32 (td_tail);
  ed->next_ed = grub_cpu_to_le32 (0);

  grub_dprintf ("ohci", "program OHCI\n");

  /* Program the OHCI to actually transfer.  */
  switch (transfer->type)
    {
    case GRUB_USB_TRANSACTION_TYPE_BULK:
      {
	grub_dprintf ("ohci", "add to bulk list\n");

	status = grub_ohci_readreg32 (o, GRUB_OHCI_REG_CMDSTATUS);
	control = grub_ohci_readreg32 (o, GRUB_OHCI_REG_CONTROL);

	/* Disable the Control and Bulk lists.  */
	control &= ~(3 << 4);
	grub_ohci_writereg32 (o, GRUB_OHCI_REG_CONTROL, control);

	/* Clear BulkListFilled.  */
	status &= ~(1 << 2);
	grub_ohci_writereg32 (o, GRUB_OHCI_REG_CMDSTATUS, status);

	grub_ohci_writereg32 (o, GRUB_OHCI_REG_BULKHEAD, (grub_uint32_t) ed);

	/* Enable the Bulk list.  */
	control |= 1 << 5;
	grub_ohci_writereg32 (o, GRUB_OHCI_REG_CONTROL, control);

	/* Set BulkListFilled.  */
	status |= 1 << 2;
	grub_ohci_writereg32 (o, GRUB_OHCI_REG_CMDSTATUS, status);

	break;
      }

    case GRUB_USB_TRANSACTION_TYPE_CONTROL:
      {
	grub_dprintf ("ohci", "add to control list\n");
	status = grub_ohci_readreg32 (o, GRUB_OHCI_REG_CMDSTATUS);
	control = grub_ohci_readreg32 (o, GRUB_OHCI_REG_CONTROL);

	/* Disable the Control and Bulk lists.  */
	control &= ~(3 << 4);
	grub_ohci_writereg32 (o, GRUB_OHCI_REG_CONTROL, control);

	/* Clear ControlListFilled.  */
	status &= ~(1 << 1);
	grub_ohci_writereg32 (o, GRUB_OHCI_REG_CMDSTATUS, status);

	grub_ohci_writereg32 (o, GRUB_OHCI_REG_CONTROLHEAD,
			      (grub_uint32_t) ed);
	grub_ohci_writereg32 (o, GRUB_OHCI_REG_CONTROLHEAD+1,
			      (grub_uint32_t) ed);

	/* Enable the Control list.  */
	control |= 1 << 4;
	grub_ohci_writereg32 (o, GRUB_OHCI_REG_CONTROL, control);

	/* Set ControlListFilled.  */
	status |= 1 << 1;
	grub_ohci_writereg32 (o, GRUB_OHCI_REG_CMDSTATUS, status);
	break;
      }
    }

  grub_dprintf ("ohci", "wait for completion\n");
  grub_dprintf ("ohci", "control=0x%02x status=0x%02x\n",
		grub_ohci_readreg32 (o, GRUB_OHCI_REG_CONTROL),
		grub_ohci_readreg32 (o, GRUB_OHCI_REG_CMDSTATUS));

  /* Wait until the transfer is completed or STALLs.  */
  while ((ed->td_head & ~0xf) != (ed->td_tail & ~0xf))
    {
      grub_cpu_idle ();

      grub_dprintf ("ohci", "head=0x%02x tail=0x%02x\n", ed->td_head, ed->td_tail);

      /* Detected a STALL.  */
      if (ed->td_head & 1)
	break;
    }

  grub_dprintf ("ohci", "complete\n");

/*   if (ed->td_head & 1) */
/*     err = GRUB_USB_ERR_STALL; */
/*   else if (ed->td */


  if (ed->td_head & 1)
    {
      grub_uint8_t errcode;
      grub_ohci_td_t tderr;

      tderr = (grub_ohci_td_t) grub_ohci_readreg32 (o,
						    GRUB_OHCI_REG_DONEHEAD);
      errcode = tderr->token >> 28;

      switch (errcode)
	{
	case 0:
	  /* XXX: Should not happen!  */
	  grub_error (GRUB_ERR_IO, "OHCI without reporting the reason");
	  err = GRUB_USB_ERR_INTERNAL;
	  break;

	case 1:
	  /* XXX: CRC error.  */
	  err = GRUB_USB_ERR_TIMEOUT;
	  break;

	case 2:
	  err = GRUB_USB_ERR_BITSTUFF;
	  break;

	case 3:
	  /* XXX: Data Toggle error.  */
	  err = GRUB_USB_ERR_DATA;
	  break;

	case 4:
	  err = GRUB_USB_ERR_STALL;
	  break;

	case 5:
	  /* XXX: Not responding.  */
	  err = GRUB_USB_ERR_TIMEOUT;
	  break;

	case 6:
	  /* XXX: PID Check bits failed.  */
	  err = GRUB_USB_ERR_BABBLE;
	  break;

	case 7:
	  /* XXX: PID unexpected failed.  */
	  err = GRUB_USB_ERR_BABBLE;
	  break;

	case 8:
	  /* XXX: Data overrun error.  */
	  err = GRUB_USB_ERR_DATA;
	  break;

	case 9:
	  /* XXX: Data underrun error.  */
	  err = GRUB_USB_ERR_DATA;
	  break;

	case 10:
	  /* XXX: Reserved.  */
	  err = GRUB_USB_ERR_NAK;
	  break;

	case 11:
	  /* XXX: Reserved.  */
	  err = GRUB_USB_ERR_NAK;
	  break;

	case 12:
	  /* XXX: Buffer overrun.  */
	  err = GRUB_USB_ERR_DATA;
	  break;

	case 13:
	  /* XXX: Buffer underrun.  */
	  err = GRUB_USB_ERR_DATA;
	  break;

	default:
	  err = GRUB_USB_ERR_NAK;
	  break;
	}
    }
  else
    err = GRUB_USB_ERR_NONE;

  /* Disable the Control and Bulk lists.  */
  control = grub_ohci_readreg32 (o, GRUB_OHCI_REG_CONTROL);
  control &= ~(3 << 4);
  grub_ohci_writereg32 (o, GRUB_OHCI_REG_CONTROL, control);

  /* Clear BulkListFilled and ControlListFilled.  */
  status = grub_ohci_readreg32 (o, GRUB_OHCI_REG_CMDSTATUS);
  status &= ~((1 << 2) | (1 << 3));
  grub_ohci_writereg32 (o, GRUB_OHCI_REG_CMDSTATUS, status);

  /* XXX */
  grub_free (td_list);
  grub_free (ed);

  return err;
}

static grub_err_t
grub_ohci_portstatus (grub_usb_controller_t dev,
		      unsigned int port, unsigned int enable)
{
   struct grub_ohci *o = (struct grub_ohci *) dev->data;
   grub_uint32_t status;

   /* Reset the port.  */
   status = grub_ohci_readreg32 (o, GRUB_OHCI_REG_RHUBPORT + port);
   status |= (1 << 4); /* XXX: Magic.  */
   grub_ohci_writereg32 (o, GRUB_OHCI_REG_RHUBPORT + port, status);
   grub_millisleep (100);

   /* End the reset signaling.  */
   status = grub_ohci_readreg32 (o, GRUB_OHCI_REG_RHUBPORT + port);
   status |= (1 << 20); /* XXX: Magic.  */
   grub_ohci_writereg32 (o, GRUB_OHCI_REG_RHUBPORT + port, status);
   grub_millisleep (10);

   /* Enable the port.  */
   status = grub_ohci_readreg32 (o, GRUB_OHCI_REG_RHUBPORT + port);
   status |= (enable << 1); /* XXX: Magic.  */
   grub_ohci_writereg32 (o, GRUB_OHCI_REG_RHUBPORT + port, status);

   status = grub_ohci_readreg32 (o, GRUB_OHCI_REG_RHUBPORT + port);
   grub_dprintf ("ohci", "portstatus=0x%02x\n", status);

   return GRUB_ERR_NONE;
}

static grub_usb_speed_t
grub_ohci_detect_dev (grub_usb_controller_t dev, int port)
{
   struct grub_ohci *o = (struct grub_ohci *) dev->data;
   grub_uint32_t status;

   status = grub_ohci_readreg32 (o, GRUB_OHCI_REG_RHUBPORT + port);

   grub_dprintf ("ohci", "detect_dev status=0x%02x\n", status);

   if (! (status & 1))
     return GRUB_USB_SPEED_NONE;
   else if (status & (1 << 9))
     return GRUB_USB_SPEED_LOW;
   else
     return GRUB_USB_SPEED_FULL;
}

static int
grub_ohci_hubports (grub_usb_controller_t dev)
{
  struct grub_ohci *o = (struct grub_ohci *) dev->data;
  grub_uint32_t portinfo;

  portinfo = grub_ohci_readreg32 (o, GRUB_OHCI_REG_RHUBA);

  grub_dprintf ("ohci", "root hub ports=%d\n", portinfo & 0xFF);

  /* The root hub has exactly two ports.  */
  return portinfo & 0xFF;
}



static struct grub_usb_controller_dev usb_controller =
{
  .name = "ohci",
  .iterate = grub_ohci_iterate,
  .transfer = grub_ohci_transfer,
  .hubports = grub_ohci_hubports,
  .portstatus = grub_ohci_portstatus,
  .detect_dev = grub_ohci_detect_dev
};

GRUB_MOD_INIT(ohci)
{
  grub_ohci_inithw ();
  grub_usb_controller_dev_register (&usb_controller);
}

GRUB_MOD_FINI(ohci)
{
  grub_usb_controller_dev_unregister (&usb_controller);
}
