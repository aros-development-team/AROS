/* hfs.c - HFS.  */
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

#define	GRUB_HFS_SBLOCK		2
#define GRUB_HFS_MAGIC		0x4244

#define GRUB_HFS_BLKS		(data->blksz >> 9)

#define GRUB_HFS_NODE_LEAF	0xFF

/* The two supported filesystems a record can have.  */
enum
  {
    GRUB_HFS_FILETYPE_DIR = 1,
    GRUB_HFS_FILETYPE_FILE = 2
  };

/* A single extent.  A file consists of suchs extents.  */
struct grub_hfs_extent
{
  /* The first physical block.  */
  grub_uint16_t first_block;
  grub_uint16_t count;
};

/* HFS stores extents in groups of 3.  */
typedef struct grub_hfs_extent grub_hfs_datarecord_t[3];

/* The HFS superblock (The official name is `Master Directory
   Block').  */
struct grub_hfs_sblock
{
  grub_uint16_t magic;
  grub_uint8_t unused[18];
  grub_uint32_t blksz;
  grub_uint8_t unused2[4];
  grub_uint16_t first_block;
  grub_uint8_t unused4[6];

  /* A pascal style string that holds the volumename.  */
  grub_uint8_t volname[28];
  
  grub_uint8_t unused5[70];
  grub_hfs_datarecord_t extent_recs;
  grub_uint32_t catalog_size;
  grub_hfs_datarecord_t catalog_recs;
} __attribute__ ((packed));

/* A node desciptor.  This is the header of every node.  */
struct grub_hfs_node
{
  grub_uint32_t next;
  grub_uint32_t prev;
  grub_uint8_t type;
  grub_uint8_t level;
  grub_uint16_t reccnt;
  grub_uint16_t unused;
} __attribute__ ((packed));

/* The head of the B*-Tree.  */
struct grub_hfs_treeheader
{
  grub_uint16_t tree_depth;
  /* The number of the first node.  */
  grub_uint32_t root_node;
  grub_uint32_t leaves;
  grub_uint32_t first_leaf;
  grub_uint32_t last_leaf;
  grub_uint16_t node_size;
  grub_uint16_t key_size;
  grub_uint32_t nodes;
  grub_uint32_t free_nodes;
  grub_uint8_t unused[76];
} __attribute__ ((packed));

/* The state of a mounted HFS filesystem.  */
struct grub_hfs_data
{
  struct grub_hfs_sblock sblock;
  grub_disk_t disk;
  grub_hfs_datarecord_t extents;
  int fileid;
  int size;
  int ext_root;
  int ext_size;
  int cat_root;
  int cat_size;
  int blksz;
  int log2_blksz;
  int rootdir;
};

/* The key as used on disk in a catalog tree.  This is used to lookup
   file/directory nodes by parent directory ID and filename.  */
struct grub_hfs_catalog_key
{
  grub_uint8_t unused;
  grub_uint32_t parent_dir;
  
  /* Filename length.  */
  grub_uint8_t strlen;

  /* Filename.  */
  grub_uint8_t str[31];
} __attribute__ ((packed));

/* The key as used on disk in a extent overflow tree.  Using this key
   the extents can be looked up using a fileid and logical start block
   as index.  */
struct grub_hfs_extent_key
{
  /* The kind of fork.  This is used to store meta information like
     icons, attributes, etc.  We will only use the datafork, which is
     0.  */
  grub_uint8_t forktype;
  grub_uint32_t fileid;
  grub_uint16_t first_block;
} __attribute__ ((packed));

/* A dirrect record.  This is used to find out the directory ID.  */
struct grub_hfs_dirrec
{
  /* For a directory, type == 1.  */
  grub_uint8_t type;
  grub_uint8_t unused[5];
  grub_uint32_t dirid;
} __attribute__ ((packed));

/* Information about a file.  */
struct grub_hfs_filerec
{
  /* For a file, type == 2.  */
  grub_uint8_t type;
  grub_uint8_t unused[19];
  grub_uint32_t fileid;
  grub_uint8_t unused2[2];
  grub_uint32_t size;
  grub_uint8_t unused3[44];

  /* The first 3 extents of the file.  The other extents can be found
     in the extent overflow file.  */
  grub_hfs_datarecord_t extents;
} __attribute__ ((packed));

/* A record descriptor, both key and data, used to pass to call back
   functions.  */
struct grub_hfs_record
{
  void *key;
  int keylen;
  void *data;
  int datalen;
};

#ifndef GRUB_UTIL
static grub_dl_t my_mod;
#endif

static int grub_hfs_find_node (struct grub_hfs_data *, char *,
			       grub_uint32_t, int, char *, int);

/* Find block BLOCK of the file FILE in the mounted UFS filesystem
   DATA.  The first 3 extents are described by DAT.  If cache is set,
   using caching to improve non-random reads.  */
static unsigned int
grub_hfs_block (struct grub_hfs_data *data, grub_hfs_datarecord_t dat,
		int file, int block, int cache)
{
  grub_hfs_datarecord_t dr;
  int pos = 0;
  struct grub_hfs_extent_key key;
  
  int tree = 0;
  static int cache_file = 0;
  static int cache_pos = 0;  
  static grub_hfs_datarecord_t cache_dr;
  
  grub_memcpy (dr, dat, sizeof (dr));
  
  key.forktype = 0;
  key.fileid = grub_cpu_to_be32 (file);
  
  if (cache && cache_file == file  && block > cache_pos)
    {
      pos = cache_pos;
      key.first_block = grub_cpu_to_be16 (pos);
      grub_memcpy (dr, cache_dr, sizeof (cache_dr));
    }
  
  for (;;)
    {
      int i;
      
      /* Try all 3 extents.  */
      for (i = 0; i < 3; i++)
	{
	  /* Check if the block is stored in this extent.  */
	  if (grub_be_to_cpu16 (dr[i].count) + pos > block)
	    {
	      int first = grub_be_to_cpu16 (dr[i].first_block);
	      
	      /* If the cache is enabled, store the current position
		 in the tree.  */
	      if (tree && cache)
		{
		  cache_file = file;
		  cache_pos = pos;
		  grub_memcpy (cache_dr, dr, sizeof (cache_dr));
		}
	      
	      return (grub_be_to_cpu16 (data->sblock.first_block)
		      + (first + block - pos) * GRUB_HFS_BLKS);
	    }
	  
	  /* Try the next extent.  */
	  pos += grub_be_to_cpu16 (dr[i].count);
	}
      
      /* Lookup the block in the extent overflow file.  */
      key.first_block = grub_cpu_to_be16 (pos);
      tree = 1;
      grub_hfs_find_node (data, (char *) &key, data->ext_root,
			  1, (char *) &dr, sizeof (dr));
      if (grub_errno)
	return 0;
    }
}


/* Read LEN bytes from the file described by DATA starting with byte
   POS.  Return the amount of read bytes in READ.  */
static grub_ssize_t
grub_hfs_read_file (struct grub_hfs_data *data,
		    void (*read_hook) (unsigned long sector,
				       unsigned offset, unsigned length),
		     int pos, unsigned int len, char *buf)
{
  int i;
  int blockcnt;

  /* Adjust len so it we can't read past the end of the file.  */
  if (len > grub_le_to_cpu32 (data->size))
    len = grub_le_to_cpu32 (data->size);

  blockcnt = ((len + pos) 
	      + data->blksz - 1) / data->blksz;

  for (i = pos / data->blksz; i < blockcnt; i++)
    {
      int blknr;
      int blockoff = pos % data->blksz;
      int blockend = data->blksz;

      int skipfirst = 0;
      
      blknr = grub_hfs_block (data, data->extents, data->fileid, i, 1);
      if (grub_errno)
	return -1;
      
      /* Last block.  */
      if (i == blockcnt - 1)
	{
	  blockend = (len + pos) % data->blksz;
	  
	  /* The last portion is exactly EXT2_BLOCK_SIZE (data).  */
	  if (!blockend)
	    blockend = data->blksz;
	}

      /* First block.  */
      if (i == pos / data->blksz)
	{
	  skipfirst = blockoff;
	  blockend -= skipfirst;
	}

      /* If the block number is 0 this block is not stored on disk but
	 is zero filled instead.  */
      if (blknr)
	{
	  data->disk->read_hook = read_hook;	  
	  grub_disk_read (data->disk, blknr, skipfirst,
			  blockend, buf);
	  data->disk->read_hook = 0;
	  if (grub_errno)
	    return -1;
	}
      
      buf += data->blksz - skipfirst;
    }
  
  return len;
}


/* Mount the filesystem on the disk DISK.  */
static struct grub_hfs_data *
grub_hfs_mount (grub_disk_t disk)
{
  struct grub_hfs_data *data;
  struct grub_hfs_catalog_key key;
  struct grub_hfs_dirrec dir;
  int first_block;
  
  struct
  {
    struct grub_hfs_node node;
    struct grub_hfs_treeheader head;
  } treehead;
  
  data = grub_malloc (sizeof (struct grub_hfs_data));
  if (!data)
    return 0;

  /* Read the superblock.  */
  if (grub_disk_read (disk, GRUB_HFS_SBLOCK, 0,
		      sizeof (struct grub_hfs_sblock), (char *) &data->sblock))
    goto fail;
  
  /* Check if this is a HFS filesystem.  */
  if (grub_be_to_cpu16 (data->sblock.magic) != GRUB_HFS_MAGIC)
    {
      grub_error (GRUB_ERR_BAD_FS, "not a hfs filesystem");
      goto fail;
    }
  
  data->blksz = grub_be_to_cpu32 (data->sblock.blksz);
  data->disk = disk;
  
  /* Lookup the root node of the extent overflow tree.  */
  first_block = ((grub_be_to_cpu16 (data->sblock.extent_recs[0].first_block) 
		  * GRUB_HFS_BLKS)
		 + grub_be_to_cpu16 (data->sblock.first_block));
  
  if (grub_disk_read (data->disk, first_block, 0,
		      sizeof (treehead), (char *)  &treehead))
    goto fail;
  data->ext_root = grub_be_to_cpu32 (treehead.head.root_node);
  data->ext_size = grub_be_to_cpu16 (treehead.head.node_size);
  
  /* Lookup the root node of the catalog tree.  */
  first_block = ((grub_be_to_cpu16 (data->sblock.catalog_recs[0].first_block) 
		  * GRUB_HFS_BLKS)
		 + grub_be_to_cpu16 (data->sblock.first_block));
  if (grub_disk_read (data->disk, first_block, 0,
		      sizeof (treehead), (char *)  &treehead))
    goto fail;
  data->cat_root = grub_be_to_cpu32 (treehead.head.root_node);
  data->cat_size = grub_be_to_cpu16 (treehead.head.node_size);
  
  /* Lookup the root directory node in the catalog tree using the
     volume name.  */
  key.parent_dir = grub_cpu_to_be32 (1);
  key.strlen = data->sblock.volname[0];
  grub_strcpy (key.str, data->sblock.volname + 1);
  
  if (grub_hfs_find_node (data, (char *) &key, data->cat_root,
			  0, (char *) &dir, sizeof (dir)) == 0)
    {
      grub_error (GRUB_ERR_BAD_FS, "can not find the hfs root directory");
      goto fail;
    }
    
  if (grub_errno)
    goto fail;

  data->rootdir = grub_be_to_cpu32 (dir.dirid);
  
  return data;
 fail:
  grub_free (data);
  
  if (grub_errno == GRUB_ERR_OUT_OF_RANGE)
    grub_error (GRUB_ERR_BAD_FS, "not a hfs filesystem");
  
  return 0;
}


/* Compare the K1 and K2 catalog file keys.  */
static int
grub_hfs_cmp_catkeys (struct grub_hfs_catalog_key *k1,
		      struct grub_hfs_catalog_key *k2)
{
  int cmp = (grub_be_to_cpu32 (k1->parent_dir)
	     - grub_be_to_cpu32 (k2->parent_dir));
  
  if (cmp != 0)
    return cmp;
  
  cmp = grub_strncasecmp (k1->str, k2->str, k1->strlen);
  
  /* This is required because the compared strings are not of equal
     length.  */
  if (cmp == 0 && k1->strlen < k2->strlen)
    return -1;
  return cmp;
}


/* Compare the K1 and K2 extent overflow file keys.  */
static int
grub_hfs_cmp_extkeys (struct grub_hfs_extent_key *k1,
		      struct grub_hfs_extent_key *k2)
{
  int cmp = k1->forktype - k2->forktype;
  if (cmp == 0)
    cmp = grub_be_to_cpu32 (k1->fileid) - grub_be_to_cpu32 (k2->fileid);
  if (cmp == 0)
    cmp = (grub_be_to_cpu16 (k1->first_block) 
	   - grub_be_to_cpu16 (k2->first_block));
  return cmp;
}


/* Iterate the records in the node with index IDX in the mounted HFS
   filesystem DATA.  This node holds data of the type TYPE (0 =
   catalog node, 1 = extent overflow node).  If this is set, continue
   iterating to the next node.  For every records, call NODE_HOOK.  */
static grub_err_t
grub_hfs_iterate_records (struct grub_hfs_data *data, int type, int idx,
			  int this, int (*node_hook) (struct grub_hfs_node *hnd,
						      struct grub_hfs_record *))
{
  int nodesize = type == 0 ? data->cat_size : data->ext_size;
  
  union
  {
    struct grub_hfs_node node;
    char rawnode[nodesize];
    grub_uint16_t offsets[nodesize / 2];
  } node;
  
  do
    {
      int i;
      struct grub_hfs_extent *dat;
      int blk;
      
      dat = (struct grub_hfs_extent *) (type == 0 
					? (&data->sblock.catalog_recs)
					: (&data->sblock.extent_recs));
      
      /* Read the node into memory.  */
      blk = grub_hfs_block (data, dat,
			    0, idx / (data->blksz / nodesize), 0);
      blk += (idx % (data->blksz / nodesize));
      if (grub_errno)
	return grub_errno;
      
      if (grub_disk_read (data->disk, blk, 0,
			  sizeof (node), (char *)  &node))
	return grub_errno;
      
      /* Iterate over all records in this node.  */
      for (i = 0; i < grub_be_to_cpu16 (node.node.reccnt); i++)
	{
	  int pos = (nodesize >> 1) - 1 - i;
 	  struct pointer
	  {
	    grub_uint8_t keylen;
	    grub_uint8_t key;
	  } __attribute__ ((packed)) *pnt;
	  pnt = (struct pointer *) (grub_be_to_cpu16 (node.offsets[pos])
				    + node.rawnode);
	  
	  struct grub_hfs_record rec = 
	    {
	      &pnt->key,
	      pnt->keylen,
	      &pnt->key + pnt->keylen +(pnt->keylen + 1) % 2,
	      nodesize - grub_be_to_cpu16 (node.offsets[pos]) 
	      - pnt->keylen - 1
	    };
	  
	  if (node_hook (&node.node, &rec))
	    return 0;
	}
      
      if (idx % (data->blksz / nodesize) == 0)
	idx = grub_be_to_cpu32 (node.node.next);
      else
	idx++;
    } while (idx && this);
  
  return 0;
}


/* Lookup a record in the mounted filesystem DATA using the key KEY.
   The index of the node on top of the tree is IDX.  The tree is of
   the type TYPE (0 = catalog node, 1 = extent overflow node).  Return
   the data in DATAR with a maximum length of DATALEN.  */
static int
grub_hfs_find_node (struct grub_hfs_data *data, char *key,
		    grub_uint32_t idx, int type, char *datar, int datalen)
{
  int found = -1;
  int isleaf = 0;
  
  auto int node_found (struct grub_hfs_node *, struct grub_hfs_record *);
    
  int node_found (struct grub_hfs_node *hnd, struct grub_hfs_record *rec)
    {
      int cmp = 1;
      
      if (type == 0)
	cmp = grub_hfs_cmp_catkeys (rec->key, (void *) key);
      else
	cmp = grub_hfs_cmp_extkeys (rec->key, (void *) key);
      
      /* If the key is smaller or equal to the currect node, mark the
	 entry.  In case of a non-leaf mode it will be used to lookup
	 the rest of the tree.  */
      if (cmp <= 0)
	{
	  grub_uint32_t *node = (grub_uint32_t *) rec->data;
	  found = grub_be_to_cpu32 (*node);
	}
      else /* The key can not be found in the tree. */
	return 1;
      
      /* Check if this node is a leaf node.  */
      if (hnd->type == GRUB_HFS_NODE_LEAF)
	{
	  isleaf = 1;
	  
	  /* Found it!!!!  */
	  if (cmp == 0)
	    {
	      grub_memcpy (datar, rec->data,
			   rec->datalen < datalen ? rec->datalen : datalen);
	      return 1;
	    }
	}
      
      return 0;
    }
  
  if (grub_hfs_iterate_records (data, type, idx, 0, node_found))
    return 0;
  
  if (found == -1)
    return 0;

  if (isleaf)
    return 1;
  
  return grub_hfs_find_node (data, key, found, type, datar, datalen);
}


/* Iterate over the directory with the id DIR.  The tree is searched
   starting with the node ROOT_IDX.  For every entry in this directory
   call HOOK.  */
static grub_err_t
grub_hfs_iterate_dir (struct grub_hfs_data *data, grub_uint32_t root_idx,
		      unsigned int dir, int (*hook) (struct grub_hfs_record *))
{
  int found = -1;
  int isleaf = 0;
  int next = 0;
  
  /* The lowest key possible with DIR as root directory.  */
  struct grub_hfs_catalog_key key = {0, grub_cpu_to_be32 (dir), 0, ""};

  auto int node_found (struct grub_hfs_node *, struct grub_hfs_record *);
  auto int it_dir (struct grub_hfs_node * __attribute ((unused)),
		   struct grub_hfs_record *);

  
  int node_found (struct grub_hfs_node *hnd, struct grub_hfs_record *rec)
    {
      struct grub_hfs_catalog_key *ckey = rec->key;
      
      if (grub_hfs_cmp_catkeys (rec->key, (void *) &key) <= 0)
	found = grub_be_to_cpu32 (*(grub_uint32_t *) rec->data);
      
      if (hnd->type == 0xFF && ckey->strlen > 0)
	{
	  isleaf = 1;
	  next = grub_be_to_cpu32 (hnd->next);
	  
	  /* An entry was found.  */
	  if (grub_be_to_cpu32 (ckey->parent_dir) == dir)
	    return hook (rec);
	}
      
      return 0;
    }
  
  int it_dir (struct grub_hfs_node *hnd __attribute ((unused)),
	      struct grub_hfs_record *rec)
    {
      struct grub_hfs_catalog_key *ckey = rec->key;
      struct grub_hfs_catalog_key *origkey = &key;
      
      /* Stop when the entries do not match anymore.  */
      if (grub_be_to_cpu32 (ckey->parent_dir) 
	  != grub_be_to_cpu32 ((origkey)->parent_dir))
	return 1;
      
      return hook (rec);
    }
  
  if (grub_hfs_iterate_records (data, 0, root_idx, 0, node_found))
    return grub_errno;
  
  if (found == -1)
    return 0;
  
  /* If there was a matching record in this leaf node, continue the
     iteration until the last record was found.  */
  if (isleaf)
    {
      grub_hfs_iterate_records (data, 0, next, 1, it_dir);
      return grub_errno;
    }
  
  return grub_hfs_iterate_dir (data, found, dir, hook);
}


/* Find a file or directory with the pathname PATH in the filesystem
   DATA.  Return the file record in RETDATA when it is non-zero.
   Return the directory number in RETINODE when it is non-zero.  */
static grub_err_t
grub_hfs_find_dir (struct grub_hfs_data *data, const char *path,
		   struct grub_hfs_filerec *retdata, int *retinode)
{
  int inode = data->rootdir;
  char *next;
  char *origpath;
  struct grub_hfs_filerec frec;
  struct grub_hfs_dirrec *dir = (struct grub_hfs_dirrec *) &frec;
  frec.type = GRUB_HFS_FILETYPE_DIR;
  
  if (path[0] != '/')
    {
      grub_error (GRUB_ERR_BAD_FILENAME, "bad filename");
      return 0;
    }
  
  origpath = grub_strdup (path);
  if (!origpath)
    return grub_errno;
  
  path = origpath;
  path++;
  
  while (path && grub_strlen (path))
    {
      if (frec.type != GRUB_HFS_FILETYPE_DIR)
	{
	  grub_error (GRUB_ERR_BAD_FILE_TYPE, "not a directory");
	  goto fail;
	}
      
      /* Isolate a part of the path.  */
      next = grub_strchr (path, '/');
      if (next)
	{
	  next[0] = '\0';
	  next++;
	}
      
      struct grub_hfs_catalog_key key;
      
      key.parent_dir = grub_cpu_to_be32 (inode);
      key.strlen = grub_strlen (path);
      grub_strcpy (key.str, path);
      
      /* Lookup this node.  */
      if (!grub_hfs_find_node (data, (char *) &key, data->cat_root,
			       0, (char *) &frec, sizeof (frec)))
	{
	  grub_error (GRUB_ERR_FILE_NOT_FOUND, "file not found");
	  goto fail;
	}

      if (grub_errno)
	goto fail;
      
      inode = grub_be_to_cpu32 (dir->dirid);
      path = next;
    }

  if (retdata)
    grub_memcpy (retdata, &frec, sizeof (frec));
  
  if (retinode)
    *retinode = inode;
  
 fail:
  grub_free (origpath);
  return grub_errno;
}



static grub_err_t
grub_hfs_dir (grub_device_t device, const char *path, 
		  int (*hook) (const char *filename, int dir))
{
  int inode;

  auto int dir_hook (struct grub_hfs_record *rec);

  int dir_hook (struct grub_hfs_record *rec)
    {
      char fname[32] = { 0 };
      char *filetype = rec->data;
      struct grub_hfs_catalog_key *ckey = rec->key;
      
      grub_strncpy (fname, ckey->str, ckey->strlen);
      
      if (*filetype == GRUB_HFS_FILETYPE_DIR)
	return hook (fname, 1);
      else if (*filetype == GRUB_HFS_FILETYPE_FILE)
	return hook (fname, 0);
      return 0;
    }
  
  struct grub_hfs_data *data;
  struct grub_hfs_filerec frec;

#ifndef GRUB_UTIL
  grub_dl_ref (my_mod);
#endif
  
  data = grub_hfs_mount (device->disk);
  if (!data)
    goto fail;
  
  /* First the directory ID for the directory.  */
  if (grub_hfs_find_dir (data, path, &frec, &inode))
    goto fail;

  if (frec.type != GRUB_HFS_FILETYPE_DIR)
    {
      grub_error (GRUB_ERR_BAD_FILE_TYPE, "not a directory");
      goto fail;
    }
  
  grub_hfs_iterate_dir (data, data->cat_root, inode, dir_hook);
  
 fail:
  grub_free (data);

#ifndef GRUB_UTIL
  grub_dl_unref (my_mod);
#endif
  
  return grub_errno;
}


/* Open a file named NAME and initialize FILE.  */
static grub_err_t
grub_hfs_open (struct grub_file *file, const char *name)
{
  struct grub_hfs_data *data;
  struct grub_hfs_filerec frec;
  
#ifndef GRUB_UTIL
  grub_dl_ref (my_mod);
#endif

  data = grub_hfs_mount (file->device->disk);
  
  if (grub_hfs_find_dir (data, name, &frec, 0))
    {
      grub_free (data);
#ifndef GRUB_UTIL
  grub_dl_unref (my_mod);
#endif
      return grub_errno;
    }
  
  if (frec.type != GRUB_HFS_FILETYPE_FILE)
    {
      grub_free (data);
      grub_error (GRUB_ERR_BAD_FILE_TYPE, "not a file");
#ifndef GRUB_UTIL
      grub_dl_unref (my_mod);
#endif
      return grub_errno;
    }
  
  grub_memcpy (data->extents, frec.extents, sizeof (grub_hfs_datarecord_t));
  file->size = grub_be_to_cpu32 (frec.size);
  data->size = grub_be_to_cpu32 (frec.size);
  data->fileid = grub_be_to_cpu32 (frec.fileid);
  file->offset = 0;

  file->data = data;
  
  return 0;
}

static grub_ssize_t
grub_hfs_read (grub_file_t file, char *buf, grub_ssize_t len)
{
  struct grub_hfs_data *data = 
    (struct grub_hfs_data *) file->data;
  
  return grub_hfs_read_file (data, file->read_hook, file->offset, len, buf);
}


static grub_err_t
grub_hfs_close (grub_file_t file)
{
  grub_free (file->data);

#ifndef GRUB_UTIL
  grub_dl_unref (my_mod);
#endif

  return 0;
}


static grub_err_t
grub_hfs_label (grub_device_t device, char **label)
{
  struct grub_hfs_data *data;

  data = grub_hfs_mount (device->disk);
  
  if (data)
    *label = grub_strndup (data->sblock.volname + 1, *data->sblock.volname);
  else
    *label = 0;

  grub_free (data);
  return grub_errno;
}



static struct grub_fs grub_hfs_fs =
  {
    .name = "hfs",
    .dir = grub_hfs_dir,
    .open = grub_hfs_open,
    .read = grub_hfs_read,
    .close = grub_hfs_close,
    .label = grub_hfs_label,
    .next = 0
  };

#ifdef GRUB_UTIL
void
grub_hfs_init (void)
{
  grub_fs_register (&grub_hfs_fs);
}

void
grub_hfs_fini (void)
{
  grub_fs_unregister (&grub_hfs_fs);
}
#else /* ! GRUB_UTIL */
GRUB_MOD_INIT
{
  grub_fs_register (&grub_hfs_fs);
  my_mod = mod;
}

GRUB_MOD_FINI
{
  grub_fs_unregister (&grub_hfs_fs);
}
#endif /* ! GRUB_UTIL */
