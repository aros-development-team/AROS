#include <clib/macros.h>
#include <devices/scsidisk.h>
#include <devices/trackdisk.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>
#include <exec/errors.h>
#include <exec/io.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <string.h>
#include "stdio.h"

#include "/fs/deviceio.h"
#include "/fs/deviceio_protos.h"
#include "/fs/cachedio_protos.h"

#include "/fs/fs.h"

#include "/bitfuncs.c"

#define BLCKFACCURACY   (5)                          /* 2^5 = 32 */


/* ASM prototypes */

extern ULONG __asm RANDOM(register __d0 ULONG);
extern LONG __asm STACKSWAP(void);
extern ULONG __asm CALCCHECKSUM(register __d0 ULONG,register __a0 ULONG *);
extern ULONG __asm MULU64(register __d0 ULONG,register __d1 ULONG,register __a0 ULONG *);
extern WORD __asm COMPRESSFROMZERO(register __a0 UWORD *,register __a1 UBYTE *,register __d0 ULONG);


static const char version[]={"\0$VER: SFSdumpblock 1.0 " __AMIGADATE__ "\r\n"};

LONG read2(ULONG block);
BOOL checkchecksum(void *buffer);
void setchecksum(void *buffer);
void dumpblock(ULONG block, void *data, ULONG bytes);
ULONG calculatechecksum(void *buffer);

struct DosEnvec *dosenvec;

/* blocks_  = the number of blocks of something (blocks can contain 1 or more sectors)
   block_   = a block number of something (relative to start of partition)
   sectors_ = the number of sectors of something (1 or more sectors form a logical block)
   sector_  = a sector number (relative to the start of the disk)
   bytes_   = the number of bytes of something
   byte_    = a byte number of something
   shifts_  = the number of bytes written as 2^x of something
   mask_    = a mask for something */

extern ULONG blocks_total;               /* size of the partition in blocks */

extern ULONG byte_low;                   /* the byte offset of our partition on the disk */
extern ULONG byte_lowh;                  /* high 32 bits */
extern ULONG byte_high;                  /* the byte offset of the end of our partition (excluding) on the disk */
extern ULONG byte_highh;                 /* high 32 bits */

extern ULONG bytes_block;                /* size of a block in bytes */

extern UWORD shifts_block;               /* shift count needed to convert a blockoffset<->byteoffset */

extern ULONG bufmemtype;

UBYTE *buffer;

LONG main() {
  struct RDArgs *readarg;
  UBYTE template[]="DEVICE=DRIVE/A,BLOCK/N/A,BINARY/S\n";

  struct {char *device;
          ULONG *block;
          ULONG binary;} arglist={NULL};

  if((DOSBase=(struct DosLibrary *)OpenLibrary("dos.library",39))!=0) {
    if((IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39))!=0) {
      if((readarg=ReadArgs(template,(LONG *)&arglist,0))!=0) {
        struct DosList *dl;
        UBYTE *devname=arglist.device;

        while(*devname!=0) {
          if(*devname==':') {
            *devname=0;
            break;
          }
          devname++;
        }

        dl=LockDosList(LDF_DEVICES|LDF_READ);
        if((dl=FindDosEntry(dl,arglist.device,LDF_DEVICES))!=0) {
          struct FileSysStartupMsg *fssm;
          struct MsgPort *msgport;

          fssm=(struct FileSysStartupMsg *)BADDR(dl->dol_misc.dol_handler.dol_Startup);
          dosenvec=(struct DosEnvec *)BADDR(fssm->fssm_Environ);
          msgport=dl->dol_Task;

          UnLockDosList(LDF_DEVICES|LDF_READ);

          if((initcachedio((UBYTE *)BADDR(fssm->fssm_Device)+1, fssm->fssm_Unit, fssm->fssm_Flags, dosenvec))==0) {

            // setiocache(arglist.lines!=0 ? *arglist.lines : 128, arglist.readaheadsize!=0 ? *arglist.readaheadsize : 8192, FALSE);     /* 1 MB for read-ahead cache, no copyback mode. */

            if((buffer=AllocVec(bytes_block, bufmemtype))!=0) {

              /** *********************************************
               * Here starts the code:
               */

              read2(*arglist.block);

              if(arglist.binary!=0) {
                Write(Output(), buffer, bytes_block);
              }
              else {
                if(checkchecksum(buffer)!=DOSTRUE) {
                  printf("Warning: Checksum of block %ld is wrong.  It is 0x%08lx while it should be 0x%08lx.\n", *arglist.block, ((struct fsBlockHeader *)buffer)->checksum, calculatechecksum(buffer));
                }
                if(((struct fsBlockHeader *)buffer)->ownblock != *arglist.block) {
                  printf("Warning: Block %ld indicates that it is actually block %ld (ownblock is wrong!)\n", *arglist.block, ((struct fsBlockHeader *)buffer)->ownblock);
                }
                dumpblock(*arglist.block,buffer,bytes_block);
              }

              /**
               * Here ends the code.
               * ********************************************/

              FreeVec(buffer);
            }
            else {
              printf("Not enough memory\n");
            }

            cleanupcachedio();
          }
        }
        else {
          VPrintf("Unknown device %s\n",&arglist.device);
          UnLockDosList(LDF_DEVICES|LDF_READ);
        }

        FreeArgs(readarg);
      }
      CloseLibrary((struct Library *)IntuitionBase);
    }
    CloseLibrary((struct Library *)DOSBase);
  }
  return(0);
}



LONG read2(ULONG block) {
//  return(transfer(DIO_READ, buffer, block, 1));
  return(read(block, buffer, 1));
}



/*
            else if(SetSignal(0L,SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C) {
              PutStr("\n***Break\n");
              break;
            }
*/



void dumpblock(ULONG block, void *data, ULONG bytes) {
  ULONG *d=(ULONG *)data;
  UBYTE *d2=(UBYTE *)data;
  UWORD off=0;
  UBYTE s[40];

  if(bytes<bytes_block) {
    printf("Dump of first %ld bytes of block %ld.\n",bytes,block);
  }
  else {
    printf("Dump of block %ld.\n",block);
  }

  while(bytes>0) {
    WORD n;
    UBYTE c;
    UBYTE *s2;

    n=16;
    s2=s;

    while(--n>=0) {
      c=*d2++;

      if(c<32) {
        c+=64;
      }
      if(c>=127 && c<=160) {
        c='.';
      }

      *s2++=c;
    }
    *s2=0;

    printf("0x%04lx: %08lx %08lx %08lx %08lx %s\n",off,d[0],d[1],d[2],d[3],s);

    bytes-=16;
    d+=4;
    off+=16;
  }
}



LONG req(UBYTE *fmt, UBYTE *gads, ... ) {
  ULONG args[5];
  ULONG *arg=args;
  UBYTE *fmt2;
  LONG gadget=0;

  /* Simple requester function which is called by deviceio.o
     for displaying low-level device errors and accesses outside
     the partition. */

  *arg=(ULONG)fmt;

  if((fmt2=AllocVec(strlen(fmt)+100,0))!=0) {

    RawDoFmt("%s",args,(void (*)())"\x16\xC0\x4E\x75",fmt2);

    {
      struct EasyStruct es;
      ULONG *args=(ULONG *)&gads;

      args++;

      es.es_StructSize=sizeof(struct EasyStruct);
      es.es_Flags=0;
      es.es_Title="SFScheck request";
      es.es_TextFormat=fmt2;
      es.es_GadgetFormat=gads;

      gadget=EasyRequestArgs(0,&es,0,args);
    }

    FreeVec(fmt2);
  }

  return(gadget);
}



void starttimeout(void) {
  /* Called by deviceio.o each time there is a physical
     disk access.  You can use this to start a timer and
     call motoroff() when the disk hasn't been accessed
     for a specific amount of time (SFS uses 1 second).

     SFScheck doesn't use this function. */
}



BOOL checkchecksum(void *buffer) {
  if(CALCCHECKSUM(bytes_block, buffer)==0) {
    return(DOSTRUE);
  }
  return(DOSFALSE);
}


ULONG calculatechecksum(void *buffer) {
  ULONG oldchecksum,checksum;

  struct fsBlockHeader *bh=(struct fsBlockHeader *)buffer;

  oldchecksum=bh->checksum;
  bh->checksum=0;    /* Important! */
  checksum=-CALCCHECKSUM(bytes_block, (ULONG *)buffer);
  bh->checksum=oldchecksum;
  return checksum;
}


void setchecksum(void *buffer) {
  struct fsBlockHeader *bh=(struct fsBlockHeader *)buffer;

  bh->checksum=0;    /* Important! */
  bh->checksum=-CALCCHECKSUM(bytes_block, (ULONG *)buffer);
}
