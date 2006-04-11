/* sun.c - Read SUN style partition tables.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002, 2005 Free Software Foundation, Inc.
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
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/dl.h>
#include <grub/symbol.h>
#include <grub/types.h>
#include <grub/err.h>

#define GRUB_PARTMAP_SUN_MAGIC 0xDABE
#define GRUB_PARTMAP_SUN_MAX_PARTS 8
#define GRUB_PARTMAP_SUN_WHOLE_DISK_ID 0x05

struct grub_sun_partition_info
{
  grub_uint8_t spare1;
  grub_uint8_t id;
  grub_uint8_t spare2;
  grub_uint8_t flags;
} __attribute__ ((packed));

struct grub_sun_partition_descriptor
{
  grub_uint32_t start_cylinder;
  grub_uint32_t num_sectors;
} __attribute__ ((packed));

struct grub_sun_block
{
  grub_uint8_t  info[128];      /* Informative text string.  */
  grub_uint8_t  spare0[14];
  struct grub_sun_partition_info infos[8];
  grub_uint8_t  spare1[246];    /* Boot information etc.  */
  grub_uint16_t  rspeed;        /* Disk rotational speed.  */
  grub_uint16_t  pcylcount;     /* Physical cylinder count.  */
  grub_uint16_t  sparecyl;      /* extra sects per cylinder.  */
  grub_uint8_t  spare2[4];      /* More magic...  */
  grub_uint16_t  ilfact;        /* Interleave factor.  */
  grub_uint16_t  ncyl;          /* Data cylinder count.  */
  grub_uint16_t  nacyl;         /* Alt. cylinder count.  */
  grub_uint16_t  ntrks;         /* Tracks per cylinder.  */
  grub_uint16_t  nsect;         /* Sectors per track.  */
  grub_uint8_t  spare3[4];      /* Even more magic...  */
  struct grub_sun_partition_descriptor partitions[8];
  grub_uint16_t  magic;         /* Magic number.  */
  grub_uint16_t  csum;          /* Label xor'd checksum.  */
} __attribute__ ((packed));

static struct grub_partition_map grub_sun_partition_map;

#ifndef GRUB_UTIL
static grub_dl_t my_mod;
#endif

/* Verify checksum (true=ok).  */
static int
grub_sun_is_valid (struct grub_sun_block *label)
{
  grub_uint16_t *pos;
  grub_uint16_t sum = 0;
  for (pos = (grub_uint16_t *) label; pos < (grub_uint16_t *) (label + 1); pos++)
    sum ^= *pos;
  return !sum;
}

static grub_err_t
sun_partition_map_iterate (grub_disk_t disk,
                           int (*hook) (grub_disk_t disk,
					const grub_partition_t partition))
{
  struct grub_partition *p;
  struct grub_disk raw;
  struct grub_sun_block block;
  int partnum;
  raw = *disk;
  raw.partition = 0;
  p = (struct grub_partition *) grub_malloc (sizeof (struct grub_partition));
  if (!p)
    return grub_errno;

  p->offset = 0;
  p->data = 0;
  p->partmap = &grub_sun_partition_map;
  if (grub_disk_read (&raw, 0, 0, sizeof (struct grub_sun_block),
		      (char *) &block) == GRUB_ERR_NONE)
    {
      if (GRUB_PARTMAP_SUN_MAGIC != grub_be_to_cpu16 (block.magic))
	grub_error (GRUB_ERR_BAD_PART_TABLE, "not a sun partiton table");
      if (!grub_sun_is_valid (&block))
	grub_error (GRUB_ERR_BAD_PART_TABLE, "invalid checksum");
      /* Maybe another error value would be better, because partition
	 table _is_ recognised but invalid.  */
      for (partnum = 0; partnum < GRUB_PARTMAP_SUN_MAX_PARTS; partnum++)
	{
	  if (block.infos[partnum].id == 0 ||
	      block.infos[partnum].id == GRUB_PARTMAP_SUN_WHOLE_DISK_ID)
	    continue;
	  p->start = grub_be_to_cpu32
	    (block.partitions[partnum].start_cylinder)
	    * grub_be_to_cpu16 (block.ntrks)
	    * grub_be_to_cpu16 (block.nsect);
	  p->len = grub_be_to_cpu32 (block.partitions[partnum].num_sectors);
	  p->index = partnum;
	  if (p->len)
	    {
	      if (hook (disk, p))
		partnum = GRUB_PARTMAP_SUN_MAX_PARTS;
	    }
	}
    }
  grub_free (p);

  return grub_errno;
}

static grub_partition_t
sun_partition_map_probe (grub_disk_t disk, const char *str)
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
          if (p)
            grub_memcpy (p, partition, sizeof (*p));
          return 1;
        }
      return 0;
    }

  grub_errno = GRUB_ERR_NONE;
  partnum = grub_strtoul (s, 0, 10);
  if (grub_errno == GRUB_ERR_NONE)
    {
      if (sun_partition_map_iterate (disk, find_func))
        {
          grub_free (p);
          p = 0;
        }
    }
  else
    {
      grub_error (GRUB_ERR_BAD_FILENAME, "invalid partition");
      p = 0;
    }
  return p;
}

static char *
sun_partition_map_get_name (const grub_partition_t p)
{
  char *name;
  name = grub_malloc (13);
  if (name)
    grub_sprintf (name, "%d", p->index);
  return name;
}

/* Partition map type.  */
static struct grub_partition_map grub_sun_partition_map =
  {
    .name = "sun_partition_map",
    .iterate = sun_partition_map_iterate,
    .probe = sun_partition_map_probe,
    .get_name = sun_partition_map_get_name
  };

#ifdef GRUB_UTIL
void
grub_sun_partition_map_init (void)
{
  grub_partition_map_register (&grub_sun_partition_map);
}

void
grub_sun_partition_map_fini (void)
{
  grub_partition_map_unregister (&grub_sun_partition_map);
}
#else
GRUB_MOD_INIT
{
  grub_partition_map_register (&grub_sun_partition_map);
  my_mod = mod;
}

GRUB_MOD_FINI
{
  grub_partition_map_unregister (&grub_sun_partition_map);
}
#endif
