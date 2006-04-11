/* xfs.c - XFS.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005  Free Software Foundation, Inc.
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

#define	XFS_INODE_EXTENTS	9

#define XFS_INODE_FORMAT_INO	1
#define XFS_INODE_FORMAT_EXT	2
#define XFS_INODE_FORMAT_BTREE	3


struct grub_xfs_sblock
{
  grub_uint8_t magic[4];
  grub_uint32_t bsize;
  grub_uint8_t unused1[48];
  grub_uint64_t rootino;
  grub_uint8_t unused2[20];
  grub_uint32_t agsize; 
  grub_uint8_t unused3[20];
  grub_uint8_t label[12];
  grub_uint8_t log2_bsize;
  grub_uint8_t unused4[2];
  grub_uint8_t log2_inop;
  grub_uint8_t log2_agblk;
} __attribute__ ((packed));

struct grub_xfs_dir_header
{
  grub_uint8_t entries;
  grub_uint8_t smallino;
  grub_uint32_t parent;
} __attribute__ ((packed));

struct grub_xfs_dir_entry
{
  grub_uint8_t len;
  grub_uint16_t offset;
  char name[1];
  /* Inode number follows, 32 bits.  */
} __attribute__ ((packed));

struct grub_xfs_dir2_entry
{
  grub_uint64_t inode;
  grub_uint8_t len;
} __attribute__ ((packed));

typedef grub_uint32_t grub_xfs_extent[4];

struct grub_xfs_inode
{
  grub_uint8_t magic[2];
  grub_uint16_t mode;
  grub_uint8_t version;
  grub_uint8_t format;
  grub_uint8_t unused2[50];
  grub_uint64_t size;
  grub_uint8_t unused3[36];
  union
  {
    char raw[156];
    struct dir
    {
      struct grub_xfs_dir_header dirhead;
      struct grub_xfs_dir_entry direntry[1];
    } dir;
    grub_xfs_extent extents[XFS_INODE_EXTENTS];
  } data __attribute__ ((packed));
} __attribute__ ((packed));

struct grub_xfs_dirblock_tail
{
  grub_uint32_t leaf_count;
  grub_uint32_t leaf_stale;
} __attribute__ ((packed));

struct grub_fshelp_node
{
  struct grub_xfs_data *data;
  struct grub_xfs_inode inode;
  grub_uint64_t ino;
  int inode_read;
};

struct grub_xfs_data
{
  struct grub_xfs_sblock sblock;
  struct grub_xfs_inode *inode;
  grub_disk_t disk;
  int pos;
  int bsize;
  int agsize;
  struct grub_fshelp_node diropen;

};

#ifndef GRUB_UTIL
static grub_dl_t my_mod;
#endif



/* Filetype information as used in inodes.  */
#define FILETYPE_INO_MASK	0170000
#define FILETYPE_INO_REG	0100000
#define FILETYPE_INO_DIRECTORY	0040000
#define FILETYPE_INO_SYMLINK	0120000

#define GRUB_XFS_INO_AGBITS(data)		\
  ((data)->sblock.log2_agblk + (data)->sblock.log2_inop)
#define GRUB_XFS_INO_INOINAG(data, ino)		\
  (grub_be_to_cpu64 (ino) & ((1 << GRUB_XFS_INO_AGBITS (data)) - 1))
#define GRUB_XFS_INO_AG(data,ino)		\
  (grub_be_to_cpu64 (ino) >> GRUB_XFS_INO_AGBITS (data))

#define GRUB_XFS_EXTENT_OFFSET(inode,ex) \
	((grub_be_to_cpu32 ((inode)->data.extents[ex][0]) & ~(1 << 31)) << 23 \
	| grub_be_to_cpu32 ((inode)->data.extents[ex][1]) >> 9)

#define GRUB_XFS_EXTENT_BLOCK(inode,ex)		\
  ((grub_uint64_t) (grub_be_to_cpu32 ((inode)->data.extents[ex][1]) \
	  	  & (~255)) << 43 \
   | (grub_uint64_t) grub_be_to_cpu32 ((inode)->data.extents[ex][2]) << 11 \
   | grub_be_to_cpu32 ((inode)->data.extents[ex][3]) >> 21)

#define GRUB_XFS_EXTENT_SIZE(inode,ex)		\
  (grub_be_to_cpu32 ((inode)->data.extents[ex][3]) & ((1 << 20) - 1))

#define GRUB_XFS_ROUND_TO_DIRENT(pos)	((((pos) + 8 - 1) / 8) * 8)
#define GRUB_XFS_NEXT_DIRENT(pos,len)		\
  (pos) + GRUB_XFS_ROUND_TO_DIRENT (8 + 1 + len + 2)

static inline int
grub_xfs_inode_block (struct grub_xfs_data *data,
		      grub_uint64_t ino)
{
  long long int inoinag = GRUB_XFS_INO_INOINAG (data, ino);
  long long ag = GRUB_XFS_INO_AG (data, ino);
  long long block;

  block = (inoinag >> 4) + ag * data->agsize;
  block <<= (data->sblock.log2_bsize - GRUB_DISK_SECTOR_BITS);
  return block;
}


static inline int
grub_xfs_inode_offset (struct grub_xfs_data *data,
		       grub_uint64_t ino)
{
  int inoag = GRUB_XFS_INO_INOINAG (data, ino);
  return (inoag & ((1 << 4) - 1)) << 8;
}


static grub_err_t
grub_xfs_read_inode (struct grub_xfs_data *data, grub_uint64_t ino,
		     struct grub_xfs_inode *inode)
{
  int block = grub_xfs_inode_block (data, ino);
  int offset = grub_xfs_inode_offset (data, ino);

  /* Read the inode.  */
  if (grub_disk_read (data->disk, block, offset,
		      sizeof (struct grub_xfs_inode), (char *) inode))
    return grub_errno;

  if (grub_strncmp (inode->magic, "IN", 2))
    return grub_error (GRUB_ERR_BAD_FS, "not a correct XFS inode.\n");

  return 0;
}


static int
grub_xfs_read_block (grub_fshelp_node_t node, int fileblock)
{
  int ex;

  if (node->inode.format != XFS_INODE_FORMAT_EXT)
    {
      grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
		  "xfs does not support inode format %d yet",
		  node->inode.format);
      return 0;
    }

  /* Iterate over each extent to figure out which extent has
     the block we are looking for.  */
  for (ex = 0; ex < XFS_INODE_EXTENTS; ex++)
    {
      grub_uint64_t start = GRUB_XFS_EXTENT_BLOCK (&node->inode, ex);
      int offset = GRUB_XFS_EXTENT_OFFSET (&node->inode, ex);
      int size = GRUB_XFS_EXTENT_SIZE (&node->inode, ex);

      unsigned int ag = start >> node->data->sblock.log2_agblk;
      unsigned int block = start & ((1 << node->data->sblock.log2_agblk) - 1);

      if (fileblock < offset + size)
	return (fileblock - offset + block) + ag * node->data->agsize;
    }

  grub_error (GRUB_ERR_FILE_READ_ERROR,
	      "xfs block %d for inode %d is not in an extent.\n",
	      fileblock, grub_be_to_cpu64 (node->ino));
  return 0;
}


/* Read LEN bytes from the file described by DATA starting with byte
   POS.  Return the amount of read bytes in READ.  */
static grub_ssize_t
grub_xfs_read_file (grub_fshelp_node_t node,
		     void (*read_hook) (unsigned long sector,
					unsigned offset, unsigned length),
		     int pos, unsigned int len, char *buf)
{
  return grub_fshelp_read_file (node->data->disk, node, read_hook,
				pos, len, buf, grub_xfs_read_block,
				grub_be_to_cpu64 (node->inode.size),
				node->data->sblock.log2_bsize
				- GRUB_DISK_SECTOR_BITS);
}


static char *
grub_xfs_read_symlink (grub_fshelp_node_t node)
{
  int size = grub_be_to_cpu64 (node->inode.size);

  switch (node->inode.format)
    {
    case XFS_INODE_FORMAT_INO:
      return grub_strndup (node->inode.data.raw, size);

    case XFS_INODE_FORMAT_EXT:
      {
	char *symlink;
	grub_ssize_t numread;

	symlink = grub_malloc (size + 1);
	if (!symlink)
	  return 0;

	numread = grub_xfs_read_file (node, 0, 0, size, symlink);
	if (numread != size)
	  {
	    grub_free (symlink);
	    return 0;
	  }
	symlink[size] = '\0';
	return symlink;
      }
    }

  return 0;
}


static enum grub_fshelp_filetype
grub_xfs_mode_to_filetype (grub_uint16_t mode)
{
  if ((grub_be_to_cpu16 (mode)
       & FILETYPE_INO_MASK) == FILETYPE_INO_DIRECTORY)
    return GRUB_FSHELP_DIR;
  else if ((grub_be_to_cpu16 (mode)
	    & FILETYPE_INO_MASK) == FILETYPE_INO_SYMLINK)
    return GRUB_FSHELP_SYMLINK;
  else if ((grub_be_to_cpu16 (mode)
	    & FILETYPE_INO_MASK) == FILETYPE_INO_REG)
    return GRUB_FSHELP_REG;
  return GRUB_FSHELP_UNKNOWN;
}


static int
grub_xfs_iterate_dir (grub_fshelp_node_t dir,
		       int NESTED_FUNC_ATTR
		       (*hook) (const char *filename,
				enum grub_fshelp_filetype filetype,
				grub_fshelp_node_t node))
{
  struct grub_fshelp_node *diro = (struct grub_fshelp_node *) dir;
  auto int call_hook (grub_uint64_t ino, char *filename);
    
  int call_hook (grub_uint64_t ino, char *filename)
    {
      struct grub_fshelp_node *fdiro;

      fdiro = grub_malloc (sizeof (struct grub_fshelp_node));
      if (!fdiro)
	return 0;
	      
      /* The inode should be read, otherwise the filetype can
	 not be determined.  */
      fdiro->ino = ino;
      fdiro->inode_read = 1;
      fdiro->data = diro->data;
      grub_xfs_read_inode (diro->data, ino, &fdiro->inode);

      return hook (filename,
		   grub_xfs_mode_to_filetype (fdiro->inode.mode),
		   fdiro);
    }
  
  switch (diro->inode.format)
    {
    case XFS_INODE_FORMAT_INO:
      {
	struct grub_xfs_dir_entry *de = &diro->inode.data.dir.direntry[0];
	int smallino = !diro->inode.data.dir.dirhead.smallino;
	int i;
	grub_uint64_t parent;

	/* If small inode numbers are used to pack the direntry, the
	   parent inode number is small too.  */
	if (smallino)
	  {
	    parent = grub_be_to_cpu32 (diro->inode.data.dir.dirhead.parent);
	    parent = grub_cpu_to_be64 (parent);
	  }
	else
	  {
	    parent = *(grub_uint64_t *) &diro->inode.data.dir.dirhead.parent;
	    /* The header is a bit bigger than usual.  */
	    de = (struct grub_xfs_dir_entry *) ((char *) de + 4);
	  }

	/* Synthesize the direntries for `.' and `..'.  */
	if (call_hook (diro->ino, "."))
	  return 1;

	if (call_hook (parent, ".."))
	  return 1;

	for (i = 0; i < diro->inode.data.dir.dirhead.entries; i++)
	  {
	    grub_uint64_t ino;
	    void *inopos = (((char *) de)
			    + sizeof (struct grub_xfs_dir_entry)
			    + de->len - 1);
	    char name[de->len + 1];
	    
	    if (smallino)
	      {
		ino = grub_be_to_cpu32 (*(grub_uint32_t *) inopos);
		ino = grub_cpu_to_be64 (ino);
	      }
	    else
	      ino = *(grub_uint64_t *) inopos;

	    grub_memcpy (name, de->name, de->len);
	    name[de->len] = '\0';
	    if (call_hook (ino, name))
	      return 1;

	    de = ((struct grub_xfs_dir_entry *) 
		  (((char *) de)+ sizeof (struct grub_xfs_dir_entry) + de->len
		   + ((smallino ? sizeof (grub_uint32_t)
		       : sizeof (grub_uint64_t))) - 1));
	  }
	break;
      }

    case XFS_INODE_FORMAT_BTREE:
    case XFS_INODE_FORMAT_EXT:
      {
	grub_ssize_t numread;
	char *dirblock;
	grub_uint64_t blk;

	dirblock = grub_malloc (dir->data->bsize);
	if (! dirblock)
	  return 0;

	/* Iterate over every block the directory has.  */
	for (blk = 0;
	     blk < (grub_be_to_cpu64 (dir->inode.size) 
		    >> dir->data->sblock.log2_bsize);
	     blk++)
	  {
	    /* The header is skipped, the first direntry is stored
	       from byte 16.  */
	    int pos = 16;
	    int entries;
	    int tail_start = (dir->data->bsize
			      - sizeof (struct grub_xfs_dirblock_tail));

	    struct grub_xfs_dirblock_tail *tail;
	    tail = (struct grub_xfs_dirblock_tail *) &dirblock[tail_start];

	    numread = grub_xfs_read_file (dir, 0,
					  blk << dir->data->sblock.log2_bsize,
					  dir->data->bsize, dirblock);

	    entries = (grub_be_to_cpu32 (tail->leaf_count)
		       - grub_be_to_cpu32 (tail->leaf_stale));

	    /* Iterate over all entries within this block.  */
	    while (pos < (dir->data->bsize
			  - (int) sizeof (struct grub_xfs_dir2_entry)))
	      {
		struct grub_xfs_dir2_entry *direntry;
		grub_uint16_t *freetag;
		char *filename;

		direntry = (struct grub_xfs_dir2_entry *) &dirblock[pos];
		freetag = (grub_uint16_t *) direntry;

		if (*freetag == 0XFFFF)
		  {
		    grub_uint16_t *skip = (grub_uint16_t *) (freetag + 1);

		    /* This entry is not used, go to the next one.  */
		    pos += grub_be_to_cpu16 (*skip);

		    continue;
		  }

		filename = &dirblock[pos + sizeof (*direntry)];
		/* The byte after the filename is for the tag, which
		   is not used by GRUB.  So it can be overwritten.  */
		filename[direntry->len] = '\0';

		if (call_hook (direntry->inode, filename))
		  {
		    grub_free (dirblock);
		    return 1;
		  }

		/* Check if last direntry in this block is
		   reached.  */
		entries--;
		if (!entries)
		  break;

		/* Select the next directory entry.  */
		pos = GRUB_XFS_NEXT_DIRENT (pos, direntry->len);
		pos = GRUB_XFS_ROUND_TO_DIRENT (pos);
	      }
	  }
	grub_free (dirblock);
	break;
      }

    default:
      grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
		  "xfs does not support inode format %d yet",
		  diro->inode.format);
    }
  return 0;
}


static struct grub_xfs_data *
grub_xfs_mount (grub_disk_t disk)
{
  struct grub_xfs_data *data = 0;

  data = grub_malloc (sizeof (struct grub_xfs_data));
  if (!data)
    return 0;

  /* Read the superblock.  */
  if (grub_disk_read (disk, 0, 0,
		      sizeof (struct grub_xfs_sblock), (char *) &data->sblock))
    goto fail;
  
  if (grub_strncmp (data->sblock.magic, "XFSB", 4))
    {
      grub_error (GRUB_ERR_BAD_FS, "not a xfs filesystem");
      goto fail;
    }

  data->diropen.data = data;
  data->diropen.ino = data->sblock.rootino;
  data->diropen.inode_read = 1;
  data->bsize = grub_be_to_cpu32 (data->sblock.bsize);
  data->agsize = grub_be_to_cpu32 (data->sblock.agsize);

  data->disk = disk;
  data->inode = &data->diropen.inode;
  data->pos = 0;

  grub_xfs_read_inode (data, data->diropen.ino, data->inode);

  return data;
 fail:
  
  grub_free (data);
  
  return 0;
}


static grub_err_t
grub_xfs_dir (grub_device_t device, const char *path, 
	      int (*hook) (const char *filename, int dir))
{
  struct grub_xfs_data *data = 0;;
  struct grub_fshelp_node *fdiro = 0;
  
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
  
  data = grub_xfs_mount (device->disk);
  if (!data)
    goto fail;
  
  grub_fshelp_find_file (path, &data->diropen, &fdiro, grub_xfs_iterate_dir,
			 grub_xfs_read_symlink, GRUB_FSHELP_DIR);
  if (grub_errno)
    goto fail;

  grub_xfs_iterate_dir (fdiro, iterate);
  
 fail:
  if (fdiro != &data->diropen)
    grub_free (fdiro);
  grub_free (data);

#ifndef GRUB_UTIL
  grub_dl_unref (my_mod);
#endif

  return grub_errno;

  return 0;
}


/* Open a file named NAME and initialize FILE.  */
static grub_err_t
grub_xfs_open (struct grub_file *file, const char *name)
{
  struct grub_xfs_data *data;
  struct grub_fshelp_node *fdiro = 0;
  
#ifndef GRUB_UTIL
  grub_dl_ref (my_mod);
#endif
  
  data = grub_xfs_mount (file->device->disk);
  if (!data)
    goto fail;
  
  grub_fshelp_find_file (name, &data->diropen, &fdiro, grub_xfs_iterate_dir,
			 grub_xfs_read_symlink, GRUB_FSHELP_REG);
  if (grub_errno)
    goto fail;
  
  if (!fdiro->inode_read)
    {
      grub_xfs_read_inode (data, fdiro->ino, &fdiro->inode);
      if (grub_errno)
	goto fail;
    }
  
  grub_memcpy (data->inode,
	       &fdiro->inode,
	       sizeof (struct grub_xfs_inode));
  grub_free (fdiro);

  file->size = grub_be_to_cpu64 (data->inode->size);
  file->data = data;
  file->offset = 0;

  return 0;

 fail:
  if (fdiro != &data->diropen)
    grub_free (fdiro);
  grub_free (data);
  
#ifndef GRUB_UTIL
  grub_dl_unref (my_mod);
#endif

  return grub_errno;
}


static grub_ssize_t
grub_xfs_read (grub_file_t file, char *buf, grub_ssize_t len)
{
  struct grub_xfs_data *data = 
    (struct grub_xfs_data *) file->data;

  return grub_xfs_read_file (&data->diropen, file->read_hook,
			      file->offset, len, buf);
}


static grub_err_t
grub_xfs_close (grub_file_t file)
{
  grub_free (file->data);

#ifndef GRUB_UTIL
  grub_dl_unref (my_mod);
#endif

  return GRUB_ERR_NONE;
}


static grub_err_t
grub_xfs_label (grub_device_t device, char **label)
{
  struct grub_xfs_data *data;
  grub_disk_t disk = device->disk;

#ifndef GRUB_UTIL
  grub_dl_ref (my_mod);
#endif

  data = grub_xfs_mount (disk);
  if (data)
    *label = grub_strndup (data->sblock.label, 12);
  else
    *label = 0;

#ifndef GRUB_UTIL
  grub_dl_unref (my_mod);
#endif

  grub_free (data);

  return grub_errno;
}



static struct grub_fs grub_xfs_fs =
  {
    .name = "xfs",
    .dir = grub_xfs_dir,
    .open = grub_xfs_open,
    .read = grub_xfs_read,
    .close = grub_xfs_close,
    .label = grub_xfs_label,
    .next = 0
  };

#ifdef GRUB_UTIL
void
grub_xfs_init (void)
{
  grub_fs_register (&grub_xfs_fs);
}

void
grub_xfs_fini (void)
{
  grub_fs_unregister (&grub_xfs_fs);
}
#else /* ! GRUB_UTIL */
GRUB_MOD_INIT
{
  grub_fs_register (&grub_xfs_fs);
  my_mod = mod;
}

GRUB_MOD_FINI
{
  grub_fs_unregister (&grub_xfs_fs);
}
#endif /* ! GRUB_UTIL */
