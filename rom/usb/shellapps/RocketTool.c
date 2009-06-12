/*
 *----------------------------------------------------------------------------
 *                     Rocket Launcher Tool for Poseidon
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "debug.h"

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/poseidon.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/diskfont.h>
#include <proto/lowlevel.h>

#include <libraries/lowlevel.h>
#include <devices/usb_hid.h>

#include "RocketTool.h"
#include <string.h>

#define ARGS_LEFT     0
#define ARGS_RIGHT    1
#define ARGS_UP       2
#define ARGS_DOWN     3
#define ARGS_FIRE     4
#define ARGS_TIME     5
#define ARGS_JOYPORT  6
#define ARGS_UNIT     7
#define ARGS_SIZEOF   8

static const char *prgname = "RocketTool";
static const char *template = "LEFT/S,RIGHT/S,UP/S,DOWN/S,FIRE/S,TIME/N/K,JOYPORT/N/K,UNIT/N/K";
static const char *version = "$VER: RocketTool 1.1 (12.06.09) by Chris Hodges <chrisly@platon42.de>";
static IPTR ArgsArray[ARGS_SIZEOF];
static struct RDArgs *ArgsHook = NULL;

struct Library *ps;
struct Library *LowLevelBase;

AROS_UFP3(void, releasehook,
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(APTR, pab, A2),
          AROS_UFPA(struct NepClassRocket *, nch, A1));

struct NepClassRocket * SetupRocket(void);
struct NepClassRocket * AllocRocket(struct NepClassRocket *nch);
void FreeRocket(struct NepClassRocket *nch);

AROS_UFH3(void, releasehook,
          AROS_UFHA(struct Hook *, hook, A0),
          AROS_UFHA(APTR, pab, A2),
          AROS_UFHA(struct NepClassRocket *, nch, A1))
{
    AROS_USERFUNC_INIT
    /*psdAddErrorMsg(RETURN_WARN, (STRPTR) prgname,
                   "Rocket killed!");*/
    Signal(nch->nch_Task, SIGBREAKF_CTRL_C);
    AROS_USERFUNC_EXIT
}

struct NepClassRocket * SetupRocket(void)
{
    struct NepClassRocket *nch;
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
                               DA_VendorID, 0x1130,
                               DA_ProductID, 0x0202,
                               TAG_END);
        } while(pd && (unit--));

        if(!pd)
        {
            PutStr("No USB Rocket Launcher found!\n");
            return(NULL);
        }
        if((nch = psdAllocVec(sizeof(struct NepClassRocket))))
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
                if(AllocRocket(nch))
                {
                    return(nch);
                } else {
                    PutStr("Couldn't allocate Rocket Laucher...\n");
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

struct NepClassRocket * AllocRocket(struct NepClassRocket *nch)
{
    nch->nch_Task = FindTask(NULL);

    if((nch->nch_TaskMsgPort = CreateMsgPort()))
    {
        nch->nch_IF0 = psdFindInterface(nch->nch_Device, NULL,
                                        IFA_InterfaceNum, 0,
                                        TAG_END);
        nch->nch_IF1 = psdFindInterface(nch->nch_Device, NULL,
                                        IFA_InterfaceNum, 1,
                                        TAG_END);
        if(nch->nch_IF0 && nch->nch_IF1)
        {
            if((nch->nch_EP0Pipe = psdAllocPipe(nch->nch_Device, nch->nch_TaskMsgPort, NULL)))
            {
                return(nch);
            } else {
                PutStr("Couldn't allocate default pipe\n");
            }
        } else {
            PutStr("Couldn't find interfaces\n");
        }
        DeleteMsgPort(nch->nch_TaskMsgPort);
    }
    return(NULL);
}


void FreeRocket(struct NepClassRocket *nch)
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

BOOL SendInit(struct NepClassRocket *nch)
{
    UBYTE cmd0[8] = { 0x55, 0x53, 0x42, 0x43, 0x00, 0x00, 0x04, 0x00 };
    UBYTE cmd1[8] = { 0x55, 0x53, 0x42, 0x43, 0x00, 0x40, 0x02, 0x00 };
    LONG ioerr;

    psdPipeSetup(nch->nch_EP0Pipe, URTF_OUT|URTF_CLASS|URTF_INTERFACE, UHR_SET_REPORT, 0x0200, 1);
    ioerr = psdDoPipe(nch->nch_EP0Pipe, cmd0, 8);
    if(ioerr)
    {
        Printf("Error sending init cmd 0: %s (%ld)\n",
               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
        return FALSE;
    }
    psdPipeSetup(nch->nch_EP0Pipe, URTF_OUT|URTF_CLASS|URTF_INTERFACE, UHR_SET_REPORT, 0x0200, 1);
    ioerr = psdDoPipe(nch->nch_EP0Pipe, cmd1, 8);
    if(ioerr)
    {
        Printf("Error sending init cmd 1: %s (%ld)\n",
               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
        return FALSE;
    }
    return TRUE;
}

BOOL SendCommand(struct NepClassRocket *nch)
{
    LONG ioerr;

    psdPipeSetup(nch->nch_EP0Pipe, URTF_OUT|URTF_CLASS|URTF_INTERFACE, UHR_SET_REPORT, 0x0200, 0);
    ioerr = psdDoPipe(nch->nch_EP0Pipe, nch->nch_Buf, 64);
    if(ioerr)
    {
        Printf("Error sending command: %s (%ld)\n",
               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
        return FALSE;
    }
    return TRUE;
}

/**************************************************************************/

int main(int argc, char *argv[])
{
    struct NepClassRocket *nch;
    ULONG ret = RETURN_OK;
    ULONG joyport;

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
    if(!(nch = SetupRocket()))
    {
        FreeArgs(ArgsHook);
        CloseLibrary(ps);
        return(RETURN_ERROR);
    }
    if(ArgsArray[ARGS_JOYPORT])
    {
        // interactive mode
        joyport = *((ULONG *) ArgsArray[ARGS_JOYPORT]);
        if((LowLevelBase = OpenLibrary("lowlevel.library", 1)))
        {
            ULONG val;
            Printf("Interactive mode using joyport %ld. Press Ctrl-C to abort.\n", joyport);

            // FIXME this seems to be not implemented in AROS yet
            //SetJoyPortAttrs(joyport, SJA_Reinitialize, TRUE, TAG_END);
            //SetJoyPortAttrs(joyport, SJA_Type, SJA_TYPE_AUTOSENSE, TAG_END);
            do
            {
                if(SetSignal(0,0) & SIGBREAKF_CTRL_C)
                {
                    break;
                }
                val = ReadJoyPort(joyport);
                if(((val & JP_TYPE_MASK) == JP_TYPE_JOYSTK) || ((val & JP_TYPE_MASK) == JP_TYPE_GAMECTLR))
                {
                    nch->nch_Buf[1] = (val & JPF_JOY_LEFT) ? 1 : 0;
                    nch->nch_Buf[2] = (val & JPF_JOY_RIGHT) ? 1 : 0;
                    nch->nch_Buf[3] = (val & JPF_JOY_UP) ? 1 : 0;
                    nch->nch_Buf[4] = (val & JPF_JOY_DOWN) ? 1 : 0;
                    nch->nch_Buf[5] = (val & JPF_BUTTON_RED) ? 1 : 0;
                }
                if(SendInit(nch))
                {
                    SendCommand(nch);
                } else {
                    ret = RETURN_WARN;
                    break;
                }
                Delay(10);
            } while(TRUE);
            //SetJoyPortAttrs(1, SJA_Reinitialize, TRUE, TAG_END);

            CloseLibrary(LowLevelBase);
        } else {
            ret = RETURN_FAIL;
        }
    } else {
        if(ArgsArray[ARGS_LEFT])
        {
            nch->nch_Buf[1] = 1;
        }
        if(ArgsArray[ARGS_RIGHT])
        {
            nch->nch_Buf[2] = 1;
        }
        if(ArgsArray[ARGS_UP])
        {
            nch->nch_Buf[3] = 1;
        }
        if(ArgsArray[ARGS_DOWN])
        {
            nch->nch_Buf[4] = 1;
        }
        if(ArgsArray[ARGS_FIRE])
        {
            nch->nch_Buf[5] = 1;
        }
        nch->nch_Buf[6] = 0x08;
        nch->nch_Buf[7] = 0x08;
        if(SendInit(nch))
        {
            SendCommand(nch);
            if(ArgsArray[ARGS_TIME])
            {
                Delay(*((ULONG *) ArgsArray[ARGS_TIME]));
                memset(nch->nch_Buf, 0x00, 6);
                SendInit(nch);
                SendCommand(nch);
            }
        } else {
            ret = RETURN_WARN;
        }
    }
    FreeRocket(nch);
    FreeArgs(ArgsHook);
    CloseLibrary(ps);
    return(ret);
}
