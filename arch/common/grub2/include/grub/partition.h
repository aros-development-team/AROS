/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2004  Free Software Foundation, Inc.
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

#ifndef GRUB_PART_HEADER
#define GRUB_PART_HEADER	1

#include <grub/dl.h>

struct grub_disk;

typedef struct grub_partition *grub_partition_t;

/* Partition map type.  */
struct grub_partition_map
{
  /* The name of the partition map type.  */
  const char *name;
  
  /* Call HOOK with each partition, until HOOK returns non-zero.  */
  grub_err_t (*iterate) (struct grub_disk *disk,
			 int (*hook) (struct grub_disk *disk, const grub_partition_t partition));
  
  /* Return the partition named STR on the disk DISK.  */
  grub_partition_t (*probe) (struct grub_disk *disk,
			     const char *str);
  
  /* Return the name of the partition PARTITION.  */
  char *(*get_name) (const grub_partition_t partition);
  
  /* The next partition map type.  */
  struct grub_partition_map *next;
};
typedef struct grub_partition_map *grub_partition_map_t;

/* Partition description.  */
struct grub_partition
{
  /* The start sector.  */
  unsigned long start;

  /* The length in sector units.  */
  unsigned long len;

  /* The offset of the partition table.  */
  unsigned long offset;

  /* The index of this partition in the partition table.  */
  int index;
  
  /* Partition map type specific data.  */
  void *data;
  
  /* The type partition map.  */
  grub_partition_map_t partmap;
};

grub_partition_t EXPORT_FUNC(grub_partition_probe) (struct grub_disk *disk,
						    const char *str);
int EXPORT_FUNC(grub_partition_iterate) (struct grub_disk *disk,
					 int (*hook) (struct grub_disk *disk,
						      const grub_partition_t partition));
char *EXPORT_FUNC(grub_partition_get_name) (const grub_partition_t partition);

int EXPORT_FUNC(grub_partition_map_iterate) (int (*hook) (const grub_partition_map_t partmap));
					      
void EXPORT_FUNC(grub_partition_map_register) (grub_partition_map_t partmap);

void EXPORT_FUNC(grub_partition_map_unregister) (grub_partition_map_t partmap);

#ifdef GRUB_UTIL
void grub_pc_partition_map_init (void);
void grub_pc_partition_map_fini (void);
void grub_amiga_partition_map_init (void);
void grub_amiga_partition_map_fini (void);
void grub_apple_partition_map_init (void);
void grub_apple_partition_map_fini (void);
void grub_sun_partition_map_init (void);
void grub_sun_partition_map_fini (void);
#endif

static inline unsigned long
grub_partition_get_start (const grub_partition_t p)
{
  return p->start;
}

static inline unsigned long
grub_partition_get_len (const grub_partition_t p)
{
  return p->len;
}

#endif /* ! GRUB_PART_HEADER */
