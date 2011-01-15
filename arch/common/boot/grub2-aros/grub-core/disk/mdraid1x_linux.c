/* mdraid_linux.c - module to handle Linux Software RAID.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008,2009,2010  Free Software Foundation, Inc.
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

/* Linux RAID on disk structures and constants,
   copied from include/linux/raid/md_p.h.  */

#define SB_MAGIC			0xa92b4efc

/*
 * The version-1 superblock :
 * All numeric fields are little-endian.
 *
 * Total size: 256 bytes plus 2 per device.
 * 1K allows 384 devices.
 */

struct grub_raid_super_1x
{
  /* Constant array information - 128 bytes.  */
  grub_uint32_t magic;		/* MD_SB_MAGIC: 0xa92b4efc - little endian.  */
  grub_uint32_t major_version;	/* 1.  */
  grub_uint32_t feature_map;	/* Bit 0 set if 'bitmap_offset' is meaningful.   */
  grub_uint32_t pad0;		/* Always set to 0 when writing.  */

  grub_uint8_t set_uuid[16];	/* User-space generated.  */
  char set_name[32];		/* Set and interpreted by user-space.  */

  grub_uint64_t ctime;		/* Lo 40 bits are seconds, top 24 are microseconds or 0.  */
  grub_uint32_t level;		/* -4 (multipath), -1 (linear), 0,1,4,5.  */
  grub_uint32_t layout;		/* only for raid5 and raid10 currently.  */
  grub_uint64_t size;		/* Used size of component devices, in 512byte sectors.  */

  grub_uint32_t chunksize;	/* In 512byte sectors.  */
  grub_uint32_t raid_disks;
  grub_uint32_t bitmap_offset;	/* Sectors after start of superblock that bitmap starts
				 * NOTE: signed, so bitmap can be before superblock
				 * only meaningful of feature_map[0] is set.
				 */

  /* These are only valid with feature bit '4'.  */
  grub_uint32_t new_level;	/* New level we are reshaping to.  */
  grub_uint64_t reshape_position;	/* Next address in array-space for reshape.  */
  grub_uint32_t delta_disks;	/* Change in number of raid_disks.  */
  grub_uint32_t new_layout;	/* New layout.  */
  grub_uint32_t new_chunk;	/* New chunk size (512byte sectors).  */
  grub_uint8_t pad1[128 - 124];	/* Set to 0 when written.  */

  /* Constant this-device information - 64 bytes.  */
  grub_uint64_t data_offset;	/* Sector start of data, often 0.  */
  grub_uint64_t data_size;	/* Sectors in this device that can be used for data.  */
  grub_uint64_t super_offset;	/* Sector start of this superblock.  */
  grub_uint64_t recovery_offset;	/* Sectors before this offset (from data_offset) have been recovered.  */
  grub_uint32_t dev_number;	/* Permanent identifier of this  device - not role in raid.  */
  grub_uint32_t cnt_corrected_read;	/* Number of read errors that were corrected by re-writing.  */
  grub_uint8_t device_uuid[16];	/* User-space setable, ignored by kernel.  */
  grub_uint8_t devflags;	/* Per-device flags.  Only one defined...  */
  grub_uint8_t pad2[64 - 57];	/* Set to 0 when writing.  */

  /* Array state information - 64 bytes.  */
  grub_uint64_t utime;		/* 40 bits second, 24 btes microseconds.  */
  grub_uint64_t events;		/* Incremented when superblock updated.  */
  grub_uint64_t resync_offset;	/* Data before this offset (from data_offset) known to be in sync.  */
  grub_uint32_t sb_csum;	/* Checksum upto devs[max_dev].  */
  grub_uint32_t max_dev;	/* Size of devs[] array to consider.  */
  grub_uint8_t pad3[64 - 32];	/* Set to 0 when writing.  */

  /* Device state information. Indexed by dev_number.
   * 2 bytes per device.
   * Note there are no per-device state flags. State information is rolled
   * into the 'roles' value.  If a device is spare or faulty, then it doesn't
   * have a meaningful role.
   */
  grub_uint16_t dev_roles[0];	/* Role in array, or 0xffff for a spare, or 0xfffe for faulty.  */
};
/* Could be __attribute__ ((packed)), but since all members in this struct
   are already appropriately aligned, we can omit this and avoid suboptimal
   assembly in some cases.  */

#define WriteMostly1    1	/* Mask for writemostly flag in above devflags.  */

static grub_err_t
grub_mdraid_detect (grub_disk_t disk, struct grub_raid_array *array,
		    grub_disk_addr_t *start_sector)
{
  grub_disk_addr_t sector = 0;
  grub_uint64_t size;
  struct grub_raid_super_1x sb;
  grub_uint8_t minor_version;

  /* The sector where the mdraid 0.90 superblock is stored, if available.  */
  size = grub_disk_get_size (disk);

  /* Check for an 1.x superblock.
   * It's always aligned to a 4K boundary
   * and depending on the minor version it can be:
   * 0: At least 8K, but less than 12K, from end of device
   * 1: At start of device
   * 2: 4K from start of device.
   */

  for (minor_version = 0; minor_version < 3; ++minor_version)
    {
      if (size == GRUB_DISK_SIZE_UNKNOWN && minor_version == 0)
	continue;
	
      switch (minor_version)
	{
	case 0:
	  sector = (size - 8 * 2) & ~(4 * 2 - 1);
	  break;
	case 1:
	  sector = 0;
	  break;
	case 2:
	  sector = 4 * 2;
	  break;
	}

      if (grub_disk_read (disk, sector, 0, sizeof (struct grub_raid_super_1x),
			  &sb))
	return grub_errno;

      if (grub_le_to_cpu32 (sb.magic) != SB_MAGIC)
	continue;

      {
	grub_uint64_t sb_size;
	struct grub_raid_super_1x *real_sb;
	grub_uint32_t level;

	if (grub_le_to_cpu32 (sb.major_version) != 1)
	  return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
			     "Unsupported RAID version: %d",
			     grub_le_to_cpu32 (sb.major_version));

	level = grub_le_to_cpu32 (sb.level);

	/* Multipath.  */
	if ((int) level == -4)
	  level = 1;

	if (level != 0 && level != 1 && level != 4 &&
	    level != 5 && level != 6 && level != 10)
	  return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
			     "Unsupported RAID level: %d", sb.level);

	/* 1.x superblocks don't have a fixed size on disk.  So we have to
	   read it again now that we now the max device count.  */
	sb_size = sizeof (struct grub_raid_super_1x) 
	  + 2 * grub_le_to_cpu32 (sb.max_dev);
	real_sb = grub_malloc (sb_size);
	if (! real_sb)
	  return grub_errno;

	if (grub_disk_read (disk, sector, 0, sb_size, real_sb))
	  {
	    grub_free (real_sb);
	    return grub_errno;
	  }

	array->name = grub_strdup (real_sb->set_name);
	if (! array->name)
	  {
	    grub_free (real_sb);
	    return grub_errno;
	  }

	array->number = 0;
	array->level = grub_le_to_cpu32 (real_sb->level);
	array->layout = grub_le_to_cpu32 (real_sb->layout);
	array->total_devs = grub_le_to_cpu32 (real_sb->raid_disks);
	array->disk_size = grub_le_to_cpu64 (real_sb->size);
	array->chunk_size = grub_le_to_cpu32 (real_sb->chunksize);

	if (grub_le_to_cpu32 (real_sb->dev_number) >=
	    grub_le_to_cpu32 (real_sb->max_dev))
	  return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
			     "spares aren't implemented");

	array->index = grub_le_to_cpu16
	  (real_sb->dev_roles[grub_le_to_cpu32 (real_sb->dev_number)]);
	array->uuid_len = 16;
	array->uuid = grub_malloc (16);
	if (!array->uuid)
	  {
	    grub_free (real_sb);
	    return grub_errno;
	  }

	grub_memcpy (array->uuid, real_sb->set_uuid, 16);
	
	*start_sector = grub_le_to_cpu64 (real_sb->data_offset);

	grub_free (real_sb);
	return 0;
      }
    }

  return grub_error (GRUB_ERR_OUT_OF_RANGE, "not 1.x raid");
}

static struct grub_raid grub_mdraid_dev = {
  .name = "mdraid1x",
  .detect = grub_mdraid_detect,
  .next = 0
};

GRUB_MOD_INIT (mdraid1x)
{
  grub_raid_register (&grub_mdraid_dev);
}

GRUB_MOD_FINI (mdraid1x)
{
  grub_raid_unregister (&grub_mdraid_dev);
}
