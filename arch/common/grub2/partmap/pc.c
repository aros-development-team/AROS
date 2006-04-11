/* pc.c - Read PC style partition tables.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2004,2005 Free Software Foundation, Inc.
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
#include <grub/pc_partition.h>
#include <grub/disk.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/dl.h>

static struct grub_partition_map grub_pc_partition_map;

#ifndef GRUB_UTIL
static grub_dl_t my_mod;
#endif


/* Parse the partition representation in STR and return a partition.  */
static grub_partition_t
grub_partition_parse (const char *str)
{
  grub_partition_t p;
  struct grub_pc_partition *pcdata;
  
  char *s = (char *) str;
  
  p = (grub_partition_t) grub_malloc (sizeof (*p));
  if (! p)
    return 0;
  
  pcdata = (struct grub_pc_partition *) grub_malloc (sizeof (*pcdata));
  if (! pcdata)
    goto fail;
  
  p->data = pcdata;
  p->partmap = &grub_pc_partition_map;
  
  /* Initialize some of the fields with invalid values.  */
  pcdata->bsd_part = pcdata->dos_type = pcdata->bsd_type = p->index = -1;

  /* Get the DOS partition number.  */
  pcdata->dos_part = grub_strtoul (s, &s, 0);
  
  if (grub_errno)
    {
      /* Not found. Maybe only a BSD label is specified.  */
      pcdata->dos_part = -1;
      grub_errno = GRUB_ERR_NONE;
    }
  else if (*s == ',')
    s++;

  if (*s)
    {
      if (*s >= 'a' && *s <= 'h')
	{
	  pcdata->bsd_part = *s - 'a';
	  s++;
	}

      if (*s)
	goto fail;
    }

  if (pcdata->dos_part == -1 && pcdata->bsd_part == -1)
    goto fail;

  return p;
  
 fail:
  grub_free (p);
  grub_free (pcdata);
  grub_error (GRUB_ERR_BAD_FILENAME, "invalid partition");
  return 0;
}

static grub_err_t
pc_partition_map_iterate (grub_disk_t disk,
			  int (*hook) (grub_disk_t disk,
				       const grub_partition_t partition))
{
  struct grub_partition p;
  struct grub_pc_partition pcdata;
  struct grub_pc_partition_mbr mbr;
  struct grub_pc_partition_disk_label label;
  struct grub_disk raw;

  /* Enforce raw disk access.  */
  raw = *disk;
  raw.partition = 0;
  
  p.offset = 0;
  pcdata.ext_offset = 0;
  pcdata.dos_part = -1;
  p.data = &pcdata;
  p.partmap = &grub_pc_partition_map;
  
  while (1)
    {
      int i;
      struct grub_pc_partition_entry *e;
      
      /* Read the MBR.  */
      if (grub_disk_read (&raw, p.offset, 0, sizeof (mbr), (char *) &mbr))
	goto finish;

      /* Check if it is valid.  */
      if (mbr.signature != grub_cpu_to_le16 (GRUB_PC_PARTITION_SIGNATURE))
	return grub_error (GRUB_ERR_BAD_PART_TABLE, "no signature");

      /* Analyze DOS partitions.  */
      for (p.index = 0; p.index < 4; p.index++)
	{
	  e = mbr.entries + p.index;
	  
	  p.start = p.offset + grub_le_to_cpu32 (e->start);
	  p.len = grub_le_to_cpu32 (e->length);
	  pcdata.bsd_part = -1;
	  pcdata.dos_type = e->type;
	  pcdata.bsd_type = -1;

	  grub_dprintf ("partition",
			"partition %d: flag 0x%x, type 0x%x, start 0x%lx, len 0x%lx\n",
			p.index, e->flag, pcdata.dos_type, p.start, p.len);

	  /* If this partition is a normal one, call the hook.  */
	  if (! grub_pc_partition_is_empty (e->type)
	      && ! grub_pc_partition_is_extended (e->type))
	    {
	      pcdata.dos_part++;
	      
	      if (hook (disk, &p))
		goto finish;

	      /* Check if this is a BSD partition.  */
	      if (grub_pc_partition_is_bsd (e->type))
		{
		  /* Check if the BSD label is within the DOS partition.  */
		  if (p.len <= GRUB_PC_PARTITION_BSD_LABEL_SECTOR)
		    return grub_error (GRUB_ERR_BAD_PART_TABLE,
				       "no space for disk label");

		  /* Read the BSD label.  */
		  if (grub_disk_read (&raw,
				      (p.start
				       + GRUB_PC_PARTITION_BSD_LABEL_SECTOR),
				      0,
				      sizeof (label),
				      (char *) &label))
		    goto finish;

		  /* Check if it is valid.  */
		  if (label.magic
		      != grub_cpu_to_le32 (GRUB_PC_PARTITION_BSD_LABEL_MAGIC))
		    return grub_error (GRUB_ERR_BAD_PART_TABLE,
				       "invalid disk label magic 0x%x",
				       label.magic);

		  for (pcdata.bsd_part = 0;
		       pcdata.bsd_part < grub_cpu_to_le16 (label.num_partitions);
		       pcdata.bsd_part++)
		    {
		      struct grub_pc_partition_bsd_entry *be
			= label.entries + pcdata.bsd_part;

		      p.start = grub_le_to_cpu32 (be->offset);
		      p.len = grub_le_to_cpu32 (be->size);
		      pcdata.bsd_type = be->fs_type;
		      
		      if (be->fs_type != GRUB_PC_PARTITION_BSD_TYPE_UNUSED)
			if (hook (disk, &p))
			  goto finish;
		    }
		}
	    }
	  else if (pcdata.dos_part < 4)
	    /* If this partition is a logical one, shouldn't increase the
	       partition number.  */
	    pcdata.dos_part++;
	}

      /* Find an extended partition.  */
      for (i = 0; i < 4; i++)
	{
	  e = mbr.entries + i;
	  
	  if (grub_pc_partition_is_extended (e->type))
	    {
	      p.offset = pcdata.ext_offset + grub_le_to_cpu32 (e->start);
	      if (! pcdata.ext_offset)
		pcdata.ext_offset = p.offset;

	      break;
	    }
	}

      /* If no extended partition, the end.  */
      if (i == 4)
	break;
    }

 finish:
  return grub_errno;
}


static grub_partition_t
pc_partition_map_probe (grub_disk_t disk, const char *str)
{
  grub_partition_t p;
  struct grub_pc_partition *pcdata;
  
  auto int find_func (grub_disk_t d, const grub_partition_t partition);

  int find_func (grub_disk_t d __attribute__ ((unused)),
		 const grub_partition_t partition)
    {
      struct grub_pc_partition *partdata = partition->data;
      if ((pcdata->dos_part == partdata->dos_part || pcdata->dos_part == -1)
	  && pcdata->bsd_part == partdata->bsd_part)
	{
	  grub_memcpy (p, partition, sizeof (*p));
	  p->data = pcdata;
	  grub_memcpy (pcdata, partdata, sizeof (*pcdata));
	  return 1;
	}
      
      return 0;
    }
  
  p = grub_partition_parse (str);
  if (! p)
    return 0;
  
  pcdata = p->data;
  if (pc_partition_map_iterate (disk, find_func))
    goto fail;

  if (p->index < 0)
    {
      grub_error (GRUB_ERR_BAD_DEVICE, "no such partition");
      goto fail;
    }

  return p;

 fail:
  grub_free (p);
  grub_free (pcdata);
  return 0;
}


static char *
pc_partition_map_get_name (const grub_partition_t p)
{
  char *name;
  struct grub_pc_partition *pcdata = p->data;
  
  name = grub_malloc (13);
  if (! name)
    return 0;

  if (pcdata->bsd_part < 0)
    grub_sprintf (name, "%d", pcdata->dos_part);
  else
    grub_sprintf (name, "%d,%c", pcdata->dos_part, pcdata->bsd_part + 'a');

  return name;
}


/* Partition map type.  */
static struct grub_partition_map grub_pc_partition_map =
  {
    .name = "pc_partition_map",
    .iterate = pc_partition_map_iterate,
    .probe = pc_partition_map_probe,
    .get_name = pc_partition_map_get_name
  };

#ifdef GRUB_UTIL
void
grub_pc_partition_map_init (void)
{
  grub_partition_map_register (&grub_pc_partition_map);
}

void
grub_pc_partition_map_fini (void)
{
  grub_partition_map_unregister (&grub_pc_partition_map);
}
#else
GRUB_MOD_INIT
{
  grub_partition_map_register (&grub_pc_partition_map);
  my_mod = mod;
}

GRUB_MOD_FINI
{
  grub_partition_map_unregister (&grub_pc_partition_map);
}
#endif
