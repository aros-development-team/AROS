/* ofdisk.c - Open Firmware disk access.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2004,2006,2007,2008,2009  Free Software Foundation, Inc.
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

#include <grub/misc.h>
#include <grub/disk.h>
#include <grub/mm.h>
#include <grub/ieee1275/ieee1275.h>
#include <grub/ieee1275/ofdisk.h>
#include <grub/i18n.h>
#include <grub/time.h>

static char *last_devpath;
static grub_ieee1275_ihandle_t last_ihandle;

struct ofdisk_hash_ent
{
  char *devpath;
  /* Pointer to shortest available name on nodes representing canonical names,
     otherwise NULL.  */
  const char *shortest;
  struct ofdisk_hash_ent *next;
};

#define OFDISK_HASH_SZ	8
static struct ofdisk_hash_ent *ofdisk_hash[OFDISK_HASH_SZ];

static int
ofdisk_hash_fn (const char *devpath)
{
  int hash = 0;
  while (*devpath)
    hash ^= *devpath++;
  return (hash & (OFDISK_HASH_SZ - 1));
}

static struct ofdisk_hash_ent *
ofdisk_hash_find (const char *devpath)
{
  struct ofdisk_hash_ent *p = ofdisk_hash[ofdisk_hash_fn(devpath)];

  while (p)
    {
      if (!grub_strcmp (p->devpath, devpath))
	break;
      p = p->next;
    }
  return p;
}

static struct ofdisk_hash_ent *
ofdisk_hash_add_real (char *devpath)
{
  struct ofdisk_hash_ent *p;
  struct ofdisk_hash_ent **head = &ofdisk_hash[ofdisk_hash_fn(devpath)];

  p = grub_malloc(sizeof (*p));
  if (!p)
    return NULL;

  p->devpath = devpath;
  p->next = *head;
  p->shortest = 0;
  *head = p;
  return p;
}

static struct ofdisk_hash_ent *
ofdisk_hash_add (char *devpath, char *curcan)
{
  struct ofdisk_hash_ent *p, *pcan;

  p = ofdisk_hash_add_real (devpath);

  grub_dprintf ("disk", "devpath = %s, canonical = %s\n", devpath, curcan);

  if (!curcan)
    {
      p->shortest = devpath;
      return p;
    }

  pcan = ofdisk_hash_find (curcan);
  if (!pcan)
    pcan = ofdisk_hash_add_real (curcan);
  else
    grub_free (curcan);

  if (!pcan)
    grub_errno = GRUB_ERR_NONE;
  else
    {
      if (!pcan->shortest
	  || grub_strlen (pcan->shortest) > grub_strlen (devpath))
	pcan->shortest = devpath;
    }

  return p;
}

static void
scan (void)
{
  auto int dev_iterate_real (const char *name, const char *path);

  int dev_iterate_real (const char *name, const char *path)
    {
      struct ofdisk_hash_ent *op;

      grub_dprintf ("disk", "disk name = %s, path = %s\n", name,
		    path);

      op = ofdisk_hash_find (path);
      if (!op)
	{
	  char *name_dup = grub_strdup (name);
	  char *can = grub_strdup (path);
	  if (!name_dup || !can)
	    {
	      grub_errno = GRUB_ERR_NONE;
	      grub_free (name_dup);
	      grub_free (can);
	      return 0;
	    }
	  op = ofdisk_hash_add (name_dup, can);
	}
      return 0;
    }

  auto int dev_iterate_alias (struct grub_ieee1275_devalias *alias);
  int dev_iterate_alias (struct grub_ieee1275_devalias *alias)
  {
    if (grub_strcmp (alias->type, "block") != 0)
      return 0;
    return dev_iterate_real (alias->name, alias->path);
  }

  auto int dev_iterate (struct grub_ieee1275_devalias *alias);
  int dev_iterate (struct grub_ieee1275_devalias *alias)
  {
    if (grub_strcmp (alias->type, "vscsi") == 0)
      {
	static grub_ieee1275_ihandle_t ihandle;
	struct set_color_args
	{
	  struct grub_ieee1275_common_hdr common;
	  grub_ieee1275_cell_t method;
	  grub_ieee1275_cell_t ihandle;
	  grub_ieee1275_cell_t catch_result;
	  grub_ieee1275_cell_t nentries;
	  grub_ieee1275_cell_t table;
	}
	args;
	char *buf, *bufptr;
	unsigned i;

	if (grub_ieee1275_open (alias->path, &ihandle))
	  return 0;
    
	INIT_IEEE1275_COMMON (&args.common, "call-method", 2, 3);
	args.method = (grub_ieee1275_cell_t) "vscsi-report-luns";
	args.ihandle = ihandle;
	args.table = 0;
	args.nentries = 0;

	if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
	  {
	    grub_ieee1275_close (ihandle);
	    return 0;
	  }

	buf = grub_malloc (grub_strlen (alias->path) + 32);
	if (!buf)
	  return 0;
	bufptr = grub_stpcpy (buf, alias->path);

	for (i = 0; i < args.nentries; i++)
	  {
	    grub_uint64_t *ptr;

	    ptr = *(grub_uint64_t **) (args.table + 4 + 8 * i);
	    while (*ptr)
	      {
		grub_snprintf (bufptr, 32, "/disk@%" PRIxGRUB_UINT64_T, *ptr++);
		if (dev_iterate_real (buf, buf))
		  return 1;
	      }
	  }
	grub_ieee1275_close (ihandle);
	grub_free (buf);
	return 0;
      }

    if (!grub_ieee1275_test_flag (GRUB_IEEE1275_FLAG_NO_TREE_SCANNING_FOR_DISKS)
	&& grub_strcmp (alias->type, "block") == 0)
      return dev_iterate_real (alias->path, alias->path);

    return grub_children_iterate (alias->path, dev_iterate);
  }

  grub_devalias_iterate (dev_iterate_alias);
  grub_children_iterate ("/", dev_iterate);
}

static int
grub_ofdisk_iterate (int (*hook) (const char *name),
		     grub_disk_pull_t pull)
{
  unsigned i;

  if (pull != GRUB_DISK_PULL_NONE)
    return 0;

  scan ();
  
  for (i = 0; i < ARRAY_SIZE (ofdisk_hash); i++)
    {
      static struct ofdisk_hash_ent *ent;
      for (ent = ofdisk_hash[i]; ent; ent = ent->next)
	{
	  if (!ent->shortest)
	    continue;
	  if (grub_ieee1275_test_flag (GRUB_IEEE1275_FLAG_OFDISK_SDCARD_ONLY))
	    {
	      grub_ieee1275_phandle_t dev;
	      char tmp[8];

	      if (grub_ieee1275_finddevice (ent->devpath, &dev))
		{
		  grub_dprintf ("disk", "finddevice (%s) failed\n",
				ent->devpath);
		  continue;
		}

	      if (grub_ieee1275_get_property (dev, "iconname", tmp,
					      sizeof tmp, 0))
		{
		  grub_dprintf ("disk", "get iconname failed\n");
		  continue;
		}

	      if (grub_strcmp (tmp, "sdmmc") != 0)
		{
		  grub_dprintf ("disk", "device is not an SD card\n");
		  continue;
		}
	    }

	  if (grub_strncmp (ent->shortest, "cdrom", 5) == 0)
	    continue;

	  {
	    char buffer[sizeof ("ieee1275/") + 2 * grub_strlen (ent->shortest)];
	    const char *iptr;
	    char *optr;
	    optr = grub_stpcpy (buffer, "ieee1275/");
	    for (iptr = ent->shortest; *iptr; )
	      {
		if (*iptr == ',')
		  *optr++ = '\\';
		*optr++ = *iptr++;
	      }
	    *optr = 0;
	    if (hook (buffer))
	      return 1;
	  }
	}
    }	  
  return 0;
}

static char *
compute_dev_path (const char *name)
{
  char *devpath = grub_malloc (grub_strlen (name) + 3);
  char *p, c;

  if (!devpath)
    return NULL;

  /* Un-escape commas. */
  p = devpath;
  while ((c = *name++) != '\0')
    {
      if (c == '\\' && *name == ',')
	{
	  *p++ = ',';
	  name++;
	}
      else
	*p++ = c;
    }

  *p++ = '\0';

  return devpath;
}

static grub_err_t
grub_ofdisk_open (const char *name, grub_disk_t disk)
{
  grub_ieee1275_phandle_t dev;
  char *devpath;
  /* XXX: This should be large enough for any possible case.  */
  char prop[64];
  grub_ssize_t actual;

  if (grub_strncmp (name, "ieee1275/", sizeof ("ieee1275/") - 1) != 0)
      return grub_error (GRUB_ERR_UNKNOWN_DEVICE,
			 "not IEEE1275 device");
  devpath = compute_dev_path (name + sizeof ("ieee1275/") - 1);
  if (! devpath)
    return grub_errno;

  grub_dprintf ("disk", "Opening `%s'.\n", devpath);

  if (grub_ieee1275_finddevice (devpath, &dev))
    {
      grub_free (devpath);
      return grub_error (GRUB_ERR_UNKNOWN_DEVICE,
			 "can't read device properties");
    }

  if (grub_ieee1275_get_property (dev, "device_type", prop, sizeof (prop),
				  &actual))
    {
      grub_free (devpath);
      return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "can't read the device type");
    }

  if (grub_strcmp (prop, "block"))
    {
      grub_free (devpath);
      return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "not a block device");
    }

  /* XXX: There is no property to read the number of blocks.  There
     should be a property `#blocks', but it is not there.  Perhaps it
     is possible to use seek for this.  */
  disk->total_sectors = GRUB_DISK_SIZE_UNKNOWN;

  {
    struct ofdisk_hash_ent *op;
    op = ofdisk_hash_find (devpath);
    if (!op)
      op = ofdisk_hash_add (devpath, NULL);
    else
      grub_free (devpath);
    if (!op)
      return grub_errno;
    disk->id = (unsigned long) op;
    disk->data = op->devpath;
  }

  return 0;
}

static void
grub_ofdisk_close (grub_disk_t disk)
{
  if (disk->data == last_devpath)
    {
      if (last_ihandle)
	grub_ieee1275_close (last_ihandle);
      last_ihandle = 0;
      last_devpath = NULL;
    }
  disk->data = 0;
}

static grub_err_t
grub_ofdisk_prepare (grub_disk_t disk, grub_disk_addr_t sector)
{
  grub_ssize_t status;
  unsigned long long pos;

  if (disk->data != last_devpath)
    {
      if (last_ihandle)
	grub_ieee1275_close (last_ihandle);
      last_ihandle = 0;
      last_devpath = NULL;

      if (! grub_ieee1275_test_flag (GRUB_IEEE1275_FLAG_NO_PARTITION_0))
	{
	  char name2[grub_strlen (disk->data) + 3];
	  char *p;
	  
	  grub_strcpy (name2, disk->data);
	  p = name2 + grub_strlen (name2);
	  *p++ = ':';
	  *p++ = '0';
	  *p = 0;
	  grub_ieee1275_open (name2, &last_ihandle);
	}
      else
	grub_ieee1275_open (disk->data, &last_ihandle);
      if (! last_ihandle)
	return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "can't open device");
      last_devpath = disk->data;      
    }

  pos = sector << GRUB_DISK_SECTOR_BITS;

  grub_ieee1275_seek (last_ihandle, pos, &status);
  if (status < 0)
    return grub_error (GRUB_ERR_READ_ERROR,
		       "seek error, can't seek block %llu",
		       (long long) sector);
  return 0;
}

static grub_err_t
grub_ofdisk_read (grub_disk_t disk, grub_disk_addr_t sector,
		  grub_size_t size, char *buf)
{
  grub_err_t err;
  grub_ssize_t actual;
  err = grub_ofdisk_prepare (disk, sector);
  if (err)
    return err;
  grub_ieee1275_read (last_ihandle, buf, size  << GRUB_DISK_SECTOR_BITS,
		      &actual);
  if (actual != (grub_ssize_t) (size  << GRUB_DISK_SECTOR_BITS))
    return grub_error (GRUB_ERR_READ_ERROR, N_("failure reading sector 0x%llx "
					       "from `%s'"),
		       (unsigned long long) sector,
		       disk->name);

  return 0;
}

static grub_err_t
grub_ofdisk_write (grub_disk_t disk, grub_disk_addr_t sector,
		   grub_size_t size, const char *buf)
{
  grub_err_t err;
  grub_ssize_t actual;
  err = grub_ofdisk_prepare (disk, sector);
  if (err)
    return err;
  grub_ieee1275_write (last_ihandle, buf, size  << GRUB_DISK_SECTOR_BITS,
		       &actual);
  if (actual != (grub_ssize_t) (size << GRUB_DISK_SECTOR_BITS))
    return grub_error (GRUB_ERR_WRITE_ERROR, N_("failure writing sector 0x%llx "
						"to `%s'"),
		       (unsigned long long) sector,
		       disk->name);

  return 0;
}

static struct grub_disk_dev grub_ofdisk_dev =
  {
    .name = "ofdisk",
    .id = GRUB_DISK_DEVICE_OFDISK_ID,
    .iterate = grub_ofdisk_iterate,
    .open = grub_ofdisk_open,
    .close = grub_ofdisk_close,
    .read = grub_ofdisk_read,
    .write = grub_ofdisk_write,
    .next = 0
  };

void
grub_ofdisk_init (void)
{
  grub_disk_dev_register (&grub_ofdisk_dev);
}

void
grub_ofdisk_fini (void)
{
  if (last_ihandle)
    grub_ieee1275_close (last_ihandle);
  last_ihandle = 0;
  last_devpath = NULL;

  grub_disk_dev_unregister (&grub_ofdisk_dev);
}
