/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2006,2007,2008  Free Software Foundation, Inc.
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
#include <grub/partition.h>
#include <grub/mm.h>
#include <grub/types.h>
#include <grub/misc.h>
#include <grub/err.h>
#include <grub/term.h>
#include <grub/efi/api.h>
#include <grub/efi/efi.h>
#include <grub/efi/disk.h>

struct grub_efidisk_data
{
  grub_efi_handle_t handle;
  grub_efi_device_path_t *device_path;
  grub_efi_device_path_t *last_device_path;
  grub_efi_block_io_t *block_io;
  struct grub_efidisk_data *next;
};

/* GUID.  */
static grub_efi_guid_t block_io_guid = GRUB_EFI_BLOCK_IO_GUID;

static struct grub_efidisk_data *fd_devices;
static struct grub_efidisk_data *hd_devices;
static struct grub_efidisk_data *cd_devices;

/* Duplicate a device path.  */
static grub_efi_device_path_t *
duplicate_device_path (const grub_efi_device_path_t *dp)
{
  grub_efi_device_path_t *p;
  grub_size_t total_size = 0;

  for (p = (grub_efi_device_path_t *) dp;
       ;
       p = GRUB_EFI_NEXT_DEVICE_PATH (p))
    {
      total_size += GRUB_EFI_DEVICE_PATH_LENGTH (p);
      if (GRUB_EFI_END_ENTIRE_DEVICE_PATH (p))
	break;
    }

  p = grub_malloc (total_size);
  if (! p)
    return 0;

  grub_memcpy (p, dp, total_size);
  return p;
}

/* Return the device path node right before the end node.  */
static grub_efi_device_path_t *
find_last_device_path (const grub_efi_device_path_t *dp)
{
  grub_efi_device_path_t *next, *p;

  if (GRUB_EFI_END_ENTIRE_DEVICE_PATH (dp))
    return 0;

  for (p = (grub_efi_device_path_t *) dp, next = GRUB_EFI_NEXT_DEVICE_PATH (p);
       ! GRUB_EFI_END_ENTIRE_DEVICE_PATH (next);
       p = next, next = GRUB_EFI_NEXT_DEVICE_PATH (next))
    ;

  return p;
}

static struct grub_efidisk_data *
make_devices (void)
{
  grub_efi_uintn_t num_handles;
  grub_efi_handle_t *handles;
  grub_efi_handle_t *handle;
  struct grub_efidisk_data *devices = 0;

  /* Find handles which support the disk io interface.  */
  handles = grub_efi_locate_handle (GRUB_EFI_BY_PROTOCOL, &block_io_guid,
				    0, &num_handles);
  if (! handles)
    return 0;

  /* Make a linked list of devices.  */
  for (handle = handles; num_handles--; handle++)
    {
      grub_efi_device_path_t *dp;
      grub_efi_device_path_t *ldp;
      struct grub_efidisk_data *d;
      grub_efi_block_io_t *bio;

      dp = grub_efi_get_device_path (*handle);
      if (! dp)
	continue;

      ldp = find_last_device_path (dp);
      if (! ldp)
	/* This is empty. Why?  */
	continue;

      bio = grub_efi_open_protocol (*handle, &block_io_guid,
				    GRUB_EFI_OPEN_PROTOCOL_GET_PROTOCOL);
      if (! bio)
	/* This should not happen... Why?  */
	continue;

      d = grub_malloc (sizeof (*d));
      if (! d)
	{
	  /* Uggh.  */
	  grub_free (handles);
	  return 0;
	}

      d->handle = *handle;
      d->device_path = dp;
      d->last_device_path = ldp;
      d->block_io = bio;
      d->next = devices;
      devices = d;
    }

  grub_free (handles);

  return devices;
}

/* Find the parent device.  */
static struct grub_efidisk_data *
find_parent_device (struct grub_efidisk_data *devices,
		    struct grub_efidisk_data *d)
{
  grub_efi_device_path_t *dp, *ldp;
  struct grub_efidisk_data *parent;

  dp = duplicate_device_path (d->device_path);
  if (! dp)
    return 0;

  ldp = find_last_device_path (dp);
  ldp->type = GRUB_EFI_END_DEVICE_PATH_TYPE;
  ldp->subtype = GRUB_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE;
  ldp->length[0] = sizeof (*ldp);
  ldp->length[1] = 0;

  for (parent = devices; parent; parent = parent->next)
    {
      /* Ignore itself.  */
      if (parent == d)
	continue;

      if (grub_efi_compare_device_paths (parent->device_path, dp) == 0)
	break;
    }

  grub_free (dp);
  return parent;
}

static int
iterate_child_devices (struct grub_efidisk_data *devices,
		       struct grub_efidisk_data *d,
		       int (*hook) (struct grub_efidisk_data *child))
{
  struct grub_efidisk_data *p;

  for (p = devices; p; p = p->next)
    {
      grub_efi_device_path_t *dp, *ldp;

      dp = duplicate_device_path (p->device_path);
      if (! dp)
	return 0;

      ldp = find_last_device_path (dp);
      ldp->type = GRUB_EFI_END_DEVICE_PATH_TYPE;
      ldp->subtype = GRUB_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE;
      ldp->length[0] = sizeof (*ldp);
      ldp->length[1] = 0;

      if (grub_efi_compare_device_paths (dp, d->device_path) == 0)
	if (hook (p))
	  {
	    grub_free (dp);
	    return 1;
	  }

      grub_free (dp);
    }

  return 0;
}

/* Add a device into a list of devices in an ascending order.  */
static void
add_device (struct grub_efidisk_data **devices, struct grub_efidisk_data *d)
{
  struct grub_efidisk_data **p;
  struct grub_efidisk_data *n;

  for (p = devices; *p; p = &((*p)->next))
    {
      int ret;

      ret = grub_efi_compare_device_paths (find_last_device_path ((*p)->device_path),
					   find_last_device_path (d->device_path));
      if (ret == 0)
	ret = grub_efi_compare_device_paths ((*p)->device_path,
					     d->device_path);
      if (ret == 0)
	return;
      else if (ret > 0)
	break;
    }

  n = grub_malloc (sizeof (*n));
  if (! n)
    return;

  grub_memcpy (n, d, sizeof (*n));
  n->next = (*p);
  (*p) = n;
}

/* Name the devices.  */
static void
name_devices (struct grub_efidisk_data *devices)
{
  struct grub_efidisk_data *d;

  /* First, identify devices by media device paths.  */
  for (d = devices; d; d = d->next)
    {
      grub_efi_device_path_t *dp;

      dp = d->last_device_path;
      if (! dp)
	continue;

      if (GRUB_EFI_DEVICE_PATH_TYPE (dp) == GRUB_EFI_MEDIA_DEVICE_PATH_TYPE)
	{
	  int is_hard_drive = 0;

	  switch (GRUB_EFI_DEVICE_PATH_SUBTYPE (dp))
	    {
	    case GRUB_EFI_HARD_DRIVE_DEVICE_PATH_SUBTYPE:
	      is_hard_drive = 1;
	      /* Fall through by intention.  */
	    case GRUB_EFI_CDROM_DEVICE_PATH_SUBTYPE:
	      {
		struct grub_efidisk_data *parent, *parent2;

		parent = find_parent_device (devices, d);
		if (!parent)
		  {
#ifdef DEBUG_NAMES
		    grub_printf ("skipping orphaned partition: ");
		    grub_efi_print_device_path (d->device_path);
#endif
		    break;
		  }
		parent2 = find_parent_device (devices, parent);
		if (parent2)
		  {
#ifdef DEBUG_NAMES
		    grub_printf ("skipping subpartition: ");
		    grub_efi_print_device_path (d->device_path);
#endif
		    /* Mark itself as used.  */
		    d->last_device_path = 0;
		    break;
		  }
		if (!parent->last_device_path)
		  {
		    d->last_device_path = 0;
		    break;
		  }
		if (is_hard_drive)
		  {
#ifdef DEBUG_NAMES
		    grub_printf ("adding a hard drive by a partition: ");
		    grub_efi_print_device_path (parent->device_path);
#endif
		    add_device (&hd_devices, parent);
		  }
		else
		  {
#ifdef DEBUG_NAMES
		    grub_printf ("adding a cdrom by a partition: ");
		    grub_efi_print_device_path (parent->device_path);
#endif
		    add_device (&cd_devices, parent);
		  }

		/* Mark the parent as used.  */
		parent->last_device_path = 0;
	      }
	      /* Mark itself as used.  */
	      d->last_device_path = 0;
	      break;

	    default:
#ifdef DEBUG_NAMES
	      grub_printf ("skipping other type: ");
	      grub_efi_print_device_path (d->device_path);
#endif
	      /* For now, ignore the others.  */
	      break;
	    }
	}
      else
	{
#ifdef DEBUG_NAMES
	  grub_printf ("skipping non-media: ");
	  grub_efi_print_device_path (d->device_path);
#endif
	}
    }

  /* Let's see what can be added more.  */
  for (d = devices; d; d = d->next)
    {
      grub_efi_device_path_t *dp;
      grub_efi_block_io_media_t *m;

      dp = d->last_device_path;
      if (! dp)
	continue;

      m = d->block_io->media;
      if (m->logical_partition)
	{
	  /* Only one partition in a non-media device. Assume that this
	     is a floppy drive.  */
#ifdef DEBUG_NAMES
	  grub_printf ("adding a floppy by guessing: ");
	  grub_efi_print_device_path (d->device_path);
#endif
	  add_device (&fd_devices, d);
	}
      else if (m->read_only && m->block_size > GRUB_DISK_SECTOR_SIZE)
	{
	  /* This check is too heuristic, but assume that this is a
	     CDROM drive.  */
#ifdef DEBUG_NAMES
	  grub_printf ("adding a cdrom by guessing: ");
	  grub_efi_print_device_path (d->device_path);
#endif
	  add_device (&cd_devices, d);
	}
      else
	{
	  /* The default is a hard drive.  */
#ifdef DEBUG_NAMES
	  grub_printf ("adding a hard drive by guessing: ");
	  grub_efi_print_device_path (d->device_path);
#endif
	  add_device (&hd_devices, d);
	}
    }
}

static void
free_devices (struct grub_efidisk_data *devices)
{
  struct grub_efidisk_data *p, *q;

  for (p = devices; p; p = q)
    {
      q = p->next;
      grub_free (p);
    }
}

/* Enumerate all disks to name devices.  */
static void
enumerate_disks (void)
{
  struct grub_efidisk_data *devices;

  devices = make_devices ();
  if (! devices)
    return;

  name_devices (devices);
  free_devices (devices);
}

static int
grub_efidisk_iterate (int (*hook) (const char *name),
		      grub_disk_pull_t pull)
{
  struct grub_efidisk_data *d;
  char buf[16];
  int count;

  switch (pull)
    {
    case GRUB_DISK_PULL_NONE:
      for (d = hd_devices, count = 0; d; d = d->next, count++)
	{
	  grub_snprintf (buf, sizeof (buf), "hd%d", count);
	  grub_dprintf ("efidisk", "iterating %s\n", buf);
	  if (hook (buf))
	    return 1;
	}
      break;
    case GRUB_DISK_PULL_REMOVABLE:
      for (d = fd_devices, count = 0; d; d = d->next, count++)
	{
	  grub_snprintf (buf, sizeof (buf), "fd%d", count);
	  grub_dprintf ("efidisk", "iterating %s\n", buf);
	  if (hook (buf))
	    return 1;
	}

      for (d = cd_devices, count = 0; d; d = d->next, count++)
	{
	  grub_snprintf (buf, sizeof (buf), "cd%d", count);
	  grub_dprintf ("efidisk", "iterating %s\n", buf);
	  if (hook (buf))
	    return 1;
	}
      break;
    default:
      return 0;
    }

  return 0;
}

static int
get_drive_number (const char *name)
{
  unsigned long drive;

  if ((name[0] != 'f' && name[0] != 'h' && name[0] != 'c') || name[1] != 'd')
    goto fail;

  drive = grub_strtoul (name + 2, 0, 10);
  if (grub_errno != GRUB_ERR_NONE)
    goto fail;

  return (int) drive ;

 fail:
  grub_error (GRUB_ERR_UNKNOWN_DEVICE, "not a efidisk");
  return -1;
}

static struct grub_efidisk_data *
get_device (struct grub_efidisk_data *devices, int num)
{
  struct grub_efidisk_data *d;

  for (d = devices; d && num; d = d->next, num--)
    ;

  if (num == 0)
    return d;

  return 0;
}

static grub_err_t
grub_efidisk_open (const char *name, struct grub_disk *disk)
{
  int num;
  struct grub_efidisk_data *d = 0;
  grub_efi_block_io_media_t *m;

  grub_dprintf ("efidisk", "opening %s\n", name);

  num = get_drive_number (name);
  if (num < 0)
    return grub_errno;

  switch (name[0])
    {
    case 'f':
      d = get_device (fd_devices, num);
      break;
    case 'c':
      d = get_device (cd_devices, num);
      break;
    case 'h':
      d = get_device (hd_devices, num);
      break;
    default:
      /* Never reach here.  */
      break;
    }

  if (! d)
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "no such device");

  disk->id = ((num << 8) | name[0]);
  m = d->block_io->media;
  /* FIXME: Probably it is better to store the block size in the disk,
     and total sectors should be replaced with total blocks.  */
  grub_dprintf ("efidisk", "m = %p, last block = %llx, block size = %x\n",
		m, (unsigned long long) m->last_block, m->block_size);
  disk->total_sectors = m->last_block + 1;
  if (m->block_size & (m->block_size - 1) || !m->block_size)
    return grub_error (GRUB_ERR_IO, "invalid sector size %d",
		       m->block_size);
  for (disk->log_sector_size = 0;
       (1U << disk->log_sector_size) < m->block_size;
       disk->log_sector_size++);
  disk->data = d;

  grub_dprintf ("efidisk", "opening %s succeeded\n", name);

  return GRUB_ERR_NONE;
}

static void
grub_efidisk_close (struct grub_disk *disk __attribute__ ((unused)))
{
  /* EFI disks do not allocate extra memory, so nothing to do here.  */
  grub_dprintf ("efidisk", "closing %s\n", disk->name);
}

static grub_err_t
grub_efidisk_read (struct grub_disk *disk, grub_disk_addr_t sector,
		   grub_size_t size, char *buf)
{
  /* For now, use the disk io interface rather than the block io's.  */
  struct grub_efidisk_data *d;
  grub_efi_block_io_t *bio;
  grub_efi_status_t status;

  d = disk->data;
  bio = d->block_io;

  grub_dprintf ("efidisk",
		"reading 0x%lx sectors at the sector 0x%llx from %s\n",
		(unsigned long) size, (unsigned long long) sector, disk->name);

  status = efi_call_5 (bio->read_blocks, bio, bio->media->media_id,
		       (grub_efi_uint64_t) sector,
		       (grub_efi_uintn_t) size << disk->log_sector_size,
		       buf);
  if (status != GRUB_EFI_SUCCESS)
    return grub_error (GRUB_ERR_READ_ERROR,
		       N_("failure reading sector 0x%llx from `%s'"),
		       (unsigned long long) sector,
		       disk->name);

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_efidisk_write (struct grub_disk *disk, grub_disk_addr_t sector,
		    grub_size_t size, const char *buf)
{
  /* For now, use the disk io interface rather than the block io's.  */
  struct grub_efidisk_data *d;
  grub_efi_block_io_t *bio;
  grub_efi_status_t status;

  d = disk->data;
  bio = d->block_io;

  grub_dprintf ("efidisk",
		"writing 0x%lx sectors at the sector 0x%llx to %s\n",
		(unsigned long) size, (unsigned long long) sector, disk->name);

  status = efi_call_5 (bio->write_blocks, bio, bio->media->media_id,
		       (grub_efi_uint64_t) sector,
		       (grub_efi_uintn_t) size << disk->log_sector_size,
		       (void *) buf);
  if (status != GRUB_EFI_SUCCESS)
    return grub_error (GRUB_ERR_WRITE_ERROR,
		       N_("failure writing sector 0x%llx to `%s'"),
		       (unsigned long long) sector, disk->name);

  return GRUB_ERR_NONE;
}

static struct grub_disk_dev grub_efidisk_dev =
  {
    .name = "efidisk",
    .id = GRUB_DISK_DEVICE_EFIDISK_ID,
    .iterate = grub_efidisk_iterate,
    .open = grub_efidisk_open,
    .close = grub_efidisk_close,
    .read = grub_efidisk_read,
    .write = grub_efidisk_write,
    .next = 0
  };

void
grub_efidisk_init (void)
{
  enumerate_disks ();
  grub_disk_dev_register (&grub_efidisk_dev);
}

void
grub_efidisk_fini (void)
{
  free_devices (fd_devices);
  free_devices (hd_devices);
  free_devices (cd_devices);
  grub_disk_dev_unregister (&grub_efidisk_dev);
}

/* Some utility functions to map GRUB devices with EFI devices.  */
grub_efi_handle_t
grub_efidisk_get_device_handle (grub_disk_t disk)
{
  struct grub_efidisk_data *d;
  char type;

  if (disk->dev->id != GRUB_DISK_DEVICE_EFIDISK_ID)
    return 0;

  d = disk->data;
  type = disk->name[0];

  switch (type)
    {
    case 'f':
      /* This is the simplest case.  */
      return d->handle;

    case 'c':
      /* FIXME: probably this is not correct.  */
      return d->handle;

    case 'h':
      /* If this is the whole disk, just return its own data.  */
      if (! disk->partition)
	return d->handle;

      /* Otherwise, we must query the corresponding device to the firmware.  */
      {
	struct grub_efidisk_data *devices;
	grub_efi_handle_t handle = 0;
	auto int find_partition (struct grub_efidisk_data *c);

	int find_partition (struct grub_efidisk_data *c)
	  {
	    grub_efi_hard_drive_device_path_t hd;

	    grub_memcpy (&hd, c->last_device_path, sizeof (hd));

	    if ((GRUB_EFI_DEVICE_PATH_TYPE (c->last_device_path)
		 == GRUB_EFI_MEDIA_DEVICE_PATH_TYPE)
		&& (GRUB_EFI_DEVICE_PATH_SUBTYPE (c->last_device_path)
		    == GRUB_EFI_HARD_DRIVE_DEVICE_PATH_SUBTYPE)
		&& (grub_partition_get_start (disk->partition)
		    == hd.partition_start)
		&& (grub_partition_get_len (disk->partition)
		    == hd.partition_size))
	      {
		handle = c->handle;
		return 1;
	      }

	    return 0;
	  }

	devices = make_devices ();
	iterate_child_devices (devices, d, find_partition);
	free_devices (devices);

	if (handle != 0)
	  return handle;
      }
      break;

    default:
      break;
    }

  return 0;
}

#define NEEDED_BUFLEN sizeof ("XdXXXXXXXXXX")
static inline int
get_diskname_from_path_real (const grub_efi_device_path_t *path,
			     struct grub_efidisk_data *head,
			     char *buf)
{
  int count = 0;
  struct grub_efidisk_data *d;
  for (d = head, count = 0; d; d = d->next, count++)
    if (grub_efi_compare_device_paths (d->device_path, path) == 0)
      {
	grub_snprintf (buf, NEEDED_BUFLEN - 1, "d%d", count);
	return 1;
      }
  return 0;
}

static inline int
get_diskname_from_path (const grub_efi_device_path_t *path,
			char *buf)
{
  if (get_diskname_from_path_real (path, hd_devices, buf + 1))
    {
      buf[0] = 'h';
      return 1;
    }

  if (get_diskname_from_path_real (path, fd_devices, buf + 1))
    {
      buf[0] = 'f';
      return 1;
    }

  if (get_diskname_from_path_real (path, cd_devices, buf + 1))
    {
      buf[0] = 'c';
      return 1;
    }
  return 0;
}

char *
grub_efidisk_get_device_name (grub_efi_handle_t *handle)
{
  grub_efi_device_path_t *dp, *ldp;
  char device_name[NEEDED_BUFLEN];

  dp = grub_efi_get_device_path (handle);
  if (! dp)
    return 0;

  ldp = find_last_device_path (dp);
  if (! ldp)
    return 0;

  if (GRUB_EFI_DEVICE_PATH_TYPE (ldp) == GRUB_EFI_MEDIA_DEVICE_PATH_TYPE
      && (GRUB_EFI_DEVICE_PATH_SUBTYPE (ldp)
	  == GRUB_EFI_HARD_DRIVE_DEVICE_PATH_SUBTYPE))
    {
      char *partition_name = NULL;
      char *dev_name;
      grub_efi_device_path_t *dup_dp, *dup_ldp;
      grub_efi_hard_drive_device_path_t hd;
      grub_disk_t parent = 0;

      auto int find_partition (grub_disk_t disk, const grub_partition_t part);

      /* Find the identical partition.  */
      int find_partition (grub_disk_t disk __attribute__ ((unused)),
			  const grub_partition_t part)
	{
	  if (grub_partition_get_start (part) == hd.partition_start
	      && grub_partition_get_len (part) == hd.partition_size)
	    {
	      partition_name = grub_partition_get_name (part);
	      return 1;
	    }

	  return 0;
	}

      /* It is necessary to duplicate the device path so that GRUB
	 can overwrite it.  */
      dup_dp = duplicate_device_path (dp);
      if (! dup_dp)
	return 0;

      dup_ldp = find_last_device_path (dup_dp);
      dup_ldp->type = GRUB_EFI_END_DEVICE_PATH_TYPE;
      dup_ldp->subtype = GRUB_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE;
      dup_ldp->length[0] = sizeof (*dup_ldp);
      dup_ldp->length[1] = 0;

      if (!get_diskname_from_path (dup_dp, device_name))
	return 0;
      parent = grub_disk_open (device_name);
      grub_free (dup_dp);

      if (! parent)
	return 0;

      /* Find a partition which matches the hard drive device path.  */
      grub_memcpy (&hd, ldp, sizeof (hd));
      if (hd.partition_start == 0
	  && hd.partition_size == grub_disk_get_size (parent))
	{
	  dev_name = grub_strdup (parent->name);
	}
      else
	{
	  grub_partition_iterate (parent, find_partition);

	  if (! partition_name)
	    {
	      grub_disk_close (parent);
	      return 0;
	    }

	  dev_name = grub_xasprintf ("%s,%s", parent->name, partition_name);
	  grub_free (partition_name);
	}
      grub_disk_close (parent);

      return dev_name;
    }
  else
    {
      /* This should be an entire disk.  */
      if (!get_diskname_from_path (dp, device_name))
	return 0;
      return grub_strdup (device_name);
    }
}
