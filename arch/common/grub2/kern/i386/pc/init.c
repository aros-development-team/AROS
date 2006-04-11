/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002, 2003, 2004, 2005  Free Software Foundation, Inc.
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

#include <grub/kernel.h>
#include <grub/mm.h>
#include <grub/machine/init.h>
#include <grub/machine/memory.h>
#include <grub/machine/console.h>
#include <grub/machine/biosdisk.h>
#include <grub/machine/kernel.h>
#include <grub/types.h>
#include <grub/err.h>
#include <grub/dl.h>
#include <grub/misc.h>
#include <grub/loader.h>
#include <grub/env.h>
#include <grub/cache.h>

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

  grub_sprintf (dev, "(%cd%u",
		(grub_boot_drive & 0x80) ? 'h' : 'f',
		grub_boot_drive & 0x7f);
  
  if (grub_install_dos_part >= 0)
    grub_sprintf (dev + grub_strlen (dev), ",%u", grub_install_dos_part);

  if (grub_install_bsd_part >= 0)
    grub_sprintf (dev + grub_strlen (dev), ",%c", grub_install_bsd_part + 'a');

  grub_sprintf (dev + grub_strlen (dev), ")%s", grub_prefix);
  grub_strcpy (grub_prefix, dev);
  
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
      }
}

void
grub_machine_init (void)
{
  grub_uint32_t cont;
  struct grub_machine_mmap_entry *entry
    = (struct grub_machine_mmap_entry *) GRUB_MEMORY_MACHINE_SCRATCH_ADDR;
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
  
  /* Check if grub_get_mmap_entry works.  */
  cont = grub_get_mmap_entry (entry, 0);

  if (entry->size)
    do
      {
	/* Avoid the lower memory.  */
	if (entry->addr < 0x100000)
	  {
	    if (entry->len <= 0x100000 - entry->addr)
	      goto next;

	    entry->len -= 0x100000 - entry->addr;
	    entry->addr = 0x100000;
	  }
	
	/* Ignore >4GB.  */
	if (entry->addr <= 0xFFFFFFFF && entry->type == 1)
	  {
	    grub_addr_t addr;
	    grub_size_t len;

	    addr = (grub_addr_t) entry->addr;
	    len = ((addr + entry->len > 0xFFFFFFFF)
		   ? 0xFFFFFFFF - addr
		   : (grub_size_t) entry->len);
	    add_mem_region (addr, len);
	  }
	
      next:
	if (! cont)
	  break;
	
	cont = grub_get_mmap_entry (entry, cont);
      }
    while (entry->size);
  else
    {
      grub_uint32_t eisa_mmap = grub_get_eisa_mmap ();

      if (eisa_mmap)
	{
	  if ((eisa_mmap & 0xFFFF) == 0x3C00)
	    add_mem_region (0x100000, (eisa_mmap << 16) + 0x100000 * 15);
	  else
	    {
	      add_mem_region (0x100000, (eisa_mmap & 0xFFFF) << 10);
	      add_mem_region (0x1000000, eisa_mmap << 16);
	    }
	}
      else
	add_mem_region (0x100000, grub_get_memsize (1) << 10);
    }

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
  
  /* The memory system was initialized, thus register built-in devices.  */
  grub_biosdisk_init ();


  /* Initialize the prefix.  */
  grub_env_set ("prefix", make_install_device ());
}

void
grub_machine_fini (void)
{
  grub_biosdisk_fini ();
  grub_console_fini ();
}

/* Return the end of the core image.  */
grub_addr_t
grub_arch_modules_addr (void)
{
  return grub_end_addr;
}
