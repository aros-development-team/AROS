/* hfsplus.c - HFS+ Filesystem.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2006,2007,2008,2009  Free Software Foundation, Inc.
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

/* HFS+ is documented at http://developer.apple.com/technotes/tn/tn1150.html */

#include <grub/err.h>
#include <grub/file.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/disk.h>
#include <grub/dl.h>
#include <grub/types.h>
#include <grub/fshelp.h>
#include <grub/hfs.h>
#include <grub/charset.h>

GRUB_MOD_LICENSE ("GPLv3+");

#define GRUB_HFSPLUS_MAGIC 0x482B
#define GRUB_HFSPLUSX_MAGIC 0x4858
#define GRUB_HFSPLUS_SBLOCK 2

/* A HFS+ extent.  */
struct grub_hfsplus_extent
{
  /* The first block of a file on disk.  */
  grub_uint32_t start;
  /* The amount of blocks described by this extent.  */
  grub_uint32_t count;
} __attribute__ ((packed));

/* The descriptor of a fork.  */
struct grub_hfsplus_forkdata
{
  grub_uint64_t size;
  grub_uint32_t clumpsize;
  grub_uint32_t blocks;
  struct grub_hfsplus_extent extents[8];
} __attribute__ ((packed));

/* The HFS+ Volume Header.  */
struct grub_hfsplus_volheader
{
  grub_uint16_t magic;
  grub_uint16_t version;
  grub_uint32_t attributes;
  grub_uint8_t unused1[12];
  grub_uint32_t utime;
  grub_uint8_t unused2[16];
  grub_uint32_t blksize;
  grub_uint8_t unused3[60];
  grub_uint64_t num_serial;
  struct grub_hfsplus_forkdata allocations_file;
  struct grub_hfsplus_forkdata extents_file;
  struct grub_hfsplus_forkdata catalog_file;
  struct grub_hfsplus_forkdata attrib_file;
  struct grub_hfsplus_forkdata startup_file;
} __attribute__ ((packed));

/* The type of node.  */
enum grub_hfsplus_btnode_type
  {
    GRUB_HFSPLUS_BTNODE_TYPE_LEAF = -1,
    GRUB_HFSPLUS_BTNODE_TYPE_INDEX = 0,
    GRUB_HFSPLUS_BTNODE_TYPE_HEADER = 1,
    GRUB_HFSPLUS_BTNODE_TYPE_MAP = 2,
  };

struct grub_hfsplus_btnode
{
  grub_uint32_t next;
  grub_uint32_t prev;
  grub_int8_t type;
  grub_uint8_t height;
  grub_uint16_t count;
  grub_uint16_t unused;
} __attribute__ ((packed));

/* The header of a HFS+ B+ Tree.  */
struct grub_hfsplus_btheader
{
  grub_uint16_t depth;
  grub_uint32_t root;
  grub_uint32_t leaf_records;
  grub_uint32_t first_leaf_node;
  grub_uint32_t last_leaf_node;
  grub_uint16_t nodesize;
  grub_uint16_t keysize;
  grub_uint32_t total_nodes;
  grub_uint32_t free_nodes;
  grub_uint16_t reserved1;
  grub_uint32_t clump_size;  /* ignored */
  grub_uint8_t btree_type;
  grub_uint8_t key_compare;
  grub_uint32_t attributes;
} __attribute__ ((packed));

/* The on disk layout of a catalog key.  */
struct grub_hfsplus_catkey
{
  grub_uint16_t keylen;
  grub_uint32_t parent;
  grub_uint16_t namelen;
  grub_uint16_t name[30];
} __attribute__ ((packed));

/* The on disk layout of an extent overflow file key.  */
struct grub_hfsplus_extkey
{
  grub_uint16_t keylen;
  grub_uint8_t type;
  grub_uint8_t unused;
  grub_uint32_t fileid;
  grub_uint32_t start;
} __attribute__ ((packed));

struct grub_hfsplus_key
{
  union
  {
    struct grub_hfsplus_extkey extkey;
    struct grub_hfsplus_catkey catkey;
    grub_uint16_t keylen;
  };
} __attribute__ ((packed));

struct grub_hfsplus_catfile
{
  grub_uint16_t type;
  grub_uint16_t flags;
  grub_uint32_t parentid; /* Thread only.  */
  grub_uint32_t fileid;
  grub_uint8_t unused1[4];
  grub_uint32_t mtime;
  grub_uint8_t unused2[22];
  grub_uint16_t mode;
  grub_uint8_t unused3[44];
  struct grub_hfsplus_forkdata data;
  struct grub_hfsplus_forkdata resource;
} __attribute__ ((packed));

/* Filetype information as used in inodes.  */
#define GRUB_HFSPLUS_FILEMODE_MASK	0170000
#define GRUB_HFSPLUS_FILEMODE_REG	0100000
#define GRUB_HFSPLUS_FILEMODE_DIRECTORY	0040000
#define GRUB_HFSPLUS_FILEMODE_SYMLINK	0120000

/* Some pre-defined file IDs.  */
#define GRUB_HFSPLUS_FILEID_ROOTDIR	2
#define GRUB_HFSPLUS_FILEID_OVERFLOW	3
#define GRUB_HFSPLUS_FILEID_CATALOG	4

enum grub_hfsplus_filetype
  {
    GRUB_HFSPLUS_FILETYPE_DIR = 1,
    GRUB_HFSPLUS_FILETYPE_REG = 2,
    GRUB_HFSPLUS_FILETYPE_DIR_THREAD = 3,
    GRUB_HFSPLUS_FILETYPE_REG_THREAD = 4
  };

#define GRUB_HFSPLUSX_BINARYCOMPARE	0xBC
#define GRUB_HFSPLUSX_CASEFOLDING	0xCF

/* Internal representation of a catalog key.  */
struct grub_hfsplus_catkey_internal
{
  grub_uint32_t parent;
  const grub_uint16_t *name;
  grub_size_t namelen;
};

/* Internal representation of an extent overflow key.  */
struct grub_hfsplus_extkey_internal
{
  grub_uint32_t fileid;
  grub_uint8_t type;
  grub_uint32_t start;
};

struct grub_hfsplus_key_internal
{
  union
  {
    struct grub_hfsplus_extkey_internal extkey;
    struct grub_hfsplus_catkey_internal catkey;
  };
};



struct grub_fshelp_node
{
  struct grub_hfsplus_data *data;
  struct grub_hfsplus_extent extents[8];
  grub_uint64_t size;
  grub_uint32_t fileid;
  grub_int32_t mtime;
};

struct grub_hfsplus_btree
{
  grub_uint32_t root;
  grub_size_t nodesize;

  /* Catalog file node.  */
  struct grub_fshelp_node file;
};

/* Information about a "mounted" HFS+ filesystem.  */
struct grub_hfsplus_data
{
  struct grub_hfsplus_volheader volheader;
  grub_disk_t disk;

  unsigned int log2blksize;

  struct grub_hfsplus_btree catalog_tree;
  struct grub_hfsplus_btree extoverflow_tree;

  struct grub_fshelp_node dirroot;
  struct grub_fshelp_node opened_file;

  /* This is the offset into the physical disk for an embedded HFS+
     filesystem (one inside a plain HFS wrapper).  */
  grub_disk_addr_t embedded_offset;
  int case_sensitive;
};

static grub_dl_t my_mod;


/* Return the offset of the record with the index INDEX, in the node
   NODE which is part of the B+ tree BTREE.  */
static inline grub_off_t
grub_hfsplus_btree_recoffset (struct grub_hfsplus_btree *btree,
			   struct grub_hfsplus_btnode *node, int index)
{
  char *cnode = (char *) node;
  void *recptr;
  recptr = (&cnode[btree->nodesize - index * sizeof (grub_uint16_t) - 2]);
  return grub_be_to_cpu16 (grub_get_unaligned16 (recptr));
}

/* Return a pointer to the record with the index INDEX, in the node
   NODE which is part of the B+ tree BTREE.  */
static inline struct grub_hfsplus_key *
grub_hfsplus_btree_recptr (struct grub_hfsplus_btree *btree,
			   struct grub_hfsplus_btnode *node, int index)
{
  char *cnode = (char *) node;
  grub_off_t offset;
  offset = grub_hfsplus_btree_recoffset (btree, node, index);
  return (struct grub_hfsplus_key *) &cnode[offset];
}


/* Find the extent that points to FILEBLOCK.  If it is not in one of
   the 8 extents described by EXTENT, return -1.  In that case set
   FILEBLOCK to the next block.  */
static grub_disk_addr_t
grub_hfsplus_find_block (struct grub_hfsplus_extent *extent,
			 grub_disk_addr_t *fileblock)
{
  int i;
  grub_disk_addr_t blksleft = *fileblock;

  /* First lookup the file in the given extents.  */
  for (i = 0; i < 8; i++)
    {
      if (blksleft < grub_be_to_cpu32 (extent[i].count))
	return grub_be_to_cpu32 (extent[i].start) + blksleft;
      blksleft -= grub_be_to_cpu32 (extent[i].count);
    }

  *fileblock = blksleft;
  return 0xffffffffffffffffULL;
}

static grub_err_t
grub_hfsplus_btree_search (struct grub_hfsplus_btree *btree,
			   struct grub_hfsplus_key_internal *key,
			   int (*compare_keys) (struct grub_hfsplus_key *keya,
						struct grub_hfsplus_key_internal *keyb),
			   struct grub_hfsplus_btnode **matchnode, 
			   grub_off_t *keyoffset);

static int grub_hfsplus_cmp_extkey (struct grub_hfsplus_key *keya,
				    struct grub_hfsplus_key_internal *keyb);

/* Search for the block FILEBLOCK inside the file NODE.  Return the
   blocknumber of this block on disk.  */
static grub_disk_addr_t
grub_hfsplus_read_block (grub_fshelp_node_t node, grub_disk_addr_t fileblock)
{
  struct grub_hfsplus_btnode *nnode = 0;
  grub_disk_addr_t blksleft = fileblock;
  struct grub_hfsplus_extent *extents = &node->extents[0];

  while (1)
    {
      struct grub_hfsplus_extkey *key;
      struct grub_hfsplus_key_internal extoverflow;
      grub_disk_addr_t blk;
      grub_off_t ptr;

      /* Try to find this block in the current set of extents.  */
      blk = grub_hfsplus_find_block (extents, &blksleft);

      /* The previous iteration of this loop allocated memory.  The
	 code above used this memory, it can be freed now.  */
      grub_free (nnode);
      nnode = 0;

      if (blk != 0xffffffffffffffffULL)
	return blk;

      /* For the extent overflow file, extra extents can't be found in
	 the extent overflow file.  If this happens, you found a
	 bug...  */
      if (node->fileid == GRUB_HFSPLUS_FILEID_OVERFLOW)
	{
	  grub_error (GRUB_ERR_READ_ERROR,
		      "extra extents found in an extend overflow file");
	  break;
	}

      /* Set up the key to look for in the extent overflow file.  */
      extoverflow.extkey.fileid = node->fileid;
      extoverflow.extkey.type = 0;
      extoverflow.extkey.start = fileblock - blksleft;

      if (grub_hfsplus_btree_search (&node->data->extoverflow_tree,
				     &extoverflow,
				     grub_hfsplus_cmp_extkey, &nnode, &ptr))
	{
	  grub_error (GRUB_ERR_READ_ERROR,
		      "no block found for the file id 0x%x and the block offset 0x%x",
		      node->fileid, fileblock);
	  break;
	}

      /* The extent overflow file has 8 extents right after the key.  */
      key = (struct grub_hfsplus_extkey *)
	grub_hfsplus_btree_recptr (&node->data->extoverflow_tree, nnode, ptr);
      extents = (struct grub_hfsplus_extent *) (key + 1);

      /* The block wasn't found.  Perhaps the next iteration will find
	 it.  The last block we found is stored in BLKSLEFT now.  */
    }

  grub_free (nnode);

  /* Too bad, you lose.  */
  return -1;
}


/* Read LEN bytes from the file described by DATA starting with byte
   POS.  Return the amount of read bytes in READ.  */
static grub_ssize_t
grub_hfsplus_read_file (grub_fshelp_node_t node,
			void NESTED_FUNC_ATTR (*read_hook) (grub_disk_addr_t sector,
					   unsigned offset, unsigned length),
			grub_off_t pos, grub_size_t len, char *buf)
{
  return grub_fshelp_read_file (node->data->disk, node, read_hook,
				pos, len, buf, grub_hfsplus_read_block,
				node->size,
				node->data->log2blksize - GRUB_DISK_SECTOR_BITS,
				node->data->embedded_offset);
}

static struct grub_hfsplus_data *
grub_hfsplus_mount (grub_disk_t disk)
{
  struct grub_hfsplus_data *data;
  struct grub_hfsplus_btheader header;
  struct grub_hfsplus_btnode node;
  grub_uint16_t magic;
  union {
    struct grub_hfs_sblock hfs;
    struct grub_hfsplus_volheader hfsplus;
  } volheader;

  data = grub_malloc (sizeof (*data));
  if (!data)
    return 0;

  data->disk = disk;

  /* Read the bootblock.  */
  grub_disk_read (disk, GRUB_HFSPLUS_SBLOCK, 0, sizeof (volheader),
		  &volheader);
  if (grub_errno)
    goto fail;

  data->embedded_offset = 0;
  if (grub_be_to_cpu16 (volheader.hfs.magic) == GRUB_HFS_MAGIC)
    {
      grub_disk_addr_t extent_start;
      grub_disk_addr_t ablk_size;
      grub_disk_addr_t ablk_start;

      /* See if there's an embedded HFS+ filesystem.  */
      if (grub_be_to_cpu16 (volheader.hfs.embed_sig) != GRUB_HFSPLUS_MAGIC)
	{
	  grub_error (GRUB_ERR_BAD_FS, "not a HFS+ filesystem");
	  goto fail;
	}

      /* Calculate the offset needed to translate HFS+ sector numbers.  */
      extent_start = grub_be_to_cpu16 (volheader.hfs.embed_extent.first_block);
      ablk_size = grub_be_to_cpu32 (volheader.hfs.blksz);
      ablk_start = grub_be_to_cpu16 (volheader.hfs.first_block);
      data->embedded_offset = (ablk_start
			       + extent_start
			       * (ablk_size >> GRUB_DISK_SECTOR_BITS));

      grub_disk_read (disk, data->embedded_offset + GRUB_HFSPLUS_SBLOCK, 0,
		      sizeof (volheader), &volheader);
      if (grub_errno)
	goto fail;
    }

  /* Make sure this is an HFS+ filesystem.  XXX: Do we really support
     HFX?  */
  magic = grub_be_to_cpu16 (volheader.hfsplus.magic);
  if (((magic != GRUB_HFSPLUS_MAGIC) && (magic != GRUB_HFSPLUSX_MAGIC))
      || volheader.hfsplus.blksize == 0
      || ((volheader.hfsplus.blksize & (volheader.hfsplus.blksize - 1)) != 0)
      || grub_be_to_cpu32 (volheader.hfsplus.blksize) < GRUB_DISK_SECTOR_SIZE)
    {
      grub_error (GRUB_ERR_BAD_FS, "not a HFS+ filesystem");
      goto fail;
    }

  grub_memcpy (&data->volheader, &volheader.hfsplus,
	       sizeof (volheader.hfsplus));

  for (data->log2blksize = 0;
       (1U << data->log2blksize) < grub_be_to_cpu32 (data->volheader.blksize);
       data->log2blksize++);

  /* Make a new node for the catalog tree.  */
  data->catalog_tree.file.data = data;
  data->catalog_tree.file.fileid = GRUB_HFSPLUS_FILEID_CATALOG;
  grub_memcpy (&data->catalog_tree.file.extents,
	       data->volheader.catalog_file.extents,
	       sizeof data->volheader.catalog_file.extents);
  data->catalog_tree.file.size =
    grub_be_to_cpu64 (data->volheader.catalog_file.size);

  /* Make a new node for the extent overflow file.  */
  data->extoverflow_tree.file.data = data;
  data->extoverflow_tree.file.fileid = GRUB_HFSPLUS_FILEID_OVERFLOW;
  grub_memcpy (&data->extoverflow_tree.file.extents,
	       data->volheader.extents_file.extents,
	       sizeof data->volheader.catalog_file.extents);

  data->extoverflow_tree.file.size =
    grub_be_to_cpu64 (data->volheader.extents_file.size);

  /* Read the essential information about the trees.  */
  if (grub_hfsplus_read_file (&data->catalog_tree.file, 0,
			      sizeof (struct grub_hfsplus_btnode),
			      sizeof (header), (char *) &header) <= 0)
    goto fail;

  data->catalog_tree.root = grub_be_to_cpu32 (header.root);
  data->catalog_tree.nodesize = grub_be_to_cpu16 (header.nodesize);
  data->case_sensitive = ((magic == GRUB_HFSPLUSX_MAGIC) &&
			  (header.key_compare == GRUB_HFSPLUSX_BINARYCOMPARE));

  if (grub_hfsplus_read_file (&data->extoverflow_tree.file, 0,
			      sizeof (struct grub_hfsplus_btnode),
			      sizeof (header), (char *) &header) <= 0)
    goto fail;

  data->extoverflow_tree.root = grub_be_to_cpu32 (header.root);

  if (grub_hfsplus_read_file (&data->extoverflow_tree.file, 0, 0,
			      sizeof (node), (char *) &node) <= 0)
    goto fail;

  data->extoverflow_tree.root = grub_be_to_cpu32 (header.root);
  data->extoverflow_tree.nodesize = grub_be_to_cpu16 (header.nodesize);

  data->dirroot.data = data;
  data->dirroot.fileid = GRUB_HFSPLUS_FILEID_ROOTDIR;

  return data;

 fail:

  if (grub_errno == GRUB_ERR_OUT_OF_RANGE)
    grub_error (GRUB_ERR_BAD_FS, "not a HFS+ filesystem");

  grub_free (data);
  return 0;
}

/* Compare the on disk catalog key KEYA with the catalog key we are
   looking for (KEYB).  */
static int
grub_hfsplus_cmp_catkey (struct grub_hfsplus_key *keya,
			 struct grub_hfsplus_key_internal *keyb)
{
  struct grub_hfsplus_catkey *catkey_a = &keya->catkey;
  struct grub_hfsplus_catkey_internal *catkey_b = &keyb->catkey;
  int diff;
  grub_size_t len;

  /* Safe unsigned comparison */
  grub_uint32_t aparent = grub_be_to_cpu32 (catkey_a->parent);
  if (aparent > catkey_b->parent)
    return 1;
  if (aparent < catkey_b->parent)
    return -1;

  len = grub_be_to_cpu16 (catkey_a->namelen);
  if (len > catkey_b->namelen)
    len = catkey_b->namelen;
  /* Since it's big-endian memcmp gives the same result as manually comparing
     uint16_t but may be faster.  */
  diff = grub_memcmp (catkey_a->name, catkey_b->name,
		      len * sizeof (catkey_a->name[0]));
  if (diff == 0)
    diff = grub_be_to_cpu16 (catkey_a->namelen) - catkey_b->namelen;

  return diff;
}

/* Compare the on disk catalog key KEYA with the catalog key we are
   looking for (KEYB).  */
static int
grub_hfsplus_cmp_catkey_id (struct grub_hfsplus_key *keya,
			 struct grub_hfsplus_key_internal *keyb)
{
  struct grub_hfsplus_catkey *catkey_a = &keya->catkey;
  struct grub_hfsplus_catkey_internal *catkey_b = &keyb->catkey;

  /* Safe unsigned comparison */
  grub_uint32_t aparent = grub_be_to_cpu32 (catkey_a->parent);
  if (aparent > catkey_b->parent)
    return 1;
  if (aparent < catkey_b->parent)
    return -1;

  return 0;
}

/* Compare the on disk extent overflow key KEYA with the extent
   overflow key we are looking for (KEYB).  */
static int
grub_hfsplus_cmp_extkey (struct grub_hfsplus_key *keya,
			 struct grub_hfsplus_key_internal *keyb)
{
  struct grub_hfsplus_extkey *extkey_a = &keya->extkey;
  struct grub_hfsplus_extkey_internal *extkey_b = &keyb->extkey;
  grub_uint32_t akey;

  /* Safe unsigned comparison */
  akey = grub_be_to_cpu32 (extkey_a->fileid);
  if (akey > extkey_b->fileid)
    return 1;
  if (akey < extkey_b->fileid)
    return -1;

  if (extkey_a->type > extkey_b->type)
    return 1;
  if (extkey_a->type < extkey_b->type)
    return -1;
  
  akey = grub_be_to_cpu32 (extkey_a->start);
  if (akey > extkey_b->start)
    return 1;
  if (akey < extkey_b->start)
    return -1;
  return 0;
}

static char *
grub_hfsplus_read_symlink (grub_fshelp_node_t node)
{
  char *symlink;
  grub_ssize_t numread;

  symlink = grub_malloc (node->size + 1);
  if (!symlink)
    return 0;

  numread = grub_hfsplus_read_file (node, 0, 0, node->size, symlink);
  if (numread != (grub_ssize_t) node->size)
    {
      grub_free (symlink);
      return 0;
    }
  symlink[node->size] = '\0';

  return symlink;
}

static int
grub_hfsplus_btree_iterate_node (struct grub_hfsplus_btree *btree,
				 struct grub_hfsplus_btnode *first_node,
				 grub_disk_addr_t first_rec,
				 int (*hook) (void *record))
{
  grub_disk_addr_t rec;
  grub_uint64_t saved_node = -1;
  grub_uint64_t node_count = 0;

  for (;;)
    {
      char *cnode = (char *) first_node;

      /* Iterate over all records in this node.  */
      for (rec = first_rec; rec < grub_be_to_cpu16 (first_node->count); rec++)
	{
	  if (hook (grub_hfsplus_btree_recptr (btree, first_node, rec)))
	    return 1;
	}

      if (! first_node->next)
	break;

      if (node_count && first_node->next == saved_node)
	{
	  grub_error (GRUB_ERR_BAD_FS, "HFS+ btree loop");
	  return 0;
	}
      if (!(node_count & (node_count - 1)))
	saved_node = first_node->next;
      node_count++;

      if (grub_hfsplus_read_file (&btree->file, 0,
				  (((grub_disk_addr_t)
				    grub_be_to_cpu32 (first_node->next))
				   * btree->nodesize),
				  btree->nodesize, cnode) <= 0)
	return 1;

      /* Don't skip any record in the next iteration.  */
      first_rec = 0;
    }

  return 0;
}

/* Lookup the node described by KEY in the B+ Tree BTREE.  Compare
   keys using the function COMPARE_KEYS.  When a match is found,
   return the node in MATCHNODE and a pointer to the data in this node
   in KEYOFFSET.  MATCHNODE should be freed by the caller.  */
static grub_err_t
grub_hfsplus_btree_search (struct grub_hfsplus_btree *btree,
			   struct grub_hfsplus_key_internal *key,
			   int (*compare_keys) (struct grub_hfsplus_key *keya,
						struct grub_hfsplus_key_internal *keyb),
			   struct grub_hfsplus_btnode **matchnode, 
			   grub_off_t *keyoffset)
{
  grub_uint64_t currnode;
  char *node;
  struct grub_hfsplus_btnode *nodedesc;
  grub_disk_addr_t rec;
  grub_uint64_t save_node;
  grub_uint64_t node_count = 0;

  node = grub_malloc (btree->nodesize);
  if (! node)
    return grub_errno;

  currnode = btree->root;
  save_node = currnode - 1;
  while (1)
    {
      int match = 0;

      if (save_node == currnode)
	{
	  grub_free (node);
	  return grub_error (GRUB_ERR_BAD_FS, "HFS+ btree loop");
	}
      if (!(node_count & (node_count - 1)))
	save_node = currnode;
      node_count++;

      /* Read a node.  */
      if (grub_hfsplus_read_file (&btree->file, 0,
				  (grub_disk_addr_t) currnode
				  * (grub_disk_addr_t) btree->nodesize,
				  btree->nodesize, (char *) node) <= 0)
	{
	  grub_free (node);
	  return grub_error (GRUB_ERR_BAD_FS, "couldn't read i-node");
	}

      nodedesc = (struct grub_hfsplus_btnode *) node;

      /* Find the record in this tree.  */
      for (rec = 0; rec < grub_be_to_cpu16 (nodedesc->count); rec++)
	{
	  struct grub_hfsplus_key *currkey;
	  currkey = grub_hfsplus_btree_recptr (btree, nodedesc, rec);

	  /* The action that has to be taken depend on the type of
	     record.  */
	  if (nodedesc->type == GRUB_HFSPLUS_BTNODE_TYPE_LEAF
	      && compare_keys (currkey, key) == 0)
	    {
	      /* An exact match was found!  */

	      *matchnode = nodedesc;
	      *keyoffset = rec;

	      return 0;
	    }
	  else if (nodedesc->type == GRUB_HFSPLUS_BTNODE_TYPE_INDEX)
	    {
	      void *pointer;

	      /* The place where the key could have been found didn't
		 contain the key.  This means that the previous match
		 is the one that should be followed.  */
	      if (compare_keys (currkey, key) > 0)
		break;

	      /* Mark the last key which is lower or equal to the key
		 that we are looking for.  The last match that is
		 found will be used to locate the child which can
		 contain the record.  */
	      pointer = ((char *) currkey
			 + grub_be_to_cpu16 (currkey->keylen)
			 + 2);
	      currnode = grub_be_to_cpu32 (grub_get_unaligned32 (pointer));
	      match = 1;
	    }
	}

      /* No match is found, no record with this key exists in the
	 tree.  */
      if (! match)
	{
	  *matchnode = 0;
	  grub_free (node);
	  return 1;
	}
    }
}

static int
grub_hfsplus_iterate_dir (grub_fshelp_node_t dir,
			  int NESTED_FUNC_ATTR
			  (*hook) (const char *filename,
				   enum grub_fshelp_filetype filetype,
				   grub_fshelp_node_t node))
{
  int ret = 0;

  auto int list_nodes (void *record);
  int list_nodes (void *record)
    {
      struct grub_hfsplus_catkey *catkey;
      char *filename;
      int i;
      struct grub_fshelp_node *node;
      struct grub_hfsplus_catfile *fileinfo;
      enum grub_fshelp_filetype type = GRUB_FSHELP_UNKNOWN;

      catkey = (struct grub_hfsplus_catkey *) record;

      fileinfo =
	(struct grub_hfsplus_catfile *) ((char *) record
					 + grub_be_to_cpu16 (catkey->keylen)
					 + 2 + (grub_be_to_cpu16(catkey->keylen)
						% 2));

      /* Stop iterating when the last directory entry is found.  */
      if (grub_be_to_cpu32 (catkey->parent) != dir->fileid)
	return 1;

      /* Determine the type of the node that is found.  */
      switch (fileinfo->type)
	{
	case grub_cpu_to_be16_compile_time (GRUB_HFSPLUS_FILETYPE_REG):
	  {
	    int mode = (grub_be_to_cpu16 (fileinfo->mode)
			& GRUB_HFSPLUS_FILEMODE_MASK);

	    if (mode == GRUB_HFSPLUS_FILEMODE_REG)
	      type = GRUB_FSHELP_REG;
	    else if (mode == GRUB_HFSPLUS_FILEMODE_SYMLINK)
	      type = GRUB_FSHELP_SYMLINK;
	    else
	      type = GRUB_FSHELP_UNKNOWN;
	    break;
	  }
	case grub_cpu_to_be16_compile_time (GRUB_HFSPLUS_FILETYPE_DIR):
	  type = GRUB_FSHELP_DIR;
	  break;
	case grub_cpu_to_be16_compile_time (GRUB_HFSPLUS_FILETYPE_DIR_THREAD):
	  if (dir->fileid == 2)
	    return 0;
	  node = grub_malloc (sizeof (*node));
	  if (!node)
	    return 1;
	  node->data = dir->data;
	  node->mtime = 0;
	  node->size = 0;
	  node->fileid = grub_be_to_cpu32 (fileinfo->parentid);

	  ret = hook ("..", GRUB_FSHELP_DIR, node);
	  return ret;
	}

      if (type == GRUB_FSHELP_UNKNOWN)
	return 0;

      filename = grub_malloc (grub_be_to_cpu16 (catkey->namelen)
			      * GRUB_MAX_UTF8_PER_UTF16 + 1);
      if (! filename)
	return 0;

      /* Make sure the byte order of the UTF16 string is correct.  */
      for (i = 0; i < grub_be_to_cpu16 (catkey->namelen); i++)
	{
	  catkey->name[i] = grub_be_to_cpu16 (catkey->name[i]);

	  if (catkey->name[i] == '/')
	    catkey->name[i] = ':';

	  /* If the name is obviously invalid, skip this node.  */
	  if (catkey->name[i] == 0)
	    return 0;
	}

      *grub_utf16_to_utf8 ((grub_uint8_t *) filename, catkey->name,
			   grub_be_to_cpu16 (catkey->namelen)) = '\0';

      /* Restore the byte order to what it was previously.  */
      for (i = 0; i < grub_be_to_cpu16 (catkey->namelen); i++)
	{
	  if (catkey->name[i] == ':')
	    catkey->name[i] = '/';
	  catkey->name[i] = grub_be_to_cpu16 (catkey->name[i]);
	}

      /* hfs+ is case insensitive.  */
      if (! dir->data->case_sensitive)
	type |= GRUB_FSHELP_CASE_INSENSITIVE;

      /* A valid node is found; setup the node and call the
	 callback function.  */
      node = grub_malloc (sizeof (*node));
      if (!node)
	return 1;
      node->data = dir->data;

      grub_memcpy (node->extents, fileinfo->data.extents,
		   sizeof (node->extents));
      node->mtime = grub_be_to_cpu32 (fileinfo->mtime) - 2082844800;
      node->size = grub_be_to_cpu64 (fileinfo->data.size);
      node->fileid = grub_be_to_cpu32 (fileinfo->fileid);

      ret = hook (filename, type, node);

      grub_free (filename);

      return ret;
    }

  struct grub_hfsplus_key_internal intern;
  struct grub_hfsplus_btnode *node;
  grub_disk_addr_t ptr;

  {
    struct grub_fshelp_node *fsnode;
    fsnode = grub_malloc (sizeof (*fsnode));
    if (!fsnode)
      return 1;
    *fsnode = *dir;
    if (hook (".", GRUB_FSHELP_DIR, fsnode))
      return 1;
  }

  /* Create a key that points to the first entry in the directory.  */
  intern.catkey.parent = dir->fileid;
  intern.catkey.name = 0;
  intern.catkey.namelen = 0;

  /* First lookup the first entry.  */
  if (grub_hfsplus_btree_search (&dir->data->catalog_tree, &intern,
				 grub_hfsplus_cmp_catkey, &node, &ptr))
    return 0;

  /* Iterate over all entries in this directory.  */
  grub_hfsplus_btree_iterate_node (&dir->data->catalog_tree, node, ptr,
				   list_nodes);

  grub_free (node);

  return ret;
}

/* Open a file named NAME and initialize FILE.  */
static grub_err_t
grub_hfsplus_open (struct grub_file *file, const char *name)
{
  struct grub_hfsplus_data *data;
  struct grub_fshelp_node *fdiro = 0;

  grub_dl_ref (my_mod);

  data = grub_hfsplus_mount (file->device->disk);
  if (!data)
    goto fail;

  grub_fshelp_find_file (name, &data->dirroot, &fdiro,
			 grub_hfsplus_iterate_dir,
			 grub_hfsplus_read_symlink, GRUB_FSHELP_REG);
  if (grub_errno)
    goto fail;

  file->size = fdiro->size;
  data->opened_file = *fdiro;
  grub_free (fdiro);

  file->data = data;
  file->offset = 0;

  return 0;

 fail:
  if (data && fdiro != &data->dirroot)
    grub_free (fdiro);
  grub_free (data);

  grub_dl_unref (my_mod);

  return grub_errno;
}


static grub_err_t
grub_hfsplus_close (grub_file_t file)
{
  grub_free (file->data);

  grub_dl_unref (my_mod);

  return GRUB_ERR_NONE;
}

/* Read LEN bytes data from FILE into BUF.  */
static grub_ssize_t
grub_hfsplus_read (grub_file_t file, char *buf, grub_size_t len)
{
  struct grub_hfsplus_data *data =
    (struct grub_hfsplus_data *) file->data;

  return grub_hfsplus_read_file (&data->opened_file, file->read_hook,
				     file->offset, len, buf);
}

static grub_err_t
grub_hfsplus_dir (grub_device_t device, const char *path,
		  int (*hook) (const char *filename,
			       const struct grub_dirhook_info *info))
{
  struct grub_hfsplus_data *data = 0;
  struct grub_fshelp_node *fdiro = 0;

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
      info.mtimeset = 1;
      info.mtime = node->mtime;
      info.case_insensitive = !! (filetype & GRUB_FSHELP_CASE_INSENSITIVE);
      grub_free (node);
      return hook (filename, &info);
    }

  grub_dl_ref (my_mod);

  data = grub_hfsplus_mount (device->disk);
  if (!data)
    goto fail;

  /* Find the directory that should be opened.  */
  grub_fshelp_find_file (path, &data->dirroot, &fdiro,
			 grub_hfsplus_iterate_dir,
			 grub_hfsplus_read_symlink, GRUB_FSHELP_DIR);
  if (grub_errno)
    goto fail;

  /* Iterate over all entries in this directory.  */
  grub_hfsplus_iterate_dir (fdiro, iterate);

 fail:
  if (data && fdiro != &data->dirroot)
    grub_free (fdiro);
  grub_free (data);

  grub_dl_unref (my_mod);

  return grub_errno;
}


static grub_err_t
grub_hfsplus_label (grub_device_t device, char **label)
{
  struct grub_hfsplus_data *data;
  grub_disk_t disk = device->disk;
  struct grub_hfsplus_catkey *catkey;
  int i, label_len;
  struct grub_hfsplus_key_internal intern;
  struct grub_hfsplus_btnode *node;
  grub_disk_addr_t ptr;

  *label = 0;

  data = grub_hfsplus_mount (disk);
  if (!data)
    return grub_errno;

  /* Create a key that points to the label.  */
  intern.catkey.parent = 1;
  intern.catkey.name = 0;
  intern.catkey.namelen = 0;

  /* First lookup the first entry.  */
  if (grub_hfsplus_btree_search (&data->catalog_tree, &intern,
				 grub_hfsplus_cmp_catkey_id, &node, &ptr))
    {
      grub_free (data);
      return 0;
    }

  catkey = (struct grub_hfsplus_catkey *)
    grub_hfsplus_btree_recptr (&data->catalog_tree, node, ptr);

  label_len = grub_be_to_cpu16 (catkey->namelen);
  for (i = 0; i < label_len; i++)
    {
      catkey->name[i] = grub_be_to_cpu16 (catkey->name[i]);

      /* If the name is obviously invalid, skip this node.  */
      if (catkey->name[i] == 0)
	return 0;
    }

  *label = grub_malloc (label_len * GRUB_MAX_UTF8_PER_UTF16 + 1);
  if (! *label)
    return grub_errno;

  *grub_utf16_to_utf8 ((grub_uint8_t *) (*label), catkey->name,
		       label_len) = '\0';

  grub_free (node);
  grub_free (data);

  return GRUB_ERR_NONE;
}

/* Get mtime.  */
static grub_err_t
grub_hfsplus_mtime (grub_device_t device, grub_int32_t *tm)
{
  struct grub_hfsplus_data *data;
  grub_disk_t disk = device->disk;

  grub_dl_ref (my_mod);

  data = grub_hfsplus_mount (disk);
  if (!data)
    *tm = 0;
  else
    *tm = grub_be_to_cpu32 (data->volheader.utime) - 2082844800;

  grub_dl_unref (my_mod);

  grub_free (data);

  return grub_errno;

}

static grub_err_t
grub_hfsplus_uuid (grub_device_t device, char **uuid)
{
  struct grub_hfsplus_data *data;
  grub_disk_t disk = device->disk;

  grub_dl_ref (my_mod);

  data = grub_hfsplus_mount (disk);
  if (data)
    {
      *uuid = grub_xasprintf ("%016llx",
			     (unsigned long long)
			     grub_be_to_cpu64 (data->volheader.num_serial));
    }
  else
    *uuid = NULL;

  grub_dl_unref (my_mod);

  grub_free (data);

  return grub_errno;
}



static struct grub_fs grub_hfsplus_fs =
  {
    .name = "hfsplus",
    .dir = grub_hfsplus_dir,
    .open = grub_hfsplus_open,
    .read = grub_hfsplus_read,
    .close = grub_hfsplus_close,
    .label = grub_hfsplus_label,
    .mtime = grub_hfsplus_mtime,
    .uuid = grub_hfsplus_uuid,
#ifdef GRUB_UTIL
    .reserved_first_sector = 1,
    .blocklist_install = 1,
#endif
    .next = 0
  };

GRUB_MOD_INIT(hfsplus)
{
  grub_fs_register (&grub_hfsplus_fs);
  my_mod = mod;
}

GRUB_MOD_FINI(hfsplus)
{
  grub_fs_unregister (&grub_hfsplus_fs);
}
