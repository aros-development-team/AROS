#include <devices/scsidisk.h>
#include <devices/trackdisk.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>
#include <exec/errors.h>
#include <exec/io.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <clib-org/exec_protos.h>
#include "stdio.h"


/* Almost ALL functions in this source are support functions
   which were quickly copied from the main SFS source.  The
   most interesting function is saveextents(), which performs
   the actual function of this program. */


#include "/fs/bitmap.h"
#include "/fs/objects.h"
#include "/fs/deviceio.h"
#include "/fs/fs.h"
#include "/fs/btreenodes.h"

#include <devices/newstyle.h>      /* Doesn't include exec/types.h which is why it is placed here */

#include "/bitfuncs.c"

#define BLCKFACCURACY   (5)                          /* 2^5 = 32 */



/* ASM prototypes */

extern ULONG __asm RANDOM(register __d0 ULONG);
extern LONG __asm STACKSWAP(void);
extern ULONG __asm CALCCHECKSUM(register __d0 ULONG,register __a0 ULONG *);
extern ULONG __asm MULU64(register __d0 ULONG,register __d1 ULONG,register __a0 ULONG *);
extern WORD __asm COMPRESSFROMZERO(register __a0 UWORD *,register __a1 UBYTE *,register __d0 ULONG);



LONG safedoio(UWORD action,UBYTE *buffer,ULONG blocks,ULONG blockoffset);
void display(void);

struct DosEnvec *dosenvec;

/* blocks_  = the number of blocks of something (blocks can contain 1 or more sectors)
   block_   = a block number of something (relative to start of partition)
   sectors_ = the number of sectors of something (1 or more sectors form a logical block)
   sector_  = a sector number (relative to the start of the disk)
   bytes_   = the number of bytes of something
   byte_    = a byte number of something
   shifts_  = the number of bytes written as 2^x of something
   mask_    = a mask for something */

ULONG sectors_total;              /* size of the partition in sectors */
UWORD sectors_block;              /* number of sectors in a block */
UWORD pad4;

ULONG blocks_total;               /* size of the partition in blocks */
ULONG blocks_reserved_start;      /* number of blocks reserved at start (=reserved) */
ULONG blocks_reserved_end;        /* number of blocks reserved at end (=prealloc) */
ULONG blocks_used;                /* number of blocks in use excluding reserved blocks
                                     (=numblockused) */
ULONG blocks_bitmap;              /* number of BTMP blocks for this partition */
ULONG blocks_inbitmap;            /* number of blocks a single bitmap block can contain info on */
ULONG blocks_maxtransfer=1048576; /* max. blocks which may be transfered to the device at once (limits io_Length) */

ULONG block_root;                 /* the block offset of the root block */
ULONG block_extentbnoderoot;      /* the block offset of the root of the extent bnode tree */

ULONG byte_low;                   /* the byte offset of our partition on the disk */
ULONG byte_lowh;                  /* high 32 bits */
ULONG byte_high;                  /* the byte offset of the end of our partition (excluding) on the disk */
ULONG byte_highh;                 /* high 32 bits */

ULONG bytes_block;                /* size of a block in bytes */
ULONG bytes_sector;               /* size of a sector in bytes */

ULONG mask_block;
ULONG mask_block32;               /* masks the least significant bits of a BLCKf pointer */
ULONG mask_mask=0xFFFFFFFF;       /* mask as specified by mountlist */

UWORD shifts_block;               /* shift count needed to convert a blockoffset<->byteoffset */
UWORD shifts_block32;             /* shift count needed to convert a blockoffset<->32byteoffset (only used by nodes.c!) */

ULONG disktype;                   /* the type of media inserted (same as id_DiskType in InfoData
                                     structure) */
UBYTE newstyledevice=FALSE;
UBYTE does64bit=FALSE;
UBYTE is_casesensitive=FALSE;
UBYTE pad1;
UWORD cmdread=CMD_READ;
UWORD cmdwrite=CMD_WRITE;


struct MsgPort *msgport;
struct IOStdReq *ioreq;
void *pool;

ULONG block_objectnodesbase;
ULONG block_extentroot;

UBYTE *buffer;
ULONG *bitmap;

ULONG returncode=0;

LONG main() {
  struct RDArgs *readarg;
  UBYTE template[]="DEVICE=DRIVE/A\n";

  struct {char *device;} arglist={NULL};

  if((DOSBase=(struct DosLibrary *)OpenLibrary("dos.library",37))!=0) {
    if((pool=CreatePool(0,16384,8192))!=0) {
      if((readarg=ReadArgs(template,(LONG *)&arglist,0))!=0) {
        struct MsgPort *msgport;
        struct DeviceNode *dn;
        UBYTE *devname=arglist.device;

        while(*devname!=0) {
          if(*devname==':') {
            *devname=0;
            break;
          }
          devname++;
        }

        dn=(struct DeviceNode *)LockDosList(LDF_DEVICES|LDF_READ);
        if((dn=(struct DeviceNode *)FindDosEntry((struct DosList *)dn,arglist.device,LDF_DEVICES))!=0) {
          struct FileSysStartupMsg *fssm;

          fssm=(struct FileSysStartupMsg *)BADDR(dn->dn_Startup);
          dosenvec=(struct DosEnvec *)BADDR(fssm->fssm_Environ);

          UnLockDosList(LDF_DEVICES|LDF_READ);

          if((msgport=CreateMsgPort())!=0) {
            if((ioreq=CreateIORequest(msgport,sizeof(struct IOStdReq)))!=0) {
              if(OpenDevice((UBYTE *)BADDR(fssm->fssm_Device)+1,fssm->fssm_Unit,(struct IORequest *)ioreq,fssm->fssm_Flags)==0) {
                {
                  struct DosEnvec *de=dosenvec;
                  UWORD bs;
                  ULONG sectorspercilinder;

                  bytes_sector=de->de_SizeBlock<<2;
                  bytes_block=bytes_sector*de->de_SectorPerBlock;

                  bs=bytes_block;

                  shifts_block=0;
                  while((bs>>=1)!=0) {
                    shifts_block++;
                  }
                  shifts_block32=shifts_block-BLCKFACCURACY;

                  mask_block32=(1<<shifts_block32)-1;
                  mask_block=bytes_block-1;

                  /* Absolute offset on the entire disk are expressed in Sectors;
                     Offset relative to the start of the partition are expressed in Blocks */

                  sectorspercilinder=de->de_Surfaces*de->de_BlocksPerTrack;            /* 32 bit */
                  sectors_total=sectorspercilinder*(de->de_HighCyl+1-de->de_LowCyl);   /* 32 bit */

                  /* sectorspercilinder * bytes_sector cannot exceed 32-bit !! */

                  byte_lowh  = MULU64(sectorspercilinder * bytes_sector, de->de_LowCyl,    &byte_low);
                  byte_highh = MULU64(sectorspercilinder * bytes_sector, de->de_HighCyl+1, &byte_high);

                  sectors_block=de->de_SectorPerBlock;

                  blocks_total=sectors_total/de->de_SectorPerBlock;
                  blocks_reserved_start=de->de_Reserved;
                  blocks_reserved_end=de->de_PreAlloc;

                  if(blocks_reserved_start<1) {
                    blocks_reserved_start=1;
                  }

                  if(blocks_reserved_end<1) {
                    blocks_reserved_end=1;
                  }

                  blocks_inbitmap=(bytes_block-sizeof(struct fsBitmap))<<3;  /* must be a multiple of 32 !! */
                  blocks_bitmap=(blocks_total+blocks_inbitmap-1)/blocks_inbitmap;

                  blocks_used=0;

                  printf("Partition start offset : 0x%08lx:%08lx   End offset  : 0x%08lx:%08lx\n",byte_lowh,byte_low,byte_highh,byte_high);
                  printf("sectors/block : %ld\n",de->de_SectorPerBlock);
                  printf("bytes/sector  : %ld\n",bytes_sector);
                  printf("bytes/block   : %ld\n",bytes_block);
                  printf("Total blocks in partition : %ld\n",blocks_total);
                }



                /* If the partition's high byte is beyond the 4 GB border we will
                   try and detect a 64-bit device. */

                if(byte_highh!=0) {
                  struct NSDeviceQueryResult nsdqr;
                  UWORD *cmdcheck;
                  LONG errorcode;

                  /* Checks for newstyle devices and 64-bit support */

                  nsdqr.SizeAvailable=0;
                  nsdqr.DevQueryFormat=0;

                  ioreq->io_Command=NSCMD_DEVICEQUERY;
                  ioreq->io_Length=sizeof(nsdqr);
                  ioreq->io_Data=(APTR)&nsdqr;

                  if(DoIO((struct IORequest *)ioreq)==0 && (ioreq->io_Actual >= 16) && (nsdqr.SizeAvailable == ioreq->io_Actual) && (nsdqr.DeviceType == NSDEVTYPE_TRACKDISK)) {
                    /* This must be a new style trackdisk device */

                    newstyledevice=TRUE;

                    printf("Device is a new style device (NSD)\n");

                    /* Is it safe to use 64 bits with this driver?  We can reject
                       bad mounts pretty easily via this check. */

                    for(cmdcheck=nsdqr.SupportedCommands; *cmdcheck; cmdcheck++) {
                      if(*cmdcheck == NSCMD_TD_READ64) {
                        /* This trackdisk style device supports the complete 64-bit
                           command set without returning IOERR_NOCMD! */

                        printf("Device supports 64-bit commands\n");

                        does64bit=TRUE;
                        cmdread=NSCMD_TD_READ64;
                        cmdwrite=NSCMD_TD_WRITE64;
                      }
                    }
                  }
                  else {
                    /* Checks for TD64 supporting devices */

                    ioreq->io_Command=24;  /* READ64 */
                    ioreq->io_Length=0;
                    ioreq->io_Actual=0;
                    ioreq->io_Offset=0;
                    ioreq->io_Data=0;

                    errorcode=DoIO((struct IORequest *)ioreq);
                    if(errorcode!=-1 && errorcode!=IOERR_NOCMD) {
                      printf("Device supports 64-bit commands\n");

                      does64bit=TRUE;
                      cmdread=24;
                      cmdwrite=25;
                    }
                  }
                }

                if((buffer=AllocVec(bytes_block*2,0))!=0) {
                  display();
                }
                else {
                  printf("Not enough memory\n");
                }

                CloseDevice((struct IORequest *)ioreq);
              }
              DeleteIORequest(ioreq);
            }
            DeleteMsgPort(msgport);
          }
        }
        else {
          VPrintf("Unknown device %s\n",&arglist.device);
          UnLockDosList(LDF_DEVICES|LDF_READ);
        }

        FreeArgs(readarg);
      }
      DeletePool(pool);
    }
    CloseLibrary((struct Library *)DOSBase);
  }
  return(returncode);
}



/*
            else if(SetSignal(0L,SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C) {
              PutStr("\n***Break\n");
              break;
            }
*/


BOOL validblock(ULONG block) {
  if(block<blocks_total) {
    return(TRUE);
  }
  return(FALSE);
}




LONG doio(void) {
  return(DoIO((struct IORequest *)ioreq));
}




LONG safedoio(UWORD action,UBYTE *buffer,ULONG blocks,ULONG blockoffset) {
  ULONG command;
  ULONG start,starthigh;
  ULONG end,endhigh;
  LONG errorcode;

  if(action==DIO_READ) {
    command=cmdread;
  }
  else if(action==DIO_WRITE) {
    command=cmdwrite;
  }
  else {
    return(-1);
  }


  start=(blockoffset<<shifts_block)+byte_low;
  starthigh=(blockoffset>>(32-shifts_block))+byte_lowh;            /* High offset */
  if(start < byte_low) {
    starthigh+=1;     /* Add X bit :-) */
  }

  end=start+(blocks<<shifts_block);
  endhigh=starthigh;
  if(end < start) {
    endhigh+=1;       /* Add X bit */
  }

  ioreq->io_Data=buffer;
  ioreq->io_Command=command;
  ioreq->io_Length=blocks<<shifts_block;
  ioreq->io_Offset=start;
  ioreq->io_Actual=starthigh;

  if(  ( starthigh > byte_lowh || (starthigh == byte_lowh && start >= byte_low) ) &&
       ( endhigh < byte_highh || (endhigh == byte_highh && end <= byte_high) )  ) {

    /* We're about to do a physical disk access.  (Re)set timeout.  Since
       the drive's motor will be turned off with the timeout as well we
       always (re)set the timeout even if doio() would return an error. */

    if((errorcode=doio())!=0) {
      return(errorcode);
    }
  }
  else {
//    DEBUG(("byte_low = 0x%08lx:%08lx, byte_high = 0x%08lx:%08lx\n",byte_lowh,byte_low,byte_highh,byte_high));
//    DEBUG(("start = 0x%08lx:%08lx, end = 0x%08lx:%08lx\n",starthigh,start,endhigh,end));

    return(INTERR_OUTSIDE_PARTITION);
  }

  return(0);
}




BOOL checkchecksum(void *d) {
  ULONG *data=(ULONG *)d;

  if(CALCCHECKSUM(bytes_block,data)==0) {
    return(TRUE);
  }
  return(FALSE);
}



struct fsObject *nextobject(struct fsObject *o) {
  UBYTE *p;

  /* skips the passed in fsObject and gives a pointer back to the place where
     a next fsObject structure could be located */

  p=(UBYTE *)&o->name[0];

  /* skip the filename */
  while(*p++!=0) {
  }

  /* skip the comment */
  while(*p++!=0) {
  }

  /* ensure WORD boundary */
  if((((ULONG)p) & 0x01)!=0) {
    p++;
  }

  return((struct fsObject *)p);
}



WORD isobject(struct fsObject *o, struct fsObjectContainer *oc) {
  UBYTE *endadr;

  endadr=(UBYTE *)oc+bytes_block-sizeof(struct fsObject)-2;

  if((UBYTE *)o<endadr && o->name[0]!=0) {
    return(TRUE);
  }
  return(FALSE);
}



LONG read(ULONG block) {
  return(safedoio(DIO_READ, buffer, 1, block));
}



struct BNode *searchforbnode(ULONG key, struct BTreeContainer *tc) {
  struct BNode *tn;
  WORD n=tc->nodecount-1;

  tn=(struct BNode *)((UBYTE *)tc->bnode+n*tc->nodesize);

  for(;;) {
    if(n<=0 || key >= tn->key) {
      return(tn);
    }
    tn=(struct BNode *)((UBYTE *)tn-tc->nodesize);
    n--;
  }
}



LONG findbnode(BLCK rootblock,ULONG key,struct BNode **returned_bnode) {
  LONG errorcode;

  while((errorcode=read(rootblock))==0) {
    struct fsBNodeContainer *bnc=(struct fsBNodeContainer *)buffer;
    struct BTreeContainer *btc=&bnc->btc;

    if(btc->nodecount==0) {
      *returned_bnode=0;
      break;
    }

    *returned_bnode=searchforbnode(key,btc);
    if(btc->isleaf==TRUE) {
      break;
    }
    rootblock=(*returned_bnode)->data;
  }

  return(errorcode);
}



void display(void) {
  struct fsExtentBNode *ebn;
  LONG n=blocks_bitmap;
  ULONG bitmapblock=34;
  BLCK block=0;
  LONG errorcode=0;

  while(n-->0 && (errorcode=read(bitmapblock))==0) {
    struct fsBitmap *b=(struct fsBitmap *)(buffer+bytes_block);

    CopyMemQuick(buffer, b, bytes_block);

    for(;;) {
      if(block>=blocks_total) {
        break;
      }

      if((block / blocks_inbitmap)!=bitmapblock-34) {
        break;
      }

      if((errorcode=findbnode(6, block, (struct BNode **)&ebn))!=0) {
        printf("Error %ld occured during scanning (findbnode).\n",errorcode);

        return;
      }

      if(ebn==0 || ebn->key!=block) {
        ULONG offset=(block % blocks_inbitmap);

        if((b->bitmap[offset/32] & (1<<(31-(offset & 0x1F))))==0) {
          printf("u");
        }
        else {
          printf(".");
        }
        block++;
      }
      else {
        UWORD blocks=ebn->blocks;

        if(blocks==1) {
          printf("H");
        }
        else {
          if((ebn->prev & 0x80000000)==0) {
            printf("<");
          }
          else {
            printf("[");
          }

          blocks-=2;

          while(blocks-->0) {
            printf("o");
          }

          if(ebn->next==0) {
            printf("]");
          }
          else {
            printf(">");
          }
        }

        block+=ebn->blocks;
      }
    }

    bitmapblock++;
  }

  printf("\n");

  if(errorcode!=0) {
    printf("Error %ld occured during scanning.\n",errorcode);
  }
}
