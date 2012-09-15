/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2010  Free Software Foundation, Inc.
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

#include <grub/relocator.h>
#include <grub/relocator_private.h>
#include <grub/memory.h>
#include <grub/ieee1275/ieee1275.h>

unsigned 
grub_relocator_firmware_get_max_events (void)
{
  int counter = 0;
  auto int NESTED_FUNC_ATTR count (grub_uint64_t addr __attribute__ ((unused)),
				   grub_uint64_t len __attribute__ ((unused)),
				   grub_memory_type_t type __attribute__ ((unused)));
  int NESTED_FUNC_ATTR count (grub_uint64_t addr __attribute__ ((unused)), 
			      grub_uint64_t len __attribute__ ((unused)), 
			      grub_memory_type_t type __attribute__ ((unused)))
  {
    counter++;
    return 0;
  }

  if (grub_ieee1275_test_flag (GRUB_IEEE1275_FLAG_CANNOT_INTERPRET))
    return 0;
  grub_machine_mmap_iterate (count);
  return 2 * counter;
}

unsigned 
grub_relocator_firmware_fill_events (struct grub_relocator_mmap_event *events)
{
  int counter = 0;
  auto int NESTED_FUNC_ATTR fill (grub_uint64_t addr, grub_uint64_t len,
				  grub_memory_type_t type);
  int NESTED_FUNC_ATTR fill (grub_uint64_t addr, grub_uint64_t len,
			     grub_memory_type_t type)
  {
    if (type != GRUB_MEMORY_AVAILABLE)
      return 0;

    if (grub_ieee1275_test_flag (GRUB_IEEE1275_FLAG_NO_PRE1_5M_CLAIM))
      {
	if (addr + len <= 0x180000)
	  return 0;

	if (addr < 0x180000)
	  {
	    len = addr + len - 0x180000;
	    addr = 0x180000;
	  }
      }

    events[counter].type = REG_FIRMWARE_START;
    events[counter].pos = addr;
    counter++;
    events[counter].type = REG_FIRMWARE_END;
    events[counter].pos = addr + len;
    counter++;

    return 0;
  }

  if (grub_ieee1275_test_flag (GRUB_IEEE1275_FLAG_CANNOT_INTERPRET))
    return 0;
  grub_machine_mmap_iterate (fill);
  return counter;
}

int
grub_relocator_firmware_alloc_region (grub_addr_t start, grub_size_t size)
{
  grub_err_t err;
  err = grub_claimmap (start, size);
  grub_errno = 0;
  return (err == 0);
}

void
grub_relocator_firmware_free_region (grub_addr_t start, grub_size_t size)
{
  grub_ieee1275_release (start, size);
}
