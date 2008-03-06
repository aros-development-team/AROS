/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2004,2005,2006,2007  Free Software Foundation, Inc.
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

#define GRUB_FLOPPY_REG_DIGITAL_OUTPUT		0x3f2

extern char _start[];
extern char _end[];

grub_addr_t grub_os_area_addr;
grub_size_t grub_os_area_size;
grub_size_t grub_lower_mem, grub_upper_mem;

/* FIXME: we need interrupts to do this right */
static grub_uint32_t grub_time_tics = 0;

grub_uint32_t
grub_get_rtc (void)
{
  return grub_time_tics;
}

/* Stop the floppy drive from spinning, so that other software is
   jumped to with a known state.  */
void
grub_stop_floppy (void)
{
  grub_outb (0, GRUB_FLOPPY_REG_DIGITAL_OUTPUT);
}

void
grub_millisleep (grub_uint32_t ms __attribute__ ((unused)))
{
}

void
grub_exit (void)
{
  grub_printf ("grub_exit() is not implemented.\n");
  grub_stop ();
}

void
grub_arch_sync_caches (void *address __attribute__ ((unused)),
		       grub_size_t len __attribute__ ((unused)))
{
}

static char *
make_install_device (void)
{
  return NULL;
}

void
grub_machine_init (void)
{
  /* Initialize the console as early as possible.  */
  grub_console_init ();

  grub_lower_mem = GRUB_MEMORY_MACHINE_LOWER_USABLE;
  grub_upper_mem = 0;

  auto int heap_init (mem_region_t);
  int heap_init (mem_region_t mem_region)
  {
    grub_uint64_t addr = mem_region->addr;
    grub_uint64_t size = mem_region->size;

#if GRUB_CPU_SIZEOF_VOID_P == 4
    /* Restrict ourselves to 32-bit memory space.  */
    if (addr > ULONG_MAX)
      {
	grub_upper_mem = ULONG_MAX;
	return 0;
      }
    if (addr + size > ULONG_MAX)
      size = ULONG_MAX - addr;
#endif

    grub_upper_mem = grub_max (grub_upper_mem, addr + size);

    if (mem_region->type != GRUB_LINUXBIOS_MEMORY_AVAILABLE)
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
      grub_mm_init_region ((void *) addr, (grub_size_t) size);

    return 0;
  }

  grub_available_iterate (heap_init);

  /* This variable indicates size, not offset.  */
  grub_upper_mem -= GRUB_MEMORY_MACHINE_UPPER_START;
}

void
grub_machine_set_prefix (void)
{
  /* Initialize the prefix.  */
  grub_env_set ("prefix", make_install_device ());
}

void
grub_machine_fini (void)
{
  grub_console_fini ();
}

/* Return the end of the core image.  */
grub_addr_t
grub_arch_modules_addr (void)
{
  return ALIGN_UP(_end, GRUB_MOD_ALIGN);
}
