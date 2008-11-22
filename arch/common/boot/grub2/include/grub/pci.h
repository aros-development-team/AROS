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

#ifndef	GRUB_PCI_H
#define	GRUB_PCI_H	1

#include <grub/types.h>
#include <grub/symbol.h>

#define  GRUB_PCI_ADDR_SPACE_MASK	0x01
#define  GRUB_PCI_ADDR_SPACE_MEMORY	0x00
#define  GRUB_PCI_ADDR_SPACE_IO		0x01

#define  GRUB_PCI_ADDR_MEM_TYPE_MASK	0x06
#define  GRUB_PCI_ADDR_MEM_TYPE_32	0x00	/* 32 bit address */
#define  GRUB_PCI_ADDR_MEM_TYPE_1M	0x02	/* Below 1M [obsolete] */
#define  GRUB_PCI_ADDR_MEM_TYPE_64	0x04	/* 64 bit address */
#define  GRUB_PCI_ADDR_MEM_PREFETCH	0x08	/* prefetchable */

#define  GRUB_PCI_ADDR_MEM_MASK		~0xf
#define  GRUB_PCI_ADDR_IO_MASK		~0x03

typedef grub_uint32_t grub_pci_id_t;
typedef int (*grub_pci_iteratefunc_t) (int bus, int device, int func,
				       grub_pci_id_t pciid);
typedef grub_uint32_t grub_pci_address_t;

grub_pci_address_t EXPORT_FUNC(grub_pci_make_address) (int bus, int device,
						       int function, int reg);

void EXPORT_FUNC(grub_pci_iterate) (grub_pci_iteratefunc_t hook);

#include <grub/cpu/pci.h>

#endif /* GRUB_PCI_H */
