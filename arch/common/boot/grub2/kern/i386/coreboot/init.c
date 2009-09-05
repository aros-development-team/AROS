/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2004,2005,2006,2007,2008,2009  Free Software Foundation, Inc.
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

#include <grub/kernel.h>
#include <grub/mm.h>
#include <grub/machine/time.h>
#include <grub/machine/init.h>
#include <grub/machine/memory.h>
#include <grub/machine/console.h>
#include <grub/machine/kernel.h>
#include <grub/machine/machine.h>
#include <grub/types.h>
#include <grub/err.h>
#include <grub/dl.h>
#include <grub/misc.h>
#include <grub/loader.h>
#include <grub/env.h>
#include <grub/cache.h>
#include <grub/time.h>
#include <grub/symbol.h>
#include <grub/cpu/io.h>
#include <grub/cpu/kernel.h>
#include <grub/cpu/tsc.h>

#define GRUB_FLOPPY_REG_DIGITAL_OUTPUT		0x3f2

extern char _start[];
extern char _end[];

grub_addr_t grub_os_area_addr;
grub_size_t grub_os_area_size;

grub_uint32_t
grub_get_rtc (void)
{
  grub_fatal ("grub_get_rtc() is not implemented.\n");
}

/* Stop the floppy drive from spinning, so that other software is
   jumped to with a known state.  */
void
grub_stop_floppy (void)
{
  grub_outb (0, GRUB_FLOPPY_REG_DIGITAL_OUTPUT);
}

void
grub_exit (void)
{
  grub_fatal ("grub_exit() is not implemented.\n");
}

void
grub_arch_sync_caches (void *address __attribute__ ((unused)),
		       grub_size_t len __attribute__ ((unused)))
{
}

void
grub_machine_init (void)
{
  /* Initialize the console as early as possible.  */
  grub_vga_text_init ();

  auto int NESTED_FUNC_ATTR heap_init (grub_uint64_t, grub_uint64_t, grub_uint32_t);
  int NESTED_FUNC_ATTR heap_init (grub_uint64_t addr, grub_uint64_t size, grub_uint32_t type)
  {
#if GRUB_CPU_SIZEOF_VOID_P == 4
    /* Restrict ourselves to 32-bit memory space.  */
    if (addr > GRUB_ULONG_MAX)
      return 0;
    if (addr + size > GRUB_ULONG_MAX)
      size = GRUB_ULONG_MAX - addr;
#endif

    if (type != GRUB_MACHINE_MEMORY_AVAILABLE)
      return 0;

    /* Avoid the lower memory.  */
    if (addr < GRUB_MEMORY_MACHINE_LOWER_SIZE)
      {
	if (addr + size <= GRUB_MEMORY_MACHINE_LOWER_SIZE)
	  return 0;
	else
	  {
	    size -= GRUB_MEMORY_MACHINE_LOWER_SIZE - addr;
	    addr = GRUB_MEMORY_MACHINE_LOWER_SIZE;
	  }
      }

    if (addr == GRUB_MEMORY_MACHINE_UPPER_START
	|| (addr >= GRUB_MEMORY_MACHINE_LOWER_SIZE
	    && addr <= GRUB_MEMORY_MACHINE_UPPER_START
	    && (addr + size > GRUB_MEMORY_MACHINE_UPPER_START)))
      {
	grub_size_t quarter = size >> 2;

	grub_os_area_addr = addr;
	grub_os_area_size = size - quarter;
	grub_mm_init_region ((void *) (grub_os_area_addr + grub_os_area_size),
			     quarter);
      }
    else
      grub_mm_init_region ((void *) (grub_addr_t) addr, (grub_size_t) size);

    return 0;
  }

  grub_machine_mmap_init ();
  grub_machine_mmap_iterate (heap_init);

  grub_tsc_init ();
}

void
grub_machine_set_prefix (void)
{
  /* Initialize the prefix.  */
  grub_env_set ("prefix", grub_prefix);
}

void
grub_machine_fini (void)
{
  grub_vga_text_fini ();
  grub_stop_floppy ();
}

/* Return the end of the core image.  */
grub_addr_t
grub_arch_modules_addr (void)
{
#ifdef GRUB_MACHINE_QEMU
  return grub_core_entry_addr + grub_kernel_image_size;
#else
  return ALIGN_UP((grub_addr_t) _end, GRUB_MOD_ALIGN);
#endif
}
