#include "asmsupport.h"

#include <clib/macros.h>
#include <dos/dos.h>
#include <exec/types.h>
#include <proto/exec.h>

#include "bitmap.h"
#include "cachebuffers_protos.h"
#include "debug.h"
#include "objects.h"
#include "transactions_protos.h"

#include "globals.h"

extern LONG readcachebuffercheck(struct CacheBuffer **,ULONG,ULONG);
extern void setchecksum(struct CacheBuffer *);

extern LONG req_unusual(UBYTE *fmt, ... );


LONG getfreeblocks(ULONG *returned_freeblocks) {
  struct CacheBuffer *cb;
  LONG errorcode;

  if((errorcode=readcachebuffercheck(&cb,globals->block_root,OBJECTCONTAINER_ID))==0) {
    struct fsRootInfo *ri=(struct fsRootInfo *)((UBYTE *)cb->data+globals->bytes_block-sizeof(struct fsRootInfo));

    *returned_freeblocks=BE2L(ri->be_freeblocks);
  }

  return(errorcode);
}



LONG getusedblocks(ULONG *returned_usedblocks) {
  ULONG freeblocks;
  LONG errorcode;

  if((errorcode=getfreeblocks(&freeblocks))==0) {
    *returned_usedblocks=globals->blocks_total-globals->blocks_reserved_start-globals->blocks_reserved_end-freeblocks;
  }

  return(errorcode);
}



LONG setfreeblocks(ULONG freeblocks) {
  struct CacheBuffer *cb;
  LONG errorcode;

  if((errorcode=readcachebuffercheck(&cb,globals->block_root,OBJECTCONTAINER_ID))==0) {
    struct fsRootInfo *ri=(struct fsRootInfo *)((UBYTE *)cb->data+globals->bytes_block-sizeof(struct fsRootInfo));

    preparecachebuffer(cb);

    ri->be_freeblocks=L2BE(freeblocks);

    errorcode=storecachebuffer(cb);
  }

  return(errorcode);
}



static BOOL enoughspace(ULONG blocks) {
  ULONG blocksfree;

  if(getfreeblocks(&blocksfree)!=0 || blocksfree-transactionspace() < blocks+ALWAYSFREE) {
    return(FALSE);
  }

  return(TRUE);
}


LONG availablespace(BLCK block,ULONG maxneeded);


LONG markspace(BLCK block,ULONG blocks) {
  ULONG freeblocks;
  LONG errorcode;

  _XDEBUG((DEBUG_BITMAP,"markspace: Marking %ld blocks from block %ld\n",blocks,block));

  if((availablespace(block, blocks))<blocks) {
    req_unusual("Attempted to mark %ld blocks from block %ld,\n but some of them were already full.", blocks, block);
    return(-1);
  }

  if((errorcode=getfreeblocks(&freeblocks))==0) {
    if((errorcode=setfreeblocks(freeblocks-blocks))==0) {
      struct CacheBuffer *cb;
      ULONG skipblocks=block/globals->blocks_inbitmap;
      ULONG longs=(globals->bytes_block-sizeof(struct fsBitmap))>>2;
      ULONG bitmapblock;
      // ULONG newrovingptr=block+blocks;

      // blocks_useddiff+=blocks;

      block-=skipblocks*globals->blocks_inbitmap;
      bitmapblock=globals->block_bitmapbase+skipblocks;

      /* /block/ is bitoffset in the /bitmapblock/ */

      while(blocks>0) {
        if((errorcode=readcachebuffercheck(&cb,bitmapblock++,BITMAP_ID))==0) {
          struct fsBitmap *b;

          preparecachebuffer(cb);

          b=cb->data;

          blocks-=bmclr(b->bitmap,longs,block,blocks);
          block=0;

          if((errorcode=storecachebuffer(cb))!=0) {
            return(errorcode);
          }
        }
        else {
          return(errorcode);
        }
      }

      // block_rovingblockptr=newrovingptr;
      // if(block_rovingblockptr>=blocks_total) {
      //   block_rovingblockptr=0;
      // }
    }
  }

  return(errorcode);
}



LONG freespace(BLCK block,ULONG blocks) {
  ULONG freeblocks;
  LONG errorcode;

  _XDEBUG((DEBUG_BITMAP,"freespace: Freeing %ld blocks from block %ld\n",blocks,block));

  if((errorcode=getfreeblocks(&freeblocks))==0) {
    if((errorcode=setfreeblocks(freeblocks+blocks))==0) {
      struct CacheBuffer *cb;
      ULONG skipblocks=block/globals->blocks_inbitmap;
      ULONG longs=(globals->bytes_block-sizeof(struct fsBitmap))>>2;
      ULONG bitmapblock;

      // blocks_useddiff-=blocks;

      block-=skipblocks*globals->blocks_inbitmap;
      bitmapblock=globals->block_bitmapbase+skipblocks;

      /* /block/ is bitoffset in the /bitmapblock/ */

      while(blocks>0) {
        if((errorcode=readcachebuffercheck(&cb,bitmapblock++,BITMAP_ID))==0) {
          struct fsBitmap *b;

          preparecachebuffer(cb);

          b=cb->data;

          blocks-=bmset(b->bitmap,longs,block,blocks);
          block=0;

          if((errorcode=storecachebuffer(cb))!=0) {
            return(errorcode);
          }
        }
        else {
          return(errorcode);
        }
      }
    }
  }

  return(errorcode);
}




LONG extractspace(UBYTE *dest, BLCK block, ULONG blocks) {
  ULONG skipblocks=block/globals->blocks_inbitmap;
  ULONG byteoffset=(block-(skipblocks*globals->blocks_inbitmap))>>3;
  ULONG bytesinbitmap=globals->blocks_inbitmap>>3;
  ULONG bytes=(blocks+7)>>3;
  ULONG bitmapblock=globals->block_bitmapbase+skipblocks;

  if((block & 0x07)!=0 || block>=globals->blocks_total || block+blocks>globals->blocks_total) {
    return(ERROR_BAD_NUMBER);
  }

  /* /block/ is bitoffset in the /bitmapblock/ */

  while(bytes>0) {
    struct CacheBuffer *cb;
    LONG errorcode;

    if((errorcode=readcachebuffercheck(&cb, bitmapblock++, BITMAP_ID))==0) {
      struct fsBitmap *b;
      ULONG bytestocopy=bytesinbitmap-byteoffset;

      if(bytestocopy>bytes) {
        bytestocopy=bytes;
      }

      b=cb->data;

      CopyMem((UBYTE *)b->bitmap + byteoffset, dest, bytestocopy);

      bytes-=bytestocopy;
      dest+=bytestocopy;
      byteoffset=0;
    }
    else {
      return(errorcode);
    }
  }

  return(0);
}



LONG availablespace(BLCK block,ULONG maxneeded) {
  struct CacheBuffer *cb;
  struct fsBitmap *b;
  ULONG longs=globals->blocks_inbitmap>>5;
  ULONG maxbitmapblock=globals->block_bitmapbase+globals->blocks_bitmap;
  LONG blocksfound=0;
  ULONG bitstart;
  LONG bitend;
  LONG errorcode=0;
  BLCK nextblock=globals->block_bitmapbase+block/globals->blocks_inbitmap;

  /* Determines the amount of free blocks starting from block /block/.
     If there are no blocks found or if there was an error -1 is returned,
     otherwise this function will count the number of free blocks until
     an allocated block is encountered or until maxneeded has been
     exceeded. */

  bitstart=block % globals->blocks_inbitmap;

  while(nextblock<maxbitmapblock && (errorcode=readcachebuffercheck(&cb,nextblock++,BITMAP_ID))==0) {
    b=cb->data;

    if((bitend=bmffz(b->bitmap,longs,bitstart))<(32*longs)) {
      blocksfound+=bitend-bitstart;
      return(blocksfound);
    }

    blocksfound+=globals->blocks_inbitmap-bitstart;
    if(blocksfound>=maxneeded) {
      return(blocksfound);
    }

    bitstart=0;
  }

  if(errorcode!=0) {
    return(-1);
  }

  return(blocksfound);
}



LONG allocatedspace(BLCK block,ULONG maxneeded) {
  struct CacheBuffer *cb;
  struct fsBitmap *b;
  ULONG longs=globals->blocks_inbitmap>>5;
  ULONG maxbitmapblock=globals->block_bitmapbase+globals->blocks_bitmap;
  LONG blocksfound=0;
  ULONG bitstart;
  LONG bitend;
  LONG errorcode=0;
  BLCK nextblock=globals->block_bitmapbase+block/globals->blocks_inbitmap;

  /* Determines the amount of free blocks starting from block /block/.
     If there are no blocks found or if there was an error -1 is returned,
     otherwise this function will count the number of free blocks until
     an allocated block is encountered or until maxneeded has been
     exceeded. */

  bitstart=block % globals->blocks_inbitmap;

  while(nextblock<maxbitmapblock && (errorcode=readcachebuffercheck(&cb,nextblock++,BITMAP_ID))==0) {
    b=cb->data;

    if((bitend=bmffo(b->bitmap,longs,bitstart))<(32*longs)) {
      blocksfound+=bitend-bitstart;
      return(blocksfound);
    }

    blocksfound+=globals->blocks_inbitmap-bitstart;
    if(blocksfound>=maxneeded) {
      return(blocksfound);
    }

    bitstart=0;
  }

  if(errorcode!=0) {
    return(-1);
  }

  return(blocksfound);
}



LONG findspace2(ULONG maxneeded, BLCK start, BLCK end, BLCK *returned_block, ULONG *returned_blocks) {
  struct CacheBuffer *cb;
  ULONG longs=globals->blocks_inbitmap>>5;
  ULONG space=0;
  ULONG block;
  BLCK bitmapblock=globals->block_bitmapbase + start/globals->blocks_inbitmap;
  BLCK breakpoint;
  LONG bitstart,bitend;
  LONG reads;
  LONG errorcode;

  /* This function checks the bitmap and tries to locate /maxneeded/ adjacent
     unused blocks, or less if there aren't that many unused adjacent blocks.

     The number of blocks found will be stored in returned_blocks, and the
     location of those blocks stored in returned_block.  The number of blocks
     found can be zero if the area to search in didn't contain any free blocks.
     This is not an error -- the return value will be 0.

     Errors which occur during the execution of this function are returned as
     normal.  The contents of returned_block and returned_blocks is undefined
     in that case.

     The search-area is limited by startblock (inclusive) and endblock (exclusive).
     Note that the search-area loops, so it is possible to do wrap-around searches
     by setting endblock<=startblock (see table below):

     Start-block    End-block    Area searched

          0            10        1111111111
          5            10        .....11111
          5             7        .....11...
          5             5        2222211111
          9            10        .........1
          9             0        .........1
          9             1        2........1
         10             2        11........ */

  if(start>=globals->blocks_total) {
    start-=globals->blocks_total;
  }

  if(end==0) {
    end=globals->blocks_total;
  }

  reads=((end-1)/globals->blocks_inbitmap) + 1 - start/globals->blocks_inbitmap;
  if(start>=end) {
    reads+=(globals->blocks_total-1)/globals->blocks_inbitmap + 1;
  }

  breakpoint=(start<end ? end : globals->blocks_total);

  *returned_block=0;
  *returned_blocks=0;

  bitend=start % globals->blocks_inbitmap;
  block=start-bitend;

  while((errorcode=readcachebuffercheck(&cb, bitmapblock++, BITMAP_ID))==0) {
    struct fsBitmap *b=cb->data;
    ULONG localbreakpoint=breakpoint-block;

    if(localbreakpoint>globals->blocks_inbitmap) {
      localbreakpoint=globals->blocks_inbitmap;
    }

    /* At this point space contains the amount of free blocks at
       the end of the previous bitmap block.  If there are no
       free blocks at the start of this bitmap block, space will
       be set to zero, since in that case the space isn't adjacent. */

    while((bitstart=bmffo(b->bitmap, longs, bitend))<globals->blocks_inbitmap) {

      /* found the start of an empty space, now find out how large it is */

      if(bitstart >= localbreakpoint) {     // Oct  3 1999: Check if the start of this empty space is within bounds.
        break;
      }

      if(bitstart!=0) {
        space=0;
      }

      bitend=bmffz(b->bitmap, longs, bitstart);

      if(bitend > localbreakpoint) {
        bitend=localbreakpoint;
      }

      space+=bitend-bitstart;

      if(*returned_blocks<space) {
        *returned_block=block+bitend-space;
        if(space>=maxneeded) {
          *returned_blocks=maxneeded;
          return(0);
        }
        *returned_blocks=space;
      }

      if(bitend>=localbreakpoint) {
        break;
      }
    }

    if(--reads==0) {
      break;
    }

    /* no (more) empty spaces found in this block */

    if(bitend!=globals->blocks_inbitmap) {
      space=0;
    }

    bitend=0;
    block+=globals->blocks_inbitmap;

    if(block>=globals->blocks_total) {
      block=0;
      space=0;
      breakpoint=end;
      bitmapblock=globals->block_bitmapbase;
    }
  }

  return(errorcode);
}


#if 0
LONG findspace(ULONG blocksneeded,BLCK startblock,BLCK endblock,BLCK *returned_block) {
  ULONG blocks;
  LONG errorcode;

  /* This function checks the bitmap and tries to locate at least /blocksneeded/
     adjacent unused blocks.  If found it set returned_block to the start block.
     If not it is set to zero.  Any error is returned.

     The search-area is limited by startblock (inclusive) and endblock (exclusive). */

  if((errorcode=findspace2(blocksneeded, startblock, endblock, returned_block, &blocks))==0) {
    if(blocks!=blocksneeded) {
      _DEBUG(("findspace: %ld != %ld\n", blocks, blocksneeded));
      return(ERROR_DISK_FULL);
    }
  }

  return(errorcode);
}
#endif


LONG findspace(ULONG blocksneeded, BLCK startblock, BLCK endblock, BLCK *returned_block) {
  ULONG blocks;
  LONG errorcode;

  /* This function checks the bitmap and tries to locate at least /blocksneeded/
     adjacent unused blocks.  If found it sets returned_block to the start block
     and returns no error.  If not found, ERROR_DISK_IS_FULL is returned and
     returned_block is set to zero.  Any other errors are returned as well.

     The search-area is limited by startblock (inclusive) and endblock (exclusive).
     Note that the search-area loops, so it is possible to do wrap-around searches
     by setting endblock<=startblock (see table below):

     Start-block    End-block    Area searched

          0            10        1111111111
          5            10        .....11111
          5             7        .....11...
          5             5        2222211111
          9            10        .........1
          9             0        .........1
          9             1        2........1 */

  if((errorcode=findspace2(blocksneeded, startblock, endblock, returned_block, &blocks))==0) {
    if(blocks!=blocksneeded) {
      _DEBUG(("findspace: %ld != %ld\n", blocks, blocksneeded));
      return(ERROR_DISK_FULL);
    }
  }

  return(errorcode);
}



LONG findandmarkspace(ULONG blocksneeded, BLCK *returned_block) {
  LONG errorcode;

  // This function is currently only used by AdminSpace allocation. */

  if(enoughspace(blocksneeded)!=FALSE) {

//  if((errorcode=findspace(blocksneeded, block_rovingblockptr, block_rovingblockptr, returned_block))==0)

    if((errorcode=findspace(blocksneeded, 0, globals->blocks_total, returned_block))==0) {
      errorcode=markspace(*returned_block,blocksneeded);
    }
  }
  else {
    errorcode=ERROR_DISK_FULL;
  }

  return(errorcode);
}



LONG smartfindandmarkspace(BLCK startblock,ULONG blocksneeded) {
  LONG freeblocks,allocblocks;
  BLCK block=startblock;
  ULONG blocksfound=0;
  UWORD entry=0;
  UWORD n;
  ULONG blocks, smallestfragment, newsmallestfragment;
  LONG errorcode;

  /* This function searches the bitmap to locate /blocks/ blocks.  It will
     return the amount of blocks needed in a number of fragments if necessary.

     Followed strategy:

     If there are less free blocks than there are needed then this function
     will keep looking until the bitmap has been scanned completely.  It will
     return every free block available in this case.

     If the number of blocks found is larger than the amount of needed blocks
     then there are a couple of options:

     a) If the blocks found are a single fragment, then return immediately.
     b) If the blocks have fragments which average above 64K and atleast
        a quarter of the bitmap has been scanned, then return immediately.
     c) If the blocks have fragments which average above 16K and atleast
        half the bitmap has been scanned, then return immediately.
     d) Otherwise scan the bitmap completely (expensive!). */

  if(enoughspace(blocksneeded)!=FALSE) {

    while(entry<SPACELIST_MAX) {
      freeblocks=availablespace(block,blocksneeded);
      _XDEBUG((DEBUG_BITMAP,"sfams: availablespace returned %ld for block %ld while we needed %ld blocks\n",freeblocks,block,blocksneeded));

      if(freeblocks>0) {
        if(freeblocks>=blocksneeded) {
          entry=0;
          globals->spacelist[entry].block=block;
          globals->spacelist[entry++].blocks=freeblocks;
          blocksfound=freeblocks;
          break;
        }

        globals->spacelist[entry].block=block;

        if(block<startblock && block+freeblocks>=startblock) {
          freeblocks=startblock-block;

          blocksfound+=freeblocks;
          globals->spacelist[entry++].blocks=freeblocks;
          break;
        }

        blocksfound+=freeblocks;
        globals->spacelist[entry++].blocks=freeblocks;
      }
      else if(freeblocks<0) {
        return(ERROR_DISK_FULL);
      }

      block+=freeblocks;
      if(block>=globals->blocks_total) {
        if(startblock==0) {
          break;
        }
        block=0;
      }

      allocblocks=allocatedspace(block,globals->blocks_inbitmap*2);
      _XDEBUG((DEBUG_BITMAP,"sfams: allocatedspace returned %ld for block %ld while we needed %ld blocks\n",allocblocks,block,globals->blocks_inbitmap*2));

      if(allocblocks<0) {
        _DEBUG(("sfams: disk full 1\n"));
        return(ERROR_DISK_FULL);
      }
      else if(block<startblock && block+allocblocks>=startblock) {
        break;
      }

      block+=allocblocks;
      if(block>=globals->blocks_total) {
        if(startblock==0) {
          break;
        }
        block=0;
      }
    }

    if(entry==0) {
      _DEBUG(("sfams: disk full 2\n"));
      return(ERROR_DISK_FULL);
    }

    globals->spacelist[entry].block=0;

    /* blocksfound, entry & blocksneeded */

    /* We need to eliminate the smallest fragments from the spacelist. */

    smallestfragment=1;

    while(blocksfound>=blocksneeded+smallestfragment) {
      n=0;
      newsmallestfragment=0xFFFF;

      while(globals->spacelist[n].block!=0) {
        blocks=globals->spacelist[n].blocks;
        if(blocks!=0) {
          if(blocks==smallestfragment) {
            blocksfound-=smallestfragment;
            globals->spacelist[n].blocks=0;
            if(blocksfound-smallestfragment<blocksneeded) {
              break;
            }
          }
          else if(blocks<newsmallestfragment) {
            newsmallestfragment=blocks;
          }
        }
        n++;
      }

      smallestfragment=newsmallestfragment;
    }

    /* Now we need to mark the remaining fragments as used space.  The
       last fragment may need to be partially marked, depending on
       /blocksneeded/. */

    n=0;
    while(globals->spacelist[n].block!=0) {
      blocks=globals->spacelist[n].blocks;
      if(blocks!=0) {
        if(blocksneeded<blocks) {
          blocks=blocksneeded;
        }

        if((errorcode=markspace(globals->spacelist[n].block,blocks))!=0) {
          return(errorcode);
        }

        blocksneeded-=blocks;
      }

      n++;
    }
  }
  else {
    _DEBUG(("sfams: disk full 3\n"));
    return(ERROR_DISK_FULL);
  }

  return(0);
}




/*

 This function returns for (128, 353219, blocks_total) : 351840 (!!)  not allowed!

*/



LONG findspace2_backwards(ULONG maxneeded, BLCK start, BLCK end, BLCK *returned_block, ULONG *returned_blocks) {
  struct CacheBuffer *cb;
  ULONG space=0;
  ULONG block;
  BLCK bitmapblock=globals->block_bitmapbase+start/globals->blocks_inbitmap;
  BLCK breakpoint;
  BLCK prevbitmapblock=globals->block_bitmapbase+(end-1)/globals->blocks_inbitmap;
  LONG bitstart,bitend;
  LONG errorcode;

  /* This function checks the bitmap and tries to locate /maxneeded/ adjacent
     unused blocks, or less if there aren't that many unused adjacent blocks.

     The number of blocks found will be stored in returned_blocks, and the
     location of those blocks stored in returned_block.  The number of blocks
     found can be zero if the area to search in didn't contain any free blocks.
     This is not an error -- the return value will be 0.

     Errors which occur during the execution of this function are returned as
     normal.  The contents of returned_block and returned_blocks is undefined
     in that case.

     The search-area is limited by start (inclusive) and end (exclusive). */

/*
  if(start>=blocks_total) {
    start-=blocks_total;
  }

  if(end==0) {
    end=blocks_total;
  }

  reads=((end-1)/blocks_inbitmap) + 1 - start/blocks_inbitmap;
  if(start>=end) {
    reads+=(blocks_total-1)/blocks_inbitmap + 1;
  }

  breakpoint=(start<end ? start : 0);
*/
  breakpoint=start;

  *returned_block=0;
  *returned_blocks=0;

  bitstart=(end-1) % globals->blocks_inbitmap;
  block=(end-1) - bitstart;

  while((errorcode=readcachebuffercheck(&cb, prevbitmapblock--, BITMAP_ID))==0) {
    struct fsBitmap *b=cb->data;
    LONG localbreakpoint=breakpoint-block;

    if(localbreakpoint<0 || localbreakpoint>=globals->blocks_inbitmap) {
      localbreakpoint=0;
    }

    while((bitend=bmflo(b->bitmap, bitstart))>=0) {

      /* Found the end of an empty space, now find out how large it is. */

      if(bitend < localbreakpoint) {       // Check if the end of this empty space is within bounds.
        break;
      }

      if(bitend!=globals->blocks_inbitmap-1) {
        space=0;
      }

      bitstart=bmflz(b->bitmap, bitend);

      if(bitstart+1 < localbreakpoint) {
        bitstart=localbreakpoint-1;
      }

      space+=bitend-bitstart;

      if(*returned_blocks<space) {
        *returned_block=block+bitstart+1;
        if(space>=maxneeded) {
          *returned_block=*returned_block+space-maxneeded;
          *returned_blocks=maxneeded;
          return(0);
        }
        *returned_blocks=space;
      }

      if(bitstart<=localbreakpoint) {
        break;
      }
    }

    if(prevbitmapblock<bitmapblock) {
      break;
    }

    /* no (more) empty spaces found in this block */

    if(bitstart!=-1) {
      space=0;
    }

    bitstart=globals->blocks_inbitmap-1;
    block-=globals->blocks_inbitmap;
  }

  return(errorcode);
}
