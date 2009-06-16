/*
 *----------------------------------------------------------------------------
 *                     Power Management Tool for Poseidon
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "debug.h"

#include <devices/usb_hid.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/poseidon.h>

#include "PowManTool.h"
#include <string.h>

#define ARGS_OUTLET         0
#define ARGS_ON             1
#define ARGS_OFF            2
#define ARGS_TOGGLE         3
#define ARGS_STATUS         4
#define ARGS_UNIT           5
#define ARGS_SIZEOF         6

static const char *prgname = "PowManTool";
static const char *template = "SOCKET=OUTLET/N,ON/S,OFF/S,TOGGLE/S,STATUS/S,UNIT/N/K";
static const char *version = "$VER: PowManTool 1.0 (12.06.09) by Chris Hodges <chrisly@platon42.de>";
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
                   "PowMan killed!");*/
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
                               DA_VendorID, 0x04b4,
                               DA_ProductID, 0xfd11,
                               TAG_END);

        } while(pd && (unit--));

        if(!pd)
        {
            PutStr("No GemBird PowerManager found!\n");
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
                    PutStr("Couldn't allocate PowerManager...\n");
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
        if((nch->nch_EP0Pipe = psdAllocPipe(nch->nch_Device, nch->nch_TaskMsgPort, NULL)))
        {
            return(nch);
        } else {
            PutStr("Couldn't allocate default pipe\n");
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
    psdFreePipe(nch->nch_EP0Pipe);
    DeleteMsgPort(nch->nch_TaskMsgPort);
    psdFreeVec(nch);
}

BOOL SendCommand(struct NepClassUPS *nch, ULONG outlet, ULONG cmd)
{
    LONG ioerr;
    UBYTE buf[2];
    buf[0] = outlet*3;
    buf[1] = cmd;
    psdPipeSetup(nch->nch_EP0Pipe, URTF_OUT|URTF_CLASS|URTF_INTERFACE, UHR_SET_REPORT, (ULONG) 0x0300|buf[0], 0);
    ioerr = psdDoPipe(nch->nch_EP0Pipe, buf, 2);
    if(ioerr)
    {
        Printf("Error sending cmd %s: %s (%ld)\n",
               cmd,
               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
        return(FALSE);
    }
    return(TRUE);
}

BOOL GetStatus(struct NepClassUPS *nch, ULONG outlet)
{
    LONG ioerr;
    UBYTE buf[2];
    psdPipeSetup(nch->nch_EP0Pipe, URTF_IN|URTF_CLASS|URTF_INTERFACE, UHR_GET_REPORT, 0x0300|(outlet*3), 0);
    ioerr = psdDoPipe(nch->nch_EP0Pipe, buf, 2);
    if(ioerr)
    {
        Printf("Error getting status: %s (%ld)\n",
               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
        return(FALSE);
    }
    return(buf[1]);
}

/**************************************************************************/

int main(int argc, char *argv[])
{
    struct NepClassUPS *nch;
    ULONG ret;
    LONG outlet = 0;
    BOOL all = FALSE;
    if(!(ArgsHook = ReadArgs(template, ArgsArray, NULL)))
    {
        PutStr("Wrong arguments!\n");
        return(RETURN_FAIL);
    }
    if(ArgsArray[ARGS_OUTLET])
    {
        outlet = *((LONG *) ArgsArray[ARGS_OUTLET]);
        if((outlet < 1) || (outlet > 4))
        {
            PutStr("Only outlets from 1 to 4 are supported!\n");
            FreeArgs(ArgsHook);
            return(RETURN_ERROR);
        }
    }
    ps = OpenLibrary("poseidon.library", 4);
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
    if(!outlet)
    {
        all = TRUE;
        outlet = 1;
    }
    ret = RETURN_OK;
    do
    {
        if(ArgsArray[ARGS_TOGGLE])
        {
            if(GetStatus(nch, outlet))
            {
                SendCommand(nch, outlet, 0x00);
            } else {
                SendCommand(nch, outlet, 0x03);
            }
        }
        if(ArgsArray[ARGS_STATUS])
        {
            if(ArgsArray[ARGS_OFF])
            {
                ret |= GetStatus(nch, outlet) ? RETURN_WARN : RETURN_OK;
            } else {
                ret |= GetStatus(nch, outlet) ? RETURN_OK : RETURN_WARN;
            }
        } else {
            if(ArgsArray[ARGS_ON])
            {
                SendCommand(nch, outlet, 0x03);
            }
            else if(ArgsArray[ARGS_OFF])
            {
                SendCommand(nch, outlet, 0x00);
            }
        }
        outlet++;
    } while(all && (outlet <= 4));

    FreeUPS(nch);
    FreeArgs(ArgsHook);
    CloseLibrary(ps);
    return(ret);
}
