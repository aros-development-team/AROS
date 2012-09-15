/* bonito.c - PCI bonito interface.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
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

#include <grub/pci.h>
#include <grub/misc.h>

static grub_uint32_t base_win[GRUB_MACHINE_PCI_NUM_WIN];
static const grub_size_t sizes_win[GRUB_MACHINE_PCI_NUM_WIN] = 
  {GRUB_MACHINE_PCI_WIN1_SIZE, GRUB_MACHINE_PCI_WIN_SIZE, 
   GRUB_MACHINE_PCI_WIN_SIZE};
/* Usage counters.  */
static int usage_win[GRUB_MACHINE_PCI_NUM_WIN];
static grub_addr_t addr_win[GRUB_MACHINE_PCI_NUM_WIN] = 
  {GRUB_MACHINE_PCI_WIN1_ADDR, GRUB_MACHINE_PCI_WIN2_ADDR,
   GRUB_MACHINE_PCI_WIN3_ADDR};

static inline void
write_bases (void)
{
  int i;
  grub_uint32_t reg = 0;
  for (i = 0; i < GRUB_MACHINE_PCI_NUM_WIN; i++) 
    reg |= (((base_win[i] >> GRUB_MACHINE_PCI_WIN_SHIFT) 
	     & GRUB_MACHINE_PCI_WIN_MASK) 
	    << (i * GRUB_MACHINE_PCI_WIN_MASK_SIZE));
  GRUB_MACHINE_PCI_IO_CTRL_REG = reg;
}

volatile void *
grub_pci_device_map_range (grub_pci_device_t dev __attribute__ ((unused)),
			   grub_addr_t base, grub_size_t size)
{
  int i;
  grub_addr_t newbase;

  /* First try already used registers. */
  for (i = 0; i < GRUB_MACHINE_PCI_NUM_WIN; i++)
    if (usage_win[i] && base_win[i] <= base 
	&& base_win[i] + sizes_win[i] > base + size)
      {
	usage_win[i]++;
	return (void *) 
	  (addr_win[i] | (base & GRUB_MACHINE_PCI_WIN_OFFSET_MASK));
      }
  /* Map new register.  */
  newbase = base & ~GRUB_MACHINE_PCI_WIN_OFFSET_MASK;
  for (i = 0; i < GRUB_MACHINE_PCI_NUM_WIN; i++)
    if (!usage_win[i] && newbase <= base 
	&& newbase + sizes_win[i] > base + size)
      {
	usage_win[i]++;
	base_win[i] = newbase;
	write_bases ();
	return (void *) 
	  (addr_win[i] | (base & GRUB_MACHINE_PCI_WIN_OFFSET_MASK));
      }
  grub_fatal ("Out of PCI windows.");
}

void *
grub_pci_device_map_range_cached (grub_pci_device_t dev,
				  grub_addr_t base, grub_size_t size)
{
  return (void *) (((grub_addr_t) grub_pci_device_map_range (dev, base, size))
		   & ~0x20000000);
}

void
grub_pci_device_unmap_range (grub_pci_device_t dev __attribute__ ((unused)),
			     volatile void *mem,
			     grub_size_t size __attribute__ ((unused)))
{
  int i;
  for (i = 0; i < GRUB_MACHINE_PCI_NUM_WIN; i++)
    if (usage_win[i] && addr_win[i] 
	== (((grub_addr_t) mem | 0x20000000)
	    & ~GRUB_MACHINE_PCI_WIN_OFFSET_MASK))
      {
	usage_win[i]--;
	return;
      }
  grub_fatal ("Tried to unmap not mapped region");
}
