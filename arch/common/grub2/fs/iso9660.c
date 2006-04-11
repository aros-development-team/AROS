/* iso9660.c - iso9660 implementation with extensions:
   SUSP, Rock Ridge.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2004  Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <grub/err.h>
#include <grub/file.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/disk.h>
#include <grub/dl.h>
#include <grub/types.h>
#include <grub/fshelp.h>

#define GRUB_ISO9660_FSTYPE_DIR		0040000
#define GRUB_ISO9660_FSTYPE_REG		0100000
#define GRUB_ISO9660_FSTYPE_SYMLINK	0120000
#define GRUB_ISO9660_FSTYPE_MASK	0170000

#define GRUB_ISO9660_LOG2_BLKSZ		2
#define GRUB_ISO9660_BLKSZ		2048

#define GRUB_ISO9660_RR_DOT		2
#define GRUB_ISO9660_RR_DOTDOT		4

/* The head of a volume descriptor.  */
struct grub_iso9660_voldesc
{
  grub_uint8_t type;
  grub_uint8_t magic[5];
  grub_uint8_t version;
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
  grub_uint8_t unused1[7];
  grub_uint8_t flags;
  grub_uint8_t unused2[6];
  grub_uint8_t namelen;
} __attribute__ ((packed));

/* The primary volume descriptor.  Only little endian is used.  */
struct grub_iso9660_primary_voldesc
{
  struct grub_iso9660_voldesc voldesc;
  grub_uint8_t unused1[33];
  grub_uint8_t volname[32];
  grub_uint8_t unused2[60];
  grub_uint32_t path_table_size;
  grub_uint8_t unused3[4];
  grub_uint32_t path_table;
  grub_uint8_t unused4[12];
  struct grub_iso9660_dir rootdir;
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
  unsigned int first_sector;
  unsigned int length;
  int rockridge;
  int susp_skip;
};

struct grub_fshelp_node
{
  struct grub_iso9660_data *data;
  unsigned int size;
  unsigned int blk;
  unsigned int dir_blk;
  unsigned int dir_off;
};

#ifndef GRUB_UTIL
static grub_dl_t my_mod;
#endif


/* Iterate over the susp entries, starting with block SUA_BLOCK on the
   offset SUA_POS with a size of SUA_SIZE bytes.  Hook is called for
   every entry.  */
static grub_err_t
grub_iso9660_susp_iterate (struct grub_iso9660_data *data,
			   int sua_block, int sua_pos, int sua_size,
			   grub_err_t (*hook)
			   (struct grub_iso9660_susp_entry *entry))
{
  char *sua;
  struct grub_iso9660_susp_entry *entry;
  
  auto grub_err_t load_sua (void);
  
  /* Load a part of the System Usage Area.  */
  grub_err_t load_sua (void)
    {
      sua = grub_malloc (sua_size);
      if (!sua)
	return grub_errno;
      
      if (grub_disk_read (data->disk, sua_block, sua_pos,
			  sua_size, sua))
	return grub_errno;
      
      entry = (struct grub_iso9660_susp_entry *) sua;
      return 0;
    }
  
  if (load_sua ())
    return grub_errno;
  
  for (; (char *) entry < (char *) sua + sua_size - 1;
       entry = (struct grub_iso9660_susp_entry *)
	 ((char *) entry + entry->len))
    {
      /* The last entry.  */
      if (!grub_strncmp (entry->sig, "ST", 2))
	break;
      
      /* Additional entries are stored elsewhere.  */
      if (!grub_strncmp (entry->sig, "CE", 2))
	{
	  struct grub_iso9660_susp_ce *ce;
	  
	  ce = (struct grub_iso9660_susp_ce *) entry;
	  sua_size = grub_le_to_cpu32 (ce->len);
	  sua_pos = grub_le_to_cpu32 (ce->off);
	  sua_block = grub_le_to_cpu32 (ce->blk) << GRUB_ISO9660_LOG2_BLKSZ;
	  
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


static struct grub_iso9660_data *
grub_iso9660_mount (grub_disk_t disk)
{
  struct grub_iso9660_data *data = 0;
  struct grub_iso9660_dir rootdir;
  int sua_pos;
  int sua_size;
  char *sua;
  struct grub_iso9660_susp_entry *entry;
  
  auto grub_err_t susp_iterate (struct grub_iso9660_susp_entry *);
  
  grub_err_t susp_iterate (struct grub_iso9660_susp_entry *susp_entry)
    {
      /* The "ER" entry is used to detect extensions.  The
	 `IEEE_P1285' extension means Rock ridge.  */
      if (!grub_strncmp (susp_entry->sig, "ER", 2))
	{
	  data->rockridge = 1;
	  return 1;
	}
      return 0;
    }
  
  data = grub_malloc (sizeof (struct grub_iso9660_data));
  if (!data)
    return 0;
  
  /* Read the superblock.  */
  if (grub_disk_read (disk, 16 << GRUB_ISO9660_LOG2_BLKSZ, 0,
		      sizeof (struct grub_iso9660_primary_voldesc),
		      (char *) &data->voldesc))
    {
      grub_error (GRUB_ERR_BAD_FS, "not a iso9660 filesystem");
      goto fail;
    }

  if (grub_strncmp (data->voldesc.voldesc.magic, "CD001", 5))
    {
      grub_error (GRUB_ERR_BAD_FS, "not a iso9660 filesystem");
      goto fail;
    }
  
  data->disk = disk;
  data->rockridge = 0;
  
  /* Read the system use area and test it to see if SUSP is
     supported.  */
  if (grub_disk_read (disk, (grub_le_to_cpu32 (data->voldesc.rootdir.first_sector)
			     << GRUB_ISO9660_LOG2_BLKSZ), 0,
		      sizeof (rootdir), (char *) &rootdir))
    {
      grub_error (GRUB_ERR_BAD_FS, "not a iso9660 filesystem");
      goto fail;
    }
  
  if (grub_disk_read (disk, (grub_le_to_cpu32 (data->voldesc.rootdir.first_sector)
			     << GRUB_ISO9660_LOG2_BLKSZ), 0,
		      sizeof (rootdir), (char *) &rootdir))
    {
      grub_error (GRUB_ERR_BAD_FS, "not a iso9660 filesystem");
      goto fail;
    }
  
  sua_pos = (sizeof (rootdir) + rootdir.namelen
	     + (rootdir.namelen % 2) - 1);
  sua_size = rootdir.len - sua_pos;

  sua = grub_malloc (sua_size);
  if (!sua)
    goto fail;
  
  if (grub_disk_read (disk, (grub_le_to_cpu32 (data->voldesc.rootdir.first_sector)
			     << GRUB_ISO9660_LOG2_BLKSZ), sua_pos,
		      sua_size, sua))
    {
      grub_error (GRUB_ERR_BAD_FS, "not a iso9660 filesystem");
      goto fail;
    }
  
  entry = (struct grub_iso9660_susp_entry *) sua;
  
  /* Test if the SUSP protocol is used on this filesystem.  */
  if (!grub_strncmp (entry->sig, "SP", 2))
    {
      /* The 2nd data byte stored how many bytes are skipped everytime
	 to get to the SUA (System Usage Area).  */
      data->susp_skip = entry->data[2];
      entry = (struct grub_iso9660_susp_entry *) ((char *) entry + entry->len);
      
      /* Iterate over the entries in the SUA area to detect
	 entensions.  */
      if (grub_iso9660_susp_iterate (data,
				     (grub_le_to_cpu32 (data->voldesc.rootdir.first_sector)
				      << GRUB_ISO9660_LOG2_BLKSZ),
				     sua_pos, sua_size, susp_iterate))
	goto fail;
    }
  
  return data;
    
 fail:
  grub_free (data);
  return 0;
}


static char *
grub_iso9660_read_symlink (grub_fshelp_node_t node)
{
  struct grub_iso9660_dir dirent;
  int sua_off;
  int sua_size;
  char *symlink = 0;
  int addslash = 0;
  
  auto void add_part (const char *part, int len);
  auto grub_err_t susp_iterate_sl (struct grub_iso9660_susp_entry *);

  /* Extend the symlink.  */
  void add_part (const char *part, int len)
    {
      int size = grub_strlen (symlink);
      
      symlink = grub_realloc (symlink, size + len + 1);
      if (!symlink)
	return;
      grub_strncat (symlink, part, len);

      return;
    }
    
  /* Read in a symlink.  */
  grub_err_t susp_iterate_sl (struct grub_iso9660_susp_entry *entry)
    {
      if (!grub_strncmp ("SL", entry->sig, 2))
	{
	  unsigned int pos = 1;

	  /* The symlink is not stored as a POSIX symlink, translate it.  */
	  while (pos < grub_le_to_cpu32 (entry->len))
	    {
	      if (addslash)
		{
		  add_part ("/", 1);
		  addslash = 0;
		}
	      
	      /* The current position is the `Component Flag'.  */
	      switch (entry->data[pos] & 30)
		{
		case 0:
		  {
		    /* The data on pos + 2 is the actual data, pos + 1
		       is the length.  Both are part of the `Component
		       Record'.  */
		    add_part (&entry->data[pos + 2],
			      entry->data[pos + 1]);
		    if ((entry->data[pos] & 1))
		      addslash = 1;

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
  
  if (grub_disk_read (node->data->disk, node->dir_blk, node->dir_off,
		      sizeof (dirent), (char *) &dirent))
    return 0;
  
  sua_off = (sizeof (dirent) + dirent.namelen + 1 - (dirent.namelen % 2)
	     + node->data->susp_skip);
  sua_size = dirent.len - sua_off;
  
  symlink = grub_malloc (1);
  if (!symlink)
    return 0;
  
  *symlink = '\0';
  
  if (grub_iso9660_susp_iterate (node->data, node->dir_blk,
				 node->dir_off + sua_off,
				 sua_size, susp_iterate_sl))
    {
      grub_free (symlink);
      return 0;
    }
  
  return symlink;
}


static int
grub_iso9660_iterate_dir (grub_fshelp_node_t dir,
			  int NESTED_FUNC_ATTR
			  (*hook) (const char *filename,
				   enum grub_fshelp_filetype filetype,
				   grub_fshelp_node_t node))
{
  struct grub_iso9660_dir dirent;
  unsigned int offset = 0;
  char *filename;
  int filename_alloc = 0;
  enum grub_fshelp_filetype type;
  
  auto grub_err_t susp_iterate_dir (struct grub_iso9660_susp_entry *);

  grub_err_t susp_iterate_dir (struct grub_iso9660_susp_entry *entry)
    {
      /* The filename in the rock ridge entry.  */
      if (!grub_strncmp ("NM", entry->sig, 2))
	{
	  /* The flags are stored at the data position 0, here the
	     filename type is stored.  */
	  if (entry->data[0] & GRUB_ISO9660_RR_DOT)
	    filename = ".";
	  else if (entry->data[0] & GRUB_ISO9660_RR_DOTDOT)
	    filename = "..";
	  else
	    {
	      int size = 1;
	      if (filename)
		{
		  size += grub_strlen (filename);
		  grub_realloc (filename,
				grub_strlen (filename)
				+ entry->len);
		}
	      else
		{
		  size = entry->len - 5;
		  filename = grub_malloc (size + 1);
		  filename[0] = '\0';
		}
	      filename_alloc = 1;
	      grub_strncpy (filename, &entry->data[1], size);
	      filename [size] = '\0';
	    }
	}
      /* The mode information (st_mode).  */
      else if (!grub_strncmp (entry->sig, "PX", 2))
	{
	  /* At position 0 of the PX record the st_mode information is
	     stored.  */
	  grub_uint32_t mode = ((*(grub_uint32_t *) &entry->data[0])
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
      
      return 0;
    }

  while (offset < dir->size)
    {
      if (grub_disk_read (dir->data->disk,
			  (dir->blk << GRUB_ISO9660_LOG2_BLKSZ)
			  + offset / GRUB_DISK_SECTOR_SIZE,
			  offset % GRUB_DISK_SECTOR_SIZE,
			  sizeof (dirent), (char *) &dirent))
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
		       - (dirent.namelen % 2));;
	int sua_size = dirent.len - sua_off;
	
	sua_off += offset + dir->data->susp_skip;
	
	filename = 0;
	filename_alloc = 0;
	type = GRUB_FSHELP_UNKNOWN;

	if (dir->data->rockridge
	    && grub_iso9660_susp_iterate (dir->data,
					  (dir->blk << GRUB_ISO9660_LOG2_BLKSZ)
					  + (sua_off
					     / GRUB_DISK_SECTOR_SIZE),
					  sua_off % GRUB_DISK_SECTOR_SIZE,
					  sua_size, susp_iterate_dir))
	  return 0;
	
	/* Read the name.  */
	if (grub_disk_read (dir->data->disk,
			    (dir->blk << GRUB_ISO9660_LOG2_BLKSZ)
			    + nameoffset / GRUB_DISK_SECTOR_SIZE,
			    nameoffset % GRUB_DISK_SECTOR_SIZE,
			    dirent.namelen, (char *) name))
	  return 0;
	
	node = grub_malloc (sizeof (struct grub_fshelp_node));
	if (!node)
	  return 0;
	
	/* Setup a new node.  */
	node->data = dir->data;
	node->size = grub_le_to_cpu32 (dirent.size);
	node->blk = grub_le_to_cpu32 (dirent.first_sector);
	node->dir_blk = ((dir->blk << GRUB_ISO9660_LOG2_BLKSZ)
			 + offset / GRUB_DISK_SECTOR_SIZE);
	node->dir_off = offset % GRUB_DISK_SECTOR_SIZE;
	
	/* If the filetype was not stored using rockridge, use
	   whatever is stored in the iso9660 filesystem.  */
	if (type == GRUB_FSHELP_UNKNOWN)
	  {
	    if ((dirent.flags & 3) == 2)
	      type = GRUB_FSHELP_DIR;
	    else
	      type = GRUB_FSHELP_REG;
	  }
	
	/* The filename was not stored in a rock ridge entry.  Read it
	   from the iso9660 filesystem.  */
	if (!filename)
	  {
	    name[dirent.namelen] = '\0';
	    filename = grub_strrchr (name, ';');
	    if (filename)
	      *filename = '\0';
	    
	    if (dirent.namelen == 1 && name[0] == 0)
	      filename = ".";
	    else if (dirent.namelen == 1 && name[0] == 1)
	      filename = "..";
	    else
	      filename = name;
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
      
      offset += dirent.len;
    }
  
  return 0;
}



static grub_err_t
grub_iso9660_dir (grub_device_t device, const char *path, 
		  int (*hook) (const char *filename, int dir))
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
      grub_free (node);
      
      if (filetype == GRUB_FSHELP_DIR)
	return hook (filename, 1);
      else 
	return hook (filename, 0);
      
      return 0;
    }

#ifndef GRUB_UTIL
  grub_dl_ref (my_mod);
#endif

  data = grub_iso9660_mount (device->disk);
  if (!data)
    goto fail;
  
  rootnode.data = data;
  rootnode.blk = grub_le_to_cpu32 (data->voldesc.rootdir.first_sector);
  rootnode.size = grub_le_to_cpu32 (data->voldesc.rootdir.size);
  
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

#ifndef GRUB_UTIL
  grub_dl_unref (my_mod);
#endif

  return grub_errno;
}


/* Open a file named NAME and initialize FILE.  */
static grub_err_t
grub_iso9660_open (struct grub_file *file, const char *name)
{
  struct grub_iso9660_data *data;
  struct grub_fshelp_node rootnode;
  struct grub_fshelp_node *foundnode;
  
#ifndef GRUB_UTIL
  grub_dl_ref (my_mod);
#endif

  data = grub_iso9660_mount (file->device->disk);
  if (!data)
    goto fail;
  
  rootnode.data = data;
  rootnode.blk = grub_le_to_cpu32 (data->voldesc.rootdir.first_sector);
  rootnode.size = grub_le_to_cpu32 (data->voldesc.rootdir.size);
  
  /* Use the fshelp function to traverse the path.  */
  if (grub_fshelp_find_file (name, &rootnode,
			     &foundnode,
			     grub_iso9660_iterate_dir,
			     grub_iso9660_read_symlink,
			     GRUB_FSHELP_REG))
    goto fail;
  
  data->first_sector = foundnode->blk;
  data->length = foundnode->size;
  
  file->data = data;
  file->size = foundnode->size;
  file->offset = 0;
  
  return 0;
  
 fail:
#ifndef GRUB_UTIL
  grub_dl_unref (my_mod);
#endif
  
  grub_free (data);
  
  return grub_errno;;
}


static grub_ssize_t
grub_iso9660_read (grub_file_t file, char *buf, grub_ssize_t len)
{
  struct grub_iso9660_data *data = 
    (struct grub_iso9660_data *) file->data;
  
  /* XXX: The file is stored in as a single extent.  */
  data->disk->read_hook = file->read_hook;	  
  grub_disk_read (data->disk,
		  data->first_sector << GRUB_ISO9660_LOG2_BLKSZ,
		  file->offset,
		  len, buf);
  data->disk->read_hook = 0;
  
  return len;
}


static grub_err_t
grub_iso9660_close (grub_file_t file)
{
  grub_free (file->data);
  
#ifndef GRUB_UTIL
  grub_dl_unref (my_mod);
#endif
  
  return GRUB_ERR_NONE;
}


static grub_err_t
grub_iso9660_label (grub_device_t device, char **label)
{
  struct grub_iso9660_data *data;
  data = grub_iso9660_mount (device->disk);
  
  if (data)
    {
      *label = grub_strndup (data->voldesc.volname, 32);
      grub_free (data);
    }
  else
    *label = 0;

  return grub_errno;
}



static struct grub_fs grub_iso9660_fs =
  {
    .name = "iso9660",
    .dir = grub_iso9660_dir,
    .open = grub_iso9660_open,
    .read = grub_iso9660_read,
    .close = grub_iso9660_close,
    .label = grub_iso9660_label,
    .next = 0
  };

#ifdef GRUB_UTIL
void
grub_iso9660_init (void)
{
  grub_fs_register (&grub_iso9660_fs);
}

void
grub_iso9660_fini (void)
{
  grub_fs_unregister (&grub_iso9660_fs);
}
#else /* ! GRUB_UTIL */
GRUB_MOD_INIT
{
  grub_fs_register (&grub_iso9660_fs);
  my_mod = mod;
}

GRUB_MOD_FINI
{
  grub_fs_unregister (&grub_iso9660_fs);
}
#endif /* ! GRUB_UTIL */
