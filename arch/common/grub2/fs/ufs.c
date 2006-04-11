/* ufs.c - Unix File System */
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


#define GRUB_UFS_MAGIC		0x11954
#define GRUB_UFS2_MAGIC		0x19540119
#define GRUB_UFS_INODE		2
#define GRUB_UFS_FILETYPE_DIR	4
#define GRUB_UFS_FILETYPE_LNK	10
#define GRUB_UFS_MAX_SYMLNK_CNT	8

#define GRUB_UFS_DIRBLKS	12
#define GRUB_UFS_INDIRBLKS	3

#define GRUB_UFS_ATTR_DIR	040000

/* Calculate in which group the inode can be found.  */
#define inode_group(inode,sblock) ()

#define UFS_BLKSZ(sblock) (grub_le_to_cpu32 (sblock->bsize))

#define INODE(data,field) (data->ufs_type == UFS1 ? \
                           data->inode.  field : data->inode2.  field)
#define INODE_ENDIAN(data,field,bits1,bits2) (data->ufs_type == UFS1 ? \
                           grub_le_to_cpu##bits1 (data->inode.field) : \
                           grub_le_to_cpu##bits2 (data->inode2.field))
#define INODE_SIZE(data) INODE_ENDIAN (data,size,32,64)
#define INODE_MODE(data) INODE_ENDIAN (data,mode,16,16)
#define INODE_BLKSZ(data) (data->ufs_type == UFS1 ? 32 : 64)
#define INODE_DIRBLOCKS(data,blk) INODE_ENDIAN \
                                   (data,blocks.dir_blocks[blk],32,64)
#define INODE_INDIRBLOCKS(data,blk) INODE_ENDIAN \
                                     (data,blocks.indir_blocks[blk],32,64)

/* The blocks on which the superblock can be found.  */
static int sblocklist[] = { 128, 16, 0, 512, -1 };

struct grub_ufs_sblock
{
  grub_uint8_t unused[16];
  /* The offset of the inodes in the cylinder group.  */
  grub_uint32_t inoblk_offs;
  
  grub_uint8_t unused2[4];
  
  /* The start of the cylinder group.  */
  grub_uint32_t cylg_offset;
  
  grub_uint8_t unused3[20];
  
  /* The size of a block in bytes.  */
  grub_int32_t bsize;
  grub_uint8_t unused4[48];
  
  /* The size of filesystem blocks to disk blocks.  */
  grub_uint32_t log2_blksz;
  grub_uint8_t unused5[80];
  
  /* Inodes stored per cylinder group.  */
  grub_uint32_t ino_per_group;
  
  /* The frags per cylinder group.  */
  grub_uint32_t frags_per_group;
  
  grub_uint8_t unused7[1180];
  
  /* Magic value to check if this is really a UFS filesystem.  */
  grub_uint32_t magic;
};

/* UFS inode.  */
struct grub_ufs_inode
{
  grub_uint16_t mode;
  grub_uint16_t nlinks;
  grub_uint16_t uid;
  grub_uint16_t gid;
  grub_int64_t size;
  grub_uint64_t atime;
  grub_uint64_t mtime;
  grub_uint64_t ctime;
  union
  {
    struct
    {
      grub_uint32_t dir_blocks[GRUB_UFS_DIRBLKS];
      grub_uint32_t indir_blocks[GRUB_UFS_INDIRBLKS];
    } blocks;
    grub_uint8_t symlink[(GRUB_UFS_DIRBLKS + GRUB_UFS_INDIRBLKS) * 4];
  };
  grub_uint32_t flags;
  grub_uint32_t nblocks;
  grub_uint32_t gen;
  grub_uint32_t unused;
  grub_uint8_t pad[12];
};

/* UFS inode.  */
struct grub_ufs2_inode
{
  grub_uint16_t mode;
  grub_uint16_t nlinks;
  grub_uint32_t uid;
  grub_uint32_t gid;
  grub_uint32_t blocksize;
  grub_int64_t size;
  grub_int64_t nblocks;
  grub_uint64_t atime;
  grub_uint64_t mtime;
  grub_uint64_t ctime;
  grub_uint64_t create_time;
  grub_uint32_t atime_sec;
  grub_uint32_t mtime_sec;
  grub_uint32_t ctime_sec;
  grub_uint32_t create_time_sec;
  grub_uint32_t gen;
  grub_uint32_t kernel_flags;
  grub_uint32_t flags;
  grub_uint32_t extsz;
  grub_uint64_t ext[2];
  union
  {
    struct
    {
      grub_uint64_t dir_blocks[GRUB_UFS_DIRBLKS];
      grub_uint64_t indir_blocks[GRUB_UFS_INDIRBLKS];
    } blocks;
    grub_uint8_t symlink[(GRUB_UFS_DIRBLKS + GRUB_UFS_INDIRBLKS) * 8];
  };

  grub_uint8_t unused[24];
};

/* Directory entry.  */
struct grub_ufs_dirent
{
  grub_uint32_t ino;
  grub_uint16_t direntlen;
  grub_uint8_t filetype;
  grub_uint8_t namelen;
};

/* Information about a "mounted" ufs filesystem.  */
struct grub_ufs_data
{
  struct grub_ufs_sblock sblock;
  grub_disk_t disk;
  union
  {
    struct grub_ufs_inode inode;
    struct grub_ufs2_inode inode2;
  };
  enum
    {
      UFS1,
      UFS2,
      UNKNOWN
    } ufs_type;
  int ino;
  int linknest;
};

#ifndef GRUB_UTIL
static grub_dl_t my_mod;
#endif

/* Forward declaration.  */
static grub_err_t grub_ufs_find_file (struct grub_ufs_data *data,
				      const char *path);


static int
grub_ufs_get_file_block (struct grub_ufs_data *data, unsigned int blk)
{
  struct grub_ufs_sblock *sblock = &data->sblock;
  unsigned int indirsz;
  
  /* Direct.  */
  if (blk < GRUB_UFS_DIRBLKS)
    return INODE_DIRBLOCKS (data, blk);
  
  blk -= GRUB_UFS_DIRBLKS;
  
  indirsz = UFS_BLKSZ (sblock) / INODE_BLKSZ (data);
  /* Single indirect block.  */
  if (blk < indirsz)
    {
      grub_uint32_t indir[UFS_BLKSZ (sblock)];
      grub_disk_read (data->disk, INODE_INDIRBLOCKS (data, 0),
		      0, sizeof (indir), (char *) indir);
      return indir[blk];
    }
  blk -= indirsz;
  
  /* Double indirect block.  */
  if (blk < UFS_BLKSZ (sblock) / indirsz)
    {
      grub_uint32_t indir[UFS_BLKSZ (sblock)];
      
      grub_disk_read (data->disk, INODE_INDIRBLOCKS (data, 1),
		      0, sizeof (indir), (char *) indir);
      grub_disk_read (data->disk,  indir[blk / indirsz],
		      0, sizeof (indir), (char *) indir);
      
      return indir[blk % indirsz];
    }


  grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
	      "ufs does not support tripple indirect blocks");
  return 0;
}


/* Read LEN bytes from the file described by DATA starting with byte
   POS.  Return the amount of read bytes in READ.  */
static grub_ssize_t
grub_ufs_read_file (struct grub_ufs_data *data,
		    void (*read_hook) (unsigned long sector,
				       unsigned offset, unsigned length),
		    int pos, unsigned int len, char *buf)
{
  struct grub_ufs_sblock *sblock = &data->sblock;
  int i;
  int blockcnt;

  /* Adjust len so it we can't read past the end of the file.  */
  if (len > INODE_SIZE (data))
    len = INODE_SIZE (data);

  blockcnt = (len + pos + UFS_BLKSZ (sblock) - 1) / UFS_BLKSZ (sblock);
  
  for (i = pos / UFS_BLKSZ (sblock); i < blockcnt; i++)
    {
      int blknr;
      int blockoff = pos % UFS_BLKSZ (sblock);
      int blockend = UFS_BLKSZ (sblock);
      
      int skipfirst = 0;
      
      blknr = grub_ufs_get_file_block (data, i);
      if (grub_errno)
	return -1;
      
      /* Last block.  */
      if (i == blockcnt - 1)
	{
	  blockend = (len + pos) % UFS_BLKSZ (sblock);
	  
	  if (!blockend)
	    blockend = UFS_BLKSZ (sblock);
	}
      
      /* First block.  */
      if (i == (pos / (int) UFS_BLKSZ (sblock)))
	{
	  skipfirst = blockoff;
	  blockend -= skipfirst;
	}
      
      /* XXX: If the block number is 0 this block is not stored on
	 disk but is zero filled instead.  */
      if (blknr)
	{
	  data->disk->read_hook = read_hook;
	  grub_disk_read (data->disk,
			  blknr << grub_le_to_cpu32 (data->sblock.log2_blksz),
			  skipfirst, blockend, buf);
	  data->disk->read_hook = 0;
	  if (grub_errno)
	    return -1;
	}
      else
	grub_memset (buf, UFS_BLKSZ (sblock) - skipfirst, 0);

      buf += UFS_BLKSZ (sblock) - skipfirst;
    }
  
  return len;
}


/* Read inode INO from the mounted filesystem described by DATA.  This
   inode is used by default now.  */
static grub_err_t
grub_ufs_read_inode (struct grub_ufs_data *data, int ino)
{
  struct grub_ufs_sblock *sblock = &data->sblock;
  
  /* Determine the group the inode is in.  */
  int group = ino / grub_le_to_cpu32 (sblock->ino_per_group);
  
  /* Determine the inode within the group.  */
  int grpino = ino % grub_le_to_cpu32 (sblock->ino_per_group);
  
  /* The first block of the group.  */
  int grpblk = group * (grub_le_to_cpu32 (sblock->frags_per_group));
  
  if (data->ufs_type == UFS1)
    {
      struct grub_ufs_inode *inode = &data->inode;
      
      grub_disk_read (data->disk,
		      (((grub_le_to_cpu32 (sblock->inoblk_offs) + grpblk)
			<< grub_le_to_cpu32 (data->sblock.log2_blksz)))
		      + grpino / 4,
		      (grpino % 4) * sizeof (struct grub_ufs_inode),
		      sizeof (struct grub_ufs_inode),
		      (char *) inode);
    }
  else
    {
      struct grub_ufs2_inode *inode = &data->inode2;
      
      grub_disk_read (data->disk,
		      (((grub_le_to_cpu32 (sblock->inoblk_offs) + grpblk)
			<< grub_le_to_cpu32 (data->sblock.log2_blksz)))
		      + grpino / 2,
		      (grpino % 2) * sizeof (struct grub_ufs2_inode),
		      sizeof (struct grub_ufs2_inode),
		      (char *) inode);
    }
  
  data->ino = ino;
  return grub_errno;
}


/* Lookup the symlink the current inode points to.  INO is the inode
   number of the directory the symlink is relative to.  */
static grub_err_t
grub_ufs_lookup_symlink (struct grub_ufs_data *data, int ino)
{
  char symlink[INODE_SIZE (data)];
  
  if (++data->linknest > GRUB_UFS_MAX_SYMLNK_CNT)
    return grub_error (GRUB_ERR_SYMLINK_LOOP, "too deep nesting of symlinks");
  
  if (INODE_SIZE (data) < (GRUB_UFS_DIRBLKS + GRUB_UFS_INDIRBLKS
			  * INODE_BLKSZ (data)))
    grub_strcpy (symlink, INODE (data, symlink));
  else
    {
      grub_disk_read (data->disk, 
		      (INODE_DIRBLOCKS (data, 0) 
		       << grub_le_to_cpu32 (data->sblock.log2_blksz)),
		      0, INODE_SIZE (data), symlink);
      symlink[INODE_SIZE (data)] = '\0';
    }

  /* The symlink is an absolute path, go back to the root inode.  */
  if (symlink[0] == '/')
    ino = GRUB_UFS_INODE;
  
  /* Now load in the old inode.  */
  if (grub_ufs_read_inode (data, ino))
    return grub_errno;
  
  grub_ufs_find_file (data, symlink);
  if (grub_errno)
    grub_error (grub_errno, "Can not follow symlink `%s'.", symlink);
  
  return grub_errno;
}


/* Find the file with the pathname PATH on the filesystem described by
   DATA.  */
static grub_err_t
grub_ufs_find_file (struct grub_ufs_data *data, const char *path)
{
  char fpath[grub_strlen (path)];
  char *name = fpath;
  char *next;
  unsigned int pos = 0;
  int dirino;
  
  grub_strncpy (fpath, path, grub_strlen (path));
  
  /* Skip the first slash.  */
  if (name[0] == '/')
    {
      name++;
      if (!*name)
	return 0;
    }

  /* Extract the actual part from the pathname.  */
  next = grub_strchr (name, '/');
  if (next)
    {
      next[0] = '\0';
      next++;
    }
  
  do
    {
      struct grub_ufs_dirent dirent;
      
      if (grub_strlen (name) == 0)
	return GRUB_ERR_NONE;
      
      if (grub_ufs_read_file (data, 0, pos, sizeof (dirent),
			      (char *) &dirent) < 0)
	return grub_errno;
      
      {
	char filename[dirent.namelen + 1];

	if (grub_ufs_read_file (data, 0, pos + sizeof (dirent),
				dirent.namelen, filename) < 0)
	  return grub_errno;
	
	filename[dirent.namelen] = '\0';
	
	if (!grub_strcmp (name, filename))
	  {
	    dirino = data->ino;
	    grub_ufs_read_inode (data, grub_le_to_cpu32 (dirent.ino));
	    
	    if (dirent.filetype == GRUB_UFS_FILETYPE_LNK)
	      {
		grub_ufs_lookup_symlink (data, dirino);
		if (grub_errno)
		  return grub_errno;
	      }

	    if (!next)
	      return 0;

	    pos = 0;

	    name = next;
	    next = grub_strchr (name, '/');
	    if (next)
	      {
		next[0] = '\0';
		next++;
	      }
	    
	    if (!(dirent.filetype & GRUB_UFS_FILETYPE_DIR))
	      return grub_error (GRUB_ERR_BAD_FILE_TYPE, "not a directory");
	    
	    continue;
	  }
      }
      
      pos += grub_le_to_cpu16 (dirent.direntlen);
    } while (pos < grub_le_to_cpu32 (INODE_SIZE (data)));
  
  grub_error (GRUB_ERR_FILE_NOT_FOUND, "file not found");
  return grub_errno;
}


/* Mount the filesystem on the disk DISK.  */
static struct grub_ufs_data *
grub_ufs_mount (grub_disk_t disk)
{
  struct grub_ufs_data *data;
  int *sblklist = sblocklist;
  
  data = grub_malloc (sizeof (struct grub_ufs_data));
  if (!data)
    return 0;
  
  /* Find a UFS1 or UFS2 sblock.  */
  data->ufs_type = UNKNOWN;
  while (*sblklist != -1)
    {
      grub_disk_read (disk, *sblklist, 0, sizeof (struct grub_ufs_sblock),
		      (char *) &data->sblock);
      if (grub_errno)
	goto fail;
      
      if (grub_le_to_cpu32 (data->sblock.magic) == GRUB_UFS_MAGIC)
	{
	  data->ufs_type = UFS1;
	  break;
	}
      else if (grub_le_to_cpu32 (data->sblock.magic) == GRUB_UFS2_MAGIC)
	{
	  data->ufs_type = UFS2;
	  break;
	}
      sblklist++;
    }
  if (data->ufs_type == UNKNOWN)
    {
      grub_error (GRUB_ERR_BAD_FS, "not an ufs filesystem");
      goto fail;
    }

  data->disk = disk;
  data->linknest = 0;
  return data;

 fail:
  grub_free (data);
  
  if (grub_errno == GRUB_ERR_OUT_OF_RANGE)
    grub_error (GRUB_ERR_BAD_FS, "not a ufs filesystem");
  
  return 0;
}


static grub_err_t
grub_ufs_dir (grub_device_t device, const char *path, 
	       int (*hook) (const char *filename, int dir))
{
  struct grub_ufs_data *data;
  struct grub_ufs_sblock *sblock;
  unsigned int pos = 0;

  data = grub_ufs_mount (device->disk);
  if (!data)
    return grub_errno;
  
  grub_ufs_read_inode (data, GRUB_UFS_INODE);
  if (grub_errno)
    return grub_errno;
  
  sblock = &data->sblock;
  
  if (!path || path[0] != '/')
    {
      grub_error (GRUB_ERR_BAD_FILENAME, "bad filename");
      return grub_errno;
    }
  
  grub_ufs_find_file (data, path);
  if (grub_errno)
    goto fail;  
  
  if (!(INODE_MODE (data) & GRUB_UFS_ATTR_DIR))
    {
      grub_error (GRUB_ERR_BAD_FILE_TYPE, "not a directory");
      goto fail;
    }
  
  while (pos < INODE_SIZE (data))
    {
      struct grub_ufs_dirent dirent;
      
      if (grub_ufs_read_file (data, 0, pos, sizeof (dirent),
			      (char *) &dirent) < 0)
	break;
      
      {
	char filename[dirent.namelen + 1];
	
	if (grub_ufs_read_file (data, 0, pos + sizeof (dirent),
				dirent.namelen, filename) < 0)
	  break;
	
	filename[dirent.namelen] = '\0';
	if (hook (filename, dirent.filetype == GRUB_UFS_FILETYPE_DIR))
	  break;
      }
      
      pos += grub_le_to_cpu16 (dirent.direntlen);
    }

 fail:
  grub_free (data);

  return grub_errno;
}


/* Open a file named NAME and initialize FILE.  */
static grub_err_t
grub_ufs_open (struct grub_file *file, const char *name)
{
  struct grub_ufs_data *data;
  data = grub_ufs_mount (file->device->disk);
  if (!data)
    return grub_errno;
  
  grub_ufs_read_inode (data, 2);
  if (grub_errno)
    {
      grub_free (data);
      return grub_errno;
    }
    
  if (!name || name[0] != '/')
    {
      grub_error (GRUB_ERR_BAD_FILENAME, "bad filename");
      return grub_errno;
    }
  
  grub_ufs_find_file (data, name);
  if (grub_errno)
    {
      grub_free (data);
      return grub_errno;
    }
  
  file->data = data;
  file->size = INODE_SIZE (data);

  return GRUB_ERR_NONE;
}


static grub_ssize_t
grub_ufs_read (grub_file_t file, char *buf, grub_ssize_t len)
{
  struct grub_ufs_data *data = 
    (struct grub_ufs_data *) file->data;
  
  return grub_ufs_read_file (data, file->read_hook, file->offset, len, buf);
}


static grub_err_t
grub_ufs_close (grub_file_t file)
{
  grub_free (file->data);
  
  return GRUB_ERR_NONE;
}


static grub_err_t
grub_ufs_label (grub_device_t device __attribute ((unused)),
		char **label __attribute ((unused)))
{
  return GRUB_ERR_NONE;
}


static struct grub_fs grub_ufs_fs =
  {
    .name = "ufs",
    .dir = grub_ufs_dir,
    .open = grub_ufs_open,
    .read = grub_ufs_read,
    .close = grub_ufs_close,
    .label = grub_ufs_label,
    .next = 0
  };

#ifdef GRUB_UTIL
void
grub_ufs_init (void)
{
  grub_fs_register (&grub_ufs_fs);
}

void
grub_ufs_fini (void)
{
  grub_fs_unregister (&grub_ufs_fs);
}
#else /* ! GRUB_UTIL */
GRUB_MOD_INIT
{
  grub_fs_register (&grub_ufs_fs);
  my_mod = mod;
}

GRUB_MOD_FINI
{
  grub_fs_unregister (&grub_ufs_fs);
}
#endif /* ! GRUB_UTIL */
