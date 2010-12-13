#include <dos/dos.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "/fs/blockstructure.h"

/* ASM prototypes */

extern ULONG __asm RANDOM(register __d0 ULONG);
extern LONG __asm STACKSWAP(void);
extern ULONG __asm CALCCHECKSUM(register __d0 ULONG,register __a0 ULONG *);
extern ULONG __asm MULU64(register __d0 ULONG,register __d1 ULONG,register __a0 ULONG *);
extern WORD __asm COMPRESSFROMZERO(register __a0 UWORD *,register __a1 UBYTE *,register __d0 ULONG);

void setchecksum(void *cb);

static const char version[]={"\0$VER: SFSchecksum 1.0 " __AMIGADATE__ "\r\n"};

ULONG bytes_block;                /* size of a block in bytes */

LONG main() {
  struct RDArgs *readarg;
  UBYTE template[]="FILE/A,BLOCKSIZE/N/A,BLOCK/N\n";

  struct {char *file;
          ULONG *blocksize;
          ULONG *blockno;} arglist={NULL};

  if((DOSBase=(struct DosLibrary *)OpenLibrary("dos.library",39))!=0) {
    if((readarg=ReadArgs(template,(LONG *)&arglist,0))!=0) {
      UBYTE *buffer;
      ULONG blockno=0;

      if(arglist.blockno!=0) {
        blockno=*arglist.blockno;
      }

      bytes_block=*arglist.blocksize;

      if((buffer=AllocVec(*arglist.blocksize, MEMF_PUBLIC))!=0) {
        BPTR fh;

        if((fh=Open(arglist.file, MODE_OLDFILE))!=0) {
          if((Seek(fh, *arglist.blocksize * blockno, OFFSET_BEGINNING))==0) {
            ULONG old;

            if(Read(fh, buffer, *arglist.blocksize)==*arglist.blocksize) {
              old=*((ULONG *)(buffer+4));

              setchecksum(buffer);

              VPrintf("Block is of type 0x%08lx;", buffer);
              VPrintf(" ownblock is 0x%08lx\n", buffer + 8);
              VPrintf("Checksum for this block was 0x%08lx",&old);
              VPrintf(" but should be 0x%08lx\n",buffer+4);
            }
            else {
              PutStr("End of file reached.\n");
            }
          }
          else {
            PrintFault(IoErr(), "error while seeking in file");
          }

          Close(fh);
        }
        else {
          PrintFault(IoErr(), "error while opening file");
        }

        FreeVec(buffer);
      }
      else {
        PutStr("Out of memory\n");
      }

      FreeArgs(readarg);
    }
    else {
      PutStr("wrong arguments\n");
    }

    CloseLibrary((struct Library *)DOSBase);
  }
  return(0);
}



void setchecksum(void *cb) {
  struct fsBlockHeader *bh=cb;

  bh->checksum=0;    /* Important! */
  bh->checksum=-CALCCHECKSUM(bytes_block,cb);
}


BOOL checkchecksum(void *d) {
  ULONG *data=(ULONG *)d;

  if(CALCCHECKSUM(bytes_block,data)==0) {
    return(TRUE);
  }
  return(FALSE);
}
