/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2004,2005,2006,2007,2008  Free Software Foundation, Inc.
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
#include <grub/cpu/tsc.h>

struct mem_region
{
  grub_addr_t addr;
  grub_size_t size;
};

#define MAX_REGIONS	32

static struct mem_region mem_regions[MAX_REGIONS];
static int num_regions;

grub_addr_t grub_os_area_addr;
grub_size_t grub_os_area_size;
grub_size_t grub_lower_mem, grub_upper_mem;

void 
grub_arch_sync_caches (void *address __attribute__ ((unused)),
		       grub_size_t len __attribute__ ((unused)))
{
}

static char *
make_install_device (void)
{
  /* XXX: This should be enough.  */
  char dev[100];

  if (grub_prefix[0] != '(')
    {
      /* If the root drive is not set explicitly, assume that it is identical
         to the boot drive.  */
      if (grub_root_drive == 0xFF)
        grub_root_drive = grub_boot_drive;
      
      grub_sprintf (dev, "(%cd%u", (grub_root_drive & 0x80) ? 'h' : 'f',
                    grub_root_drive & 0x7f);
      
      if (grub_install_dos_part >= 0)
	grub_sprintf (dev + grub_strlen (dev), ",%u", grub_install_dos_part + 1);
      
      if (grub_install_bsd_part >= 0)
	grub_sprintf (dev + grub_strlen (dev), ",%c", grub_install_bsd_part + 'a');
      
      grub_sprintf (dev + grub_strlen (dev), ")%s", grub_prefix);
      grub_strcpy (grub_prefix, dev);
    }
      
  return grub_prefix;
}

/* Add a memory region.  */
static void
add_mem_region (grub_addr_t addr, grub_size_t size)
{
  if (num_regions == MAX_REGIONS)
    /* Ignore.  */
    return;

  mem_regions[num_regions].addr = addr;
  mem_regions[num_regions].size = size;
  num_regions++;
}

/* Compact memory regions.  */
static void
compact_mem_regions (void)
{
  int i, j;

  /* Sort them.  */
  for (i = 0; i < num_regions - 1; i++)
    for (j = i + 1; j < num_regions; j++)
      if (mem_regions[i].addr > mem_regions[j].addr)
	{
	  struct mem_region tmp = mem_regions[i];
	  mem_regions[i] = mem_regions[j];
	  mem_regions[j] = tmp;
	}

  /* Merge overlaps.  */
  for (i = 0; i < num_regions - 1; i++)
    if (mem_regions[i].addr + mem_regions[i].size >= mem_regions[i + 1].addr)
      {
	j = i + 1;
	
	if (mem_regions[i].addr + mem_regions[i].size
	    < mem_regions[j].addr + mem_regions[j].size)
	  mem_regions[i].size = (mem_regions[j].addr + mem_regions[j].size
				 - mem_regions[i].addr);

	grub_memmove (mem_regions + j, mem_regions + j + 1,
		      (num_regions - j - 1) * sizeof (struct mem_region));
	i--;
        num_regions--;
      }
}

void
grub_machine_init (void)
{
  int i;
  
  /* Initialize the console as early as possible.  */
  grub_console_init ();
  
  grub_lower_mem = grub_get_memsize (0) << 10;
  
  /* Sanity check.  */
  if (grub_lower_mem < GRUB_MEMORY_MACHINE_RESERVED_END)
    grub_fatal ("too small memory");

#if 0
  /* Turn on Gate A20 to access >1MB.  */
  grub_gate_a20 (1);
#endif

  /* Add the lower memory into free memory.  */
  if (grub_lower_mem >= GRUB_MEMORY_MACHINE_RESERVED_END)
    add_mem_region (GRUB_MEMORY_MACHINE_RESERVED_END,
		    grub_lower_mem - GRUB_MEMORY_MACHINE_RESERVED_END);
  
  auto int NESTED_FUNC_ATTR hook (grub_uint64_t, grub_uint64_t, grub_uint32_t);
  int NESTED_FUNC_ATTR hook (grub_uint64_t addr, grub_uint64_t size, grub_uint32_t type)
    {
      /* Avoid the lower memory.  */
      if (addr < 0x100000)
	{
	  if (size <= 0x100000 - addr)
	    return 0;
	  
	  size -= 0x100000 - addr;
	  addr = 0x100000;
	}
	
      /* Ignore >4GB.  */
      if (addr <= 0xFFFFFFFF && type == GRUB_MACHINE_MEMORY_AVAILABLE)
	{
	  grub_size_t len;
	  
	  len = (grub_size_t) ((addr + size > 0xFFFFFFFF)
		 ? 0xFFFFFFFF - addr
		 : size);
	  add_mem_region (addr, len);
	}

      return 0;
    }

  grub_machine_mmap_iterate (hook);
  
  compact_mem_regions ();

  /* Add the memory regions to free memory, except for the region starting
     from 1MB. This region is partially used for loading OS images.
     For now, 1/4 of this is added to free memory.  */
  for (i = 0; i < num_regions; i++)
    if (mem_regions[i].addr == 0x100000)
      {
	grub_size_t quarter = mem_regions[i].size >> 2;

	grub_upper_mem = mem_regions[i].size;
	grub_os_area_addr = mem_regions[i].addr;
	grub_os_area_size = mem_regions[i].size - quarter;
	grub_mm_init_region ((void *) (grub_os_area_addr + grub_os_area_size),
			     quarter);
      }
    else
      grub_mm_init_region ((void *) mem_regions[i].addr, mem_regions[i].size);
  
  if (! grub_os_area_addr)
    grub_fatal ("no upper memory");

  grub_tsc_init ();
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
  return GRUB_MEMORY_MACHINE_DECOMPRESSION_ADDR
    + (grub_kernel_image_size - GRUB_KERNEL_MACHINE_RAW_SIZE);
}
