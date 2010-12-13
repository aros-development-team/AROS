#include "asmsupport.h"

//#include <clib/alib_protos.h>
#include <devices/trackdisk.h>
#include <exec/types.h>
#include <proto/exec.h>

#include "transactions.h"
#include "transactions_protos.h"

#include "bitmap_protos.h"
#include "cachebuffers_protos.h"
#include "debug.h"
#include "cachedio_protos.h"
#include "support_protos.h"

#include "globals.h"

extern LONG req(UBYTE *fmt, UBYTE *gads, ... );
extern LONG req_unusual(UBYTE *fmt, ... );
extern void dreq(UBYTE *fmt, ... );
extern void outputcachebuffer(struct CacheBuffer *cb);

extern void setchecksum(struct CacheBuffer *);
extern BOOL checkchecksum(struct CacheBuffer *);
extern LONG readcachebuffercheck(struct CacheBuffer **,ULONG,ULONG);
extern void starttimeout(void);
extern void stoptimeout(void);

/* Internal prototypes */

void linkoperation(struct Operation *o);
LONG removetransactionfailure(void);
LONG writeoperations(void);
void restorecachebuffer(struct CacheBuffer *cb);

/* Operation   : A single block modification
   Transaction : A series of block modifications which together result in a valid disk */

/* Definitions for Red Black tree */

#define ROOT     globals->operationroot       /* The name of the root variable */
#define RBNODE   struct Operation    /* The name of the structure containing the left,
                                        right, parent, nodecolor & data fields. */
#define SENTINEL globals->operationsentinel   /* The name of the sentinel node variable (must be a RBNODE) */
#define DATA     oi.blckno           /* The name of the data field (example: data, sub.mydata) */

#define CompLT(a,b) (a < b)      /* modify these lines to establish data type */
#define CompEQ(a,b) (a == b)

#ifdef CHECKCODE_TRANSACTIONS
void cop(struct Operation *o, UBYTE *where) {
  if(o->magicvalue!=0x4a4f) {
    req_unusual("magicvalue of operation is incorrect.\nAddress = 0x%08lx (location: %s)\n", o, where);
  }
}
#endif

#include "redblacktree.c"

#define linkoperation(x)   InsertNode(x)

/* linkoperation() and removeoperation() are the functions which add and remove
   operations to a list or tree of somekind.  They can be easily changed to
   support different forms of lists. */

void removeoperation(struct Operation *o) {
  #ifdef CHECKCODE_TRANSACTIONS
  cop(o, "removeoperation");
  #endif
  DeleteNode(o);

  globals->transactionpoolsize-=o->oi.length+sizeof(struct Operation);
  FreePooled(globals->transactionpool,o,o->oi.length+sizeof(struct Operation));
}

void removecurrentoperation2(BLCK blck, struct Operation *o) {
  if((o=FindNodeFrom(o,blck))!=0) {
    if(o->new==globals->transactionnestcount) {
      removeoperation(o);
    }
    else {
      removecurrentoperation2(blck, o->left);
      removecurrentoperation2(blck, o->right);
    }
  }
}



void removecurrentoperation(BLCK blck) {
  /* This function removes the operation for a specific block for the current
     transaction.  Operations for this block in older transactions won't be
     affected. */

  removecurrentoperation2(blck, ROOT);
}



LONG inittransactions() {
  ULONG poolsize;

  InitRedBlackTree();
  #ifdef CHECKCODE_TRANSACTIONS
  operationsentinel.magicvalue=0x4a4f;
  #endif

  poolsize=globals->bytes_block*4;
  if(poolsize<16384) {
    poolsize=16384;
  }

  if((globals->compressbuffer=AllocMem(globals->bytes_block+(globals->bytes_block>>4),1))!=0) {
    if((globals->transactionpool=CreatePool(0,poolsize,poolsize>>1))!=0) {
      return(0);
    }
  }

  cleanuptransactions();

  return(ERROR_NO_FREE_STORE);
}



void cleanuptransactions() {
  if(globals->transactionpool!=0) {
    DeletePool(globals->transactionpool);
  }

  if(globals->compressbuffer!=0) {
    FreeMem(globals->compressbuffer,globals->bytes_block+(globals->bytes_block>>4));
  }
}



static LONG fillwithoperations(struct fsTransactionStorage *ts,struct Operation **o,UBYTE **src,LONG *length) {
  LONG bytesleft=globals->bytes_block-sizeof(struct fsTransactionStorage);
  LONG copylength;
  UBYTE *dest=ts->data;

  while(bytesleft>0) {
    if(*length==0) {
      if(*o!=0) {
        *src=(UBYTE *)&(*o)->oi;
        *length=((*o)->oi.length | 1)+sizeof(struct OperationInformation)-1;

        *o=NextNode(*o);
      }
      else {
        return(TRUE);
      }
    }

    copylength=*length;
    if(copylength>bytesleft) {
      copylength=bytesleft;
    }

    CopyMem(*src,dest,copylength);

    *src+=copylength;
    *length-=copylength;

    dest+=copylength;
    bytesleft-=copylength;
  }

  if(*length==0 && *o==0) {
    return(TRUE);
  }

  return(FALSE);
}




LONG savetransaction(BLCK *firsttransactionblock) {
  struct CacheBuffer *cb;
  ULONG blckno;
  LONG errorcode;

  _XDEBUG((DEBUG_TRANSACTION,"savetransaction: Entry.  Transaction size = %ld\n", globals->transactionpoolsize));

  *firsttransactionblock=0;

  if((cb=getcachebuffer())!=0) {
    struct Operation *o=FirstNode();
    struct fsTransactionStorage *ts=cb->data;
    UBYTE *src;
    LONG length=0;
    LONG done;

    lockcachebuffer(cb);

    if((errorcode=findspace(1, globals->block_rovingblockptr, globals->block_rovingblockptr, &blckno))==0) {
      ULONG startblock;

      _XDEBUG((DEBUG_TRANSACTION,"savetransaction: findspace succesfully completed\n"));

      /* Warning, this function doesn't mark space, so it assumes that
         findspace never returns the same space if startblock is set
         to blckno+1 each time. */

      do {

        _XDEBUG((DEBUG_TRANSACTION,"savetransaction: Storing operations of transaction in block %ld\n",blckno));

        startblock=blckno+1;

        clearcachebuffer(cb);

        if(*firsttransactionblock==0) {
          *firsttransactionblock=blckno;
        }
        cb->blckno=blckno;

        ts->bheader.id=TRANSACTIONSTORAGE_ID;
        ts->bheader.be_ownblock=L2BE(blckno);

        done=fillwithoperations(ts,&o,&src,&length);

        if(done==FALSE) {
          if(startblock==globals->block_rovingblockptr) {
            errorcode=ERROR_DISK_FULL;
            break;
          }

          if((errorcode=findspace(1, startblock, globals->block_rovingblockptr, &blckno))!=0) {
            break;
          }

          ts->be_next=L2BE(blckno);
        }

        setchecksum(cb);
        if((errorcode=writecachebuffer(cb))!=0) {
          break;
        }

      } while(done==FALSE);
    }

    unlockcachebuffer(cb);
  }
  else {
    errorcode=ERROR_NO_FREE_STORE;
  }

  _XDEBUG((DEBUG_TRANSACTION,"savetransaction: Exiting with errorcode = %ld\n",errorcode));

  return(errorcode);
}



/* Read TransactionFailure block and process the transaction list it points to. */

LONG loadtransaction(BLCK block) {
  struct CacheBuffer *cb;
  struct Operation *o=0;
  UBYTE *src;
  UBYTE *dest=0;
  UWORD length=0;
  UWORD bytesleft;
  LONG errorcode=0;

  while(block!=0 && (errorcode=readcachebuffercheck(&cb,block,TRANSACTIONSTORAGE_ID))==0) {
    struct fsTransactionStorage *ts=cb->data;

    block=BE2L(ts->be_next);

    bytesleft=globals->bytes_block-sizeof(struct fsTransactionStorage);
    src=ts->data;

    while(bytesleft!=0) {
      if(length!=0) {
        UWORD copylength=length;

        if(copylength>bytesleft) {
          copylength=bytesleft;
        }

        CopyMem(src,dest,copylength);

        src+=copylength;
        dest+=copylength;
        length-=copylength;
        bytesleft-=copylength;
      }
      else {
        UWORD len=*((UWORD *)src);

        if(o!=0) {
          linkoperation(o);
        }

        if(len==0) {
          return(0);
        }

        length=(len | 1)+sizeof(struct OperationInformation)-1;

        if((o=AllocPooled(globals->transactionpool,len+sizeof(struct Operation)))==0) {
          return(ERROR_NO_FREE_STORE);
        }
        globals->transactionpoolsize+=len+sizeof(struct Operation);

        dest=(UBYTE *)&o->oi;
      }
    }
  }

  if(errorcode==0 && o!=0) {
    linkoperation(o);
  }

  return(errorcode);
}




LONG checkfortransaction(void) {
  struct CacheBuffer *cb;
  LONG errorcode;

  if((errorcode=readcachebuffer(&cb,globals->block_root+2))==0) {
    struct fsBlockHeader *bh=cb->data;

    if(checkchecksum(cb)==DOSTRUE && bh->be_ownblock==L2BE(globals->block_root+2)) {
      if(bh->id==TRANSACTIONFAILURE_ID) {
        struct fsTransactionFailure *tf=cb->data;

        /* A transaction failed to complete.. we'll need to re-do it. */

        req("Has an unfinished transaction which\nwill be loaded now.", "Ok");

        if((errorcode=loadtransaction(BE2L(tf->be_firsttransaction)))==0) {

          /* Succesfully loaded the Transaction buffer of the Transaction which
             failed to complete before. */

          req("The transaction loaded succesfully and\nit will now be re-applied to this volume.", "Ok");

          _DEBUG(("Before writeoperations()\n"));

          if((errorcode=writeoperations())==0) {

/*
            request(PROGRAMNAME " request","%s\n"\
                                           "Debug requester 1\n",
                                           "Ok",((UBYTE *)BADDR(devnode->dn_Name))+1);
*/
            _DEBUG(("Before removetransactionfailure()\n"));
            if((errorcode=removetransactionfailure())==0) {

              req("The transaction has been succesfully\napplied to this volume.\nYou can continue normally now.", "Ok");
            }
          }
        }

        if(errorcode!=0) {
          req("The transaction couldn't be re-applied\nto the volume because of error %ld.\n\nPlease reboot your system to try again.", "Ok");
        }
      }
    }
    else {
      return(INTERR_CHECKSUM_FAILURE);
    }
  }

  return(errorcode);
}




static void combineoperations(void) {
  struct Operation *o;

  /* Combine operations will combine the operations of the current
     transaction with those of the previous transaction level.  The
     newer operations are kept, and any old operations operating on
     the same block are discarded.  The newer operations are given
     a transaction level one lower than their current level.

     Operations with OI_DELETE will be removed, including a possible
     existing operation for the same block. */

  if((o=FirstNode())!=0) {
    struct Operation *oold=0;
    struct Operation *onew=0;
    struct Operation *onext;
    BLCK block;
    UBYTE tnc=globals->transactionnestcount-1;

    #ifdef CHECKCODE_TRANSACTIONS
    cop(o, "combineoperations 1");
    #endif

    for(;;) {
      onext=NextNode(o);
      block=o->oi.blckno;

      if(o->new>=tnc) {
        if(o->new==tnc) {
          oold=o;
        }
        else {
          onew=o;
          onew->new=tnc;
        }

        if(oold!=0 && onew!=0) {
          #ifdef CHECKCODE_TRANSACTIONS
          cop(oold, "combineoperations 2");
          cop(onew, "combineoperations 3");
          #endif

          if((onew->oi.bits & OI_DELETE)!=0) {
            struct CacheBuffer *cb;

            /* The new operation indicates that the operations on this block
               can be discarded because the block has been deleted.  In this
               case, we not only delete the old operation, but the new one
               as well.

               Also, because there will be no operations for this block
               anymore a possibly present CB_ORIGINAL cachebuffer might need
               to be turned into a CB_ORIGIGNAL|CB_LATEST cachebuffer.

               Also, all CB_LATEST cachebuffers, except original ones, must
               be terminated. */

            if((cb=findlatestcachebuffer(onew->oi.blckno))!=0) {
              emptycachebuffer(cb);
            }

            if((cb=findoriginalcachebuffer(onew->oi.blckno))!=0) {
              cb->bits|=CB_LATEST;
            }

            removeoperation(onew);
          }

          removeoperation(oold);
          oold=0;
          onew=0;
        }
      }

      if((o=onext)==0) {
        break;
      }

      if(block!=onext->oi.blckno) {
        oold=0;
        onew=0;
      }
    }
  }
}



#ifdef NOCOMPRESSION

LONG addoperation2(struct CacheBuffer *cb_org, struct CacheBuffer *cb_new) {
  struct Operation *o;

  // _DEBUG(("addoperation: Adding a new operation for block %ld\n",block));

  /* First remove all Operations with the same block in this Transaction */

  removecurrentoperation(cb_new->blckno);

  /* Add our new Operation to the Transaction */

  if((o=AllocPooled(transactionpool,bytes_block+sizeof(struct Operation)))!=0) {
    UBYTE bits=0;

    transactionpoolsize+=bytes_block+sizeof(struct Operation);

    if(cb_org!=0) {
      compress(cb_org->data,cb_new->data,o->oi.data);
    }
    else {
      compressfromzero(cb_new->data,o->oi.data);
    }

    if((cb_new->bits & CB_EMPTY)!=0) {
      bits=OI_EMPTY;
    }

    o->oi.blckno=cb_new->blckno;
    o->oi.length=bytes_block;
    o->oi.bits=bits;
    o->new=transactionnestcount;

    linkoperation(o);

    return(0);
  }

  return(ERROR_NO_FREE_STORE);
}

#endif


LONG addoperation(BLCK block,UBYTE *data,UWORD length,UBYTE bits) {
  struct Operation *o;

  /* First remove all Operations with the same block in this Transaction */

  removecurrentoperation(block);

  /* Add our new Operation to the Transaction */

  if((o=AllocPooled(globals->transactionpool, length+sizeof(struct Operation)))!=0) {
    globals->transactionpoolsize+=length+sizeof(struct Operation);

    o->oi.blckno=block;
    o->oi.length=length;
    o->oi.bits=bits;
    #ifdef CHECKCODE_TRANSACTIONS
    o->magicvalue=0x4A4F;
    #endif
    o->new=globals->transactionnestcount;

    CopyMem(data,o->oi.data,length);

    linkoperation(o);

    return(0);
  }

  return(ERROR_NO_FREE_STORE);
}



LONG addfreeoperation(BLCK block) {
  struct Operation *o;

  /* Adds an operation for the indicated block which indicates that
     this block is no longer used, and thus when this transaction is
     merged with the previous ones all operations for this block can
     be removed.  Any operations already present in this transaction
     level which deal with the same block are discarded. */

  removecurrentoperation(block);

  if((o=AllocPooled(globals->transactionpool, sizeof(struct Operation)))!=0) {
    globals->transactionpoolsize+=sizeof(struct Operation);

    o->oi.blckno=block;
    o->oi.length=0;
    o->oi.bits=OI_DELETE;
    #ifdef CHECKCODE_TRANSACTIONS
    o->magicvalue=0x4A4F;
    #endif
    o->new=globals->transactionnestcount;

    linkoperation(o);

    return(0);
  }

  return(ERROR_NO_FREE_STORE);
}



WORD isthereanoperationfor(BLCK block) {
  if(FindNode(block)==0) {
    return(FALSE);
  }

  return(TRUE);
}



void newtransaction(void) {
  /* Begins a new transaction. */

  if(globals->transactionnestcount!=0) {
    req_unusual("Transaction nest count wasn't zero!");
  }

  globals->transactionnestcount++;

  _XDEBUG((DEBUG_TRANSACTION,"--NEW-----> poolsize = %ld\n",globals->transactionpoolsize));
}



void endtransaction(void) {

  /* Call this to indicate that the entire transaction was succesfull.

     CB_ORIGINAL -> leave alone.
     CB_ORIGINAL|CB_LATEST -> leave alone.
     CB_LATEST -> leave alone. */

  _XDEBUG((DEBUG_TRANSACTION,"--END----->\n"));

  combineoperations();

  if(--globals->transactionnestcount==0) {  /* Important!  Combineoperations() must be called before lowering transactionnestcount */

    /* Actions below only need to be done when transactionnestcount is zero */

    /* We'll have to indicate that a flush is needed
       and (re)set the timeout. */

    starttimeout();

    /* If the transaction buffer becomes a specific size, we might want
       to enforce a flush. */

    if(globals->transactionpoolsize>MAX_TRANSACTIONPOOLSIZE) {
      _TDEBUG(("endtransaction: poolsize larger than 32 kB -> flushed transaction\n"));
      flushtransaction();
    }
  }
}



void deletetransaction(void) {
  struct Operation *o;
  struct Operation *oprev;

  /* Deletes everything in the transaction buffer which belongs to the
     current transaction level.  All CacheBuffers which are locked will
     be re-read to ensure they are restored to what they were before
     newtransaction() was called.

     All CacheBuffers which had an operation on them in the to be deleted
     transaction will be restored to what they were at the time of
     newtransaction().

     CB_ORIGINAL -> if there is no latest operation for this cb then turn into
                    CB_ORIGINAL|CB_LATEST.

     CB_ORIGINAL|CB_LATEST -> leave alone.

     CB_LATEST -> if there is an operation in this transaction for this Cachebuffer,
                  then reload with latest version.  It doesn't matter if it is still
                  locked, because it is possible that there is no latest version which
                  means the block must become the original version again:
                  CB_LATEST -> CB_LATEST|CB_ORIGINAL.

     CB_EMPTY|CB_LATEST -> if there is an operation in this transaction for this
                           Cachebuffer then restore to latest version, if present.
                           Otherwise clear it. */

  _DEBUG(("-DEL------>\n"));

  o=FirstNode();

  while(o!=0) {
    oprev=o;
    o=NextNode(o);

    if(oprev->new==globals->transactionnestcount) {
      struct CacheBuffer *cb;
      BLCK block=oprev->oi.blckno;
      UBYTE bits=oprev->oi.bits;

      removeoperation(oprev);   // Must be placed before restorecachebuffer() call below.

      if((bits & OI_DELETE)==0) {          // Apr 3 1999: Added this if.
        if((cb=findlatestcachebuffer(block))!=0) {
          if((cb->bits & CB_ORIGINAL)!=0) {
            dreq("deletetransaction: Fatal error - there is an operation for a cachebuffer which has not been modified!\nPlease notify the author!");
            outputcachebuffer(cb);
          }

          /* Found a CB_LATEST cachebuffer which must be restored. */

          restorecachebuffer(cb);
        }
        else if((cb=findoriginalcachebuffer(block))!=0) {
          if((cb->bits & CB_LATEST)!=0) {
            dreq("deletetransaction: Fatal error - there is an operation for a cachebuffer which hasn't been modified!\nPlease notify the author!");
            outputcachebuffer(cb);
          }

          /* Found a CB_ORIGINAL cachebuffer which must be marked LATEST if there are
             no operations for this cachebuffer anymore. */

          if(isthereanoperationfor(cb->blckno)==FALSE) {
            cb->bits|=CB_LATEST;
          }
        }
      }
    }
  }

  globals->transactionnestcount--;
}



LONG flusherror(LONG errorcode) {
  if(errorcode==TDERR_WriteProt) {
    return(req("Updating the volume failed because it is write protected!\n"\
               "Please disable the write protection and press 'Retry'.\n"\
               "Failing to do so may result in loss of data and may\n"\
               "confuse programs which have just written to this disk.\n",
               "Retry|Cancel"));
  }
  else if(errorcode==TDERR_DiskChanged) {
    return(req("Updating the volume failed because no disk is inserted!\n"\
               "Please insert the disk and press 'Retry'.\n"\
               "Failing to do so may result in loss of data and may\n"\
               "confuse programs which have just written to this disk.\n",
               "Retry|Cancel"));
  }
  else {
    ULONG freeblocks=-1;

    getfreeblocks(&freeblocks);

    return(req("Updating the volume failed due to an unknown error.\n"\
               "The errorcode is %ld (free blocks = %ld).\n"\
               "Please try and fix the problem and press 'Retry'.\n"\
               "Failing to do so may result in loss of data and may\n"\
               "confuse programs which have just written to this disk.\n",
               "Retry|Cancel", errorcode, freeblocks));
  }

  return(0);
}



LONG writeoperations(void) {
  struct CacheBuffer *cb;
  struct Operation *o;
  struct Operation *oprev;
  LONG errorcode=0;

  o=FirstNode();

  while(o!=0) {
    /* Luckily just 'reading' the most recent version of the cachebuffer
       is enough to get the most recent version of the block we're looking
       for.  Writing it to disk is all we need to do. */

/*
    _DEBUG(("writeoperations: Writing block %ld\n",o->oi.blckno));
            request(PROGRAMNAME " request","%s\n"\
                                           "Writing block %ld\n",
                                           "Ok",((UBYTE *)BADDR(devnode->dn_Name))+1, o->oi.blckno);
*/

    while((errorcode=readcachebuffer(&cb,o->oi.blckno))!=0 && flusherror(errorcode)==1) {
    }
    if(errorcode!=0) {
/*
            request(PROGRAMNAME " request","%s\n"\
                                           "Debug requester 3 (errorcode = %ld)\n",
                                           "Ok",((UBYTE *)BADDR(devnode->dn_Name))+1, errorcode);
*/
      break;
    }

    while((errorcode=writecachebuffer(cb))!=0 && flusherror(errorcode)==1) {
    }
    if(errorcode!=0) {
/*
            request(PROGRAMNAME " request","%s\n"\
                                           "Debug requester 4 (errorcode = %ld)\n",
                                           "Ok",((UBYTE *)BADDR(devnode->dn_Name))+1, errorcode);
*/
      break;
    }

    emptyoriginalcachebuffer(cb->blckno);
    cb->bits&=~CB_EMPTY;
    cb->bits|=CB_ORIGINAL|CB_LATEST;

    oprev=o;
    o=NextNode(o);
    removeoperation(oprev);
  }

  #ifdef CHECKCODE
    {
      struct CacheBuffer *cb;

      cb=(struct CacheBuffer *)globals->cblrulist.mlh_Head;

      while(cb->node.mln_Succ!=0) {
        if((cb->bits & (CB_ORIGINAL|CB_LATEST))==CB_ORIGINAL) {
          dreq("writeoperations: Fatal error!\nPlease notify the author!\n");
          outputcachebuffer(cb);
        }

        cb=(struct CacheBuffer *)(cb->node.mln_Succ);
      }
    }
  #endif

  return(errorcode);
}



LONG removetransactionfailure(void) {
  struct CacheBuffer *cb;
  LONG errorcode;

  while((errorcode=flushiocache())!=0 && flusherror(errorcode)==1) {     /* This commits any dirty data. */
  }

  if(errorcode==0) {
    if((cb=getcachebuffer())!=0) {
      struct fsBlockHeader *bh=cb->data;

      clearcachebuffer(cb);

      bh->id=TRANSACTIONOK_ID;
      bh->be_ownblock=L2BE(globals->block_root+2);

      cb->blckno=globals->block_root+2;

/*
            request(PROGRAMNAME " request","%s\n"\
                                           "Debug requester 2\n",
                                           "Ok",((UBYTE *)BADDR(devnode->dn_Name))+1);
*/

      setchecksum(cb);
      while((errorcode=writecachebuffer(cb))!=0 && flusherror(errorcode)==1) {
      }

      if(errorcode==0) {
        while((errorcode=flushiocache())!=0 && flusherror(errorcode)==1) {     /* This commits any dirty data. */
        }
      }
    }
    else {
      errorcode=ERROR_NO_FREE_STORE;
    }
  }

  return(errorcode);
}



BOOL hastransaction(void) {

  /* This function returns TRUE if there is an uncommited transaction. */

  return((BOOL)(ROOT!=&SENTINEL));
}



LONG flushtransaction(void) {
  struct CacheBuffer *cb;
  BLCK firsttransactionblock;
  LONG errorcode;

  _XDEBUG((DEBUG_TRANSACTION,"flushtransaction: Entry\n"));

  while((errorcode=flushiocache())!=0 && flusherror(errorcode)==1) {     /* This commits any dirty data. */
  }

  if(errorcode==0 && hastransaction()) {

    _DEBUG(("flushtransaction: There is a transaction\n"));

    while((errorcode=savetransaction(&firsttransactionblock))!=0 && flusherror(errorcode)==1) {
    }

    if(errorcode==0) {

      _XDEBUG((DEBUG_TRANSACTION,"flushtransaction: Saved transaction\n"));

      while((errorcode=flushiocache())!=0 && flusherror(errorcode)==1) {     /* This commits any dirty data. */
      }

      _XDEBUG((DEBUG_TRANSACTION,"flushtransaction: Disk updated\n"));

      if(errorcode==0) {
        if((cb=getcachebuffer())!=0) {
          struct fsTransactionFailure *tf=cb->data;

          clearcachebuffer(cb);

          tf->bheader.id=TRANSACTIONFAILURE_ID;
          tf->bheader.be_ownblock=L2BE(globals->block_root+2);

          tf->be_firsttransaction=L2BE(firsttransactionblock);

          cb->blckno=globals->block_root+2;

          setchecksum(cb);
          while((errorcode=writecachebuffer(cb))!=0 && flusherror(errorcode)==1) {
          }

          _XDEBUG((DEBUG_TRANSACTION,"flushtransaction: Set TransactionFailure block\n"));

          if(errorcode==0) {

            while((errorcode=flushiocache())!=0 && flusherror(errorcode)==1) {     /* This commits any dirty data. */
            }

            _XDEBUG((DEBUG_TRANSACTION,"flushtransaction: Disk updated (2)\n"));

            if(errorcode==0) {
              if((errorcode=writeoperations())==0) {           /* writeoperations() */

                _XDEBUG((DEBUG_TRANSACTION,"flushtransaction: Updated all blocks\n"));

                if((errorcode=removetransactionfailure())==0) {
                  stoptimeout();
                }
              }
            }
          }
        }
        else {
          errorcode=ERROR_NO_FREE_STORE;
        }
      }
    }
  }

  if(errorcode!=0) {
    /* Okay, we've warned the user quite a few times -- now it's too late. */

    req("Unable to flush all buffers correctly (errorcode = %ld).\n"\
        "A reboot is the recommended course of action to prevent\n"\
        "any (further) loss of data.\n", "Ok", errorcode);

    /**** More action needs to be taken here!  Maybe even a retry option. */

  }

  _DEBUG(("flushtransaction: Done.  errorcode = %ld\n",errorcode));

  return(errorcode);
}




struct Operation *getlatestoperation2(BLCK block, struct Operation *o) {

  if((o=FindNodeFrom(o,block))!=0) {
    struct Operation *o2;
    struct Operation *o3;

    o2=getlatestoperation2(block, o->left);
    o3=getlatestoperation2(block, o->right);

    if(o2!=0 && o2->new > o->new) {
      o=o2;
    }

    if(o3!=0 && o3->new > o->new) {
      o=o3;
    }
  }

  return(o);
}



struct Operation *getlatestoperation(BLCK block) {
  struct Operation *o;

  /* Returns the latest operation (not necessarily the current operation)
     for a block.  Zero indicates there were no operations for this block. */

  _XDEBUG((DEBUG_CACHEBUFFER,"getlatestoperation: Entry for block %ld\n",block));

  o=getlatestoperation2(block, ROOT);

  _XDEBUG((DEBUG_CACHEBUFFER,"getlatestoperation: Exit\n"));

  return(o);
}



void restorecachebuffer(struct CacheBuffer *cb) {
  struct Operation *o;

  /* Restore the CacheBuffer to its latest version if present.
     If there is no latest version then clear it if it is of type
     CB_EMPTY, or make it the original if not. */

  /* Be careful when using this function when you're walking
     the list of CacheBuffers.  emptycachebuffer() can be called
     in this function, which means you must make sure you already
     know what the next cachebuffer is going to be before calling
     this function. */

  #ifdef CHECKCODE
    if((cb->bits & (CB_ORIGINAL|CB_LATEST))!=CB_LATEST) {
      dreq("restorecachebuffer: Fatal error - cachebuffer is of wrong type!\nPlease notify the author!");
      outputcachebuffer(cb);
    }
  #endif

  _XDEBUG((DEBUG_CACHEBUFFER,"restorecachebuffer: Entry.  cb->blckno = %ld, cb->bits = %ld\n",cb->blckno, (ULONG)cb->bits));

  if((o=getlatestoperation(cb->blckno))!=0 && ((o->oi.bits & OI_EMPTY)!=0 || (cb->bits & CB_EMPTY)==0)) {
    if((o->oi.bits & OI_EMPTY)==0) {
      struct CacheBuffer *cb_org;

      if((cb_org=findoriginalcachebuffer(cb->blckno))==0) {
        dreq("restorecachebuffer: Fatal error - couldn't find original cachebuffer!\nPlease notify the author!");
        outputcachebuffer(cb);
      }

      CopyMemQuick(cb_org->data, cb->data, globals->bytes_block);
    }
    uncompress(cb->data, o->oi.data, o->oi.length);
  }
  else if((cb->bits & CB_EMPTY)!=0) {
    clearcachebuffer(cb);
  }
  else {
    struct CacheBuffer *cb_org;

    if((cb_org=findoriginalcachebuffer(cb->blckno))==0) {
      dreq("restorecachebuffer: Fatal error - can't find original cachebuffer!\nPlease notify the author!");
      outputcachebuffer(cb);
    }

    CopyMemQuick(cb_org->data, cb->data, globals->bytes_block);
    resetcachebuffer(cb_org);

    cb->bits|=CB_ORIGINAL|CB_LATEST;
  }
}



#if 0
void restorecachebuffer(struct CacheBuffer *cb) {
  struct Operation *o;

  /* cb is converted to the latest version of the cachebuffer or if
     there's no latest, to the original version. */

  if((o=getlatestoperation(cb->blckno))!=0) {
    if((o->oi.bits & OI_EMPTY)==0) {
      struct CacheBuffer *cb_org;
      LONG errorcode;

      if((cb_org=findoriginalcachebuffer(cb->blckno))==0) {
        dreq("restorecachebuffer: Fatal error - couldn't find original cachebuffer!\nPlease notify the author!");
      }

      CopyMemQuick(cb_org->data,cb->data,bytes_block);
    }
    uncompress(cb->data,o->oi.data,o->oi.length);

    cb->bits&=~(CB_ORIGINAL);
    cb->bits|=CB_LATEST;
  }
  else if((cb->bits & CB_EMPTY)!=0) {
    clearcachebuffer(cb);
  }
  else {
    struct CacheBuffer *cb_org;

    if((cb_org=findoriginalcachebuffer(cb->blckno))==0) {
      dreq("restorecachebuffer: Fatal error - couldn't find original cachebuffer!\nPlease notify the author!");
    }

    CopyMemQuick(cb_org->data,cb->data,bytes_block);
    emptycachebuffer(cb_org);

    cb->bits&=~(CB_LATEST);
    cb->bits|=CB_ORIGINAL;
  }
}
#endif




LONG applyoperation(BLCK block, struct CacheBuffer **returned_cb) {
  struct Operation *o;

  /* This function either creates the latest version of a block (by
     reading the original block if needed) or, if there is no latest
     version, returns the original block itself.

     If no error occurs, then returned_cb is filled with a CacheBuffer
     holding the latest version of a block (undefined otherwise). */

  if((o=getlatestoperation(block))!=0) {
    struct CacheBuffer *cb;

    globals->statistics.cache_operationdecode++;

    _XDEBUG((DEBUG_CACHEBUFFER,"applyoperation: Entry. block = %ld -> found an operation!\n",block));

    if((o->oi.bits & OI_EMPTY)!=0) {
      globals->statistics.cache_emptyoperationdecode++;

      cb=createnewcachebuffer(block);
    }
    else {
      LONG errorcode;

      if((errorcode=readoriginalcachebuffer(&cb, block))!=0) {
        return(errorcode);
      }

      saveoriginalcachebuffer(cb);
    }
    uncompress(cb->data, o->oi.data, o->oi.length);

    *returned_cb=cb;
  }
  else {
    LONG errorcode;

    _XDEBUG((DEBUG_CACHEBUFFER,"applyoperation: Entry. block = %ld -> returning original!\n",block));

    if((errorcode=readoriginalcachebuffer(returned_cb, block))!=0) {
      return(errorcode);
    }
  }

  return(0);
}



ULONG transactionspace(void) {
  UWORD space=(globals->bytes_block-sizeof(struct fsTransactionStorage));

  /* Returns the space needed in blocks which would be needed to
     store all transaction information. */

  /* (X + 512 + 8 + 496 - 1) / 496 = 2.04 */

//  return((transactionpoolsize+bytes_block+(bytes_block>>6)+space-1)/space);

  /* (X + 496-1) / 496 = 0 */

  return((globals->transactionpoolsize + space-1)/space);
}


/*

things to do after an endtransaction()
--------------------------------------
CB_ORIGINAL -> leave alone.
CB_ORIGINAL|CB_LATEST -> leave alone.
CB_LATEST -> leave alone.
CB_EMPTY|CB_LATEST -> leave alone.
+ clear prepared bit for this transaction level.

things to do after an deletetransaction()
-----------------------------------------
CB_ORIGINAL -> if there is no latest operation for this cb then turn into CB_ORIGINAL|CB_LATEST.
CB_ORIGINAL|CB_LATEST -> leave alone.
CB_LATEST -> if prepared in this transaction then reload with latest version.
CB_EMPTY|CB_LATEST -> if prepared in this transaction then restore to latest
                      version, if present.  Otherwise clear it.
+ clear prepared bit for this transaction level.


things to do after an flushtransaction()
----------------------------------------
CB_ORIGINAL -> empty.
CB_ORIGINAL|CB_LATEST -> leave alone.
CB_LATEST -> turn into CB_ORIGINAL|CB_LATEST.
CB_EMPTY|CB_LATEST -> turn into CB_ORIGINAL|CB_LATEST.
(all prepared bits should be cleared at this point).

Rules
-----

prepare-, store-, dump- and newcachebuffer may only be
called during a transaction.

dumpcachebuffer or storecachebuffer MUST be called for every
time a preparecachebuffer or newcachebuffer was called.
Also you must make sure that all this occurs in the same
transaction level.

lockcachebuffer & unlockcachebuffer may be called at any
time.  The only rule is that you must call unlockcachebuffer
for every lockcachebuffer.


Operations
----------

There are basicly 3 types of operations:

1) Modification of existing blocks (bits = 0).
2) Creation of a new block (bits = OI_EMPTY)
3) Deletion of a existing block (bits = OI_DELETED)

EndTransaction:

Operations of type 1&2 are simply merged with existing
operations in previous transactions.  Operations of type 3
indicate that all previous operations on that same block are
no longer required and can be deleted.

DeleteTransaction:

All operations of the current transaction nest level are
simply removed.


CacheBuffer rules
-----------------

There rules are in effect at all times, and when
adding/deleting cachebuffers or operations you must make
sure that these rules don't break.

 - There is at maximum one CB_ORIGINAL cachebuffer of any
   block.

 - There is at maximum one CB_LATEST cachebuffer of any block.

 - CB_ORIGINAL and CB_LATEST can be set for the same buffer.
   In that case the block was never modified and is exactly as
   it is (and will be) on disk.  Is it now allowed that there
   are operations for such a block.

 - If there is a cachebuffer with only CB_LATEST set, then the
   respective cachebuffer with CB_ORIGINAL set MUST be present
   in the cache as well.

 - If there is a cachebuffer with either CB_ORIGINAL or
   CB_LATEST set (not both!), then there MUST be an operation
   present for that block.  The presence of an operation for a
   block however does not mean a cachebuffer of that block has
   to be present.  Note that the presence of a cachebuffer with
   only CB_LATEST set means there must be one with CB_ORIGINAL
   set as well (see the other rules).

There rules have a very important purpose.  For one thing
they ensure that certain key operations can never fail.  For
example, when a diff is needed between a original and latest
version of a cachebuffer, then the original MUST be present
in the cache.  If we would have to read the original first,
that would be a potential source of errors (read error, out
of memory error) which is undesireable at some critical
points.

*/
