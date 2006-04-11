/* apple.c - Read macintosh partition tables.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002, 2004  Free Software Foundation, Inc.
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

#include <grub/disk.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/partition.h>

#define GRUB_APPLE_PART_MAGIC	0x504D

struct grub_apple_part
{
  /* The magic number to idenify this as a partition, it should have
     the value `0x504D'.  */
  grub_uint16_t magic;

  /* Reserved.  */
  grub_uint16_t reserved;

  /* The size of the partition map in blocks.  */
  grub_uint32_t partmap_size;

  /* The first physical block of the partition.  */
  grub_uint32_t first_phys_block;

  /* The amount of blocks.  */
  grub_uint32_t blockcnt;

  /* The partition name.  */
  char partname[32];

  /* The partition type.  */
  char parttype[32];

  /* The first datablock of the partition.  */
  grub_uint32_t datablocks_first;

  /* The amount datablocks.  */
  grub_uint32_t datablocks_count;

  /* The status of the partition. (???)  */
  grub_uint32_t status;

  /* The first block on which the bootcode can be found.  */
  grub_uint32_t bootcode_pos;

  /* The size of the bootcode in bytes.  */
  grub_uint32_t bootcode_size;

  /* The load address of the bootcode.  */
  grub_uint32_t bootcode_loadaddr;

  /* Reserved.  */
  grub_uint32_t reserved2;
  
  /* The entrypoint of the bootcode.  */
  grub_uint32_t bootcode_entrypoint;

  /* Reserved.  */
  grub_uint32_t reserved3;

  /* A checksum of the bootcode.  */
  grub_uint32_t bootcode_checksum;

  /* The processor type.  */
  char processor[16];

  /* Padding.  */
  grub_uint16_t pad[187];
};

static struct grub_partition_map grub_apple_partition_map;

#ifndef GRUB_UTIL
static grub_dl_t my_mod;
#endif


static grub_err_t
apple_partition_map_iterate (grub_disk_t disk,
			     int (*hook) (grub_disk_t disk,
					  const grub_partition_t partition))
{
  struct grub_partition part;
  struct grub_apple_part apart;
  struct grub_disk raw;
  int partno = 0;
  int pos = GRUB_DISK_SECTOR_SIZE;

  /* Enforce raw disk access.  */
  raw = *disk;
  raw.partition = 0;

  part.partmap = &grub_apple_partition_map;

  for (;;)
    {
      if (grub_disk_read (&raw, pos / GRUB_DISK_SECTOR_SIZE,
		      pos % GRUB_DISK_SECTOR_SIZE,
			  sizeof (struct grub_apple_part),  (char *) &apart))
	return grub_errno;

      if (apart.magic != GRUB_APPLE_PART_MAGIC)
	{
	  grub_dprintf ("partition",
			"partition %d: bad magic (found 0x%x; wanted 0x%x\n",
			partno, apart.magic, GRUB_APPLE_PART_MAGIC);
	  break;
	}

      part.start = apart.first_phys_block;
      part.len = apart.blockcnt;
      part.offset = pos;
      part.index = partno;

      grub_dprintf ("partition",
		    "partition %d: name %s, type %s, start 0x%x, len 0x%x\n",
		    partno, apart.partname, apart.parttype,
		    apart.first_phys_block, apart.blockcnt);

      if (hook (disk, &part))
	return grub_errno;

      if (apart.first_phys_block == GRUB_DISK_SECTOR_SIZE * 2)
	return 0;

      pos += sizeof (struct grub_apple_part);
      partno++;
    }

  if (pos == GRUB_DISK_SECTOR_SIZE)
    return grub_error (GRUB_ERR_BAD_PART_TABLE,
		       "Apple partition map not found.");

  return 0;
}


static grub_partition_t
apple_partition_map_probe (grub_disk_t disk, const char *str)
{
  grub_partition_t p = 0;
  int partnum = 0;
  char *s = (char *) str;

  auto int find_func (grub_disk_t d, const grub_partition_t partition);
  
  int find_func (grub_disk_t d __attribute__ ((unused)),
		 const grub_partition_t partition)
    {
      if (partnum == partition->index)
	{
	  p = (grub_partition_t) grub_malloc (sizeof (*p));
	  if (! p)
	    return 1;
	  
	  grub_memcpy (p, partition, sizeof (*p));
	  return 1;
	}
      
      return 0;
    }
  
  /* Get the partition number.  */
  partnum = grub_strtoul (s, 0, 10);
  if (grub_errno)
    {
      grub_error (GRUB_ERR_BAD_FILENAME, "invalid partition");
      return 0;
    }
  
  if (apple_partition_map_iterate (disk, find_func))
    goto fail;

  return p;

 fail:
  grub_free (p);
  return 0;
}


static char *
apple_partition_map_get_name (const grub_partition_t p)
{
  char *name;

  name = grub_malloc (13);
  if (! name)
    return 0;

  grub_sprintf (name, "%d", p->index);
  return name;
}


/* Partition map type.  */
static struct grub_partition_map grub_apple_partition_map =
  {
    .name = "apple_partition_map",
    .iterate = apple_partition_map_iterate,
    .probe = apple_partition_map_probe,
    .get_name = apple_partition_map_get_name
  };

#ifdef GRUB_UTIL
void
grub_apple_partition_map_init (void)
{
  grub_partition_map_register (&grub_apple_partition_map);
}

void
grub_apple_partition_map_fini (void)
{
  grub_partition_map_unregister (&grub_apple_partition_map);
}
#else
GRUB_MOD_INIT
{
  grub_partition_map_register (&grub_apple_partition_map);
  my_mod = mod;
}

GRUB_MOD_FINI
{
  grub_partition_map_unregister (&grub_apple_partition_map);
}
#endif
