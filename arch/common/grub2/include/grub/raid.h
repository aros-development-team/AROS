/* raid.h - On disk structures for RAID. */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2006,2007  Free Software Foundation, Inc.
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

#ifndef GRUB_RAID_H
#define GRUB_RAID_H	1

#include <grub/types.h>

struct grub_raid_array
{
  int number;              /* The device number, taken from md_minor so we
			      are consistent with the device name in
			      Linux. */
  int version;             /* 0 = 0.90, 1 = 1.0 */
  int level;               /* RAID levels, only 0, 1 or 5 at the moment. */
  int layout;              /* Only for RAID 5.  */
  unsigned int total_devs; /* Total number of devices in the array. */
  unsigned int nr_devs;    /* The number of devices we've found so far. */
  grub_size_t chunk_size; /* The size of a chunk, in 512 byte sectors. */
  grub_uint32_t uuid[4];   /* The UUID of the device. */
  char *name;              /* That will be "md<number>". */
  grub_uint64_t disk_size; /* Size of an individual disk, in 512 byte
			      sectors. */
  struct
  {
    char *name;            /* Name of the device */
    grub_disk_t disk;      /* The device itself. */
  } device[32];            /* Array of total_devs devices. */          
  struct grub_raid_array *next;
};

/* Linux RAID on disk structures and constants,
   copied from include/linux/raid/md_p.h.  */

#define GRUB_RAID_RESERVED_BYTES		(64 * 1024)
#define GRUB_RAID_RESERVED_SECTORS		(GRUB_RAID_RESERVED_BYTES / 512)

#define GRUB_RAID_NEW_SIZE_SECTORS(x)		((x & ~(GRUB_RAID_RESERVED_SECTORS - 1)) \
						 - GRUB_RAID_RESERVED_SECTORS)

#define GRUB_RAID_SB_BYTES			4096
#define GRUB_RAID_SB_WORDS			(GRUB_RAID_SB_BYTES / 4)
#define GRUB_RAID_SB_SECTORS			(GRUB_RAID_SB_BYTES / 512)

/*
 * The following are counted in 32-bit words
 */
#define	GRUB_RAID_SB_GENERIC_OFFSET		0

#define GRUB_RAID_SB_PERSONALITY_OFFSET	64
#define GRUB_RAID_SB_DISKS_OFFSET		128
#define GRUB_RAID_SB_DESCRIPTOR_OFFSET		992

#define GRUB_RAID_SB_GENERIC_CONSTANT_WORDS	32
#define GRUB_RAID_SB_GENERIC_STATE_WORDS	32
#define GRUB_RAID_SB_GENERIC_WORDS		(GRUB_RAID_SB_GENERIC_CONSTANT_WORDS \
						 + GRUB_RAID_SB_GENERIC_STATE_WORDS)
#define GRUB_RAID_SB_PERSONALITY_WORDS		64
#define GRUB_RAID_SB_DESCRIPTOR_WORDS		32
#define GRUB_RAID_SB_DISKS			27
#define GRUB_RAID_SB_DISKS_WORDS		(GRUB_RAID_SB_DISKS*GRUB_RAID_SB_DESCRIPTOR_WORDS)
#define GRUB_RAID_SB_RESERVED_WORDS		(1024 - GRUB_RAID_SB_GENERIC_WORDS \
						 - GRUB_RAID_SB_PERSONALITY_WORDS \
						 - GRUB_RAID_SB_DISKS_WORDS \
						 - GRUB_RAID_SB_DESCRIPTOR_WORDS)
#define GRUB_RAID_SB_EQUAL_WORDS		(GRUB_RAID_SB_GENERIC_WORDS \
						 + GRUB_RAID_SB_PERSONALITY_WORDS \
						 + GRUB_RAID_SB_DISKS_WORDS)

/*
 * Device "operational" state bits
 */
#define GRUB_RAID_DISK_FAULTY		0 /* disk is faulty / operational */
#define GRUB_RAID_DISK_ACTIVE		1 /* disk is running or spare disk */
#define GRUB_RAID_DISK_SYNC		2 /* disk is in sync with the raid set */
#define GRUB_RAID_DISK_REMOVED		3 /* disk is in sync with the raid set */

#define	GRUB_RAID_DISK_WRITEMOSTLY	9 /* disk is "write-mostly" is RAID1 config.
					   * read requests will only be sent here in
					   * dire need
					   */


#define GRUB_RAID_SB_MAGIC		0xa92b4efc

/*
 * Superblock state bits
 */
#define GRUB_RAID_SB_CLEAN		0
#define GRUB_RAID_SB_ERRORS		1

#define	GRUB_RAID_SB_BITMAP_PRESENT	8 /* bitmap may be present nearby */

struct grub_raid_disk_09 {
  grub_uint32_t number;		/* 0 Device number in the entire set	      */
  grub_uint32_t major;		/* 1 Device major number		      */
  grub_uint32_t minor;		/* 2 Device minor number		      */
  grub_uint32_t raid_disk;	/* 3 The role of the device in the raid set   */
  grub_uint32_t state;		/* 4 Operational state			      */
  grub_uint32_t reserved[GRUB_RAID_SB_DESCRIPTOR_WORDS - 5];
};

struct grub_raid_super_09 {
  /*
   * Constant generic information
   */
  grub_uint32_t md_magic;	/*  0 MD identifier 			      */
  grub_uint32_t major_version;	/*  1 major version to which the set conforms */
  grub_uint32_t minor_version;	/*  2 minor version ...			      */
  grub_uint32_t patch_version;	/*  3 patchlevel version ...		      */
  grub_uint32_t gvalid_words;	/*  4 Number of used words in this section    */
  grub_uint32_t set_uuid0;	/*  5 Raid set identifier		      */
  grub_uint32_t ctime;		/*  6 Creation time			      */
  grub_uint32_t level;		/*  7 Raid personality			      */
  grub_uint32_t size;		/*  8 Apparent size of each individual disk   */
  grub_uint32_t nr_disks;	/*  9 total disks in the raid set	      */
  grub_uint32_t raid_disks;	/* 10 disks in a fully functional raid set    */
  grub_uint32_t md_minor;	/* 11 preferred MD minor device number	      */
  grub_uint32_t not_persistent;	/* 12 does it have a persistent superblock    */
  grub_uint32_t set_uuid1;	/* 13 Raid set identifier #2		      */
  grub_uint32_t set_uuid2;	/* 14 Raid set identifier #3		      */
  grub_uint32_t set_uuid3;	/* 15 Raid set identifier #4		      */
  grub_uint32_t gstate_creserved[GRUB_RAID_SB_GENERIC_CONSTANT_WORDS - 16];

  /*
   * Generic state information
   */
  grub_uint32_t utime;		/*  0 Superblock update time		      */
  grub_uint32_t state;		/*  1 State bits (clean, ...)		      */
  grub_uint32_t active_disks;	/*  2 Number of currently active disks	      */
  grub_uint32_t working_disks;	/*  3 Number of working disks		      */
  grub_uint32_t failed_disks;	/*  4 Number of failed disks		      */
  grub_uint32_t spare_disks;	/*  5 Number of spare disks		      */
  grub_uint32_t sb_csum;	/*  6 checksum of the whole superblock        */
#ifdef GRUB_HOST_WORDS_BIGENDIAN
  grub_uint32_t events_hi;	/*  7 high-order of superblock update count   */
  grub_uint32_t events_lo;	/*  8 low-order of superblock update count    */
  grub_uint32_t cp_events_hi;	/*  9 high-order of checkpoint update count   */
  grub_uint32_t cp_events_lo;	/* 10 low-order of checkpoint update count    */
#else
  grub_uint32_t events_lo;	/*  7 low-order of superblock update count    */
  grub_uint32_t events_hi;	/*  8 high-order of superblock update count   */
  grub_uint32_t cp_events_lo;	/*  9 low-order of checkpoint update count    */
  grub_uint32_t cp_events_hi;	/* 10 high-order of checkpoint update count   */
#endif
  grub_uint32_t recovery_cp;	/* 11 recovery checkpoint sector count	      */
  grub_uint32_t gstate_sreserved[GRUB_RAID_SB_GENERIC_STATE_WORDS - 12];

  /*
   * Personality information
   */
  grub_uint32_t layout;		/*  0 the array's physical layout	      */
  grub_uint32_t chunk_size;	/*  1 chunk size in bytes		      */
  grub_uint32_t root_pv;	/*  2 LV root PV */
  grub_uint32_t root_block;	/*  3 LV root block */
  grub_uint32_t pstate_reserved[GRUB_RAID_SB_PERSONALITY_WORDS - 4];

  /*
   * Disks information
   */
  struct grub_raid_disk_09 disks[GRUB_RAID_SB_DISKS];

  /*
   * Reserved
   */
  grub_uint32_t reserved[GRUB_RAID_SB_RESERVED_WORDS];

  /*
   * Active descriptor
   */
  struct grub_raid_disk_09 this_disk;
};

#endif /* ! GRUB_RAID_H */
