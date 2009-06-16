/*
 *----------------------------------------------------------------------------
 *                         egalaxtouch class for poseidon
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "debug.h"

#include "egalaxtouch.class.h"
#include <proto/timer.h>

/* /// "Lib Stuff" */
static const STRPTR libname = MOD_NAME_STRING;

static int libInit(LIBBASETYPEPTR nh)
{
    struct NepClassHid *nch;
    struct NepHidBase *ret = NULL;

    KPRINTF(10, ("libInit nh: 0x%08lx SysBase: 0x%08lx\n", nh, SysBase));

    nh->nh_UtilityBase = OpenLibrary("utility.library", 39);

#define	UtilityBase	nh->nh_UtilityBase

    if(UtilityBase)
    {
        NewList(&nh->nh_Bindings);
        nch = &nh->nh_DummyNCH;
        nch->nch_ClsBase = nh;
        nch->nch_Interface = NULL;
        nch->nch_CDC =  AllocVec(sizeof(struct ClsDevCfg), MEMF_PUBLIC|MEMF_CLEAR);
        if(nch->nch_CDC)
        {
            ret = nh;
        }
        if(!ret)
        {
            CloseLibrary(UtilityBase);
        }
    } else {
        KPRINTF(20, ("libInit: OpenLibrary(\"utility.library\", 39) failed!\n"));
    }

    KPRINTF(10, ("libInit: Ok\n"));
    return(ret ? TRUE : FALSE);
}

static int libOpen(LIBBASETYPEPTR nh)
{
    KPRINTF(10, ("libOpen nh: 0x%08lx\n", nh));
    nLoadClassConfig(nh);
    return(TRUE);
}

static int libExpunge(LIBBASETYPEPTR nh)
{
    KPRINTF(10, ("libExpunge nh: 0x%08lx\n", nh));
    CloseLibrary((struct Library *) UtilityBase);
    FreeVec(nh->nh_DummyNCH.nch_CDC);
    return(TRUE);
}

ADD2INITLIB(libInit, 0)
ADD2OPENLIB(libOpen, 0)
ADD2EXPUNGELIB(libExpunge, 0)
/* \\\ */


/*
 * ***********************************************************************
 * * Library functions                                                   *
 * ***********************************************************************
 */

struct AutoBindData
{
    UWORD abd_VendID;
    UWORD abd_ProdID;
};

struct AutoBindData ClassBinds[] =
{
    { 0x0123, 0x0001 },
    { 0x0123, 0x0002 },
    { 0x1234, 0x0001 },
    { 0x1234, 0x0002 },
    { 0x0EEF, 0x0001 },
    { 0x0EEF, 0x0002 },
    { 0x3823, 0x0001 },
    { 0x3823, 0x0002 },
    { 0, 0 }
};

/* /// "usbAttemptDeviceBinding()" */
struct NepClassHid * usbAttemptDeviceBinding(struct NepHidBase *nh, struct PsdDevice *pd)
{
    struct Library *ps;
    struct AutoBindData *abd = ClassBinds;
    IPTR prodid;
    IPTR vendid;

    KPRINTF(1, ("nepHidAttemptDeviceBinding(%08lx)\n", pd));
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        psdGetAttrs(PGA_DEVICE, pd,
                    DA_VendorID, &vendid,
                    DA_ProductID, &prodid,
                    TAG_END);
        CloseLibrary(ps);
        while(abd->abd_VendID)
        {
            if((vendid == abd->abd_VendID) && (prodid == abd->abd_ProdID))
            {
                return(usbForceDeviceBinding(nh, pd));
            }
            abd++;
        }
    }
    return(NULL);
}
/* \\\ */

/* /// "usbForceDeviceBinding()" */
struct NepClassHid * usbForceDeviceBinding(struct NepHidBase *nh, struct PsdDevice *pd)
{
    struct Library *ps;
    struct NepClassHid *nch;
    struct ClsDevCfg *cdc;
    STRPTR devname;
    STRPTR devidstr;
    UBYTE buf[64];
    struct Task *tmptask;

    KPRINTF(1, ("nepHidForceDeviceBinding(%08lx)\n", pd));
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        psdGetAttrs(PGA_DEVICE, pd,
                    DA_ProductName, &devname,
                    DA_IDString, &devidstr,
                    TAG_END);
        if((nch = psdAllocVec(sizeof(struct NepClassHid))))
        {
            nch->nch_ClsBase = nh;
            nch->nch_CDC = cdc = psdAllocVec(sizeof(struct ClsDevCfg));
            if(!cdc)
            {
                psdFreeVec(cdc);
                psdFreeVec(nch);
                CloseLibrary(ps);
                return(NULL);
            }

            nch->nch_Device = pd;
            nch->nch_DevIDString = devidstr;

            nLoadBindingConfig(nch);

            psdSafeRawDoFmt(buf, 64, "egalaxtouch.class<%08lx>", nch);
            nch->nch_ReadySignal = SIGB_SINGLE;
            nch->nch_ReadySigTask = FindTask(NULL);
            SetSignal(0, SIGF_SINGLE);
            if((tmptask = psdSpawnSubTask(buf, nHidTask, nch)))
            {
                psdBorrowLocksWait(tmptask, 1UL<<nch->nch_ReadySignal);
                if(nch->nch_Task)
                {
                    nch->nch_ReadySigTask = NULL;
                    //FreeSignal(nch->nch_ReadySignal);
                    psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                                   "Touching the sky with '%s'!",
                                   devname);

                    Forbid();
                    AddTail(&nh->nh_Bindings, &nch->nch_Node);
                    Permit();
                    CloseLibrary(ps);
                    return(nch);
                }
            }
            nch->nch_ReadySigTask = NULL;
            //FreeSignal(nch->nch_ReadySignal);
            psdFreeVec(nch->nch_CDC);
            psdFreeVec(nch);
        }
        CloseLibrary(ps);
    }
    return(NULL);
}
/* \\\ */

/* /// "usbReleaseDeviceBinding()" */
void usbReleaseDeviceBinding(struct NepHidBase *nh, struct NepClassHid *nch)
{
    struct Library *ps;
    STRPTR devname;

    KPRINTF(1, ("nepHidReleaseDeviceBinding(%08lx)\n", nch));
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        Forbid();
        nch->nch_ReadySignal = SIGB_SINGLE;
        nch->nch_ReadySigTask = FindTask(NULL);
        if(nch->nch_GUITask)
        {
            Signal(nch->nch_GUITask, SIGBREAKF_CTRL_C);
        }
        Permit();
        while(nch->nch_GUITask)
        {
            Wait(1L<<nch->nch_ReadySignal);
        }

        Forbid();
        if(nch->nch_Task)
        {
            Signal(nch->nch_Task, SIGBREAKF_CTRL_C);
        }
        Permit();
        while(nch->nch_Task)
        {
            Wait(1L<<nch->nch_ReadySignal);
        }
        //FreeSignal(nch->nch_ReadySignal);
        psdGetAttrs(PGA_DEVICE, nch->nch_Device, DA_ProductName, &devname, TAG_END);
        psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                       "Couldn't keep in touch with '%s'!",
                       devname);
        Forbid();
        Remove(&nch->nch_Node);
        Permit();
        psdFreeVec(nch->nch_CDC);
        psdFreeVec(nch);
        CloseLibrary(ps);
    }
}
/* \\\ */

/* /// "usbGetAttrsA()" */
AROS_LH3(LONG, usbGetAttrsA,
         AROS_LHA(ULONG, type, D0),
         AROS_LHA(APTR, usbstruct, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, nh, 5, nep)
{
    AROS_LIBFUNC_INIT

    struct TagItem *ti;
    LONG count = 0;

    KPRINTF(1, ("nepHidGetAttrsA(%ld, %08lx, %08lx)\n", type, usbstruct, tags));
    switch(type)
    {
        case UGA_CLASS:
             if((ti = FindTagItem(UCCA_Priority, tags)))
             {
                 *((SIPTR *) ti->ti_Data) = 0;
                 count++;
             }
             if((ti = FindTagItem(UCCA_Description, tags)))
             {
                 *((STRPTR *) ti->ti_Data) = "Support for eGalax Touchscreens";
                 count++;
             }
             if((ti = FindTagItem(UCCA_HasClassCfgGUI, tags)))
             {
                 *((IPTR *) ti->ti_Data) = TRUE;
                 count++;
             }
             if((ti = FindTagItem(UCCA_HasBindingCfgGUI, tags)))
             {
                 *((IPTR *) ti->ti_Data) = TRUE;
                 count++;
             }
             if((ti = FindTagItem(UCCA_AfterDOSRestart, tags)))
             {
                 *((IPTR *) ti->ti_Data) = FALSE;
                 count++;
             }
             if((ti = FindTagItem(UCCA_UsingDefaultCfg, tags)))
             {
                 *((IPTR *) ti->ti_Data) = nh->nh_DummyNCH.nch_UsingDefaultCfg;
                 count++;
             }
             break;

         case UGA_BINDING:
             if((ti = FindTagItem(UCBA_UsingDefaultCfg, tags)))
             {
                 *((IPTR *) ti->ti_Data) = ((struct NepClassHid *) usbstruct)->nch_UsingDefaultCfg;
                 count++;
             }
             break;
    }
    return(count);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "usbSetAttrsA()" */
AROS_LH3(LONG, usbSetAttrsA,
         AROS_LHA(ULONG, type, D0),
         AROS_LHA(APTR, usbstruct, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, nh, 6, nep)
{
    AROS_LIBFUNC_INIT
    return(0);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "usbDoMethodA()" */
AROS_LH2(IPTR, usbDoMethodA,
         AROS_LHA(ULONG, methodid, D0),
         AROS_LHA(IPTR *, methoddata, A1),
         LIBBASETYPEPTR, nh, 7, nep)
{
    AROS_LIBFUNC_INIT

    struct NepClassHid *nch;

    KPRINTF(10, ("Do Method %ld\n", methodid));
    switch(methodid)
    {
        case UCM_AttemptDeviceBinding:
            return((IPTR) usbAttemptDeviceBinding(nh, (struct PsdDevice *) methoddata[0]));

        case UCM_ForceDeviceBinding:
            return((IPTR) usbForceDeviceBinding(nh, (struct PsdDevice *) methoddata[0]));

        case UCM_ReleaseDeviceBinding:
            usbReleaseDeviceBinding(nh, (struct NepClassHid *) methoddata[0]);
            return(TRUE);

        case UCM_OpenCfgWindow:
            return(nOpenBindingCfgWindow(nh, &nh->nh_DummyNCH));

        case UCM_OpenBindingCfgWindow:
            return(nOpenBindingCfgWindow(nh, (struct NepClassHid *) methoddata[0]));

        case UCM_ConfigChangedEvent:
            nLoadClassConfig(nh);
            Forbid();
            nch = (struct NepClassHid *) nh->nh_Bindings.lh_Head;
            while(nch->nch_Node.ln_Succ)
            {
                nLoadBindingConfig(nch);
                nch = (struct NepClassHid *) nch->nch_Node.ln_Succ;
            }
            Permit();
            return(TRUE);

        default:
            break;
    }
    return(0);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "nLoadClassConfig()" */
BOOL nLoadClassConfig(struct NepHidBase *nh)
{
    struct NepClassHid *nch = &nh->nh_DummyNCH;
    struct Library *ps;
    struct ClsDevCfg *cdc;
    struct PsdIFFContext *pic;

    KPRINTF(10, ("Loading Class Config...\n"));
    if(!(ps = OpenLibrary("poseidon.library", 4)))
    {
        return(FALSE);
    }

    Forbid();
    /* Create default config */
    nch->nch_CDC->cdc_ChunkID = AROS_LONG2BE(MAKE_ID('E','G','T','S'));
    nch->nch_CDC->cdc_Length = AROS_LONG2BE(sizeof(struct ClsDevCfg)-8);
    nch->nch_CDC->cdc_MinX = 0;
    nch->nch_CDC->cdc_MinY = 0;
    nch->nch_CDC->cdc_MaxX = 2047;
    nch->nch_CDC->cdc_MaxY = 2047;
    nch->nch_CDC->cdc_RMBDelay = 20;
    nch->nch_CDC->cdc_RMBTolerance = 20;
    nch->nch_CDC->cdc_RMBMode = 1;
    nch->nch_UsingDefaultCfg = TRUE;
    /* try to load default config */
    pic = psdGetClsCfg(libname);
    if(pic)
    {
        cdc = psdGetCfgChunk(pic, AROS_LONG2BE(nch->nch_CDC->cdc_ChunkID));
        if(cdc)
        {
            CopyMem(((UBYTE *) cdc) + 8, ((UBYTE *) nch->nch_CDC) + 8, min(AROS_LONG2BE(cdc->cdc_Length), AROS_LONG2BE(nch->nch_CDC->cdc_Length)));
            psdFreeVec(cdc);
            nch->nch_UsingDefaultCfg = FALSE;
        }
    }
    Permit();
    CloseLibrary(ps);
    return(FALSE);
}
/* \\\ */

/* /// "nLoadBindingConfig()" */
BOOL nLoadBindingConfig(struct NepClassHid *nch)
{
    struct NepHidBase *nh = nch->nch_ClsBase;
    struct Library *ps;
    struct ClsDevCfg *cdc;
    struct PsdIFFContext *pic;

    KPRINTF(10, ("Loading Binding Config...\n"));
    if(nch->nch_GUITask)
    {
        return(FALSE);
    }
    //nLoadClassConfig(nh);
    *nch->nch_CDC = *nh->nh_DummyNCH.nch_CDC;
    nch->nch_UsingDefaultCfg = TRUE;

    if(!(ps = OpenLibrary("poseidon.library", 4)))
    {
        return(FALSE);
    }

    Forbid();
    /* Load config */
    pic = psdGetUsbDevCfg(libname, nch->nch_DevIDString, NULL);
    if(pic)
    {
        cdc = psdGetCfgChunk(pic, AROS_LONG2BE(nch->nch_CDC->cdc_ChunkID));
        if(cdc)
        {
            CopyMem(((UBYTE *) cdc) + 8, ((UBYTE *) nch->nch_CDC) + 8, min(AROS_LONG2BE(cdc->cdc_Length), AROS_LONG2BE(nch->nch_CDC->cdc_Length)));
            psdFreeVec(cdc);
            nch->nch_UsingDefaultCfg = FALSE;
        }
    }
    Permit();
    CloseLibrary(ps);
    return(FALSE);
}
/* \\\ */

/* /// "nOpenBindingCfgWindow()" */
LONG nOpenBindingCfgWindow(struct NepHidBase *nh, struct NepClassHid *nch)
{
    struct Library *ps;
    KPRINTF(10, ("Opening GUI...\n"));
    if(!(ps = OpenLibrary("poseidon.library", 4)))
    {
        return(FALSE);
    }
    Forbid();
    if(!nch->nch_GUITask)
    {
        if((nch->nch_GUITask = psdSpawnSubTask(MOD_NAME_STRING " GUI", nGUITask, nch)))
        {
            Permit();
            CloseLibrary(ps);
            return(TRUE);
        }
    }
    Permit();
    CloseLibrary(ps);
    return(FALSE);
}
/* \\\ */

/**************************************************************************/

#undef  ps
#define ps nch->nch_Base
#undef TimerBase
#define TimerBase nch->nch_TimerBase
#undef IntuitionBase
#define IntuitionBase nch->nch_HIntBase

/* /// "nHidTask()" */
AROS_UFH0(void, nHidTask)
{
    AROS_USERFUNC_INIT

    struct NepClassHid *nch;
    struct PsdPipe *pp;
    ULONG sigmask;
    ULONG sigs;
    UWORD iecode;
    UWORD qualifier;
    UWORD buts;
    UWORD oldbuts = 0;
    UBYTE *buf;
    UBYTE *bufptr;
    LONG ioerr;
    ULONG *tagptr;
    ULONG xpos,ypos;
    ULONG xrange, yrange;
    ULONG tmp;
    ULONG packetpos = 0;
    UBYTE packet[5] = { 0, 0, 0, 0, 0 };
    ULONG len;
    LONG distx;
    LONG disty;
    BOOL changed;
    struct timeval tmptime;

    if((nch = nAllocHid()))
    {
        Forbid();
        if(nch->nch_ReadySigTask)
        {
            Signal(nch->nch_ReadySigTask, 1L<<nch->nch_ReadySignal);
        }
        Permit();
        sigmask = (1L<<nch->nch_TaskMsgPort->mp_SigBit)|SIGBREAKF_CTRL_C;
        buf = nch->nch_EP1Buf;
        psdSendPipe(nch->nch_EP1Pipe, buf, nch->nch_EP1PktSize);
        do
        {
            sigs = Wait(sigmask);
            while((pp = (struct PsdPipe *) GetMsg(nch->nch_TaskMsgPort)))
            {
                if(pp == nch->nch_EP1Pipe)
                {
                    ioerr = psdGetPipeError(pp);
                    if(ioerr == UHIOERR_RUNTPACKET)
                    {
                        KPRINTF(1, ("This should never happen. Something is wrong with the lowlevel driver!"));
                        ioerr = 0;
                    }
                    if(!ioerr)
                    {
                        KPRINTF(1, ("Data: %08lx %08lx\n", (*(ULONG *) buf), ((ULONG *) buf)[1]));
                        len = psdGetPipeActual(pp);
                        bufptr = buf;
                        if(len)
                        {
                            do
                            {
                                if(*bufptr & 0x80)
                                {
                                    packet[0] = *bufptr;
                                    packetpos = 1;
                                } else {
                                    if(packetpos)
                                    {
                                        packet[packetpos] = *bufptr;
                                        packetpos++;
                                    }
                                }
                                if(packetpos == 5)
                                {
                                    packetpos = 0;
                                    qualifier = 0;
                                    buts = packet[0] & 1;

                                    xpos = (packet[1]<<7)|packet[2];
                                    ypos = 2047 - ((packet[3]<<7)|packet[4]);
                                    KPRINTF(2, ("Raw Coords %ld, %ld, %02lx, %ld\n", xpos, ypos, packet[0], nch->nch_RMBState));
                                    switch(nch->nch_CDC->cdc_Rotate)
                                    {
                                        case 1: // 90°
                                            tmp = ypos;
                                            ypos = xpos;
                                            xpos = 2047 - tmp;
                                            break;

                                        case 2: // 180°
                                            ypos = 2047 - ypos;
                                            xpos = 2047 - xpos;
                                            break;

                                        case 3: // 270°
                                            tmp = xpos;
                                            xpos = ypos;
                                            ypos = 2047 - tmp;
                                            break;
                                    }
                                    if(nch->nch_CDC->cdc_Mirror & 1)
                                    {
                                        xpos = 2047 - xpos;
                                    }
                                    if(nch->nch_CDC->cdc_Mirror & 2)
                                    {
                                        ypos = 2047 - ypos;
                                    }

                                    if(nch->nch_TrackDims)
                                    {
                                        buts = 0;
                                        changed = FALSE;
                                        if(xpos < nch->nch_CDC->cdc_MinX)
                                        {
                                            nch->nch_CDC->cdc_MinX = xpos;
                                            changed = TRUE;
                                        }
                                        if(xpos > nch->nch_CDC->cdc_MaxX)
                                        {
                                            nch->nch_CDC->cdc_MaxX = xpos;
                                            changed = TRUE;
                                        }
                                        if(ypos < nch->nch_CDC->cdc_MinY)
                                        {
                                            nch->nch_CDC->cdc_MinY = ypos;
                                            changed = TRUE;
                                        }
                                        if(ypos > nch->nch_CDC->cdc_MaxY)
                                        {
                                            nch->nch_CDC->cdc_MaxY = ypos;
                                            changed = TRUE;
                                        }
                                        Forbid();
                                        if(changed && nch->nch_GUITask)
                                        {
                                            Signal(nch->nch_GUITask, 1UL<<nch->nch_TrackingSignal);
                                        }
                                        Permit();
                                    } else {
                                        if(xpos < nch->nch_CDC->cdc_MinX) xpos = nch->nch_CDC->cdc_MinX;
                                        if(xpos > nch->nch_CDC->cdc_MaxX) xpos = nch->nch_CDC->cdc_MaxX;
                                        if(ypos < nch->nch_CDC->cdc_MinY) ypos = nch->nch_CDC->cdc_MinY;
                                        if(ypos > nch->nch_CDC->cdc_MaxY) ypos = nch->nch_CDC->cdc_MaxY;
                                    }
                                    xrange = nch->nch_CDC->cdc_MaxX - nch->nch_CDC->cdc_MinX + 1;
                                    yrange = nch->nch_CDC->cdc_MaxY - nch->nch_CDC->cdc_MinY + 1;

                                    xpos -= nch->nch_CDC->cdc_MinX;
                                    ypos -= nch->nch_CDC->cdc_MinY;

                                    if((buts & 1) && (nch->nch_CDC->cdc_RMBMode > 0))
                                    {
                                        if(nch->nch_RMBState == RMB_IDLE)
                                        {
                                            nch->nch_TouchSpotX = xpos;
                                            nch->nch_TouchSpotY = ypos;
                                            GetSysTime(&nch->nch_TouchTime);
                                            tmptime.tv_secs = nch->nch_CDC->cdc_RMBDelay / 10;
                                            tmptime.tv_micro = (nch->nch_CDC->cdc_RMBDelay % 10) * 100000;
                                            AddTime(&nch->nch_TouchTime, &tmptime);
                                            nch->nch_RMBState = RMB_LMBDOWN;
                                        }
                                        distx = xpos - nch->nch_TouchSpotX;
                                        distx *= distx;
                                        disty = ypos - nch->nch_TouchSpotY;
                                        disty *= disty;
                                        if(nch->nch_RMBState == RMB_LMBDOWN)
                                        {
                                            if((distx + disty > (nch->nch_CDC->cdc_RMBTolerance * nch->nch_CDC->cdc_RMBTolerance)))
                                            {
                                                // moved out of tolerance
                                                nch->nch_RMBState = RMB_MOVEDTOOFAR;
                                            } else {
                                                GetSysTime(&tmptime);
                                                if(CmpTime(&nch->nch_TouchTime, &tmptime) >= 0)
                                                {
                                                    // activate RMB, but release LMB first
                                                    nch->nch_RMBState = RMB_RMBDOWN;
                                                    buts = 0;
                                                }
                                            }
                                        }
                                        else if(nch->nch_RMBState == RMB_RMBDOWN_LMBUP)
                                        {
                                            if(nch->nch_CDC->cdc_RMBMode == 2)
                                            {
                                                nch->nch_RMBState = RMB_RMBDOWN_LAST;
                                            }
                                            else if(nch->nch_CDC->cdc_RMBMode == 3)
                                            {
                                                GetSysTime(&tmptime);
                                                if(DoubleClick(nch->nch_TouchTime.tv_secs, nch->nch_TouchTime.tv_micro, tmptime.tv_secs, tmptime.tv_micro))
                                                {
                                                    nch->nch_RMBState = RMB_RMBDOWN_LAST;
                                                } else {
                                                    nch->nch_RMBState = RMB_RMBDOWN;
                                                    GetSysTime(&nch->nch_TouchTime);
                                                }
                                            }
                                            buts = 2;
                                        }
                                        else if(nch->nch_RMBState >= RMB_RMBDOWN)
                                        {
                                            buts = 2;
                                        }
                                    } else {
                                        switch(nch->nch_CDC->cdc_RMBMode)
                                        {
                                            case 2: // 1-click sticky
                                            case 3: // double click sticky
                                                if(nch->nch_RMBState == RMB_RMBDOWN)
                                                {
                                                    nch->nch_RMBState = RMB_RMBDOWN_LMBUP;
                                                }
                                                else if((nch->nch_RMBState == RMB_RMBDOWN_LAST) || (nch->nch_RMBState < RMB_RMBDOWN))
                                                {
                                                    nch->nch_RMBState = RMB_IDLE;
                                                    break;
                                                }
                                                buts = 2;
                                                break;

                                            default:
                                                nch->nch_RMBState = RMB_IDLE;
                                        }
                                    }

                                    iecode = IECODE_NOBUTTON;
                                    if(buts & 1)
                                    {
                                        qualifier |= IEQUALIFIER_LEFTBUTTON;
                                    }
                                    if(buts & 2)
                                    {
                                        qualifier |= IEQUALIFIER_RBUTTON;
                                    }
                                    if((buts^oldbuts) & 1)
                                    {
                                        iecode = (buts & 1) ? IECODE_LBUTTON : IECODE_LBUTTON|IECODE_UP_PREFIX;
                                    }
                                    else if((buts^oldbuts) & 2)
                                    {
                                        iecode = (buts & 2) ? IECODE_RBUTTON : IECODE_RBUTTON|IECODE_UP_PREFIX;
                                    }
                                    KPRINTF(1, ("Translated Coords %ld, %ld\n", xpos, ypos));

                                    tagptr = nch->nch_TabTags;
                                    nch->nch_FakeEvent.ie_EventAddress = &nch->nch_TabletEvent;
                                    nch->nch_TabletEvent.ient_RangeX = xrange;
                                    nch->nch_TabletEvent.ient_RangeY = yrange;
                                    nch->nch_TabletEvent.ient_TabletX = xpos;
                                    nch->nch_TabletEvent.ient_TabletY = ypos;
                                    nch->nch_TabletEvent.ient_TagList = (struct TagItem *) tagptr;
                                    *tagptr++ = TABLETA_ButtonBits;
                                    *tagptr++ = buts;
                                    *tagptr = TAG_END;
                                    nch->nch_FakeEvent.ie_Class = IECLASS_NEWPOINTERPOS;
                                    nch->nch_FakeEvent.ie_SubClass = IESUBCLASS_NEWTABLET;
                                    nch->nch_FakeEvent.ie_Code = IECODE_NOBUTTON;
                                    nch->nch_FakeEvent.ie_NextEvent = NULL;
                                    nch->nch_FakeEvent.ie_Qualifier = qualifier;
                                    nch->nch_InpIOReq->io_Data = &nch->nch_FakeEvent;
                                    nch->nch_InpIOReq->io_Length = sizeof(struct InputEvent);
                                    nch->nch_InpIOReq->io_Command = IND_WRITEEVENT;
                                    DoIO((struct IORequest *) nch->nch_InpIOReq);

                                    if(oldbuts != buts)
                                    {
                                        nch->nch_FakeEvent.ie_X = 0;
                                        nch->nch_FakeEvent.ie_Y = 0;
                                        nch->nch_FakeEvent.ie_Class = IECLASS_RAWMOUSE;
                                        nch->nch_FakeEvent.ie_SubClass = 0;
                                        nch->nch_FakeEvent.ie_Code = iecode;
                                        nch->nch_FakeEvent.ie_NextEvent = NULL;
                                        nch->nch_FakeEvent.ie_Qualifier = qualifier|IEQUALIFIER_RELATIVEMOUSE;
                                        nch->nch_InpIOReq->io_Data = &nch->nch_FakeEvent;
                                        nch->nch_InpIOReq->io_Length = sizeof(struct InputEvent);
                                        nch->nch_InpIOReq->io_Command = IND_WRITEEVENT;
                                        DoIO((struct IORequest *) nch->nch_InpIOReq);
                                        oldbuts = buts;
                                    }
                                }
                                bufptr++;
                            } while(--len);
                        }
                    } else {
                        KPRINTF(1, ("Int Pipe failed %ld\n", ioerr));
                        psdDelayMS(20);
                    }
                    psdSendPipe(nch->nch_EP1Pipe, buf, nch->nch_EP1PktSize);
                    break;
                }
            }
        } while(!(sigs & SIGBREAKF_CTRL_C));
        KPRINTF(20, ("Going down the river!\n"));
        psdAbortPipe(nch->nch_EP1Pipe);
        psdWaitPipe(nch->nch_EP1Pipe);
        nFreeHid(nch);
    }
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nAllocHid()" */
struct NepClassHid * nAllocHid(void)
{
    struct Task *thistask;
    struct NepClassHid *nch;

    thistask = FindTask(NULL);
    nch = thistask->tc_UserData;
    do
    {
        if(!(nch->nch_Base = OpenLibrary("poseidon.library", 4)))
        {
            Alert(AG_OpenLib);
            break;
        }
        if(!(nch->nch_HIntBase = OpenLibrary("intuition.library", 39)))
        {
            Alert(AG_OpenLib|AO_Intuition);
            break;
        }
        nch->nch_Interface = psdFindInterface(nch->nch_Device, NULL, TAG_END);
        if(!nch->nch_Interface)
        {
            KPRINTF(1, ("Ooops!?! No interfaces defined?\n"));
            break;
        }

        psdGetAttrs(PGA_INTERFACE, nch->nch_Interface,
                    IFA_InterfaceNum, &nch->nch_IfNum,
                    TAG_END);

        nch->nch_EP1 = psdFindEndpoint(nch->nch_Interface, NULL,
                                       EA_IsIn, TRUE,
                                       EA_TransferType, USEAF_INTERRUPT,
                                       TAG_END);
        if(!nch->nch_EP1)
        {
            KPRINTF(1, ("Ooops!?! No Endpoints defined?\n"));
            break;
        }
        psdGetAttrs(PGA_ENDPOINT, nch->nch_EP1,
                    EA_MaxPktSize, &nch->nch_EP1PktSize,
                    TAG_END);

        nch->nch_TimerIOReq.tr_node.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
        nch->nch_TimerIOReq.tr_node.io_Message.mn_ReplyPort    = NULL;
        nch->nch_TimerIOReq.tr_node.io_Message.mn_Length       = sizeof(struct timerequest);
        if(OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *) &nch->nch_TimerIOReq, 0))
        {
            Alert(AG_OpenDev|AO_TimerDev);
            break;
        }
        nch->nch_TimerBase = (struct Library *) nch->nch_TimerIOReq.tr_node.io_Device;

        if((nch->nch_InpMsgPort = CreateMsgPort()))
        {
            if((nch->nch_InpIOReq = (struct IOStdReq *) CreateIORequest(nch->nch_InpMsgPort, sizeof(struct IOStdReq))))
            {
                if(!OpenDevice("input.device", 0, (struct IORequest *) nch->nch_InpIOReq, 0))
                {
                    if((nch->nch_TaskMsgPort = CreateMsgPort()))
                    {
                        if((nch->nch_EP0Pipe = psdAllocPipe(nch->nch_Device, nch->nch_TaskMsgPort, NULL)))
                        {
                            if((nch->nch_EP1Pipe = psdAllocPipe(nch->nch_Device, nch->nch_TaskMsgPort, nch->nch_EP1)))
                            {
                                if((nch->nch_EP1Buf = psdAllocVec(nch->nch_EP1PktSize)))
                                {
                                    psdSetAttrs(PGA_PIPE, nch->nch_EP1Pipe,
                                                PPA_AllowRuntPackets, TRUE,
                                                TAG_END);
                                    nch->nch_Task = thistask;
                                    return(nch);
                                }
                                psdFreePipe(nch->nch_EP1Pipe);
                            }
                            psdFreePipe(nch->nch_EP0Pipe);
                        }
                        DeleteMsgPort(nch->nch_TaskMsgPort);
                    }
                    CloseDevice((struct IORequest *) nch->nch_InpIOReq);
                }
                DeleteIORequest((struct IORequest *) nch->nch_InpIOReq);
            }
            DeleteMsgPort(nch->nch_InpMsgPort);
        }
    } while(FALSE);
    CloseLibrary(nch->nch_HIntBase);
    CloseLibrary(nch->nch_Base);
    Forbid();
    nch->nch_Task = NULL;
    if(nch->nch_ReadySigTask)
    {
        Signal(nch->nch_ReadySigTask, 1L<<nch->nch_ReadySignal);
    }
    return(NULL);
}
/* \\\ */

/* /// "nFreeHid()" */
void nFreeHid(struct NepClassHid *nch)
{
    psdFreeVec(nch->nch_EP1Buf);
    psdFreePipe(nch->nch_EP1Pipe);
    psdFreePipe(nch->nch_EP0Pipe);
    DeleteMsgPort(nch->nch_TaskMsgPort);
    CloseDevice((struct IORequest *) nch->nch_InpIOReq);
    DeleteIORequest((struct IORequest *) nch->nch_InpIOReq);
    DeleteMsgPort(nch->nch_InpMsgPort);
    CloseDevice((struct IORequest *) &nch->nch_TimerIOReq);
    CloseLibrary(nch->nch_HIntBase);
    CloseLibrary(nch->nch_Base);
    Forbid();
    nch->nch_Task = NULL;
    if(nch->nch_ReadySigTask)
    {
        Signal(nch->nch_ReadySigTask, 1L<<nch->nch_ReadySignal);
    }
}
/* \\\ */

/**************************************************************************/

static const char *MirrorStrings[] =
{
    "Off",
    "Horizontal",
    "Vertical",
    "Both",
    NULL
};

static const char *RotateStrings[] =
{
    "Off",
    "90°",
    "180°",
    "270°",
    NULL
};

static const char *RMBModeStrings[] =
{
    "Off",
    "RMB until released",
    "Next click releases RMB",
    "Double click releases RMB",
    NULL
};

/* /// "nGUITask()" */
AROS_UFH0(void, nGUITask)
{
    AROS_USERFUNC_INIT

    struct Task *thistask;
    struct NepHidBase *nh;
    struct NepClassHid *nch;
    APTR pic;

    thistask = FindTask(NULL);
#undef ps
#define ps nch->nch_PsdBase
#undef IntuitionBase
#define IntuitionBase nch->nch_IntBase
#undef MUIMasterBase
#define MUIMasterBase nch->nch_MUIBase

    nch = thistask->tc_UserData;
    nh = nch->nch_ClsBase;

    ++nh->nh_Library.lib_OpenCnt;
    if(!(MUIMasterBase = OpenLibrary(MUIMASTER_NAME, MUIMASTER_VMIN)))
    {
        KPRINTF(10, ("Couldn't open muimaster.library.\n"));
        nGUITaskCleanup(nch);
        return;
    }

    if(!(IntuitionBase = OpenLibrary("intuition.library", 39)))
    {
        KPRINTF(10, ("Couldn't open intuition.library.\n"));
        nGUITaskCleanup(nch);
        return;
    }
    if(!(ps = OpenLibrary("poseidon.library", 4)))
    {
        KPRINTF(10, ("Couldn't open poseidon.library.\n"));
        nGUITaskCleanup(nch);
        return;
    }

    nch->nch_App = ApplicationObject,
        MUIA_Application_Title      , libname,
        MUIA_Application_Version    , VERSION_STRING,
        MUIA_Application_Copyright  , "©2004-2009 Chris Hodges",
        MUIA_Application_Author     , "Chris Hodges <chrisly@platon42.de>",
        MUIA_Application_Description, "Settings for the egalaxtouch.class",
        MUIA_Application_Base       , "EGALAXTOUCH",
        MUIA_Application_HelpFile   , "HELP:Poseidon.guide",
        MUIA_Application_Menustrip  , MenustripObject,
            Child, MenuObjectT("Project"),
                Child, nch->nch_AboutMI = MenuitemObject,
                    MUIA_Menuitem_Title, "About...",
                    MUIA_Menuitem_Shortcut, "?",
                    End,
                End,
            Child, MenuObjectT("Settings"),
                Child, nch->nch_UseMI = MenuitemObject,
                    MUIA_Menuitem_Title, "Save",
                    MUIA_Menuitem_Shortcut, "S",
                    End,
                Child, nch->nch_SetDefaultMI = MenuitemObject,
                    MUIA_Menuitem_Title, "Save as Default",
                    MUIA_Menuitem_Shortcut, "D",
                    End,
                Child, MenuitemObject,
                    MUIA_Menuitem_Title, NM_BARLABEL,
                    End,
                Child, nch->nch_MUIPrefsMI = MenuitemObject,
                    MUIA_Menuitem_Title, "MUI Settings",
                    MUIA_Menuitem_Shortcut, "M",
                    End,
                End,
            End,

        SubWindow, nch->nch_MainWindow = WindowObject,
            MUIA_Window_ID   , MAKE_ID('M','A','I','N'),
            MUIA_Window_Title, libname,
            MUIA_HelpNode, libname,

            WindowContents, VGroup,
                Child, VGroup, GroupFrameT(nch->nch_Interface ? "Device Settings" : "Default Device Settings"),
                    Child, VGroup, GroupFrameT("Touchscreen Calibration"),
                        Child, ColGroup(4),
                            Child, Label((ULONG) "MinX:"),
                            Child, nch->nch_MinXObj = SliderObject, SliderFrame,
                                MUIA_CycleChain, 1,
                                MUIA_Numeric_Min, 0,
                                MUIA_Numeric_Max, 1919,
                                MUIA_Numeric_Value, nch->nch_CDC->cdc_MinX,
                                End,
                            Child, Label((ULONG) "MaxX:"),
                            Child, nch->nch_MaxXObj = SliderObject, SliderFrame,
                                MUIA_CycleChain, 1,
                                MUIA_Numeric_Min, 128,
                                MUIA_Numeric_Max, 2047,
                                MUIA_Numeric_Value, nch->nch_CDC->cdc_MaxX,
                                End,
                            Child, Label((ULONG) "MinY:"),
                            Child, nch->nch_MinYObj = SliderObject, SliderFrame,
                                MUIA_CycleChain, 1,
                                MUIA_Numeric_Min, 0,
                                MUIA_Numeric_Max, 1919,
                                MUIA_Numeric_Value, nch->nch_CDC->cdc_MinY,
                                End,
                            Child, Label((ULONG) "MaxY:"),
                            Child, nch->nch_MaxYObj = SliderObject, SliderFrame,
                                MUIA_CycleChain, 1,
                                MUIA_Numeric_Min, 128,
                                MUIA_Numeric_Max, 2047,
                                MUIA_Numeric_Value, nch->nch_CDC->cdc_MaxY,
                                End,
                            Child, Label((ULONG) "Mirror:"),
                            Child, nch->nch_MirrorObj = CycleObject,
                                MUIA_Cycle_Entries, MirrorStrings,
                                MUIA_Cycle_Active, nch->nch_CDC->cdc_Mirror,
                                End,
                            Child, Label((ULONG) "Rotate:"),
                            Child, nch->nch_RotateObj = CycleObject,
                                MUIA_Cycle_Entries, RotateStrings,
                                MUIA_Cycle_Active, nch->nch_CDC->cdc_Rotate,
                                End,
                            End,
                        Child, HGroup,
                            Child, nch->nch_TrackDimsObj = ImageObject, ImageButtonFrame,
                                MUIA_Background, MUII_ButtonBack,
                                MUIA_CycleChain, 1,
                                MUIA_InputMode, MUIV_InputMode_Toggle,
                                MUIA_Image_Spec, MUII_CheckMark,
                                MUIA_Image_FreeVert, TRUE,
                                MUIA_Selected, FALSE,
                                MUIA_ShowSelState, FALSE,
                                End,
                            Child, Label((ULONG) "Track Dimensions by input (disables LMB/RMB)"),
                            Child, HSpace(0),
                            End,
                        End,
                    Child, ColGroup(2), GroupFrameT("Right Mouse Button Emulation"),
                        Child, Label((ULONG) "Mode:"),
                        Child, nch->nch_RMBModeObj = CycleObject,
                            MUIA_Cycle_Entries, RMBModeStrings,
                            MUIA_Cycle_Active, nch->nch_CDC->cdc_RMBMode,
                            End,
                        Child, Label((ULONG) "Activation Delay:"),
                        Child, nch->nch_RMBDelayObj = SliderObject, SliderFrame,
                            MUIA_CycleChain, 1,
                            MUIA_Numeric_Min, 5,
                            MUIA_Numeric_Max, 50,
                            MUIA_Numeric_Value, nch->nch_CDC->cdc_RMBDelay,
                            MUIA_Numeric_Format, "%ld00ms",
                            End,
                        Child, Label((ULONG) "Allowed Jitter:"),
                        Child, nch->nch_RMBToleranceObj = SliderObject, SliderFrame,
                            MUIA_CycleChain, 1,
                            MUIA_Numeric_Min, 0,
                            MUIA_Numeric_Max, 100,
                            MUIA_Numeric_Value, nch->nch_CDC->cdc_RMBTolerance,
                            MUIA_Numeric_Format, "%ld units",
                            End,
                        End,
                    End,
                Child, VSpace(0),
                Child, HGroup,
                    MUIA_Group_SameWidth, TRUE,
                    Child, nch->nch_UseObj = TextObject, ButtonFrame,
                        MUIA_ShowMe, nch->nch_Interface,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_Contents, "\33c Save ",
                        End,
                    Child, nch->nch_SetDefaultObj = TextObject, ButtonFrame,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_Contents, nch->nch_Interface ? "\33c Save as Default " : "\33c Save Defaults ",
                        End,
                    Child, nch->nch_CloseObj = TextObject, ButtonFrame,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_Contents, "\33c Use ",
                        End,
                    End,
                End,
            End,
        End;

    if(!nch->nch_App)
    {
        KPRINTF(10, ("Couldn't create application\n"));
        nGUITaskCleanup(nch);
        return;
    }

    DoMethod(nch->nch_MainWindow, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
             nch->nch_App, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    DoMethod(nch->nch_UseObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nch->nch_App, 2, MUIM_Application_ReturnID, ID_STORE_CONFIG);
    DoMethod(nch->nch_SetDefaultObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nch->nch_App, 2, MUIM_Application_ReturnID, ID_DEF_CONFIG);
    DoMethod(nch->nch_CloseObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nch->nch_App, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    DoMethod(nch->nch_MinXObj, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
             nch->nch_App, 2, MUIM_Application_ReturnID, ID_UPDATE_LIMITSMIN);
    DoMethod(nch->nch_MaxXObj, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
             nch->nch_App, 2, MUIM_Application_ReturnID, ID_UPDATE_LIMITSMAX);
    DoMethod(nch->nch_MinYObj, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
             nch->nch_App, 2, MUIM_Application_ReturnID, ID_UPDATE_LIMITSMIN);
    DoMethod(nch->nch_MaxYObj, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
             nch->nch_App, 2, MUIM_Application_ReturnID, ID_UPDATE_LIMITSMAX);

    DoMethod(nch->nch_MirrorObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nch->nch_App, 2, MUIM_Application_ReturnID, ID_UPDATE_MIRRORROT);
    DoMethod(nch->nch_RotateObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nch->nch_App, 2, MUIM_Application_ReturnID, ID_UPDATE_MIRRORROT);
    DoMethod(nch->nch_TrackDimsObj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
             nch->nch_App, 2, MUIM_Application_ReturnID, ID_UPDATE_MIRRORROT);

    DoMethod(nch->nch_RMBModeObj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
             nch->nch_App, 2, MUIM_Application_ReturnID, ID_UPDATE_MIRRORROT);
    DoMethod(nch->nch_RMBDelayObj, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
             nch->nch_App, 2, MUIM_Application_ReturnID, ID_UPDATE_MIRRORROT);
    DoMethod(nch->nch_RMBToleranceObj, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
             nch->nch_App, 2, MUIM_Application_ReturnID, ID_UPDATE_MIRRORROT);

    DoMethod(nch->nch_AboutMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nch->nch_App, 2, MUIM_Application_ReturnID, ID_ABOUT);
    DoMethod(nch->nch_UseMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nch->nch_App, 2, MUIM_Application_ReturnID, ID_STORE_CONFIG);
    DoMethod(nch->nch_SetDefaultMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nch->nch_App, 2, MUIM_Application_ReturnID, ID_DEF_CONFIG);
    DoMethod(nch->nch_MUIPrefsMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nch->nch_App, 2, MUIM_Application_OpenConfigWindow, 0);
    {
        ULONG isopen;
        ULONG iconify;
        ULONG sigs;
        ULONG sigmask;
        LONG retid;

        get(nch->nch_App, MUIA_Application_Iconified, &iconify);
        set(nch->nch_MainWindow, MUIA_Window_Open, TRUE);
        get(nch->nch_MainWindow, MUIA_Window_Open, &isopen);
        if(!(isopen || iconify))
        {
            nGUITaskCleanup(nch);
            return;
        }
        nch->nch_TrackingSignal = AllocSignal(-1);
        sigmask = (1<<nch->nch_TrackingSignal);
        do
        {
            retid = DoMethod(nch->nch_App, MUIM_Application_NewInput, &sigs);
            switch(retid)
            {
                case ID_UPDATE_LIMITSMIN:
                    get(nch->nch_MinXObj, MUIA_Numeric_Value, &nch->nch_CDC->cdc_MinX);
                    if(nch->nch_CDC->cdc_MinX + 128 >= nch->nch_CDC->cdc_MaxX)
                    {
                        set(nch->nch_MaxXObj, MUIA_Numeric_Value, nch->nch_CDC->cdc_MinX + 128);
                    }
                    get(nch->nch_MinYObj, MUIA_Numeric_Value, &nch->nch_CDC->cdc_MinY);
                    if(nch->nch_CDC->cdc_MinY + 128 >= nch->nch_CDC->cdc_MaxY)
                    {
                        set(nch->nch_MaxYObj, MUIA_Numeric_Value, nch->nch_CDC->cdc_MinY + 128);
                    }
                    break;

                case ID_UPDATE_LIMITSMAX:
                    get(nch->nch_MaxXObj, MUIA_Numeric_Value, &nch->nch_CDC->cdc_MaxX);
                    if(nch->nch_CDC->cdc_MinX + 128 >= nch->nch_CDC->cdc_MaxX)
                    {
                        set(nch->nch_MinXObj, MUIA_Numeric_Value, nch->nch_CDC->cdc_MaxX - 128);
                    }
                    get(nch->nch_MaxYObj, MUIA_Numeric_Value, &nch->nch_CDC->cdc_MaxY);
                    if(nch->nch_CDC->cdc_MinY + 128 >= nch->nch_CDC->cdc_MaxY)
                    {
                        set(nch->nch_MinYObj, MUIA_Numeric_Value, nch->nch_CDC->cdc_MaxY - 128);
                    }
                    break;

                case ID_UPDATE_MIRRORROT:
                {
                    ULONG trackdims = 0;
                    get(nch->nch_MirrorObj, MUIA_Cycle_Active, &nch->nch_CDC->cdc_Mirror);
                    get(nch->nch_RotateObj, MUIA_Cycle_Active, &nch->nch_CDC->cdc_Rotate);
                    get(nch->nch_RMBModeObj, MUIA_Cycle_Active, &nch->nch_CDC->cdc_RMBMode);
                    get(nch->nch_RMBDelayObj, MUIA_Numeric_Value, &nch->nch_CDC->cdc_RMBDelay);
                    get(nch->nch_RMBToleranceObj, MUIA_Numeric_Value, &nch->nch_CDC->cdc_RMBTolerance);
                    get(nch->nch_TrackDimsObj, MUIA_Selected, &trackdims);
                    if(trackdims)
                    {
                        if(!nch->nch_TrackDims)
                        {
                            nch->nch_CDC->cdc_MinX = 1024-64;
                            nch->nch_CDC->cdc_MinY = 1024-64;
                            nch->nch_CDC->cdc_MaxX = 1024+64;
                            nch->nch_CDC->cdc_MaxY = 1024+64;
                            set(nch->nch_MinXObj, MUIA_Disabled, TRUE);
                            set(nch->nch_MaxXObj, MUIA_Disabled, TRUE);
                            set(nch->nch_MinYObj, MUIA_Disabled, TRUE);
                            set(nch->nch_MaxYObj, MUIA_Disabled, TRUE);
                        }
                        nch->nch_TrackDims = TRUE;
                    } else {
                        if(nch->nch_TrackDims)
                        {
                            set(nch->nch_MinXObj, MUIA_Disabled, FALSE);
                            set(nch->nch_MaxXObj, MUIA_Disabled, FALSE);
                            set(nch->nch_MinYObj, MUIA_Disabled, FALSE);
                            set(nch->nch_MaxYObj, MUIA_Disabled, FALSE);
                        }
                        nch->nch_TrackDims = FALSE;
                    }
                    break;
                }

                case ID_DEF_CONFIG:
                case ID_STORE_CONFIG:
                case MUIV_Application_ReturnID_Quit:
                    if(retid == ID_DEF_CONFIG)
                    {
                        pic = psdGetClsCfg(libname);
                        if(!pic)
                        {
                            psdSetClsCfg(libname, NULL);
                            pic = psdGetClsCfg(libname);
                        }
                        if(pic)
                        {
                            if(psdAddCfgEntry(pic, nch->nch_CDC))
                            {
                                psdSaveCfgToDisk(NULL, FALSE);
                            }
                        }
                    }
                    if(nch->nch_Device)
                    {
                        pic = psdGetUsbDevCfg(libname, nch->nch_DevIDString, NULL);
                        if(!pic)
                        {
                            psdSetUsbDevCfg(libname, nch->nch_DevIDString, NULL, NULL);
                            pic = psdGetUsbDevCfg(libname, nch->nch_DevIDString, NULL);
                        }
                        if(pic)
                        {
                            if(psdAddCfgEntry(pic, nch->nch_CDC))
                            {
                                if(retid != MUIV_Application_ReturnID_Quit)
                                {
                                    psdSaveCfgToDisk(NULL, FALSE);
                                }
                                retid = MUIV_Application_ReturnID_Quit;
                            }
                        }
                    } else {
                        retid = MUIV_Application_ReturnID_Quit;
                    }
                    break;

                case ID_ABOUT:
                    MUI_RequestA(nch->nch_App, nch->nch_MainWindow, 0, NULL, "Blimey!", VERSION_STRING, NULL);
                    break;
            }
            if(nch->nch_TrackDims)
            {
                nnset(nch->nch_MinXObj, MUIA_Numeric_Value, nch->nch_CDC->cdc_MinX);
                nnset(nch->nch_MaxXObj, MUIA_Numeric_Value, nch->nch_CDC->cdc_MaxX);
                nnset(nch->nch_MinYObj, MUIA_Numeric_Value, nch->nch_CDC->cdc_MinY);
                nnset(nch->nch_MaxYObj, MUIA_Numeric_Value, nch->nch_CDC->cdc_MaxY);
            }
            if(retid == MUIV_Application_ReturnID_Quit)
            {
                break;
            }
            if(sigs)
            {
                sigs = Wait(sigs | sigmask | SIGBREAKF_CTRL_C);
                if(sigs & SIGBREAKF_CTRL_C)
                {
                    break;
                }
            }
        } while(TRUE);
        FreeSignal(nch->nch_TrackingSignal);
        nch->nch_TrackingSignal = -1;
        nch->nch_TrackDims = FALSE;
        set(nch->nch_MainWindow, MUIA_Window_Open, FALSE);
    }
    nGUITaskCleanup(nch);
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nGUITaskCleanup()" */
void nGUITaskCleanup(struct NepClassHid *nch)
{
    if(nch->nch_App)
    {
        MUI_DisposeObject(nch->nch_App);
        nch->nch_App = NULL;
    }
    if(MUIMasterBase)
    {
        CloseLibrary(MUIMasterBase);
        MUIMasterBase = NULL;
    }
    if(IntuitionBase)
    {
        CloseLibrary(IntuitionBase);
        IntuitionBase = NULL;
    }
    if(ps)
    {
        CloseLibrary(ps);
        ps = NULL;
    }
    Forbid();
    nch->nch_GUIBinding = NULL;
    nch->nch_GUITask = NULL;
    if(nch->nch_ReadySigTask)
    {
        Signal(nch->nch_ReadySigTask, 1L<<nch->nch_ReadySignal);
    }
    --nch->nch_ClsBase->nh_Library.lib_OpenCnt;
}
/* \\\ */

