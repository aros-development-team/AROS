#ifndef _BITMAP_PROTOS_H
#define _BITMAP_PROTOS_H

#include <exec/types.h>
#include "blockstructure.h"

LONG markspace(BLCK,ULONG);
LONG freespace(BLCK,ULONG);
LONG extractspace(UBYTE *dest, BLCK block, ULONG blocks);

LONG availablespace(BLCK block,ULONG maxneeded);
LONG allocatedspace(BLCK block,ULONG maxneeded);

LONG findspace(ULONG blocksneeded,BLCK startblock,BLCK endblock,BLCK *returned_block);
LONG findspace2(ULONG blocksneeded,BLCK startblock,BLCK endblock,BLCK *returned_block, ULONG *returned_blocks);
LONG findspace2_backwards(ULONG maxneeded, BLCK startblock, BLCK endblock, BLCK *returned_block, ULONG *returned_blocks);
LONG findandmarkspace(ULONG,BLCK *);
LONG smartfindandmarkspace(BLCK startblock,ULONG blocksneeded);

LONG getusedblocks(ULONG *returned_usedblocks);
LONG getfreeblocks(ULONG *returned_freeblocks);
LONG setfreeblocks(ULONG freeblocks);

#endif // _BITMAP_PROTOS_H
