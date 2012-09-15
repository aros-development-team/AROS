/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009,2010  Free Software Foundation, Inc.
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
#include <grub/misc.h>
#include <grub/env.h>
#include <grub/time.h>
#include <grub/types.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/time.h>
#include <grub/machine/kernel.h>
#include <grub/machine/memory.h>
#include <grub/arc/console.h>
#include <grub/cpu/memory.h>
#include <grub/cpu/time.h>
#include <grub/memory.h>
#include <grub/term.h>
#include <grub/arc/arc.h>
#include <grub/offsets.h>
#include <grub/i18n.h>

const char *type_names[] = {
#ifdef GRUB_CPU_WORDS_BIGENDIAN
  NULL,
#endif
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  "eisa", "tc", "scsi", "dti", "multi", "disk", "tape", "cdrom", "worm",
  "serial", "net", "video", "par", "point", "key", "audio", "other",
  "rdisk", "fdisk", "tape", "modem", "monitor", "print", "pointer",
  "keyboard", "term", "other", "line", "network", NULL
};

static int
iterate_rec (const char *prefix, const struct grub_arc_component *parent,
	     int (*hook) (const char *name,
			  const struct grub_arc_component *comp),
	     int alt_names)
{
  const struct grub_arc_component *comp;
  FOR_ARC_CHILDREN(comp, parent)
    {
      char *name;
      const char *cname = NULL;
      if (comp->type < ARRAY_SIZE (type_names))
	cname = type_names[comp->type];
      if (!cname)
	cname = "unknown";
      if (alt_names)
	name = grub_xasprintf ("%s/%s%lu", prefix, cname, comp->key);
      else
	name = grub_xasprintf ("%s%s(%lu)", prefix, cname, comp->key);
      if (!name)
	return 1;
      if (hook (name, comp))
	{
	  grub_free (name);
	  return 1;
	}
      if (iterate_rec ((parent ? name : prefix), comp, hook, alt_names))
	{
	  grub_free (name);
	  return 1;
	}
      grub_free (name);
    }
  return 0;
}

int
grub_arc_iterate_devs (int (*hook) (const char *name,
				    const struct grub_arc_component *comp),
		       int alt_names)
{
  return iterate_rec ((alt_names ? "arc" : ""), NULL, hook, alt_names);
}

grub_err_t
grub_machine_mmap_iterate (grub_memory_hook_t hook)
{
  struct grub_arc_memory_descriptor *cur = NULL;
  while (1)
    {
      grub_memory_type_t type;
      cur = GRUB_ARC_FIRMWARE_VECTOR->getmemorydescriptor (cur);
      if (!cur)
	return GRUB_ERR_NONE;
      switch (cur->type)
	{
	case GRUB_ARC_MEMORY_EXCEPTION_BLOCK:
	case GRUB_ARC_MEMORY_SYSTEM_PARAMETER_BLOCK:
	case GRUB_ARC_MEMORY_FW_PERMANENT:
	default:
	  type = GRUB_MEMORY_RESERVED;
	  break;

	case GRUB_ARC_MEMORY_FW_TEMPORARY:
	case GRUB_ARC_MEMORY_FREE:
	case GRUB_ARC_MEMORY_LOADED:
	case GRUB_ARC_MEMORY_FREE_CONTIGUOUS:
	  type = GRUB_MEMORY_AVAILABLE;
	  break;
	case GRUB_ARC_MEMORY_BADRAM:
	  type = GRUB_MEMORY_BADRAM;
	  break;
	}
      if (hook (((grub_uint64_t) cur->start_page) << 12,
		((grub_uint64_t) cur->num_pages)  << 12, type))
	return GRUB_ERR_NONE;
    }
}

extern grub_uint32_t grub_total_modules_size __attribute__ ((section(".text")));
grub_addr_t grub_modbase;

void
grub_machine_init (void)
{
  struct grub_arc_memory_descriptor *cur = NULL;

  grub_modbase = GRUB_KERNEL_MIPS_ARC_LINK_ADDR - grub_total_modules_size;
  grub_console_init_early ();

  /* FIXME: measure this.  */
  grub_arch_cpuclock = 150000000;
  grub_install_get_time_ms (grub_rtc_get_time_ms);

  while (1)
    {
      grub_uint64_t start, end;
      cur = GRUB_ARC_FIRMWARE_VECTOR->getmemorydescriptor (cur);
      if (!cur)
	break;
      if (cur->type != GRUB_ARC_MEMORY_FREE
	  && cur->type != GRUB_ARC_MEMORY_LOADED
	  && cur->type != GRUB_ARC_MEMORY_FREE_CONTIGUOUS)
	continue;
      start = ((grub_uint64_t) cur->start_page) << 12;
      end = ((grub_uint64_t) cur->num_pages)  << 12;
      end += start;
      if ((grub_uint64_t) end > ((GRUB_KERNEL_MIPS_ARC_LINK_ADDR
				  - grub_total_modules_size) & 0x1fffffff))
	end = ((GRUB_KERNEL_MIPS_ARC_LINK_ADDR - grub_total_modules_size)
	       & 0x1fffffff);
      if (end > start)
	grub_mm_init_region ((void *) (grub_addr_t) (start | 0x80000000),
			     end - start);
    }

  grub_console_init_lately ();

  grub_arcdisk_init ();
}

void
grub_machine_fini (void)
{
}

void
grub_halt (void)
{
  GRUB_ARC_FIRMWARE_VECTOR->powerdown ();

  grub_millisleep (1500);

  grub_puts_ (N_("Shutdown failed"));
  grub_refresh ();
  while (1);
}

void
grub_exit (void)
{
  GRUB_ARC_FIRMWARE_VECTOR->exit ();

  grub_millisleep (1500);

  grub_puts_ (N_("Exit failed"));
  grub_refresh ();
  while (1);
}

