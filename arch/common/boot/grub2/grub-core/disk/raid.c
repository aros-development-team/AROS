/* raid.c - module to read RAID arrays.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2006,2007,2008,2009,2010  Free Software Foundation, Inc.
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
grub_raid5_recover_func_t grub_raid5_recover_func;
grub_raid6_recover_func_t grub_raid6_recover_func;


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

    case 4:
    case 5:
    case 6:
    case 10:
      {
        unsigned int n;

        if (array->level == 10)
          {
            n = array->layout & 0xFF;
            if (n == 1)
              n = (array->layout >> 8) & 0xFF;

            n--;
          }
        else
          n = array->level / 3;

        if (array->nr_devs >= array->total_devs - n)
          return 1;

        break;
      }
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

#ifdef GRUB_UTIL
static grub_disk_memberlist_t
grub_raid_memberlist (grub_disk_t disk)
{
  struct grub_raid_array *array = disk->data;
  grub_disk_memberlist_t list = NULL, tmp;
  unsigned int i;

  for (i = 0; i < array->total_devs; i++)
    if (array->members[i].device)
      {
        tmp = grub_malloc (sizeof (*tmp));
        tmp->disk = array->members[i].device;
        tmp->next = list;
        list = tmp;
      }

  return list;
}

static const char *
grub_raid_getname (struct grub_disk *disk)
{
  struct grub_raid_array *array = disk->data;

  return array->driver->name;
}
#endif

static grub_err_t
grub_raid_open (const char *name, grub_disk_t disk)
{
  struct grub_raid_array *array;
  unsigned n;

  for (array = array_list; array != NULL; array = array->next)
    {
      if (!grub_strcmp (array->name, name))
	if (grub_is_array_readable (array))
	  break;
    }

  if (!array)
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "unknown RAID device %s",
                       name);

  disk->id = array->number;
  disk->data = array;

  grub_dprintf ("raid", "%s: total_devs=%d, disk_size=%lld\n", name,
		array->total_devs, (unsigned long long) array->disk_size);

  switch (array->level)
    {
    case 1:
      disk->total_sectors = array->disk_size;
      break;

    case 10:
      n = array->layout & 0xFF;
      if (n == 1)
        n = (array->layout >> 8) & 0xFF;

      disk->total_sectors = grub_divmod64 (array->total_devs *
                                           array->disk_size,
                                           n, 0);
      break;

    case 0:
    case 4:
    case 5:
    case 6:
      n = array->level / 3;

      disk->total_sectors = (array->total_devs - n) * array->disk_size;
      break;
    }

  grub_dprintf ("raid", "%s: level=%d, total_sectors=%lld\n", name,
		array->level, (unsigned long long) disk->total_sectors);

  return 0;
}

static void
grub_raid_close (grub_disk_t disk __attribute ((unused)))
{
  return;
}

void
grub_raid_block_xor (char *buf1, const char *buf2, int size)
{
  grub_size_t *p1;
  const grub_size_t *p2;

  p1 = (grub_size_t *) buf1;
  p2 = (const grub_size_t *) buf2;
  size /= GRUB_CPU_SIZEOF_VOID_P;

  while (size)
    {
      *(p1++) ^= *(p2++);
      size--;
    }
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
    case 1:
    case 10:
      {
        grub_disk_addr_t read_sector, far_ofs;
	grub_uint32_t disknr, b, near, far, ofs;

        read_sector = grub_divmod64 (sector, array->chunk_size, &b);
        far = ofs = near = 1;
        far_ofs = 0;

        if (array->level == 1)
          near = array->total_devs;
        else if (array->level == 10)
          {
            near = array->layout & 0xFF;
            far = (array->layout >> 8) & 0xFF;
            if (array->layout >> 16)
              {
                ofs = far;
                far_ofs = 1;
              }
            else
              far_ofs = grub_divmod64 (array->disk_size,
                                       far * array->chunk_size, 0);

            far_ofs *= array->chunk_size;
          }

        read_sector = grub_divmod64 (read_sector * near, array->total_devs,
                                     &disknr);

        ofs *= array->chunk_size;
        read_sector *= ofs;

        while (1)
          {
            grub_size_t read_size;
            unsigned int i, j;

            read_size = array->chunk_size - b;
            if (read_size > size)
              read_size = size;

            for (i = 0; i < near; i++)
              {
                unsigned int k;

                k = disknr;
                for (j = 0; j < far; j++)
                  {
                    if (array->members[k].device)
                      {
                        if (grub_errno == GRUB_ERR_READ_ERROR)
                          grub_errno = GRUB_ERR_NONE;

                        err = grub_disk_read (array->members[k].device,
                                              array->members[k].start_sector +
                                                read_sector + j * far_ofs + b,
                                              0,
                                              read_size << GRUB_DISK_SECTOR_BITS,
                                              buf);
                        if (! err)
                          break;
                        else if (err != GRUB_ERR_READ_ERROR)
                          return err;
                      }
                    else
                      err = grub_error (GRUB_ERR_READ_ERROR,
                                        "disk missing");

                    k++;
                    if (k == array->total_devs)
                      k = 0;
                  }

                if (! err)
                  break;

                disknr++;
                if (disknr == array->total_devs)
                  {
                    disknr = 0;
                    read_sector += ofs;
                  }
              }

            if (err)
              return err;

            buf += read_size << GRUB_DISK_SECTOR_BITS;
	    size -= read_size;
	    if (! size)
	      break;

            b = 0;
            disknr += (near - i);
            while (disknr >= array->total_devs)
              {
                disknr -= array->total_devs;
                read_sector += ofs;
              }
          }
        break;
      }

    case 4:
    case 5:
    case 6:
      {
	grub_disk_addr_t read_sector;
	grub_uint32_t b, p, n, disknr, e;

        /* n = 1 for level 4 and 5, 2 for level 6.  */
        n = array->level / 3;

	/* Find the first sector to read. */
	read_sector = grub_divmod64 (sector, array->chunk_size, &b);
	read_sector = grub_divmod64 (read_sector, array->total_devs - n,
                                     &disknr);
        if (array->level >= 5)
          {
            grub_divmod64 (read_sector, array->total_devs, &p);

            if (! (array->layout & GRUB_RAID_LAYOUT_RIGHT_MASK))
              p = array->total_devs - 1 - p;

            if (array->layout & GRUB_RAID_LAYOUT_SYMMETRIC_MASK)
              {
                disknr += p + n;
              }
            else
              {
                grub_uint32_t q;

                q = p + (n - 1);
                if (q >= array->total_devs)
                  q -= array->total_devs;

                if (disknr >= p)
                  disknr += n;
                else if (disknr >= q)
                  disknr += q + 1;
              }

            if (disknr >= array->total_devs)
              disknr -= array->total_devs;
          }
        else
          p = array->total_devs - n;

	read_sector *= array->chunk_size;

	while (1)
	  {
            grub_size_t read_size;
            int next_level;

            read_size = array->chunk_size - b;
            if (read_size > size)
              read_size = size;

            e = 0;
            if (array->members[disknr].device)
              {
                /* Reset read error.  */
                if (grub_errno == GRUB_ERR_READ_ERROR)
                  grub_errno = GRUB_ERR_NONE;

                err = grub_disk_read (array->members[disknr].device,
                                      array->members[disknr].start_sector +
                                        read_sector + b, 0,
                                      read_size << GRUB_DISK_SECTOR_BITS,
                                      buf);

                if ((err) && (err != GRUB_ERR_READ_ERROR))
                  break;
                e++;
              }
            else
              err = GRUB_ERR_READ_ERROR;

	    if (err)
              {
                if (array->nr_devs < array->total_devs - n + e)
                  break;

                grub_errno = GRUB_ERR_NONE;
                if (array->level == 6)
                  {
                    err = ((grub_raid6_recover_func) ?
                           (*grub_raid6_recover_func) (array, disknr, p,
                                                       buf, read_sector + b,
                                                       read_size) :
                           grub_error (GRUB_ERR_BAD_DEVICE,
                                       "raid6rec is not loaded"));
                  }
                else
                  {
                    err = ((grub_raid5_recover_func) ?
                           (*grub_raid5_recover_func) (array, disknr,
                                                       buf, read_sector + b,
                                                       read_size) :
                           grub_error (GRUB_ERR_BAD_DEVICE,
                                       "raid5rec is not loaded"));
                  }

                if (err)
                  break;
              }

	    buf += read_size << GRUB_DISK_SECTOR_BITS;
	    size -= read_size;
	    if (! size)
	      break;

            b = 0;
	    disknr++;

            if (array->layout & GRUB_RAID_LAYOUT_SYMMETRIC_MASK)
              {
                if (disknr == array->total_devs)
                  disknr = 0;

                next_level = (disknr == p);
              }
            else
              {
                if (disknr == p)
                  disknr += n;

                next_level = (disknr >= array->total_devs);
              }

            if (next_level)
              {
                read_sector += array->chunk_size;

                if (array->level >= 5)
                  {
                    if (array->layout & GRUB_RAID_LAYOUT_RIGHT_MASK)
                      p = (p == array->total_devs - 1) ? 0 : p + 1;
                    else
                      p = (p == 0) ? array->total_devs - 1 : p - 1;

                    if (array->layout & GRUB_RAID_LAYOUT_SYMMETRIC_MASK)
                      {
                        disknr = p + n;
                        if (disknr >= array->total_devs)
                          disknr -= array->total_devs;
                      }
                    else
                      {
                        disknr -= array->total_devs;
                        if (disknr == p)
                          disknr += n;
                      }
                  }
                else
                  disknr = 0;
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

static grub_err_t
insert_array (grub_disk_t disk, struct grub_raid_array *new_array,
              grub_disk_addr_t start_sector, const char *scanner_name,
	      grub_raid_t raid __attribute__ ((unused)))
{
  struct grub_raid_array *array = 0, *p;

  /* See whether the device is part of an array we have already seen a
     device from.  */
  for (p = array_list; p != NULL; p = p->next)
    if ((p->uuid_len == new_array->uuid_len) &&
        (! grub_memcmp (p->uuid, new_array->uuid, p->uuid_len)))
      {
        grub_free (new_array->uuid);
        array = p;

        /* Do some checks before adding the device to the array.  */

	if (new_array->index >= array->allocated_devs)
	  {
	    void *tmp;
	    unsigned int newnum = 2 * (new_array->index + 1);
	    tmp = grub_realloc (array->members, newnum
				* sizeof (array->members[0]));
	    if (!tmp)
	      return grub_errno;
	    array->members = tmp;
	    grub_memset (array->members + array->allocated_devs,
			 0, (newnum - array->allocated_devs)
			 * sizeof (array->members[0]));
	    array->allocated_devs = newnum;
	  }

        /* FIXME: Check whether the update time of the superblocks are
           the same.  */

        if (array->total_devs == array->nr_devs)
          /* We found more members of the array than the array
             actually has according to its superblock.  This shouldn't
             happen normally.  */
          return grub_error (GRUB_ERR_BAD_DEVICE,
			     "superfluous RAID member (%d found)",
			     array->total_devs);

        if (array->members[new_array->index].device != NULL)
          /* We found multiple devices with the same number. Again,
             this shouldn't happen.  */
	  return grub_error (GRUB_ERR_BAD_DEVICE,
			     "found two disks with the number %d",
			     new_array->number);

        if (new_array->disk_size < array->disk_size)
          array->disk_size = new_array->disk_size;
        break;
      }

  /* Add an array to the list if we didn't find any.  */
  if (!array)
    {
      array = grub_malloc (sizeof (*array));
      if (!array)
        {
          grub_free (new_array->uuid);
          return grub_errno;
        }

      *array = *new_array;
      array->nr_devs = 0;
#ifdef GRUB_UTIL
      array->driver = raid;
#endif
      array->allocated_devs = 32;
      if (new_array->index >= array->allocated_devs)
	array->allocated_devs = 2 * (new_array->index + 1);

      array->members = grub_zalloc (array->allocated_devs
				    * sizeof (array->members[0]));

      if (!array->members)
        {
          grub_free (new_array->uuid);
          return grub_errno;
        }

      if (! array->name)
	{
	  for (p = array_list; p != NULL; p = p->next)
	    {
	      if (! p->name && p->number == array->number) 
		break;
	    }
	}

      if (array->name || p)
        {
	  /* The number is already in use, so we need to find a new one.
	     (Or, in the case of named arrays, the array doesn't have its
	     own number, but we need one that doesn't clash for use as a key
	     in the disk cache.  */
          int i = array->name ? 0x40000000 : 0;

	  while (1)
	    {
	      for (p = array_list; p != NULL; p = p->next)
		{
		  if (p->number == i)
		    break;
		}

	      if (! p)
		{
		  /* We found an unused number.  */
		  array->number = i;
		  break;
		}

	      i++;
	    }
	}

      /* mdraid 1.x superblocks have only a name stored not a number.
	 Use it directly as GRUB device.  */
      if (! array->name)
	{
	  array->name = grub_xasprintf ("md%d", array->number);
	  if (! array->name)
	    {
	      grub_free (array->members);
	      grub_free (array->uuid);
	      grub_free (array);

	      return grub_errno;
	    }
	}
      else
	{
	  /* Strip off the homehost if present.  */
	  char *colon = grub_strchr (array->name, ':');
	  char *new_name = grub_xasprintf ("md/%s",
					   colon ? colon + 1 : array->name);

	  if (! new_name)
	    {
	      grub_free (array->members);
	      grub_free (array->uuid);
	      grub_free (array);

	      return grub_errno;
	    }

	  grub_free (array->name);
	  array->name = new_name;
	}

      grub_dprintf ("raid", "Found array %s (%s)\n", array->name,
                    scanner_name);

      /* Add our new array to the list.  */
      array->next = array_list;
      array_list = array;

      /* RAID 1 doesn't use a chunksize but code assumes one so set
	 one. */
      if (array->level == 1)
	array->chunk_size = 64;
    }

  /* Add the device to the array. */
  array->members[new_array->index].device = disk;
  array->members[new_array->index].start_sector = start_sector;
  array->nr_devs++;

  return 0;
}

static grub_raid_t grub_raid_list;

static void
free_array (void)
{
  struct grub_raid_array *array;

  array = array_list;
  while (array)
    {
      struct grub_raid_array *p;
      unsigned int i;

      p = array;
      array = array->next;

      for (i = 0; i < p->allocated_devs; i++)
        if (p->members[i].device)
          grub_disk_close (p->members[i].device);
      grub_free (p->members);

      grub_free (p->uuid);
      grub_free (p->name);
      grub_free (p);
    }

  array_list = 0;
}

void
grub_raid_register (grub_raid_t raid)
{
  auto int hook (const char *name);
  int hook (const char *name)
    {
      grub_disk_t disk;
      struct grub_raid_array array;
      grub_disk_addr_t start_sector;

      grub_dprintf ("raid", "Scanning for RAID devices on disk %s\n", name);

      disk = grub_disk_open (name);
      if (!disk)
        return 0;

      if ((disk->total_sectors != GRUB_ULONG_MAX) &&
	  (! grub_raid_list->detect (disk, &array, &start_sector)) &&
	  (! insert_array (disk, &array, start_sector, grub_raid_list->name,
			   grub_raid_list)))
	return 0;

      /* This error usually means it's not raid, no need to display
	 it.  */
      if (grub_errno != GRUB_ERR_OUT_OF_RANGE)
	grub_print_error ();

      grub_errno = GRUB_ERR_NONE;

      grub_disk_close (disk);

      return 0;
    }

  raid->next = grub_raid_list;
  grub_raid_list = raid;
  grub_device_iterate (&hook);
}

void
grub_raid_unregister (grub_raid_t raid)
{
  grub_raid_t *p, q;

  for (p = &grub_raid_list, q = *p; q; p = &(q->next), q = q->next)
    if (q == raid)
      {
	*p = q->next;
	break;
      }
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
#ifdef GRUB_UTIL
    .memberlist = grub_raid_memberlist,
    .raidname = grub_raid_getname,
#endif
    .next = 0
  };


GRUB_MOD_INIT(raid)
{
  grub_disk_dev_register (&grub_raid_dev);
}

GRUB_MOD_FINI(raid)
{
  grub_disk_dev_unregister (&grub_raid_dev);
  free_array ();
}
