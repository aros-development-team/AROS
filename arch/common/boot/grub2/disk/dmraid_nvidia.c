/* dmraid_nvidia.c - module to handle Nvidia fakeraid.  */
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

#include <grub/dl.h>
#include <grub/disk.h>
#include <grub/mm.h>
#include <grub/err.h>
#include <grub/misc.h>
#include <grub/raid.h>

#define NV_SIGNATURES		4

#define NV_IDLE			0
#define NV_SCDB_INIT_RAID	2
#define NV_SCDB_REBUILD_RAID	3
#define NV_SCDB_UPGRADE_RAID	4
#define NV_SCDB_SYNC_RAID	5

#define NV_LEVEL_UNKNOWN	0x00
#define NV_LEVEL_JBOD		0xFF
#define NV_LEVEL_0		0x80
#define NV_LEVEL_1		0x81
#define NV_LEVEL_3		0x83
#define NV_LEVEL_5		0x85
#define NV_LEVEL_10		0x8a
#define NV_LEVEL_1_0		0x8180

#define NV_ARRAY_FLAG_BOOT		1 /* BIOS use only.  */
#define NV_ARRAY_FLAG_ERROR		2 /* Degraded or offline.  */
#define NV_ARRAY_FLAG_PARITY_VALID	4 /* RAID-3/5 parity valid.  */

struct grub_nv_array
{
  grub_uint32_t version;
  grub_uint32_t signature[NV_SIGNATURES];
  grub_uint8_t raid_job_code;
  grub_uint8_t stripe_width;
  grub_uint8_t total_volumes;
  grub_uint8_t original_width;
  grub_uint32_t raid_level;
  grub_uint32_t stripe_block_size;
  grub_uint32_t stripe_block_size_bytes;
  grub_uint32_t stripe_block_size_log2;
  grub_uint32_t stripe_mask;
  grub_uint32_t stripe_size;
  grub_uint32_t stripe_size_bytes;
  grub_uint32_t raid_job_mask;
  grub_uint32_t original_capacity;
  grub_uint32_t flags;
};

#define NV_ID_LEN		8
#define NV_ID_STRING		"NVIDIA"
#define NV_VERSION		100

#define	NV_PRODID_LEN		16
#define	NV_PRODREV_LEN		4

struct grub_nv_super
{
  char vendor[NV_ID_LEN];	/* 0x00 - 0x07 ID string.  */
  grub_uint32_t size;		/* 0x08 - 0x0B Size of metadata in dwords.  */
  grub_uint32_t chksum;		/* 0x0C - 0x0F Checksum of this struct.  */
  grub_uint16_t version;	/* 0x10 - 0x11 NV version.  */
  grub_uint8_t unit_number;	/* 0x12 Disk index in array.  */
  grub_uint8_t reserved;	/* 0x13.  */
  grub_uint32_t capacity;	/* 0x14 - 0x17 Array capacity in sectors.  */
  grub_uint32_t sector_size;	/* 0x18 - 0x1B Sector size.  */
  char prodid[NV_PRODID_LEN];	/* 0x1C - 0x2B Array product ID.  */
  char prodrev[NV_PRODREV_LEN];	/* 0x2C - 0x2F Array product revision */
  grub_uint32_t unit_flags;	/* 0x30 - 0x33 Flags for this disk */
  struct grub_nv_array array;	/* Array information */
} __attribute__ ((packed));

static grub_err_t
grub_dmraid_nv_detect (grub_disk_t disk, struct grub_raid_array *array)
{
  grub_disk_addr_t sector;
  struct grub_nv_super sb;

  if (disk->partition)
    return grub_error (GRUB_ERR_OUT_OF_RANGE, "skip partition");

  sector = grub_disk_get_size (disk) - 2;

  if (grub_disk_read (disk, sector, 0, sizeof (sb), &sb))
    return grub_errno;

  if (grub_memcmp (sb.vendor, NV_ID_STRING, 6))
    return grub_error (GRUB_ERR_OUT_OF_RANGE, "not raid");

  if (sb.version != NV_VERSION)
    return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
                       "Unknown version: %d.%d", sb.version);

  switch (sb.array.raid_level)
    {
    case NV_LEVEL_0:
      array->level = 0;
      array->disk_size = sb.capacity / sb.array.total_volumes;
      break;

    case NV_LEVEL_1:
      array->level = 1;
      array->disk_size = sb.capacity;
      break;

    case NV_LEVEL_5:
      array->level = 5;
      array->layout = GRUB_RAID_LAYOUT_LEFT_ASYMMETRIC;
      array->disk_size = sb.capacity / (sb.array.total_volumes - 1);
      break;

    default:
      return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
                         "Unsupported RAID level: %d", sb.array.raid_level);
    }

  array->number = 0;
  array->total_devs = sb.array.total_volumes;
  array->chunk_size = sb.array.stripe_block_size;
  array->index = sb.unit_number;
  array->uuid_len = sizeof (sb.array.signature);
  array->uuid = grub_malloc (sizeof (sb.array.signature));
  if (! array->uuid)
    return grub_errno;

  grub_memcpy (array->uuid, (char *) &sb.array.signature,
               sizeof (sb.array.signature));

  return 0;
}

static struct grub_raid grub_dmraid_nv_dev =
{
  .name = "dmraid_nv",
  .detect = grub_dmraid_nv_detect,
  .next = 0
};

GRUB_MOD_INIT(dm_nv)
{
  grub_raid_register (&grub_dmraid_nv_dev);
}

GRUB_MOD_FINI(dm_nv)
{
  grub_raid_unregister (&grub_dmraid_nv_dev);
}
