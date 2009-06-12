/*
 *----------------------------------------------------------------------------
 *                     UPS Launcher Tool for Poseidon
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "debug.h"

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/poseidon.h>

#include <devices/usb_hid.h>

#include "UPSTool.h"
#include <string.h>

#define ARGS_RAW            0
#define ARGS_ID             1
#define ARGS_QUERY          2
#define ARGS_TOGGLE_BEEPER  3
#define ARGS_SHUTDOWN       4
#define ARGS_START_BT       5
#define ARGS_STOP_BT        6
#define ARGS_UNIT           7
#define ARGS_SIZEOF         8

static const char *prgname = "UPSTool";
static const char *template = "RAW/K,ID/S,QUERY/S,TOGGLEBEEPER=TG/S,SHUTDOWN/S,STARTBATTERYTEST=STARTBT/S,STOPBATTERYTEST=STOPBT/S,UNIT/N/K";
static const char *version = "$VER: UPSTool 1.0 (12.06.09) by Chris Hodges <chrisly@platon42.de>";
static IPTR ArgsArray[ARGS_SIZEOF];
static struct RDArgs *ArgsHook = NULL;

struct Library *ps;

AROS_UFP3(void, releasehook,
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(APTR, pab, A2),
          AROS_UFPA(struct NepClassUPS *, nch, A1));

struct NepClassUPS * SetupUPS(void);
struct NepClassUPS * AllocUPS(struct NepClassUPS *nch);
void FreeUPS(struct NepClassUPS *nch);

AROS_UFH3(void, releasehook,
          AROS_UFHA(struct Hook *, hook, A0),
          AROS_UFHA(APTR, pab, A2),
          AROS_UFHA(struct NepClassUPS *, nch, A1))
{
    AROS_USERFUNC_INIT
    /*psdAddErrorMsg(RETURN_WARN, (STRPTR) prgname,
                   "UPS killed!");*/
    Signal(nch->nch_Task, SIGBREAKF_CTRL_C);
    AROS_USERFUNC_EXIT
}

struct NepClassUPS * SetupUPS(void)
{
    struct NepClassUPS *nch;
    struct PsdDevice *pd = NULL;
    struct PsdAppBinding *pab;
    ULONG unit;

    if(ArgsArray[ARGS_UNIT])
    {
        unit = *((ULONG *) ArgsArray[ARGS_UNIT]);
    } else {
        unit = 0;
    }
    do
    {
        do
        {
            pd = psdFindDevice(pd,
                               DA_VendorID, 0x0F03,
                               DA_ProductID, 0x0001,
                               TAG_END);

        } while(pd && (unit--));

        if(!pd)
        {
            PutStr("No UPS found!\n");
            return(NULL);
        }
        if((nch = psdAllocVec(sizeof(struct NepClassUPS))))
        {
            nch->nch_Device = pd;
            nch->nch_ReleaseHook.h_Entry = (APTR) releasehook;
            pab = psdClaimAppBinding(ABA_Device, pd,
                                     ABA_ReleaseHook, &nch->nch_ReleaseHook,
                                     ABA_UserData, nch,
                                     ABA_ForceRelease, TRUE,
                                     TAG_END);
            if(pab)
            {
                if(AllocUPS(nch))
                {
                    return(nch);
                } else {
                    PutStr("Couldn't allocate UPS...\n");
                }
                psdReleaseAppBinding(pab);
            } else {
                PutStr("Couldn't claim binding!\n");
            }
            psdFreeVec(nch);
        }
        PutStr("Hohum...\n");
    } while(TRUE);
    return(NULL);
}

struct NepClassUPS * AllocUPS(struct NepClassUPS *nch)
{
    nch->nch_Task = FindTask(NULL);

    if((nch->nch_TaskMsgPort = CreateMsgPort()))
    {
        nch->nch_IF0 = psdFindInterface(nch->nch_Device, NULL, TAG_END);
        if(nch->nch_IF0)
        {
            nch->nch_IntEP = psdFindEndpoint(nch->nch_IF0, NULL,
                                             EA_IsIn, TRUE,
                                             EA_TransferType, USEAF_INTERRUPT,
                                             TAG_END);
            if(nch->nch_IntEP)
            {
                if((nch->nch_EP0Pipe = psdAllocPipe(nch->nch_Device, nch->nch_TaskMsgPort, NULL)))
                {
                    if((nch->nch_IntPipe = psdAllocPipe(nch->nch_Device, nch->nch_TaskMsgPort, nch->nch_IntEP)))
                    {
                        psdSetAttrs(PGA_PIPE, nch->nch_IntPipe,
                                    PPA_AllowRuntPackets, TRUE,
                                    PPA_NakTimeout, TRUE,
                                    PPA_NakTimeoutTime, 5000,
                                    TAG_END);
                        return(nch);
                    }
                    psdFreePipe(nch->nch_EP0Pipe);
                    PutStr("Couldn't allocate int pipe\n");
                } else {
                    PutStr("Couldn't allocate default pipe\n");
                }
            } else {
                PutStr("Couldn't find endpoint\n");
            }
        } else {
            PutStr("Couldn't find interface\n");
        }
        DeleteMsgPort(nch->nch_TaskMsgPort);
    }
    return(NULL);
}


void FreeUPS(struct NepClassUPS *nch)
{
    APTR pab;

    psdGetAttrs(PGA_DEVICE, nch->nch_Device,
                DA_Binding, &pab,
                TAG_END);
    psdReleaseAppBinding(pab);
    psdFreePipe(nch->nch_IntPipe);
    psdFreePipe(nch->nch_EP0Pipe);
    DeleteMsgPort(nch->nch_TaskMsgPort);
    psdFreeVec(nch);
}

BOOL SendCommand(struct NepClassUPS *nch, STRPTR cmd)
{
    LONG ioerr;
    psdSafeRawDoFmt(nch->nch_Buf, 64, "%s\r", cmd);
    psdPipeSetup(nch->nch_EP0Pipe, URTF_OUT|URTF_CLASS|URTF_INTERFACE, UHR_SET_REPORT, 0x0200, 0);
    ioerr = psdDoPipe(nch->nch_EP0Pipe, nch->nch_Buf, (ULONG) ((strlen(nch->nch_Buf)+7) & ~7));
    if(ioerr)
    {
        Printf("Error sending cmd %s: %s (%ld)\n",
               cmd,
               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
        return(FALSE);
    }
    return(TRUE);
}

STRPTR GetReply(struct NepClassUPS *nch)
{
    LONG ioerr;
    ULONG len;
    ioerr = psdDoPipe(nch->nch_IntPipe, nch->nch_Reply, 63);
    if(ioerr)
    {
        Printf("Error receiving reply: %s (%ld)\n",
               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
        return(NULL);
    }
    len = psdGetPipeActual(nch->nch_IntPipe);
    if((len > 0) && (nch->nch_Reply[len-1] == '\r'))
    {
        len--; // eliminate trailing cr
    }
    nch->nch_Reply[len] = 0; // string termination
    return(nch->nch_Reply);
}
/**************************************************************************/

int main(int argc, char *argv[])
{
    struct NepClassUPS *nch;
    ULONG ret = RETURN_ERROR;
    STRPTR replymsg;

    if(!(ArgsHook = ReadArgs(template, ArgsArray, NULL)))
    {
        PutStr("Wrong arguments!\n");
        return(RETURN_FAIL);
    }
    ps = OpenLibrary("poseidon.library", 3);
    if(!ps)
    {
        FreeArgs(ArgsHook);
        return(RETURN_FAIL);
    }
    if(!(nch = SetupUPS()))
    {
        FreeArgs(ArgsHook);
        CloseLibrary(ps);
        return(RETURN_ERROR);
    }
    if(ArgsArray[ARGS_RAW])
    {
        if(SendCommand(nch, (STRPTR) ArgsArray[ARGS_RAW]))
        {
            if((replymsg = GetReply(nch)))
            {
                Printf("%s\n", replymsg);
                ret = RETURN_OK;
            }
        }
    }
    else if(ArgsArray[ARGS_ID])
    {
        if(SendCommand(nch, "I"))
        {
            if((replymsg = GetReply(nch)))
            {
                if(replymsg[0] == '#')
                {
                    Printf("%s\n", replymsg+1);
                    ret = RETURN_OK;
                } else {
                    PutStr("Bogus reply!\n");
                }
            }
        }
    }
    else if(ArgsArray[ARGS_QUERY])
    {
        if(SendCommand(nch, "Q1"))
        {
            if((replymsg = GetReply(nch)))
            {
                if(replymsg[0] == '(')
                {
                    Printf("%s\n", replymsg+1);
                    ret = RETURN_OK;
                } else {
                    PutStr("Bogus reply!\n");
                }
            }
        }
    }
    else if(ArgsArray[ARGS_TOGGLE_BEEPER])
    {
        if(SendCommand(nch, "Q"))
        {
            if((replymsg = GetReply(nch)))
            {
                PutStr("Beeper toggled!\n");
                ret = RETURN_OK;
            }
        }
    }
    else if(ArgsArray[ARGS_SHUTDOWN])
    {
        if(SendCommand(nch, "S01"))
        {
            PutStr("Shutting down UPS!\n");
            ret = RETURN_OK;
        }
    }
    else if(ArgsArray[ARGS_START_BT])
    {
        if(SendCommand(nch, "TL"))
        {
            PutStr("Battery Test started!\n");
            ret = RETURN_OK;
        }
    }
    else if(ArgsArray[ARGS_STOP_BT])
    {
        if(SendCommand(nch, "CT"))
        {
            PutStr("Battery Test stopped!\n");
            ret = RETURN_OK;
        }
    } else {
        if(SendCommand(nch, "Q1"))
        {
            if((replymsg = GetReply(nch)))
            {
                if(*replymsg++ == '(')
                {
                    UWORD cnt = 7;
                    // get status fields
                    while(*replymsg)
                    {
                        if(*replymsg++ == ' ')
                        {
                            if(!(--cnt))
                            {
                                break;
                            }
                        }
                    }
                    if(*replymsg)
                    {
                        if((*replymsg == '0') && (replymsg[5] == '0'))
                        {
                            PutStr("On line.\n");
                            ret = RETURN_OK;
                        } else {
                            PutStr("On battery!\n");
                            ret = RETURN_WARN;
                        }
                    }
                } else {
                    PutStr("Bogus reply!\n");
                }
            }
        }
    }
    FreeUPS(nch);
    FreeArgs(ArgsHook);
    CloseLibrary(ps);
    return(ret);
}
