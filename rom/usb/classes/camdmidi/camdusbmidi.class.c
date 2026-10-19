/*
 *----------------------------------------------------------------------------
 *                         camdusbmidi class for poseidon
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include <aros/isoascii.h>

#include "debug.h"

#include "camdusbmidi.class.h"

#if !defined(__mc68000__)
extern const void *_binary_poseidonusb_start;
extern const void *_binary_poseidonusb_size;
#endif

/* /// "Lib Stuff" */
static const STRPTR libname = MOD_NAME_STRING;

static int libInit(LIBBASETYPEPTR nh)
{
    KPRINTF(10, ("libInit nh: 0x%08lx SysBase: 0x%08lx\n", nh, SysBase));

    nh->nh_UtilityBase = OpenLibrary("utility.library", 39);

#define	UtilityBase	nh->nh_UtilityBase

    if(UtilityBase)
    {
        /* Create default config */
        nh->nh_CurrentCGC.cgc_ChunkID = AROS_LONG2BE(MAKE_ID('C','M','I','D'));
        nh->nh_CurrentCGC.cgc_Length = AROS_LONG2BE(sizeof(struct ClsGlobalCfg)-8);

        NewList(&nh->nh_Bindings);
        NewList(&nh->nh_PortSets);
    } else {
        KPRINTF(20, ("libInit: OpenLibrary(\"utility.library\", 39) failed!\n"));
        nh = NULL;
    }

    KPRINTF(10, ("libInit: Ok\n"));
    return(nh ? TRUE : FALSE);
}

static int libOpen(LIBBASETYPEPTR nh)
{
    KPRINTF(10, ("libOpen nh: 0x%08lx\n", nh));
    nLoadClassConfig(nh);
    return(TRUE);
}

static int libExpunge(LIBBASETYPEPTR nh)
{
    struct CAMDPortSet *cps;
    UWORD cnt;

    KPRINTF(10, ("libExpunge nh: 0x%08lx\n", nh));
    /* camd.library holds the class open while a driver is loaded, so no
       port is in use here. */
    while((cps = (struct CAMDPortSet *) RemHead(&nh->nh_PortSets)))
    {
        for(cnt = 0; cnt < 16; cnt++)
        {
            FreeVec(cps->cps_Adapters[cnt].ca_TXBuffer);
        }
        FreeVec(cps);
    }
    CloseLibrary((struct Library *) UtilityBase);
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

#if defined(__mc68000__)
#include "CAMDDriver.c"
#endif

/* /// "nCopyID()" */
/* Copies an ID into a field of size bytes, always NUL-terminated. */
static void nCopyID(UBYTE *to, ULONG size, CONST_STRPTR from)
{
    ULONG len = 0;

    while(len + 1 < size && from[len])
    {
        to[len] = from[len];
        len++;
    }
    to[len] = 0;
}
/* \\\ */

/* /// "nFindPortSet()" */
/* Call under Forbid(). */
static struct CAMDPortSet * nFindPortSet(struct NepHidBase *nh, STRPTR id)
{
    struct CAMDPortSet *cps;

    for(cps = (struct CAMDPortSet *) nh->nh_PortSets.lh_Head;
        cps->cps_Node.ln_Succ;
        cps = (struct CAMDPortSet *) cps->cps_Node.ln_Succ)
    {
        if(!strcmp((char *) cps->cps_ID, (char *) id))
        {
            return(cps);
        }
    }
    return(NULL);
}
/* \\\ */

/* /// "nGetPortSet()" */
static struct CAMDPortSet * nGetPortSet(struct NepHidBase *nh, STRPTR id)
{
    struct CAMDPortSet *cps;
    struct CAMDPortSet *newcps;

    Forbid();
    cps = nFindPortSet(nh, id);
    Permit();
    if(cps)
    {
        return(cps);
    }
    if(!(newcps = AllocVec(sizeof(struct CAMDPortSet), MEMF_PUBLIC|MEMF_CLEAR)))
    {
        return(NULL);
    }
    nCopyID(newcps->cps_ID, sizeof(newcps->cps_ID), id);
    newcps->cps_Node.ln_Name = (char *) newcps->cps_ID;
    Forbid();
    if(!(cps = nFindPortSet(nh, id)))
    {
        AddTail(&nh->nh_PortSets, &newcps->cps_Node);
        cps = newcps;
        newcps = NULL;
    }
    Permit();
    FreeVec(newcps);
    return(cps);
}
/* \\\ */

/* /// "nAttachPorts()" */
/* The device is there: the driver's ports, open or not, send to it. */
static void nAttachPorts(struct NepClassHid *nch)
{
    struct CAMDPortSet *cps = nch->nch_Ports;
    UWORD cnt;

    /* GetFromCAMD() reads ca_MsgPort from a software interrupt. */
    Disable();
    cps->cps_Binding = nch;
    for(cnt = 0; cnt < 16; cnt++)
    {
        cps->cps_Adapters[cnt].ca_MsgPort = nch->nch_TaskMsgPort;
    }
    Enable();
}
/* \\\ */

/* /// "nDetachPorts()" */
/* The device is gone. Ports stay open in CAMD; what they are sent is
   dropped until it comes back. */
static void nDetachPorts(struct NepClassHid *nch)
{
    struct CAMDPortSet *cps = nch->nch_Ports;
    UWORD cnt;

    Disable();
    for(cnt = 0; cnt < 16; cnt++)
    {
        cps->cps_Adapters[cnt].ca_MsgPort = NULL;
    }
    cps->cps_Binding = NULL;
    Enable();
}
/* \\\ */

/* /// "nInstallDriver()" */
/* Writes DEVS:Midi/<ShortID>, unless it is there already with the same
   contents. CAMD requires the file name and the driver's name to match.
   The class does not open camd.library: CAMD loads the driver, and the
   driver opens this class. */
static void nInstallDriver(struct NepHidBase *nh, struct NepClassHid *nch, struct Library *ps)
{
    struct Library *DOSBase;
    UBYTE buf[64];
    UBYTE *tmpmem, *binstart, *oldmem;
    ULONG binsize, cnt;
    BPTR fh;
    BPTR lock;
    BOOL same = FALSE;

    if(!(DOSBase = OpenLibrary("dos.library", 37)))
    {
        return;
    }
#if !defined(__mc68000__)
    binsize = (unsigned long)&_binary_poseidonusb_size;
    binstart = (UBYTE *)&_binary_poseidonusb_start;
#else
    binsize = sizeof(CAMDDriver);
    binstart = (UBYTE *)CAMDDriver;
#endif
    if(!(tmpmem = psdAllocVec(binsize)))
    {
        CloseLibrary(DOSBase);
        return;
    }
    CopyMemQuick(binstart, tmpmem, binsize);
#if defined(__mc68000__)
    // fix name of driver -- position is hardcoded, but unlikely to move
    strcpy((char *) &tmpmem[0x46], (char *) nch->nch_ShortID); // "poseidonusb"
#else
    for(cnt = 0; cnt + CAMDUSB_NAMEMARKERSIZE + 32 <= binsize; cnt++)
    {
        if(!memcmp(&tmpmem[cnt], CAMDUSB_NAMEMARKER, CAMDUSB_NAMEMARKERSIZE))
        {
            memset(&tmpmem[cnt + CAMDUSB_NAMEMARKERSIZE], 0, 32);
            nCopyID(&tmpmem[cnt + CAMDUSB_NAMEMARKERSIZE], 32, nch->nch_ShortID);
            break;
        }
    }
#endif

    psdSafeRawDoFmt(buf, 64, "%s/%s", (STRPTR) "DEVS:Midi", nch->nch_ShortID);
    if((fh = Open(buf, MODE_OLDFILE)))
    {
        /* An older class wrote a driver CAMD would not load, named
           "poseidonusb" inside: replace it. */
        if((oldmem = psdAllocVec(binsize + 1)))
        {
            same = ((ULONG) Read(fh, oldmem, binsize + 1) == binsize) &&
                   !memcmp(oldmem, tmpmem, binsize);
            psdFreeVec(oldmem);
        }
        Close(fh);
    }
    if(!same)
    {
        if(!(fh = Open(buf, MODE_NEWFILE)))
        {
            if((lock = CreateDir("DEVS:Midi")))
            {
                UnLock(lock);
                psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                               "Created directory '%s'!",
                               (STRPTR) "DEVS:Midi");
            }
            fh = Open(buf, MODE_NEWFILE);
        }
        if(!fh)
        {
            psdAddErrorMsg(RETURN_ERROR, (STRPTR) libname,
                           "Couldn't generate CAMD MIDI driver '%s'!",
                           buf);
        } else {
            same = ((ULONG) Write(fh, tmpmem, binsize) == binsize);
            Close(fh);
            if(same)
            {
                psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                               "Generated CAMD MIDI driver '%s'!",
                               buf);
            }
        }
    }
    psdFreeVec(tmpmem);
    CloseLibrary(DOSBase);
}
/* \\\ */

/* /// "usbAttemptInterfaceBinding()" */
struct NepClassHid * usbAttemptInterfaceBinding(struct NepHidBase *nh, struct PsdInterface *pif)
{
    struct Library *ps;
    IPTR ifclass;
    IPTR subclass;
    IPTR proto;

    KPRINTF(1, ("nepHidAttemptInterfaceBinding(%08lx)\n", pif));
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        psdGetAttrs(PGA_INTERFACE, pif,
                    IFA_Class, &ifclass,
                    IFA_SubClass, &subclass,
                    IFA_Protocol, &proto,
                    TAG_DONE);
        CloseLibrary(ps);
        if((ifclass == AUDIO_CLASSCODE) && (subclass == AUDIO_MIDI_SUBCLASS))
        {
            return(usbForceInterfaceBinding(nh, pif));
        }
    }
    return(NULL);
}
/* \\\ */

/* /// "usbForceInterfaceBinding()" */
struct NepClassHid * usbForceInterfaceBinding(struct NepHidBase *nh, struct PsdInterface *pif)
{
    struct Library *ps;
    struct NepClassHid *nch;
    struct PsdConfig *pc;
    struct PsdDevice *pd;
    STRPTR devname;
    UBYTE buf[64];
    struct Task *tmptask;

    KPRINTF(1, ("nepHidAttemptInterfaceBinding(%08lx)\n", pif));
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        if((nch = psdAllocVec(sizeof(struct NepClassHid))))
        {
            STRPTR srcpos;
            STRPTR tarpos;
            UWORD cnt;

            psdGetAttrs(PGA_INTERFACE, pif, IFA_Config, &pc, TAG_END);
            psdGetAttrs(PGA_CONFIG, pc, CA_Device, &pd, TAG_END);
            psdGetAttrs(PGA_DEVICE, pd, DA_ProductName, &devname, TAG_END);

            // generate ID
            srcpos = devname;
            tarpos = nch->nch_ShortID;
            cnt = 31;
            while(*srcpos && cnt)
            {
                if(((*srcpos >= 'A') && (*srcpos <= 'Z')) ||
                   ((*srcpos >= 'a') && (*srcpos <= 'z')) ||
                   ((*srcpos >= '0') && (*srcpos <= '9')))
                {
                    *tarpos++ = *srcpos;
                    cnt--;
                }
                srcpos++;
            }
            *tarpos = 0;
            if(!nch->nch_ShortID[0])
            {
                nCopyID(nch->nch_ShortID, sizeof(nch->nch_ShortID), (CONST_STRPTR) "usbmidi");
            }

            /* Two devices of the same product get drivers of their own. */
            Forbid();
            for(cnt = 2; cnt <= 9; cnt++)
            {
                struct CAMDPortSet *cps = nFindPortSet(nh, nch->nch_ShortID);

                if(!cps || !cps->cps_Binding)
                {
                    break;
                }
                for(tarpos = nch->nch_ShortID;
                    *tarpos && tarpos < nch->nch_ShortID + sizeof(nch->nch_ShortID) - 1;
                    tarpos++);
                if(cnt > 2)
                {
                    tarpos--;
                } else if(tarpos - nch->nch_ShortID > 30) {
                    tarpos--;
                }
                *tarpos++ = '0' + cnt;
                *tarpos = 0;
            }
            Permit();

            if(!(nch->nch_Ports = nGetPortSet(nh, nch->nch_ShortID)))
            {
                psdFreeVec(nch);
                CloseLibrary(ps);
                return(NULL);
            }

            nch->nch_ClsBase = nh;
            nch->nch_Device = NULL;
            nch->nch_Interface = pif;

            nLoadClassConfig(nh);

            psdSafeRawDoFmt(buf, 64, "camdusbmidi.class<%08lx>", nch);
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
                    Forbid();
                    AddTail(&nh->nh_Bindings, &nch->nch_Node);
                    Permit();

                    psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                                   "Play it again, '%s'!",
                                   devname);
                    nInstallDriver(nh, nch, ps);
                    CloseLibrary(ps);
                    return(nch);
                }
            }
            nch->nch_ReadySigTask = NULL;
            //FreeSignal(nch->nch_ReadySignal);
            psdFreeVec(nch);
        }
        CloseLibrary(ps);
    }
    return(NULL);
}
/* \\\ */

/* /// "usbReleaseInterfaceBinding()" */
void usbReleaseInterfaceBinding(struct NepHidBase *nh, struct NepClassHid *nch)
{
    struct Library *ps;
    struct PsdConfig *pc;
    struct PsdDevice *pd;
    STRPTR devname;

    KPRINTF(1, ("nepHidReleaseInterfaceBinding(%08lx)\n", nch));
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        Forbid();
        Remove(&nch->nch_Node);
        nch->nch_ReadySignal = SIGB_SINGLE;
        nch->nch_ReadySigTask = FindTask(NULL);
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
        psdGetAttrs(PGA_INTERFACE, nch->nch_Interface, IFA_Config, &pc, TAG_END);
        psdGetAttrs(PGA_CONFIG, pc, CA_Device, &pd, TAG_END);
        psdGetAttrs(PGA_DEVICE, pd, DA_ProductName, &devname, TAG_END);
        psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                       "'%s' fell silent!",
                       devname);
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
                 *((STRPTR *) ti->ti_Data) = "USB MIDI CAMD Interface class";
                 count++;
             }
             if((ti = FindTagItem(UCCA_HasClassCfgGUI, tags)))
             {
                 *((IPTR *) ti->ti_Data) = TRUE;
                 count++;
             }
             if((ti = FindTagItem(UCCA_HasBindingCfgGUI, tags)))
             {
                 *((IPTR *) ti->ti_Data) = FALSE;
                 count++;
             }
             if((ti = FindTagItem(UCCA_AfterDOSRestart, tags)))
             {
                 *((IPTR *) ti->ti_Data) = FALSE;
                 count++;
             }
             if((ti = FindTagItem(UCCA_UsingDefaultCfg, tags)))
             {
                 *((IPTR *) ti->ti_Data) = nh->nh_UsingDefaultCfg;
                 count++;
             }
             break;

         case UGA_BINDING:
             if((ti = FindTagItem(UCCA_UsingDefaultCfg, tags)))
             {
                 *((IPTR *) ti->ti_Data) = FALSE;
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

    KPRINTF(10, ("Do Method %ld\n", methodid));
    switch(methodid)
    {
        case UCM_AttemptInterfaceBinding:
            return((IPTR) usbAttemptInterfaceBinding(nh, (struct PsdInterface *) methoddata[0]));

        case UCM_ForceInterfaceBinding:
            return((IPTR) usbForceInterfaceBinding(nh, (struct PsdInterface *) methoddata[0]));

        case UCM_ReleaseInterfaceBinding:
            usbReleaseInterfaceBinding(nh, (struct NepClassHid *) methoddata[0]);
            return(TRUE);

        case UCM_OpenCfgWindow:
            return(nOpenCfgWindow(nh));

        case UCM_ConfigChangedEvent:
            nLoadClassConfig(nh);
            return(TRUE);

        default:
            break;
    }
    return(0);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "usbCAMDOpenPort()" */
AROS_LH5(APTR, usbCAMDOpenPort,
         AROS_LHA(APTR, xmitfct, A0),
         AROS_LHA(APTR, recvfct, A1),
         AROS_LHA(APTR, userdata, A2),
         AROS_LHA(STRPTR, idstr, A3),
         AROS_LHA(ULONG, port, D0),
         LIBBASETYPEPTR, nh, 15, nep)
{
    AROS_LIBFUNC_INIT

    struct CAMDPortSet *cps;
    struct CAMDAdapter *ca;
    UBYTE *buffer;

    KPRINTF(10, ("Open Port %ld, ID=%s\n", port, idstr));
    if(port > 15)
    {
        KPRINTF(20, ("Port %ld out of range!\n", port));
        return(0);
    }
    /* The port opens whether the device is plugged in or not, so CAMD
       clients can link to it and wait. */
    if(!(cps = nGetPortSet(nh, idstr)))
    {
        return(0);
    }
    ca = &cps->cps_Adapters[port];
    /* Kept until the class is expunged: the device's task may be reading
       it while CAMD closes the port. */
    if(!ca->ca_TXBuffer)
    {
        if(!(buffer = AllocVec(4096, MEMF_PUBLIC|MEMF_CLEAR)))
        {
            KPRINTF(20, ("Out of memory!\n"));
            return(0);
        }
        Disable();
        ca->ca_TXBufSize = 4096;
        ca->ca_TXBuffer = buffer;
        Enable();
    }
    Disable();
    ca->ca_PortNum = port;
    ca->ca_TXFunc = xmitfct;
    ca->ca_RXFunc = recvfct;
    ca->ca_UserData = userdata;
    ca->ca_TXReadPos = 0;
    ca->ca_TXWritePos = 0;
    ca->ca_SysExMode = 0;
    ca->ca_MsgPort = cps->cps_Binding ? cps->cps_Binding->nch_TaskMsgPort : NULL;
    Enable();
    return(ca);
    
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "usbCAMDClosePort()" */
AROS_LH2(void, usbCAMDClosePort,
         AROS_LHA(ULONG, port, D0),
         AROS_LHA(STRPTR, idstr, A1),
         LIBBASETYPEPTR, nh, 16, nep)
{
    AROS_LIBFUNC_INIT

    struct CAMDPortSet *cps;

    KPRINTF(10, ("Close Port %ld, ID=%s\n", port, idstr));
    if(port > 15)
    {
        KPRINTF(10, ("Port %ld out of range!\n", port));
        return;
    }
    Forbid();
    cps = nFindPortSet(nh, idstr);
    Permit();
    if(cps)
    {
        cps->cps_Adapters[port].ca_IsOpen = FALSE;
    }
    
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/**************************************************************************/

#undef  ps
#define ps nch->nch_Base

/* /// "nHidTask()" */
AROS_UFH0(void, nHidTask)
{
    AROS_USERFUNC_INIT

    struct NepClassHid *nch;
    struct PsdPipe *pp;
    ULONG sigmask;
    ULONG sigs;
    UBYTE *buf;
    LONG ioerr;
    ULONG len;

    if((nch = nAllocHid()))
    {
        nAttachPorts(nch);
        Forbid();
        if(nch->nch_ReadySigTask)
        {
            Signal(nch->nch_ReadySigTask, 1L<<nch->nch_ReadySignal);
        }
        Permit();
        sigmask = (1L<<nch->nch_TaskMsgPort->mp_SigBit)|SIGBREAKF_CTRL_C;
        buf = nch->nch_EPInBuf;
        psdSendPipe(nch->nch_EPInPipe, buf, 1024);
        do
        {
            sigs = Wait(sigmask);
            while((pp = (struct PsdPipe *) GetMsg(nch->nch_TaskMsgPort)))
            {
                if(pp == nch->nch_EPInPipe)
                {
                    if(!(ioerr = psdGetPipeError(pp)))
                    {
                        len = psdGetPipeActual(pp);
                        nParseMidi(nch, buf, len);
                    }
                    psdSendPipe(nch->nch_EPInPipe, buf, 1024);
                    break;
                }
            }
            nParseMidiOut(nch);
        } while(!(sigs & SIGBREAKF_CTRL_C));
        KPRINTF(20, ("Going down the river!\n"));
        psdAbortPipe(nch->nch_EPInPipe);
        psdWaitPipe(nch->nch_EPInPipe);
        nFreeHid(nch);
    }
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nParseMidiOut()" */
void nParseMidiOut(struct NepClassHid *nch)
{
    UBYTE *out = nch->nch_EPOutBuf;
    UWORD goodpkt = 0;
    UWORD cnt;
    struct CAMDAdapter *ca;
    ca = nch->nch_Ports->cps_Adapters;
    for(cnt = 0; cnt < 16; cnt++)
    {
        if(ca->ca_IsOpen && ca->ca_TXReadPos != ca->ca_TXWritePos)
        {
            ULONG chan = ca->ca_PortNum<<4;
            ULONG mask = ca->ca_TXBufSize - 1;
            ULONG len = (ca->ca_TXWritePos + ca->ca_TXBufSize - ca->ca_TXReadPos) & mask;
            UBYTE *buf = ca->ca_TXBuffer;
            UBYTE cmd;
            if(len > 100)
            {
                KPRINTF(1, ("TXWritePos=%ld, TXReadPos=%ld, len=%ld", ca->ca_TXWritePos, ca->ca_TXReadPos, len));
            }
            //KPRINTF(1, ("Data OUT %ld\n", len));
            do
            {
                cmd = buf[ca->ca_TXReadPos];
                if(cmd & 0x80)
                {
                    if(ca->ca_SysExMode && (cmd != 0xf0) && (cmd != 0xf7))
                    {
                        KPRINTF(10, ("SysEx Paused\n"));
                        // sysex paused
                        ca->ca_SysExMode = 2;
                    }
                    switch(cmd>>4)
                    {
                        case 0x8:
                        case 0x9:
                        case 0xa:
                        case 0xb:
                        case 0xe:
                            if(len < 3)
                            {
                                len = 0;
                                break;
                            }
                            len -= 3;
                            *out++ = (cmd >> 4) | chan;
                            *out++ = cmd;
                            ca->ca_TXReadPos = (ca->ca_TXReadPos+1) & mask;
                            *out++ = buf[ca->ca_TXReadPos];
                            ca->ca_TXReadPos = (ca->ca_TXReadPos+1) & mask;
                            *out++ = buf[ca->ca_TXReadPos];
                            ca->ca_TXReadPos = (ca->ca_TXReadPos+1) & mask;
                            goodpkt++;
                            break;

                        case 0xc:
                        case 0xd:
                            if(len < 2)
                            {
                                len = 0;
                                break;
                            }
                            len -= 2;
                            *out++ = (cmd >> 4) | chan;
                            *out++ = cmd;
                            ca->ca_TXReadPos = (ca->ca_TXReadPos+1) & mask;
                            *out++ = buf[ca->ca_TXReadPos];
                            ca->ca_TXReadPos = (ca->ca_TXReadPos+1) & mask;
                            *out++ = 0;
                            goodpkt++;
                            break;

                        case 0xf:
                            switch(cmd & 0xf)
                            {
                                case 0x0: // SysEx start
                                    KPRINTF(10, ("SysEx Starts\n"));
                                    if(!ca->ca_SysExMode)
                                    {
                                        ca->ca_SysExMode = 1;
                                        ca->ca_SysExData = cmd;
                                        ca->ca_SysExNum = 1;
                                    }
                                    len--;
                                    ca->ca_TXReadPos = (ca->ca_TXReadPos+1) & mask;
                                    break;

                                case 0x2: // SPP
                                    if(len < 3)
                                    {
                                        len = 0;
                                        break;
                                    }
                                    len -= 3;
                                    *out++ = 0x3|chan;
                                    *out++ = cmd;
                                    ca->ca_TXReadPos = (ca->ca_TXReadPos+1) & mask;
                                    *out++ = buf[ca->ca_TXReadPos];
                                    ca->ca_TXReadPos = (ca->ca_TXReadPos+1) & mask;
                                    *out++ = buf[ca->ca_TXReadPos];
                                    ca->ca_TXReadPos = (ca->ca_TXReadPos+1) & mask;
                                    goodpkt++;
                                    break;

                                case 0x3: // SongSelect
                                    if(len < 2)
                                    {
                                        len = 0;
                                        break;
                                    }
                                    len -= 2;
                                    *out++ = 0x2|chan;
                                    *out++ = cmd;
                                    ca->ca_TXReadPos = (ca->ca_TXReadPos+1) & mask;
                                    *out++ = buf[ca->ca_TXReadPos];
                                    ca->ca_TXReadPos = (ca->ca_TXReadPos+1) & mask;
                                    *out++ = 0;
                                    goodpkt++;
                                    break;

                                case 0x7: // SysEx Ends
                                    len--;
                                    ca->ca_TXReadPos = (ca->ca_TXReadPos+1) & mask;
                                    if(ca->ca_SysExMode)
                                    {
                                        KPRINTF(10, ("SysEx Ends\n"));
                                        if(ca->ca_SysExMode == 2)
                                        {
                                            KPRINTF(10, ("SysEx Resume\n"));
                                            // was paused, continue
                                            ca->ca_SysExMode = 1;
                                            break;
                                        }

                                        if(ca->ca_SysExNum == 0)
                                        {
                                            *out++ = 0x5|chan;
                                            *out++ = cmd;
                                            *out++ = 0;
                                            *out++ = 0;
                                        }
                                        else if(ca->ca_SysExNum == 1)
                                        {
                                            *out++ = 0x6|chan;
                                            *out++ = ca->ca_SysExData;
                                            *out++ = cmd;
                                            *out++ = 0;
                                        } else {
                                            *out++ = 0x7|chan;
                                            *out++ = ca->ca_SysExData>>8;
                                            *out++ = ca->ca_SysExData;
                                            *out++ = cmd;
                                        }
                                        KPRINTF(10, ("LastPkt: %08lx\n", *((ULONG *) &out[-4])));
                                        goodpkt++;
                                        ca->ca_SysExMode = 0;
                                    } else {
                                        KPRINTF(10, ("SysEx End Ignored\n"));
                                    }
                                    break;

                                case 0x8: // Timing Clock.
                                case 0x9: // Undefined.  (Reserved)
                                case 0xa: // Start.
                                case 0xb: // Continue.
                                case 0xc: // Stop.
                                case 0xd: // Undefined.  (Reserved)
                                case 0xe: // Active Sensing.
                                case 0xf: // Reset.
                                    len--;
                                    *out++ = 0xf|chan;
                                    *out++ = cmd;
                                    ca->ca_TXReadPos = (ca->ca_TXReadPos+1) & mask;
                                    *out++ = 0;
                                    *out++ = 0;
                                    goodpkt++;
                                    break;
                            }
                            break;
                    }
                    if(!len)
                    {
                        break;
                    }
                } else {
                    if(ca->ca_SysExMode)
                    {
                        ca->ca_SysExData <<= 8;
                        ca->ca_SysExData |= cmd;
                        ca->ca_SysExNum++;
                        if(ca->ca_SysExNum == 3)
                        {
                            KPRINTF(1, ("Cont 3B SysEx %06lx\n", ca->ca_SysExData));
                            *out++ = 0x4|chan;
                            *out++ = (ca->ca_SysExData >> 16) & 0xff;
                            *out++ = (ca->ca_SysExData >> 8) & 0xff;
                            *out++ = ca->ca_SysExData & 0xff;
                            goodpkt++;
                            ca->ca_SysExData = 0;
                            ca->ca_SysExNum = 0;
                        }
                    }
                    len--;
                    ca->ca_TXReadPos = (ca->ca_TXReadPos+1) & mask;
                }
            } while(len && (goodpkt < 2040));
        }
        ca++;
    }
    if(goodpkt)
    {
        //KPRINTF(1, ("Sending %ld pkts\n", goodpkt));
        psdDoPipe(nch->nch_EPOutPipe, nch->nch_EPOutBuf, (ULONG) goodpkt<<2);
    }
}
/* \\\ */

/* /// "nParseMidi()" */
void nParseMidi(struct NepClassHid *nch, UBYTE *buf, ULONG len)
{
    UWORD cmd;
    UWORD chan;
    struct CAMDAdapter *ca;
    struct {
        ULONG length;
        UBYTE data[3];
    } message;

    while(len > 3)
    {
        KPRINTF(1, ("Msg: %02lx %02lx %02lx %02lx\n", buf[0], buf[1], buf[2], buf[3]));
        chan = buf[0] >> 4;
        ca = &nch->nch_Ports->cps_Adapters[chan];
        if(ca->ca_IsOpen)
        {
            cmd = buf[0] & 0x0f;
            switch(cmd)
            {
                case 0x0: // Miscellaneous function codes. Reserved for future extensions.
                case 0x1: // Cable events. Reserved for future expansion.
                    break;

                case 0x5: // Single-byte System Common Message or SysEx ends with following single byte.
                case 0xF: // Single Byte
                    message.length = 1;
                    CopyMem(buf + 1, message.data, 1);
                    CallHookA(&ca->ca_CAMDRXFunc, (Object *) &message, NULL);
                    break;

                case 0x2: // Two-byte System Common messages like MTC, SongSelect, etc.
                case 0x6: // SysEx ends with following two bytes. *** FIXME? ***
                    message.length = 2;
                    CopyMem(buf + 1, message.data, 2);
                    CallHookA(&ca->ca_CAMDRXFunc, (Object *) &message, NULL);
                    break;

                case 0x3: // Three-byte System Common messages like SPP, etc.
                case 0x4: // SysEx starts or continues *** FIXME? ***
                case 0x7: // SysEx ends with following three bytes.  *** FIXME? ***
                case 0x8: // Note-off
                case 0x9: // Note-on
                case 0xA: // Poly-KeyPress
                case 0xB: // Control Change
                case 0xE: // PitchBend Change
                    message.length = 3;
                    CopyMem(buf + 1, message.data, 3);
                    CallHookA(&ca->ca_CAMDRXFunc, (Object *) &message, NULL);
                    break;

                case 0xC: // Program Change
                case 0xD: // Channel Pressure
                    message.length = 2;
                    CopyMem(buf + 1, message.data, 2);
                    CallHookA(&ca->ca_CAMDRXFunc, (Object *) &message, NULL);
                    break;
            }
        } else {
            KPRINTF(1, ("Not open\n"));
        }

        buf += 4;
        len -= 4;
    }
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
        psdGetAttrs(PGA_INTERFACE, nch->nch_Interface,
                    IFA_Config, &nch->nch_Config,
                    IFA_InterfaceNum, &nch->nch_IfNum,
                    TAG_END);
        psdGetAttrs(PGA_CONFIG, nch->nch_Config,
                    CA_Device, &nch->nch_Device,
                    TAG_END);

        nch->nch_EPIn = psdFindEndpoint(nch->nch_Interface, NULL,
                                        EA_IsIn, TRUE,
                                        EA_TransferType, USEAF_BULK,
                                        TAG_END);

        nch->nch_EPOut = psdFindEndpoint(nch->nch_Interface, NULL,
                                         EA_IsIn, FALSE,
                                         EA_TransferType, USEAF_BULK,
                                         TAG_END);
        if(!(nch->nch_EPIn && nch->nch_EPOut))
        {
            KPRINTF(1, ("Ooops!?! No Endpoints defined?\n"));
            psdAddErrorMsg(RETURN_FAIL, (STRPTR) libname,
                           "No Bulk-In or Bulk-Out Endpoint!");
            break;
        }
        if((nch->nch_TaskMsgPort = CreateMsgPort()))
        {
            if((nch->nch_EP0Pipe = psdAllocPipe(nch->nch_Device, nch->nch_TaskMsgPort, NULL)))
            {
                if((nch->nch_EPInPipe = psdAllocPipe(nch->nch_Device, nch->nch_TaskMsgPort, nch->nch_EPIn)))
                {
                    psdSetAttrs(PGA_PIPE, nch->nch_EPInPipe,
                                PPA_NakTimeout, FALSE,
                                PPA_AllowRuntPackets, TRUE,
                                TAG_END);

                    if((nch->nch_EPOutPipe = psdAllocPipe(nch->nch_Device, nch->nch_TaskMsgPort, nch->nch_EPOut)))
                    {
                        psdSetAttrs(PGA_PIPE, nch->nch_EPOutPipe,
                                    PPA_NakTimeout, TRUE,
                                    PPA_AllowRuntPackets, TRUE,
                                    PPA_NakTimeoutTime, 5000,
                                    TAG_END);
                        if((nch->nch_EPInBuf = psdAllocVec(1024)))
                        {
                            if((nch->nch_EPOutBuf = psdAllocVec(8192)))
                            {
                                nch->nch_Task = thistask;
                                return(nch);
                            }
                            psdFreeVec(nch->nch_EPInBuf);
                        }
                        psdFreePipe(nch->nch_EPOutPipe);
                    }
                    psdFreePipe(nch->nch_EPInPipe);
                }
                psdFreePipe(nch->nch_EP0Pipe);
            }
            DeleteMsgPort(nch->nch_TaskMsgPort);
        }
    } while(FALSE);
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
    /* delete all msgport references to this instance, disabling all midi transmissions */
    nDetachPorts(nch);
    psdFreeVec(nch->nch_EPOutBuf);
    psdFreeVec(nch->nch_EPInBuf);
    psdFreePipe(nch->nch_EPOutPipe);
    psdFreePipe(nch->nch_EPInPipe);
    psdFreePipe(nch->nch_EP0Pipe);
    DeleteMsgPort(nch->nch_TaskMsgPort);
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

#undef ps

/* /// "nLoadClassConfig()" */
BOOL nLoadClassConfig(struct NepHidBase *nh)
{
    struct Library *ps;
    struct ClsGlobalCfg *cgc;
    struct PsdIFFContext *pic;

    KPRINTF(10, ("Loading Class Config...\n"));
    if(!(ps = OpenLibrary("poseidon.library", 4)))
    {
        return(FALSE);
    }
    Forbid();
    nh->nh_UsingDefaultCfg = TRUE;
    pic = psdGetClsCfg(libname);
    if(pic)
    {
        if((cgc = psdGetCfgChunk(pic, AROS_LONG2BE(nh->nh_CurrentCGC.cgc_ChunkID))))
        {
            CopyMem(((UBYTE *) cgc) + 8, ((UBYTE *) &nh->nh_CurrentCGC) + 8, min(AROS_LONG2BE(cgc->cgc_Length), AROS_LONG2BE(nh->nh_CurrentCGC.cgc_Length)));
            psdFreeVec(cgc);
            nh->nh_UsingDefaultCfg = FALSE;
        }
    }
    Permit();
    CloseLibrary(ps);
    return(FALSE);
}
/* \\\ */

/* /// "nOpenCfgWindow()" */
LONG nOpenCfgWindow(struct NepHidBase *nh)
{
    struct Library *ps;
    KPRINTF(10, ("Opening GUI...\n"));
    if(!(ps = OpenLibrary("poseidon.library", 4)))
    {
        return(FALSE);
    }
    Forbid();
    if(!nh->nh_GUITask)
    {
        if((nh->nh_GUITask = psdSpawnSubTask(MOD_NAME_STRING " GUI", nGUITask, nh)))
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

/* /// "nGUITask()" */
AROS_UFH0(void, nGUITask)
{
    AROS_USERFUNC_INIT

    struct Task *thistask;
    struct NepHidBase *nh;
    APTR pic;

    thistask = FindTask(NULL);
#undef ps
#define ps nh->nh_PsdBase
#undef IntuitionBase
#define IntuitionBase nh->nh_IntBase
#undef MUIMasterBase
#define MUIMasterBase nh->nh_MUIBase

    nh = thistask->tc_UserData;
    ++nh->nh_Library.lib_OpenCnt;
    if(!(MUIMasterBase = OpenLibrary(MUIMASTER_NAME, MUIMASTER_VMIN)))
    {
        KPRINTF(10, ("Couldn't open muimaster.library.\n"));
        nGUITaskCleanup(nh);
        return;
    }

    if(!(IntuitionBase = OpenLibrary("intuition.library", 39)))
    {
        KPRINTF(10, ("Couldn't open intuition.library.\n"));
        nGUITaskCleanup(nh);
        return;
    }
    if(!(ps = OpenLibrary("poseidon.library", 4)))
    {
        KPRINTF(10, ("Couldn't open poseidon.library.\n"));
        nGUITaskCleanup(nh);
        return;
    }

    nh->nh_App = ApplicationObject,
        MUIA_Application_Title      , (IPTR)libname,
        MUIA_Application_Version    , (IPTR)VERSION_STRING,
        MUIA_Application_Copyright  , (IPTR)(ISOASCII_COPYRIGHT "2006-2009 Chris Hodges"),
        MUIA_Application_Author     , (IPTR)"Chris Hodges <chrisly@platon42.de>",
        MUIA_Application_Description, (IPTR)"Settings for the camdusbmidi.class",
        MUIA_Application_Base       , (IPTR)"CAMDUSBMIDI",
        MUIA_Application_HelpFile   , (IPTR)"HELP:Poseidon.guide",
        MUIA_Application_Menustrip  , (IPTR)MenustripObject,
            Child, (IPTR)MenuObjectT((IPTR)"Project"),
                Child, (IPTR)(nh->nh_AboutMI = MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)"About...",
                    MUIA_Menuitem_Shortcut, (IPTR)"?",
                    End),
                End,
            Child, (IPTR)MenuObjectT((IPTR)"Settings"),
                Child, (IPTR)(nh->nh_UseMI = MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)"Save",
                    MUIA_Menuitem_Shortcut, (IPTR)"S",
                    End),
                Child, (IPTR)MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)NM_BARLABEL,
                    End,
                Child, (IPTR)(nh->nh_MUIPrefsMI = MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)"MUI Settings",
                    MUIA_Menuitem_Shortcut, (IPTR)"M",
                    End),
                End,
            End,

        SubWindow, (IPTR)(nh->nh_MainWindow = WindowObject,
            MUIA_Window_ID   , MAKE_ID('M','A','I','N'),
            MUIA_Window_Title, (IPTR)libname,
            MUIA_HelpNode, (IPTR)libname,

            WindowContents, (IPTR)VGroup,
                Child, (IPTR)ColGroup(2), GroupFrameT("Global Settings"),
                    Child, (IPTR)Label((IPTR) "None"),
                    Child, (IPTR)HSpace(0),
                    End,
                Child, (IPTR)VSpace(0),
                Child, (IPTR)HGroup,
                    MUIA_Group_SameWidth, TRUE,
                    Child, (IPTR)(nh->nh_UseObj = TextObject, ButtonFrame,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_Contents, (IPTR)"\33c Save ",
                        End),
                    Child, (IPTR)(nh->nh_CloseObj = TextObject, ButtonFrame,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_CycleChain, 1,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_Contents, (IPTR)"\33c Use ",
                        End),
                    End,
                End,
            End),
        End;

    if(!nh->nh_App)
    {
        KPRINTF(10, ("Couldn't create application\n"));
        nGUITaskCleanup(nh);
        return;
    }
    DoMethod(nh->nh_MainWindow, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
             nh->nh_App, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    DoMethod(nh->nh_UseObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nh->nh_App, 2, MUIM_Application_ReturnID, ID_STORE_CONFIG);
    DoMethod(nh->nh_CloseObj, MUIM_Notify, MUIA_Pressed, FALSE,
             nh->nh_App, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    DoMethod(nh->nh_AboutMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nh->nh_App, 2, MUIM_Application_ReturnID, ID_ABOUT);
    DoMethod(nh->nh_UseMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nh->nh_App, 2, MUIM_Application_ReturnID, ID_STORE_CONFIG);
    DoMethod(nh->nh_MUIPrefsMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             nh->nh_App, 2, MUIM_Application_OpenConfigWindow, 0);

    {
        IPTR  isopen = 0;
        IPTR  iconify = 0;
        ULONG sigs;
        ULONG sigmask;
        LONG retid;

        get(nh->nh_App, MUIA_Application_Iconified, &iconify);
        set(nh->nh_MainWindow, MUIA_Window_Open, TRUE);
        get(nh->nh_MainWindow, MUIA_Window_Open, &isopen);
        if(!(isopen || iconify))
        {
            nGUITaskCleanup(nh);
            return;
        }
        sigmask = 0;
        do
        {
            retid = DoMethod(nh->nh_App, MUIM_Application_NewInput, &sigs);
            switch(retid)
            {
                case ID_STORE_CONFIG:
                case MUIV_Application_ReturnID_Quit:
                    pic = psdGetClsCfg(libname);
                    if(!pic)
                    {
                        psdSetClsCfg(libname, NULL);
                        pic = psdGetClsCfg(libname);
                    }
                    if(pic)
                    {
                        if(psdAddCfgEntry(pic, &nh->nh_CurrentCGC))
                        {
                            if(retid != MUIV_Application_ReturnID_Quit)
                            {
                                psdSaveCfgToDisk(NULL, FALSE);
                            }
                            retid = MUIV_Application_ReturnID_Quit;
                        }
                    }
                    break;

               case ID_ABOUT:
                    MUI_RequestA(nh->nh_App, nh->nh_MainWindow, 0, NULL, "Marvellous!", VERSION_STRING, NULL);
                    break;
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
        set(nh->nh_MainWindow, MUIA_Window_Open, FALSE);
    }
    nGUITaskCleanup(nh);
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nGUITaskCleanup()" */
void nGUITaskCleanup(struct NepHidBase *nh)
{
    if(nh->nh_App)
    {
        MUI_DisposeObject(nh->nh_App);
        nh->nh_App = NULL;
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
    nh->nh_GUITask = NULL;
    --nh->nh_Library.lib_OpenCnt;
}
/* \\\ */

