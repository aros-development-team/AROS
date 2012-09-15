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
#include <grub/machine/memory.h>
#include <grub/machine/console.h>
#include <grub/offsets.h>
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
#include <grub/cpu/floppy.h>
#include <grub/cpu/tsc.h>
#ifdef GRUB_MACHINE_QEMU
#include <grub/machine/kernel.h>
#endif

extern grub_uint8_t _start[];
extern grub_uint8_t _end[];
extern grub_uint8_t _edata[];

void  __attribute__ ((noreturn))
grub_exit (void)
{
  /* We can't use grub_fatal() in this function.  This would create an infinite
     loop, since grub_fatal() calls grub_abort() which in turn calls grub_exit().  */
  while (1)
    grub_cpu_idle ();
}

#ifdef GRUB_MACHINE_QEMU
grub_addr_t grub_modbase;
#else
grub_addr_t grub_modbase = ALIGN_UP((grub_addr_t) _end, GRUB_KERNEL_MACHINE_MOD_ALIGN);
#endif

void
grub_machine_init (void)
{
#ifdef GRUB_MACHINE_QEMU
  grub_modbase = grub_core_entry_addr + (_edata - _start);

  grub_qemu_init_cirrus ();
#endif
  /* Initialize the console as early as possible.  */
  grub_vga_text_init ();

  auto int NESTED_FUNC_ATTR heap_init (grub_uint64_t, grub_uint64_t, 
				       grub_memory_type_t);
  int NESTED_FUNC_ATTR heap_init (grub_uint64_t addr, grub_uint64_t size,
				  grub_memory_type_t type)
  {
#if GRUB_CPU_SIZEOF_VOID_P == 4
    /* Restrict ourselves to 32-bit memory space.  */
    if (addr > GRUB_ULONG_MAX)
      return 0;
    if (addr + size > GRUB_ULONG_MAX)
      size = GRUB_ULONG_MAX - addr;
#endif

    if (type != GRUB_MEMORY_AVAILABLE)
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

    grub_mm_init_region ((void *) (grub_addr_t) addr, (grub_size_t) size);

    return 0;
  }

#if defined (GRUB_MACHINE_MULTIBOOT) || defined (GRUB_MACHINE_QEMU)
  grub_machine_mmap_init ();
#endif
  grub_machine_mmap_iterate (heap_init);

  grub_tsc_init ();
}

void
grub_machine_get_bootlocation (char **device __attribute__ ((unused)),
			       char **path __attribute__ ((unused)))
{
}

void
grub_machine_fini (void)
{
  grub_vga_text_fini ();
  grub_stop_floppy ();
}
