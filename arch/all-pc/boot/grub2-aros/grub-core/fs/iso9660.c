/* iso9660.c - iso9660 implementation with extensions:
   SUSP, Rock Ridge.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2004,2005,2006,2007,2008,2009,2010  Free Software Foundation, Inc.
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

#include <grub/err.h>
#include <grub/file.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/disk.h>
#include <grub/dl.h>
#include <grub/types.h>
#include <grub/fshelp.h>
#include <grub/charset.h>
#include <grub/datetime.h>

GRUB_MOD_LICENSE ("GPLv3+");

#define GRUB_ISO9660_FSTYPE_DIR		0040000
#define GRUB_ISO9660_FSTYPE_REG		0100000
#define GRUB_ISO9660_FSTYPE_SYMLINK	0120000
#define GRUB_ISO9660_FSTYPE_MASK	0170000

#define GRUB_ISO9660_LOG2_BLKSZ		2
#define GRUB_ISO9660_BLKSZ		2048

#define GRUB_ISO9660_RR_DOT		2
#define GRUB_ISO9660_RR_DOTDOT		4

#define GRUB_ISO9660_VOLDESC_BOOT	0
#define GRUB_ISO9660_VOLDESC_PRIMARY	1
#define GRUB_ISO9660_VOLDESC_SUPP	2
#define GRUB_ISO9660_VOLDESC_PART	3
#define GRUB_ISO9660_VOLDESC_END	255

/* The head of a volume descriptor.  */
struct grub_iso9660_voldesc
{
  grub_uint8_t type;
  grub_uint8_t magic[5];
  grub_uint8_t version;
} __attribute__ ((packed));

struct grub_iso9660_date2
{
  grub_uint8_t year;
  grub_uint8_t month;
  grub_uint8_t day;
  grub_uint8_t hour;
  grub_uint8_t minute;
  grub_uint8_t second;
  grub_uint8_t offset;
} __attribute__ ((packed));

/* A directory entry.  */
struct grub_iso9660_dir
{
  grub_uint8_t len;
  grub_uint8_t ext_sectors;
  grub_uint32_t first_sector;
  grub_uint32_t first_sector_be;
  grub_uint32_t size;
  grub_uint32_t size_be;
  struct grub_iso9660_date2 mtime;
  grub_uint8_t flags;
  grub_uint8_t unused2[6];
  grub_uint8_t namelen;
} __attribute__ ((packed));

struct grub_iso9660_date
{
  grub_uint8_t year[4];
  grub_uint8_t month[2];
  grub_uint8_t day[2];
  grub_uint8_t hour[2];
  grub_uint8_t minute[2];
  grub_uint8_t second[2];
  grub_uint8_t hundredth[2];
  grub_uint8_t offset;
} __attribute__ ((packed));

/* The primary volume descriptor.  Only little endian is used.  */
struct grub_iso9660_primary_voldesc
{
  struct grub_iso9660_voldesc voldesc;
  grub_uint8_t unused1[33];
  grub_uint8_t volname[32];
  grub_uint8_t unused2[16];
  grub_uint8_t escape[32];
  grub_uint8_t unused3[12];
  grub_uint32_t path_table_size;
  grub_uint8_t unused4[4];
  grub_uint32_t path_table;
  grub_uint8_t unused5[12];
  struct grub_iso9660_dir rootdir;
  grub_uint8_t unused6[624];
  struct grub_iso9660_date created;
  struct grub_iso9660_date modified;
} __attribute__ ((packed));

/* A single entry in the path table.  */
struct grub_iso9660_path
{
  grub_uint8_t len;
  grub_uint8_t sectors;
  grub_uint32_t first_sector;
  grub_uint16_t parentdir;
  grub_uint8_t name[0];
} __attribute__ ((packed));

/* An entry in the System Usage area of the directory entry.  */
struct grub_iso9660_susp_entry
{
  grub_uint8_t sig[2];
  grub_uint8_t len;
  grub_uint8_t version;
  grub_uint8_t data[0];
} __attribute__ ((packed));

/* The CE entry.  This is used to describe the next block where data
   can be found.  */
struct grub_iso9660_susp_ce
{
  struct grub_iso9660_susp_entry entry;
  grub_uint32_t blk;
  grub_uint32_t blk_be;
  grub_uint32_t off;
  grub_uint32_t off_be;
  grub_uint32_t len;
  grub_uint32_t len_be;
} __attribute__ ((packed));

struct grub_iso9660_data
{
  struct grub_iso9660_primary_voldesc voldesc;
  grub_disk_t disk;
  int rockridge;
  int susp_skip;
  int joliet;
  struct grub_fshelp_node *node;
};

struct grub_fshelp_node
{
  struct grub_iso9660_data *data;
  grub_size_t have_dirents, alloc_dirents;
  int have_symlink;
  struct grub_iso9660_dir dirents[8];
  char symlink[0];
};

enum
  {
    FLAG_TYPE_PLAIN = 0,
    FLAG_TYPE_DIR = 2,
    FLAG_TYPE = 3,
    FLAG_MORE_EXTENTS = 0x80
  };

static grub_dl_t my_mod;


static grub_err_t
iso9660_to_unixtime (const struct grub_iso9660_date *i, grub_int32_t *nix)
{
  struct grub_datetime datetime;
  
  if (! i->year[0] && ! i->year[1]
      && ! i->year[2] && ! i->year[3]
      && ! i->month[0] && ! i->month[1]
      && ! i->day[0] && ! i->day[1]
      && ! i->hour[0] && ! i->hour[1]
      && ! i->minute[0] && ! i->minute[1]
      && ! i->second[0] && ! i->second[1]
      && ! i->hundredth[0] && ! i->hundredth[1])
    return grub_error (GRUB_ERR_BAD_NUMBER, "empty date");
  datetime.year = (i->year[0] - '0') * 1000 + (i->year[1] - '0') * 100
    + (i->year[2] - '0') * 10 + (i->year[3] - '0');
  datetime.month = (i->month[0] - '0') * 10 + (i->month[1] - '0');
  datetime.day = (i->day[0] - '0') * 10 + (i->day[1] - '0');
  datetime.hour = (i->hour[0] - '0') * 10 + (i->hour[1] - '0');
  datetime.minute = (i->minute[0] - '0') * 10 + (i->minute[1] - '0');
  datetime.second = (i->second[0] - '0') * 10 + (i->second[1] - '0');
  
  if (!grub_datetime2unixtime (&datetime, nix))
    return grub_error (GRUB_ERR_BAD_NUMBER, "incorrect date");
  *nix -= i->offset * 60 * 15;
  return GRUB_ERR_NONE;
}

static int
iso9660_to_unixtime2 (const struct grub_iso9660_date2 *i, grub_int32_t *nix)
{
  struct grub_datetime datetime;

  datetime.year = i->year + 1900;
  datetime.month = i->month;
  datetime.day = i->day;
  datetime.hour = i->hour;
  datetime.minute = i->minute;
  datetime.second = i->second;
  
  if (!grub_datetime2unixtime (&datetime, nix))
    return 0;
  *nix -= i->offset * 60 * 15;
  return 1;
}

static grub_err_t
read_node (grub_fshelp_node_t node, grub_off_t off, grub_size_t len, char *buf)
{
  grub_size_t i = 0;

  while (len > 0)
    {
      grub_size_t toread;
      grub_err_t err;
      while (i < node->have_dirents
	     && off >= grub_le_to_cpu32 (node->dirents[i].size))
	{
	  off -= grub_le_to_cpu32 (node->dirents[i].size);
	  i++;
	}
      if (i == node->have_dirents)
	return grub_error (GRUB_ERR_OUT_OF_RANGE, "read out of range");
      toread = grub_le_to_cpu32 (node->dirents[i].size);
      if (toread > len)
	toread = len;
      err = grub_disk_read (node->data->disk,
			    ((grub_disk_addr_t) grub_le_to_cpu32 (node->dirents[i].first_sector)) << GRUB_ISO9660_LOG2_BLKSZ,
			    off, toread, buf);
      if (err)
	return err;
      len -= toread;
      off += toread;
      buf += toread;
    }
  return GRUB_ERR_NONE;
}

/* Iterate over the susp entries, starting with block SUA_BLOCK on the
   offset SUA_POS with a size of SUA_SIZE bytes.  Hook is called for
   every entry.  */
static grub_err_t
grub_iso9660_susp_iterate (grub_fshelp_node_t node, grub_off_t off,
			   grub_ssize_t sua_size,
			   grub_err_t (*hook)
			   (struct grub_iso9660_susp_entry *entry))
{
  char *sua;
  struct grub_iso9660_susp_entry *entry;
  grub_disk_addr_t ce_block;
  int is_ce = 0;

  auto grub_err_t load_sua (void);

  /* Load a part of the System Usage Area.  */
  grub_err_t load_sua (void)
    {
      grub_err_t err;
      sua = grub_malloc (sua_size);
      if (!sua)
	return grub_errno;

      if (is_ce)
	err = grub_disk_read (node->data->disk, ce_block, off,
			      sua_size, sua);
      else
	err = read_node (node, off, sua_size, sua);
      if (err)
	return err;

      entry = (struct grub_iso9660_susp_entry *) sua;
      return 0;
    }

  if (sua_size <= 0)
    return GRUB_ERR_NONE;

  if (load_sua ())
    return grub_errno;

  for (; (char *) entry < (char *) sua + sua_size - 1;
       entry = (struct grub_iso9660_susp_entry *)
	 ((char *) entry + entry->len))
    {
      /* The last entry.  */
      if (grub_strncmp ((char *) entry->sig, "ST", 2) == 0)
	break;

      /* Additional entries are stored elsewhere.  */
      if (grub_strncmp ((char *) entry->sig, "CE", 2) == 0)
	{
	  struct grub_iso9660_susp_ce *ce;

	  is_ce = 1;
	  ce = (struct grub_iso9660_susp_ce *) entry;
	  sua_size = grub_le_to_cpu32 (ce->len);
	  off = grub_le_to_cpu32 (ce->off);
	  ce_block = grub_le_to_cpu32 (ce->blk) << GRUB_ISO9660_LOG2_BLKSZ;

	  grub_free (sua);
	  if (load_sua ())
	    return grub_errno;
	}

      if (hook (entry))
	{
	  grub_free (sua);
	  return 0;
	}
    }

  grub_free (sua);
  return 0;
}

static char *
grub_iso9660_convert_string (grub_uint8_t *us, int len)
{
  char *p;
  int i;
  grub_uint16_t t[len];

  p = grub_malloc (len * GRUB_MAX_UTF8_PER_UTF16 + 1);
  if (! p)
    return p;

  for (i=0; i<len; i++)
    t[i] = grub_be_to_cpu16 (grub_get_unaligned16 (us + 2 * i));

  *grub_utf16_to_utf8 ((grub_uint8_t *) p, t, len) = '\0';

  return p;
}

static grub_err_t
set_rockridge (struct grub_iso9660_data *data)
{
  int sua_pos;
  int sua_size;
  char *sua;
  struct grub_iso9660_dir rootdir;
  struct grub_iso9660_susp_entry *entry;

  auto grub_err_t susp_iterate (struct grub_iso9660_susp_entry *);

  grub_err_t susp_iterate (struct grub_iso9660_susp_entry *susp_entry)
    {
      /* The "ER" entry is used to detect extensions.  The
	 `IEEE_P1285' extension means Rock ridge.  */
      if (grub_strncmp ((char *) susp_entry->sig, "ER", 2) == 0)
	{
	  data->rockridge = 1;
	  return 1;
	}
      return 0;
    }

  data->rockridge = 0;

  /* Read the system use area and test it to see if SUSP is
     supported.  */
  if (grub_disk_read (data->disk,
		      (grub_le_to_cpu32 (data->voldesc.rootdir.first_sector)
		       << GRUB_ISO9660_LOG2_BLKSZ), 0,
		      sizeof (rootdir), (char *) &rootdir))
    return grub_error (GRUB_ERR_BAD_FS, "not a ISO9660 filesystem");

  sua_pos = (sizeof (rootdir) + rootdir.namelen
	     + (rootdir.namelen % 2) - 1);
  sua_size = rootdir.len - sua_pos;

  if (!sua_size)
    return GRUB_ERR_NONE;

  sua = grub_malloc (sua_size);
  if (! sua)
    return grub_errno;

  if (grub_disk_read (data->disk,
		      (grub_le_to_cpu32 (data->voldesc.rootdir.first_sector)
		       << GRUB_ISO9660_LOG2_BLKSZ), sua_pos,
		      sua_size, sua))
    {
      grub_free (sua);
      return grub_error (GRUB_ERR_BAD_FS, "not a ISO9660 filesystem");
    }

  entry = (struct grub_iso9660_susp_entry *) sua;

  /* Test if the SUSP protocol is used on this filesystem.  */
  if (grub_strncmp ((char *) entry->sig, "SP", 2) == 0)
    {
      struct grub_fshelp_node rootnode;

      rootnode.data = data;
      rootnode.alloc_dirents = ARRAY_SIZE (rootnode.dirents);
      rootnode.have_dirents = 1;
      rootnode.have_symlink = 0;
      rootnode.dirents[0] = data->voldesc.rootdir;

      /* The 2nd data byte stored how many bytes are skipped every time
	 to get to the SUA (System Usage Area).  */
      data->susp_skip = entry->data[2];
      entry = (struct grub_iso9660_susp_entry *) ((char *) entry + entry->len);

      /* Iterate over the entries in the SUA area to detect
	 extensions.  */
      if (grub_iso9660_susp_iterate (&rootnode,
				     sua_pos, sua_size, susp_iterate))
	{
	  grub_free (sua);
	  return grub_errno;
	}
    }
  grub_free (sua);
  return GRUB_ERR_NONE;
}

static struct grub_iso9660_data *
grub_iso9660_mount (grub_disk_t disk)
{
  struct grub_iso9660_data *data = 0;
  struct grub_iso9660_primary_voldesc voldesc;
  int block;

  data = grub_zalloc (sizeof (struct grub_iso9660_data));
  if (! data)
    return 0;

  data->disk = disk;

  block = 16;
  do
    {
      int copy_voldesc = 0;

      /* Read the superblock.  */
      if (grub_disk_read (disk, block << GRUB_ISO9660_LOG2_BLKSZ, 0,
			  sizeof (struct grub_iso9660_primary_voldesc),
			  (char *) &voldesc))
        {
          grub_error (GRUB_ERR_BAD_FS, "not a ISO9660 filesystem");
          goto fail;
        }

      if (grub_strncmp ((char *) voldesc.voldesc.magic, "CD001", 5) != 0)
        {
          grub_error (GRUB_ERR_BAD_FS, "not a ISO9660 filesystem");
          goto fail;
        }

      if (voldesc.voldesc.type == GRUB_ISO9660_VOLDESC_PRIMARY)
	copy_voldesc = 1;
      else if (!data->rockridge
	       && (voldesc.voldesc.type == GRUB_ISO9660_VOLDESC_SUPP)
	       && (voldesc.escape[0] == 0x25) && (voldesc.escape[1] == 0x2f)
	       &&
               ((voldesc.escape[2] == 0x40) ||	/* UCS-2 Level 1.  */
                (voldesc.escape[2] == 0x43) ||  /* UCS-2 Level 2.  */
                (voldesc.escape[2] == 0x45)))	/* UCS-2 Level 3.  */
        {
          copy_voldesc = 1;
          data->joliet = 1;
        }

      if (copy_voldesc)
	{
	  grub_memcpy((char *) &data->voldesc, (char *) &voldesc,
		      sizeof (struct grub_iso9660_primary_voldesc));
	  if (set_rockridge (data))
	    goto fail;
	}

      block++;
    } while (voldesc.voldesc.type != GRUB_ISO9660_VOLDESC_END);

  return data;

 fail:
  grub_free (data);
  return 0;
}


static char *
grub_iso9660_read_symlink (grub_fshelp_node_t node)
{
  return node->have_symlink 
    ? grub_strdup (node->symlink
		   + (node->have_dirents) * sizeof (node->dirents[0])
		   - sizeof (node->dirents)) : grub_strdup ("");
}

static grub_off_t
get_node_size (grub_fshelp_node_t node)
{
  grub_off_t ret = 0;
  grub_size_t i;

  for (i = 0; i < node->have_dirents; i++)
    ret += grub_le_to_cpu32 (node->dirents[i].size);
  return ret;
}

static int
grub_iso9660_iterate_dir (grub_fshelp_node_t dir,
			  int NESTED_FUNC_ATTR
			  (*hook) (const char *filename,
				   enum grub_fshelp_filetype filetype,
				   grub_fshelp_node_t node))
{
  struct grub_iso9660_dir dirent;
  grub_off_t offset = 0;
  char *filename = 0;
  int filename_alloc = 0;
  enum grub_fshelp_filetype type;
  grub_off_t len;
  char *symlink = 0;
  int was_continue = 0;

  /* Extend the symlink.  */
  auto inline void  __attribute__ ((always_inline)) add_part (const char *part,
							      int len2);

  auto inline void  __attribute__ ((always_inline)) add_part (const char *part,
							      int len2)
    {
      int size = symlink ? grub_strlen (symlink) : 0;

      symlink = grub_realloc (symlink, size + len2 + 1);
      if (! symlink)
	return;

      symlink[size] = 0;
      grub_strncat (symlink, part, len2);
    }

  auto grub_err_t susp_iterate_dir (struct grub_iso9660_susp_entry *);

  grub_err_t susp_iterate_dir (struct grub_iso9660_susp_entry *entry)
    {
      /* The filename in the rock ridge entry.  */
      if (grub_strncmp ("NM", (char *) entry->sig, 2) == 0)
	{
	  /* The flags are stored at the data position 0, here the
	     filename type is stored.  */
	  /* FIXME: Fix this slightly improper cast.  */
	  if (entry->data[0] & GRUB_ISO9660_RR_DOT)
	    filename = (char *) ".";
	  else if (entry->data[0] & GRUB_ISO9660_RR_DOTDOT)
	    filename = (char *) "..";
	  else if (entry->len >= 5)
	    {
	      grub_size_t size = 1, csize = 1;
	      char *old;
	      csize = size = entry->len - 5;
	      old = filename;
	      if (filename_alloc)
		{
		  size += grub_strlen (filename);
		  filename = grub_realloc (filename, size + 1);
		}
	      else
		{
		  filename_alloc = 1;
		  filename = grub_zalloc (size + 1);
		  filename[0] = 0;
		}
	      if (!filename)
		{
		  filename = old;
		  return grub_errno;
		}
	      filename_alloc = 1;
	      grub_strncat (filename, (char *) &entry->data[1], csize);
	      filename[size] = '\0';
	    }
	}
      /* The mode information (st_mode).  */
      else if (grub_strncmp ((char *) entry->sig, "PX", 2) == 0)
	{
	  /* At position 0 of the PX record the st_mode information is
	     stored (little-endian).  */
	  grub_uint32_t mode = ((entry->data[0] + (entry->data[1] << 8))
				& GRUB_ISO9660_FSTYPE_MASK);

	  switch (mode)
	    {
	    case GRUB_ISO9660_FSTYPE_DIR:
	      type = GRUB_FSHELP_DIR;
	      break;
	    case GRUB_ISO9660_FSTYPE_REG:
	      type = GRUB_FSHELP_REG;
	      break;
	    case GRUB_ISO9660_FSTYPE_SYMLINK:
	      type = GRUB_FSHELP_SYMLINK;
	      break;
	    default:
	      type = GRUB_FSHELP_UNKNOWN;
	    }
	}
      else if (grub_strncmp ("SL", (char *) entry->sig, 2) == 0)
	{
	  unsigned int pos = 1;

	  /* The symlink is not stored as a POSIX symlink, translate it.  */
	  while (pos + sizeof (*entry) < entry->len)
	    {
	      /* The current position is the `Component Flag'.  */
	      switch (entry->data[pos] & 30)
		{
		case 0:
		  {
		    /* The data on pos + 2 is the actual data, pos + 1
		       is the length.  Both are part of the `Component
		       Record'.  */
		    if (symlink && !was_continue)
		      add_part ("/", 1);
		    add_part ((char *) &entry->data[pos + 2],
			      entry->data[pos + 1]);
		    was_continue = (entry->data[pos] & 1);
		    break;
		  }

		case 2:
		  add_part ("./", 2);
		  break;

		case 4:
		  add_part ("../", 3);
		  break;

		case 8:
		  add_part ("/", 1);
		  break;
		}
	      /* In pos + 1 the length of the `Component Record' is
		 stored.  */
	      pos += entry->data[pos + 1] + 2;
	    }

	  /* Check if `grub_realloc' failed.  */
	  if (grub_errno)
	    return grub_errno;
	}

      return 0;
    }

  len = get_node_size (dir);

  for (; offset < len; offset += dirent.len)
    {
      symlink = 0;
      was_continue = 0;

      if (read_node (dir, offset, sizeof (dirent), (char *) &dirent))
	return 0;

      /* The end of the block, skip to the next one.  */
      if (!dirent.len)
	{
	  offset = (offset / GRUB_ISO9660_BLKSZ + 1) * GRUB_ISO9660_BLKSZ;
	  continue;
	}

      {
	char name[dirent.namelen + 1];
	int nameoffset = offset + sizeof (dirent);
	struct grub_fshelp_node *node;
	int sua_off = (sizeof (dirent) + dirent.namelen + 1
		       - (dirent.namelen % 2));
	int sua_size = dirent.len - sua_off;

	sua_off += offset + dir->data->susp_skip;

	filename = 0;
	filename_alloc = 0;
	type = GRUB_FSHELP_UNKNOWN;

	if (dir->data->rockridge
	    && grub_iso9660_susp_iterate (dir, sua_off, sua_size,
					  susp_iterate_dir))
	  return 0;

	/* Read the name.  */
	if (read_node (dir, nameoffset, dirent.namelen, (char *) name))
	  return 0;

	node = grub_malloc (sizeof (struct grub_fshelp_node));
	if (!node)
	  return 0;

	node->alloc_dirents = ARRAY_SIZE (node->dirents);
	node->have_dirents = 1;

	/* Setup a new node.  */
	node->data = dir->data;
	node->have_symlink = 0;

	/* If the filetype was not stored using rockridge, use
	   whatever is stored in the iso9660 filesystem.  */
	if (type == GRUB_FSHELP_UNKNOWN)
	  {
	    if ((dirent.flags & FLAG_TYPE) == FLAG_TYPE_DIR)
	      type = GRUB_FSHELP_DIR;
	    else
	      type = GRUB_FSHELP_REG;
	  }

	/* . and .. */
	if (!filename && dirent.namelen == 1 && name[0] == 0)
	  filename = (char *) ".";

	if (!filename && dirent.namelen == 1 && name[0] == 1)
	  filename = (char *) "..";

	/* The filename was not stored in a rock ridge entry.  Read it
	   from the iso9660 filesystem.  */
	if (!dir->data->joliet && !filename)
	  {
	    char *ptr;
	    name[dirent.namelen] = '\0';
	    filename = grub_strrchr (name, ';');
	    if (filename)
	      *filename = '\0';
	    /* ISO9660 names are not case-preserving.  */
	    type |= GRUB_FSHELP_CASE_INSENSITIVE;
	    for (ptr = name; *ptr; ptr++)
	      *ptr = grub_tolower (*ptr);
	    if (ptr != name && *(ptr - 1) == '.')
	      *(ptr - 1) = 0;
	    filename = name;
	  }

        if (dir->data->joliet && !filename)
          {
            char *oldname, *semicolon;

            oldname = name;
            filename = grub_iso9660_convert_string
                  ((grub_uint8_t *) oldname, dirent.namelen >> 1);

	    semicolon = grub_strrchr (filename, ';');
	    if (semicolon)
	      *semicolon = '\0';

            if (filename_alloc)
              grub_free (oldname);

            filename_alloc = 1;
          }

	node->dirents[0] = dirent;
	while (dirent.flags & FLAG_MORE_EXTENTS)
	  {
	    offset += dirent.len;
	    if (read_node (dir, offset, sizeof (dirent), (char *) &dirent))
	      {
		if (filename_alloc)
		  grub_free (filename);
		grub_free (node);
		return 0;
	      }
	    if (node->have_dirents >= node->alloc_dirents)
	      {
		struct grub_fshelp_node *new_node;
		node->alloc_dirents *= 2;
		new_node = grub_realloc (node, 
					 sizeof (struct grub_fshelp_node)
					 + ((node->alloc_dirents
					     - ARRAY_SIZE (node->dirents))
					    * sizeof (node->dirents[0])));
		if (!new_node)
		  {
		    if (filename_alloc)
		      grub_free (filename);
		    grub_free (node);
		    return 0;
		  }
		node = new_node;
	      }
	    node->dirents[node->have_dirents++] = dirent;
	  }
	if (symlink)
	  {
	    if ((node->alloc_dirents - node->have_dirents)
		* sizeof (node->dirents[0]) < grub_strlen (symlink) + 1)
	      {
		struct grub_fshelp_node *new_node;
		new_node = grub_realloc (node,
					 sizeof (struct grub_fshelp_node)
					 + ((node->alloc_dirents
					     - ARRAY_SIZE (node->dirents))
					    * sizeof (node->dirents[0]))
					 + grub_strlen (symlink) + 1);
		if (!new_node)
		  {
		    if (filename_alloc)
		      grub_free (filename);
		    grub_free (node);
		    return 0;
		  }
		node = new_node;
	      }
	    node->have_symlink = 1;
	    grub_strcpy (node->symlink
			 + node->have_dirents * sizeof (node->dirents[0])
			 - sizeof (node->dirents), symlink);
	    grub_free (symlink);
	    symlink = 0;
	    was_continue = 0;
	  }
	if (hook (filename, type, node))
	  {
	    if (filename_alloc)
	      grub_free (filename);
	    return 1;
	  }
	if (filename_alloc)
	  grub_free (filename);
      }
    }

  return 0;
}



static grub_err_t
grub_iso9660_dir (grub_device_t device, const char *path,
		  int (*hook) (const char *filename,
			       const struct grub_dirhook_info *info))
{
  struct grub_iso9660_data *data = 0;
  struct grub_fshelp_node rootnode;
  struct grub_fshelp_node *foundnode;

  auto int NESTED_FUNC_ATTR iterate (const char *filename,
				     enum grub_fshelp_filetype filetype,
				     grub_fshelp_node_t node);

  int NESTED_FUNC_ATTR iterate (const char *filename,
				enum grub_fshelp_filetype filetype,
				grub_fshelp_node_t node)
    {
      struct grub_dirhook_info info;
      grub_memset (&info, 0, sizeof (info));
      info.dir = ((filetype & GRUB_FSHELP_TYPE_MASK) == GRUB_FSHELP_DIR);
      info.mtimeset = !!iso9660_to_unixtime2 (&node->dirents[0].mtime, &info.mtime);

      grub_free (node);
      return hook (filename, &info);
    }

  grub_dl_ref (my_mod);

  data = grub_iso9660_mount (device->disk);
  if (! data)
    goto fail;

  rootnode.data = data;
  rootnode.alloc_dirents = 0;
  rootnode.have_dirents = 1;
  rootnode.have_symlink = 0;
  rootnode.dirents[0] = data->voldesc.rootdir;

  /* Use the fshelp function to traverse the path.  */
  if (grub_fshelp_find_file (path, &rootnode,
			     &foundnode,
			     grub_iso9660_iterate_dir,
			     grub_iso9660_read_symlink,
			     GRUB_FSHELP_DIR))
    goto fail;

  /* List the files in the directory.  */
  grub_iso9660_iterate_dir (foundnode, iterate);

  if (foundnode != &rootnode)
    grub_free (foundnode);

 fail:
  grub_free (data);

  grub_dl_unref (my_mod);

  return grub_errno;
}


/* Open a file named NAME and initialize FILE.  */
static grub_err_t
grub_iso9660_open (struct grub_file *file, const char *name)
{
  struct grub_iso9660_data *data;
  struct grub_fshelp_node rootnode;
  struct grub_fshelp_node *foundnode;

  grub_dl_ref (my_mod);

  data = grub_iso9660_mount (file->device->disk);
  if (!data)
    goto fail;

  rootnode.data = data;
  rootnode.alloc_dirents = 0;
  rootnode.have_dirents = 1;
  rootnode.have_symlink = 0;
  rootnode.dirents[0] = data->voldesc.rootdir;

  /* Use the fshelp function to traverse the path.  */
  if (grub_fshelp_find_file (name, &rootnode,
			     &foundnode,
			     grub_iso9660_iterate_dir,
			     grub_iso9660_read_symlink,
			     GRUB_FSHELP_REG))
    goto fail;

  data->node = foundnode;
  file->data = data;
  file->size = get_node_size (foundnode);
  file->offset = 0;

  return 0;

 fail:
  grub_dl_unref (my_mod);

  grub_free (data);

  return grub_errno;
}


static grub_ssize_t
grub_iso9660_read (grub_file_t file, char *buf, grub_size_t len)
{
  struct grub_iso9660_data *data =
    (struct grub_iso9660_data *) file->data;

  /* XXX: The file is stored in as a single extent.  */
  data->disk->read_hook = file->read_hook;
  read_node (data->node, file->offset, len, buf);
  data->disk->read_hook = NULL;

  if (grub_errno)
    return -1;

  return len;
}


static grub_err_t
grub_iso9660_close (grub_file_t file)
{
  struct grub_iso9660_data *data =
    (struct grub_iso9660_data *) file->data;
  grub_free (data->node);
  grub_free (data);

  grub_dl_unref (my_mod);

  return GRUB_ERR_NONE;
}


static grub_err_t
grub_iso9660_label (grub_device_t device, char **label)
{
  struct grub_iso9660_data *data;
  data = grub_iso9660_mount (device->disk);

  if (data)
    {
      if (data->joliet)
        *label = grub_iso9660_convert_string (data->voldesc.volname, 16);
      else
        *label = grub_strndup ((char *) data->voldesc.volname, 32);
      if (*label)
	{
	  char *ptr;
	  for (ptr = *label; *ptr;ptr++);
	  ptr--;
	  while (ptr >= *label && *ptr == ' ')
	    *ptr-- = 0;
	}

      grub_free (data);
    }
  else
    *label = 0;

  return grub_errno;
}


static grub_err_t
grub_iso9660_uuid (grub_device_t device, char **uuid)
{
  struct grub_iso9660_data *data;
  grub_disk_t disk = device->disk;

  grub_dl_ref (my_mod);

  data = grub_iso9660_mount (disk);
  if (data)
    {
      if (! data->voldesc.modified.year[0] && ! data->voldesc.modified.year[1]
	  && ! data->voldesc.modified.year[2] && ! data->voldesc.modified.year[3]
	  && ! data->voldesc.modified.month[0] && ! data->voldesc.modified.month[1]
	  && ! data->voldesc.modified.day[0] && ! data->voldesc.modified.day[1]
	  && ! data->voldesc.modified.hour[0] && ! data->voldesc.modified.hour[1]
	  && ! data->voldesc.modified.minute[0] && ! data->voldesc.modified.minute[1]
	  && ! data->voldesc.modified.second[0] && ! data->voldesc.modified.second[1]
	  && ! data->voldesc.modified.hundredth[0] && ! data->voldesc.modified.hundredth[1])
	{
	  grub_error (GRUB_ERR_BAD_NUMBER, "no creation date in filesystem to generate UUID");
	  *uuid = NULL;
	}
      else
	{
	  *uuid = grub_xasprintf ("%c%c%c%c-%c%c-%c%c-%c%c-%c%c-%c%c-%c%c",
				 data->voldesc.modified.year[0],
				 data->voldesc.modified.year[1],
				 data->voldesc.modified.year[2],
				 data->voldesc.modified.year[3],
				 data->voldesc.modified.month[0],
				 data->voldesc.modified.month[1],
				 data->voldesc.modified.day[0],
				 data->voldesc.modified.day[1],
				 data->voldesc.modified.hour[0],
				 data->voldesc.modified.hour[1],
				 data->voldesc.modified.minute[0],
				 data->voldesc.modified.minute[1],
				 data->voldesc.modified.second[0],
				 data->voldesc.modified.second[1],
				 data->voldesc.modified.hundredth[0],
				 data->voldesc.modified.hundredth[1]);
	}
    }
  else
    *uuid = NULL;

	grub_dl_unref (my_mod);

  grub_free (data);

  return grub_errno;
}

/* Get writing time of filesystem. */
static grub_err_t 
grub_iso9660_mtime (grub_device_t device, grub_int32_t *timebuf)
{
  struct grub_iso9660_data *data;
  grub_disk_t disk = device->disk;
  grub_err_t err;

  grub_dl_ref (my_mod);

  data = grub_iso9660_mount (disk);
  if (!data)
    {
      grub_dl_unref (my_mod);
      return grub_errno;
    }
  err = iso9660_to_unixtime (&data->voldesc.modified, timebuf);

  grub_dl_unref (my_mod);

  grub_free (data);

  return err;
}




static struct grub_fs grub_iso9660_fs =
  {
    .name = "iso9660",
    .dir = grub_iso9660_dir,
    .open = grub_iso9660_open,
    .read = grub_iso9660_read,
    .close = grub_iso9660_close,
    .label = grub_iso9660_label,
    .uuid = grub_iso9660_uuid,
    .mtime = grub_iso9660_mtime,
#ifdef GRUB_UTIL
    .reserved_first_sector = 1,
    .blocklist_install = 1,
#endif
    .next = 0
  };

GRUB_MOD_INIT(iso9660)
{
  grub_fs_register (&grub_iso9660_fs);
  my_mod = mod;
}

GRUB_MOD_FINI(iso9660)
{
  grub_fs_unregister (&grub_iso9660_fs);
}
