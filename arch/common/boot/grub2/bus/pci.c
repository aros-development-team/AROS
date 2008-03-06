/* pci.c - Generic PCI interfaces.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2007  Free Software Foundation, Inc.
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

grub_pci_address_t
grub_pci_make_address (int bus, int device, int function, int reg)
{
  return (1 << 31) | (bus << 16) | (device << 11) | (function << 8) | (reg << 2);
}

void
grub_pci_iterate (grub_pci_iteratefunc_t hook)
{
  int bus;
  int dev;
  int func;
  grub_pci_address_t addr;
  grub_pci_id_t id;

  for (bus = 0; bus < 256; bus++)
    {
      for (dev = 0; dev < 32; dev++)
	{
	  for (func = 0; func < 3; func++)
	    {
	      addr = grub_pci_make_address (bus, dev, func, 0);
	      id = grub_pci_read (addr);

	      /* Check if there is a device present.  */
	      if (id >> 16 == 0xFFFF)
		continue;

	      if (hook (bus, dev, func, id))
		return;
	    }
	}
    }
}
