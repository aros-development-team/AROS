/* fs_file.c - Access partition by a file it contains.  */
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

#include <grub/disk.h>
#include <grub/dl.h>
#include <grub/file.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/partition.h>

static grub_device_t
search_fs_file (const char *key, unsigned long *count)
{
  char *filename = NULL;
  grub_device_t ret = NULL;
  *count = 0;

  auto int iterate_device (const char *name);
  int iterate_device (const char *name)
  {
    int len;
    grub_file_t file;

    (*count)++;

    len = grub_strlen (name) + 2 + grub_strlen (key) + 1;
    filename = grub_realloc (filename, len);
    if (! filename)
      return 1;

    grub_sprintf (filename, "(%s)%s", name, key);
    file = grub_file_open (filename);
    if (file)
      {
	grub_file_close (file);
	ret = grub_device_open (name);
	return 1;
      }

    grub_errno = GRUB_ERR_NONE;
    return 0;
  }

  grub_device_iterate (iterate_device);
  grub_free (filename);

  return ret;
}

static grub_err_t
grub_fs_file_open (const char *name, grub_disk_t disk)
{
  grub_device_t dev;

  if (grub_strncmp (name, "FILE=", sizeof ("FILE=") - 1))
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "not a FILE virtual volume");

  dev = search_fs_file (name + sizeof ("FILE=") - 1, &disk->id);
  if (! dev)
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "no matching file found");

  disk->total_sectors = dev->disk->total_sectors;
  disk->has_partitions = 0;
  if (dev->disk->partition)
    {
      disk->partition = grub_malloc (sizeof (*disk->partition));
      if (disk->partition)
	grub_memcpy (disk->partition, dev->disk->partition,
		     sizeof (*disk->partition));
    }
  else
    disk->partition = NULL;

  disk->data = dev;

  return GRUB_ERR_NONE;
}

static void
grub_fs_file_close (grub_disk_t disk)
{
  grub_device_t parent = disk->data;
  grub_device_close (parent);
}

static grub_err_t
grub_fs_file_read (grub_disk_t disk, grub_disk_addr_t sector,
		   grub_size_t size, char *buf)
{
  grub_device_t parent = disk->data;
  return parent->disk->dev->read (parent->disk, sector, size, buf);
}

static grub_err_t
grub_fs_file_write (grub_disk_t disk, grub_disk_addr_t sector,
		    grub_size_t size, const char *buf)
{
  grub_device_t parent = disk->data;
  return parent->disk->dev->write (parent->disk, sector, size, buf);
}

static struct grub_disk_dev grub_fs_file_dev = {
  .name = "fs_file",
  .id = GRUB_DISK_DEVICE_FILE_ID,
  .open = grub_fs_file_open,
  .close = grub_fs_file_close,
  .read = grub_fs_file_read,
  .write = grub_fs_file_write,
  .next = 0
};

GRUB_MOD_INIT (fs_file)
{
  grub_disk_dev_register (&grub_fs_file_dev);
}

GRUB_MOD_FINI (fs_file)
{
  grub_disk_dev_unregister (&grub_fs_file_dev);
}
