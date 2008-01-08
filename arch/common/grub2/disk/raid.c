/* raid.c - module to read RAID arrays.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2006, 2007  Free Software Foundation, Inc.
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

#include <grub/dl.h>
#include <grub/disk.h>
#include <grub/mm.h>
#include <grub/err.h>
#include <grub/misc.h>
#include <grub/raid.h>

/* Linked list of RAID arrays. */
static struct grub_raid_array *array_list;


static char
grub_is_array_readable (struct grub_raid_array *array)
{
  switch (array->level)
    {
    case 0:
      if (array->nr_devs == array->total_devs)
	return 1;
      break;

    case 1:
      if (array->nr_devs >= 1)
	return 1;
      break;

    case 5:
      if (array->nr_devs >= array->total_devs - 1)
	return 1;
      break;
    }

  return 0;
}

static int
grub_raid_iterate (int (*hook) (const char *name))
{
  struct grub_raid_array *array;
  
  for (array = array_list; array != NULL; array = array->next)
    {
      if (grub_is_array_readable (array))
	if (hook (array->name))
	  return 1;
    }

  return 0;
}

static grub_err_t
grub_raid_open (const char *name, grub_disk_t disk)
{
  struct grub_raid_array *array;
  
  for (array = array_list; array != NULL; array = array->next)
    {
      if (!grub_strcmp (array->name, name))
	if (grub_is_array_readable (array))
	  break;
    }

  if (!array)
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "Unknown device");

  disk->has_partitions = 1;
  disk->id = array->number;
  disk->data = array;

  switch (array->level)
    {
    case 0:
      /* FIXME: RAID0 disks can have different sizes! */
      disk->total_sectors = array->total_devs * array->disk_size;
      break;

    case 1:
      disk->total_sectors = array->disk_size;
      break;

    case 5:
      disk->total_sectors = (array->total_devs - 1) * array->disk_size;
      break;
    }
  
  return 0;
}

static void
grub_raid_close (grub_disk_t disk __attribute ((unused)))
{
  return;
}

static grub_err_t
grub_raid_read (grub_disk_t disk, grub_disk_addr_t sector,
		grub_size_t size, char *buf)
{
  struct grub_raid_array *array = disk->data;
  grub_err_t err = 0;

  switch (array->level)
    {
    case 0:
      {
	grub_uint64_t a;
	grub_uint32_t b;
	unsigned int disknr;
	grub_disk_addr_t read_sector;
	grub_size_t read_size;

	/* Find the first sector to read. */
	a = grub_divmod64 (sector, array->chunk_size, NULL);
	grub_divmod64 (a, array->total_devs, &disknr);

	a = grub_divmod64 (sector, array->chunk_size * array->total_devs, NULL);
	grub_divmod64 (sector, array->chunk_size, &b);
	read_sector = a * array->chunk_size + b;

	grub_divmod64 (read_sector, array->chunk_size, &b);
	read_size = array->chunk_size - b;
	
	if (read_size > size)
	  read_size = size;
	
	while (1)
	  {
	    grub_uint32_t i;

	    err = grub_disk_read (array->device[disknr].disk, read_sector, 0,
				  read_size << GRUB_DISK_SECTOR_BITS, buf);
	    if (err)
	      break;

	    buf += read_size;
	    size -= read_size;
	    if (! size)
	      break;

	    if (size > array->chunk_size)
	      read_size = array->chunk_size;
	    else
	      read_size = size;

	    /* Check whether the sector was aligned on a chunk size
	       boundary. If this isn't the case, it's the first read
	       and the next read should be set back to start of the
	       boundary.  */
	    grub_divmod64 (read_sector, array->chunk_size, &i);
	    read_sector -= i;

	    disknr++;
	    /* See whether the disk was the last disk, and start
	       reading from the first disk in that case. */
	    if (disknr == array->total_devs)
	      {
		disknr = 0;
		read_sector += array->chunk_size;
	      }
	  }
      }
      break;

    case 1:
      /* This is easy, we can read from any disk we want. We will loop
	 over all disks until we've found one that is available. In
	 case of errs, we will try the to read the next disk. */
      {
	unsigned int i = 0;
	
	for (i = 0; i < array->total_devs; i++)
	  {
	    if (array->device[i].disk)
	      {
		err = grub_disk_read (array->device[i].disk, sector, 0,
				      size << GRUB_DISK_SECTOR_BITS, buf);

		if (!err)
		  break;
	      }
	  }
      }
      break;

    case 5:
      {
	grub_uint64_t a;
	grub_uint32_t b;
	int disknr;
	grub_disk_addr_t read_sector;
	grub_size_t read_size;

	/* Find the first sector to read. */
	a = grub_divmod64 (sector, array->chunk_size, NULL);
	grub_divmod64 (a, (array->total_devs - 1), &b);
	disknr = b;

	a = grub_divmod64 (sector, array->chunk_size * (array->total_devs - 1),
			   NULL);
	grub_divmod64 (sector, array->chunk_size, &b);
	read_sector = a * array->chunk_size + b;

	grub_divmod64 (read_sector, array->chunk_size * array->total_devs, &b);
	disknr -= (b / array->chunk_size);
	if (disknr < 0)
	  disknr += array->total_devs;
	  
	grub_divmod64 (read_sector, array->chunk_size, &b);
	read_size = array->chunk_size - b;

	if (read_size > size)
	  read_size = size;
	
	while (1)
	  {
	    grub_uint32_t i;

	    if (array->device[disknr].disk)
	      err = grub_disk_read (array->device[disknr].disk, read_sector, 0,
				    read_size << GRUB_DISK_SECTOR_BITS, buf);

	    /* If an error occurs when we already have an degraded
	       array we can't recover from that. */
	    if (err && ((array->total_devs - 1) == array->nr_devs))
	      break;
	    
	    if (err || ! array->device[disknr].disk)
	      {
		/* Either an error occured or the disk is not
		   available.  We have to compute this block from the
		   blocks on the other hard disks. */
		grub_size_t buf_size = read_size << GRUB_DISK_SECTOR_BITS;
		char buf2[buf_size];
		unsigned int j;

		grub_memset (buf, 0, buf_size);
		
		for (j = 0; j < array->total_devs; j++)
		  {
		    unsigned int k;

		    if (j != (unsigned int) disknr)
		      {
			err = grub_disk_read (array->device[j].disk, read_sector,
					      0, buf_size, buf2);
			if (err)
			  return err;
			
			for (k = 0; k < buf_size; k++)
			  buf[k] = buf[k] ^ buf2[k];
		      }
		  }
	      }

	    buf += (read_size << GRUB_DISK_SECTOR_BITS);
	    size -= read_size;
	    if (! size)
	      break;

	    if (size > array->chunk_size)
	      read_size = array->chunk_size;
	    else
	      read_size = size;

	    /* Check whether the sector was aligned on a chunk size
	       boundary. If this isn't the case, it's the first read
	       and the next read should be set back to start of the
	       boundary.  */
	    grub_divmod64 (read_sector, array->chunk_size, &i);
	    read_sector -= i;

	    disknr++;
	    grub_divmod64 (read_sector,
			   array->chunk_size * array->total_devs, &i);
	    if ((unsigned int) disknr == (array->total_devs - (i / array->chunk_size) - 1))
	      disknr++;
	    /* See whether the disk was the last disk, and start
	       reading from the first disk in that case. */
	    if ((unsigned int) disknr == array->total_devs)
	      {
		disknr = 0;
		read_sector += array->chunk_size;
		grub_divmod64 (read_sector,
			       array->chunk_size * array->total_devs, &i);

		if ((i / array->chunk_size) == (array->total_devs - 1))
		  disknr++;
	      }
	  }
      }
      break;
    }
  
  return err;
}

static grub_err_t
grub_raid_write (grub_disk_t disk __attribute ((unused)),
		 grub_disk_addr_t sector __attribute ((unused)),
		 grub_size_t size __attribute ((unused)),
		 const char *buf __attribute ((unused)))
{
  return GRUB_ERR_NOT_IMPLEMENTED_YET;
}

static int
grub_raid_scan_device (const char *name)
{
  grub_err_t err;
  grub_disk_t disk;
  grub_disk_addr_t sector;
  grub_uint64_t size;
  struct grub_raid_super_09 sb;
  struct grub_raid_array *p, *array = NULL;

  disk = grub_disk_open (name);
  if (!disk)
    return 0;

  /* The sector where the RAID superblock is stored, if available. */
  size = grub_disk_get_size (disk);
  sector = GRUB_RAID_NEW_SIZE_SECTORS(size);

  err = grub_disk_read (disk, sector, 0, GRUB_RAID_SB_BYTES, (char *) &sb);
  grub_disk_close (disk);
  if (err)
    return 0;

  /* Look whether there is a RAID superblock. */
  if (sb.md_magic != GRUB_RAID_SB_MAGIC)
    return 0;
  
  /* FIXME: Also support version 1.0. */
  if (sb.major_version != 0 || sb.minor_version != 90)
    {
      grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
		  "Unsupported RAID version: %d.%d",
		  sb.major_version, sb.minor_version);
      return 0;
    }

  /* FIXME: Check the checksum. */

  /* FIXME: Support all RAID levels.  */
  if (sb.level != 0 && sb.level != 1 && sb.level != 5)
    {
      grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
		  "Unsupported RAID level: %d",
		  sb.level);
      return 0;
    }

  /* FIXME: Support all layouts.  */
  if (sb.level == 5 && sb.layout != 2)
    {
      grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
		  "Unsupported RAID5 layout: %d",
		  sb.layout);
      return 0;
    }
  
  /* See whether the device is part of an array we have already seen a
     device from. */
  for (p = array_list; p != NULL; p = p->next)
    {
      if (p->uuid[0] == sb.set_uuid0 && p->uuid[1] == sb.set_uuid1
	  && p->uuid[2] == sb.set_uuid2 && p->uuid[3] == sb.set_uuid3)
	{
	  array = p;
	  break;
	}
    }

  /* Do some checks before adding the device to the array.  */
  if (array)
    {
      /* FIXME: Check whether the update time of the superblocks are
	 the same. */
      
      if (array->total_devs == array->nr_devs)
	{
	  /* We found more members of the array than the array
	     actually has according to its superblock.  This shouldn't
	     happen normally, but what is the sanest things to do in such
	     a case? */
	     
	  grub_error (GRUB_ERR_BAD_NUMBER,
		      "array->nr_devs > array->total_devs (%d)?!?",
		      array->total_devs);

	  return 0;
	}
  
      if (array->device[sb.this_disk.number].name != 0)
	{
	  /* We found multiple devices with the same number. Again,
	     this shouldn't happen.*/

	  grub_error (GRUB_ERR_BAD_NUMBER,
		      "Found two disks with the number %d?!?",
		      sb.this_disk.number);

	  return 0;
	}
    }

  /* Add an array to the list if we didn't find any.  */
  if (!array)
    {
      array = grub_malloc (sizeof (*array));
      if (!array)
	return 0;
      grub_memset (array, 0, sizeof (*array));      
      array->number = sb.md_minor;
      array->version = sb.major_version;
      array->level = sb.level;
      array->layout = sb.layout;
      array->total_devs = sb.nr_disks;
      array->nr_devs = 0;
      array->uuid[0] = sb.set_uuid0;
      array->uuid[1] = sb.set_uuid1;
      array->uuid[2] = sb.set_uuid2;
      array->uuid[3] = sb.set_uuid3;
      /* The superblock specifies the size in 1024-byte sectors. */
      array->disk_size = sb.size * 2;
      array->chunk_size = sb.chunk_size / 512;
      
      /* Check whether we don't have multiple arrays with the same number. */
      for (p = array_list; p != NULL; p = p->next)
	{
	  if (p->number == array->number)
	    break;
	}

      if (p)
	{
	  /* The number is already in use, so we need to find an new number. */
	  int i = 0;

	  while (1)
	    {
	      for (p = array_list; p != NULL; p = p->next)
		{
		  if (p->number == i)
		    break;
		}

	      if (!p)
		{
		  /* We found an unused number.  */
		  array->number = i;
		  break;
		}

	      i++;
	    }
	}

      array->name = grub_malloc (13);
      if (! array->name)
	{
	  grub_free (array);

	  return 0;
	}

      grub_sprintf (array->name, "md%d", array->number);

      /* Add our new array to the list.  */
      array->next = array_list;
      array_list = array;
    }

  /* Add the device to the array. */
  array->device[sb.this_disk.number].name = grub_strdup (name);
  array->device[sb.this_disk.number].disk = grub_disk_open (name);
  
  if (! array->device[sb.this_disk.number].name
      || ! array->device[sb.this_disk.number].disk)
    {
      grub_free (array->device[sb.this_disk.number].name);

      /* Remove array from the list if we have just added it. */
      if (array->nr_devs == 0)
	{
	  array_list = array->next;
	  grub_free (array->name);
	  grub_free (array);
	}
	  
      return 0;
    }

  array->nr_devs++;
  
  return 0;
}

static struct grub_disk_dev grub_raid_dev =
  {
    .name = "raid",
    .id = GRUB_DISK_DEVICE_RAID_ID,
    .iterate = grub_raid_iterate,
    .open = grub_raid_open,
    .close = grub_raid_close,
    .read = grub_raid_read,
    .write = grub_raid_write,
    .next = 0
  };


GRUB_MOD_INIT(raid)
{
  grub_device_iterate (&grub_raid_scan_device);
  grub_disk_dev_register (&grub_raid_dev);
}

GRUB_MOD_FINI(raid)
{
  grub_disk_dev_unregister (&grub_raid_dev);
  /* FIXME: free the array list. */
}
