#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <utility/tagitem.h>

#include "../FS/packets.h"

const char version[]="\0$VER: SFSformat 1.1 (" ADATE ")\r\n";

LONG main()
{
    struct RDArgs *readarg;
    UBYTE template[]="DEVICE=DRIVE/A/K,NAME/A/K,CASESENSITIVE/S,NORECYCLED/S,SHOWRECYCLED/S";
    UBYTE choice='N';

    struct {
        char *device;
        char *name;
        IPTR  casesensitive;
        IPTR  norecycled;
        IPTR  showrecycled;
    } arglist={NULL};

    if((DOSBase=(struct DosLibrary *)OpenLibrary("dos.library",37))!=0)
    {
        if((readarg=ReadArgs(template,(IPTR *)&arglist,0))!=0)
        {
            struct MsgPort *msgport;
            struct DosList *dl;
            UBYTE *devname=arglist.device;

            while(*devname!=0)
            {
                if(*devname==':')
                {
//                    *devname=0;
                    break;
                }
                devname++;
            }

            dl=LockDosList(LDF_DEVICES|LDF_READ);
            if((dl=FindDosEntry(dl,arglist.device,LDF_DEVICES))!=0)
            {
                LONG errorcode=0;

                msgport=dl->dol_Task;
                UnLockDosList(LDF_DEVICES|LDF_READ);
                        
                Printf("About to format drive %s. ", arglist.device);
                Printf("This will destroy all data on the drive!\n");
                Printf("Are you sure? (y/N)"); Flush(Output());
        
                Read(Input(), &choice, 1);
                
                if(choice == 'y' || choice == 'Y')
                {
                    //Printf("a");
                    if(Inhibit(arglist.device,DOSTRUE))
                    {
                    //Printf("b");
                        {
                            struct TagItem tags[5];
                            struct TagItem *tag=tags;

                            tag->ti_Tag=ASF_NAME;
                            tag->ti_Data=(IPTR)arglist.name;
                            tag++;

                            if(arglist.casesensitive!=0)
                            {
                                tag->ti_Tag=ASF_CASESENSITIVE;
                                tag->ti_Data=TRUE;
                                tag++;
                            }

                            if(arglist.norecycled!=0)
                            {
                                tag->ti_Tag=ASF_NORECYCLED;
                                tag->ti_Data=TRUE;
                                tag++;
                            }

                            if(arglist.showrecycled!=0)
                            {
                                tag->ti_Tag=ASF_SHOWRECYCLED;
                                tag->ti_Data=TRUE;
                                tag++;
                            }

                            tag->ti_Tag=TAG_END;
                            tag->ti_Data=0;

                            if((errorcode=DoPkt(msgport, ACTION_SFS_FORMAT, (SIPTR)&tags, 0, 0, 0, 0))==DOSFALSE)
                            {
                                PrintFault(IoErr(),"error while initializing the drive");
                            }
                        }

                        Inhibit(arglist.device,DOSFALSE);
                    }
                    else {
                        PrintFault(IoErr(),"error while locking the drive");
                    }
                }
                else if(SetSignal(0L,SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C)
                {
                    PutStr("\n***Break\n");
                }
            }
            else {
                VPrintf("Unknown device %s\n",(IPTR *)&arglist.device);
                UnLockDosList(LDF_DEVICES|LDF_READ);
            }
            FreeArgs(readarg);
        }
        else {
            PutStr("wrong args!\n");
        }

        CloseLibrary((struct Library *)DOSBase);
    }
    return(0);
}
