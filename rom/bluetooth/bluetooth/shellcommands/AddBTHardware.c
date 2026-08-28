/*
** AddBTHardware (ported from the Poseidon shell commands by Chris Hodges)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <exec/exec.h>
#include <dos/dosextens.h>
#include <dos/datetime.h>
#include <dos/exall.h>
#include <libraries/bluetooth.h>
#include <proto/bluetooth.h>
#include <proto/exec.h>
#include <proto/dos.h>

#define ARGS_DEVICE   0
#define ARGS_UNIT     1
#define ARGS_QUIET    2
#define ARGS_REMOVE   3
#define ARGS_ALL      4
#define ARGS_SIZEOF   5

static const char *template = "DEVICE,UNIT/N,QUIET/S,REMOVE/S,ALL/S";
const char *version = "$VER: AddBTHardware 1.0 (18.08.2026) (ported from the Poseidon shell commands by Chris Hodges)";
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
    struct Library *BluetoothBase;
    char *errmsg = NULL;
    struct List *bthlist;
    struct Node *bth;
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
    
    if((BluetoothBase = OpenLibrary("bluetooth.library", 1)))
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
            btLockReadBase();
            do
            {
                btGetAttrs(BGA_STACK, NULL, BSA_HardwareList, &bthlist, TAG_END);
                bth = bthlist->lh_Head;
                while((next = bth->ln_Succ))
                {
                    btGetAttrs(BGA_HARDWARE, bth,
                                BHA_DeviceName, &cmpdevname,
                                BHA_DeviceUnit, &cmpunit,
                                TAG_END);
                    if(ArgsArray[ARGS_ALL] || ((!stricmp(FilePart(cmpdevname), FilePart(devname))) && (cmpunit == unit)))
                    {
                        if(!ArgsArray[ARGS_QUIET])
                        {
                            Printf("Removing hardware %s, unit %ld...\n", cmpdevname, cmpunit);
                        }
                        btUnlockBase();
                        btRemHardware(bth);
                        btLockReadBase();
                        break;
                    }
                    bth = next;
                }
            } while(ArgsArray[ARGS_ALL] && next);
            btUnlockBase();
        } else {
            do
            {
                if(!ArgsArray[ARGS_QUIET])
                {
                    Printf("Adding hardware %s, unit %ld...", devname, unit);
                }
                if((bth = btAddHardware(devname, unit)))
                {
                    if(!ArgsArray[ARGS_QUIET])
                    {
                        if(btEnumerateHardware(bth))
                        {
                            PutStr("okay!\n");
                        } else {
                            PutStr("enumeration failed!\n");
                        }
                    } else {
                        btEnumerateHardware(bth);
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
            btClassScan();
        }
        CloseLibrary(BluetoothBase);
    } else {
        errmsg = "Unable to open bluetooth.library\n";
    }
    fail(errmsg);
    return(0); // never gets here, just to shut the compiler up
}
