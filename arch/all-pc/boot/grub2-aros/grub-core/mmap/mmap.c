/* Mmap management. */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
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

#include <grub/memory.h>
#include <grub/machine/memory.h>
#include <grub/err.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/command.h>
#include <grub/dl.h>
#include <grub/i18n.h>

GRUB_MOD_LICENSE ("GPLv3+");

#ifndef GRUB_MMAP_REGISTER_BY_FIRMWARE

struct grub_mmap_region *grub_mmap_overlays = 0;
static int curhandle = 1;

#endif

grub_err_t
grub_mmap_iterate (grub_memory_hook_t hook)
{

  /* This function resolves overlapping regions and sorts the memory map.
     It uses scanline (sweeping) algorithm.
  */
  /* If same page is used by multiple types it's resolved
     according to priority:
     1 - free memory
     2 - memory usable by firmware-aware code
     3 - unusable memory
     4 - a range deliberately empty
  */
  int priority[] =
    {
      [GRUB_MEMORY_AVAILABLE] = 1,
      [GRUB_MEMORY_RESERVED] = 3,
      [GRUB_MEMORY_ACPI] = 2,
      [GRUB_MEMORY_CODE] = 3,
      [GRUB_MEMORY_NVS] = 3,
      [GRUB_MEMORY_HOLE] = 4,
    };

  int i, done;

  /* Scanline events. */
  struct grub_mmap_scan
  {
    /* At which memory address. */
    grub_uint64_t pos;
    /* 0 = region starts, 1 = region ends. */
    int type;
    /* Which type of memory region? */
    int memtype;
  };
  struct grub_mmap_scan *scanline_events;
  struct grub_mmap_scan t;

  /* Previous scanline event. */
  grub_uint64_t lastaddr;
  int lasttype;
  /* Current scanline event. */
  int curtype;
  /* How many regions of given type overlap at current location? */
  int present[ARRAY_SIZE (priority)];
  /* Number of mmap chunks. */
  int mmap_num;

#ifndef GRUB_MMAP_REGISTER_BY_FIRMWARE
  struct grub_mmap_region *cur;
#endif

  auto int NESTED_FUNC_ATTR count_hook (grub_uint64_t, grub_uint64_t,
					grub_uint32_t);
  int NESTED_FUNC_ATTR count_hook (grub_uint64_t addr __attribute__ ((unused)),
				   grub_uint64_t size __attribute__ ((unused)),
				   grub_memory_type_t type __attribute__ ((unused)))
  {
    mmap_num++;
    return 0;
  }

  auto int NESTED_FUNC_ATTR fill_hook (grub_uint64_t, grub_uint64_t,
					grub_uint32_t);
  int NESTED_FUNC_ATTR fill_hook (grub_uint64_t addr,
				  grub_uint64_t size,
				  grub_memory_type_t type)
  {
    scanline_events[i].pos = addr;
    scanline_events[i].type = 0;
    if (type < ARRAY_SIZE (priority) && priority[type])
      scanline_events[i].memtype = type;
    else
      {
	grub_dprintf ("mmap", "Unknown memory type %d. Assuming unusable\n",
		      type);
	scanline_events[i].memtype = GRUB_MEMORY_RESERVED;
      }
    i++;

    scanline_events[i].pos = addr + size;
    scanline_events[i].type = 1;
    scanline_events[i].memtype = scanline_events[i - 1].memtype;
    i++;

    return 0;
  }

  mmap_num = 0;

#ifndef GRUB_MMAP_REGISTER_BY_FIRMWARE
  for (cur = grub_mmap_overlays; cur; cur = cur->next)
    mmap_num++;
#endif

  grub_machine_mmap_iterate (count_hook);

  /* Initialize variables. */
  grub_memset (present, 0, sizeof (present));
  scanline_events = (struct grub_mmap_scan *)
    grub_malloc (sizeof (struct grub_mmap_scan) * 2 * mmap_num);

  if (! scanline_events)
    return grub_errno;

  i = 0;
#ifndef GRUB_MMAP_REGISTER_BY_FIRMWARE
  /* Register scanline events. */
  for (cur = grub_mmap_overlays; cur; cur = cur->next)
    {
      scanline_events[i].pos = cur->start;
      scanline_events[i].type = 0;
      if (cur->type < ARRAY_SIZE (priority) && priority[cur->type])
	scanline_events[i].memtype = cur->type;
      else
	scanline_events[i].memtype = GRUB_MEMORY_RESERVED;
      i++;

      scanline_events[i].pos = cur->end;
      scanline_events[i].type = 1;
      scanline_events[i].memtype = scanline_events[i - 1].memtype;
      i++;
    }
#endif /* ! GRUB_MMAP_REGISTER_BY_FIRMWARE */

  grub_machine_mmap_iterate (fill_hook);

  /* Primitive bubble sort. It has complexity O(n^2) but since we're
     unlikely to have more than 100 chunks it's probably one of the
     fastest for one purpose. */
  done = 1;
  while (done)
    {
      done = 0;
      for (i = 0; i < 2 * mmap_num - 1; i++)
	if (scanline_events[i + 1].pos < scanline_events[i].pos
	    || (scanline_events[i + 1].pos == scanline_events[i].pos
		&& scanline_events[i + 1].type == 0
		&& scanline_events[i].type == 1))
	  {
	    t = scanline_events[i + 1];
	    scanline_events[i + 1] = scanline_events[i];
	    scanline_events[i] = t;
	    done = 1;
	  }
    }

  lastaddr = scanline_events[0].pos;
  lasttype = scanline_events[0].memtype;
  for (i = 0; i < 2 * mmap_num; i++)
    {
      unsigned k;
      /* Process event. */
      if (scanline_events[i].type)
	present[scanline_events[i].memtype]--;
      else
	present[scanline_events[i].memtype]++;

      /* Determine current region type. */
      curtype = -1;
      for (k = 0; k < ARRAY_SIZE (priority); k++)
	if (present[k] && (curtype == -1 || priority[k] > priority[curtype]))
	  curtype = k;

      /* Announce region to the hook if necessary. */
      if ((curtype == -1 || curtype != lasttype)
	  && lastaddr != scanline_events[i].pos
	  && lasttype != -1
	  && lasttype != GRUB_MEMORY_HOLE
	  && hook (lastaddr, scanline_events[i].pos - lastaddr, lasttype))
	{
	  grub_free (scanline_events);
	  return GRUB_ERR_NONE;
	}

      /* Update last values if necessary. */
      if (curtype == -1 || curtype != lasttype)
	{
	  lasttype = curtype;
	  lastaddr = scanline_events[i].pos;
	}
    }

  grub_free (scanline_events);
  return GRUB_ERR_NONE;
}

#ifndef GRUB_MMAP_REGISTER_BY_FIRMWARE
int
grub_mmap_register (grub_uint64_t start, grub_uint64_t size, int type)
{
  struct grub_mmap_region *cur;

  grub_dprintf ("mmap", "registering\n");

  cur = (struct grub_mmap_region *)
    grub_malloc (sizeof (struct grub_mmap_region));
  if (! cur)
    return 0;

  cur->next = grub_mmap_overlays;
  cur->start = start;
  cur->end = start + size;
  cur->type = type;
  cur->handle = curhandle++;
  grub_mmap_overlays = cur;

  if (grub_machine_mmap_register (start, size, type, curhandle))
    {
      grub_mmap_overlays = cur->next;
      grub_free (cur);
      return 0;
    }

  return cur->handle;
}

grub_err_t
grub_mmap_unregister (int handle)
{
  struct grub_mmap_region *cur, *prev;

  for (cur = grub_mmap_overlays, prev = 0; cur; prev = cur, cur = cur->next)
    if (handle == cur->handle)
      {
	grub_err_t err;
	err = grub_machine_mmap_unregister (handle);
	if (err)
	  return err;

	if (prev)
	  prev->next = cur->next;
	else
	  grub_mmap_overlays = cur->next;
	grub_free (cur);
	return GRUB_ERR_NONE;
      }
  return grub_error (GRUB_ERR_BUG, "mmap overlay not found");
}

#endif /* ! GRUB_MMAP_REGISTER_BY_FIRMWARE */

#define CHUNK_SIZE	0x400

static inline grub_uint64_t
fill_mask (grub_uint64_t addr, grub_uint64_t mask, grub_uint64_t iterator)
{
  int i, j;
  grub_uint64_t ret = (addr & mask);

  /* Find first fixed bit. */
  for (i = 0; i < 64; i++)
    if ((mask & (1ULL << i)) != 0)
      break;
  j = 0;
  for (; i < 64; i++)
    if ((mask & (1ULL << i)) == 0)
      {
	if ((iterator & (1ULL << j)) != 0)
	  ret |= 1ULL << i;
	j++;
      }
  return ret;
}

static grub_err_t
grub_cmd_badram (grub_command_t cmd __attribute__ ((unused)),
		 int argc, char **args)
{
  char * str;
  grub_uint64_t badaddr, badmask;

  auto int NESTED_FUNC_ATTR hook (grub_uint64_t, grub_uint64_t,
				 grub_memory_type_t);
  int NESTED_FUNC_ATTR hook (grub_uint64_t addr,
			     grub_uint64_t size,
			     grub_memory_type_t type __attribute__ ((unused)))
  {
    grub_uint64_t iterator, low, high, cur;
    int tail, var;
    int i;
    grub_dprintf ("badram", "hook %llx+%llx\n", (unsigned long long) addr,
		  (unsigned long long) size);

    /* How many trailing zeros? */
    for (tail = 0; ! (badmask & (1ULL << tail)); tail++);

    /* How many zeros in mask? */
    var = 0;
    for (i = 0; i < 64; i++)
      if (! (badmask & (1ULL << i)))
	var++;

    if (fill_mask (badaddr, badmask, 0) >= addr)
      iterator = 0;
    else
      {
	low = 0;
	high = ~0ULL;
	/* Find starting value. Keep low and high such that
	   fill_mask (low) < addr and fill_mask (high) >= addr;
	*/
	while (high - low > 1)
	  {
	    cur = (low + high) / 2;
	    if (fill_mask (badaddr, badmask, cur) >= addr)
	      high = cur;
	    else
	      low = cur;
	  }
	iterator = high;
      }

    for (; iterator < (1ULL << (var - tail))
	   && (cur = fill_mask (badaddr, badmask, iterator)) < addr + size;
	 iterator++)
      {
	grub_dprintf ("badram", "%llx (size %llx) is a badram range\n",
		      (unsigned long long) cur, (1ULL << tail));
	grub_mmap_register (cur, (1ULL << tail), GRUB_MEMORY_HOLE);
      }
    return 0;
  }

  if (argc != 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("one argument expected"));

  grub_dprintf ("badram", "executing badram\n");

  str = args[0];

  while (1)
    {
      /* Parse address and mask.  */
      badaddr = grub_strtoull (str, &str, 16);
      if (*str == ',')
	str++;
      badmask = grub_strtoull (str, &str, 16);
      if (*str == ',')
	str++;

      if (grub_errno == GRUB_ERR_BAD_NUMBER)
	{
	  grub_errno = 0;
	  return GRUB_ERR_NONE;
	}

      /* When part of a page is tainted, we discard the whole of it.  There's
	 no point in providing sub-page chunks.  */
      badmask &= ~(CHUNK_SIZE - 1);

      grub_dprintf ("badram", "badram %llx:%llx\n",
		    (unsigned long long) badaddr, (unsigned long long) badmask);

      grub_mmap_iterate (hook);
    }
}

static grub_uint64_t
parsemem (const char *str)
{
  grub_uint64_t ret;
  char *ptr;

  ret = grub_strtoul (str, &ptr, 0);

  switch (*ptr)
    {
    case 'K':
      return ret << 10;
    case 'M':
      return ret << 20;
    case 'G':
      return ret << 30;
    case 'T':
      return ret << 40;
    }
  return ret;
}

static grub_err_t
grub_cmd_cutmem (grub_command_t cmd __attribute__ ((unused)),
		 int argc, char **args)
{
  grub_uint64_t from, to;

  auto int NESTED_FUNC_ATTR hook (grub_uint64_t, grub_uint64_t,
				 grub_memory_type_t);
  int NESTED_FUNC_ATTR hook (grub_uint64_t addr,
			     grub_uint64_t size,
			     grub_memory_type_t type __attribute__ ((unused)))
  {
    grub_uint64_t end = addr + size;

    if (addr <= from)
      addr = from;
    if (end >= to)
      end = to;

    if (end <= addr)
      return 0;

    grub_mmap_register (addr, end - addr, GRUB_MEMORY_HOLE);
    return 0;
  }

  if (argc != 2)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("two arguments expected"));

  from = parsemem (args[0]);
  if (grub_errno)
    return grub_errno;

  to = parsemem (args[1]);
  if (grub_errno)
    return grub_errno;

  grub_mmap_iterate (hook);

  return GRUB_ERR_NONE;
}

static grub_command_t cmd, cmd_cut;


GRUB_MOD_INIT(mmap)
{
  cmd = grub_register_command ("badram", grub_cmd_badram,
			       N_("ADDR1,MASK1[,ADDR2,MASK2[,...]]"),
			       N_("Declare memory regions as faulty (badram)."));
  cmd_cut = grub_register_command ("cutmem", grub_cmd_cutmem,
				   N_("FROM[K|M|G] TO[K|M|G]"),
				   N_("Remove any memory regions in specified range."));

}

GRUB_MOD_FINI(mmap)
{
  grub_unregister_command (cmd);
  grub_unregister_command (cmd_cut);
}

