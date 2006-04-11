/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002, 2004, 2005  Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef GRUB_INIT_MACHINE_HEADER
#define GRUB_INIT_MACHINE_HEADER	1

#include <grub/types.h>
#include <grub/symbol.h>

/* FIXME: Should these be declared in memory.h?  */
extern grub_size_t EXPORT_VAR(grub_lower_mem);
extern grub_size_t EXPORT_VAR(grub_upper_mem);

extern grub_addr_t EXPORT_VAR(grub_os_area_addr);
extern grub_size_t EXPORT_VAR(grub_os_area_size);

/* Get the memory size in KB. If EXTENDED is zero, return conventional
   memory, otherwise return extended memory.  */
grub_uint16_t grub_get_memsize (int extended);

/* Get a packed EISA memory map. Lower 16 bits are between 1MB and 16MB
   in 1KB parts, and upper 16 bits are above 16MB in 64KB parts.  */
grub_uint32_t grub_get_eisa_mmap (void);

struct grub_machine_mmap_entry
{
  grub_uint32_t size;
  grub_uint64_t addr;
  grub_uint64_t len;
  grub_uint32_t type;
};

/* Get a memory map entry. Return next continuation value. Zero means
   the end.  */
grub_uint32_t grub_get_mmap_entry (struct grub_machine_mmap_entry *entry,
				   grub_uint32_t cont);

/* Turn on/off Gate A20.  */
void grub_gate_a20 (int on);

/* Reboot the machine.  */
void EXPORT_FUNC (grub_reboot) (void);

/* Halt the system, using APM if possible. If NO_APM is true, don't
 * use APM even if it is available.  */
void EXPORT_FUNC (grub_halt) (int no_apm);


#endif /* ! GRUB_INIT_MACHINE_HEADER */
