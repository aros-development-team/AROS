#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <utility/tagitem.h>

#include "../FS/packets.h"
#include "../FS/query.h"

const char version[]="\0$VER: SetCache 1.2 (" ADATE ")\r\n";

int main()
{
    struct RDArgs *readarg;
    UBYTE template[]="DEVICE/A,LINES/N,READAHEAD/N,NOCOPYBACK/S\n";

    struct {char *name;
          ULONG *lines;
          ULONG *readahead;
          IPTR nocopyback;} arglist={NULL};

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
                ULONG copyback=1;
                LONG errorcode;
                msgport=dl->dol_Task;
                UnLockDosList(LDF_DEVICES|LDF_READ);

                if(arglist.lines!=0 || arglist.readahead!=0 || arglist.nocopyback!=0) {
                    struct TagItem tags[]={
		        {ASQ_CACHE_LINES        , 0},
                        {ASQ_CACHE_READAHEADSIZE, 0},
                        {ASQ_CACHE_MODE         , 0},
                        {TAG_END                , 0}
		    };

                    if((errorcode=DoPkt(msgport, ACTION_SFS_QUERY, (SIPTR)&tags, 0, 0, 0, 0))!=DOSFALSE) {
                        STACKULONG lines,readahead;

                        lines=tags[0].ti_Data;
                        readahead=tags[1].ti_Data;

                        if(arglist.nocopyback!=0) {
                            copyback=0;
                        }

                        if(arglist.lines!=0) {
                            lines=*arglist.lines;
                        }

                        if(arglist.readahead!=0) {
                            readahead=*arglist.readahead;
                        }

                        VPrintf("Setting cache to %ld lines ", &lines);
                        VPrintf("of %ld bytes and copyback mode ", &readahead);
                        if(copyback!=0) {
                            PutStr("enabled.\n");
                        }
                        else {
                            PutStr("disabled.\n");
                        }

                        if((errorcode=DoPkt(msgport,ACTION_SET_CACHE, lines, readahead, copyback, 0, 0))==DOSFALSE) {
                            PrintFault(IoErr(),"error while setting new cache size");
                        }
                    }
                    else {
                        PrintFault(IoErr(),"error while reading old cache settings");
                    }
                }

                {
                    struct TagItem tags[]={
			{ASQ_CACHE_LINES        , 0},
                        {ASQ_CACHE_READAHEADSIZE, 0},
                        {ASQ_CACHE_MODE         , 0}
		    };

                    if((errorcode=DoPkt(msgport, ACTION_SFS_QUERY, (SIPTR)&tags, 0, 0, 0, 0))!=DOSFALSE) {
                        VPrintf("Current cache settings: %ld lines,", &tags[0].ti_Data);
                        VPrintf(" %ld bytes readahead, ", &tags[1].ti_Data);
                        if(tags[2].ti_Data==0) {
                            PutStr("no copyback.\n");
                        }
                        else {
                            PutStr("copyback.\n");
                        }
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
