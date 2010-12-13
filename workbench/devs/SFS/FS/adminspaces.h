#ifndef _ADMINSPACES_H
#define _ADMINSPACES_H

#include <exec/types.h>
#include <libraries/iffparse.h>
#include "blockstructure.h"

struct fsAdminSpace {
  BLCK  be_space;
  ULONG be_bits;      /* Set bits are used blocks, bit 31 is
                      the first block in the AdminSpace. */
};


#define ADMINSPACECONTAINER_ID     AROS_LONG2BE(MAKE_ID('A','D','M','C'))

struct fsAdminSpaceContainer {
  struct fsBlockHeader bheader;

  BLCK be_next;
  BLCK be_previous;

  UBYTE bits;    /* Bits 0-2: %000 = 2^0 = bitmap size 1 byte.
                              %001 = 2^1 = bitmap size 2 bytes.
                              %010 = 2^2 = bitmap size 4 bytes.
                              %011 = 2^3 = bitmap size 8 bytes.
                              %100 = 2^4 = bitmap size 16 bytes.
                              %101 = 2^5 = bitmap size 32 bytes.
                              %110 = 2^6 = bitmap size 64 bytes.
                              %111 = 2^7 = bitmap size 128 bytes. */
  UBYTE be_pad1;
  UWORD be_pad2;

  struct fsAdminSpace adminspace[0];
};



/*

  INT((blocksize-sizeof(struct fsAdminSpaceContainer)) / (2^bits+4)) * 2^bits

  Effectiveness     512  1024  2048  4096  8192
  -----------------------------------------------
     4              244   500
     8              320   664
    16              384   800
    32              468   864
    64              476   896  1856  3776  7680
   128              396   896  1920  3840  7808
  -----------------------------------------------
*/

/* 
  30.000 files -> 3000-6000 OBJC -> 6000-12000 blocks.

  1952 blocks/ADMC



*/

#endif // _ADMINSPACES_H
