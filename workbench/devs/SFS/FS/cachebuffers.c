#include "asmsupport.h"

#include <exec/memory.h>
#include <exec/types.h>
#include <proto/exec.h>

#include "cachebuffers.h"
#include "cachebuffers_protos.h"

#include "bitmap_protos.h"
#include "debug.h"

#include "cachedio_protos.h"

#include "support_protos.h"
#include "transactions_protos.h"
#include "transactions.h"

#include "globals.h"

extern LONG req(UBYTE *fmt, UBYTE *gads, ... );
extern LONG req_unusual(UBYTE *fmt, ... );

extern void outputcachebuffer(struct CacheBuffer *cb);
extern void dumpcachebuffers(void);
extern void dreq(UBYTE *fmt, ... );

extern void setchecksum(struct CacheBuffer *);

/* Internal globals */

LONG initcachebuffers(void) {
  WORD n=HASHSIZE;

  initlist((struct List *)&globals->cblrulist);
  while(n--!=0) {
    initlist((struct List *)&globals->cbhashlist[n]);
  }

  return(0);
}



static void checkcb(struct CacheBuffer *cb,UBYTE *string) {
//  if(cb->id!=0x4A48 || cb->data!=&cb->attached_data[0] || (cb->bits & (CB_ORIGINAL|CB_EMPTY))==(CB_ORIGINAL|CB_EMPTY) || (cb->bits & (CB_ORIGINAL|CB_LATEST))==(CB_ORIGINAL|CB_LATEST) || (cb->bits & (CB_ORIGINAL|CB_LATEST|CB_EMPTY))==CB_EMPTY) {
  if(cb->id!=0x4A48 || cb->data!=&cb->attached_data[0] || (cb->bits & (CB_ORIGINAL|CB_EMPTY))==(CB_ORIGINAL|CB_EMPTY) || (cb->bits & (CB_ORIGINAL|CB_LATEST|CB_EMPTY))==CB_EMPTY) {

    /* Aargh, this doesn't seem to be a REAL cachebuffer... */

    req_unusual("Function '%s' detected an invalid CacheBuffer!", string);

    _DEBUG(("checkcb: *** Not a valid cachebuffer!! ***\nDetected by function '%s'\n",string));
    outputcachebuffer(cb);
    dumpcachebuffers();
  }
}


struct CacheBuffer *findoriginalcachebuffer(BLCK blckno) {
  struct CacheBuffer *cb;

  cb=(struct CacheBuffer *)(globals->cbhashlist[blckno & (HASHSIZE-1)].mlh_Head-1);

  while(cb->hashnode.mln_Succ!=0) {
    if(cb->blckno==blckno && (cb->bits & CB_ORIGINAL)!=0) {
      return(cb);
    }
    cb=(struct CacheBuffer *)(cb->hashnode.mln_Succ-1);
  }

  return(0);
}



struct CacheBuffer *findlatestcachebuffer(BLCK blckno) {
  struct CacheBuffer *cb;

  cb=(struct CacheBuffer *)(globals->cbhashlist[blckno & (HASHSIZE-1)].mlh_Head-1);

  while(cb->hashnode.mln_Succ!=0) {
    if(cb->blckno==blckno && (cb->bits & CB_LATEST)!=0) {
      return(cb);
    }
    cb=(struct CacheBuffer *)(cb->hashnode.mln_Succ-1);
  }

  return(0);
}



static __inline void mrucachebuffer(struct CacheBuffer *cb) {

  /* Moves the passed in CacheBuffer to the end of the LRU list.
     This means it becomes the MRU CacheBuffer. */

  removem(&cb->node);
  addtailm(&globals->cblrulist,&cb->node);
}



LONG readcachebuffer(struct CacheBuffer **returned_cb, BLCK block) {

  /* Obtains the specified cachebuffer by any means necessary.  This
     function will always obtain the latest version of the block in
     question.  It first looks for a Cachebuffer currently being
     modified and returns that if found.  Otherwise it reads the
     original cachebuffer and applies the most recent changes to it. */

  _XDEBUG((DEBUG_CACHEBUFFER,"    readcb: block %ld\n",block));

  globals->statistics.cache_accesses++;

  if((*returned_cb=findlatestcachebuffer(block))==0) {
    return(applyoperation(block, returned_cb));
  }

  checkcb(*returned_cb,"readcachebuffer");

  mrucachebuffer(*returned_cb);

  if(((*returned_cb)->bits & CB_LATEST)==0) {
    dreq("readcachebuffer didn't return the latest cachebuffer!\nPlease notify the author!");
    outputcachebuffer(*returned_cb);
  }

  return(0);
}



LONG readoriginalcachebuffer(struct CacheBuffer **returned_cb,BLCK blckno) {
  struct CacheBuffer *cb;
  LONG errorcode;

  /* Reads a cachebuffer from disk (if needed).  The cachebuffer will not
     be locked.  Note that this function returns the original cachebuffer
     (as currently stored on disk)! */

  if((cb=findoriginalcachebuffer(blckno))!=0) {
    /* We managed to find the original! */

    _XDEBUG((DEBUG_CACHEBUFFER,"    readorgcb: block %ld (from cache)\n",blckno));

    mrucachebuffer(cb);
  }
  else if((cb=getcachebuffer())!=0) {

    _XDEBUG((DEBUG_CACHEBUFFER,"    readorgcb: block %ld (from disk)\n",blckno));

    #ifdef CHECKCODE
      if(findlatestcachebuffer(blckno)!=0) {
        dreq("readoriginalcachebuffer: Fatal error!\nPlease notify the author!");
      }
    #endif

    /* We found an empty cachebuffer in which we can read the original now. */

    globals->statistics.cache_misses++;

    if((errorcode=read(blckno,cb->data,1))!=0) {
      return(errorcode);
    }

    cb->blckno=blckno;
    cb->bits=CB_ORIGINAL;

    if(isthereanoperationfor(blckno)==FALSE) {
      cb->bits|=CB_LATEST;
    }

    addtailm(&globals->cbhashlist[blckno & (HASHSIZE-1)],&cb->hashnode);
  }
  else {
    return(ERROR_NO_FREE_STORE);
  }

  /* We either found the original, or just read it succesfully. */

  *returned_cb=cb;
  return(0);
}



void emptyoriginalcachebuffer(BLCK blckno) {
  struct CacheBuffer *cb;

  if((cb=findoriginalcachebuffer(blckno))!=0) {
    /* We managed to find the original! */
    emptycachebuffer(cb);
  }
}



void resetcachebuffer(struct CacheBuffer *cb) {
  /* Resets the CacheBuffer to its default state.  All fields are resetted
     to their defaults, the CacheBuffer will be properly delinked */

  checkcb(cb,"resetcachebuffer");

  #ifdef CHECKCODE
    if(cb->locked!=0) {
      dreq("resetcachebuffer: CacheBuffer is still locked!\nPlease notify the author!");
      outputcachebuffer(cb);
    }
  #endif

  if(cb->hashnode.mln_Succ!=0 && cb->hashnode.mln_Pred!=0) {
    removem(&cb->hashnode);
  }

  cb->hashnode.mln_Succ=0;
  cb->hashnode.mln_Pred=0;
  cb->locked=0;
  cb->bits=0;
  cb->blckno=0;
}



void emptycachebuffer(struct CacheBuffer *cb) {
  /* Empties the CacheBuffer so it can be used for new data.  All fields are
     resetted to their defaults, the CacheBuffer will be properly delinked */

  resetcachebuffer(cb);

  /* Add empty buffer to head of LRU chain.  This will not only increase
     performance when looking for a free buffer, but it will also ensure
     that EMPTY buffers are reused first, while otherwise a potentially
     useful buffer could be reused. */

  removem(&cb->node);
//  addheadm(&cblrulist,&cb->node);
  AddHead((struct List *)&globals->cblrulist,(struct Node *)&cb->node);
}



void lockcachebuffer(struct CacheBuffer *cb) {
  /* Make absolutely sure the cachebuffer in question is unlocked
     again (as many times as it was locked!), or face the
     consequences... */

  #ifdef CHECKCODE
    checkcb(cb,"lockcachebuffer");
  #endif

  #ifdef CHECKCODE
    if((cb->bits & (CB_ORIGINAL|CB_LATEST))==CB_ORIGINAL) {
      dreq("Original non-latest cachebuffers may not be locked.\nPlease notify the author!");
      outputcachebuffer(cb);
    }
  #endif

  cb->locked++;
}



void unlockcachebuffer(struct CacheBuffer *cb) {
  if(cb!=0 && cb->locked!=0) {
    #ifdef CHECKCODE
    checkcb(cb,"unlockcachebuffer");
    #endif
    cb->locked--;
  }

  #ifdef CHECKCODE
  else if(cb!=0) {
    dreq("unlockcachebuffer: cb->locked was zero!");
    outputcachebuffer(cb);
  }
  #endif
}



struct CacheBuffer *createnewcachebuffer(BLCK block) {
  struct CacheBuffer *cb;

  #ifdef CHECKCODE
    if(findlatestcachebuffer(block)!=0) {
      dreq("createnewcachebuffer: Fatal error!\nPlease notify the author!");
    }
  #endif

  cb=getcachebuffer();

  cb->blckno=block;
  cb->bits=CB_LATEST|CB_EMPTY;

  addtailm(&globals->cbhashlist[block & (HASHSIZE-1)],&cb->hashnode);

  return(cb);
}



struct CacheBuffer *newcachebuffer(BLCK block) {
  struct CacheBuffer *cb;

  /* Looks for an unused cachebuffer and clears it.  This cachebufer
     doesn't have an original (it was marked free, and thus contains
     junk) so it will be treated as if it was zero-filled. */

  /*

  CB_ORIGINAL -> Indicates there IS a later version (even if it isn't currently in cache).
  CB_ORIGINAL|CB_LATEST -> Indicates there were no modifications to this block ever.
  CB_LATEST -> Impossible, there must be a CB_ORIGINAL as well then.
  (nothing found) -> Create from empty.

  */

  /*** We probably should prevent CB_EMPTY style cachebuffers and normal
       style to be together in the cache at the same time.  Blocks which
       have been marked free should never be written out to disk anyway... */

  if((cb=findlatestcachebuffer(block))!=0) {
    preparecachebuffer(cb);
    clearcachebuffer(cb);
  }
  else {
    cb=createnewcachebuffer(block);

    clearcachebuffer(cb);
    lockcachebuffer(cb);
  }

  return(cb);
}



void preparecachebuffer(struct CacheBuffer *cb) {
  /* Prepares a cachebuffer to be changed.  A copy of the original is kept
     for later comparison, but the copy is available for reuse.  If the
     cachebuffer is already a newer version then no copy of the original
     needs to be made.

     Note: Because the this function doesn't require both the original and
           the modified version to be locked in memory, there will ALWAYS
           be enough room to succesfully execute this function.  In the
           worst case it will simply immediately use the current version! */

  #ifdef CHECKCODE
     if(globals->transactionnestcount==0) {
       dreq("No transaction was started when preparecachebuffer() was called!\nPlease notify the author!");
       outputcachebuffer(cb);
     }

     if((cb->bits & CB_LATEST)==0) {
       dreq("preparecachebuffer(): Only latest cachebuffers may be prepared!\nPlease notify the author!");
       outputcachebuffer(cb);
     }

    checkcb(cb,"preparecachebuffer");
  #endif


  /*

  CB_ORIGINAL -> Aren't allowed to be locked (and thus prepared).
  CB_ORIGINAL|CB_LATEST -> Make copy, and copy becomes CB_ORIGINAL.  cb becomes CB_LATEST.
  CB_LATEST -> Do nothing.

  */

  lockcachebuffer(cb);

  if((cb->bits & CB_ORIGINAL)!=0) {
    saveoriginalcachebuffer(cb);
  }
}





struct CacheBuffer *saveoriginalcachebuffer(struct CacheBuffer *cb) {
  struct CacheBuffer *cb_new;

  /* Makes a copy of the original CacheBuffer into another free CacheBuffer.
     The new location of the original CacheBuffer is returned.  The old
     location is converted into a CB_LATEST CacheBuffer. */

  #ifdef CHECKCODE
    if((cb->bits & CB_ORIGINAL)==0) {
      dreq("saveoriginalcachebuffer: Only original cachebuffers may be saved.\nPlease notify the author!");
    }
  #endif

//  lockcachebuffer(cb);      /* Lock original */
  cb->locked++;

  cb_new=getcachebuffer();

  unlockcachebuffer(cb);

  cb_new->blckno=cb->blckno;
  cb_new->bits=CB_ORIGINAL;

  cb->bits&=~(CB_ORIGINAL|CB_CHECKSUM);
  cb->bits|=CB_LATEST;

  CopyMemQuick(cb->data, cb_new->data, globals->bytes_block);

  addtailm(&globals->cbhashlist[cb_new->blckno & (HASHSIZE-1)],&cb_new->hashnode);

  return(cb_new);
}



static LONG compresscachebuffer(struct CacheBuffer *cb_org,struct CacheBuffer *cb_new) {
  UWORD length;
  UBYTE bits=0;

  /* cb_org can be 0, in which case we mean a CacheBuffer filled with zeroes. */

  /* This function creates a new transaction using the 2 cachebuffers passed in */

  if(cb_org!=0) {
    length=compress(cb_org->data,cb_new->data,globals->compressbuffer);
  }
  else {
    length=compressfromzero(cb_new->data,globals->compressbuffer);
  }

  if((cb_new->bits & CB_EMPTY)!=0) {
    bits=OI_EMPTY;
  }

  return(addoperation(cb_new->blckno,globals->compressbuffer,length,bits));
}



LONG storecachebuffer_nochecksum(struct CacheBuffer *cb) {
  struct CacheBuffer *cb_org=0;
  ULONG blocksfree;
  LONG errorcode;

  #ifdef CHECKCODE
     if(globals->transactionnestcount==0) {
       dreq("No transaction was started when storecachebuffer() was called!\nPlease notify the author!");
       outputcachebuffer(cb);
     }

     checkcb(cb,"storecachebuffer");
  #endif

  /* This function guarantees that the passed in cachebuffer is stil valid
     after calling this function.  This is because there still is a lock
     imposed by preparecachebuffer() which is only removed at the end of
     this function. */

  if((errorcode=getfreeblocks(&blocksfree))==0) {

    if(blocksfree >= transactionspace()) {
      /* Any changes made to this cachebuffer are stored in the transaction
         buffer. */

      if((cb->bits & CB_EMPTY)==0) {
        errorcode=readoriginalcachebuffer(&cb_org,cb->blckno);
      }

      if(errorcode==0) {
        #ifdef NOCOMPRESSION
          errorcode=addoperation2(cb_org,cb);
        #else
          errorcode=compresscachebuffer(cb_org,cb);
        #endif
      }
    }
    else {
      errorcode=ERROR_DISK_FULL;
    }

  }

  if(errorcode!=0) {
    dumpcachebuffer(cb);
  }
  else {
    unlockcachebuffer(cb);
  }

  return(errorcode);
}



LONG storecachebuffer(struct CacheBuffer *cb) {

  /* This function guarantees that the passed in cachebuffer is stil valid
     after calling this function.  This is because there still is a lock
     imposed by preparecachebuffer() which is only removed at the end of
     this function. */

  setchecksum(cb);

  return(storecachebuffer_nochecksum(cb));
}



#ifdef BLOCKCOMPRESSION

LONG changecachebuffer(struct CacheBuffer *cb, UBYTE *modifiedblocks) {
  struct CacheBuffer *cb_org=0;
  struct Operation *o;
  ULONG blocksfree;
  LONG errorcode;

  #ifdef CHECKCODE
     if(transactionnestcount==0) {
       dreq("No transaction was started when changecachebuffer() was called!\nPlease notify the author!");
       outputcachebuffer(cb);
     }

     checkcb(cb,"changecachebuffer");
  #endif

  /* This function guarantees that the passed in cachebuffer is stil valid
     after calling this function.  This is because there still is a lock
     imposed by preparecachebuffer() which is only removed at the end of
     this function. */

  BEGIN();

  if((o=getlatestoperation(cb->blckno))==0) {
    _DEBUG(("changecachebuffer: Using storecachebuffer()\n"));
    return(storecachebuffer(cb));
  }

  END("getlatestoperation()");

//  _DEBUG(("changecachebuffer: Using mergediffs()\n"));

  BEGIN();

  if((errorcode=getfreeblocks(&blocksfree))==0) {

    if(blocksfree >= transactionspace()) {
      /* Any changes made to this cachebuffer are stored in the transaction
         buffer. */

      if((cb->bits & CB_EMPTY)==0) {
        errorcode=readoriginalcachebuffer(&cb_org,cb->blckno);
      }

      END("getfreeblocks");

      if(errorcode==0) {
        UWORD length;
        UBYTE bits=0;

        BEGIN();

        length=mergediffs(&o->oi.data[0], compressbuffer, o->oi.length, cb->data, cb_org->data, modifiedblocks);

        END("mergediffs");

        if((cb->bits & CB_EMPTY)!=0) {
          bits=OI_EMPTY;
        }

        BEGIN();

        errorcode=addoperation(cb->blckno, compressbuffer, length, bits);

        END("addoperation");
      }
    }
    else {
      errorcode=ERROR_DISK_FULL;
    }

  }

  unlockcachebuffer(cb);

  return(errorcode);
}

#endif


void dumpcachebuffer(struct CacheBuffer *cb) {
  /* Any changes made to this cachebuffer will not be stored, and this
     cachebuffer will be emptied. */

  unlockcachebuffer(cb);
  restorecachebuffer(cb);
}



static void dumpcachebuffers3(void) {
  struct CacheBuffer *cb;

  cb=(struct CacheBuffer *)globals->cblrulist.mlh_Head;

  while(cb->node.mln_Succ!=0) {
    checkcb(cb,"dump/getcachebuffer");
    cb=(struct CacheBuffer *)(cb->node.mln_Succ);
  }
}



void killunlockedcachebuffers(void) {
  struct CacheBuffer *cb;

  cb=(struct CacheBuffer *)globals->cblrulist.mlh_Head;

  while(cb->node.mln_Succ!=0) {

    if(!(
        (cb->locked>0) ||
        ((((cb->bits & (CB_ORIGINAL|CB_LATEST)))==CB_ORIGINAL) &&
        (findlatestcachebuffer(cb->blckno)!=0))
        ))
    {
      clearcachebuffer(cb);
      resetcachebuffer(cb);
    }

    cb=(struct CacheBuffer *)(cb->node.mln_Succ);
  }
}


struct CacheBuffer *getcachebuffer() {
  struct CacheBuffer *cb;
  LONG buffers=globals->totalbuffers;

  /* It's absolutely essential that getcachebuffer always uses the
     LEAST recently used cachebuffer which isn't currently used in
     an operation.  The reason for this is not only because this is
     a good algorithm to ensure the cache contains blocks which are
     the most often used, but also because a lot of functions -rely-
     on the fact that a buffer which has been used (read!) recently
     remains in the cache for a while longer(!).

     Because this process of 'relying' on a recently read buffer to
     still be in cache is a bit tricky business, we recently added
     locking functions.  These functions (lockcachebuffer() &
     unlockcachebuffer() can prevent this function from returning
     the cachebuffer in question for re-use.  Only this function is
     effected by this locking process.  Always make sure the cache
     buffer is unlocked again! */

  /* a CacheBuffer can be reused if the following criteria are met:

      - The CacheBuffer is not locked, and

      - If the CacheBuffer is CB_ORIGINAL, but not CB_LATEST
        then it must not have a corresponding CB_LATEST
        CacheBuffer.  In other words, originals don't get
        reused if there is a later version of the same block
        still in cache. */

//  killunlockedcachebuffers();

  do {
//    cb=(struct CacheBuffer *)globals->cblrulist.mlh_Head;
//    mrucachebuffer(cb);

    /* weissms: changed to trick gcc-4.4.4 optimizer */
    cb=(struct CacheBuffer *)RemHead((struct List*)&globals->cblrulist);
    addtailm(&globals->cblrulist,&cb->node);

  } while((cb->locked>0 || ((cb->bits & (CB_ORIGINAL|CB_LATEST))==CB_ORIGINAL && findlatestcachebuffer(cb->blckno)!=0)) && buffers-->0);

  if(buffers<=0) {
    _XDEBUG((DEBUG_CACHEBUFFER,"getcachebuffer: No more cachebuffers available!\n"));
    dumpcachebuffers();

    req_unusual("SFS has ran out of cache buffers.");

    return(0);
  }

  resetcachebuffer(cb);  /* emptycachebuffer also adds cachebuffer at top of LRU list... we don't want that. */

  return(cb);
}


void clearcachebuffer(struct CacheBuffer *cb) {
  ULONG blocksize=globals->bytes_block>>4;
  ULONG *block=cb->data;

  checkcb(cb,"clearcachebuffer");

  while(blocksize--!=0) {
    *block++=0;
    *block++=0;
    *block++=0;
    *block++=0;
  }
}



LONG writecachebuffer(struct CacheBuffer *cb) {

  checkcb(cb,"writecachebuffer");

  return(write(cb->blckno,cb->data,1));
}



void dumpcachebuffers(void) {
  struct CacheBuffer *cb;

  cb=(struct CacheBuffer *)globals->cblrulist.mlh_Head;

  _DEBUG(("Blck-- Lock Bits Data---- ID------ cb-adr-- Hashed?\n"));
  while(cb->node.mln_Succ!=0) {
    _DEBUG(("%6ld %4ld %4ld %08lx %08lx %08lx ",cb->blckno,(LONG)cb->locked,(LONG)cb->bits,cb->data,*(ULONG *)cb->data,cb));
    if(cb->hashnode.mln_Succ==0 && cb->hashnode.mln_Pred==0) {
      _DEBUG(("No\n"));
    }
    else {
      _DEBUG(("Yes\n"));
    }

    cb=(struct CacheBuffer *)(cb->node.mln_Succ);
  }
}



static void dumpcachebuffers2(void) {
  struct CacheBuffer *cb;
  ULONG cnt=0;

  cb=(struct CacheBuffer *)globals->cblrulist.mlh_Head;

  while(cb->node.mln_Succ!=0) {
    cnt++;
    cb=(struct CacheBuffer *)(cb->node.mln_Succ);
  }

  if(cnt!=globals->totalbuffers) {
    _DEBUG(("------------ cachebuffers have been killed!! ---------------\n"));
    dumpcachebuffers();
  }
}




LONG addcachebuffers(LONG buffers) {
  struct CacheBuffer *cb;
  LONG newbuffers;
  LONG counter=0;
  LONG errorcode=0;

  newbuffers=globals->totalbuffers+buffers;

  if(newbuffers<MINCACHESIZE) {
    newbuffers=MINCACHESIZE;
  }

  buffers=newbuffers-globals->totalbuffers;
  /* if buffers is positive than add 'buffers' buffers, else free some */

  if(buffers<0) {
    if((errorcode=flushtransaction())!=0) {
      return(errorcode);
    }
    invalidatecachebuffers();
  }

  if(buffers>0) {
    _DEBUG(("Allocating buffers\n"));

    while(buffers!=0 && (cb=AllocMem(globals->bytes_block+sizeof(struct CacheBuffer),MEMF_CLEAR|globals->bufmemtype))!=0) {
      _DEBUG(("*"));
      counter++;
      addtailm(&globals->cblrulist,&cb->node);
      buffers--;

      cb->data=&cb->attached_data[0];
      cb->id=0x4A48;
    }
    _DEBUG((" end\n"));

    if(buffers!=0) {
      _DEBUG(("Allocation failed!\n"));

      buffers=-counter;      /* This makes sure that the already allocated buffers are freed again */
      newbuffers=globals->totalbuffers;
      errorcode=ERROR_NO_FREE_STORE;
    }
  }

  if(buffers<0) {
    while(buffers++!=0) {
      cb=(struct CacheBuffer *)globals->cblrulist.mlh_TailPred;
      RemTail((struct List *)&globals->cblrulist);
      resetcachebuffer(cb);
      FreeMem(cb,sizeof(struct CacheBuffer)+globals->bytes_block);
    }
  }

  globals->totalbuffers=newbuffers;

  return(errorcode);
}



void invalidatecachebuffers() {
  struct CacheBuffer *cb;

  for(cb=(struct CacheBuffer *)globals->cblrulist.mlh_Head; cb!=(struct CacheBuffer *)&globals->cblrulist.mlh_Tail; cb=(struct CacheBuffer *)cb->node.mln_Succ) {
    resetcachebuffer(cb);
  }
}



/*

CacheBuffer types:
------------------

CB_ORIGINAL - Indicates that the CacheBuffer is a direct
copy of the one stored on disk.

CB_LATEST - Indicates that this CacheBuffer is the most
recent version of the block.  It also means that this
CacheBuffer can be locked.

If neither of these flags is set then the CacheBuffer is
considered unused.

For every CacheBuffer with CB_LATEST set, there must be a
CacheBuffer (for the same block) with CB_ORIGINAL set (this
could be the same CacheBuffer).

A CacheBuffer can be reused if the following criteria are
met:

 - The CacheBuffer is not locked, and

 - If the CacheBuffer is CB_ORIGINAL, but not CB_LATEST
   then it must not have a corresponding CB_LATEST
   CacheBuffer.  In other words, originals don't get
   reused if there is a later version of the same block
   still in cache.


readcachebuffer()

Returns a cachebuffer with the latest version of a block
(CB_LATEST set).  The original cachebuffer is read if not
already present.

applyoperation()

Requires a cachebuffer and if needed an original version of
that block.

readoriginalcachebuffer()

Returns a cachebuffer with the original version of a block.

*/
