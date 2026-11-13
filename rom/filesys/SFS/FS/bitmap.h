#ifndef _BITMAP_H
#define _BITMAP_H

#include <exec/types.h>
#include <libraries/iffparse.h>
#include "blockstructure.h"

#define SPACELIST_MAX (1000)

/* Used by smartfindandmarkspace() */

struct Space {
  BLCK block;
  ULONG blocks;
};



/* The fsBitmap structure is used for Bitmap blocks.  Every partition has
   a bitmap, and the number of blocks the partition consists of determines
   its size.  The position of the first Bitmap block is stored in the root
   block.  If there are any more bitmap blocks then these are located in
   order directly after the first bitmap block.  For every block on disk
   there is a single bit in a bitmap block which tells you whether it is
   in use or not. */

#define BITMAP_ID               AROS_LONG2BE(MAKE_ID('B','T','M','P'))

struct fsBitmap {
  struct fsBlockHeader bheader;

  ULONG bitmap[0];

  /* Bits are 1 if the block is free, and 0 if full.
     Bitmap must consist of an integral number of longwords. */
};

#endif // _BITMAP_H
