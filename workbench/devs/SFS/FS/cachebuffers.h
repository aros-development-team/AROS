#ifndef _CACHEBUFFERS_H
#define _CACHEBUFFERS_H

#include <exec/nodes.h>
#include <exec/types.h>

struct CacheBuffer {
  /* Keep the below two structures in this order.  There is no support for linking
     structures in two different lists properly, so to find the start of the CacheBuffer
     structure in the case of the 2nd MinNode we substract a few bytes from it. */

  struct MinNode node;               // LRU link
  struct MinNode hashnode;           // Linked chain of blocks with same hash value

  UBYTE locked;                      // 0 if unlocked.
  UBYTE bits;                        /* Bit 0: Set means this block is original.
                                        Bit 1: Set means this block is the latest version.
                                        Bit 2: Set means that this block has an empty original.
                                        Bit 3: Set means that this block was checksummed correctly. */
  UWORD id;                          // a 2-byte id code used to check if this really IS a cachebuffer...

  ULONG blckno;                      // Partition Blocknumber, or zero if this block isn't in use

  void *data;                        /* Make sure this is LONGWORD aligned! */

  UBYTE attached_data[0];
};

#define CB_ORIGINAL (1)
#define CB_LATEST   (2)
#define CB_EMPTY    (4)
#define CB_CHECKSUM (8)

/*
  CB_ORIGINAL          : This CacheBuffer contains a block exactly like it is currently on disk.

  CB_LATEST            : This CacheBuffer contains the latest version of a block (ie, including
                         modifications made during a transaction).

  CB_LATEST | CB_EMPTY : Same as CB_LATEST, but the block has no original version (it was based
                         on an as of yet unallocated block).

  Other combinations of CB_LATEST, CB_ORIGINAL and CB_EMPTY are invalid.

  An exception to this rule could be none set at all, which would indicate an unused cachebuffer.
*/

#endif // _CACHEBUFFERS_H
