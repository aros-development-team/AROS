/* pci.c - Generic PCI interfaces.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2007,2009  Free Software Foundation, Inc.
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
#include <grub/pci.h>
#include <grub/mm.h>

/* FIXME: correctly support 64-bit architectures.  */
/* #if GRUB_TARGET_SIZEOF_VOID_P == 4 */
struct grub_pci_dma_chunk *
grub_memalign_dma32 (grub_size_t align, grub_size_t size)
{
  return grub_memalign (align, size);
}

void
grub_dma_free (struct grub_pci_dma_chunk *ch)
{
  grub_free (ch);
}
/* #endif */

#ifdef GRUB_MACHINE_MIPS_YEELOONG
volatile void *
grub_dma_get_virt (struct grub_pci_dma_chunk *ch)
{
  return (void *) ((((grub_uint32_t) ch) & 0x1fffffff) | 0xa0000000);
}

grub_uint32_t
grub_dma_get_phys (struct grub_pci_dma_chunk *ch)
{
  return (((grub_uint32_t) ch) & 0x1fffffff) | 0x80000000;
}
#else

volatile void *
grub_dma_get_virt (struct grub_pci_dma_chunk *ch)
{
  return (void *) ch;
}

grub_uint32_t
grub_dma_get_phys (struct grub_pci_dma_chunk *ch)
{
  return (grub_uint32_t) (grub_addr_t) ch;
}

#endif

grub_pci_address_t
grub_pci_make_address (grub_pci_device_t dev, int reg)
{
  return (1 << 31) | (dev.bus << 16) | (dev.device << 11)
    | (dev.function << 8) | reg;
}

void
grub_pci_iterate (grub_pci_iteratefunc_t hook)
{
  grub_pci_device_t dev;
  grub_pci_address_t addr;
  grub_pci_id_t id;
  grub_uint32_t hdr;

  for (dev.bus = 0; dev.bus < GRUB_PCI_NUM_BUS; dev.bus++)
    {
      for (dev.device = 0; dev.device < GRUB_PCI_NUM_DEVICES; dev.device++)
	{
	  for (dev.function = 0; dev.function < 8; dev.function++)
	    {
	      addr = grub_pci_make_address (dev, GRUB_PCI_REG_PCI_ID);
	      id = grub_pci_read (addr);

	      /* Check if there is a device present.  */
	      if (id >> 16 == 0xFFFF)
		{
		  if (dev.function == 0)
		    /* Devices are required to implement function 0, so if
		       it's missing then there is no device here.  */
		    break;
		  else
		    continue;
		}

#ifdef GRUB_MACHINE_MIPS_YEELOONG
	      /* Skip ghosts.  */
	      if (id == GRUB_YEELOONG_OHCI_PCIID
		  && dev.function == GRUB_YEELOONG_OHCI_GHOST_FUNCTION)
		continue;
	      if (id == GRUB_YEELOONG_EHCI_PCIID
		  && dev.function == GRUB_YEELOONG_EHCI_GHOST_FUNCTION)
		continue;
#endif

	      if (hook (dev, id))
		return;

	      /* Probe only func = 0 if the device if not multifunction */
	      if (dev.function == 0)
		{
		  addr = grub_pci_make_address (dev, GRUB_PCI_REG_CACHELINE);
		  hdr = grub_pci_read (addr);
		  if (!(hdr & 0x800000))
		    break;
		}
	    }
	}
    }
}
