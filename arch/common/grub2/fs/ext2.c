/* ext2.c - Second Extended filesystem */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003, 2004  Free Software Foundation, Inc.
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

/* Magic value used to identify an ext2 filesystem.  */
#define	EXT2_MAGIC		0xEF53
/* Amount of indirect blocks in an inode.  */
#define INDIRECT_BLOCKS		12
/* Maximum lenght of a pathname.  */
#define EXT2_PATH_MAX		4096
/* Maximum nesting of symlinks, used to prevent a loop.  */
#define	EXT2_MAX_SYMLINKCNT	8

/* Filetype used in directory entry.  */
#define	FILETYPE_UNKNOWN	0
#define	FILETYPE_REG		1
#define	FILETYPE_DIRECTORY	2
#define	FILETYPE_SYMLINK	7

/* Filetype information as used in inodes.  */
#define FILETYPE_INO_MASK	0170000
#define FILETYPE_INO_REG	0100000
#define FILETYPE_INO_DIRECTORY	0040000
#define FILETYPE_INO_SYMLINK	0120000

#include <grub/err.h>
#include <grub/file.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/disk.h>
#include <grub/dl.h>
#include <grub/types.h>
#include <grub/fshelp.h>

/* Log2 size of ext2 block in 512 blocks.  */
#define LOG2_EXT2_BLOCK_SIZE(data)			\
	(grub_le_to_cpu32 (data->sblock.log2_block_size) + 1)
     
/* Log2 size of ext2 block in bytes.  */
#define LOG2_BLOCK_SIZE(data)					\
	(grub_le_to_cpu32 (data->sblock.log2_block_size) + 10)

/* The size of an ext2 block in bytes.  */
#define EXT2_BLOCK_SIZE(data)		(1 << LOG2_BLOCK_SIZE(data))

/* The ext2 superblock.  */
struct grub_ext2_sblock
{
  grub_uint32_t total_inodes;
  grub_uint32_t total_blocks;
  grub_uint32_t reserved_blocks;
  grub_uint32_t free_blocks;
  grub_uint32_t free_inodes;
  grub_uint32_t first_data_block;
  grub_uint32_t log2_block_size;
  grub_uint32_t log2_fragment_size;
  grub_uint32_t blocks_per_group;
  grub_uint32_t fragments_per_group;
  grub_uint32_t inodes_per_group;
  grub_uint32_t mtime;
  grub_uint32_t utime;
  grub_uint16_t mnt_count;
  grub_uint16_t max_mnt_count;
  grub_uint16_t magic;
  grub_uint16_t fs_state;
  grub_uint16_t error_handling;
  grub_uint16_t minor_revision_level;
  grub_uint32_t lastcheck;
  grub_uint32_t checkinterval;
  grub_uint32_t creator_os;
  grub_uint32_t revision_level;
  grub_uint16_t uid_reserved;
  grub_uint16_t gid_reserved;
  grub_uint32_t first_inode;
  grub_uint16_t inode_size;
  grub_uint16_t block_group_number;
  grub_uint32_t feature_compatibility;
  grub_uint32_t feature_incompat;
  grub_uint32_t feature_ro_compat;
  grub_uint32_t unique_id[4];
  char volume_name[16];
  char last_mounted_on[64];
  grub_uint32_t compression_info;
};

/* The ext2 blockgroup.  */
struct grub_ext2_block_group
{
  grub_uint32_t block_id;
  grub_uint32_t inode_id;
  grub_uint32_t inode_table_id;
  grub_uint16_t free_blocks;
  grub_uint16_t free_inodes;
  grub_uint16_t pad;
  grub_uint32_t reserved[3];
};

/* The ext2 inode.  */
struct grub_ext2_inode
{
  grub_uint16_t mode;
  grub_uint16_t uid;
  grub_uint32_t size;
  grub_uint32_t atime;
  grub_uint32_t ctime;
  grub_uint32_t mtime;
  grub_uint32_t dtime;
  grub_uint16_t gid;
  grub_uint16_t nlinks;
  grub_uint32_t blockcnt;  /* Blocks of 512 bytes!! */
  grub_uint32_t flags;
  grub_uint32_t osd1;
  union
  {
    struct datablocks
    {
      grub_uint32_t dir_blocks[INDIRECT_BLOCKS];
      grub_uint32_t indir_block;
      grub_uint32_t double_indir_block;
      grub_uint32_t tripple_indir_block;
    } blocks;
    char symlink[60];
  };
  grub_uint32_t version;
  grub_uint32_t acl;
  grub_uint32_t dir_acl;
  grub_uint32_t fragment_addr;
  grub_uint32_t osd2[3];
};

/* The header of an ext2 directory entry.  */
struct ext2_dirent
{
  grub_uint32_t inode;
  grub_uint16_t direntlen;
  grub_uint8_t namelen;
  grub_uint8_t filetype;
};

struct grub_fshelp_node
{
  struct grub_ext2_data *data;
  struct grub_ext2_inode inode;
  int ino;
  int inode_read;
};

/* Information about a "mounted" ext2 filesystem.  */
struct grub_ext2_data
{
  struct grub_ext2_sblock sblock;
  grub_disk_t disk;
  struct grub_ext2_inode *inode;
  struct grub_fshelp_node diropen;
};

#ifndef GRUB_UTIL
static grub_dl_t my_mod;
#endif

/* Read into BLKGRP the blockgroup descriptor of blockgroup GROUP of
   the mounted filesystem DATA.  */
inline static grub_err_t
grub_ext2_blockgroup (struct grub_ext2_data *data, int group, 
		      struct grub_ext2_block_group *blkgrp)
{
  return grub_disk_read (data->disk,
			 ((grub_le_to_cpu32 (data->sblock.first_data_block) + 1)
			  << LOG2_EXT2_BLOCK_SIZE (data)),
			 group * sizeof (struct grub_ext2_block_group), 
			 sizeof (struct grub_ext2_block_group), (char *) blkgrp);
}


static int
grub_ext2_read_block (grub_fshelp_node_t node, int fileblock)
{
  struct grub_ext2_data *data = node->data;
  struct grub_ext2_inode *inode = &node->inode;
  int blknr;
  int blksz = EXT2_BLOCK_SIZE (data);
  int log2_blksz = LOG2_EXT2_BLOCK_SIZE (data);
  
  /* Direct blocks.  */
  if (fileblock < INDIRECT_BLOCKS)
    blknr = grub_le_to_cpu32 (inode->blocks.dir_blocks[fileblock]);
  /* Indirect.  */
  else if (fileblock < INDIRECT_BLOCKS + blksz / 4)
    {
      grub_uint32_t indir[blksz / 4];

      if (grub_disk_read (data->disk, 
			  grub_le_to_cpu32 (inode->blocks.indir_block)
			  << log2_blksz,
			  0, blksz, (char *) indir))
	return grub_errno;
	  
      blknr = grub_le_to_cpu32 (indir[fileblock - INDIRECT_BLOCKS]);
    }
  /* Double indirect.  */
  else if (fileblock < INDIRECT_BLOCKS + blksz / 4 * (blksz  / 4 + 1))
    {
      unsigned int perblock = blksz / 4;
      unsigned int rblock = fileblock - (INDIRECT_BLOCKS 
					 + blksz / 4);
      grub_uint32_t indir[blksz / 4];

      if (grub_disk_read (data->disk, 
			  grub_le_to_cpu32 (inode->blocks.double_indir_block) 
			  << log2_blksz,
			  0, blksz, (char *) indir))
	return grub_errno;

      if (grub_disk_read (data->disk,
			  grub_le_to_cpu32 (indir[rblock / perblock])
			  << log2_blksz,
			  0, blksz, (char *) indir))
	return grub_errno;

      
      blknr = grub_le_to_cpu32 (indir[rblock % perblock]);
    }
  /* Tripple indirect.  */
  else
    {
      grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
		  "ext2fs doesn't support tripple indirect blocks");
      return grub_errno;
    }

  return blknr;
}


/* Read LEN bytes from the file described by DATA starting with byte
   POS.  Return the amount of read bytes in READ.  */
static grub_ssize_t
grub_ext2_read_file (grub_fshelp_node_t node,
		     void (*read_hook) (unsigned long sector,
					unsigned offset, unsigned length),
		     int pos, unsigned int len, char *buf)
{
  return grub_fshelp_read_file (node->data->disk, node, read_hook,
				pos, len, buf, grub_ext2_read_block,
				node->inode.size,
				LOG2_EXT2_BLOCK_SIZE (node->data));
    
}


/* Read the inode INO for the file described by DATA into INODE.  */
static grub_err_t
grub_ext2_read_inode (struct grub_ext2_data *data,
		      int ino, struct grub_ext2_inode *inode)
{
  struct grub_ext2_block_group blkgrp;
  struct grub_ext2_sblock *sblock = &data->sblock;
  int inodes_per_block;
  
  unsigned int blkno;
  unsigned int blkoff;

  /* It is easier to calculate if the first inode is 0.  */
  ino--;
  
  grub_ext2_blockgroup (data, ino / grub_le_to_cpu32 (sblock->inodes_per_group),
			&blkgrp);
  if (grub_errno)
    return grub_errno;

  inodes_per_block = EXT2_BLOCK_SIZE (data) / 128;
  blkno = (ino % grub_le_to_cpu32 (sblock->inodes_per_group))
    / inodes_per_block;
  blkoff = (ino % grub_le_to_cpu32 (sblock->inodes_per_group))
    % inodes_per_block;
  
  /* Read the inode.  */
  if (grub_disk_read (data->disk, 
		      ((grub_le_to_cpu32 (blkgrp.inode_table_id) + blkno)
		       << LOG2_EXT2_BLOCK_SIZE (data)),
		      sizeof (struct grub_ext2_inode) * blkoff,
		      sizeof (struct grub_ext2_inode), (char *) inode))
    return grub_errno;
  
  return 0;
}

static struct grub_ext2_data *
grub_ext2_mount (grub_disk_t disk)
{
  struct grub_ext2_data *data;

  data = grub_malloc (sizeof (struct grub_ext2_data));
  if (!data)
    return 0;

  /* Read the superblock.  */
  grub_disk_read (disk, 1 * 2, 0, sizeof (struct grub_ext2_sblock),
			(char *) &data->sblock);
  if (grub_errno)
    goto fail;

  /* Make sure this is an ext2 filesystem.  */
  if (grub_le_to_cpu16 (data->sblock.magic) != EXT2_MAGIC)
    goto fail;
  
  data->diropen.data = data;
  data->diropen.ino = 2;
  data->diropen.inode_read = 1;

  data->inode = &data->diropen.inode;
  data->disk = disk;

  grub_ext2_read_inode (data, 2, data->inode);
  if (grub_errno)
    goto fail;
  
  return data;

 fail:
  grub_error (GRUB_ERR_BAD_FS, "not an ext2 filesystem");
  grub_free (data);
  return 0;
}

static char *
grub_ext2_read_symlink (grub_fshelp_node_t node)
{
  char *symlink;
  struct grub_fshelp_node *diro = node;
  
  if (!diro->inode_read)
    {
      grub_ext2_read_inode (diro->data, diro->ino, &diro->inode);
      if (grub_errno)
	return 0;
    }
  
  symlink = grub_malloc (grub_le_to_cpu32 (diro->inode.size) + 1);
  if (!symlink)
    return 0;
  
  /* If the filesize of the symlink is bigger than
     60 the symlink is stored in a separate block,
     otherwise it is stored in the inode.  */
  if (grub_le_to_cpu32 (diro->inode.size) <= 60)
    grub_strncpy (symlink, 
		  diro->inode.symlink,
		  grub_le_to_cpu32 (diro->inode.size));
  else
    {
      grub_ext2_read_file (diro, 0, 0,
			   grub_le_to_cpu32 (diro->inode.size),
			   symlink);
      if (grub_errno)
	{
	  grub_free (symlink);
	  return 0;
	}
    }
  
  symlink[grub_le_to_cpu32 (diro->inode.size)] = '\0';
  return symlink;
}

static int
grub_ext2_iterate_dir (grub_fshelp_node_t dir,
		       int NESTED_FUNC_ATTR
		       (*hook) (const char *filename,
				enum grub_fshelp_filetype filetype,
				grub_fshelp_node_t node))
{
  unsigned int fpos = 0;
  struct grub_fshelp_node *diro = (struct grub_fshelp_node *) dir;
  
  if (!diro->inode_read)
    {
      grub_ext2_read_inode (diro->data, diro->ino, &diro->inode);
      if (grub_errno)
	return 0;
    }
  
  /* Search the file.  */
  while (fpos < grub_le_to_cpu32 (diro->inode.size))
    {
      struct ext2_dirent dirent;

      grub_ext2_read_file (diro, 0, fpos, sizeof (struct ext2_dirent),
			   (char *) &dirent);
      if (grub_errno)
	return 0;
      
      if (dirent.namelen != 0)
	{
	  char filename[dirent.namelen + 1];
	  struct grub_fshelp_node *fdiro;
	  enum grub_fshelp_filetype type = GRUB_FSHELP_UNKNOWN;
	  
	  grub_ext2_read_file (diro, 0, fpos + sizeof (struct ext2_dirent),
			       dirent.namelen, filename);
	  if (grub_errno)
	    return 0;
	  
	  fdiro = grub_malloc (sizeof (struct grub_fshelp_node));
	  if (!fdiro)
	    return 0;
	  
	  fdiro->data = diro->data;
	  fdiro->ino = grub_le_to_cpu32 (dirent.inode);
	  
	  filename[dirent.namelen] = '\0';

	  if (dirent.filetype != FILETYPE_UNKNOWN)
	    {
	      fdiro->inode_read = 0;

	      if (dirent.filetype == FILETYPE_DIRECTORY)
		type = GRUB_FSHELP_DIR;
	      else if (dirent.filetype == FILETYPE_SYMLINK)
		type = GRUB_FSHELP_SYMLINK;
	      else if (dirent.filetype == FILETYPE_REG)
		type = GRUB_FSHELP_REG;
	    }
	  else
	    {
	      /* The filetype can not be read from the dirent, read
		 the inode to get more information.  */
	      grub_ext2_read_inode (diro->data, grub_le_to_cpu32 (dirent.inode),
				    &fdiro->inode);
	      if (grub_errno)
		{
		  grub_free (fdiro);
		  return 0;
		}
	      
	      fdiro->inode_read = 1;
	      
	      if ((grub_le_to_cpu16 (fdiro->inode.mode)
		   & FILETYPE_INO_MASK) == FILETYPE_INO_DIRECTORY)
		type = GRUB_FSHELP_DIR;
	      else if ((grub_le_to_cpu16 (fdiro->inode.mode)
			& FILETYPE_INO_MASK) == FILETYPE_INO_SYMLINK)
		type = GRUB_FSHELP_SYMLINK;
	      else if ((grub_le_to_cpu16 (fdiro->inode.mode)
			& FILETYPE_INO_MASK) == FILETYPE_INO_REG)
		type = GRUB_FSHELP_REG;
	    }
	  
	  if (hook (filename, type, fdiro))
	    return 1;
	}
      
      fpos += grub_le_to_cpu16 (dirent.direntlen);
    }
  
  return 0;
}

/* Open a file named NAME and initialize FILE.  */
static grub_err_t
grub_ext2_open (struct grub_file *file, const char *name)
{
  struct grub_ext2_data *data;
  struct grub_fshelp_node *fdiro = 0;
  
#ifndef GRUB_UTIL
  grub_dl_ref (my_mod);
#endif
  
  data = grub_ext2_mount (file->device->disk);
  if (!data)
    goto fail;
  
  grub_fshelp_find_file (name, &data->diropen, &fdiro, grub_ext2_iterate_dir,
			 grub_ext2_read_symlink, GRUB_FSHELP_REG);
  if (grub_errno)
    goto fail;
  
  if (!fdiro->inode_read)
    {
      grub_ext2_read_inode (data, fdiro->ino, &fdiro->inode);
      if (grub_errno)
	goto fail;
    }
  
  grub_memcpy (data->inode, &fdiro->inode, sizeof (struct grub_ext2_inode));
  grub_free (fdiro);

  file->size = grub_le_to_cpu32 (data->inode->size);
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

static grub_err_t
grub_ext2_close (grub_file_t file)
{
  grub_free (file->data);

#ifndef GRUB_UTIL
  grub_dl_unref (my_mod);
#endif

  return GRUB_ERR_NONE;
}

/* Read LEN bytes data from FILE into BUF.  */
static grub_ssize_t
grub_ext2_read (grub_file_t file, char *buf, grub_ssize_t len)
{
  struct grub_ext2_data *data = 
    (struct grub_ext2_data *) file->data;
  
  return grub_ext2_read_file (&data->diropen, file->read_hook,
			      file->offset, len, buf);
}


static grub_err_t
grub_ext2_dir (grub_device_t device, const char *path, 
	       int (*hook) (const char *filename, int dir))
{
  struct grub_ext2_data *data = 0;;
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
  
  data = grub_ext2_mount (device->disk);
  if (!data)
    goto fail;
  
  grub_fshelp_find_file (path, &data->diropen, &fdiro, grub_ext2_iterate_dir,
			 grub_ext2_read_symlink, GRUB_FSHELP_DIR);
  if (grub_errno)
    goto fail;
  
  grub_ext2_iterate_dir (fdiro, iterate);
  
 fail:
  if (fdiro != &data->diropen)
    grub_free (fdiro);
  grub_free (data);

#ifndef GRUB_UTIL
  grub_dl_unref (my_mod);
#endif

  return grub_errno;
}

static grub_err_t
grub_ext2_label (grub_device_t device, char **label)
{
  struct grub_ext2_data *data;
  grub_disk_t disk = device->disk;

#ifndef GRUB_UTIL
  grub_dl_ref (my_mod);
#endif

  data = grub_ext2_mount (disk);
  if (data)
    *label = grub_strndup (data->sblock.volume_name, 14);
  else
    *label = 0;

#ifndef GRUB_UTIL
  grub_dl_unref (my_mod);
#endif

  grub_free (data);

  return grub_errno;
}


static struct grub_fs grub_ext2_fs =
  {
    .name = "ext2",
    .dir = grub_ext2_dir,
    .open = grub_ext2_open,
    .read = grub_ext2_read,
    .close = grub_ext2_close,
    .label = grub_ext2_label,
    .next = 0
  };

#ifdef GRUB_UTIL
void
grub_ext2_init (void)
{
  grub_fs_register (&grub_ext2_fs);
}

void
grub_ext2_fini (void)
{
  grub_fs_unregister (&grub_ext2_fs);
}
#else /* ! GRUB_UTIL */
GRUB_MOD_INIT
{
  grub_fs_register (&grub_ext2_fs);
  my_mod = mod;
}

GRUB_MOD_FINI
{
  grub_fs_unregister (&grub_ext2_fs);
}
#endif /* ! GRUB_UTIL */
