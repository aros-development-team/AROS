#include "asmsupport.h"

#include <dos/dos.h>
#include <exec/lists.h>
#include <exec/types.h>
#include <proto/exec.h>

#include "cachedio_protos.h"
#include "deviceio.h"
#include "deviceio_protos.h"
#include "req_protos.h"

#define removem(n)    (n)->mln_Succ->mln_Pred=(n)->mln_Pred; (n)->mln_Pred->mln_Succ=(n)->mln_Succ
#define addtailm(l,n) (n)->mln_Succ=(l)->mlh_TailPred->mln_Succ; (l)->mlh_TailPred->mln_Succ=(n); (n)->mln_Pred=(l)->mlh_TailPred; (l)->mlh_TailPred=(n)
#define addheadm(l,n) (n)->mln_Succ=(l)->mlh_Head; (n)->mln_Pred=(struct MinNode *)(l); (l)->mlh_Head->mln_Pred=(n); (l)->mlh_Head=(n);

#include "debug.h"

#include "globals.h"

static LONG copybackiocache(struct IOCache *ioc);

/* Internal structures */

struct IOCache {
  struct MinNode node;   /* LRU chain */

  struct IOCache *nexthash;
  struct IOCache *prevhash;

  void *data;

  ULONG block;           /* Unused IOCache has blocks = 0 */
  ULONG blocks;

  ULONG dirty[4];        /* Set bits indicate blocks which need to be written to disk. */
  ULONG valid[4];        /* Set bits indicate blocks which contain up-to-date data. */

  UBYTE bits;            /* See defines below */
  UBYTE locked;          /* Indicates that lruiocache should not return this iocache */
  UWORD pad3;

  /* Possible combinations for dirty and valid:

     dirty         : Illegal value.
     valid         : Block is the same as on disk.
     valid & dirty : Block is newer than the one on disk and must be flushed.
     (none)        : Block has not yet been read from disk. */

};

/* defines for IOCache bits */

#define IOC_DIRTY  (1)   /* IOCache contains dirty data */

/*

Functions making use of the IOCache mechanism:

read - reads one or more blocks into a buffer.
write - writes one or more blocks from a buffer.

The IOCache system is a simple caching mechanism intended to
make maximum use of small reads.  The filesystem likes to
read single blocks, which are usually only 512 bytes in
size.  Most harddisks however can read a lot more data in a
single access without a speed penalty.

This is where the IOCaches come in.  Instead of reading just
512 bytes, the system will read for example 4096 bytes.  It
reads this information into IOCache buffers.  The data
actually required is copied from the IOCache into the
supplied buffer.

This makes Writes a little bit more complex.  Because data
can reside in the cache each Write must be checked to see if
one of the IOCaches doesn't need to be updated or flushed.
Whether an IOCache must be updated or flushed depends on the
mode the cache operates in.  WriteThrough caching means the
buffers are just updated and kept in memory.  Otherwise a
buffer is simply marked invalid.

*/



void hashit(struct IOCache *ioc) {
  WORD hashentry=(ioc->block>>globals->iocache_shift) & (IOC_HASHSIZE-1);

  ioc->nexthash=globals->ioc_hashtable[hashentry];
  if(ioc->nexthash!=0) {
    ioc->nexthash->prevhash=ioc;
  }
  ioc->prevhash=0;
  globals->ioc_hashtable[hashentry]=ioc;
}



static void dehash(struct IOCache *ioc) {
  if(ioc->blocks!=0) {
    if(ioc->nexthash!=0) {
      ioc->nexthash->prevhash=ioc->prevhash;
    }
    if(ioc->prevhash!=0) {
      ioc->prevhash->nexthash=ioc->nexthash;
    }
    else {
      globals->ioc_hashtable[ (ioc->block>>globals->iocache_shift) & (IOC_HASHSIZE-1)]=ioc->nexthash;    /* Aug 11 1998: changed '=0' to '=ioc->nexthash' !! */
    }
  }

  ioc->nexthash=0;
  ioc->prevhash=0;
}



void invalidateiocache(struct IOCache *ioc) {
  dehash(ioc);
  ioc->blocks=0;
  ioc->block=0;
  ioc->bits=0;
//  ioc->dirtylow=255;
//  ioc->dirtyhigh=0;

  ioc->dirty[0]=0;
  ioc->dirty[1]=0;
  ioc->dirty[2]=0;
  ioc->dirty[3]=0;

  ioc->valid[0]=0;
  ioc->valid[1]=0;
  ioc->valid[2]=0;
  ioc->valid[3]=0;
}



void free(struct MinList *lruhead) {

  /* Frees all IOCache buffers attached to the passed in list and the
     listheader itself.  If lruhead is zero then this function does
     nothing. */

  if(lruhead!=0) {
    while(lruhead->mlh_TailPred != (struct MinNode *)lruhead) {
      struct IOCache *ioc=(struct IOCache *)lruhead->mlh_Head;

      removem(lruhead->mlh_Head);

      FreeVec(ioc);
    }

    FreeMem(lruhead, sizeof(struct MinList));
  }
}



struct MinList *allocate(ULONG size, LONG n) {
  struct MinList *lruhead;

  /* Allocates n IOCache buffers of /size/ bytes and attaches
     them to the returned MinList.  The MinList is returned
     and is zero if there wasn't enough memory. */

  if((lruhead=AllocMem(sizeof(struct MinList), globals->bufmemtype))!=0) {
    struct IOCache *ioc;

    lruhead->mlh_Head=(struct MinNode *)&lruhead->mlh_Tail;
    lruhead->mlh_Tail=0;
    lruhead->mlh_TailPred=(struct MinNode *)lruhead;

    size+=sizeof(struct IOCache)+16;

    while(--n>=0 && (ioc=AllocVec(size, globals->bufmemtype))!=0) {
      ioc->blocks=0;       // Mar 11 1999: Added this line to avoid that dehash() makes a mess of things.
      ioc->nexthash=0;
      ioc->prevhash=0;
      ioc->locked=0;
      invalidateiocache(ioc);

      /* ioc->data is aligned to 16 byte boundaries. */
      ioc->data=(UBYTE *)((IPTR)((UBYTE *)ioc+sizeof(struct IOCache)+15) & (~0x0F));

      addtailm(lruhead, &ioc->node);
    }

    if(n<0) {
      return(lruhead);
    }

    free(lruhead);
  }

  return(0);
}



ULONG queryiocache_lines(void) {
  return(globals->iocache_lines);
}



ULONG queryiocache_readaheadsize(void) {
  return(globals->iocache_sizeinblocks << globals->shifts_block);
}



BYTE queryiocache_copyback(void) {
  return(globals->iocache_copyback);
}



LONG setiocache(ULONG lines, ULONG readahead, BYTE copyback) {
  struct MinList *lruhead;
  ULONG sizeinblocks=readahead>>globals->shifts_block;
  WORD shift;

  /* This function changes the size and type of the IOCache.  The
     old Cache settings will remain in effect if the new ones
     couldn't be applied due to lack of memory.

     When this function is called first time, there are no old
     settings! */

  if(sizeinblocks<4) {
    shift=1;
  }
  else if(sizeinblocks<8) {
    shift=2;
  }
  else if(sizeinblocks<16) {
    shift=3;
  }
  else if(sizeinblocks<32) {
    shift=4;
  }
  else if(sizeinblocks<64) {
    shift=5;
  }
  else if(sizeinblocks<128) {
    shift=6;
  }
  else {
    shift=7;
  }

  sizeinblocks=1<<shift;

  if(lines<4) {
    lines=4;
  }
  else if(lines>1024) {
    lines=1024;
  }

  if((lruhead=allocate(sizeinblocks<<globals->shifts_block, lines))!=0) {
    LONG errorcode=0;

    if(globals->iocache_lruhead==0 || (errorcode=flushiocache())==0) {
      WORD m=IOC_HASHSIZE;

      globals->iocache_sizeinblocks=sizeinblocks;
      globals->iocache_lines=lines;
      globals->iocache_copyback=copyback;
      if(copyback==FALSE) {
        globals->iocache_readonwrite=TRUE;
      }
      globals->iocache_mask=sizeinblocks-1;
      globals->iocache_shift=shift;

      free(globals->iocache_lruhead);
      globals->iocache_lruhead=lruhead;

      if(globals->iocache_readonwrite==FALSE && globals->iocache_copyback!=FALSE) {
        globals->ioc_buffer=(struct IOCache *)globals->iocache_lruhead->mlh_Head;
        globals->ioc_buffer->locked=TRUE;
      }

      while(--m>=0) {
        globals->ioc_hashtable[m]=0;
      }
    }
    else {
      free(lruhead);
    }

    return(errorcode);
  }

  return(ERROR_NO_FREE_STORE);
}




/* The IOCache is automatically disabled when iocache_lines == 0 */

LONG initcachedio(UBYTE *devicename, IPTR unit, ULONG flags, struct DosEnvec *de)
{
  LONG errorcode;

  if((errorcode=initdeviceio(devicename, unit, flags, de))==0) {

    /* Note: There MUST be atleast 4 IOCache_lines for cachedio to work correctly at the moment!! */

    if((setiocache(8, 8192, TRUE))==0) {
      return(0);
    }
  }

  return(errorcode);
}



void cleanupcachedio(void) {

  /* Only call this if initcachedio() was succesful. */

  flushiocache();    /*** returns an errorcode... */
  free(globals->iocache_lruhead);
  globals->iocache_lruhead=0;

  globals->iocache_lines=0;

  cleanupdeviceio();
}



struct IOCache *findiocache(BLCK block) {
  struct IOCache *ioc=globals->ioc_hashtable[ (block>>globals->iocache_shift) & (IOC_HASHSIZE-1) ];

  /* For internal use only.  This function will find the IOCache, if available.
     It won't move the block to the end of the LRU chain though -- use locateiocache
     instead. */

  while(ioc!=0) {
    if(block>=ioc->block && block<ioc->block+ioc->blocks) {
      return(ioc);
    }

    ioc=ioc->nexthash;
  }

  return(0);
}



struct IOCache *locateiocache(BLCK block) {
  struct IOCache *ioc;

  if((ioc=findiocache(block))!=0) {
    removem(&ioc->node);
    addtailm(globals->iocache_lruhead, &ioc->node);
  }

  return(ioc);
}



LONG lruiocache(struct IOCache **returned_ioc) {
  struct IOCache *ioc;
  LONG errorcode;

  /* Must be volatile to keep ioc variable value up to date regardless of
     compiler optimizations */
  volatile struct MinList *ioclist = globals->iocache_lruhead;

  /* Returns the least recently used IOCache */

  do {
    ioc=(struct IOCache *) ioclist->mlh_Head;

    removem(&ioc->node);
    addtailm(globals->iocache_lruhead, &ioc->node);

  } while(ioc->locked!=0);

  if((errorcode=copybackiocache(ioc))!=0) {
    return(errorcode);
  }
  invalidateiocache(ioc);

  *returned_ioc=ioc;

  return(0);
}



void reuseiocache(struct IOCache *ioc) {

  /* This function makes sure that the passed in IOCache is reused
     as quickly as possible.  This is a good idea if you used an IOCache
     and you're certain it won't be needed again.  In such a case you
     can call this function so this cache is the first to be reused. */

  removem(&ioc->node);
  addheadm(globals->iocache_lruhead, &ioc->node);
}



LONG validateiocache(struct IOCache *ioc, ULONG blockoffset, ULONG blocks) {
  LONG errorcode;

  /* This function will read the missing data from this IOCache from disk
     and merge it with any existing dirty blocks.  To reduce copying it
     will either use the newly read IOCache or the current IOCache to
     store the final result.  It will switch the data pointers so there is
     no need for rehashing. */

  if(globals->iocache_readonwrite!=FALSE || bmtsto(ioc->valid, blockoffset, blocks)!=0) {
    return(0);
  }

//  _DEBUG(("validateiocache: ioc->block = %ld, ioc->blocks = %ld, ioc->dirty = 0x%08lx, ioc->valid = 0x%08lx\n", ioc->block, ioc->blocks, ioc->dirty[0], ioc->valid[0]));

  if((errorcode=transfer(DIO_READ, globals->ioc_buffer->data, ioc->block, ioc->blocks))==0) {
    LONG i=globals->iocache_sizeinblocks;

//    _DEBUG(("validateiocache: BMCNTO returned %ld\n", BMCNTO(ioc->dirty, 0, ioc->blocks)));

    if(bmcnto(ioc->dirty, 0, ioc->blocks) < globals->iocache_sizeinblocks/2) {
      void *data;

      /* Copying the dirty blocks to the new IOCache. */

//      _DEBUG(("validateiocache: Using new IOCache\n"));

      while(--i>=0) {
        if(bmtsto(ioc->dirty, i, 1)!=FALSE) {
          CopyMemQuick((UBYTE *)ioc->data + (i<<globals->shifts_block), (UBYTE *)globals->ioc_buffer->data + (i<<globals->shifts_block), globals->bytes_block);
        }
      }

      data=globals->ioc_buffer->data;
      globals->ioc_buffer->data=ioc->data;
      ioc->data=data;
    }
    else {

      /* Copying the newly read blocks to the existing IOCache. */

//      _DEBUG(("validateiocache: Using existing IOCache\n"));

      while(--i>=0) {
        if(bmtstz(ioc->dirty, i, 1)!=FALSE) {
          CopyMemQuick((UBYTE *)globals->ioc_buffer->data + (i<<globals->shifts_block), (UBYTE *)ioc->data + (i<<globals->shifts_block), globals->bytes_block);
        }
      }
    }

    ioc->valid[0]=0xFFFFFFFF;
    ioc->valid[1]=0xFFFFFFFF;
    ioc->valid[2]=0xFFFFFFFF;
    ioc->valid[3]=0xFFFFFFFF;
  }

  return(errorcode);
}



static LONG copybackiocache(struct IOCache *ioc) {
  LONG errorcode=0;
  LONG dirtylow, dirtyhigh;

  /* Writes out any dirty data, and resets the dirty bit.

     For extra efficiency this function will in case of a physical
     disk-access also flush any buffers following this one, to avoid
     physical head movement. */

//  _DEBUG(("copybackiocache: ioc->block = %ld\n", ioc->block));

  while(ioc!=0 && ioc->blocks!=0 && (ioc->bits & IOC_DIRTY)!=0) {
    
    _DEBUG(("copybackiocache: ioc->dirty=%p (@=%08x) ioc->blocks-1=%d\n", ioc->dirty, AROS_BE2LONG(*(ULONG*)ioc->dirty), ioc->blocks-1));

    if((dirtyhigh=bmflo(ioc->dirty, ioc->blocks-1))<0) {
      _DEBUG(("copybackiocache: Say what?\n"));
      break;
//    dirtyhigh = ioc->blocks-1;
    }

    dirtylow=bmffo(ioc->dirty, 4, 0);

//    _DEBUG(("copybackiocache: dirtylow = %ld, dirtyhigh = %ld, ioc->dirty = 0x%08lx\n", dirtylow, dirtyhigh, ioc->dirty[0]));

    /* dirtylow and dirtyhigh are known.  Now, to check if we can write
       all these changes in a single write we check if all the blocks
       between dirtylow and dirtyhigh are VALID (not dirty, although
       most of them probably will be dirty). */

    if(bmffz(ioc->valid, 4, dirtylow)<dirtyhigh) {
//      _DEBUG(("copybackiocache: calling validateiocache\n"));
      if((errorcode=validateiocache(ioc, 0, ioc->blocks))!=0) {
        break;
      }
    }

    if((errorcode=transfer(DIO_WRITE, (UBYTE *)ioc->data + (dirtylow<<globals->shifts_block), ioc->block + dirtylow, dirtyhigh - dirtylow + 1))!=0) {
      break;
    }

    ioc->bits&=~IOC_DIRTY;
    ioc->dirty[0]=0;
    ioc->dirty[1]=0;
    ioc->dirty[2]=0;
    ioc->dirty[3]=0;
//    ioc->dirtylow=255;
//    ioc->dirtyhigh=0;

    ioc=findiocache(ioc->block+ioc->blocks);
  }

  return(errorcode);
}



LONG flushiocache(void) {
  struct IOCache *ioc;
  LONG errorcode=0;

  /* Writes all dirty data to disk, but keeps the cached data for
     later reads.  Use this to ensure data is comitted to disk
     when doing critical operations. */

  ioc=(struct IOCache *)globals->iocache_lruhead->mlh_Head;

  while(ioc->node.mln_Succ!=0) {
    if((errorcode=copybackiocache(ioc))!=0) {
      break;
    }

    ioc=(struct IOCache *)(ioc->node.mln_Succ);
  }

  if(errorcode==0) {
    update();
  }

//  _DEBUG(("flushiocache: errorcode = %ld\n", errorcode));

  return(errorcode);
}



void invalidateiocaches(void) {
  struct IOCache *ioc;

  /* Clears all buffers in the IOCache.  This should be used BEFORE
     directly writing to the disk (for example, call this before
     ACTION_INHIBIT(TRUE)).  Before calling this function make
     sure all pending changes have been flushed using flushiocache() */

  ioc=(struct IOCache *)globals->iocache_lruhead->mlh_Head;

  while(ioc->node.mln_Succ!=0) {
    invalidateiocache(ioc);

    ioc=(struct IOCache *)(ioc->node.mln_Succ);
  }
}



/*
void copyiocachetobuffer(struct IOCache *ioc, BLCK *block, UBYTE **buffer, ULONG *blocks) {
  ULONG blockoffset=*block-ioc->block;
  ULONG blocklength=ioc->blocks-blockoffset;

  if(*blocks<blocklength) {
    blocklength=*blocks;
  }

  if(((ULONG)(*buffer) & 0x00000003) != 0) {
    CopyMem((UBYTE *)ioc->data + (blockoffset<<shifts_block), *buffer, blocklength<<shifts_block);
  }
  else {
    CopyMemQuick((UBYTE *)ioc->data + (blockoffset<<shifts_block), *buffer, blocklength<<shifts_block);
  }

  *block+=blocklength;
  *blocks-=blocklength;
  *buffer+=blocklength<<shifts_block;
}
*/


LONG readintocache(BLCK block, struct IOCache **returned_ioc) {
  struct IOCache *ioc;
  LONG errorcode=0;

  if((ioc=locateiocache(block))==0) {
    if((errorcode=lruiocache(&ioc))==0) {
      ULONG blockstart=block & ~globals->iocache_mask;
      ULONG blocklength=globals->iocache_sizeinblocks;

      if(blockstart+blocklength>globals->blocks_total) {
        blocklength=globals->blocks_total-blockstart;
      }

      if((errorcode=transfer(DIO_READ, ioc->data, blockstart, blocklength))==0) {
        ioc->block=blockstart;
        ioc->blocks=blocklength;

        ioc->valid[0]=0xFFFFFFFF;
        ioc->valid[1]=0xFFFFFFFF;
        ioc->valid[2]=0xFFFFFFFF;
        ioc->valid[3]=0xFFFFFFFF;

        hashit(ioc);
      }
    }
  }

  *returned_ioc=ioc;

  return(errorcode);
}



LONG readonwriteintocache(BLCK block, struct IOCache **returned_ioc) {
  struct IOCache *ioc;
  LONG errorcode=0;

  /* Only does a physical read if iocache_readonwrite is TRUE */

  if((ioc=locateiocache(block))==0) {
    if((errorcode=lruiocache(&ioc))==0) {
      ULONG blockstart=block & ~globals->iocache_mask;
      ULONG blocklength=globals->iocache_sizeinblocks;

      if(blockstart+blocklength>globals->blocks_total) {
        blocklength=globals->blocks_total-blockstart;
      }

      if(globals->iocache_readonwrite==FALSE || (errorcode=transfer(DIO_READ, ioc->data, blockstart, blocklength))==0) {
        ioc->block=blockstart;
        ioc->blocks=blocklength;

        if(globals->iocache_readonwrite!=FALSE) {
          ioc->valid[0]=0xFFFFFFFF;
          ioc->valid[1]=0xFFFFFFFF;
          ioc->valid[2]=0xFFFFFFFF;
          ioc->valid[3]=0xFFFFFFFF;
        }

        hashit(ioc);
      }
    }
  }

  *returned_ioc=ioc;

  return(errorcode);
}



static LONG copybackoverlappingiocaches(BLCK block, ULONG blocks) {
  struct IOCache *ioc;
  BLCK lastblock;
  LONG errorcode=0;

  /* This function copies back any IOCaches which fall (partially) in the
     region specified by the input parameters. */

  lastblock=(block+blocks-1) & ~globals->iocache_mask;
  block=block & ~globals->iocache_mask;

  while(block<=lastblock) {                       // Aug 6 1998: Changed '<' into '<='.
    if((ioc=locateiocache(block))!=0) {
      if((errorcode=copybackiocache(ioc))!=0) {
        break;
      }
    }
    block+=globals->iocache_sizeinblocks;
  }

  return(errorcode);
}



LONG readbytes(BLCK block, UBYTE *buffer, UWORD offsetinblock, UWORD bytes) {
  struct IOCache *ioc;
  LONG errorcode;

  /* This function is intended to copy data directly from a IOCache buffer
     which was read.  It can be used for small amounts of data only (1 to
     bytes_block bytes)

     This function will fall back to reading a single block if the cache
     is disabled. */

  if((errorcode=readintocache(block, &ioc))==0 && (errorcode=validateiocache(ioc, block-ioc->block, 1))==0) {
    CopyMem((UBYTE *)ioc->data+((block-ioc->block)<<globals->shifts_block) + offsetinblock, buffer, bytes);
  }

  return(errorcode);
}



LONG writebytes(BLCK block, UBYTE *buffer, UWORD offsetinblock, UWORD bytes) {
  struct IOCache *ioc;
  LONG errorcode;

  /* This function is intended to copy data directly into a IOCache buffer
     which was read.  It can be used for small amounts of data only (1 to
     bytes_block bytes).

     This function will fall back to reading/modifying/writing a single
     block if the cache or copyback caching is disabled. */

  if((errorcode=readintocache(block, &ioc))==0 && (errorcode=validateiocache(ioc, block-ioc->block, 1))==0) {
    CopyMem(buffer, (UBYTE *)ioc->data + ((block-ioc->block)<<globals->shifts_block) + offsetinblock, bytes);
    if(globals->iocache_copyback==FALSE) {
      errorcode=write(block, (UBYTE *)ioc->data+((block-ioc->block)<<globals->shifts_block), 1);
    }
    else {
      bmset(ioc->dirty, 4, block-ioc->block, 1);
      ioc->bits|=IOC_DIRTY;
    }
  }

  return(errorcode);
}



LONG read(BLCK block, UBYTE *buffer, ULONG blocks) {
  LONG errorcode=0;

  if(blocks!=0) {

    /* The readahead caching system works simple; if the data-request is lesser
       than or equal to the line-size than we first try to locate the data in
       the cache.  If it is available it is copied and returned.  If the data
       isn't available then it is read into the cache, and copied afterwards.

       Large requests are processed seperately and don't go through the cache.
       The idea is that large requests are quite fast when loaded from the HD
       anyway.  Also, to be able to properly speed up even large requests you'd
       also need a substantially larger cache, which isn't what we want here. */

    if(globals->iocache_lines!=0 && blocks<=globals->iocache_sizeinblocks>>1) {        // ****** // dit kan sneller, als het ondanks het te grote request toch in de cache staat!
      struct IOCache *ioc;

      while(errorcode==0 && blocks!=0) {
        if((errorcode=readintocache(block, &ioc))==0) {
          ULONG blockoffset=block-ioc->block;
          ULONG blocklength=ioc->blocks-blockoffset;

          if(blocks<blocklength) {
            blocklength=blocks;
          }

          if((errorcode=validateiocache(ioc, blockoffset, blocklength))!=0) {
            break;
          }

          if(((IPTR)buffer & 0x00000003) != 0) {
            CopyMem((UBYTE *)ioc->data + (blockoffset<<globals->shifts_block), buffer, blocklength<<globals->shifts_block);
          }
          else {
            CopyMemQuick((UBYTE *)ioc->data + (blockoffset<<globals->shifts_block), buffer, blocklength<<globals->shifts_block);
          }

          block+=blocklength;
          blocks-=blocklength;
          buffer+=blocklength<<globals->shifts_block;
        }
      }
    }
    else {
      if((errorcode=copybackoverlappingiocaches(block, blocks))==0) {
        errorcode=transfer(DIO_READ, buffer, block, blocks);
      }
    }
  }

  return(errorcode);
}



void writethroughoverlappingiocaches(BLCK block, ULONG blocks, UBYTE *buffer) {
  struct IOCache *ioc;
  BLCK firstblock;
  BLCK lastblock;

  /* This function copies data from the buffer to any IOCaches which fall (partially)
     in the region specified by the input parameters. */

  firstblock=block & ~globals->iocache_mask;
  lastblock=(block+blocks-1) & ~globals->iocache_mask;

  while(firstblock<=lastblock) {
    if((ioc=locateiocache(firstblock))!=0) {
      ULONG offsetinline;
      BLCK startinline;
      UBYTE *src;
      UBYTE *dst;
      ULONG maxinline, overlappedblocks;

//      _DEBUG(("IOCACHE: found overlapping cache (%ld-%ld) for block %ld of %ld blocks\n",ioc->block,ioc->block+ioc->blocks-1,block,blocks));

      /* |-------|            |-----|               |-------|            |---------|
           |=======|        |=========|           |=======|                |=====|

      block  blocks  firstblock  offsetinline  overlappedblocks
      ---------------------------------------------------------
        1      20        0           1                7     21 -( 0 + 1) = 20
                         8           0                8     21 -( 8 + 0) = 13
                        16           0                5     21 -(16 + 0) = 5
      ---------------------------------------------------------
        1       3        0           1                3      4 -( 0 + 1) = 3
       20       3       16           4                3     23 -(16 + 4) = 3
      ---------------------------------------------------------  */

      offsetinline=firstblock<block ? block-firstblock : 0;
      startinline=firstblock+offsetinline;
      maxinline=ioc->blocks-offsetinline;
      overlappedblocks=block+blocks-startinline;

      if(overlappedblocks>maxinline) {
        overlappedblocks=maxinline;
      }

      /* startblock and endblock (exclusive) now contain the region to be
         overwritten in this IOCache. */

      if(globals->iocache_copyback!=FALSE && (ioc->bits & IOC_DIRTY)!=0) {

        /* Copyback mode is active!  We need to unmark any dirty blocks
           which are now overwritten. */

        bmclr(ioc->dirty, 4, offsetinline, overlappedblocks);
      }

      bmset(ioc->valid, 4, offsetinline, overlappedblocks);

      src=buffer + ((startinline-block)<<globals->shifts_block);
      dst=(UBYTE *)ioc->data + (offsetinline<<globals->shifts_block);

      if(((IPTR)buffer & 0x00000003) != 0) {
        CopyMem(src, dst, overlappedblocks<<globals->shifts_block);
      }
      else {
        CopyMemQuick(src, dst, overlappedblocks<<globals->shifts_block);
      }
    }

    firstblock+=globals->iocache_sizeinblocks;
  }
}



LONG writethrough(BLCK block, UBYTE *buffer, ULONG blocks) {

  /* This function writes data to disk, it writes through existing
     cache buffers but doesn't cause reads if cache is in copyback
     mode. */

  if(globals->iocache_lines!=0) {
    writethroughoverlappingiocaches(block,blocks,buffer);
  }

  return(transfer(DIO_WRITE,buffer,block,blocks));
}



LONG write(BLCK block, UBYTE *buffer, ULONG blocks) {

  if(globals->iocache_lines!=0) {
    ULONG maxblocks=globals->iocache_sizeinblocks>>2;

    if(maxblocks==0) {
      maxblocks=1;
    }

    if(globals->iocache_copyback!=FALSE && blocks<=maxblocks) {
      struct IOCache *ioc;
      struct IOCache *ioc2;
      LONG errorcode;

      if((errorcode=readonwriteintocache(block, &ioc))==0) {              /* a trick, which works because cachesystem consists of atleast 4 blocks. */
        if((errorcode=readonwriteintocache(block+blocks-1, &ioc2))==0) {
          WORD offsetinline=block-ioc->block;

          writethroughoverlappingiocaches(block, blocks, buffer);  /* This function kills dirty blocks if needed. */

          ioc->bits|=IOC_DIRTY;

          if(ioc==ioc2) {
            bmset(ioc->dirty, 4, offsetinline, blocks);
            bmset(ioc->valid, 4, offsetinline, blocks);
          }
          else {
            LONG blocks2=globals->iocache_sizeinblocks-offsetinline;

            ioc2->bits|=IOC_DIRTY;

            bmset(ioc->dirty,  4, offsetinline, blocks2);
            bmset(ioc->valid,  4, offsetinline, blocks2);
            bmset(ioc2->dirty, 4, 0, blocks-blocks2);
            bmset(ioc2->valid, 4, 0, blocks-blocks2);
          }
        }
      }

      return(errorcode);
    }
  }

  return(writethrough(block, buffer, blocks));
}



LONG getbuffer(UBYTE **tempbuffer, ULONG *maxblocks) {
  struct IOCache *ioc;

  lruiocache(&ioc);         /** Might fail!!  Make sure it never does! */
  reuseiocache(ioc);
  *tempbuffer=ioc->data;
  *maxblocks=globals->iocache_sizeinblocks;

  return(0);
}
