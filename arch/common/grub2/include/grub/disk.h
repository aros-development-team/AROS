/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002, 2003, 2004, 2005  Free Software Foundation, Inc.
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

#ifndef GRUB_DISK_HEADER
#define GRUB_DISK_HEADER	1

#include <grub/symbol.h>
#include <grub/err.h>
#include <grub/types.h>
#include <grub/device.h>

/* These are used to set a device id. When you add a new disk device,
   you must define a new id for it here.  */
enum grub_disk_dev_id
  {
    GRUB_DISK_DEVICE_BIOSDISK_ID,
    GRUB_DISK_DEVICE_OFDISK_ID,
    GRUB_DISK_DEVICE_LOOPBACK_ID
  };

struct grub_disk;

/* Disk device.  */
struct grub_disk_dev
{
  /* The device name.  */
  const char *name;

  /* The device id used by the cache manager.  */
  unsigned long id;
  
  /* Call HOOK with each device name, until HOOK returns non-zero.  */
  int (*iterate) (int (*hook) (const char *name));

  /* Open the device named NAME, and set up DISK.  */
  grub_err_t (*open) (const char *name, struct grub_disk *disk);

  /* Close the disk DISK.  */
  void (*close) (struct grub_disk *disk);

  /* Read SIZE sectors from the sector SECTOR of the disk DISK into BUF.  */
  grub_err_t (*read) (struct grub_disk *disk, unsigned long sector,
		      unsigned long size, char *buf);

  /* Write SIZE sectors from BUF into the sector SECTOR of the disk DISK.  */
  grub_err_t (*write) (struct grub_disk *disk, unsigned long sector,
		       unsigned long size, const char *buf);

  /* The next disk device.  */
  struct grub_disk_dev *next;
};
typedef struct grub_disk_dev *grub_disk_dev_t;

struct grub_partition;

/* Disk.  */
struct grub_disk
{
  /* The disk name.  */
  const char *name;

  /* The underlying disk device.  */
  grub_disk_dev_t dev;

  /* The total number of sectors.  */
  unsigned long total_sectors;

  /* If partitions can be stored.  */
  int has_partitions;

  /* The id used by the disk cache manager.  */
  unsigned long id;
  
  /* The partition information. This is machine-specific.  */
  struct grub_partition *partition;

  /* Called when a sector was read.  */
  void (*read_hook) (unsigned long sector, unsigned offset, unsigned length);

  /* Device-specific data.  */
  void *data;
};
typedef struct grub_disk *grub_disk_t;

/* The sector size.  */
#define GRUB_DISK_SECTOR_SIZE	0x200
#define GRUB_DISK_SECTOR_BITS	9

/* The maximum number of disk caches.  */
#define GRUB_DISK_CACHE_NUM	1021

/* The size of a disk cache in sector units.  */
#define GRUB_DISK_CACHE_SIZE	8
#define GRUB_DISK_CACHE_BITS	3

/* This is called from the memory manager.  */
void grub_disk_cache_invalidate_all (void);

void EXPORT_FUNC(grub_disk_dev_register) (grub_disk_dev_t dev);
void EXPORT_FUNC(grub_disk_dev_unregister) (grub_disk_dev_t dev);
int EXPORT_FUNC(grub_disk_dev_iterate) (int (*hook) (const char *name));

grub_disk_t EXPORT_FUNC(grub_disk_open) (const char *name);
void EXPORT_FUNC(grub_disk_close) (grub_disk_t disk);
grub_err_t EXPORT_FUNC(grub_disk_read) (grub_disk_t disk,
					unsigned long sector,
					unsigned long offset,
					unsigned long size,
					char *buf);
grub_err_t EXPORT_FUNC(grub_disk_write) (grub_disk_t disk,
					 unsigned long sector,
					 unsigned long offset,
					 unsigned long size,
					 const char *buf);


#endif /* ! GRUB_DISK_HEADER */
