/*
** AddUSBHardware by Chris Hodges <chrisly@platon42.de>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <exec/exec.h>
#include <dos/dosextens.h>
#include <dos/datetime.h>
#include <dos/exall.h>
#include <libraries/poseidon.h>
#include <proto/poseidon.h>
#include <proto/exec.h>
#include <proto/dos.h>

#define ARGS_DEVICE   0
#define ARGS_UNIT     1
#define ARGS_QUIET    2
#define ARGS_REMOVE   3
#define ARGS_ALL      4
#define ARGS_SIZEOF   5

static const char *template = "DEVICE,UNIT/N,QUIET/S,REMOVE/S,ALL/S";
static const char *version = "$VER: AddUSBHardware 1.7 (03.06.09) by Chris Hodges <chrisly@platon42.de>";
static IPTR ArgsArray[ARGS_SIZEOF];
static struct RDArgs *ArgsHook = NULL;

//extern struct DOSBase *DOSBase;

void fail(char *str)
{
    if(ArgsHook)
    {
        FreeArgs(ArgsHook);
        ArgsHook = NULL;
    }
    if(str)
    {
        PutStr(str);
        exit(20);
    }
    exit(0);
}

int main(int argc, char *argv[])
{
    struct Library *ps;
    char *errmsg = NULL;
    struct List *phwlist;
    struct Node *phw;
    struct Node *next;
    ULONG unit;
    STRPTR devname = NULL;
    ULONG cmpunit;
    STRPTR cmpdevname;

    if(!(ArgsHook = ReadArgs(template, ArgsArray, NULL)))
    {
        fail("Wrong arguments!\n");
    }

    if((!ArgsArray[ARGS_DEVICE]) && (!(ArgsArray[ARGS_REMOVE] && ArgsArray[ARGS_ALL])))
    {
        fail("DEVICE argument is mandatory except for REMOVE ALL!\n");
    }
    
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        unit = 0;
        if(ArgsArray[ARGS_DEVICE])
        {
            devname = (STRPTR) ArgsArray[ARGS_DEVICE];
        }
        if(ArgsArray[ARGS_UNIT])
        {
            unit = *((ULONG *) ArgsArray[ARGS_UNIT]);
        }
        if(ArgsArray[ARGS_REMOVE])
        {
            psdLockReadPBase();
            do
            {
                psdGetAttrs(PGA_STACK, NULL, PA_HardwareList, &phwlist, TAG_END);
                phw = phwlist->lh_Head;
                while((next = phw->ln_Succ))
                {
                    psdGetAttrs(PGA_HARDWARE, phw,
                                HA_DeviceName, &cmpdevname,
                                HA_DeviceUnit, &cmpunit,
                                TAG_END);
                    if(ArgsArray[ARGS_ALL] || ((!stricmp(FilePart(cmpdevname), FilePart(devname))) && (cmpunit == unit)))
                    {
                        if(!ArgsArray[ARGS_QUIET])
                        {
                            Printf("Removing hardware %s, unit %ld...\n", cmpdevname, cmpunit);
                        }
                        psdUnlockPBase();
                        psdRemHardware(phw);
                        psdLockReadPBase();
                        break;
                    }
                    phw = next;
                }
            } while(ArgsArray[ARGS_ALL] && next);
            psdUnlockPBase();
        } else {
            do
            {
                if(!ArgsArray[ARGS_QUIET])
                {
                    Printf("Adding hardware %s, unit %ld...", devname, unit);
                }
                if((phw = psdAddHardware(devname, unit)))
                {
                    if(!ArgsArray[ARGS_QUIET])
                    {
                        if(psdEnumerateHardware(phw))
                        {
                            PutStr("okay!\n");
                        } else {
                            PutStr("enumeration failed!\n");
                        }
                    } else {
                        psdEnumerateHardware(phw);
                    }
                } else {
                    if(!ArgsArray[ARGS_QUIET])
                    {
                        PutStr("failed!\n");
                    }
                    errmsg = "";
                    break;
                }
                unit++;
            } while(ArgsArray[ARGS_ALL]);
            psdClassScan();
        }
        CloseLibrary(ps);
    } else {
        errmsg = "Unable to open poseidon.library\n";
    }
    fail(errmsg);
    return(0); // never gets here, just to shut the compiler up
}
