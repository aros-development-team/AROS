/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2007,2008  Free Software Foundation, Inc.
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

#include <grub/machine/memory.h>
#include <grub/machine/lbio.h>
#include <grub/types.h>
#include <grub/err.h>
#include <grub/misc.h>

static grub_err_t
grub_linuxbios_table_iterate (int (*hook) (grub_linuxbios_table_item_t))
{
  grub_linuxbios_table_header_t table_header;
  grub_linuxbios_table_item_t table_item;

  auto int check_signature (grub_linuxbios_table_header_t);
  int check_signature (grub_linuxbios_table_header_t tbl_header)
  {
    if (! grub_memcmp (tbl_header->signature, "LBIO", 4))
      return 1;

    return 0;
  }

  /* Assuming table_header is aligned to its size (8 bytes).  */

  for (table_header = (grub_linuxbios_table_header_t) 0x500;
       table_header < (grub_linuxbios_table_header_t) 0x1000; table_header++)
    if (check_signature (table_header))
      goto signature_found;

  for (table_header = (grub_linuxbios_table_header_t) 0xf0000;
       table_header < (grub_linuxbios_table_header_t) 0x100000; table_header++)
    if (check_signature (table_header))
      goto signature_found;

  grub_fatal ("Could not find coreboot table\n");

signature_found:

  table_item =
    (grub_linuxbios_table_item_t) ((long) table_header +
			       (long) table_header->size);
  for (; table_item->size;
       table_item = (grub_linuxbios_table_item_t) ((long) table_item + (long) table_item->size))
    {
      if (table_item->tag == GRUB_LINUXBIOS_MEMBER_LINK
         && check_signature ((grub_linuxbios_table_header_t) (grub_addr_t)
                             *(grub_uint64_t *) (table_item + 1)))
       {
         table_header = (grub_linuxbios_table_header_t) (grub_addr_t)
           *(grub_uint64_t *) (table_item + 1);
         goto signature_found;   
       }
      if (hook (table_item))
       return 1;
    }

  return 0;
}

grub_err_t
grub_machine_mmap_iterate (grub_memory_hook_t hook)
{
  mem_region_t mem_region;

  auto int iterate_linuxbios_table (grub_linuxbios_table_item_t);
  int iterate_linuxbios_table (grub_linuxbios_table_item_t table_item)
  {
    if (table_item->tag != GRUB_LINUXBIOS_MEMBER_MEMORY)
      return 0;

    mem_region =
      (mem_region_t) ((long) table_item +
				 sizeof (struct grub_linuxbios_table_item));
    while ((long) mem_region < (long) table_item + (long) table_item->size)
      {
	if (hook (mem_region->addr, mem_region->size,
		  /* Multiboot mmaps match with the coreboot mmap definition.
		     Therefore, we can just pass type through.  */
		  mem_region->type))
	  return 1;

	mem_region++;
      }

    return 0;
  }

  grub_linuxbios_table_iterate (iterate_linuxbios_table);

  return 0;
}
