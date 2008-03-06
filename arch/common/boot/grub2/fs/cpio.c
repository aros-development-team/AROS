/* cpio.c - cpio and tar filesystem.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2007,2008 Free Software Foundation, Inc.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <grub/file.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/disk.h>
#include <grub/dl.h>

#define	MAGIC_BCPIO	070707

struct HEAD_BCPIO
{
  grub_uint16_t magic;
  grub_uint16_t dev;
  grub_uint16_t ino;
  grub_uint16_t mode;
  grub_uint16_t uid;
  grub_uint16_t gid;
  grub_uint16_t nlink;
  grub_uint16_t rdev;
  grub_uint16_t mtime_1;
  grub_uint16_t mtime_2;
  grub_uint16_t namesize;
  grub_uint16_t filesize_1;
  grub_uint16_t filesize_2;
} __attribute__ ((packed));

#define MAGIC_USTAR	"ustar"

struct HEAD_USTAR
{
  char name[100];
  char mode[8];
  char uid[8];
  char gid[8];
  char size[12];
  char mtime[12];
  char chksum[8];
  char typeflag;
  char linkname[100];
  char magic[6];
  char version[2];
  char uname[32];
  char gname[32];
  char devmajor[8];
  char devminor[8];
  char prefix[155];
} __attribute__ ((packed));

#define HEAD_LENG	sizeof(struct HEAD_USTAR)

#define MODE_BCPIO	1
#define MODE_USTAR	2

struct grub_cpio_data
{
  grub_disk_t disk;
  grub_uint32_t hofs;
  grub_uint32_t dofs;
  grub_uint32_t size;
  int mode;
};

#ifndef GRUB_UTIL
static grub_dl_t my_mod;
#endif

static grub_err_t
grub_cpio_find_file (struct grub_cpio_data *data, char **name,
		     grub_uint32_t * ofs)
{
  if (data->mode == MODE_BCPIO)
    {
      struct HEAD_BCPIO hd;

      if (grub_disk_read
	  (data->disk, 0, data->hofs, sizeof (hd), (char *) &hd))
	return grub_errno;

      if (hd.magic != MAGIC_BCPIO)
	return grub_error (GRUB_ERR_BAD_FS, "Invalid cpio archive");

      data->size = (((grub_uint32_t) hd.filesize_1) << 16) + hd.filesize_2;

      if (hd.namesize & 1)
	hd.namesize++;

      if ((*name = grub_malloc (hd.namesize)) == NULL)
	return grub_errno;

      if (grub_disk_read (data->disk, 0, data->hofs + sizeof (hd),
			  hd.namesize, *name))
	{
	  grub_free (*name);
	  return grub_errno;
	}

      if (data->size == 0 && hd.mode == 0 && hd.namesize == 11 + 1
	  && ! grub_memcmp(*name, "TRAILER!!!", 11))
	{
	  *ofs = 0;
	  return GRUB_ERR_NONE;
	}

      data->dofs = data->hofs + sizeof (hd) + hd.namesize;
      *ofs = data->dofs + data->size;
      if (data->size & 1)
	(*ofs)++;
    }
  else
    {
      struct HEAD_USTAR hd;

      if (grub_disk_read
	  (data->disk, 0, data->hofs, sizeof (hd), (char *) &hd))
	return grub_errno;

      if (!hd.name[0])
	{
	  *ofs = 0;
	  return GRUB_ERR_NONE;
	}

      if (grub_memcmp (hd.magic, MAGIC_USTAR, sizeof (MAGIC_USTAR) - 1))
	return grub_error (GRUB_ERR_BAD_FS, "Invalid tar archive");

      if ((*name = grub_strdup (hd.name)) == NULL)
	return grub_errno;

      data->size = grub_strtoul (hd.size, NULL, 8);
      data->dofs = data->hofs + GRUB_DISK_SECTOR_SIZE;
      *ofs = data->dofs + ((data->size + GRUB_DISK_SECTOR_SIZE - 1) &
			   ~(GRUB_DISK_SECTOR_SIZE - 1));
    }
  return GRUB_ERR_NONE;
}

static struct grub_cpio_data *
grub_cpio_mount (grub_disk_t disk)
{
  char hd[HEAD_LENG];
  struct grub_cpio_data *data;
  int mode;

  if (grub_disk_read (disk, 0, 0, sizeof (hd), hd))
    goto fail;

  if (((struct HEAD_BCPIO *) hd)->magic == MAGIC_BCPIO)
    mode = MODE_BCPIO;
  else if (!grub_memcmp (((struct HEAD_USTAR *) hd)->magic, MAGIC_USTAR,
			 sizeof (MAGIC_USTAR) - 1))
    mode = MODE_USTAR;
  else
    goto fail;

  data = (struct grub_cpio_data *) grub_malloc (sizeof (*data));
  if (!data)
    goto fail;

  data->disk = disk;
  data->mode = mode;

  return data;

fail:
  grub_error (GRUB_ERR_BAD_FS, "not a cpio filesystem");
  return 0;
}

static grub_err_t
grub_cpio_dir (grub_device_t device, const char *path,
	       int (*hook) (const char *filename, int dir))
{
  struct grub_cpio_data *data;
  grub_uint32_t ofs;
  char *prev, *name;
  const char *np;
  int len;

#ifndef GRUB_UTIL
  grub_dl_ref (my_mod);
#endif

  prev = 0;

  data = grub_cpio_mount (device->disk);
  if (!data)
    goto fail;

  np = path + 1;
  len = grub_strlen (path) - 1;

  data->hofs = 0;
  while (1)
    {
      if (grub_cpio_find_file (data, &name, &ofs))
	goto fail;

      if (!ofs)
	break;

      if (grub_memcmp (np, name, len) == 0)
	{
	  char *p, *n;

	  n = name + len;
	  if (*n == '/')
	    n++;

	  p = grub_strchr (name + len, '/');
	  if (p)
	    *p = 0;

	  if ((!prev) || (grub_strcmp (prev, name) != 0))
	    {
	      hook (name + len, p != NULL);
	      if (prev)
		grub_free (prev);
	      prev = name;
	    }
	  else
	    grub_free (name);
	}
      data->hofs = ofs;
    }

fail:

  if (prev)
    grub_free (prev);

  if (data)
    grub_free (data);

#ifndef GRUB_UTIL
  grub_dl_unref (my_mod);
#endif

  return grub_errno;
}

static grub_err_t
grub_cpio_open (grub_file_t file, const char *name)
{
  struct grub_cpio_data *data;
  grub_uint32_t ofs;
  char *fn;

#ifndef GRUB_UTIL
  grub_dl_ref (my_mod);
#endif

  data = grub_cpio_mount (file->device->disk);
  if (!data)
    goto fail;

  data->hofs = 0;
  while (1)
    {
      if (grub_cpio_find_file (data, &fn, &ofs))
	goto fail;

      if (!ofs)
	{
	  grub_error (GRUB_ERR_FILE_NOT_FOUND, "file not found");
	  break;
	}

      if (grub_strcmp (name + 1, fn) == 0)
	{
	  file->data = data;
	  file->size = data->size;
	  grub_free (fn);

	  return GRUB_ERR_NONE;
	}

      grub_free (fn);
      data->hofs = ofs;
    }

fail:

  if (data)
    grub_free (data);

#ifndef GRUB_UTIL
  grub_dl_unref (my_mod);
#endif

  return grub_errno;
}

static grub_ssize_t
grub_cpio_read (grub_file_t file, char *buf, grub_size_t len)
{
  struct grub_cpio_data *data;

  data = file->data;
  return (grub_disk_read (data->disk, 0, data->dofs + file->offset,
			  len, buf)) ? -1 : len;
}

static grub_err_t
grub_cpio_close (grub_file_t file)
{
  grub_free (file->data);

#ifndef GRUB_UTIL
  grub_dl_unref (my_mod);
#endif

  return grub_errno;
}

static struct grub_fs grub_cpio_fs = {
  .name = "cpiofs",
  .dir = grub_cpio_dir,
  .open = grub_cpio_open,
  .read = grub_cpio_read,
  .close = grub_cpio_close,
  .label = 0,
  .next = 0
};

GRUB_MOD_INIT (cpio)
{
  grub_fs_register (&grub_cpio_fs);
#ifndef GRUB_UTIL
  my_mod = mod;
#endif
}

GRUB_MOD_FINI (cpio)
{
  grub_fs_unregister (&grub_cpio_fs);
}
