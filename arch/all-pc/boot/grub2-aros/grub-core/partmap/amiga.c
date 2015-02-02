/* amiga.c - Read amiga partition tables (RDB).  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2004,2005,2006,2007  Free Software Foundation, Inc.
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

#include <grub/disk.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/partition.h>
#include <grub/dl.h>

GRUB_MOD_LICENSE ("GPLv3+");

#define AMIGA_CHECKSUM_WORDS 128

struct grub_amiga_rdsk
{
  /* "RDSK".  */
  grub_uint8_t magic[4];
#define GRUB_AMIGA_RDSK_MAGIC "RDSK"
  grub_uint32_t size;
  grub_int32_t checksum;
  grub_uint32_t scsihost;
  grub_uint32_t blksz;
  grub_uint32_t flags;
  grub_uint32_t badblcklst;
  grub_uint32_t partitionlst;
  grub_uint32_t fslst;

  grub_uint32_t unused[AMIGA_CHECKSUM_WORDS - 9];
} GRUB_PACKED;

struct grub_amiga_partition
{
  /* "PART".  */
  grub_uint8_t magic[4];
#define GRUB_AMIGA_PART_MAGIC "PART"
  grub_uint32_t size;
  grub_int32_t checksum;
  grub_uint32_t scsihost;
  grub_uint32_t next;
  grub_uint32_t flags;
  grub_uint32_t unused1[2];
  grub_uint32_t devflags;
  grub_uint8_t namelen;
  grub_uint8_t name[31];
  grub_uint32_t unused2[15];

  grub_uint32_t unused3[3];
  grub_uint32_t heads;
  grub_uint32_t unused4;
  grub_uint32_t block_per_track;
  grub_uint32_t unused5[3];
  grub_uint32_t lowcyl;
  grub_uint32_t highcyl;

  grub_uint32_t firstcyl;
  grub_uint32_t unused[AMIGA_CHECKSUM_WORDS - 44];
} GRUB_PACKED;

static struct grub_partition_map grub_amiga_partition_map;



static grub_uint32_t
amiga_partition_map_checksum (void *buf)
{
  grub_uint32_t *ptr = buf;
  grub_uint32_t r = 0;
  grub_size_t sz;
  /* Fancy and quick way of checking sz >= 512 / 4 = 128.  */
  if (ptr[1] & ~grub_cpu_to_be32_compile_time (AMIGA_CHECKSUM_WORDS - 1))
    sz = AMIGA_CHECKSUM_WORDS;
  else
    sz = grub_be_to_cpu32 (ptr[1]);

  for (; sz; sz--, ptr++)
    r += grub_be_to_cpu32 (*ptr);

  return r;
}

static grub_err_t
amiga_partition_map_iterate (grub_disk_t disk,
			     grub_partition_iterate_hook_t hook,
			     void *hook_data)
{
  struct grub_partition part;
  struct grub_amiga_rdsk rdsk;
  int partno = 0;
  int next = -1;
  unsigned pos;

  /* The RDSK block is one of the first 15 blocks.  */
  for (pos = 0; pos < 15; pos++)
    {
      /* Read the RDSK block which is a descriptor for the entire disk.  */
      if (grub_disk_read (disk, pos, 0, sizeof (rdsk), &rdsk))
	return grub_errno;

      if (grub_memcmp (rdsk.magic, GRUB_AMIGA_RDSK_MAGIC,
		       sizeof (rdsk.magic)) == 0
	  && amiga_partition_map_checksum (&rdsk) == 0)
	{
	  /* Found the first PART block.  */
	  next = grub_be_to_cpu32 (rdsk.partitionlst);
	  break;
	}
    }

  if (next == -1)
    return grub_error (GRUB_ERR_BAD_PART_TABLE,
		       "Amiga partition map not found");

  /* The end of the partition list is marked using "-1".  */
  while (next != -1)
    {
      struct grub_amiga_partition apart;

      /* Read the RDSK block which is a descriptor for the entire disk.  */
      if (grub_disk_read (disk, next, 0, sizeof (apart), &apart))
	return grub_errno;

      if (grub_memcmp (apart.magic, GRUB_AMIGA_PART_MAGIC,
		       sizeof (apart.magic)) != 0
	  || amiga_partition_map_checksum (&apart) != 0)
	return grub_error (GRUB_ERR_BAD_PART_TABLE,
			   "invalid Amiga partition map");
      /* Calculate the first block and the size of the partition.  */
      part.start = (grub_be_to_cpu32 (apart.lowcyl)
		    * grub_be_to_cpu32 (apart.heads)
		    * grub_be_to_cpu32 (apart.block_per_track));
      part.len = ((grub_be_to_cpu32 (apart.highcyl)
		   - grub_be_to_cpu32 (apart.lowcyl) + 1)
		  * grub_be_to_cpu32 (apart.heads)
		  * grub_be_to_cpu32 (apart.block_per_track));

      part.offset = next;
      part.number = partno;
      part.index = 0;
      part.partmap = &grub_amiga_partition_map;

      if (hook (disk, &part, hook_data))
	return grub_errno;

      next = grub_be_to_cpu32 (apart.next);
      partno++;
    }

  return 0;
}


/* Partition map type.  */
static struct grub_partition_map grub_amiga_partition_map =
  {
    .name = "amiga",
    .iterate = amiga_partition_map_iterate,
  };

GRUB_MOD_INIT(part_amiga)
{
  grub_partition_map_register (&grub_amiga_partition_map);
}

GRUB_MOD_FINI(part_amiga)
{
  grub_partition_map_unregister (&grub_amiga_partition_map);
}
