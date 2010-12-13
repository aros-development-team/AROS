#ifndef _BLOCKSTRUCTURE_H
#define _BLOCKSTRUCTURE_H

#include <exec/types.h>

/* a BLCK is a block number.  Blocksize in bytes = SectorSize * SectorsPerBlock.

   BLCK pointers are used throughout the filesystem.  All structures which
   require to associate themselves with another structure do so using BLCK
   pointers.  Byte pointers are not used. */

typedef ULONG BLCK;
typedef BLCK BLCKn;


/* Below is the standard block header.  This header is found before EVERY
   type of block used in the filesystem, except data blocks.

   The id field is used to check if the block is of the correct type when
   it is being referred to using a BLCK pointer.

   The checksum field is the SUM of all LONGs in a block plus one, and then
   negated.  When applying a checksum the checksum field itself should be
   set to zero.  To check to see if the checksum is okay, the sum of all
   longs in a block should be zero.

   The ownblock BLCK pointer points to the block itself.  This field is an
   extra safety check to ensure we are using a valid block. */

struct fsBlockHeader {
  ULONG id;         /* 4 character id string of this block */
  ULONG be_checksum;   /* The checksum */
  BLCK  be_ownblock;   /* The blocknumber of the block this block is stored at */
};



/* Now follows the structure of the Boot block.  The Boot block is always
   located at block offset 0.  It contains only a version number at the
   moment to identify the block structure of the disk. */

/*
struct fsBootBlock {
  struct fsBlockHeader bheader;

  UWORD version;                   // Version number of the filesystem block structure
};
*/

#define STRUCTURE_VERSION (3)



/* The fsRootInfo structure has all kinds of information about the format
   of the disk. */

struct fsRootInfo {
  ULONG be_deletedblocks;             /* Amount in blocks which deleted files consume. */
  ULONG be_deletedfiles;              /* Number of deleted files in recycled. */
  ULONG be_freeblocks;                /* Cached number of free blocks on disk. */

  ULONG be_datecreated;

  BLCK  be_lastallocatedblock;        /* Block which was most recently allocated */
  BLCK  be_lastallocatedadminspace;   /* AdminSpaceContainer which most recently was used to allocate a block */
  ULONG be_lastallocatedextentnode;   /* ExtentNode which was most recently created */
  ULONG be_lastallocatedobjectnode;   /* ObjectNode which was most recently created */

  BLCK  be_rovingpointer;
};



/* An SFS disk has two Root blocks, one located at the start of
   the partition and one at the end.  On startup the fs will check
   both Roots to see if it is a valid SFS disk.  If either one is
   missing SFS can still continue.

   A Root block could be missing on purpose.  For example, if you
   extend the partition at the end (adding a few MB's) then SFS
   can detect this with the information stored in the Root block
   located at the beginning (since only the end-offset has changed).
   Same goes for the other way around, as long as you don't change
   start and end point at the same time.

   When a Root block is missing because the partition has been
   made a bit larger, then SFS will in the future be able to
   'resize' itself without re-formatting the disk.

   Note: ownblock pointers won't be correct anymore when start of
         partition has changed... */

struct fsRootBlock {
  struct fsBlockHeader bheader;

  UWORD be_version;              /* Version number of the filesystem block structure */
  UWORD be_sequencenumber;       /* The Root with the highest sequencenumber is valid */

  ULONG be_datecreated;          /* Creation date (when first formatted).  Cannot be changed. */
  UBYTE bits;                 /* various settings, see defines below. */
  UBYTE pad1;
  UWORD be_pad2;

  ULONG be_reserved1[2];

  ULONG be_firstbyteh;           /* The first byte of our partition from the start of the */
  ULONG be_firstbyte;            /* disk.  firstbyteh = upper 32 bits, firstbyte = lower 32 bits. */

  ULONG be_lastbyteh;            /* The last byte of our partition, excluding this one. */
  ULONG be_lastbyte;

  BLCK  be_totalblocks;          /* size of this partition in blocks */
  ULONG be_blocksize;            /* blocksize used */

  ULONG be_reserved2[2];
  ULONG be_reserved3[8];

  BLCK  be_bitmapbase;           /* location of the bitmap */
  BLCK  be_adminspacecontainer;  /* location of first adminspace container */
  BLCK  be_rootobjectcontainer;  /* location of the root objectcontainer */
  BLCK  be_extentbnoderoot;      /* location of the root of the extentbnode B-tree */
  BLCK  be_objectnoderoot;       /* location of the root of the objectnode tree */

  ULONG be_reserved4[3];
};

#define ROOTBITS_CASESENSITIVE (128)   /* When set, filesystem names are treated case
                                          insensitive. */
#define ROOTBITS_RECYCLED      (64)    /* When set, files being deleted will first be
                                          moved to the Recycled directory. */

struct fsExtentBNode {
  ULONG be_key;     /* data! */
  ULONG be_next;
  ULONG be_prev;
  UWORD be_blocks;  /* The size in blocks of the region this Extent controls */
} __attribute__((packed));

#endif // _BLOCKSTRUCTURE_H
