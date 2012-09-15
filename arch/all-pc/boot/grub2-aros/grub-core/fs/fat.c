/* fat.c - FAT filesystem */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2000,2001,2002,2003,2004,2005,2007,2008,2009  Free Software Foundation, Inc.
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

#include <grub/fs.h>
#include <grub/disk.h>
#include <grub/file.h>
#include <grub/types.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/err.h>
#include <grub/dl.h>
#include <grub/charset.h>
#include <grub/fat.h>
#include <grub/i18n.h>

GRUB_MOD_LICENSE ("GPLv3+");

enum
  {
    GRUB_FAT_ATTR_READ_ONLY = 0x01,
    GRUB_FAT_ATTR_HIDDEN = 0x02,
    GRUB_FAT_ATTR_SYSTEM = 0x04,
#ifndef MODE_EXFAT
    GRUB_FAT_ATTR_VOLUME_ID = 0x08,
#endif
    GRUB_FAT_ATTR_DIRECTORY = 0x10,
    GRUB_FAT_ATTR_ARCHIVE = 0x20,

#ifndef MODE_EXFAT
    GRUB_FAT_ATTR_LONG_NAME = (GRUB_FAT_ATTR_READ_ONLY
			       | GRUB_FAT_ATTR_HIDDEN
			       | GRUB_FAT_ATTR_SYSTEM
			       | GRUB_FAT_ATTR_VOLUME_ID),
#endif
    GRUB_FAT_ATTR_VALID = (GRUB_FAT_ATTR_READ_ONLY
			   | GRUB_FAT_ATTR_HIDDEN
			   | GRUB_FAT_ATTR_SYSTEM
			   | GRUB_FAT_ATTR_DIRECTORY
			   | GRUB_FAT_ATTR_ARCHIVE
#ifndef MODE_EXFAT
			   | GRUB_FAT_ATTR_VOLUME_ID
#endif
			   )
  };

#ifdef MODE_EXFAT
struct grub_exfat_bpb
{
  grub_uint8_t jmp_boot[3];
  grub_uint8_t oem_name[8];
  grub_uint8_t mbz[53];
  grub_uint64_t num_hidden_sectors;
  grub_uint64_t num_total_sectors;
  grub_uint32_t num_reserved_sectors;
  grub_uint32_t sectors_per_fat;
  grub_uint32_t cluster_offset;
  grub_uint32_t cluster_count;
  grub_uint32_t root_cluster;
  grub_uint32_t num_serial;
  grub_uint16_t fs_revision;
  grub_uint16_t volume_flags;
  grub_uint8_t bytes_per_sector_shift;
  grub_uint8_t sectors_per_cluster_shift;
  grub_uint8_t num_fats;
  grub_uint8_t num_ph_drive;
  grub_uint8_t reserved[8];
} __attribute__ ((packed));
typedef struct grub_exfat_bpb grub_current_fat_bpb_t;
#else
typedef struct grub_fat_bpb grub_current_fat_bpb_t;
#endif

#ifdef MODE_EXFAT
struct grub_fat_dir_entry
{
  grub_uint8_t entry_type;
  union
  {
    grub_uint8_t placeholder[31];
    struct {
      grub_uint8_t secondary_count;
      grub_uint16_t checksum;
      grub_uint16_t attr;
      grub_uint16_t reserved1;
      grub_uint32_t c_time;
      grub_uint32_t m_time;
      grub_uint32_t a_time;
      grub_uint8_t c_time_tenth;
      grub_uint8_t m_time_tenth;
      grub_uint8_t a_time_tenth;
      grub_uint8_t reserved2[9];
    }  __attribute__ ((packed)) file;
    struct {
      grub_uint8_t flags;
      grub_uint8_t reserved1;
      grub_uint8_t name_length;
      grub_uint16_t name_hash;
      grub_uint16_t reserved2;
      grub_uint64_t valid_size;
      grub_uint32_t reserved3;
      grub_uint32_t first_cluster;
      grub_uint64_t file_size;
    }   __attribute__ ((packed)) stream_extension;
    struct {
      grub_uint8_t flags;
      grub_uint16_t str[15];
    }  __attribute__ ((packed))  file_name;
    struct {
      grub_uint8_t character_count;
      grub_uint16_t str[15];
    }  __attribute__ ((packed))  volume_label;
  }  __attribute__ ((packed)) type_specific;
} __attribute__ ((packed));

struct grub_fat_dir_node
{
  grub_uint32_t attr;
  grub_uint32_t first_cluster;
  grub_uint64_t file_size;
  grub_uint64_t valid_size;
  int have_stream;
  int is_label;
};

typedef struct grub_fat_dir_node grub_fat_dir_node_t;

#else
struct grub_fat_dir_entry
{
  grub_uint8_t name[11];
  grub_uint8_t attr;
  grub_uint8_t nt_reserved;
  grub_uint8_t c_time_tenth;
  grub_uint16_t c_time;
  grub_uint16_t c_date;
  grub_uint16_t a_date;
  grub_uint16_t first_cluster_high;
  grub_uint16_t w_time;
  grub_uint16_t w_date;
  grub_uint16_t first_cluster_low;
  grub_uint32_t file_size;
} __attribute__ ((packed));

struct grub_fat_long_name_entry
{
  grub_uint8_t id;
  grub_uint16_t name1[5];
  grub_uint8_t attr;
  grub_uint8_t reserved;
  grub_uint8_t checksum;
  grub_uint16_t name2[6];
  grub_uint16_t first_cluster;
  grub_uint16_t name3[2];
} __attribute__ ((packed));

typedef struct grub_fat_dir_entry grub_fat_dir_node_t;

#endif

struct grub_fat_data
{
  int logical_sector_bits;
  grub_uint32_t num_sectors;

  grub_uint32_t fat_sector;
  grub_uint32_t sectors_per_fat;
  int fat_size;

  grub_uint32_t root_cluster;
#ifndef MODE_EXFAT
  grub_uint32_t root_sector;
  grub_uint32_t num_root_sectors;
#endif

  int cluster_bits;
  grub_uint32_t cluster_eof_mark;
  grub_uint32_t cluster_sector;
  grub_uint32_t num_clusters;

  grub_uint8_t attr;
  grub_ssize_t file_size;
  grub_uint32_t file_cluster;
  grub_uint32_t cur_cluster_num;
  grub_uint32_t cur_cluster;

  grub_uint32_t uuid;
};

static grub_dl_t my_mod;

#ifndef MODE_EXFAT
static int
fat_log2 (unsigned x)
{
  int i;

  if (x == 0)
    return -1;

  for (i = 0; (x & 1) == 0; i++)
    x >>= 1;

  if (x != 1)
    return -1;

  return i;
}
#endif

static struct grub_fat_data *
grub_fat_mount (grub_disk_t disk)
{
  grub_current_fat_bpb_t bpb;
  struct grub_fat_data *data = 0;
  grub_uint32_t first_fat, magic;

  if (! disk)
    goto fail;

  data = (struct grub_fat_data *) grub_malloc (sizeof (*data));
  if (! data)
    goto fail;

  /* Read the BPB.  */
  if (grub_disk_read (disk, 0, 0, sizeof (bpb), &bpb))
    goto fail;

#ifdef MODE_EXFAT
  if (grub_memcmp ((const char *) bpb.oem_name, "EXFAT   ",
		   sizeof (bpb.oem_name)) != 0)
    goto fail;    
#endif

  /* Get the sizes of logical sectors and clusters.  */
#ifdef MODE_EXFAT
  data->logical_sector_bits = bpb.bytes_per_sector_shift;
#else
  data->logical_sector_bits =
    fat_log2 (grub_le_to_cpu16 (bpb.bytes_per_sector));
#endif
  if (data->logical_sector_bits < GRUB_DISK_SECTOR_BITS
      || data->logical_sector_bits >= 16)
    goto fail;
  data->logical_sector_bits -= GRUB_DISK_SECTOR_BITS;

#ifdef MODE_EXFAT
  data->cluster_bits = bpb.sectors_per_cluster_shift;
#else
  data->cluster_bits = fat_log2 (bpb.sectors_per_cluster);
#endif
  if (data->cluster_bits < 0 || data->cluster_bits > 25)
    goto fail;
  data->cluster_bits += data->logical_sector_bits;

  /* Get information about FATs.  */
#ifdef MODE_EXFAT
  data->fat_sector = (grub_le_to_cpu32 (bpb.num_reserved_sectors)
		      << data->logical_sector_bits);
#else
  data->fat_sector = (grub_le_to_cpu16 (bpb.num_reserved_sectors)
		      << data->logical_sector_bits);
#endif
  if (data->fat_sector == 0)
    goto fail;

#ifdef MODE_EXFAT
  data->sectors_per_fat = (grub_le_to_cpu32 (bpb.sectors_per_fat)
			   << data->logical_sector_bits);
#else
  data->sectors_per_fat = ((bpb.sectors_per_fat_16
			    ? grub_le_to_cpu16 (bpb.sectors_per_fat_16)
			    : grub_le_to_cpu32 (bpb.version_specific.fat32.sectors_per_fat_32))
			   << data->logical_sector_bits);
#endif
  if (data->sectors_per_fat == 0)
    goto fail;

  /* Get the number of sectors in this volume.  */
#ifdef MODE_EXFAT
  data->num_sectors = ((grub_le_to_cpu64 (bpb.num_total_sectors))
		       << data->logical_sector_bits);
#else
  data->num_sectors = ((bpb.num_total_sectors_16
			? grub_le_to_cpu16 (bpb.num_total_sectors_16)
			: grub_le_to_cpu32 (bpb.num_total_sectors_32))
		       << data->logical_sector_bits);
#endif
  if (data->num_sectors == 0)
    goto fail;

  /* Get information about the root directory.  */
  if (bpb.num_fats == 0)
    goto fail;

#ifndef MODE_EXFAT
  data->root_sector = data->fat_sector + bpb.num_fats * data->sectors_per_fat;
  data->num_root_sectors
    = ((((grub_uint32_t) grub_le_to_cpu16 (bpb.num_root_entries)
	 * sizeof (struct grub_fat_dir_entry)
	 + grub_le_to_cpu16 (bpb.bytes_per_sector) - 1)
	>> (data->logical_sector_bits + GRUB_DISK_SECTOR_BITS))
       << (data->logical_sector_bits));
#endif

#ifdef MODE_EXFAT
  data->cluster_sector = (grub_le_to_cpu32 (bpb.cluster_offset) 
			  << data->logical_sector_bits);
  data->num_clusters = (grub_le_to_cpu32 (bpb.cluster_count)
			  << data->logical_sector_bits);
#else
  data->cluster_sector = data->root_sector + data->num_root_sectors;
  data->num_clusters = (((data->num_sectors - data->cluster_sector)
			 >> data->cluster_bits)
			+ 2);
#endif

  if (data->num_clusters <= 2)
    goto fail;

#ifdef MODE_EXFAT
  {
    /* exFAT.  */
    grub_uint16_t flags = grub_le_to_cpu16 (bpb.volume_flags);

    data->root_cluster = grub_le_to_cpu32 (bpb.root_cluster);
    data->fat_size = 32;
    data->cluster_eof_mark = 0xffffffff;

    if ((flags & 0x1) && bpb.num_fats > 1)
      data->fat_sector += data->sectors_per_fat;
  }
#else
  if (! bpb.sectors_per_fat_16)
    {
      /* FAT32.  */
      grub_uint16_t flags = grub_le_to_cpu16 (bpb.version_specific.fat32.extended_flags);

      data->root_cluster = grub_le_to_cpu32 (bpb.version_specific.fat32.root_cluster);
      data->fat_size = 32;
      data->cluster_eof_mark = 0x0ffffff8;

      if (flags & 0x80)
	{
	  /* Get an active FAT.  */
	  unsigned active_fat = flags & 0xf;

	  if (active_fat > bpb.num_fats)
	    goto fail;

	  data->fat_sector += active_fat * data->sectors_per_fat;
	}

      if (bpb.num_root_entries != 0 || bpb.version_specific.fat32.fs_version != 0)
	goto fail;
    }
  else
    {
      /* FAT12 or FAT16.  */
      data->root_cluster = ~0U;

      if (data->num_clusters <= 4085 + 2)
	{
	  /* FAT12.  */
	  data->fat_size = 12;
	  data->cluster_eof_mark = 0x0ff8;
	}
      else
	{
	  /* FAT16.  */
	  data->fat_size = 16;
	  data->cluster_eof_mark = 0xfff8;
	}
    }
#endif

  /* More sanity checks.  */
  if (data->num_sectors <= data->fat_sector)
    goto fail;

  if (grub_disk_read (disk,
		      data->fat_sector,
		      0,
		      sizeof (first_fat),
		      &first_fat))
    goto fail;

  first_fat = grub_le_to_cpu32 (first_fat);

  if (data->fat_size == 32)
    {
      first_fat &= 0x0fffffff;
      magic = 0x0fffff00;
    }
  else if (data->fat_size == 16)
    {
      first_fat &= 0x0000ffff;
      magic = 0xff00;
    }
  else
    {
      first_fat &= 0x00000fff;
      magic = 0x0f00;
    }

  /* Serial number.  */
#ifdef MODE_EXFAT
    data->uuid = grub_le_to_cpu32 (bpb.num_serial);
#else
  if (bpb.sectors_per_fat_16)
    data->uuid = grub_le_to_cpu32 (bpb.version_specific.fat12_or_fat16.num_serial);
  else
    data->uuid = grub_le_to_cpu32 (bpb.version_specific.fat32.num_serial);
#endif

#ifndef MODE_EXFAT
  /* Ignore the 3rd bit, because some BIOSes assigns 0xF0 to the media
     descriptor, even if it is a so-called superfloppy (e.g. an USB key).
     The check may be too strict for this kind of stupid BIOSes, as
     they overwrite the media descriptor.  */
  if ((first_fat | 0x8) != (magic | bpb.media | 0x8))
    goto fail;
#else
  (void) magic;
#endif

  /* Start from the root directory.  */
  data->file_cluster = data->root_cluster;
  data->cur_cluster_num = ~0U;
  data->attr = GRUB_FAT_ATTR_DIRECTORY;
  return data;

 fail:

  grub_free (data);
  grub_error (GRUB_ERR_BAD_FS, "not a FAT filesystem");
  return 0;
}

static grub_ssize_t
grub_fat_read_data (grub_disk_t disk, struct grub_fat_data *data,
		    void NESTED_FUNC_ATTR (*read_hook) (grub_disk_addr_t sector,
				       unsigned offset, unsigned length),
		    grub_off_t offset, grub_size_t len, char *buf)
{
  grub_size_t size;
  grub_uint32_t logical_cluster;
  unsigned logical_cluster_bits;
  grub_ssize_t ret = 0;
  unsigned long sector;

#ifndef MODE_EXFAT
  /* This is a special case. FAT12 and FAT16 doesn't have the root directory
     in clusters.  */
  if (data->file_cluster == ~0U)
    {
      size = (data->num_root_sectors << GRUB_DISK_SECTOR_BITS) - offset;
      if (size > len)
	size = len;

      if (grub_disk_read (disk, data->root_sector, offset, size, buf))
	return -1;

      return size;
    }
#endif

  /* Calculate the logical cluster number and offset.  */
  logical_cluster_bits = (data->cluster_bits
			  + GRUB_DISK_SECTOR_BITS);
  logical_cluster = offset >> logical_cluster_bits;
  offset &= (1ULL << logical_cluster_bits) - 1;

  if (logical_cluster < data->cur_cluster_num)
    {
      data->cur_cluster_num = 0;
      data->cur_cluster = data->file_cluster;
    }

  while (len)
    {
      while (logical_cluster > data->cur_cluster_num)
	{
	  /* Find next cluster.  */
	  grub_uint32_t next_cluster;
	  unsigned long fat_offset;

	  switch (data->fat_size)
	    {
	    case 32:
	      fat_offset = data->cur_cluster << 2;
	      break;
	    case 16:
	      fat_offset = data->cur_cluster << 1;
	      break;
	    default:
	      /* case 12: */
	      fat_offset = data->cur_cluster + (data->cur_cluster >> 1);
	      break;
	    }

	  /* Read the FAT.  */
	  if (grub_disk_read (disk, data->fat_sector, fat_offset,
			      (data->fat_size + 7) >> 3,
			      (char *) &next_cluster))
	    return -1;

	  next_cluster = grub_le_to_cpu32 (next_cluster);
	  switch (data->fat_size)
	    {
	    case 16:
	      next_cluster &= 0xFFFF;
	      break;
	    case 12:
	      if (data->cur_cluster & 1)
		next_cluster >>= 4;

	      next_cluster &= 0x0FFF;
	      break;
	    }

	  grub_dprintf ("fat", "fat_size=%d, next_cluster=%u\n",
			data->fat_size, next_cluster);

	  /* Check the end.  */
	  if (next_cluster >= data->cluster_eof_mark)
	    return ret;

	  if (next_cluster < 2 || next_cluster >= data->num_clusters)
	    {
	      grub_error (GRUB_ERR_BAD_FS, "invalid cluster %u",
			  next_cluster);
	      return -1;
	    }

	  data->cur_cluster = next_cluster;
	  data->cur_cluster_num++;
	}

      /* Read the data here.  */
      sector = (data->cluster_sector
		+ ((data->cur_cluster - 2)
		   << data->cluster_bits));
      size = (1 << logical_cluster_bits) - offset;
      if (size > len)
	size = len;

      disk->read_hook = read_hook;
      grub_disk_read (disk, sector, offset, size, buf);
      disk->read_hook = 0;
      if (grub_errno)
	return -1;

      len -= size;
      buf += size;
      ret += size;
      logical_cluster++;
      offset = 0;
    }

  return ret;
}

struct grub_fat_iterate_context
{
#ifdef MODE_EXFAT
  struct grub_fat_dir_node dir;
#else
  struct grub_fat_dir_entry dir;
#endif
  char *filename;
  grub_uint16_t *unibuf;
  grub_ssize_t offset;
};

static grub_err_t
grub_fat_iterate_init (struct grub_fat_iterate_context *ctxt)
{
  ctxt->offset = -sizeof (struct grub_fat_dir_entry);

#ifndef MODE_EXFAT
  /* Allocate space enough to hold a long name.  */
  ctxt->filename = grub_malloc (0x40 * 13 * GRUB_MAX_UTF8_PER_UTF16 + 1);
  ctxt->unibuf = (grub_uint16_t *) grub_malloc (0x40 * 13 * 2);
#else
  ctxt->unibuf = grub_malloc (15 * 256 * 2);
  ctxt->filename = grub_malloc (15 * 256 * GRUB_MAX_UTF8_PER_UTF16 + 1);
#endif

  if (! ctxt->filename || ! ctxt->unibuf)
    {
      grub_free (ctxt->filename);
      grub_free (ctxt->unibuf);
      return grub_errno;
    }
  return GRUB_ERR_NONE;
}

static void
grub_fat_iterate_fini (struct grub_fat_iterate_context *ctxt)
{
  grub_free (ctxt->filename);
  grub_free (ctxt->unibuf);
}

#ifdef MODE_EXFAT
static grub_err_t
grub_fat_iterate_dir_next (grub_disk_t disk, struct grub_fat_data *data,
			   struct grub_fat_iterate_context *ctxt)
{
  grub_memset (&ctxt->dir, 0, sizeof (ctxt->dir));
  while (1)
    {
      struct grub_fat_dir_entry dir;

      ctxt->offset += sizeof (dir);

      if (grub_fat_read_data (disk, data, 0, ctxt->offset, sizeof (dir),
			      (char *) &dir)
	   != sizeof (dir))
	break;

      if (dir.entry_type == 0)
	break;
      if (!(dir.entry_type & 0x80))
	continue;

      if (dir.entry_type == 0x85)
	{
	  unsigned i, nsec, slots = 0;

	  nsec = dir.type_specific.file.secondary_count;

	  ctxt->dir.attr = grub_cpu_to_le16 (dir.type_specific.file.attr);
	  ctxt->dir.have_stream = 0;
	  for (i = 0; i < nsec; i++)
	    {
	      struct grub_fat_dir_entry sec;
	      ctxt->offset += sizeof (sec);
	      if (grub_fat_read_data (disk, data, 0,
				      ctxt->offset, sizeof (sec), (char *) &sec)
		  != sizeof (sec))
		break;
	      if (!(sec.entry_type & 0x80))
		continue;
	      if (!(sec.entry_type & 0x40))
		break;
	      switch (sec.entry_type)
		{
		case 0xc0:
		  ctxt->dir.first_cluster = grub_cpu_to_le32 (sec.type_specific.stream_extension.first_cluster);
		  ctxt->dir.valid_size
		    = grub_cpu_to_le64 (sec.type_specific.stream_extension.valid_size);
		  ctxt->dir.file_size
		    = grub_cpu_to_le64 (sec.type_specific.stream_extension.file_size);
		  ctxt->dir.have_stream = 1;
		  break;
		case 0xc1:
		  {
		    int j;
		    for (j = 0; j < 15; j++)
		      ctxt->unibuf[slots * 15 + j] 
			= grub_le_to_cpu16 (sec.type_specific.file_name.str[j]);
		    slots++;
		  }
		  break;
		default:
		  grub_dprintf ("exfat", "unknown secondary type 0x%02x\n",
				sec.entry_type);
		}
	    }

	  if (i != nsec)
	    {
	      ctxt->offset -= sizeof (dir);
	      continue;
	    }

	  *grub_utf16_to_utf8 ((grub_uint8_t *) ctxt->filename, ctxt->unibuf,
			       slots * 15) = '\0';

	  return 0;
	}
      /* Allocation bitmap. */
      if (dir.entry_type == 0x81)
	continue;
      /* Upcase table. */
      if (dir.entry_type == 0x82)
	continue;
      /* Volume label. */
      if (dir.entry_type == 0x83)
	continue;
      grub_dprintf ("exfat", "unknown primary type 0x%02x\n",
		    dir.entry_type);
    }
  return grub_errno ? : GRUB_ERR_EOF;
}

#else

static grub_err_t
grub_fat_iterate_dir_next (grub_disk_t disk, struct grub_fat_data *data,
			   struct grub_fat_iterate_context *ctxt)
{
  char *filep = 0;
  int checksum = -1;
  int slot = -1, slots = -1;

  while (1)
    {
      unsigned i;

      /* Adjust the offset.  */
      ctxt->offset += sizeof (ctxt->dir);

      /* Read a directory entry.  */
      if (grub_fat_read_data (disk, data, 0,
			      ctxt->offset, sizeof (ctxt->dir),
			      (char *) &ctxt->dir)
	   != sizeof (ctxt->dir) || ctxt->dir.name[0] == 0)
	break;

      /* Handle long name entries.  */
      if (ctxt->dir.attr == GRUB_FAT_ATTR_LONG_NAME)
	{
	  struct grub_fat_long_name_entry *long_name
	    = (struct grub_fat_long_name_entry *) &ctxt->dir;
	  grub_uint8_t id = long_name->id;

	  if (id & 0x40)
	    {
	      id &= 0x3f;
	      slots = slot = id;
	      checksum = long_name->checksum;
	    }

	  if (id != slot || slot == 0 || checksum != long_name->checksum)
	    {
	      checksum = -1;
	      continue;
	    }

	  slot--;
	  grub_memcpy (ctxt->unibuf + slot * 13, long_name->name1, 5 * 2);
	  grub_memcpy (ctxt->unibuf + slot * 13 + 5, long_name->name2, 6 * 2);
	  grub_memcpy (ctxt->unibuf + slot * 13 + 11, long_name->name3, 2 * 2);
	  continue;
	}

      /* Check if this entry is valid.  */
      if (ctxt->dir.name[0] == 0xe5 || (ctxt->dir.attr & ~GRUB_FAT_ATTR_VALID))
	continue;

      /* This is a workaround for Japanese.  */
      if (ctxt->dir.name[0] == 0x05)
	ctxt->dir.name[0] = 0xe5;

      if (checksum != -1 && slot == 0)
	{
	  grub_uint8_t sum;

	  for (sum = 0, i = 0; i < sizeof (ctxt->dir.name); i++)
	    sum = ((sum >> 1) | (sum << 7)) + ctxt->dir.name[i];

	  if (sum == checksum)
	    {
	      int u;

	      for (u = 0; u < slots * 13; u++)
		ctxt->unibuf[u] = grub_le_to_cpu16 (ctxt->unibuf[u]);

	      *grub_utf16_to_utf8 ((grub_uint8_t *) ctxt->filename,
				   ctxt->unibuf,
				   slots * 13) = '\0';

	      return GRUB_ERR_NONE;
	    }

	  checksum = -1;
	}

      /* Convert the 8.3 file name.  */
      filep = ctxt->filename;
      if (ctxt->dir.attr & GRUB_FAT_ATTR_VOLUME_ID)
	{
	  for (i = 0; i < sizeof (ctxt->dir.name) && ctxt->dir.name[i]; i++)
	    *filep++ = ctxt->dir.name[i];
	  while (i > 0 && ctxt->dir.name[i - 1] == ' ')
	    {
	      filep--;
	      i--;
	    }
	}
      else
	{
	  for (i = 0; i < 8 && ctxt->dir.name[i]; i++)
	    *filep++ = grub_tolower (ctxt->dir.name[i]);
	  while (i > 0 && ctxt->dir.name[i - 1] == ' ')
	    {
	      filep--;
	      i--;
	    }

	  *filep++ = '.';

	  for (i = 8; i < 11 && ctxt->dir.name[i]; i++)
	    *filep++ = grub_tolower (ctxt->dir.name[i]);
	  while (i > 8 && ctxt->dir.name[i - 1] == ' ')
	    {
	      filep--;
	      i--;
	    }

	  if (i == 8)
	    filep--;
	}
      *filep = '\0';
      return GRUB_ERR_NONE;
    }

  return grub_errno ? : GRUB_ERR_EOF;
}

#endif

/* Find the underlying directory or file in PATH and return the
   next path. If there is no next path or an error occurs, return NULL.
   If HOOK is specified, call it with each file name.  */
static char *
grub_fat_find_dir (grub_disk_t disk, struct grub_fat_data *data,
		   const char *path, const char *origpath,
		   int (*hook) (const char *filename,
				const struct grub_dirhook_info *info))
{
  char *dirname, *dirp;
  int call_hook;
  int found = 0;
  struct grub_fat_iterate_context ctxt;
  grub_err_t err;

  if (! (data->attr & GRUB_FAT_ATTR_DIRECTORY))
    {
      grub_error (GRUB_ERR_BAD_FILE_TYPE, N_("not a directory"));
      return 0;
    }

  /* Extract a directory name.  */
  while (*path == '/')
    path++;

  dirp = grub_strchr (path, '/');
  if (dirp)
    {
      unsigned len = dirp - path;

      dirname = grub_malloc (len + 1);
      if (! dirname)
	goto fail;

      grub_memcpy (dirname, path, len);
      dirname[len] = '\0';
    }
  else
    /* This is actually a file.  */
    dirname = grub_strdup (path);

  call_hook = (! dirp && hook);

  err = grub_fat_iterate_init (&ctxt);
  if (err)
    {
      grub_free (dirname);
      return 0;
    }

  while (!(err = grub_fat_iterate_dir_next (disk, data, &ctxt)))
    {
      struct grub_dirhook_info info;
      grub_memset (&info, 0, sizeof (info));

      info.dir = !! (ctxt.dir.attr & GRUB_FAT_ATTR_DIRECTORY);
      info.case_insensitive = 1;

#ifdef MODE_EXFAT
      if (!ctxt.dir.have_stream)
	continue;
#else
      if (ctxt.dir.attr & GRUB_FAT_ATTR_VOLUME_ID)
	continue;
#endif
      if (*dirname == '\0' && call_hook)
	{
	  if (hook (ctxt.filename, &info))
	    break;
	  else
	    continue;
	}

      if (grub_strcasecmp (dirname, ctxt.filename) == 0)
	{
	  found = 1;
	  data->attr = ctxt.dir.attr;
#ifdef MODE_EXFAT
	  data->file_size = ctxt.dir.file_size;
	  data->file_cluster = ctxt.dir.first_cluster;
#else
	  data->file_size = grub_le_to_cpu32 (ctxt.dir.file_size);
	  data->file_cluster = ((grub_le_to_cpu16 (ctxt.dir.first_cluster_high) << 16)
				| grub_le_to_cpu16 (ctxt.dir.first_cluster_low));
#endif
	  data->cur_cluster_num = ~0U;

	  if (call_hook)
	    hook (ctxt.filename, &info);

	  break;
	}
    }

  grub_fat_iterate_fini (&ctxt);
  if (err == GRUB_ERR_EOF)
    err = 0;

  if (grub_errno == GRUB_ERR_NONE && ! found && !call_hook)
    grub_error (GRUB_ERR_FILE_NOT_FOUND, N_("file `%s' not found"), origpath);

 fail:
  grub_free (dirname);

  return found ? dirp : 0;
}

static grub_err_t
grub_fat_dir (grub_device_t device, const char *path,
	      int (*hook) (const char *filename,
			   const struct grub_dirhook_info *info))
{
  struct grub_fat_data *data = 0;
  grub_disk_t disk = device->disk;
  grub_size_t len;
  char *dirname = 0;
  char *p;

  grub_dl_ref (my_mod);

  data = grub_fat_mount (disk);
  if (! data)
    goto fail;

  /* Make sure that DIRNAME terminates with '/'.  */
  len = grub_strlen (path);
  dirname = grub_malloc (len + 1 + 1);
  if (! dirname)
    goto fail;
  grub_memcpy (dirname, path, len);
  p = dirname + len;
  if (path[len - 1] != '/')
    *p++ = '/';
  *p = '\0';
  p = dirname;

  do
    {
      p = grub_fat_find_dir (disk, data, p, path, hook);
    }
  while (p && grub_errno == GRUB_ERR_NONE);

 fail:

  grub_free (dirname);
  grub_free (data);

  grub_dl_unref (my_mod);

  return grub_errno;
}

static grub_err_t
grub_fat_open (grub_file_t file, const char *name)
{
  struct grub_fat_data *data = 0;
  char *p = (char *) name;

  grub_dl_ref (my_mod);

  data = grub_fat_mount (file->device->disk);
  if (! data)
    goto fail;

  do
    {
      p = grub_fat_find_dir (file->device->disk, data, p, name, 0);
      if (grub_errno != GRUB_ERR_NONE)
	goto fail;
    }
  while (p);

  if (data->attr & GRUB_FAT_ATTR_DIRECTORY)
    {
      grub_error (GRUB_ERR_BAD_FILE_TYPE, N_("not a regular file"));
      goto fail;
    }

  file->data = data;
  file->size = data->file_size;

  return GRUB_ERR_NONE;

 fail:

  grub_free (data);

  grub_dl_unref (my_mod);

  return grub_errno;
}

static grub_ssize_t
grub_fat_read (grub_file_t file, char *buf, grub_size_t len)
{
  return grub_fat_read_data (file->device->disk, file->data, file->read_hook,
			     file->offset, len, buf);
}

static grub_err_t
grub_fat_close (grub_file_t file)
{
  grub_free (file->data);

  grub_dl_unref (my_mod);

  return grub_errno;
}

#ifdef MODE_EXFAT
static grub_err_t
grub_fat_label (grub_device_t device, char **label)
{
  struct grub_fat_dir_entry dir;
  grub_ssize_t offset = -sizeof(dir);
  struct grub_fat_data *data;
  grub_disk_t disk = device->disk;

  data = grub_fat_mount (disk);
  if (! data)
    return grub_errno;

  *label = NULL;

  while (1)
    {
      offset += sizeof (dir);

      if (grub_fat_read_data (disk, data, 0,
			       offset, sizeof (dir), (char *) &dir)
	   != sizeof (dir))
	break;

      if (dir.entry_type == 0)
	break;
      if (!(dir.entry_type & 0x80))
	continue;

      /* Volume label. */
      if (dir.entry_type == 0x83)
	{
	  grub_size_t chc;
	  grub_uint16_t t[ARRAY_SIZE (dir.type_specific.volume_label.str)];
	  grub_size_t i;
	  *label = grub_malloc (ARRAY_SIZE (dir.type_specific.volume_label.str)
				* GRUB_MAX_UTF8_PER_UTF16 + 1);
	  if (!*label)
	    {
	      grub_free (data);
	      return grub_errno;
	    }
	  chc = dir.type_specific.volume_label.character_count;
	  if (chc > ARRAY_SIZE (dir.type_specific.volume_label.str))
	    chc = ARRAY_SIZE (dir.type_specific.volume_label.str);
	  for (i = 0; i < chc; i++)
	    t[i] = grub_le_to_cpu16 (dir.type_specific.volume_label.str[i]);
	  *grub_utf16_to_utf8 ((grub_uint8_t *) *label, t, chc) = '\0';
	}
    }

  grub_free (data);
  return grub_errno;
}

#else

static grub_err_t
grub_fat_label (grub_device_t device, char **label)
{
  struct grub_fat_data *data;
  grub_disk_t disk = device->disk;
  grub_err_t err;
  struct grub_fat_iterate_context ctxt;

  *label = 0;

  grub_dl_ref (my_mod);

  data = grub_fat_mount (disk);
  if (! data)
    goto fail;

  if (! (data->attr & GRUB_FAT_ATTR_DIRECTORY))
    {
      grub_error (GRUB_ERR_BAD_FILE_TYPE, N_("not a directory"));
      return 0;
    }

  err = grub_fat_iterate_init (&ctxt);
  if (err)
    goto fail;

  while (!(err = grub_fat_iterate_dir_next (disk, data, &ctxt)))
    if ((ctxt.dir.attr & ~GRUB_FAT_ATTR_ARCHIVE) == GRUB_FAT_ATTR_VOLUME_ID)
      {
	*label = grub_strdup (ctxt.filename);
	break;
      }

  grub_fat_iterate_fini (&ctxt);

 fail:

  grub_dl_unref (my_mod);

  grub_free (data);

  return grub_errno;
}

#endif

static grub_err_t
grub_fat_uuid (grub_device_t device, char **uuid)
{
  struct grub_fat_data *data;
  grub_disk_t disk = device->disk;

  grub_dl_ref (my_mod);

  data = grub_fat_mount (disk);
  if (data)
    {
      char *ptr;
      *uuid = grub_xasprintf ("%04x-%04x",
			     (grub_uint16_t) (data->uuid >> 16),
			     (grub_uint16_t) data->uuid);
      for (ptr = *uuid; ptr && *ptr; ptr++)
	*ptr = grub_toupper (*ptr);
    }
  else
    *uuid = NULL;

  grub_dl_unref (my_mod);

  grub_free (data);

  return grub_errno;
}

static struct grub_fs grub_fat_fs =
  {
#ifdef MODE_EXFAT
    .name = "exfat",
#else
    .name = "fat",
#endif
    .dir = grub_fat_dir,
    .open = grub_fat_open,
    .read = grub_fat_read,
    .close = grub_fat_close,
    .label = grub_fat_label,
    .uuid = grub_fat_uuid,
#ifdef GRUB_UTIL
#ifdef MODE_EXFAT
    /* ExFAT BPB is 30 larger than FAT32 one.  */
    .reserved_first_sector = 0,
#else
    .reserved_first_sector = 1,
#endif
    .blocklist_install = 1,
#endif
    .next = 0
  };

#ifdef MODE_EXFAT
GRUB_MOD_INIT(exfat)
#else
GRUB_MOD_INIT(fat)
#endif
{
  COMPILE_TIME_ASSERT (sizeof (struct grub_fat_dir_entry) == 32);
  grub_fs_register (&grub_fat_fs);
  my_mod = mod;
}
#ifdef MODE_EXFAT
GRUB_MOD_FINI(exfat)
#else
GRUB_MOD_FINI(fat)
#endif
{
  grub_fs_unregister (&grub_fat_fs);
}

