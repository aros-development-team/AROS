#include <dos/dos.h>
#include <exec/types.h>

#include "adminspaces.h"

#include "bitmap_protos.h"
#include "cachebuffers_protos.h"
#include "debug.h"
#include "fs.h"
#include "transactions_protos.h"
#include "asmsupport.h"

#include "globals.h"

extern LONG readcachebuffercheck(struct CacheBuffer **,ULONG,ULONG);
extern LONG req(UBYTE *fmt, UBYTE *gads, ... );

LONG allocadminspace(struct CacheBuffer **returned_cb) {
  struct CacheBuffer *cb;
  BLCK adminspaceblock=globals->block_adminspace;
  LONG errorcode;

  /* Returns a new, cleared CacheBuffer with blckno correctly
     initialised.  This function can only be called from
     within an operation.  *returned_cb is not destroyed
     when there was an error. */

  while((errorcode=readcachebuffercheck(&cb,adminspaceblock,ADMINSPACECONTAINER_ID))==0) {
    struct fsAdminSpaceContainer *asc=cb->data;
    struct fsAdminSpace *as=asc->adminspace;
    LONG adminspaces=(globals->bytes_block-sizeof(struct fsAdminSpaceContainer))/sizeof(struct fsAdminSpace);

    while(adminspaces-->0) {
      WORD bitoffset;

      if(as->be_space!=0 && (bitoffset=bfffz(BE2L(as->be_bits),0)) < 32) {
        BLCK emptyadminblock=BE2L(as->be_space)+bitoffset;

        preparecachebuffer(cb);

        as->be_bits|=L2BE(1<<(31-bitoffset));

        if((errorcode=storecachebuffer(cb))!=0) {
          return(errorcode);
        }

        if((cb=newcachebuffer(emptyadminblock))==0) {
          return(ERROR_NO_FREE_STORE);
        }

        *returned_cb=cb;

        return(0);
      }
      as++;
    }

    if((adminspaceblock=BE2L(asc->be_next))==0) {
      ULONG startblock;

      /* If we get here it means current adminspace areas are all filled.
         We would now need to find a new area and create a fsAdminSpace
         structure in one of the AdminSpaceContainer blocks.  If these
         don't have any room left for new adminspace areas a new
         AdminSpaceContainer would have to be created first which is
         placed as the first block in the newly found admin area. */

      adminspaceblock=globals->block_adminspace;

      if((errorcode=findandmarkspace(32,&startblock))==0) {
        while((errorcode=readcachebuffercheck(&cb,adminspaceblock,ADMINSPACECONTAINER_ID))==0) {
          struct fsAdminSpaceContainer *asc=cb->data;
          struct fsAdminSpace *as=asc->adminspace;
          LONG adminspaces=(globals->bytes_block-sizeof(struct fsAdminSpaceContainer))/sizeof(struct fsAdminSpace);

          while(adminspaces-->0 && as->be_space!=0) {
            as++;
          }

          if(adminspaces>=0) {
            /* Found a unused AdminSpace in this AdminSpaceContainer! */

            preparecachebuffer(cb);

            as->be_space=L2BE(startblock);
            as->be_bits=0;

            errorcode=storecachebuffer(cb);
            break;
          }

          if(BE2L(asc->be_next)==0) {

            /* Oh-oh... we marked our new adminspace area in use, but we couldn't
               find space to store a fsAdminSpace structure in the existing
               fsAdminSpaceContainer blocks.  This means we need to create and
               link a new fsAdminSpaceContainer as the first block in our newly
               marked adminspace. */

            preparecachebuffer(cb);

            asc->be_next=L2BE(startblock);

            if((errorcode=storecachebuffer(cb))!=0) {
              return(errorcode);
            }

            /* Now preparing new AdminSpaceContainer */

            if((cb=newcachebuffer(startblock))==0) {
              return(ERROR_NO_FREE_STORE);
            }

            asc=cb->data;

            asc->bheader.id=ADMINSPACECONTAINER_ID;
            asc->bheader.be_ownblock=L2BE(startblock);
            asc->be_previous=L2BE(adminspaceblock);

            asc->adminspace[0].be_space=L2BE(startblock);
            asc->adminspace[0].be_bits=L2BE(0x80000000);
            asc->bits=32;

            if((errorcode=storecachebuffer(cb))!=0) {
              return(errorcode);
            }

            adminspaceblock=startblock;
            break;   /* Breaks through to outer loop! */
          }

          adminspaceblock=BE2L(asc->be_next);
        }
      }

      if(errorcode!=0) {
        return(errorcode);
      }
    }
  }

  return(errorcode);
}



LONG freeadminspace(BLCK block) {
  struct CacheBuffer *cb;
  BLCK adminspaceblock=globals->block_adminspace;
  LONG errorcode;

  _DEBUG(("freeadminspace: Entry -- freeing block %ld\n",block));

  while((errorcode=readcachebuffercheck(&cb,adminspaceblock,ADMINSPACECONTAINER_ID))==0) {
    struct fsAdminSpaceContainer *asc=cb->data;
    struct fsAdminSpace *as=asc->adminspace;
    LONG adminspaces=(globals->bytes_block-sizeof(struct fsAdminSpaceContainer))/sizeof(struct fsAdminSpace);

    while(adminspaces-->0) {
      if(block>=BE2L(as->be_space) && block<BE2L(as->be_space)+32) {
        WORD bitoffset=block-BE2L(as->be_space);

        /* block to be freed has been located */

        _DEBUG(("freeadminspace: Block to be freed is located in AdminSpaceContainer block at %ld\n",adminspaceblock));

        preparecachebuffer(cb);

        as->be_bits&=~(L2BE(1<<(31-bitoffset)));

        if((errorcode=storecachebuffer(cb))==0) {
          errorcode=addfreeoperation(block);
        }

        /*** Extra code need to be added here to free the AdminSpace structure if it isn't
             being used anymore.  This could mean that the entire AdminSpaceBlock could no
             longer be needed either.... */

        return(errorcode);
      }

      as++;
    }

    if((adminspaceblock=BE2L(asc->be_next))==0) {
      break;
    }
  }

  if(errorcode==0) {
    /*** Strange.  We have been looking for the block to free, but couldn't find it in any
         of the AdminSpace containers... */

    req("Unable to free an administration block.\nThe block cannot be found.", "Ok");

    return(ERROR_OBJECT_NOT_FOUND);
  }

  return(errorcode);
}
