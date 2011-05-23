#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <utility/tagitem.h>
#include <stdio.h>

#include "../FS/packets.h"
#include "../FS/query.h"

const char version[]="\0$VER: SFSquery 1.0 (" ADATE ")\r\n";

LONG main() {
  struct RDArgs *readarg;
  UBYTE template[]="DEVICE/A\n";

  struct {char *name;} arglist={NULL};

  if((DOSBase=(struct DosLibrary *)OpenLibrary("dos.library",37))!=0) {
    if((readarg=ReadArgs(template,(IPTR *)&arglist,0))!=0) {
      struct MsgPort *msgport;
      struct DosList *dl;
      UBYTE *devname=arglist.name;

      while(*devname!=0) {
        if(*devname==':') {
          *devname=0;
          break;
        }
        devname++;
      }

      dl=LockDosList(LDF_DEVICES|LDF_READ);
      if((dl=FindDosEntry(dl,arglist.name,LDF_DEVICES))!=0) {
        LONG errorcode;

        msgport=dl->dol_Task;
        UnLockDosList(LDF_DEVICES|LDF_READ);

        {
            struct TagItem tags[]={
		{ASQ_CACHE_ACCESSES       , 0},
                {ASQ_CACHE_MISSES         , 0},

                {ASQ_START_BYTEH          , 0},
                {ASQ_START_BYTEL          , 0},
                {ASQ_END_BYTEH            , 0},
                {ASQ_END_BYTEL            , 0},
                {ASQ_DEVICE_API           , 0},

                {ASQ_BLOCK_SIZE           , 0},
                {ASQ_TOTAL_BLOCKS         , 0},

                {ASQ_ROOTBLOCK            , 0},
                {ASQ_ROOTBLOCK_OBJECTNODES, 0},
                {ASQ_ROOTBLOCK_EXTENTS    , 0},
                {ASQ_FIRST_BITMAP_BLOCK   , 0},
                {ASQ_FIRST_ADMINSPACE     , 0},

                {ASQ_CACHE_LINES          , 0},
                {ASQ_CACHE_READAHEADSIZE  , 0},
                {ASQ_CACHE_MODE           , 0},
                {ASQ_CACHE_BUFFERS        , 0},

                {ASQ_IS_CASESENSITIVE     , 0},
                {ASQ_HAS_RECYCLED         , 0},
                {TAG_END                  , 0}
	    };


          printf("SFSquery information for %s:\n", arglist.name);
          if((errorcode=DoPkt(msgport, ACTION_SFS_QUERY, (SIPTR)&tags, 0, 0, 0, 0))!=DOSFALSE) {
            ULONG perc;

            if(tags[0].ti_Data!=0) {
              perc=tags[1].ti_Data*100 / tags[0].ti_Data;
            }
            else {
              perc=0;
            }

            printf("Start/end-offset : 0x%08lx:%08lx - 0x%08lx:%08lx bytes\n", tags[2].ti_Data, tags[3].ti_Data, tags[4].ti_Data, tags[5].ti_Data);
            printf("Device API       : ");

            switch(tags[6].ti_Data) {
            case ASQDA_NSD:
              printf("NSD (64-bit)\n");
              break;
            case ASQDA_TD64:
              printf("TD64\n");
              break;
            case ASQDA_SCSIDIRECT:
              printf("SCSI direct\n");
              break;
            default:
              printf("(standard)\n");
              break;
            }

            printf("Bytes/block      : %-8ld   Total blocks : %ld\n", tags[7].ti_Data, tags[8].ti_Data);
            printf("Cache accesses   : %-8ld   Cache misses : %ld (%ld%%)\n", tags[0].ti_Data, tags[1].ti_Data, (long)perc);
            printf("Read-ahead cache : %ldx %ld bytes ",tags[14].ti_Data, tags[15].ti_Data);

            if(tags[16].ti_Data!=0) {
              printf("(Copyback)\n");
            }
            else {
              printf("(Write-through)\n");
            }

            printf("DOS buffers      : %-8ld\n", tags[17].ti_Data);

            printf("SFS settings     : ");

            if(tags[18].ti_Data!=0) {
              printf("[CASE SENSITIVE] ");
            }

            if(tags[19].ti_Data!=0) {
              printf("[RECYCLED] ");
            }

            printf("\n");
          }
        }
      }
      else {
        VPrintf("Couldn't find device '%s:'.\n",(IPTR *)&arglist.name);
        UnLockDosList(LDF_DEVICES|LDF_READ);
      }

      FreeArgs(readarg);
    }
    else {
      PutStr("Wrong arguments!\n");
    }
    CloseLibrary((struct Library *)DOSBase);
  }
  return(0);
}
