/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2004  Free Software Foundation, Inc.
 *
 *  GRUB is free software; you can redistribute it and/or modify
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
 *  along with GRUB; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <grub/partition.h>
#include <grub/disk.h>

static grub_partition_map_t grub_partition_map_list;

void
grub_partition_map_register (grub_partition_map_t partmap)
{
  partmap->next = grub_partition_map_list;
  grub_partition_map_list = partmap;
}

void
grub_partition_map_unregister (grub_partition_map_t partmap)
{
  grub_partition_map_t *p, q;
  
  for (p = &grub_partition_map_list, q = *p; q; p = &(q->next), q = q->next)
    if (q == partmap)
      {
        *p = q->next;
	break;
      }
}

int
grub_partition_map_iterate (int (*hook) (const grub_partition_map_t partmap))
{
  grub_partition_map_t p;

  for (p = grub_partition_map_list; p; p = p->next)
    if (hook (p))
      return 1;

  return 0;
}

grub_partition_t
grub_partition_probe (struct grub_disk *disk, const char *str)
{
  grub_partition_t part = 0;

  auto int part_map_probe (const grub_partition_map_t partmap);

  int part_map_probe (const grub_partition_map_t partmap)
    {
      part = partmap->probe (disk, str);
      if (part)
	return 1;

      if (grub_errno == GRUB_ERR_BAD_PART_TABLE)
	{
	  /* Continue to next partition map type.  */
	  grub_errno = GRUB_ERR_NONE;
	  return 0;
	}

      return 1;
    }

  /* Use the first partition map type found.  */
  grub_partition_map_iterate (part_map_probe);

  return part;
}

int
grub_partition_iterate (struct grub_disk *disk,
			int (*hook) (grub_disk_t disk,
				     const grub_partition_t partition))
{
  grub_partition_map_t partmap = 0;
  int ret = 0;
  
  auto int part_map_iterate (const grub_partition_map_t p);
  auto int part_map_iterate_hook (grub_disk_t d,
				  const grub_partition_t partition);

  int part_map_iterate_hook (grub_disk_t d __attribute__ ((unused)),
			     const grub_partition_t partition __attribute__ ((unused)))
    {
      return 1;
    }
  
  int part_map_iterate (const grub_partition_map_t p)
    {
      grub_err_t err = p->iterate (disk, part_map_iterate_hook);

      if (err != GRUB_ERR_NONE)
	{
	  /* Continue to next partition map type.  */
	  grub_errno = GRUB_ERR_NONE;
	  return 0;
	}

      partmap = p;
      return 1;
    }

  grub_partition_map_iterate (part_map_iterate);
  if (partmap)
    ret = partmap->iterate (disk, hook);
  
  return ret;
}

char *
grub_partition_get_name (const grub_partition_t partition)
{
  return partition->partmap->get_name (partition);
}
