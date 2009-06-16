/*
 *----------------------------------------------------------------------------
 *                         ptp class for poseidon
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#warning "PTP.class is currently not 64 bit compatible as it uses the fib_DiskKey LONG field for storing a pointer."
#warning "Also dp_Res1 and dp_Res2 of DOS packets are not 64 bit types, how is this going to work for locks then?"

#include "debug.h"

#include "ptp.class.h"

#include "numtostr.h"

/* /// "Lib Stuff" */
static const STRPTR libname = MOD_NAME_STRING;

static int libInit(LIBBASETYPEPTR nh)
{
    struct NepPTPBase *ret = NULL;
    struct NepClassPTP *nch;

    KPRINTF(10, ("libInit nh: 0x%08lx SysBase: 0x%08lx\n", nh, SysBase));

    nh->nh_UtilityBase = OpenLibrary("utility.library", 39);

#define	UtilityBase	nh->nh_UtilityBase

    if(UtilityBase)
    {
        NewList(&nh->nh_Bindings);
        nch = &nh->nh_DummyNCH;
        nch->nch_ClsBase = nh;
        nch->nch_Interface = NULL;
        nch->nch_CDC = AllocVec(sizeof(struct ClsDevCfg), MEMF_PUBLIC|MEMF_CLEAR);
        if(nch->nch_CDC)
        {
            ret = nh;
        }
        if(!ret)
        {
            CloseLibrary(UtilityBase);
        }
    } else {
        KPRINTF(1, ("libInit: OpenLibrary(\"utility.library\", 39) failed!\n"));
    }

    KPRINTF(1, ("libInit: Ok\n"));
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
    struct NepClassPTP *nch;
    KPRINTF(1, ("libExpunge nh: 0x%08lx\n", nh));
    CloseLibrary((struct Library *) UtilityBase);
    nch = &nh->nh_DummyNCH;
    FreeVec(nch->nch_CDC);
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

/* /// "usbAttemptInterfaceBinding()" */
struct NepClassPTP * usbAttemptInterfaceBinding(struct NepPTPBase *nh, struct PsdInterface *pif)
{
    struct Library *ps;
    IPTR ifclass;
    IPTR ifsubclass;
    IPTR ifproto;
    IPTR subclass;
    IPTR proto;
    IPTR devclass;
    struct PsdDevice *pd;
    struct PsdConfig *pc;
    BOOL checkformtp = FALSE;
    BOOL isptp = FALSE;

    KPRINTF(1, ("nepPTPAttemptInterfaceBinding(%08lx)\n", pif));
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        psdGetAttrs(PGA_INTERFACE, pif,
                    IFA_Class, &ifclass,
                    IFA_SubClass, &ifsubclass,
                    IFA_Protocol, &ifproto,
                    IFA_Config, &pc,
                    TAG_DONE);

        if((ifclass == STILLIMG_CLASSCODE) && (ifsubclass == 0x01) && (ifproto == 0x01))
        {
            isptp = TRUE;
        }
        if(nh->nh_DummyNCH.nch_CDC->cdc_EnableMTP)
        {
            psdGetAttrs(PGA_CONFIG, pc,
                        CA_Device, &pd,
                        TAG_END);
            psdGetAttrs(PGA_DEVICE, pd,
                    DA_Class, &devclass,
                    DA_SubClass, &subclass,
                    DA_Protocol, &proto,
                    TAG_DONE);
            if(((devclass == 0xff) && (subclass == 0xff) && (proto == 0xff)) ||
               ((devclass == 0xff) && (!subclass) && (!proto)) ||
               ((!devclass) && (!subclass) && (!proto)))
            {
                checkformtp = ((ifclass == 0xff) || (!ifclass)) &&
                              ((ifsubclass == 0xff) || (!ifsubclass)) &&
                              ((ifproto == 0xff) || (!ifproto));
                if(checkformtp)
                {
                    checkformtp = psdFindEndpoint(pif, NULL,
                                                  EA_IsIn, TRUE,
                                                  EA_TransferType, USEAF_BULK,
                                                  TAG_END) &&
                                  psdFindEndpoint(pif, NULL,
                                                  EA_IsIn, FALSE,
                                                  EA_TransferType, USEAF_BULK,
                                                  TAG_END); /*&&
                                  psdFindEndpoint(pif, NULL,
                                                  EA_IsIn, TRUE,
                                                  EA_TransferType, USEAF_INTERRUPT,
                                                  TAG_END);*/
                }
                if(checkformtp)
                {
                    struct MsgPort *mp;
                    struct PsdPipe *pp;
                    STRPTR idstr;
                    if((mp = CreateMsgPort()))
                    {
                        if((pp = psdAllocPipe(pd, mp, NULL)))
                        {
                            psdSetAttrs(PGA_PIPE, pp,
                                        PPA_NakTimeout, TRUE,
                                        PPA_NakTimeoutTime, 500,
                                        TAG_END);
                            psdAddErrorMsg(RETURN_OK, (STRPTR) libname, "This might be an MTP device, we will look in detail.");
                            idstr = psdGetStringDescriptor(pp, 0xee);
                            if(idstr)
                            {
                                if((idstr[0] == 'M') && (idstr[1] == 'S') && (idstr[2] == 'F') && (idstr[3] == 'T'))
                                {
                                    psdAddErrorMsg(RETURN_OK, (STRPTR) libname, "Trying to use this MTP device with the PTP driver.");
                                    isptp = TRUE;
                                }
                                psdFreeVec(idstr);
                            }
                            psdFreePipe(pp);
                        }
                        DeleteMsgPort(mp);
                    }
                }
            }
        }
        CloseLibrary(ps);
        if(isptp)
        {
            return(usbForceInterfaceBinding(nh, pif));
        }
    }
    return(NULL);
}
/* \\\ */

/* /// "usbForceInterfaceBinding()" */
struct NepClassPTP * usbForceInterfaceBinding(struct NepPTPBase *nh, struct PsdInterface *pif)
{
    struct Library *ps;
    struct NepClassPTP *nch;
    struct PsdConfig *pc;
    struct PsdDevice *pd;
    STRPTR devname;
    STRPTR ifidstr;
    STRPTR devidstr;
    IPTR ifnum;
    IPTR devclass;
    IPTR prodid;
    IPTR ifproto;
    UBYTE buf[64];
    struct Task *tmptask;

    KPRINTF(1, ("nepPTPForceInterfaceBinding(%08lx)\n", pif));
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        psdGetAttrs(PGA_INTERFACE, pif,
                    IFA_Config, &pc,
                    IFA_IDString, &ifidstr,
                    IFA_InterfaceNum, &ifnum,
                    IFA_Protocol, &ifproto,
                    TAG_DONE);
        psdGetAttrs(PGA_CONFIG, pc,
                    CA_Device, &pd,
                    TAG_END);
        psdGetAttrs(PGA_DEVICE, pd,
                    DA_ProductName, &devname,
                    DA_Class, &devclass,
                    DA_ProductID, &prodid,
                    DA_IDString, &devidstr,
                    TAG_END);
        if((nch = AllocVec(sizeof(struct NepClassPTP), MEMF_PUBLIC|MEMF_CLEAR)))
        {
            nch->nch_ClsBase = nh;
            nch->nch_CDC = AllocVec(sizeof(struct ClsDevCfg), MEMF_PUBLIC|MEMF_CLEAR);
            if(!nch->nch_CDC)
            {
                FreeVec(nch);
                CloseLibrary(ps);
                return(NULL);
            }
            NewList(&nch->nch_Storages);
            NewList(&nch->nch_FHs);
            nch->nch_Device = pd;
            nch->nch_Interface = pif;
            nch->nch_DevIDString = devidstr;
            nch->nch_IfIDString = ifidstr;
            nch->nch_IfNum = ifnum;

            nLoadBindingConfig(nch);

            psdSafeRawDoFmt(buf, 64, "ptp.class<%08lx>", nch);
            nch->nch_ReadySignal = SIGB_SINGLE;
            nch->nch_ReadySigTask = FindTask(NULL);
            SetSignal(0, SIGF_SINGLE);
            if((tmptask = psdSpawnSubTask(buf, nPTPTask, nch)))
            {
                psdBorrowLocksWait(tmptask, 1UL<<nch->nch_ReadySignal);
                if(nch->nch_Task)
                {
                    nch->nch_ReadySigTask = NULL;
                    //FreeSignal(nch->nch_ReadySignal);
                    psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                                   "Hold still for images from for '%s'!",
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
            FreeVec(nch->nch_CDC);
            FreeVec(nch);
            CloseLibrary(ps);
            return(nch);
        }
        CloseLibrary(ps);
    }
    return(NULL);

}
/* \\\ */

/* /// "usbReleaseInterfaceBinding()" */
void usbReleaseInterfaceBinding(struct NepPTPBase *nh, struct NepClassPTP *nch)
{
    struct Library *ps;
    struct PsdConfig *pc;
    struct PsdDevice *pd;
    STRPTR devname;

    KPRINTF(1, ("nepPTPReleaseInterfaceBinding(%08lx)\n", nch));
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
        psdGetAttrs(PGA_INTERFACE, nch->nch_Interface, IFA_Config, &pc, TAG_END);
        psdGetAttrs(PGA_CONFIG, pc, CA_Device, &pd, TAG_END);
        psdGetAttrs(PGA_DEVICE, pd, DA_ProductName, &devname, TAG_END);
        psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                       "Still pictures of '%s' became fugitive!",
                       devname);
        Forbid();
        Remove(&nch->nch_Node);
        Permit();
        FreeVec(nch->nch_CDC);
        FreeVec(nch);
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

    KPRINTF(1, ("nepPTPGetAttrsA(%ld, %08lx, %08lx)\n", type, usbstruct, tags));
    switch(type)
    {
        case UGA_CLASS:
             if((ti = FindTagItem(UCCA_Priority, tags)))
             {
                 *((SIPTR *) ti->ti_Data) = -100;
                 count++;
             }
             if((ti = FindTagItem(UCCA_Description, tags)))
             {
                 *((STRPTR *) ti->ti_Data) = "Support for DigiCams and MTP via PictureTransferProtocol (PTP)";
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
                 *((IPTR *) ti->ti_Data) = ((struct NepClassPTP *) usbstruct)->nch_UsingDefaultCfg;
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

    struct NepClassPTP *nch;

    KPRINTF(1, ("Do Method %ld\n", methodid));
    switch(methodid)
    {
        case UCM_AttemptInterfaceBinding:
            return((IPTR) usbAttemptInterfaceBinding(nh, (struct PsdInterface *) methoddata[0]));

        case UCM_ForceInterfaceBinding:
            return((IPTR) usbForceInterfaceBinding(nh, (struct PsdInterface *) methoddata[0]));

        case UCM_ReleaseInterfaceBinding:
            usbReleaseInterfaceBinding(nh, (struct NepClassPTP *) methoddata[0]);
            return(TRUE);

        case UCM_OpenCfgWindow:
            return(nOpenBindingCfgWindow(nh, &nh->nh_DummyNCH));

        case UCM_OpenBindingCfgWindow:
            return(nOpenBindingCfgWindow(nh, (struct NepClassPTP *) methoddata[0]));

        case UCM_ConfigChangedEvent:
            nLoadClassConfig(nh);
            Forbid();
            nch = (struct NepClassPTP *) nh->nh_Bindings.lh_Head;
            while(nch->nch_Node.ln_Succ)
            {
                nLoadBindingConfig(nch);
                nch = (struct NepClassPTP *) nch->nch_Node.ln_Succ;
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
BOOL nLoadClassConfig(struct NepPTPBase *nh)
{
    struct NepClassPTP *nch = &nh->nh_DummyNCH;
    struct Library *ps;
    struct ClsDevCfg *cdc;
    struct PsdIFFContext *pic;

    KPRINTF(1, ("Loading Class Config...\n"));
    if(nch->nch_GUITask)
    {
        return(FALSE);
    }
    if(!(ps = OpenLibrary("poseidon.library", 4)))
    {
        return(FALSE);
    }

    Forbid();
    /* Create default config */
    nch->nch_CDC->cdc_ChunkID = AROS_LONG2BE(MAKE_ID('P','T','P','C'));
    nch->nch_CDC->cdc_Length = AROS_LONG2BE(sizeof(struct ClsDevCfg)-8);
    nch->nch_CDC->cdc_EnableMTP = FALSE;
    nch->nch_CDC->cdc_NoPartObj = FALSE;
    strcpy(nch->nch_CDC->cdc_DOSName, "PTP");
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
BOOL nLoadBindingConfig(struct NepClassPTP *nch)
{
    struct NepPTPBase *nh = nch->nch_ClsBase;
    struct Library *ps;
    struct ClsDevCfg *cdc;
    struct PsdIFFContext *pic;

    KPRINTF(1, ("Loading Binding Config...\n"));
    if(nch->nch_GUITask)
    {
        return(FALSE);
    }
    nLoadClassConfig(nh);
    *nch->nch_CDC = *nh->nh_DummyNCH.nch_CDC;
    nch->nch_UsingDefaultCfg = TRUE;

    if(!(ps = OpenLibrary("poseidon.library", 4)))
    {
        return(FALSE);
    }

    Forbid();
    /* Load config */
    pic = psdGetUsbDevCfg(libname, nch->nch_DevIDString, nch->nch_IfIDString);
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
LONG nOpenBindingCfgWindow(struct NepPTPBase *nh, struct NepClassPTP *nch)
{
    struct Library *ps;
    KPRINTF(1, ("Opening GUI...\n"));
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
#undef DOSBase
#define	DOSBase	nch->nch_DOSBase

/* /// "BSTR Macros" */
#define b2cstr(bstr, cstr) { ULONG i; for (i = 0; i < bstr[0]; i++) cstr[i] = bstr[i + 1]; cstr[i] = 0x00; }
#define c2bstr(cstr, bstr)\
    do\
    {\
        int i = 0;\
        UBYTE c;\
        STRPTR cp = (STRPTR) (cstr);\
        STRPTR bp = (STRPTR) (bstr);\
        while((c = cp[i]))\
        {\
            bp[++i] = c;\
        }\
        bp[0] = i;\
    } while(0)
/* \\\ */

/* /// "nAllocVec()" */
APTR nAllocVec(struct NepClassPTP *nch, ULONG size)
{
    ULONG *mem;
    size += 4;
    mem = (ULONG *) AllocPooled(nch->nch_MemPool, size);
    if(!mem)
    {
        return(NULL);
    }
    *mem = size;
    return(mem + 1);
}
/* \\\ */

/* /// "nFreeVec()" */
void nFreeVec(struct NepClassPTP *nch, APTR mem)
{
    ULONG *memptr = (ULONG *) mem;
    ULONG size;
    if(!mem)
    {
        return;
    }
    size = *(--memptr);
    if(size < 4)
    {
        KPRINTF(20, ("illegal size %ld for block %08lx\n", size, memptr));
    }
    FreePooled(nch->nch_MemPool, memptr, size);
}
/* \\\ */

/* /// "nPsdStrToNStr()" */
STRPTR nPsdStrToNStr(struct NepClassPTP *nch, STRPTR oldstr)
{
    ULONG len = 1;
    STRPTR strptr = oldstr;
    STRPTR tarptr;
    STRPTR newstr;
    if(!oldstr)
    {
        return(NULL);
    }
    while(*strptr++)
    {
        len++;
    }
    newstr = nAllocVec(nch, len);
    if(newstr)
    {
        strptr = oldstr;
        tarptr = newstr;
        while((*tarptr++ = *strptr++));
    }
    psdFreeVec(oldstr);
    return(newstr);
}
/* \\\ */

/* /// "nSendPTPCmd()" */
LONG nSendPTPCmd(struct NepClassPTP *nch, struct PTPOp *po)
{
    ULONG len = 4+2+2+4+(po->po_NumParam<<2);
    LONG ioerr;
    ULONG cmd = po->po_OpCode;
    struct PTPOp tmppo;
    UWORD cnt;

    KPRINTF(10, ("Sending Command %04lx (%s) with %ld params\n",
                 cmd,
                 nNumToStr(nch, NTS_OPCODE, cmd, "unknown"),
                 po->po_NumParam));

    tmppo.po_Length = AROS_LONG2LE(len);
    tmppo.po_ContainerType = AROS_WORD2LE(PCT_COMMAND);
    tmppo.po_OpCode = AROS_WORD2LE(cmd);
    //po->po_SessionID = AROS_LONG2LE(nch->nch_SessionID);
    po->po_TransID = nch->nch_TransID;
    tmppo.po_TransID = AROS_LONG2LE(nch->nch_TransID);

    for(cnt = 0; cnt < po->po_NumParam; cnt++)
    {
        tmppo.po_Param[cnt] = AROS_LONG2LE(po->po_Param[cnt]);
    }

    if(!(++nch->nch_TransID))
    {
        nch->nch_TransID++; // 0 not allowed)
    }

    ioerr = psdDoPipe(nch->nch_EPOutPipe, &tmppo, len);
    if(ioerr)
    {
        psdAddErrorMsg(RETURN_ERROR, (STRPTR) libname,
                       "Command '%s' (%04lx) failed: %s (%ld)!",
                       nNumToStr(nch, NTS_OPCODE, cmd, "unknown"), cmd,
                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
    }
    return(ioerr);
}
/* \\\ */

/* /// "nSendPTPData()" */
LONG nSendPTPData(struct NepClassPTP *nch, struct PTPOp *po, UBYTE *data, ULONG datalen)
{
    ULONG len = 4+2+2+4+datalen;
    LONG ioerr;
    ULONG cmd = po->po_OpCode;
    struct PTPOp *tmppo = (struct PTPOp *) nch->nch_InBuf;

    KPRINTF(10, ("Sending Data for Command %04lx (%s) with %ld params\n",
                 cmd,
                 nNumToStr(nch, NTS_OPCODE, cmd, "unknown"),
                 po->po_NumParam));

    tmppo->po_Length = AROS_LONG2LE(len);
    tmppo->po_ContainerType = AROS_WORD2LE(PCT_DATA);
    tmppo->po_OpCode = AROS_WORD2LE(cmd);
    tmppo->po_TransID = AROS_LONG2LE(po->po_TransID);
    CopyMem(data, nch->nch_InBuf + 12, 500);

    psdSetAttrs(PGA_PIPE, nch->nch_EPOutPipe,
                PPA_NoZeroPktTerm, (datalen < 500) ? FALSE : TRUE,
                TAG_END);

    KPRINTF(5, ("Sending data %ld\n", (datalen <= 500) ? len : 512));
    DB(dumpmem(nch->nch_InBuf, (datalen <= 116) ? len : 128));

    ioerr = psdDoPipe(nch->nch_EPOutPipe, nch->nch_InBuf, (datalen <= 500) ? len : 512);
    psdSetAttrs(PGA_PIPE, nch->nch_EPOutPipe,
                PPA_NoZeroPktTerm, FALSE,
                TAG_END);
    if(ioerr)
    {
        psdAddErrorMsg(RETURN_ERROR, (STRPTR) libname,
                       "Data (1) '%s' (%04lx) failed: %s (%ld)!",
                       nNumToStr(nch, NTS_OPCODE, cmd, "unknown"), cmd,
                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
        return(ioerr);
    }
    if(datalen > 500)
    {
        ioerr = psdDoPipe(nch->nch_EPOutPipe, &data[500], datalen - 500);
        if(ioerr)
        {
            psdAddErrorMsg(RETURN_ERROR, (STRPTR) libname,
                           "Data (2) '%s' (%04lx) failed: %s (%ld)!",
                           nNumToStr(nch, NTS_OPCODE, cmd, "unknown"), cmd,
                           psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
        }
    }
    return(ioerr);
}
/* \\\ */

/* /// "nGetPTPRsp()" */
UBYTE * nGetPTPRsp(struct NepClassPTP *nch, struct PTPResponse *pr)
{
    ULONG len;
    ULONG iolen;
    LONG ioerr;
    UWORD cnt;
    struct PTPResponse *tmppr = (struct PTPResponse *) nch->nch_InBuf;
    UBYTE *buf = NULL;
    UWORD retry = 3;

    pr->pr_DataLength = 0;
    pr->pr_RespCode = PRC_UNDEFINED;

    KPRINTF(1, ("Getting Response\n"));
    do
    {
        ioerr = psdDoPipe(nch->nch_EPInPipe, nch->nch_InBuf, 512);
        if(!ioerr)
        {
            break;
        } else {
            KPRINTF(10, ("IOerror %ld, actual=%ld\n", ioerr, psdGetPipeActual(nch->nch_EPInPipe)));
        }
    } while(--retry);

    if(ioerr)
    {
        psdAddErrorMsg(RETURN_ERROR, (STRPTR) libname,
                       "Reading Response failed: %s (%ld)!",
                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
        pr->pr_IOErr = ioerr;
        return(NULL);
    }

    iolen = psdGetPipeActual(nch->nch_EPInPipe);
    KPRINTF(5, ("Response received %ld\n", iolen));
    DB(dumpmem(nch->nch_InBuf, iolen));
    if(iolen < 12)
    {
        KPRINTF(20, ("Response %ld too short!\n", iolen));
        pr->pr_IOErr = UHIOERR_RUNTPACKET;
        return(NULL);
    }
    len = AROS_LONG2LE(tmppr->pr_Length);
    KPRINTF(10, ("Total response %ld\n", len));
    if(iolen > len)
    {
        KPRINTF(20, ("Response %ld exceeds %ld length field!\n", iolen , len));
        pr->pr_IOErr = UHIOERR_RUNTPACKET;
        return(NULL);
    }

    if((len < 512) && (iolen < len))
    {
        KPRINTF(20, ("Response %ld shorter than %ld length field!\n", iolen, len));

        if(iolen == 12)
        {
            KPRINTF(1, ("Assuming that data follows in a separate transfer.\n"));
        } else {
            pr->pr_IOErr = UHIOERR_RUNTPACKET;
            return(NULL);
        }
    }

    pr->pr_IOErr = 0;
    pr->pr_Length = len;
    pr->pr_ContainerType = AROS_WORD2LE(tmppr->pr_ContainerType);
    pr->pr_RespCode = AROS_WORD2LE(tmppr->pr_RespCode);
    if(pr->pr_TransID != AROS_LONG2LE(tmppr->pr_TransID))
    {
        KPRINTF(20, ("TransID %08lx != %08lx mismatch!\n", pr->pr_TransID, AROS_LONG2LE(tmppr->pr_TransID)));
    }

    KPRINTF(1, ("Container type %ld\n", pr->pr_ContainerType));

    if(pr->pr_ContainerType == PCT_DATA)
    {
        pr->pr_DataLength = len - 12;
        KPRINTF(1, ("Data %ld!\n", pr->pr_DataLength));
        buf = nAllocVec(nch, pr->pr_DataLength);
        if(!buf)
        {
            pr->pr_IOErr = UHIOERR_OUTOFMEMORY;
            return(NULL);
        }
        CopyMem(&nch->nch_InBuf[12], buf, iolen - 12);
        if(len > iolen)
        {
            retry = 3;
            do
            {
                ioerr = psdDoPipe(nch->nch_EPInPipe, &buf[iolen - 12], len - iolen);
                if(!ioerr)
                {
                    break;
                }
            } while(--retry);
            
            if(ioerr)
            {
                psdAddErrorMsg(RETURN_ERROR, (STRPTR) libname,
                               "Reading Data failed: %s (%ld)!",
                               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                pr->pr_IOErr = ioerr;
                nFreeVec(nch, buf);
                return(NULL);
            }
        }
        if(!(len % nch->nch_EPInPktSize))
        {
            KPRINTF(10, ("Reading zero packet\n"));
            ioerr = psdDoPipe(nch->nch_EPInPipe, NULL, 0);
        }
        DB(dumpmem(buf, (len < 256+12) ? len - 12 : 256));
        KPRINTF(1, ("Got Data. Getting Response\n"));
        retry = 3;
        do
        {
            ioerr = psdDoPipe(nch->nch_EPInPipe, nch->nch_InBuf, 512);
            if(!ioerr)
            {
                break;
            }
        } while(--retry);
        if(ioerr)
        {
            psdAddErrorMsg(RETURN_ERROR, (STRPTR) libname,
                           "Reading Response failed: %s (%ld)!",
                           psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
            pr->pr_IOErr = ioerr;
            nFreeVec(nch, buf);
            return(NULL);
        }

        iolen = psdGetPipeActual(nch->nch_EPInPipe);
        if(iolen < 12)
        {
            KPRINTF(20, ("Response %ld too short!\n", iolen));
            pr->pr_IOErr = UHIOERR_RUNTPACKET;
            nFreeVec(nch, buf);
            return(NULL);
        }
        len = AROS_LONG2LE(tmppr->pr_Length);
        if(iolen > len)
        {
            KPRINTF(20, ("Response %ld exceeds %ld length field!\n", iolen , len));
            pr->pr_IOErr = UHIOERR_RUNTPACKET;
            nFreeVec(nch, buf);
            return(NULL);
        }

        if((len < 512) && (iolen < len))
        {
            KPRINTF(20, ("Response %ld shorter than %ld length field!\n", iolen, len));
            pr->pr_IOErr = UHIOERR_RUNTPACKET;
            nFreeVec(nch, buf);
            return(NULL);
        }

        pr->pr_IOErr = 0;
        pr->pr_Length = len;
        pr->pr_ContainerType = AROS_WORD2LE(tmppr->pr_ContainerType);
        pr->pr_RespCode = AROS_WORD2LE(tmppr->pr_RespCode);
        if(pr->pr_TransID != AROS_LONG2LE(tmppr->pr_TransID))
        {
            KPRINTF(20, ("TransID %08lx != %08lx mismatch!\n", pr->pr_TransID, AROS_LONG2LE(tmppr->pr_TransID)));
        }

        KPRINTF(1, ("Container 2 type %ld\n", pr->pr_ContainerType));
    }

    if(pr->pr_ContainerType == PCT_RESPONSE)
    {
        pr->pr_NumParam = (len - 12)>>2;
        KPRINTF(5, ("Response %04lx (%s) with %ld params\n",
                 pr->pr_RespCode,
                 nNumToStr(nch, NTS_RESPCODE, (ULONG) pr->pr_RespCode, "unknown"),
                 pr->pr_NumParam));
        if(pr->pr_NumParam > 5)
        {
            pr->pr_NumParam = 5;
            KPRINTF(10, ("Parameter overflow!\n"));
        }
 
        for(cnt = 0; cnt < pr->pr_NumParam; cnt++)
        {
            pr->pr_Param[cnt] = AROS_LONG2LE(tmppr->pr_Param[cnt]);
            KPRINTF(1, ("P%ld: %08lx\n", cnt, pr->pr_Param[cnt]));
        }
        return(buf);
    }

    return(buf);
}
/* \\\ */

/* /// "nGetPTPRspData()" */
UBYTE * nGetPTPRspData(struct NepClassPTP *nch, struct PTPResponse *pr, UBYTE *buf, ULONG buflen)
{
    ULONG len;
    ULONG iolen;
    ULONG remlen;
    LONG ioerr;
    UWORD cnt;
    struct PTPResponse *tmppr = (struct PTPResponse *) nch->nch_InBuf;
    UWORD retry = 3;

    pr->pr_DataLength = 0;
    pr->pr_RespCode = PRC_UNDEFINED;

    KPRINTF(1, ("Getting Response, buffer %08lx length %ld\n", buf, buflen));
    do
    {
        ioerr = psdDoPipe(nch->nch_EPInPipe, nch->nch_InBuf, 512);
        if(!ioerr)
        {
            break;
        } else {
            KPRINTF(10, ("IOerror %ld, actual=%ld\n", ioerr, psdGetPipeActual(nch->nch_EPInPipe)));
        }
    } while(--retry);

    if(ioerr)
    {
        psdAddErrorMsg(RETURN_ERROR, (STRPTR) libname,
                       "Reading Response failed: %s (%ld)!",
                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
        pr->pr_IOErr = ioerr;
        return(NULL);
    }

    iolen = psdGetPipeActual(nch->nch_EPInPipe);
    KPRINTF(5, ("Response received %ld\n", iolen));
    DB(dumpmem(nch->nch_InBuf, iolen));
    if(iolen < 12)
    {
        KPRINTF(20, ("Response %ld too short!\n", iolen));
        pr->pr_IOErr = UHIOERR_RUNTPACKET;
        return(NULL);
    }
    len = AROS_LONG2LE(tmppr->pr_Length);
    KPRINTF(10, ("Total response %ld\n", len));
    if(iolen > len)
    {
        KPRINTF(20, ("Response %ld exceeds %ld length field!\n", iolen , len));
        pr->pr_IOErr = UHIOERR_RUNTPACKET;
        return(NULL);
    }

    if((len < 512) && (iolen < len))
    {
        KPRINTF(20, ("Response %ld shorter than %ld length field!\n", iolen, len));
        if(iolen == 12)
        {
            KPRINTF(20, ("Assuming that data follows in a separate transfer.\n"));
        } else {
            pr->pr_IOErr = UHIOERR_RUNTPACKET;
            return(NULL);
        }
    }

    pr->pr_IOErr = 0;
    pr->pr_Length = len;
    pr->pr_ContainerType = AROS_WORD2LE(tmppr->pr_ContainerType);
    pr->pr_RespCode = AROS_WORD2LE(tmppr->pr_RespCode);
    if(pr->pr_TransID != AROS_LONG2LE(tmppr->pr_TransID))
    {
        KPRINTF(20, ("TransID %08lx != %08lx mismatch!\n", pr->pr_TransID, AROS_LONG2LE(tmppr->pr_TransID)));
    }

    KPRINTF(1, ("Container type %ld\n", pr->pr_ContainerType));

    if(pr->pr_ContainerType == PCT_DATA)
    {
        pr->pr_DataLength = len - 12;
        KPRINTF(5, ("Data %ld!\n", pr->pr_DataLength));
        if(pr->pr_DataLength > buflen)
        {
            KPRINTF(10, ("Buffer too small to hold complete data\n"));
        }
        if(buflen <= iolen - 12)
        {
            KPRINTF(10, ("Very small buffer\n"));
            CopyMem(&nch->nch_InBuf[12], buf, buflen);
        } else {
            CopyMem(&nch->nch_InBuf[12], buf, iolen - 12);
        }
        if(len > iolen)
        {
            if(buflen == len - 12)
            {
                retry = 3;
                do
                {
                    ioerr = psdDoPipe(nch->nch_EPInPipe, &buf[iolen - 12], len - iolen + 12);
                    if(!ioerr)
                    {
                        break;
                    }
                } while(--retry);
            } else {
                remlen = buflen - iolen + 12;
                if(remlen & ~511)
                {
                    ioerr = psdDoPipe(nch->nch_EPInPipe, &buf[iolen - 12], remlen & ~511);
                }
                if(!ioerr)
                {
                    ioerr = psdDoPipe(nch->nch_EPInPipe, nch->nch_InBuf, 512);
                    if(!ioerr)
                    {
                        CopyMem(nch->nch_EPInPipe, &buf[iolen - 12 + (remlen & ~511)], remlen & 511);
                    }
                }
            }
            if(ioerr)
            {
                psdAddErrorMsg(RETURN_ERROR, (STRPTR) libname,
                               "Reading Data failed: %s (%ld)!",
                               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                pr->pr_IOErr = ioerr;
                nFreeVec(nch, buf);
                return(NULL);
            }
        }
        if(!(len % nch->nch_EPInPktSize))
        {
            KPRINTF(10, ("Reading zero packet\n"));
            ioerr = psdDoPipe(nch->nch_EPInPipe, NULL, 0);
        }
        //DB(dumpmem(buf, len - 12));
        KPRINTF(5, ("Got Data. Getting Response\n"));
        retry = 3;
        do
        {
            ioerr = psdDoPipe(nch->nch_EPInPipe, nch->nch_InBuf, 512);
            if(!ioerr)
            {
                break;
            }
        } while(--retry);
        if(ioerr)
        {
            psdAddErrorMsg(RETURN_ERROR, (STRPTR) libname,
                           "Reading Response failed: %s (%ld)!",
                           psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
            pr->pr_IOErr = ioerr;
            return(NULL);
        }

        iolen = psdGetPipeActual(nch->nch_EPInPipe);
        if(iolen < 12)
        {
            KPRINTF(20, ("Response %ld too short!\n", iolen));
            pr->pr_IOErr = UHIOERR_RUNTPACKET;
            return(NULL);
        }
        len = AROS_LONG2LE(tmppr->pr_Length);
        if(iolen > len)
        {
            KPRINTF(20, ("Response %ld exceeds %ld length field!\n", iolen , len));
            pr->pr_IOErr = UHIOERR_RUNTPACKET;
            return(NULL);
        }

        if((len < 512) && (iolen < len))
        {
            KPRINTF(20, ("Response %ld shorter than %ld length field!\n", iolen, len));
            pr->pr_IOErr = UHIOERR_RUNTPACKET;
            return(NULL);
        }

        pr->pr_IOErr = 0;
        pr->pr_Length = len;
        pr->pr_ContainerType = AROS_WORD2LE(tmppr->pr_ContainerType);
        pr->pr_RespCode = AROS_WORD2LE(tmppr->pr_RespCode);
        if(pr->pr_TransID != AROS_LONG2LE(tmppr->pr_TransID))
        {
            KPRINTF(20, ("TransID %08lx != %08lx mismatch!\n", pr->pr_TransID, AROS_LONG2LE(tmppr->pr_TransID)));
        }

        KPRINTF(1, ("Container 2 type %ld\n", pr->pr_ContainerType));
    }

    if(pr->pr_ContainerType == PCT_RESPONSE)
    {
        pr->pr_NumParam = (len - 12)>>2;
        KPRINTF(5, ("Response %04lx (%s) with %ld params\n",
                 pr->pr_RespCode,
                 nNumToStr(nch, NTS_RESPCODE, (ULONG) pr->pr_RespCode, "unknown"),
                 pr->pr_NumParam));
        if(pr->pr_NumParam > 5)
        {
            pr->pr_NumParam = 5;
            KPRINTF(10, ("Parameter overflow!\n"));
        }

        for(cnt = 0; cnt < pr->pr_NumParam; cnt++)
        {
            pr->pr_Param[cnt] = AROS_LONG2LE(tmppr->pr_Param[cnt]);
            KPRINTF(1, ("P%ld: %08lx\n", cnt, pr->pr_Param[cnt]));
        }
        return(buf);
    }

    return(buf);
}
/* \\\ */

/* /// "nGetPimaStr()" */
STRPTR nGetPimaStr(struct NepClassPTP *nch, UBYTE *bufptr, STRPTR emptystr)
{
    ULONG len = *bufptr++;
    STRPTR res;
    STRPTR strptr;
    UWORD widechar;

    if(!len)
    {
        if(!emptystr)
        {
            return(NULL);
        }
        res = nAllocVec(nch, (ULONG) strlen(emptystr) + 1);
        if(!res)
        {
            return(NULL);
        }
        strcpy(res, emptystr);
        return(res);
    }

    res = nAllocVec(nch, len + 1);
    if(!res)
    {
        return(NULL);
    }

    strptr = res;
    while(len)
    {
        widechar = *bufptr++;
        widechar |= (*bufptr++)<<8;
        if(!widechar)
        {
            break;
        }
        if((widechar > 255) || (widechar < 32))
        {
            widechar = '?';
        }
        *strptr++ = widechar;
        len--;
    }
    *strptr = 0;
    return(res);
}
/* \\\ */

/* /// "nGetPimaStrFiltered()" */
STRPTR nGetPimaStrFiltered(struct NepClassPTP *nch, UBYTE *bufptr)
{
    ULONG len = *bufptr++;
    STRPTR res = nAllocVec(nch, len + 1);
    STRPTR strptr = res;
    UWORD widechar;

    if(!res)
    {
        return(NULL);
    }

    while(len)
    {
        widechar = *bufptr++;
        widechar |= (*bufptr++)<<8;
        if(!widechar)
        {
            break;
        }
        if((widechar > 255) || (widechar < 32))
        {
            widechar = '_';
        }
        if((widechar == '/') || (widechar == ':') ||
           (widechar == '*') || (widechar == '"') ||
           (widechar == '\'') || (widechar == '>') ||
           (widechar == '<') || (widechar == '\\'))
        {
            // ignore
        } else {
            *strptr++ = widechar;
        }
        len--;
    }
    *strptr = 0;
    return(res);
}
/* \\\ */

/* /// "nWritePimaStr()" */
UBYTE * nWritePimaStr(struct NepClassPTP *nch, UBYTE *bufptr, STRPTR strptr)
{
    ULONG len;
    STRPTR srcptr = strptr;

    if(!strptr)
    {
        *bufptr++ = 0;
        return(bufptr);
    }

    len = 1;
    while(*srcptr++)
    {
        len++;
    }

    *bufptr++ = len;
    while(*strptr)
    {
        *bufptr++ = *strptr++;
        *bufptr++ = 0;
    }
    *bufptr++ = 0;
    *bufptr++ = 0;
    return(bufptr);
}
/* \\\ */

/* /// "nCheckPTPError()" */
BOOL nCheckPTPError(struct NepClassPTP *nch, struct PTPOp *po, struct PTPResponse *pr)
{
    if(pr->pr_IOErr)
    {
        psdAddErrorMsg(RETURN_ERROR, (STRPTR) libname,
                       "Command '%s' (%04lx) response failed: %s (%ld)!",
                       nNumToStr(nch, NTS_OPCODE, (ULONG) po->po_OpCode, "unknown"), po->po_OpCode,
                       psdNumToStr(NTS_IOERR, pr->pr_IOErr, "unknown"), pr->pr_IOErr);
        return(TRUE);
    }

    if(pr->pr_RespCode != PRC_OK)
    {
        psdAddErrorMsg(RETURN_ERROR, (STRPTR) libname,
                       "Command '%s' (%04lx) response failed: '%s' (%04lx).",
                       nNumToStr(nch, NTS_OPCODE, (ULONG) po->po_OpCode, "unknown"), po->po_OpCode,
                       nNumToStr(nch, NTS_RESPCODE, (ULONG) pr->pr_RespCode, "unknown"), pr->pr_RespCode);
        return(TRUE);
    }

    return(FALSE);
}
/* \\\ */

/* /// "nConvertArrayULONG()" */
BOOL nConvertArrayULONG(struct NepClassPTP *nch, UBYTE *buf, ULONG maxlen)
{
    ULONG cnt;
    ULONG *tmpptr = (ULONG *) buf;
    ULONG tmpval;

    if(maxlen < 4)
    {
        KPRINTF(20, ("Array crippled!\n", maxlen));
        return(FALSE);
    }
    maxlen -= 4;
    cnt = *buf++;
    cnt |= (*buf++)<<8;
    cnt |= (*buf++)<<16;
    cnt |= (*buf++)<<24;
    if(!cnt)
    {
        return(TRUE);
    }
    if((cnt<<2) > maxlen)
    {
        KPRINTF(20, ("Array exceeds buffer (%ld > %ld)!\n", cnt, maxlen>>2));
        cnt = maxlen>>2;
    }
    *tmpptr++ = cnt;
    if(cnt)
    {
        do
        {
            tmpval = *tmpptr;
            *tmpptr++ = AROS_LONG2LE(tmpval);
        } while(--cnt);
    }
    return(TRUE);
}
/* \\\ */

/* /// "nConvertPimaDate()" */
BOOL nConvertPimaDate(struct NepClassPTP *nch, struct DateStamp *ds, UBYTE *bufptr)
{
    struct NepPTPBase *nh = nch->nch_ClsBase;
    struct ClockData cd = { 0, 0, 0, 0, 0, 0, 0 };
    ULONG secs;
    UWORD ch;
    UWORD pos = 0;

    ds->ds_Days = 0;
    ds->ds_Minute = 0;
    ds->ds_Tick = 0;

    // 012345678901234
    // 20071126T135024
    if(*bufptr < 16) // minimum length required
    {
        DateStamp(ds);
        return(FALSE);
    }
    bufptr++;
    // parse YYYYMMDDThhmmss
    do
    {
        ch = *bufptr++;
        ch |= (*bufptr++)<<8;
        if((ch < '0') || (ch > '9'))
        {
            if(pos == 8)
            {
                 if(ch != 'T')
                 {
                     return(FALSE);
                 }
            } else {
                return(FALSE);
            }
        }
        ch -= '0';
        switch(pos)
        {
            case 1:
            case 2:
            case 3:
                cd.year *= 10;
            case 0:
                cd.year += ch;
                break;

            case 5:
                cd.month *= 10;
            case 4:
                cd.month += ch;
                break;

            case 7:
                cd.mday *= 10;
            case 6:
                cd.mday += ch;
                break;

            case 10:
                cd.hour *= 10;
            case 9:
                cd.hour += ch;
                break;

            case 12:
                cd.min *= 10;
            case 11:
                cd.min += ch;
                break;

            case 14:
                cd.sec *= 10;
            case 13:
                cd.sec += ch;
                break;
        }
    } while(++pos < 15);

    KPRINTF(1, ("Date: %02ld-%02ld-%04ld %02ld:%02ld:%02ld\n",
                cd.mday, cd.month, cd.year, cd.hour, cd.min, cd.sec));

    secs = CheckDate(&cd);
    if(!secs)
    {
        DateStamp(ds);
        return(FALSE);
    }

    ds->ds_Days = secs / (60*60*24);
    ds->ds_Minute = cd.min + cd.hour * 60;
    ds->ds_Tick = TICKS_PER_SECOND * cd.sec;

    KPRINTF(1, ("Days: %ld, Minute: %ld, Tick: %ld\n", ds->ds_Days, ds->ds_Minute, ds->ds_Tick));

    return(TRUE);
}
/* \\\ */

/* /// "nWritePimaDate()" */
UBYTE * nWritePimaDate(struct NepClassPTP *nch, UBYTE *bufptr, struct DateStamp *ds)
{
    struct NepPTPBase *nh = nch->nch_ClsBase;
    struct ClockData cd = { 0, 0, 0, 0, 0, 0, 0 };
    ULONG secs;
    UBYTE buf[20];

    secs = (ds->ds_Tick / TICKS_PER_SECOND) + ds->ds_Minute * 60 + ds->ds_Days * (60*60*24);
    Amiga2Date(secs, &cd);

    psdSafeRawDoFmt(buf, 20, "%04ld%02ld%02ldT%02ld%02ld%02ld", cd.year, cd.month, cd.mday, cd.hour, cd.min, cd.sec);

    bufptr = nWritePimaStr(nch, bufptr, buf);

    return(bufptr);
}
/* \\\ */

/* /// "nCalcNameHash()" */
UWORD nCalcNameHash(struct NepClassPTP *nch, STRPTR ptr)
{
    UWORD hash = 0x1234;
    if(!ptr)
    {
        return 0;
    }
    while(*ptr && (*ptr != '/') && (*ptr != ':'))
    {
        hash ^= (hash<<3)|(hash>>13);
        hash *= 13;
        hash ^= nch->nch_LowCharMap[*ptr++];
    }
    return(hash);
}
/* \\\ */

/* /// "nPTPGetDeviceInfo()" */
BOOL nPTPGetDeviceInfo(struct NepClassPTP *nch)
{
    struct PTPOp po;
    struct PTPResponse pr;
    UBYTE *buf;
    UBYTE *tmpptr;
    STRPTR srcptr;
    STRPTR tarptr;
    ULONG cnt;
    ULONG code;
    STRPTR manustr;
    STRPTR modelstr;
    STRPTR devversstr;
    STRPTR sernumstr;
    STRPTR devname;
    STRPTR devmanuname;
    LONG ioerr;

    po.po_OpCode = POC_GETDEVICEINFO;
    po.po_NumParam = 0;

    ioerr = nSendPTPCmd(nch, &po);
    if(!ioerr)
    {
        pr.pr_TransID = po.po_TransID;
        tmpptr = buf = nGetPTPRsp(nch, &pr);
        if(nCheckPTPError(nch, &po, &pr))
        {
            nFreeVec(nch, buf);
            return(FALSE);
        }
        if(buf)
        {
            tmpptr += 2+4+2;
            tmpptr += (*tmpptr++)<<1; // skip VendorExtensionDesc;
            tmpptr += 2;
            cnt = *tmpptr++;
            cnt |= (*tmpptr++)<<8;
            cnt |= (*tmpptr++)<<16;
            cnt |= (*tmpptr++)<<24;
            KPRINTF(10, ("OpCodes: %ld\n", cnt));
            nch->nch_DevCaps = 0;
            while(cnt)
            {
                code = *tmpptr++;
                code |= (*tmpptr++)<<8;
                switch(code)
                {
                    case POC_GETPARTIALOBJECT:
                        nch->nch_DevCaps |= PTPF_PARTIAL;
                        if(nch->nch_CDC->cdc_NoPartObj)
                        {
                            psdAddErrorMsg(RETURN_WARN, (STRPTR) libname, "Not fully enabling GetPartialObject even though device supports it (see GUI).");
                        }
                        break;

                    case POC_SENDOBJECTINFO:
                    case POC_SENDOBJECT:
                        nch->nch_DevCaps |= PTPF_SENDOBJ;
                        break;

                    case POC_DELETEOBJECT:
                        nch->nch_DevCaps |= PTPF_DELETEOBJ;
                        break;

                    case POC_SETOBJECTPROTECTION:
                        nch->nch_DevCaps |= PTPF_PROTOBJ;
                        break;

                    case POC_MOVEOBJECT:
                        nch->nch_DevCaps |= PTPF_MOVEOBJ;
                        break;

                    case POC_FORMATSTORE:
                        nch->nch_DevCaps |= PTPF_FORMAT;
                        break;

                }
                KPRINTF(1, ("%04lx: %s\n", code, nNumToStr(nch, NTS_OPCODE, code, "unknown")));
                cnt--;
            }
            cnt = *tmpptr++;
            cnt |= (*tmpptr++)<<8;
            cnt |= (*tmpptr++)<<16;
            cnt |= (*tmpptr++)<<24;
#if 0
            KPRINTF(10, ("EventCodes: %ld\n", cnt));
            while(cnt)
            {
                code = *tmpptr++;
                code |= (*tmpptr++)<<8;
                KPRINTF(1, ("%04lx: %s\n", code, nNumToStr(nch, NTS_EVENTCODE, code, "unknown")));
                cnt--;
            }
#else
            tmpptr += cnt<<1;
#endif

            cnt = *tmpptr++;
            cnt |= (*tmpptr++)<<8;
            cnt |= (*tmpptr++)<<16;
            cnt |= (*tmpptr++)<<24;
#if 0
            KPRINTF(10, ("DevicePropCodes: %ld\n", cnt));
            while(cnt)
            {
                code = *tmpptr++;
                code |= (*tmpptr++)<<8;
                KPRINTF(1, ("%04lx: %s\n", code, nNumToStr(nch, NTS_DEVICEPROPCODE, code, "unknown")));
                cnt--;
            }
#else
            tmpptr += cnt<<1;
#endif

            cnt = *tmpptr++;
            cnt |= (*tmpptr++)<<8;
            cnt |= (*tmpptr++)<<16;
            cnt |= (*tmpptr++)<<24;
#if 0
            KPRINTF(10, ("CaptureFormats: %ld\n", cnt));
            while(cnt)
            {
                code = *tmpptr++;
                code |= (*tmpptr++)<<8;
                KPRINTF(1, ("%04lx: %s\n", code, nNumToStr(nch, NTS_OBJECTFMTCODE, code, "unknown")));
                cnt--;
            }
#else
            tmpptr += cnt<<1;
#endif

            cnt = *tmpptr++;
            cnt |= (*tmpptr++)<<8;
            cnt |= (*tmpptr++)<<16;
            cnt |= (*tmpptr++)<<24;
#if 0
            KPRINTF(10, ("ImageFormats: %ld\n", cnt));
            while(cnt)
            {
                code = *tmpptr++;
                code |= (*tmpptr++)<<8;
                KPRINTF(1, ("%04lx: %s\n", code, nNumToStr(nch, NTS_OBJECTFMTCODE, code, "unknown")));
                cnt--;
            }
#else
            tmpptr += cnt<<1;
#endif

            psdGetAttrs(PGA_DEVICE, nch->nch_Device,
                        DA_ProductName, &devname,
                        DA_Manufacturer, &devmanuname,
                        TAG_END);

            manustr = nGetPimaStr(nch, tmpptr, devmanuname);
            tmpptr += (*tmpptr++)<<1; // skip Manufacturer
            modelstr = nGetPimaStr(nch, tmpptr, devname);
            tmpptr += (*tmpptr++)<<1; // skip Model
            devversstr = nGetPimaStr(nch, tmpptr, "?.??");
            tmpptr += (*tmpptr++)<<1; // skip Device Version
            sernumstr = nGetPimaStr(nch, tmpptr, "<none>");
            tmpptr += (*tmpptr++)<<1; // skip Serial Number

            // filter model for volume name
            cnt = 30;
            srcptr = modelstr;
            tarptr = nch->nch_VolumeName;

            do
            {
                switch(*srcptr)
                {
                    case ':':
                    case '/':
                    case '?':
                    case '"':
                    case '\'':
                        break;

                    default:
                        *tarptr++ = *srcptr;
                        --cnt;
                }
                srcptr++;
            } while(cnt);
            *tarptr = 0;

            nch->nch_RootObject.poi_Name = nch->nch_VolumeName;
            nch->nch_RootObject.poi_NameHash = nCalcNameHash(nch, nch->nch_VolumeName);

            psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                           "Model '%s' by '%s'.",
                           modelstr, manustr);
            psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                           "Camera Version '%s', Serial '%s'.",
                           devversstr, sernumstr);
            nFreeVec(nch, manustr);
            nFreeVec(nch, modelstr);
            nFreeVec(nch, devversstr);
            nFreeVec(nch, sernumstr);

            DB(dumpmem(buf, pr.pr_DataLength));
            nFreeVec(nch, buf);
            return(TRUE);
        }
    }
    return(FALSE);
}
/* \\\ */

/* /// "nPTPOpenSession()" */
BOOL nPTPOpenSession(struct NepClassPTP *nch)
{
    struct PTPOp po;
    struct PTPResponse pr;
    LONG ioerr;
    UBYTE *buf;

    nch->nch_SessionID = 0;
    nch->nch_TransID = 0;

    po.po_OpCode = POC_OPENSESSION;
    po.po_NumParam = 1;
    po.po_Param[0] = 1;

    ioerr = nSendPTPCmd(nch, &po);
    if(!ioerr)
    {
        pr.pr_TransID = po.po_TransID;
        buf = nGetPTPRsp(nch, &pr);
        if(nCheckPTPError(nch, &po, &pr))
        {
            nFreeVec(nch, buf);
            return(FALSE);
        }
        nFreeVec(nch, buf);
    }
    nch->nch_SessionID = 1;
    return(TRUE);
}
/* \\\ */

/* /// "nPTPCloseSession()" */
BOOL nPTPCloseSession(struct NepClassPTP *nch)
{
    struct PTPOp po;
    struct PTPResponse pr;
    LONG ioerr;
    UBYTE *buf;

    po.po_OpCode = POC_CLOSESESSION;
    po.po_NumParam = 0;

    ioerr = nSendPTPCmd(nch, &po);
    if(!ioerr)
    {
        pr.pr_TransID = po.po_TransID;
        buf = nGetPTPRsp(nch, &pr);
        if(nCheckPTPError(nch, &po, &pr))
        {
            nFreeVec(nch, buf);
            return(FALSE);
        }
        nFreeVec(nch, buf);
    }
    nch->nch_SessionID = 0;
    return(TRUE);
}
/* \\\ */

/* /// "nPTPGetObjectHandles()" */
ULONG * nPTPGetObjectHandles(struct NepClassPTP *nch, ULONG storageid, ULONG objhandle)
{
    struct PTPOp po;
    struct PTPResponse pr;
    LONG ioerr;
    UBYTE *buf;

    po.po_OpCode = POC_GETOBJECTHANDLES;
    po.po_NumParam = 3;
    po.po_Param[0] = storageid; // all storageIDs
    po.po_Param[1] = 0x00000000; // no special format type
    po.po_Param[2] = objhandle;  // directory ID

    ioerr = nSendPTPCmd(nch, &po);
    if(!ioerr)
    {
        pr.pr_TransID = po.po_TransID;
        buf = nGetPTPRsp(nch, &pr);
        if(nCheckPTPError(nch, &po, &pr))
        {
            nFreeVec(nch, buf);
            return(NULL);
        }
        if(nConvertArrayULONG(nch, buf, pr.pr_DataLength))
        {
            return((ULONG *) buf);
        }
        nFreeVec(nch, buf);
    }
    return(NULL);
}
/* \\\ */

/* /// "nPTPFreeObjectInfo()" */
void nPTPFreeObjectInfo(struct NepClassPTP *nch, struct PTPObjectInfo *poi)
{
    struct PTPObjectInfo *childpoi;
    if(!poi)
    {
        return;
    }
    childpoi = (struct PTPObjectInfo *) poi->poi_Children.mlh_Head;
    while(childpoi->poi_Node.mln_Succ)
    {
        Remove((struct Node *) childpoi);
        nPTPFreeObjectInfo(nch, childpoi);
        childpoi = (struct PTPObjectInfo *) poi->poi_Children.mlh_Head;
    }
    nFreeVec(nch, poi->poi_Name);
    nFreeVec(nch, poi->poi_Keywords);
    nFreeVec(nch, poi);
}
/* \\\ */

/* /// "nPTPCheckObjectInUse()" */
BOOL nPTPCheckObjectInUse(struct NepClassPTP *nch, struct PTPObjectInfo *poi)
{
    struct PTPObjectInfo *childpoi;
    BOOL res = FALSE;
    if(!poi)
    {
        return FALSE;
    }
    if(poi->poi_Flags & (PTPF_SHARED_LOCK|PTPF_EXCL_LOCK))
    {
        res = TRUE;
    }
    childpoi = (struct PTPObjectInfo *) poi->poi_Children.mlh_Head;
    while(childpoi->poi_Node.mln_Succ)
    {
        res |= nPTPCheckObjectInUse(nch, childpoi);
        childpoi = (struct PTPObjectInfo *) childpoi->poi_Node.mln_Succ;
    }
    return(res);
}
/* \\\ */

/* /// "nPTPGetObjectInfo()" */
struct PTPObjectInfo * nPTPGetObjectInfo(struct NepClassPTP *nch, ULONG objhandle)
{
    struct PTPOp po;
    struct PTPResponse pr;
    LONG ioerr;
    UBYTE *buf;
    UBYTE *bufptr;
    struct PTPObjectInfo *poi;
    UWORD protstate;

    po.po_OpCode = POC_GETOBJECTINFO;
    po.po_NumParam = 1;
    po.po_Param[0] = objhandle;  // object or directory ID

    ioerr = nSendPTPCmd(nch, &po);
    if(!ioerr)
    {
        pr.pr_TransID = po.po_TransID;
        bufptr = buf = nGetPTPRsp(nch, &pr);
        if(nCheckPTPError(nch, &po, &pr))
        {
            nFreeVec(nch, buf);
            return(NULL);
        }
        poi = nAllocVec(nch, sizeof(struct PTPObjectInfo));
        if(!poi)
        {
            nFreeVec(nch, buf);
            return(NULL);
        }
        NewList((struct List *) &poi->poi_Children);
        poi->poi_Handle = objhandle;
        poi->poi_StorageID = bufptr[0]|(bufptr[1]<<8)|(bufptr[2]<<16)|(bufptr[3]<<24);
        bufptr += 4;
        poi->poi_ObjectFmt = bufptr[0]|(bufptr[1]<<8);
        bufptr += 2;
        protstate = bufptr[0]|(bufptr[1]<<8);
        bufptr += 2;
        if(protstate == 0x0001)
        {
            poi->poi_ProtFlags = FIBF_READ;
        } else {
            poi->poi_ProtFlags = FIBF_READ|FIBF_WRITE|FIBF_DELETE;
        }
        poi->poi_Size = bufptr[0]|(bufptr[1]<<8)|(bufptr[2]<<16)|(bufptr[3]<<24);
        bufptr += 4;
        poi->poi_ThumbFmt = bufptr[0]|(bufptr[1]<<8);
        bufptr += 2;
        poi->poi_ThumbSize = bufptr[0]|(bufptr[1]<<8)|(bufptr[2]<<16)|(bufptr[3]<<24);
        bufptr += 4;
        poi->poi_ThumbWidth = bufptr[0]|(bufptr[1]<<8)|(bufptr[2]<<16)|(bufptr[3]<<24);
        bufptr += 4;
        poi->poi_ThumbHeight = bufptr[0]|(bufptr[1]<<8)|(bufptr[2]<<16)|(bufptr[3]<<24);
        bufptr += 4;
        poi->poi_ImageWidth = bufptr[0]|(bufptr[1]<<8)|(bufptr[2]<<16)|(bufptr[3]<<24);
        bufptr += 4;
        poi->poi_ImageHeight = bufptr[0]|(bufptr[1]<<8)|(bufptr[2]<<16)|(bufptr[3]<<24);
        bufptr += 4;
        poi->poi_ImageDepth = bufptr[0]|(bufptr[1]<<8)|(bufptr[2]<<16)|(bufptr[3]<<24);
        bufptr += 4;

        poi->poi_ParentHandle = bufptr[0]|(bufptr[1]<<8)|(bufptr[2]<<16)|(bufptr[3]<<24);
        bufptr += 4;

        //poi->poi_AssType = bufptr[0]|(bufptr[1]<<8);
        bufptr += 2;
        //poi->poi_AssDesc = bufptr[0]|(bufptr[1]<<8)|(bufptr[2]<<16)|(bufptr[3]<<24);
        bufptr += 4;
        //poi->poi_SeqNumber = bufptr[0]|(bufptr[1]<<8)|(bufptr[2]<<16)|(bufptr[3]<<24);
        bufptr += 4;
        if(*bufptr)
        {
            poi->poi_Name = nGetPimaStrFiltered(nch, bufptr);
        } else {
            poi->poi_Name = nPsdStrToNStr(nch, psdCopyStrFmt("Object_%08lx", poi->poi_StorageID));
        }
        bufptr += (*bufptr++)<<1; // skip Filename
        nConvertPimaDate(nch, &poi->poi_CapDate, bufptr);
        bufptr += (*bufptr++)<<1; // skip Capture Date
        nConvertPimaDate(nch, &poi->poi_ModDate, bufptr);
        bufptr += (*bufptr++)<<1; // skip Modification Date
        if(*bufptr)
        {
            poi->poi_Keywords = nGetPimaStr(nch, bufptr, NULL);
        } else {
            poi->poi_Keywords = NULL;
        }
        bufptr += (*bufptr++)<<1; // skip Keywords

        poi->poi_NameHash = nCalcNameHash(nch, poi->poi_Name);

        KPRINTF(5, ("Object %08lx: %s (%04lx), Parent=%08lx, Size %ld, Key=%s\n",
                    poi->poi_Handle, poi->poi_Name, poi->poi_ObjectFmt, poi->poi_ParentHandle,
                    poi->poi_Size, poi->poi_Keywords));

        KPRINTF(1, ("Thumb (%ld x %ld, %ld bytes), Image (%ld x %ld x %ld bits)\n",
                    poi->poi_ThumbWidth, poi->poi_ThumbHeight, poi->poi_ThumbSize,
                    poi->poi_ImageWidth, poi->poi_ImageHeight, poi->poi_ImageDepth));

        /*KPRINTF(10, ("Parent %08lx, AssType=%04lx, AssDesc=%08lx\n",
                     poi->poi_ParentHandle, poi->poi_AssType, poi->poi_AssDesc));*/

        return(poi);
    }
    return(NULL);
}
/* \\\ */

/* /// "nPTPSendObject()" */
struct PTPObjectInfo * nPTPSendObject(struct NepClassPTP *nch, struct DosPacket *dp, struct PTPObjectInfo *poi, UBYTE *databuf)
{
    struct PTPOp po;
    struct PTPResponse pr;
    LONG ioerr;
    UBYTE *buf;
    UBYTE *bufptr;
    UWORD cnt;

    do
    {
        po.po_OpCode = POC_SENDOBJECTINFO;
        po.po_NumParam = 2;
        switch(nch->nch_NoDestMode)
        {
            case 0:
                po.po_Param[0] = poi->poi_StorageID;
                po.po_Param[1] = poi->poi_ParentHandle;
                break;

            case 1:
                po.po_Param[0] = poi->poi_StorageID;
                po.po_Param[1] = 0;
                break;

            case 2:
                po.po_Param[0] = 0;
                po.po_Param[1] = 0;
                break;
        }
        ioerr = nSendPTPCmd(nch, &po);
        if(ioerr)
        {
            dp->dp_Res2 = ERROR_NOT_A_DOS_DISK;
            return(NULL);
        }
        bufptr = buf = nch->nch_OutBuf;
        *bufptr++ = poi->poi_StorageID;
        *bufptr++ = poi->poi_StorageID>>8;
        *bufptr++ = poi->poi_StorageID>>16;
        *bufptr++ = poi->poi_StorageID>>24;
        *bufptr++ = poi->poi_ObjectFmt;
        *bufptr++ = poi->poi_ObjectFmt>>8;
        if(poi->poi_ProtFlags & (FIBF_WRITE|FIBF_DELETE))
        {
            *bufptr++ = 0;
            *bufptr++ = 0;
        } else {
            *bufptr++ = 1;
            *bufptr++ = 0;
        }
        *bufptr++ = poi->poi_Size;
        *bufptr++ = poi->poi_Size>>8;
        *bufptr++ = poi->poi_Size>>16;
        *bufptr++ = poi->poi_Size>>24;

        cnt = 2+4+4+4; // thumb
        do
        {
            *bufptr++ = 0;
        } while(--cnt);

        *bufptr++ = poi->poi_ImageWidth;
        *bufptr++ = poi->poi_ImageWidth>>8;
        *bufptr++ = poi->poi_ImageWidth>>16;
        *bufptr++ = poi->poi_ImageWidth>>24;
        *bufptr++ = poi->poi_ImageHeight;
        *bufptr++ = poi->poi_ImageHeight>>8;
        *bufptr++ = poi->poi_ImageHeight>>16;
        *bufptr++ = poi->poi_ImageHeight>>24;
        *bufptr++ = poi->poi_ImageDepth;
        *bufptr++ = poi->poi_ImageDepth>>8;
        *bufptr++ = poi->poi_ImageDepth>>16;
        *bufptr++ = poi->poi_ImageDepth>>24;

        *bufptr++ = poi->poi_ParentHandle;
        *bufptr++ = poi->poi_ParentHandle>>8;
        *bufptr++ = poi->poi_ParentHandle>>16;
        *bufptr++ = poi->poi_ParentHandle>>24;

        if(poi->poi_ObjectFmt != POF_ASSOCIATION)
        {
            cnt = 2+4+4; // ass typ, desc, seq
        } else {
            *bufptr++ = 0x01; // generic folder
            *bufptr++ = 0x00;
            cnt = 4+4;
        }
        do
        {
            *bufptr++ = 0;
        } while(--cnt);

        // filename
        bufptr = nWritePimaStr(nch, bufptr, poi->poi_Name);
        bufptr = nWritePimaDate(nch, bufptr, &poi->poi_CapDate);
        bufptr = nWritePimaDate(nch, bufptr, &poi->poi_ModDate);
        bufptr = nWritePimaStr(nch, bufptr, poi->poi_Keywords);

        ioerr = nSendPTPData(nch, &po, buf, bufptr - buf);
        if(ioerr)
        {
            dp->dp_Res2 = ERROR_NOT_A_DOS_DISK;
            return(NULL);
        }

        pr.pr_TransID = po.po_TransID;
        buf = nGetPTPRsp(nch, &pr);
        nFreeVec(nch, buf);
        if(nCheckPTPError(nch, &po, &pr))
        {
            switch(pr.pr_RespCode)
            {
                case PRC_STOREREADONLY:
                    dp->dp_Res2 = ERROR_DISK_WRITE_PROTECTED;
                    break;

                case PRC_ACCESSDENIED:
                    dp->dp_Res2 = ERROR_WRITE_PROTECTED;
                    break;

                case PRC_STOREFULL:
                    dp->dp_Res2 = ERROR_DISK_FULL;
                    break;

                case PRC_INVALIDOBJECTFORMATCODE:
                    dp->dp_Res2 = ERROR_OBJECT_WRONG_TYPE;
                    break;

                case PRC_SPECIFICATIONOFDESTINATIONUNSUPPORTED:
                    if(nch->nch_NoDestMode < 2)
                    {
                        nch->nch_NoDestMode++;
                        break;
                    }

                default:
                    dp->dp_Res2 = ERROR_NOT_A_DOS_DISK;
            }
            if(dp->dp_Res2)
            {
                return(NULL);
            }
            continue; // retry
        }
        break;
    } while(TRUE);

    poi->poi_StorageID = pr.pr_Param[0];
    if(poi->poi_ParentHandle != pr.pr_Param[1])
    {
        KPRINTF(20, ("Parent Handle %ld != %ld!\n", poi->poi_ParentHandle, pr.pr_Param[1]));
        poi->poi_ParentHandle = pr.pr_Param[1];
    }
    poi->poi_Handle = pr.pr_Param[2];

    if(!poi->poi_Size)
    {
        KPRINTF(5, ("No data stage as size is zero\n"));
        return(poi);
    }

    po.po_OpCode = POC_SENDOBJECT;
    po.po_NumParam = 0;

    ioerr = nSendPTPCmd(nch, &po);
    if(ioerr)
    {
        dp->dp_Res2 = ERROR_NOT_A_DOS_DISK;
        return(NULL);
    }

    ioerr = nSendPTPData(nch, &po, databuf, poi->poi_Size);
    if(ioerr)
    {
        dp->dp_Res2 = ERROR_NOT_A_DOS_DISK;
        return(NULL);
    }
    pr.pr_TransID = po.po_TransID;
    buf = nGetPTPRsp(nch, &pr);
    nFreeVec(nch, buf);
    if(nCheckPTPError(nch, &po, &pr))
    {
        switch(pr.pr_RespCode)
        {
            case PRC_STOREFULL:
                dp->dp_Res2 = ERROR_DISK_FULL;
                break;

            default:
                dp->dp_Res2 = ERROR_NOT_A_DOS_DISK;
        }
        return(NULL);
    }

    return(poi);
}
/* \\\ */

/* /// "nPTPGetStorageIDs()" */
ULONG * nPTPGetStorageIDs(struct NepClassPTP *nch)
{
    struct PTPOp po;
    struct PTPResponse pr;
    LONG ioerr;
    UBYTE *buf;

    po.po_OpCode = POC_GETSTORAGEIDS;
    po.po_NumParam = 0;

    ioerr = nSendPTPCmd(nch, &po);
    if(!ioerr)
    {
        pr.pr_TransID = po.po_TransID;
        buf = nGetPTPRsp(nch, &pr);
        if(nCheckPTPError(nch, &po, &pr))
        {
            nFreeVec(nch, buf);
            return(NULL);
        }
        if(nConvertArrayULONG(nch, buf, pr.pr_DataLength))
        {
            return((ULONG *) buf);
        }
        nFreeVec(nch, buf);
    }
    return(NULL);
}
/* \\\ */

/* /// "nPTPFreeStorageInfo()" */
void nPTPFreeStorageInfo(struct NepClassPTP *nch, struct PTPStorageInfo *psi)
{
    if(!psi)
    {
        return;
    }
    Remove((struct Node *) psi->psi_ObjectInfo);
    nPTPFreeObjectInfo(nch, psi->psi_ObjectInfo);
    nFreeVec(nch, psi->psi_VolumeName);
    nFreeVec(nch, psi->psi_StorageDesc);
    nFreeVec(nch, psi);
}
/* \\\ */

/* /// "nPTPGetStorageInfo()" */
struct PTPStorageInfo * nPTPGetStorageInfo(struct NepClassPTP *nch, ULONG storageid)
{
    struct PTPOp po;
    struct PTPResponse pr;
    LONG ioerr;
    UBYTE *buf;
    UBYTE *bufptr;
    struct PTPStorageInfo *psi;
    struct PTPObjectInfo *poi;

    po.po_OpCode = POC_GETSTORAGEINFO;
    po.po_NumParam = 1;
    po.po_Param[0] = storageid;

    ioerr = nSendPTPCmd(nch, &po);
    if(!ioerr)
    {
        pr.pr_TransID = po.po_TransID;
        bufptr = buf = nGetPTPRsp(nch, &pr);
        if(nCheckPTPError(nch, &po, &pr))
        {
            nFreeVec(nch, buf);
            return(NULL);
        }
        psi = nAllocVec(nch, sizeof(struct PTPStorageInfo));
        if(!psi)
        {
            nFreeVec(nch, buf);
            return(NULL);
        }
        poi = nAllocVec(nch, sizeof(struct PTPObjectInfo));
        if(!poi)
        {
            nFreeVec(nch, psi);
            nFreeVec(nch, buf);
            return(NULL);
        }
        NewList((struct List *) &poi->poi_Children);
        psi->psi_ObjectInfo = poi;

        psi->psi_StorageID = storageid;
        psi->psi_StorageType = bufptr[0]|(bufptr[1]<<8);
        bufptr += 2;
        psi->psi_FSType = bufptr[0]|(bufptr[1]<<8);
        bufptr += 2;
        psi->psi_AccessCaps = bufptr[0]|(bufptr[1]<<8);
        bufptr += 2;

        psi->psi_MaxCapacityL = bufptr[0]|(bufptr[1]<<8)|(bufptr[2]<<16)|(bufptr[3]<<24);
        psi->psi_MaxCapacityH = bufptr[4]|(bufptr[5]<<8)|(bufptr[6]<<16)|(bufptr[7]<<24);
        bufptr += 8;
        psi->psi_FreeL = bufptr[0]|(bufptr[1]<<8)|(bufptr[2]<<16)|(bufptr[3]<<24);
        psi->psi_FreeH = bufptr[4]|(bufptr[5]<<8)|(bufptr[6]<<16)|(bufptr[7]<<24);
        bufptr += 8;

        psi->psi_FreeImgs = bufptr[0]|(bufptr[1]<<8)|(bufptr[2]<<16)|(bufptr[3]<<24);
        bufptr += 4;

        if(*bufptr)
        {
            psi->psi_StorageDesc = nGetPimaStr(nch, bufptr, NULL);
        } else {
            psi->psi_StorageDesc = nPsdStrToNStr(nch, psdCopyStrFmt("Storage_%08lx", psi->psi_StorageID));
        }
        bufptr += (*bufptr++)<<1; // skip Storage Descriptor

        if(*bufptr)
        {
            psi->psi_VolumeName = nGetPimaStrFiltered(nch, bufptr);
        } else {
            psi->psi_VolumeName = nPsdStrToNStr(nch, psdCopyStrFmt("Volume_%08lx", psi->psi_StorageID));
        }
        bufptr += (*bufptr++)<<1; // skip Volume Name

        KPRINTF(10, ("StorageID %08lx: %s (%s), StorageType=%04lx, FSType=%04lx, Caps=%04lx\n",
                     psi->psi_StorageID, psi->psi_VolumeName, psi->psi_StorageDesc,
                     psi->psi_StorageType, psi->psi_FSType,
                     psi->psi_AccessCaps));

        KPRINTF(10, ("Capacity %lu:%lu, Free %lu:%lu (%lu imgs)\n",
                     psi->psi_MaxCapacityH, psi->psi_MaxCapacityL,
                     psi->psi_FreeH, psi->psi_FreeL, psi->psi_FreeImgs));

        poi->poi_Handle = 0xffffffff; // as root
        poi->poi_StorageID = storageid;
        poi->poi_ObjectFmt = POF_ASSOCIATION; // directory
        poi->poi_ProtFlags = (psi->psi_AccessCaps == 0x0000) ? FIBF_READ|FIBF_WRITE|FIBF_DELETE : ((psi->psi_AccessCaps == 0x0001) ? FIBF_READ : FIBF_READ|FIBF_DELETE);
        nch->nch_RootObject.poi_ProtFlags |= poi->poi_ProtFlags;
        poi->poi_ParentHandle = 0; // no parent
        poi->poi_Name = nPsdStrToNStr(nch, psdCopyStr(psi->psi_VolumeName));
        DateStamp(&poi->poi_CapDate);
        DateStamp(&poi->poi_ModDate);
        poi->poi_Keywords = NULL;
        poi->poi_Parent = &nch->nch_RootObject;
        poi->poi_Flags = PTPF_NOPURGE;
        poi->poi_NameHash = nCalcNameHash(nch, poi->poi_Name);

        AddTail((struct List *) &nch->nch_RootObject.poi_Children, (struct Node *) poi);

        return(psi);
    }
    return(NULL);
}
/* \\\ */

/* /// "nPTPRead()" */
void nPTPRead(struct NepClassPTP *nch, struct DosPacket *dp, struct PTPFileHandle *pfh, UBYTE *buffer, ULONG len)
{
    struct PTPObjectInfo *poi = pfh->pfh_ObjectInfo;
    struct PTPOp po;
    struct PTPResponse pr;
    LONG ioerr;
    KPRINTF(10, ("Reading %ld at %ld (%08lx, '%s')\n", len, pfh->pfh_SeekPos, poi->poi_Handle, poi->poi_Name));

    if(!len)
    {
        KPRINTF(10, ("No data to read!\n"));
        dp->dp_Res1 = 0;
        dp->dp_Res2 = RETURN_OK;
        return;
    }

    if(pfh->pfh_SeekPos > poi->poi_Size)
    {
        KPRINTF(10, ("seek pos %ld past size %ld!\n", pfh->pfh_SeekPos, poi->poi_Size));
        dp->dp_Res1 = 0;
        dp->dp_Res2 = ERROR_SEEK_ERROR;
        return;
    }

    if(pfh->pfh_SeekPos + len > poi->poi_Size)
    {
        len = poi->poi_Size - pfh->pfh_SeekPos;
        KPRINTF(10, ("Reading past end, new length %ld!\n", len));
        if(!len)
        {
            dp->dp_Res1 = 0;
            dp->dp_Res2 = RETURN_OK;
            return;
        }
    }
    dp->dp_Res1 = len;

    if(pfh->pfh_Buffer)
    {
        CopyMem(&pfh->pfh_Buffer[pfh->pfh_SeekPos], buffer, len);
        pfh->pfh_SeekPos += len;
        return;
    }

    if((nch->nch_DevCaps & PTPF_PARTIAL) && (!(nch->nch_CDC->cdc_NoPartObj && (len > 32768))))
    {
        if(pfh->pfh_SeekPos || (len != poi->poi_Size))
        {
            KPRINTF(10, ("Using GetPartialObject\n"));
            po.po_OpCode = POC_GETPARTIALOBJECT;
            po.po_NumParam = 3;
            po.po_Param[0] = poi->poi_Handle;
            po.po_Param[1] = pfh->pfh_SeekPos;
            po.po_Param[2] = len;
        } else {
            KPRINTF(10, ("Using GetObject\n"));
            po.po_OpCode = POC_GETOBJECT;
            po.po_NumParam = 1;
            po.po_Param[0] = poi->poi_Handle;
        }
        ioerr = nSendPTPCmd(nch, &po);
        if(!ioerr)
        {
            pr.pr_TransID = po.po_TransID;
            nGetPTPRspData(nch, &pr, buffer, len);
            if(nCheckPTPError(nch, &po, &pr))
            {
                dp->dp_Res1 = 0;
                dp->dp_Res2 = ERROR_NOT_A_DOS_DISK;
                return;
            }
            pfh->pfh_SeekPos += len;
        } else {
            dp->dp_Res1 = 0;
            dp->dp_Res2 = ERROR_NOT_A_DOS_DISK;
            return;
        }
    } else {
        if(!pfh->pfh_Buffer)
        {
            if(nch->nch_LastObject == poi)
            {
                KPRINTF(10, ("Using Last Object Cache\n"));
                pfh->pfh_Buffer = nch->nch_LastBuffer;
                pfh->pfh_BufferLen = nch->nch_LastBufferLen;
                pfh->pfh_Flags |= PTPF_OLDLOADED;
                nch->nch_LastBuffer = NULL;
                nch->nch_LastObject = NULL; // make sure we don't try to reuse this value!
            } else {
                KPRINTF(10, ("Using GetObject\n"));
                po.po_OpCode = POC_GETOBJECT;
                po.po_NumParam = 1;
                po.po_Param[0] = poi->poi_Handle;
                ioerr = nSendPTPCmd(nch, &po);
                if(!ioerr)
                {
                    pr.pr_TransID = po.po_TransID;
                    pfh->pfh_Buffer = nGetPTPRsp(nch, &pr);
                    pfh->pfh_BufferLen = pr.pr_DataLength;
                    if(nCheckPTPError(nch, &po, &pr))
                    {
                        nFreeVec(nch, pfh->pfh_Buffer);
                        pfh->pfh_Buffer = NULL;
                        dp->dp_Res1 = 0;
                        dp->dp_Res2 = ERROR_NOT_A_DOS_DISK;
                        return;
                    }
                    pfh->pfh_Flags |= PTPF_OLDLOADED;
                } else {
                    dp->dp_Res1 = 0;
                    dp->dp_Res2 = ERROR_NOT_A_DOS_DISK;
                    return;
                }
            }
        }
        CopyMem(&pfh->pfh_Buffer[pfh->pfh_SeekPos], buffer, len);
        pfh->pfh_SeekPos += len;
    }
}
/* \\\ */

/* /// "nPTPWrite()" */
void nPTPWrite(struct NepClassPTP *nch, struct DosPacket *dp, struct PTPFileHandle *pfh, UBYTE *buffer, ULONG len)
{
    struct PTPObjectInfo *poi = pfh->pfh_ObjectInfo;
    struct PTPOp po;
    struct PTPResponse pr;
    LONG ioerr;
    UBYTE *newbuffer;
    ULONG newbuflen;
    KPRINTF(10, ("Write %ld at %ld (%08lx, '%s')\n", len, pfh->pfh_SeekPos, poi->poi_Handle, poi->poi_Name));

    if(!len)
    {
        KPRINTF(10, ("No data to write\n"));
        return;
    }

    if(pfh->pfh_SeekPos > poi->poi_Size)
    {
        KPRINTF(10, ("seek pos %ld past size %ld!\n", pfh->pfh_SeekPos, poi->poi_Size));
        dp->dp_Res1 = 0;
        dp->dp_Res2 = ERROR_SEEK_ERROR;
        return;
    }

    if(!pfh->pfh_Buffer)
    {
        pfh->pfh_BufferLen = 0; // make sure that we clear this value to avoid confusion
    }

    if(pfh->pfh_SeekPos + len > poi->poi_Size)
    {
        KPRINTF(10, ("Writing past end, new length %ld!\n", pfh->pfh_SeekPos + len));
        if(pfh->pfh_SeekPos + len > pfh->pfh_BufferLen)
        {
            newbuflen = pfh->pfh_BufferLen;
            if(newbuflen < poi->poi_Size)
            {
                newbuflen = poi->poi_Size; // make sure the buffer is at least the size of the file, when freshly allocating
            }
            while(newbuflen < pfh->pfh_SeekPos + len)
            {
                if(newbuflen <= (512<<10))
                {
                    if(newbuflen >= 65536)
                    {
                        newbuflen <<= 1; // double size of buffer
                    } else {
                        newbuflen = 65536;
                    }
                } else {
                    newbuflen += 512<<10; // increase buffer by 512 KB;
                }
            }
            KPRINTF(10, ("New Buffer length: %ld\n", newbuflen));
            newbuffer = nAllocVec(nch, newbuflen);
            if(!newbuffer)
            {
                dp->dp_Res1 = 0;
                dp->dp_Res2 = ERROR_NO_FREE_STORE;
                return;
            }
            if(pfh->pfh_Buffer)
            {
                // copy old material and free old buffer
                CopyMem(pfh->pfh_Buffer, newbuffer, poi->poi_Size);
                nFreeVec(nch, pfh->pfh_Buffer);
            }
            pfh->pfh_Buffer = newbuffer;
            pfh->pfh_BufferLen = newbuflen;
        }
    }
    dp->dp_Res1 = len;

    if(!(pfh->pfh_Flags & (PTPF_NEWFILE|PTPF_OLDLOADED)))
    {
        if(pfh->pfh_SeekPos || (len < poi->poi_Size))
        {
            KPRINTF(10, ("Loading old file using GetObject first\n"));
            po.po_OpCode = POC_GETOBJECT;
            po.po_NumParam = 1;
            po.po_Param[0] = poi->poi_Handle;
            ioerr = nSendPTPCmd(nch, &po);
            if(!ioerr)
            {
                pr.pr_TransID = po.po_TransID;
                nGetPTPRspData(nch, &pr, pfh->pfh_Buffer, poi->poi_Size);
                if(nCheckPTPError(nch, &po, &pr))
                {
                    dp->dp_Res1 = 0;
                    dp->dp_Res2 = ERROR_NOT_A_DOS_DISK;
                    return;
                }
                pfh->pfh_Flags |= PTPF_OLDLOADED;
            } else {
                dp->dp_Res1 = 0;
                dp->dp_Res2 = ERROR_NOT_A_DOS_DISK;
                return;
            }
        } else {
            KPRINTF(10, ("No need to load old file, overwriting all old data\n"));
            pfh->pfh_Flags |= PTPF_OLDLOADED;
        }
    }

    CopyMem(buffer, &pfh->pfh_Buffer[pfh->pfh_SeekPos], len);
    pfh->pfh_SeekPos += len;
    if(pfh->pfh_SeekPos > poi->poi_Size)
    {
        poi->poi_Size = pfh->pfh_SeekPos;
    }
    pfh->pfh_Flags |= PTPF_MODIFIED;
}
/* \\\ */

/* /// "nPTPSetObjectProtection()" */
void nPTPSetObjectProtection(struct NepClassPTP *nch, struct DosPacket *dp, struct PTPObjectInfo *poi, ULONG protflags)
{
    struct PTPOp po;
    struct PTPResponse pr;
    UBYTE *buf;
    LONG ioerr;
    KPRINTF(10, ("SetObjectProtection %04lx on (%08lx, '%s')\n", protflags, poi->poi_Handle, poi->poi_Name));

    if((protflags & (FIBF_WRITE|FIBF_DELETE)) != (poi->poi_ProtFlags & (FIBF_WRITE|FIBF_DELETE)))
    {
        po.po_OpCode = POC_SETOBJECTPROTECTION;
        po.po_NumParam = 2;
        po.po_Param[0] = poi->poi_Handle;
        if(protflags & (FIBF_WRITE|FIBF_DELETE))
        {
            po.po_Param[1] = 0x0000;
            protflags |= FIBF_WRITE|FIBF_DELETE;
        } else {
            po.po_Param[1] = 0x0001;
            protflags &= ~(FIBF_WRITE|FIBF_DELETE);
        }

        ioerr = nSendPTPCmd(nch, &po);
        if(!ioerr)
        {
            pr.pr_TransID = po.po_TransID;
            buf = nGetPTPRsp(nch, &pr);
            if(nCheckPTPError(nch, &po, &pr))
            {
                nFreeVec(nch, buf);
                switch(pr.pr_RespCode)
                {
                    case PRC_STOREREADONLY:
                        dp->dp_Res2 = ERROR_DISK_WRITE_PROTECTED;
                        break;

                    case PRC_ACCESSDENIED:
                        dp->dp_Res2 = ERROR_WRITE_PROTECTED;
                        break;

                    default:
                        dp->dp_Res2 = ERROR_NOT_A_DOS_DISK;
                }
                return;
            }
            nFreeVec(nch, buf);
        } else {
            dp->dp_Res2 = ERROR_NOT_A_DOS_DISK;
            return;
        }
    }
    poi->poi_ProtFlags = protflags;
}
/* \\\ */

/* /// "nPTPFormatStore()" */
void nPTPFormatStore(struct NepClassPTP *nch, struct DosPacket *dp)
{
    struct PTPOp po;
    struct PTPResponse pr;
    UBYTE *buf;
    LONG ioerr;
    struct PTPStorageInfo *psi;
    struct PTPStorageInfo *nextpsi;
    struct List newinfos;

    KPRINTF(10, ("FormatStore\n"));

    NewList(&newinfos);

    psi = (struct PTPStorageInfo *) nch->nch_Storages.lh_Head;
    while((nextpsi = (struct PTPStorageInfo *) psi->psi_Node.ln_Succ))
    {
        if(psi->psi_AccessCaps == 0x0000)
        {
            if(nPTPCheckObjectInUse(nch, psi->psi_ObjectInfo))
            {
                dp->dp_Res2 = ERROR_OBJECT_IN_USE;
            } else {
                po.po_OpCode = POC_FORMATSTORE;
                po.po_NumParam = 2;
                po.po_Param[0] = psi->psi_StorageID;
                po.po_Param[1] = 0;

                ioerr = nSendPTPCmd(nch, &po);
                if(!ioerr)
                {
                    pr.pr_TransID = po.po_TransID;
                    buf = nGetPTPRsp(nch, &pr);
                    if(nCheckPTPError(nch, &po, &pr))
                    {
                        switch(pr.pr_RespCode)
                        {
                            case PRC_STOREREADONLY:
                                dp->dp_Res2 = ERROR_DISK_WRITE_PROTECTED;
                                return;

                            case PRC_ACCESSDENIED:
                                dp->dp_Res2 = ERROR_WRITE_PROTECTED;
                                return;

                            default:
                                dp->dp_Res2 = ERROR_NOT_A_DOS_DISK;
                                return;
                        }
                    }
                    nFreeVec(nch, buf);
                    dp->dp_Res2 = 0;
                    Remove(&psi->psi_Node);
                    nPTPFreeStorageInfo(nch, psi);
                    psi = nPTPGetStorageInfo(nch, po.po_Param[0]);
                    if(psi)
                    {
                        AddTail(&newinfos, &psi->psi_Node);
                    }
                } else {
                    dp->dp_Res2 = ERROR_NOT_A_DOS_DISK;
                    return;
                }
            }
        }
        psi = nextpsi;
    }

    psi = (struct PTPStorageInfo *) newinfos.lh_Head;
    while((nextpsi = (struct PTPStorageInfo *) psi->psi_Node.ln_Succ))
    {
        Remove(&psi->psi_Node);
        AddTail(&nch->nch_Storages, &psi->psi_Node);
        psi = nextpsi;
    }
}
/* \\\ */

/* /// "nPTPMoveObject()" */
void nPTPMoveObject(struct NepClassPTP *nch, struct DosPacket *dp, struct PTPObjectInfo *poi, struct PTPObjectInfo *targetdir)
{
    struct PTPOp po;
    struct PTPResponse pr;
    UBYTE *buf;
    LONG ioerr;
    KPRINTF(10, ("MoveObject %08lx, '%s' to %08lx, '%s'\n", poi->poi_Handle, poi->poi_Name, targetdir->poi_Handle, targetdir->poi_Name));

    po.po_OpCode = POC_MOVEOBJECT;
    po.po_NumParam = 3;
    po.po_Param[0] = poi->poi_Handle;
    po.po_Param[1] = targetdir->poi_StorageID;
    po.po_Param[2] = targetdir->poi_Handle;
    ioerr = nSendPTPCmd(nch, &po);
    if(!ioerr)
    {
        pr.pr_TransID = po.po_TransID;
        buf = nGetPTPRsp(nch, &pr);
        if(nCheckPTPError(nch, &po, &pr))
        {
            nFreeVec(nch, buf);
            switch(pr.pr_RespCode)
            {
                case PRC_STOREREADONLY:
                    dp->dp_Res2 = ERROR_DISK_WRITE_PROTECTED;
                    break;

                case PRC_ACCESSDENIED:
                    dp->dp_Res2 = ERROR_WRITE_PROTECTED;
                    break;

                default:
                    dp->dp_Res2 = ERROR_NOT_A_DOS_DISK;
            }
            return;
        }
        nFreeVec(nch, buf);
        Remove((struct Node *) poi);
        AddTail((struct List *) &targetdir->poi_Children, (struct Node *) poi);
    } else {
        dp->dp_Res2 = ERROR_NOT_A_DOS_DISK;
        return;
    }
}
/* \\\ */

/* /// "nPTPDeleteObject()" */
void nPTPDeleteObject(struct NepClassPTP *nch, struct DosPacket *dp, struct PTPObjectInfo *poi)
{
    struct PTPOp po;
    struct PTPResponse pr;
    UBYTE *buf;
    LONG ioerr;
    KPRINTF(10, ("DeleteObject %08lx, '%s'\n", poi->poi_Handle, poi->poi_Name));

    po.po_OpCode = POC_DELETEOBJECT;
    po.po_NumParam = 2;
    po.po_Param[0] = poi->poi_Handle;
    po.po_Param[1] = 0;
    ioerr = nSendPTPCmd(nch, &po);
    if(!ioerr)
    {
        pr.pr_TransID = po.po_TransID;
        buf = nGetPTPRsp(nch, &pr);
        if(nCheckPTPError(nch, &po, &pr))
        {
            nFreeVec(nch, buf);
            switch(pr.pr_RespCode)
            {
                case PRC_STOREREADONLY:
                    dp->dp_Res2 = ERROR_DISK_WRITE_PROTECTED;
                    break;

                case PRC_ACCESSDENIED:
                case PRC_OBJECTWRITEPROTECTED:
                    dp->dp_Res2 = ERROR_WRITE_PROTECTED;
                    break;

                default:
                    dp->dp_Res2 = ERROR_NOT_A_DOS_DISK;
            }
            return;
        }
        nFreeVec(nch, buf);
    } else {
        dp->dp_Res2 = ERROR_NOT_A_DOS_DISK;
        return;
    }
}
/* \\\ */

/* /// "nGetObjFromLock()" */
struct PTPObjectInfo * nGetObjFromLock(struct NepClassPTP *nch, BPTR lock)
{
    struct FileLock *fl = (struct FileLock *) BADDR(lock);

    if(!lock)
    {
        KPRINTF(10, ("Lock NULL\n"));
        return(&nch->nch_RootObject);
    }
    KPRINTF(1, ("FL=%08lx, ObjectInfo=%08lx\n", fl, fl->fl_Key));
    return((struct PTPObjectInfo *) fl->fl_Key);
}
/* \\\ */

/* /// "nLoadDirectory()" */
BOOL nLoadDirectory(struct NepClassPTP *nch, struct PTPObjectInfo *poi)
{
    ULONG *idarray = nPTPGetObjectHandles(nch, poi->poi_StorageID, poi->poi_Handle);
    ULONG cnt;
    ULONG handle;
    struct PTPObjectInfo *childpoi;

    KPRINTF(10, ("Loading directory for %s\n", poi->poi_Name));
    if(idarray)
    {
        cnt = *idarray;
        KPRINTF(10, ("%ld ObjectHandles:\n", cnt));
        while(cnt)
        {
            handle = idarray[*idarray - cnt + 1];
            KPRINTF(1, ("%ld: ObjectHandle %08lx\n", *idarray - cnt, handle));
            childpoi = nPTPGetObjectInfo(nch, handle);
            if(childpoi)
            {
                childpoi->poi_Parent = poi;
                AddTail((struct List *) &poi->poi_Children, (struct Node *) childpoi);
            }
            cnt--;
        }
        nFreeVec(nch, idarray);
        poi->poi_Flags |= PTPF_FETCHED;
        return(TRUE);
    }
    return(FALSE);
}
/* \\\ */

/* /// "nSearchObject()" */
struct PTPObjectInfo * nSearchObject(struct NepClassPTP *nch, struct DosPacket *dp, struct PTPObjectInfo *dirpoi, STRPTR path)
{
    UWORD namehash;
    STRPTR srcptr;
    struct PTPObjectInfo *childpoi;

    if(!(*path))
    {
        KPRINTF(10, ("Empty path!\n"));
        return(dirpoi);
    }

    KPRINTF(10, ("Searching path %s from %s on\n", path, dirpoi->poi_Name));
    // search for ':'
    srcptr = path;
    while(*srcptr && (*srcptr != '/'))
    {
        if(*srcptr == ':')
        {
            KPRINTF(5, ("Rooting!\n"));
            dirpoi = &nch->nch_RootObject;
            path = ++srcptr;
            break;
        }
        srcptr++;
    }

    do
    {
        if(!(*path))
        {
            KPRINTF(5, ("Object found: %s\n", dirpoi->poi_Name));
            return(dirpoi);
        }

        if((dirpoi->poi_ObjectFmt == POF_ASSOCIATION) && (!(dirpoi->poi_Flags & PTPF_FETCHED)))
        {
            nLoadDirectory(nch, dirpoi);
        }

        while(*path == '/')
        {
            KPRINTF(1, ("Going up...\n"));
            dirpoi = dirpoi->poi_Parent;
            if(!dirpoi)
            {
                KPRINTF(5, ("Past root!\n"));
                dp->dp_Res2 = ERROR_OBJECT_NOT_FOUND;
                return(NULL);
            }
            path++;
        }

        namehash = nCalcNameHash(nch, path);

        if(dirpoi->poi_ObjectFmt == POF_ASSOCIATION)
        {
            childpoi = (struct PTPObjectInfo *) dirpoi->poi_Children.mlh_Head;
            while(childpoi->poi_Node.mln_Succ)
            {
                if(childpoi->poi_NameHash == namehash)
                {
                    STRPTR tarptr = childpoi->poi_Name;
                    srcptr = path;
 
                    KPRINTF(1, ("Hash matched for %s on %s\n", childpoi->poi_Name, path));
                    // deep check here
                    while(*srcptr && *tarptr && (*srcptr != '/'))
                    {
                        if(nch->nch_LowCharMap[*srcptr] != nch->nch_LowCharMap[*tarptr])
                        {
                            break;
                        }
                        srcptr++;
                        tarptr++;
                    }
                    if(!*tarptr)
                    {
                        KPRINTF(1, ("Exact match!\n"));
                        dirpoi = childpoi;
                        if(*srcptr == '/')
                        {
                            // continue after slash
                            path = srcptr + 1;
                        } else {
                            // last component?
                            path = srcptr;
                        }
                        break;
                    } else {
                        KPRINTF(1, ("False positive!\n"));
                    }
                }
                childpoi = (struct PTPObjectInfo *) childpoi->poi_Node.mln_Succ;
            }
            if(!childpoi->poi_Node.mln_Succ)
            {
                KPRINTF(5, ("Component %s not found\n", path));
                dp->dp_Res2 = ERROR_OBJECT_NOT_FOUND;
                return(NULL);
            }
        } else {
            KPRINTF(10, ("Can't go into file %s for %s\n", dirpoi->poi_Name, path));
            dp->dp_Res2 = ERROR_INVALID_COMPONENT_NAME;
            return(NULL);
        }
    } while(TRUE);
}
/* \\\ */

/* /// "nGetFileComponent()" */
STRPTR nGetFileComponent(struct NepClassPTP *nch, STRPTR path)
{
    STRPTR srcptr = path;
    STRPTR fileptr = path;
    STRPTR resptr;
    ULONG wordcnt = 0;

    while(*srcptr)
    {
        if((*srcptr == ':') || (*srcptr == '/'))
        {
            fileptr = ++srcptr;
            wordcnt = 0;
        } else {
            ++srcptr;
            ++wordcnt;
        }
    }

    srcptr = resptr = nAllocVec(nch, wordcnt + 1);
    if(resptr)
    {
        while((*srcptr++ = *fileptr++));
    }
    return(resptr);
}
/* \\\ */

/* /// "nGetPathComponent()" */
STRPTR nGetPathComponent(struct NepClassPTP *nch, STRPTR path)
{
    STRPTR srcptr = path;
    STRPTR fileptr = path;
    STRPTR resptr;

    while(*srcptr)
    {
        if(*srcptr == ':')
        {
            fileptr = srcptr + 1;
        }
        else if(*srcptr == '/')
        {
            fileptr = srcptr;
        }
        ++srcptr;
    }

    srcptr = resptr = nAllocVec(nch, fileptr - path + 1);
    if(resptr)
    {
        while(path < fileptr)
        {
            *srcptr++ = *path++;
        }
        *srcptr = 0;
    }
    return(resptr);
}
/* \\\ */

/* /// "nCreateLock()" */
struct FileLock * nCreateLock(struct NepClassPTP *nch, struct DosPacket *dp, struct PTPObjectInfo *poi, LONG lockmode)
{
    struct FileLock *fl;
    if((poi->poi_Flags & PTPF_EXCL_LOCK) || ((lockmode == ACCESS_WRITE) && (poi->poi_Flags & PTPF_SHARED_LOCK)))
    {
        KPRINTF(20, ("Can't lock %ld: Object '%s' already locked\n", lockmode, poi->poi_Name));
        dp->dp_Res2 = ERROR_OBJECT_IN_USE;
        return(NULL);
    }
    if(lockmode != ACCESS_WRITE)
    {
        lockmode = ACCESS_READ; // workaround for bugged programs
    }
    fl = (struct FileLock *) nAllocVec(nch, sizeof(struct FileLock));
    if(!fl)
    {
        dp->dp_Res2 = ERROR_NO_FREE_STORE;
        return(NULL);
    }
    KPRINTF(10, ("CreateLock: FL=%08lx for %08lx: '%s' (%s)\n", fl, poi, poi->poi_Name, (lockmode == ACCESS_READ) ? "READ" : "WRITE"));
    fl->fl_Link = nch->nch_VolEntry->dol_misc.dol_volume.dol_LockList;
    fl->fl_Key = (LONG) poi;
    fl->fl_Access = lockmode;
    fl->fl_Task = nch->nch_DOSMsgPort;
    fl->fl_Volume = MKBADDR(nch->nch_VolEntry);
    if(lockmode == ACCESS_READ)
    {
        poi->poi_ReadLocks++;
        poi->poi_Flags |= PTPF_SHARED_LOCK;
    } else {
        poi->poi_Flags |= PTPF_EXCL_LOCK;
    }
    nch->nch_VolEntry->dol_misc.dol_volume.dol_LockList = MKBADDR(fl);
    return(fl);
}
/* \\\ */

/* /// "nFreeLock()" */
void nFreeLock(struct NepClassPTP *nch, struct FileLock *fl)
{
    struct PTPObjectInfo *poi;
    struct FileLock *headfl;

    KPRINTF(1, ("FreeLock(%08lx)\n", fl));
    if(!fl)
    {
        KPRINTF(10, ("No lock!\n"));
        return;
    }
    poi = (struct PTPObjectInfo *) fl->fl_Key;
    KPRINTF(10, ("Freeing lock on %08lx: '%s'\n", poi, poi->poi_Name));
    headfl = (struct FileLock *) BADDR(nch->nch_VolEntry->dol_misc.dol_volume.dol_LockList);
    if(headfl == fl)
    {
        // head of list
        KPRINTF(1, ("Unlinking at head\n"));
        nch->nch_VolEntry->dol_misc.dol_volume.dol_LockList = fl->fl_Link;
    } else {
        do
        {
            if(headfl->fl_Link)
            {
                if((struct FileLock *) BADDR(headfl->fl_Link) == fl)
                {
                    // unlink
                    KPRINTF(1, ("Unlinking in middle\n"));
                    headfl->fl_Link = fl->fl_Link;
                    break;
                }
            } else {
                KPRINTF(20, ("LOCK NOT FOUND IN LIST!\n"));
                break;
            }
            headfl = (struct FileLock *) BADDR(headfl->fl_Link);
        } while(TRUE);
    }

    nFreeVec(nch, fl);
    if(poi->poi_Flags & PTPF_SHARED_LOCK)
    {
        if(!(--poi->poi_ReadLocks))
        {
            poi->poi_Flags &= ~PTPF_SHARED_LOCK;
        }
    } else {
        poi->poi_Flags &= ~PTPF_EXCL_LOCK;
    }
}
/* \\\ */

/* /// "nCreateFH()" */
struct PTPFileHandle * nCreateFH(struct NepClassPTP *nch, struct DosPacket *dp, struct PTPObjectInfo *poi, LONG lockmode)
{
    struct PTPFileHandle *pfh;
    if((poi->poi_Flags & PTPF_EXCL_LOCK) || ((lockmode == ACCESS_WRITE) && (poi->poi_Flags & PTPF_SHARED_LOCK)))
    {
        KPRINTF(20, ("Can't open %ld: Object '%s' already locked\n", lockmode, poi->poi_Name));
        dp->dp_Res2 = ERROR_OBJECT_IN_USE;
        return(NULL);
    }
    pfh = (struct PTPFileHandle *) nAllocVec(nch, sizeof(struct PTPFileHandle));
    if(!pfh)
    {
        dp->dp_Res2 = ERROR_NO_FREE_STORE;
        return(NULL);
    }
    KPRINTF(10, ("CreateFH: FH=%08lx for %08lx: '%s'\n", pfh, poi, poi->poi_Name));
    pfh->pfh_ObjectInfo = poi;
    if(lockmode == ACCESS_READ)
    {
        poi->poi_ReadLocks++;
        poi->poi_Flags |= PTPF_SHARED_LOCK;
    } else {
        poi->poi_Flags |= PTPF_EXCL_LOCK;
    }
    AddTail(&nch->nch_FHs, &pfh->pfh_Node);
    return(pfh);
}
/* \\\ */

/* /// "nFreeFH()" */
void nFreeFH(struct NepClassPTP *nch, struct PTPFileHandle *pfh)
{
    struct PTPObjectInfo *poi;

    KPRINTF(1, ("FreeFH(%08lx)\n", pfh));
    if(!pfh)
    {
        KPRINTF(10, ("No FH!\n"));
        return;
    }
    Remove(&pfh->pfh_Node);
    nFreeVec(nch, pfh->pfh_Buffer);
    poi = pfh->pfh_ObjectInfo;
    nFreeVec(nch, pfh);
    if(poi->poi_Flags & PTPF_SHARED_LOCK)
    {
        if(!(--poi->poi_ReadLocks))
        {
            poi->poi_Flags &= ~PTPF_SHARED_LOCK;
        }
    } else {
        poi->poi_Flags &= ~PTPF_EXCL_LOCK;
    }
}
/* \\\ */

/* /// "defaultObjectMapping" */
static struct ObjectFmtMapping defaultObjectMapping[] =
{
    { POF_SCRIPT, "sh" },
    { POF_SCRIPT, "bat" },
    { POF_EXECUTABLE, "exe" },
    { POF_TEXT, "txt" },
    { POF_HTML, "htm" },
    { POF_HTML, "html" },
    { POF_DPOF, "dps" },
    { POF_AIFF, "aiff" },
    { POF_WAV, "wav" },
    { POF_MP3, "mp3" },
    { POF_AVI, "avi" },
    { POF_MPEG, "mpg" },
    { POF_MPEG, "mpeg" },
    { POF_ASF, "asf" },
    { POF_EXIF_JPEG, "jpg" },
    { POF_EXIF_JPEG, "jpeg" },
    { POF_TIFF, "tif" },
    { POF_TIFF, "tiff" },
    { POF_BMP, "bmp" },
    { POF_GIF, "gif" },
    { POF_JFIF, "jfif" },
    { POF_PCD, "pcd" },
    { POF_PICT, "pct" },
    { POF_PNG, "png" },
    { POF_JP2, "jp2" },
    { POF_JPX, "jpx" },
    { 0, NULL }
};
/* \\\ */

/* /// "nGetFormatFromName()" */
UWORD nGetFormatFromName(struct NepClassPTP *nch, STRPTR name)
{
    STRPTR suffix = NULL;
    struct ObjectFmtMapping *ofm = defaultObjectMapping;
    while(*name)
    {
        if(*name++ == '.')
        {
            suffix = name;
        }
    }
    if(!suffix)
    {
        return POF_UNDEFINED;
    }
    while(ofm->ofm_ID)
    {
        if(stricmp(suffix, ofm->ofm_Suffix) == 0)
        {
            return ofm->ofm_ID;
        }
        ofm++;
    }
    return POF_UNDEFINED;
}
/* \\\ */

/* /// "nHandleDOSPackets()" */
void nHandleDOSPackets(struct NepClassPTP *nch)
{
    struct Message *mn;
    struct MsgPort *replyport;
    struct DosPacket *dp;
    UBYTE buf[256];

    struct PTPObjectInfo *parentdir;
    struct PTPObjectInfo *poi;
    struct PTPFileHandle *pfh;
    UBYTE *tmpbstr;
    LONG lockmode;
    struct FileLock *fl;
    struct FileInfoBlock *fib;
    struct InfoData *id;
    struct FileHandle *fh;
    UWORD cnt;
    UBYTE *srcptr;
    UBYTE *tarptr;

    while((mn = GetMsg(nch->nch_DOSMsgPort)))
    {
        KPRINTF(1, ("DOS Packet arrived %08lx!\n", mn));
        dp = (struct DosPacket *) mn->mn_Node.ln_Name;
        dp->dp_Res1 = DOSTRUE;
        dp->dp_Res2 = 0;

        KPRINTF(10, ("**** Action %ld: Arg1=%08lx Arg2=%08lx Arg3=%08lx\n",
                     dp->dp_Action, dp->dp_Arg1, dp->dp_Arg2, dp->dp_Arg3));

        switch(dp->dp_Action)
        {
            case ACTION_DIE:
                nch->nch_ShallExit = TRUE;
                break;

            // no-ops
            case ACTION_FLUSH:
                nch->nch_LastObject = NULL; // Invalidate last object
                nFreeVec(nch, nch->nch_LastBuffer);
                nch->nch_LastBuffer = NULL;
                // fall through
            case ACTION_INHIBIT:

            case ACTION_IS_FILESYSTEM:
                // returns DOSTRUE
                break;

            case ACTION_SAME_LOCK:
                dp->dp_Res1 = (nGetObjFromLock(nch, (BPTR) dp->dp_Arg1) == nGetObjFromLock(nch, (BPTR) dp->dp_Arg2)) ? DOSTRUE : DOSFALSE;
                break;

            case ACTION_FORMAT:
                nch->nch_LastObject = NULL; // Invalidate last object
                nFreeVec(nch, nch->nch_LastBuffer);
                nch->nch_LastBuffer = NULL;
                nPTPFormatStore(nch, dp);
                break;

            case ACTION_CURRENT_VOLUME:
                dp->dp_Res1 = (SIPTR) MKBADDR(nch->nch_VolEntry);
                dp->dp_Res2 = 0;
                break;

            case ACTION_INFO:
            case ACTION_DISK_INFO:
            {
                struct PTPStorageInfo *psi;
                ULONG numblocks = 0;
                ULONG numfree = 0;
                ULONG storageid = 0;
                ULONG bitshift = 9;
                if(dp->dp_Action == ACTION_INFO)
                {
                    poi = nGetObjFromLock(nch, (BPTR) dp->dp_Arg1);
                    id = (struct InfoData *) BADDR(dp->dp_Arg2);
                    storageid = poi->poi_StorageID;
                } else {
                    id = (struct InfoData *) BADDR(dp->dp_Arg1);
                }
                // determine bit shift value for optimal blocksize
                psi = (struct PTPStorageInfo *) nch->nch_Storages.lh_Head;
                while(psi->psi_Node.ln_Succ)
                {
                    if(psi->psi_MaxCapacityH != 0xffffffff)
                    {
                        while((bitshift < 24) && psi->psi_MaxCapacityH>>(bitshift-2))
                        {
                            bitshift++;
                        }
                    }
                    psi = (struct PTPStorageInfo *) psi->psi_Node.ln_Succ;
                }
                KPRINTF(5, ("Bitshift %ld\n", bitshift));
                psi = (struct PTPStorageInfo *) nch->nch_Storages.lh_Head;
                while(psi->psi_Node.ln_Succ)
                {
                    if((storageid == psi->psi_StorageID) || (!storageid))
                    {
                        if(psi->psi_MaxCapacityH>>15)
                        {
                            numblocks = 0x3fffff; // avoid overflow
                            numfree = 0x3fffff;
                            bitshift = 9;
                            break;
                        }
                        numblocks += (psi->psi_MaxCapacityL>>bitshift)|(psi->psi_MaxCapacityH<<(32-bitshift));
                        numfree += (psi->psi_FreeL>>bitshift)|(psi->psi_FreeH<<(32-bitshift));
                        KPRINTF(5, ("Blocks %ld free of %ld\n", numfree, numblocks));
                    }
                    psi = (struct PTPStorageInfo *) psi->psi_Node.ln_Succ;
                }
                if(!numblocks)
                {
                    if(numfree)
                    {
                        numblocks = numfree;
                    } else {
                        numblocks = 1; // make sure we don't get division by zero errors
                    }
                }
                if(numfree > numblocks)
                {
                    numfree = numblocks;
                }
                //numblocks = 1000;
                //numfree = 500;
                //bitshift = 9;
                id->id_NumSoftErrors = 0;
                id->id_UnitNumber = 0;
                id->id_DiskState = ID_VALIDATED;
                id->id_NumBlocks = numblocks;
                id->id_NumBlocksUsed = numblocks - numfree;
                id->id_BytesPerBlock = 1<<bitshift;
                id->id_DiskType = ID_DOS_DISK;
                id->id_VolumeNode = MKBADDR(nch->nch_VolEntry);
                id->id_InUse = nch->nch_VolEntry->dol_misc.dol_volume.dol_LockList ? DOSTRUE : DOSFALSE;
                break;
            }

            case ACTION_FINDINPUT:
            case ACTION_FINDUPDATE:
                fh = (struct FileHandle *) BADDR(dp->dp_Arg1);
                parentdir = nGetObjFromLock(nch, (BPTR) dp->dp_Arg2);
                tmpbstr = (UBYTE *) BADDR(dp->dp_Arg3);
                b2cstr(tmpbstr, buf);
 
                KPRINTF(5, ("OpenOld Object '%s', Parent=%s\n", buf, parentdir->poi_Name));
                poi = nSearchObject(nch, dp, parentdir, buf);
                if(poi)
                {
                    if(poi->poi_ObjectFmt == POF_ASSOCIATION)
                    {
                        dp->dp_Res2 = ERROR_OBJECT_WRONG_TYPE;
                        break;
                    }
                    pfh = nCreateFH(nch, dp, poi, ACCESS_READ);
                    if(!pfh)
                    {
                        break;
                    }
                    fh->fh_Arg1 = (LONG) pfh;
                    break;
                } else {
                    if(dp->dp_Action == ACTION_FINDINPUT)
                    {
                        break;
                    }
                    KPRINTF(5, ("OpenReadWrite Object '%s', Parent=%s\n", buf, parentdir->poi_Name));
                    dp->dp_Res2 = 0;
                    // fall through
                }

            case ACTION_FINDOUTPUT:
                if(!(nch->nch_DevCaps & PTPF_SENDOBJ))
                {
                    dp->dp_Res2 = ERROR_DISK_WRITE_PROTECTED;
                    break;
                }
                fh = (struct FileHandle *) BADDR(dp->dp_Arg1);
                parentdir = nGetObjFromLock(nch, (BPTR) dp->dp_Arg2);
                tmpbstr = (UBYTE *) BADDR(dp->dp_Arg3);
                b2cstr(tmpbstr, buf);
                if(dp->dp_Action == ACTION_FINDOUTPUT)
                {
                    KPRINTF(5, ("OpenNew Object '%s',  Parent=%s\n", buf, parentdir->poi_Name));
                    poi = nSearchObject(nch, dp, parentdir, buf);
                    if(poi)
                    {
                        // delete old file first
                        if(poi->poi_Flags & (PTPF_EXCL_LOCK|PTPF_SHARED_LOCK))
                        {
                            dp->dp_Res2 = ERROR_OBJECT_IN_USE;
                            break;
                        }
                        if(poi->poi_ObjectFmt == POF_ASSOCIATION)
                        {
                            dp->dp_Res2 = ERROR_OBJECT_WRONG_TYPE;
                            break;
                        }
                        if(poi->poi_Handle == 0xffffffff) // cannot delete volume
                        {
                            dp->dp_Res2 = ERROR_DIR_NOT_FOUND;
                            break;
                        }
                        nPTPDeleteObject(nch, dp, poi);
                        if(!dp->dp_Res2)
                        {
                            Remove((struct Node *) poi);
                            nPTPFreeObjectInfo(nch, poi);
                            nch->nch_LastObject = NULL; // Invalidate last object
                            nFreeVec(nch, nch->nch_LastBuffer);
                            nch->nch_LastBuffer = NULL;

                            DateStamp(&poi->poi_Parent->poi_ModDate);
                        } else {
                            break;
                        }
                    }
                }
                dp->dp_Res2 = 0;
                srcptr = nGetPathComponent(nch, buf);
                if(!srcptr)
                {
                    dp->dp_Res2 = ERROR_NO_FREE_STORE;
                    break;
                }
                parentdir = nSearchObject(nch, dp, parentdir, srcptr);
                nFreeVec(nch, srcptr);
                if(!parentdir)
                {
                    break;
                }
                KPRINTF(10, ("Parent dir for new file: %s\n", parentdir->poi_Name));
                if(parentdir->poi_StorageID == 0xffffffff) // cannot create files on root level
                {
                    dp->dp_Res2 = ERROR_DIR_NOT_FOUND;
                    break;
                }
                poi = nAllocVec(nch, sizeof(struct PTPObjectInfo));
                if(!poi)
                {
                    dp->dp_Res2 = ERROR_NO_FREE_STORE;
                    break;
                }
                NewList((struct List *) &poi->poi_Children);
                poi->poi_Handle = 0;
                poi->poi_StorageID = parentdir->poi_StorageID;
                poi->poi_ProtFlags = FIBF_READ|FIBF_WRITE|FIBF_DELETE;
                poi->poi_Size = 0;
                poi->poi_Parent = parentdir;
                poi->poi_ParentHandle = parentdir->poi_Handle;
                poi->poi_Name = nGetFileComponent(nch, buf);
                if(!poi->poi_Name)
                {
                    nFreeVec(nch, poi);
                    dp->dp_Res2 = ERROR_NO_FREE_STORE;
                    break;
                }
                poi->poi_ObjectFmt = nGetFormatFromName(nch, poi->poi_Name);
                KPRINTF(10, ("Determined object format: %02lx\n", poi->poi_ObjectFmt));

                DateStamp(&poi->poi_CapDate);
                DateStamp(&poi->poi_ModDate);
                poi->poi_NameHash = nCalcNameHash(nch, poi->poi_Name);

                pfh = nCreateFH(nch, dp, poi, ACCESS_WRITE);
                if(!pfh)
                {
                    nPTPFreeObjectInfo(nch, poi);
                    break;
                }
                pfh->pfh_Flags |= PTPF_NEWFILE|PTPF_MODIFIED;
                fh->fh_Arg1 = (LONG) pfh;
                AddTail((struct List *) &parentdir->poi_Children, (struct Node *) poi);
                break;

#if 1 // currently no way to create directories according to PTP spec
            case ACTION_CREATE_DIR:
                if(!(nch->nch_DevCaps & PTPF_SENDOBJ))
                {
                    dp->dp_Res2 = ERROR_DISK_WRITE_PROTECTED;
                    break;
                }
                parentdir = nGetObjFromLock(nch, (BPTR) dp->dp_Arg1);
                tmpbstr = (UBYTE *) BADDR(dp->dp_Arg2);
                b2cstr(tmpbstr, buf);
                KPRINTF(5, ("MakeDir '%s', Parent=%s\n", buf, parentdir->poi_Name));
                poi = nSearchObject(nch, dp, parentdir, buf);
                if(poi)
                {
                    dp->dp_Res2 = ERROR_OBJECT_EXISTS;
                    break;
                }
                dp->dp_Res2 = 0;
                srcptr = nGetPathComponent(nch, buf);
                if(!srcptr)
                {
                    dp->dp_Res2 = ERROR_NO_FREE_STORE;
                    break;
                }
                parentdir = nSearchObject(nch, dp, parentdir, srcptr);
                nFreeVec(nch, srcptr);
                if(!parentdir)
                {
                    break;
                }
                KPRINTF(10, ("Parent dir for new directory: %s\n", parentdir->poi_Name));
                if(parentdir->poi_StorageID == 0xffffffff) // cannot create directories on root level
                {
                    dp->dp_Res2 = ERROR_DIR_NOT_FOUND;
                    break;
                }
                poi = nAllocVec(nch, sizeof(struct PTPObjectInfo));
                if(!poi)
                {
                    dp->dp_Res2 = ERROR_NO_FREE_STORE;
                    break;
                }
                NewList((struct List *) &poi->poi_Children);
                poi->poi_Handle = 0;
                poi->poi_StorageID = parentdir->poi_StorageID;
                poi->poi_ProtFlags = FIBF_READ|FIBF_WRITE|FIBF_DELETE;
                poi->poi_Size = 0;
                poi->poi_Parent = parentdir;
                poi->poi_ParentHandle = parentdir->poi_Handle;
                poi->poi_Name = nGetFileComponent(nch, buf);
                if(!poi->poi_Name)
                {
                    nFreeVec(nch, poi);
                    dp->dp_Res2 = ERROR_NO_FREE_STORE;
                    break;
                }
                poi->poi_ObjectFmt = POF_ASSOCIATION;

                DateStamp(&poi->poi_CapDate);
                DateStamp(&poi->poi_ModDate);
                poi->poi_NameHash = nCalcNameHash(nch, poi->poi_Name);

                KPRINTF(10, ("Sending Object for MakeDir\n"));
                if(!(nPTPSendObject(nch, dp, poi, NULL)))
                {
                    nPTPFreeObjectInfo(nch, poi);
                } else {
                    AddTail((struct List *) &parentdir->poi_Children, (struct Node *) poi);
                    fl = nCreateLock(nch, dp, poi, ACCESS_WRITE);
                    if(fl)
                    {
                        dp->dp_Res1 = (SIPTR) MKBADDR(fl);
                        KPRINTF(1, ("Copied lock %08lx\n", dp->dp_Res1));
                    }
                }
                break;
#endif

            case ACTION_FH_FROM_LOCK:
                fh = (struct FileHandle *) BADDR(dp->dp_Arg1);
                fl = (struct FileLock *) BADDR(dp->dp_Arg2);
                poi = nGetObjFromLock(nch, (BPTR) dp->dp_Arg2);
                lockmode = fl->fl_Access;

                KPRINTF(5, ("OpenFromLock Object '%s'\n", poi->poi_Name));
                if(poi->poi_ObjectFmt == POF_ASSOCIATION)
                {
                    dp->dp_Res2 = ERROR_OBJECT_WRONG_TYPE;
                    break;
                }
                if(lockmode == ACCESS_WRITE)
                {
                    if(!(nch->nch_DevCaps & PTPF_SENDOBJ))
                    {
                        dp->dp_Res2 = ERROR_DISK_WRITE_PROTECTED;
                        break;
                    }
                    poi->poi_Flags &= ~PTPF_EXCL_LOCK;
                    pfh = nCreateFH(nch, dp, poi, ACCESS_WRITE);
                    if(!pfh)
                    {
                        poi->poi_Flags |= PTPF_EXCL_LOCK;
                        break;
                    }
                    nFreeLock(nch, fl);
                    poi->poi_Flags |= PTPF_EXCL_LOCK;
                } else {
                    pfh = nCreateFH(nch, dp, poi, ACCESS_READ);
                    if(!pfh)
                    {
                        break;
                    }
                    nFreeLock(nch, fl);
                }
                pfh->pfh_SeekPos = 0;
                fh->fh_Arg1 = (LONG) pfh;
                break;

            case ACTION_END:
                pfh = (struct PTPFileHandle *) dp->dp_Arg1;
                KPRINTF(10, ("Closing File %s\n", pfh->pfh_ObjectInfo->poi_Name));
                if(pfh->pfh_Flags & PTPF_MODIFIED)
                {
                    KPRINTF(10, ("Modified, needs updating on device\n"));
                    if(!(pfh->pfh_Flags & PTPF_NEWFILE))
                    {
                        KPRINTF(10, ("Deleting old object first\n"));
                        nPTPDeleteObject(nch, dp, pfh->pfh_ObjectInfo);
                        if(dp->dp_Res2)
                        {
                            break;
                        }
                    }
                    KPRINTF(10, ("Sending Object\n"));
                    if(!(nPTPSendObject(nch, dp, pfh->pfh_ObjectInfo, pfh->pfh_Buffer)))
                    {
                        Remove((struct Node *) pfh->pfh_ObjectInfo);
                        nPTPFreeObjectInfo(nch, pfh->pfh_ObjectInfo);
                        nFreeVec(nch, pfh->pfh_Buffer);
                        pfh->pfh_Buffer = NULL;
                    }
                }
                if(pfh->pfh_Buffer)
                {
                    nch->nch_LastObject = pfh->pfh_ObjectInfo; // New last object cache
                    nFreeVec(nch, nch->nch_LastBuffer);
                    nch->nch_LastBuffer = pfh->pfh_Buffer;
                    nch->nch_LastBufferLen = pfh->pfh_BufferLen;
                    pfh->pfh_Buffer = NULL;
                }
                nFreeFH(nch, pfh);
                break;

            case ACTION_SEEK:
            {
                LONG relpos = dp->dp_Arg2;
                LONG mode = dp->dp_Arg3;
                ULONG newpos;

                pfh = (struct PTPFileHandle *) dp->dp_Arg1;
                newpos = dp->dp_Res1 = pfh->pfh_SeekPos;
                poi = pfh->pfh_ObjectInfo;
                switch(mode)
                {
                    case OFFSET_END:
                        newpos = poi->poi_Size + relpos;
                        KPRINTF(10, ("Seek from end %ld: %ld\n", relpos, newpos));
                        break;

                    case OFFSET_CURRENT:
                        newpos += relpos;
                        KPRINTF(10, ("Seek from current %ld: %ld\n", relpos, newpos));
                        break;

                    case OFFSET_BEGINNING:
                        newpos = relpos;
                        KPRINTF(10, ("Seek from beginning %ld\n", relpos));
                        break;

                    default:
                        dp->dp_Res2 = ERROR_SEEK_ERROR;
                        break;
                }

                if(newpos > poi->poi_Size)
                {
                    dp->dp_Res2 = ERROR_SEEK_ERROR;
                }

                pfh->pfh_SeekPos = newpos;
                break;
            }

            case ACTION_READ:
                pfh = (struct PTPFileHandle *) dp->dp_Arg1;
                nPTPRead(nch, dp, pfh, (UBYTE *) dp->dp_Arg2, (ULONG) dp->dp_Arg3);
                break;

            case ACTION_WRITE:
                if(!(nch->nch_DevCaps & PTPF_SENDOBJ))
                {
                    dp->dp_Res2 = ERROR_DISK_WRITE_PROTECTED;
                    break;
                }
                pfh = (struct PTPFileHandle *) dp->dp_Arg1;
                if(pfh->pfh_ObjectInfo->poi_ProtFlags & FIBF_WRITE)
                {
                    nPTPWrite(nch, dp, pfh, (UBYTE *) dp->dp_Arg2, (ULONG) dp->dp_Arg3);
                } else {
                    dp->dp_Res2 = ERROR_WRITE_PROTECTED;
                }
                break;

            case ACTION_SET_PROTECT:
            {
                ULONG protflags;
                if(!(nch->nch_DevCaps & PTPF_PROTOBJ))
                {
                    dp->dp_Res2 = ERROR_DISK_WRITE_PROTECTED;
                    break;
                }
                parentdir = nGetObjFromLock(nch, (BPTR) dp->dp_Arg2);
                protflags = dp->dp_Arg4 ^ (FIBF_READ|FIBF_WRITE|FIBF_DELETE|FIBF_EXECUTE);
                tmpbstr = (UBYTE *) BADDR(dp->dp_Arg3);
                b2cstr(tmpbstr, buf);

                KPRINTF(5, ("Set Protect '%s', Parent=%s\n", buf, parentdir->poi_Name));
                poi = nSearchObject(nch, dp, parentdir, buf);
                if(poi)
                {
                    nPTPSetObjectProtection(nch, dp, poi, protflags);
                }
                break;
            }

            case ACTION_DELETE_OBJECT:
                if(!(nch->nch_DevCaps & PTPF_DELETEOBJ))
                {
                    dp->dp_Res2 = ERROR_DELETE_PROTECTED;
                    break;
                }
                parentdir = nGetObjFromLock(nch, (BPTR) dp->dp_Arg1);
                tmpbstr = (UBYTE *) BADDR(dp->dp_Arg2);
                b2cstr(tmpbstr, buf);

                KPRINTF(5, ("DeleteObject '%s', Parent=%s\n", buf, parentdir->poi_Name));
                poi = nSearchObject(nch, dp, parentdir, buf);
                if(poi)
                {
                    if(poi->poi_Flags & (PTPF_EXCL_LOCK|PTPF_SHARED_LOCK))
                    {
                        dp->dp_Res2 = ERROR_OBJECT_IN_USE;
                        break;
                    }
                    if(poi->poi_Children.mlh_Head->mln_Succ)
                    {
                        dp->dp_Res2 = ERROR_DIRECTORY_NOT_EMPTY;
                        break;
                    }
                    if(poi->poi_Handle == 0xffffffff)
                    {
                        dp->dp_Res2 = ERROR_DIRECTORY_NOT_EMPTY;
                        break;
                    }
                    if(!(poi->poi_ProtFlags & FIBF_DELETE))
                    {
                        dp->dp_Res2 = ERROR_DELETE_PROTECTED;
                        break;
                    }
                    nPTPDeleteObject(nch, dp, poi);
                    if(!dp->dp_Res2)
                    {
                        DateStamp(&poi->poi_Parent->poi_ModDate);
                        Remove((struct Node *) poi);
                        nPTPFreeObjectInfo(nch, poi);
                        nch->nch_LastObject = NULL; // Invalidate last object
                        nFreeVec(nch, nch->nch_LastBuffer);
                        nch->nch_LastBuffer = NULL;
                    }
                }
                break;

            case ACTION_RENAME_OBJECT:
            {
                struct PTPObjectInfo *targetdir;
                ULONG cnt;

                if(!(nch->nch_DevCaps & PTPF_MOVEOBJ))
                {
                    dp->dp_Res2 = ERROR_DISK_WRITE_PROTECTED;
                    break;
                }

                srcptr = (UBYTE *) BADDR(dp->dp_Arg2);
                tarptr = (UBYTE *) BADDR(dp->dp_Arg4);
                cnt = *srcptr;
                if(*srcptr++ != *tarptr++)
                {
                    // length mismatch
                    KPRINTF(10, ("RenameObject failed: Name lengths different!\n"));
                    dp->dp_Res2 = ERROR_RENAME_ACROSS_DEVICES;
                    break;
                }
                if(!cnt)
                {
                    dp->dp_Res2 = ERROR_INVALID_COMPONENT_NAME;
                    break;
                }
                do
                {
                    if(*srcptr++ != *tarptr++)
                    {
                        KPRINTF(10, ("RenameObject failed: Names different!\n"));
                        dp->dp_Res2 = ERROR_RENAME_ACROSS_DEVICES;
                        break;
                    }
                } while(--cnt);
                if(!dp->dp_Res2)
                {
                    break;
                }
 
                parentdir = nGetObjFromLock(nch, (BPTR) dp->dp_Arg1);
                targetdir = nGetObjFromLock(nch, (BPTR) dp->dp_Arg3);

                poi = nSearchObject(nch, dp, parentdir, buf);
                if(poi)
                {
                    KPRINTF(10, ("RenameObject moving %s from %s to %s.\n", poi->poi_Name, parentdir->poi_Name, targetdir->poi_Name));
                    if(parentdir != targetdir)
                    {
                        nPTPMoveObject(nch, dp, poi, targetdir);
                    }
                }
                break;
            }

            case ACTION_PARENT_FH:
            case ACTION_PARENT:
                if(dp->dp_Action == ACTION_PARENT)
                {
                    poi = nGetObjFromLock(nch, (BPTR) dp->dp_Arg1);
                } else {
                    pfh = (struct PTPFileHandle *) dp->dp_Arg1;
                    poi = pfh->pfh_ObjectInfo;
                }
                if(poi->poi_Parent)
                {
                    fl = nCreateLock(nch, dp, poi->poi_Parent, ACCESS_READ);
                    if(fl)
                    {
                        dp->dp_Res1 = (SIPTR) MKBADDR(fl);
                        KPRINTF(1, ("Found parent %08lx\n", dp->dp_Res1));
                    }
                } else {
                    dp->dp_Res1 = (SIPTR) NULL;
                    // don't set an error code!
                }
                break;

            case ACTION_COPY_DIR_FH:
            case ACTION_COPY_DIR:
                if(dp->dp_Action == ACTION_COPY_DIR)
                {
                    poi = nGetObjFromLock(nch, (BPTR) dp->dp_Arg1);
                } else {
                    pfh = (struct PTPFileHandle *) dp->dp_Arg1;
                    poi = pfh->pfh_ObjectInfo;
                }
                fl = nCreateLock(nch, dp, poi, ACCESS_READ);
                if(fl)
                {
                    dp->dp_Res1 = (SIPTR) MKBADDR(fl);
                    KPRINTF(1, ("Copied lock %08lx\n", dp->dp_Res1));
                }
                break;

            case ACTION_LOCATE_OBJECT:
                parentdir = nGetObjFromLock(nch, (BPTR) dp->dp_Arg1);
                tmpbstr = (UBYTE *) BADDR(dp->dp_Arg2);
                lockmode = dp->dp_Arg3;
                b2cstr(tmpbstr, buf);

                KPRINTF(5, ("Locate Object '%s', Parent=%s\n", buf, parentdir->poi_Name));
                poi = nSearchObject(nch, dp, parentdir, buf);
                if(poi)
                {
                    fl = nCreateLock(nch, dp, poi, lockmode);
                    if(fl)
                    {
                        dp->dp_Res1 = (SIPTR) MKBADDR(fl);
                        KPRINTF(1, ("Found %08lx\n", dp->dp_Res1));
                    }
                }
                break;

            case ACTION_EXAMINE_NEXT:
            case ACTION_EXAMINE_OBJECT:
            case ACTION_EXAMINE_FH:
                fib = (struct FileInfoBlock *) BADDR(dp->dp_Arg2);
                if(!fib)
                KPRINTF(1, ("FIB: %08lx\n", fib));
                if(dp->dp_Action != ACTION_EXAMINE_FH)
                {
                    poi = nGetObjFromLock(nch, (BPTR) dp->dp_Arg1);
                    if(dp->dp_Action == ACTION_EXAMINE_NEXT)
                    {
                        KPRINTF(5, ("ExamineNext: '%s'\n", poi->poi_Name));
                        if(poi->poi_ObjectFmt != POF_ASSOCIATION)
                        {
                            dp->dp_Res2 = ERROR_OBJECT_WRONG_TYPE;
                            break;
                        }
                        if(!(poi->poi_Flags & PTPF_FETCHED))
                        {
                            nLoadDirectory(nch, poi);
                        }
                        // get next child
                        poi = (struct PTPObjectInfo *) ((struct PTPObjectInfo *) fib->fib_DiskKey)->poi_Node.mln_Succ;
                        if(!poi->poi_Node.mln_Succ)
                        {
                            dp->dp_Res2 = ERROR_NO_MORE_ENTRIES;
                            break;
                        }
                        KPRINTF(1, ("Child: %s\n", poi->poi_Name));
                        fib->fib_DiskKey = (ULONG) poi;
                    } else {
                        KPRINTF(5, ("Examine (%08lx): '%s'\n", poi, poi->poi_Name));
                        fib->fib_DiskKey = (ULONG) &poi->poi_Children;
                    }
                } else {
                    pfh = (struct PTPFileHandle *) dp->dp_Arg1;
                    poi = pfh->pfh_ObjectInfo;
                }
                if(!poi->poi_Parent)
                {
                    fib->fib_DirEntryType = ST_ROOT;
                } else {
                    fib->fib_DirEntryType = (poi->poi_ObjectFmt == POF_ASSOCIATION) ? ST_USERDIR : ST_FILE;
                }
                cnt = 0;
                srcptr = poi->poi_Name;
                tarptr = fib->fib_FileName + 1;
                while((*tarptr++ = *srcptr++))
                {
                    if(++cnt > 106)
                    {
                        break;
                    }
                }
                fib->fib_FileName[0] = cnt;
                fib->fib_Protection = poi->poi_ProtFlags ^ (FIBF_READ|FIBF_WRITE|FIBF_DELETE|FIBF_EXECUTE);
                fib->fib_EntryType = fib->fib_DirEntryType;
                fib->fib_Size = poi->poi_Size;
                fib->fib_NumBlocks = (poi->poi_Size + 65535)>>16;
                fib->fib_Date = poi->poi_ModDate;
                if(poi->poi_Keywords)
                {
                    cnt = 0;
                    srcptr = poi->poi_Keywords;
                    tarptr = fib->fib_Comment + 1;
                    while((*tarptr++ = *srcptr++))
                    {
                        if(++cnt > 78)
                        {
                            break;
                        }
                    }
                    fib->fib_Comment[0] = cnt;
                }
                /*fib->fib_OwnerUID = 0;
                fib->fib_OwnerGID = 0;*/
                break;

            case ACTION_FREE_LOCK:
                fl = (struct FileLock *) BADDR(dp->dp_Arg1);
                nFreeLock(nch, fl);
                break;

            default:
                KPRINTF(10, ("**** UNKNOWN Action %ld: Arg1=%08lx Arg2=%08lx Arg3=%08lx\n",
                             dp->dp_Action, dp->dp_Arg1, dp->dp_Arg2, dp->dp_Arg3));
                dp->dp_Res2 = ERROR_ACTION_NOT_KNOWN;
                break;

        }
        if(dp->dp_Res2)
        {
            dp->dp_Res1 = DOSFALSE;
        }
        KPRINTF(10, ("**** Return: %08lx, Error: %ld\n\n", dp->dp_Res1, dp->dp_Res2));

        replyport = dp->dp_Port;
        dp->dp_Port = nch->nch_DOSMsgPort;
        mn = dp->dp_Link;
        mn->mn_Node.ln_Name = (STRPTR) dp;
        mn->mn_Node.ln_Pred = mn->mn_Node.ln_Succ = NULL;
        PutMsg(replyport, mn);
    }
}
/* \\\ */

/* /// "nPTPTask()" */
AROS_UFH0(void, nPTPTask)
{
    AROS_USERFUNC_INIT

    struct NepClassPTP *nch;
    //struct PsdPipe *pp;
    ULONG sigmask;
    ULONG sigs;
    //UBYTE eventbuf[24];
    //LONG ioerr;
    //ULONG pktlen;
    BOOL reportlocks = TRUE;
    BOOL reportfhs = TRUE;

    if((nch = nAllocPTP()))
    {
        Forbid();
        if(nch->nch_ReadySigTask)
        {
            Signal(nch->nch_ReadySigTask, 1L<<nch->nch_ReadySignal);
        }
        Permit();
        sigmask = (1L<<nch->nch_TaskMsgPort->mp_SigBit)|(1L<<nch->nch_DOSMsgPort->mp_SigBit)|SIGBREAKF_CTRL_C;

        nch->nch_SessionID = 0;
        nch->nch_TransID = 0;
        nch->nch_ShallExit = FALSE;

        nPTPGetDeviceInfo(nch);
        nPTPOpenSession(nch);

        {
            ULONG *ids = nPTPGetStorageIDs(nch);
            ULONG cnt;
            ULONG storageid;
            struct PTPStorageInfo *psi;
            if(ids)
            {
                cnt = ids[0];
                KPRINTF(10, ("%ld StorageIDs:\n", cnt));
                while(cnt)
                {
                    storageid = ids[ids[0] - cnt + 1];
                    KPRINTF(1, ("%ld: StorageID %08lx\n", ids[0] - cnt, storageid));
                    psi = nPTPGetStorageInfo(nch, storageid);
                    AddTail(&nch->nch_Storages, &psi->psi_Node);
                    cnt--;
                }
                nFreeVec(nch, ids);
            }
        }

        {
            STRPTR devname;
            struct DosList *dol;
            dol = nch->nch_DevEntry;
            dol->dol_Next = 0;
            dol->dol_Type = DLT_DEVICE;
            dol->dol_Task = nch->nch_DOSMsgPort;
            dol->dol_Lock = 0;
            dol->dol_misc.dol_handler.dol_Handler = 0;
            dol->dol_misc.dol_handler.dol_StackSize = 4096;
            dol->dol_misc.dol_handler.dol_Priority = 10;
            dol->dol_misc.dol_handler.dol_Startup = 0;
            //dol->dol_misc.dol_handler.dol_SegList = 0; // AROS does not have these two fields
            //dol->dol_misc.dol_handler.dol_GlobVec = -1;
            devname = (STRPTR) (dol + 1);
            c2bstr(nch->nch_CDC->cdc_DOSName, devname);
            dol->dol_Name = (BSTR) MKBADDR(devname);

            dol = nch->nch_VolEntry;
            dol->dol_Next = 0;
            dol->dol_Type = DLT_VOLUME;
            dol->dol_Task = nch->nch_DOSMsgPort;
            dol->dol_Lock = 0;
            DateStamp(&dol->dol_misc.dol_volume.dol_VolumeDate);
            dol->dol_misc.dol_volume.dol_LockList = 0;
            dol->dol_misc.dol_volume.dol_DiskType = ID_DOS_DISK;
            devname = (STRPTR) (dol + 1);
            c2bstr(nch->nch_RootObject.poi_Name, devname);
            dol->dol_Name = (BSTR) MKBADDR(devname);

            KPRINTF(10, ("Attempt adding DOSList...\n"));
            if(AddDosEntry(nch->nch_DevEntry))
            {
                KPRINTF(5, ("Added Device successful!\n"));
                nch->nch_ResFlags |= PTPF_DEVNODE;
            } else {
                KPRINTF(20, ("Could not add Device DOSList\n"));
            }

            if(AttemptLockDosList(LDF_DEVICES|LDF_WRITE))
            {
                if(AddDosEntry(nch->nch_VolEntry))
                {
                    KPRINTF(5, ("Added Volume!\n"));
                    nch->nch_ResFlags |= PTPF_VOLNODE;
                } else {
                    KPRINTF(20, ("Could not add Volume DOSList\n"));
                }
                UnLockDosList(LDF_DEVICES|LDF_WRITE);
            }
        }

        //psdSendPipe(nch->nch_EPIntPipe, eventbuf, 20);
        do
        {
            sigs = Wait(sigmask);
            if(sigs & SIGBREAKF_CTRL_C)
            {
                nch->nch_ShallExit = TRUE;
            }

            nHandleDOSPackets(nch);

            if(nch->nch_ShallExit)
            {
                if(!nch->nch_VolEntry->dol_misc.dol_volume.dol_LockList)
                {
                    nch->nch_ResFlags &= ~PTPF_LOCKS;
                } else {
                    struct FileLock *fl = (struct FileLock *) (&nch->nch_VolEntry->dol_misc.dol_volume.dol_LockList);
                    KPRINTF(20, ("Still locks there %08lx: %08lx.\n", fl, fl->fl_Link));
                    if(reportlocks)
                    {
                        reportlocks = FALSE;
                        psdAddErrorMsg(RETURN_ERROR, (STRPTR) libname, "Cannot exit PTP session, there are still pending %s!", (STRPTR) "locks");

                        while((fl = (struct FileLock *) BADDR(fl->fl_Link)))
                        {
                            KPRINTF(20, ("Locked on %08lx '%s'\n", fl, ((struct PTPObjectInfo *) fl->fl_Key)->poi_Name));
                            psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                           "Pending lock on '%s'",
                                           ((struct PTPObjectInfo *) fl->fl_Key)->poi_Name);

                        }
                    }
                    nch->nch_ResFlags |= PTPF_LOCKS;
                }
                if(IsListEmpty(&nch->nch_FHs))
                {
                    nch->nch_ResFlags &= ~PTPF_FHS;
                } else {
                    KPRINTF(10, ("Still filehandles there.\n"));
                    if(reportfhs)
                    {
                        reportfhs = FALSE;
                        psdAddErrorMsg(RETURN_ERROR, (STRPTR) libname, "Cannot exit PTP session, there are still pending %s!", (STRPTR) "filehandles");
                    }
                    nch->nch_ResFlags |= PTPF_FHS;
                }
                if(nch->nch_ResFlags & (PTPF_DEVNODE|PTPF_VOLNODE))
                {
                    KPRINTF(10, ("Attempting removal of DOS Entry!\n"));
                    if(AttemptLockDosList(LDF_DEVICES|LDF_WRITE))
                    {
                        if(nch->nch_ResFlags & PTPF_VOLNODE)
                        {
                            if(RemDosEntry(nch->nch_VolEntry))
                            {
                                KPRINTF(5, ("DOS Volume Entry removed!\n"));
                                nch->nch_ResFlags &= ~PTPF_VOLNODE;
                            }
                        }
                        if(nch->nch_ResFlags & PTPF_DEVNODE)
                        {
                            if(RemDosEntry(nch->nch_DevEntry))
                            {
                                KPRINTF(5, ("DOS Device Entry removed!\n"));
                                nch->nch_ResFlags &= ~PTPF_DEVNODE;
                            }
                        }
                        UnLockDosList(LDF_DEVICES|LDF_WRITE);
                    }
                }
            }

            /*while((pp = (struct PsdPipe *) GetMsg(nch->nch_TaskMsgPort)))
            {
                if(pp == nch->nch_EPIntPipe)
                {
                    pktlen = psdGetPipeActual(pp);
                    if(!(ioerr = psdGetPipeError(pp)))
                    {
                        KPRINTF(1, ("Int Packet returned %ld bytes!\n", pktlen));
                    } else {
                        KPRINTF(1, ("Int Pipe failed %ld\n", ioerr));
                        psdDelayMS(20);
                    }
                    if(!nch->nch_ShallExit)
                    {
                        psdSendPipe(nch->nch_EPIntPipe, eventbuf, 20);
                    }
                    break;
                }
            }*/
        } while(nch->nch_ResFlags || (!nch->nch_ShallExit));

        nPTPCloseSession(nch);

        KPRINTF(20, ("Going down the river!\n"));
        /*psdAbortPipe(nch->nch_EPIntPipe);
        psdWaitPipe(nch->nch_EPIntPipe);*/
        nFreePTP(nch);
    }
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nAllocPTP()" */
struct NepClassPTP * nAllocPTP(void)
{
    struct NepPTPBase *nh;
    struct Task *thistask;
    struct NepClassPTP *nch;
    struct PTPObjectInfo *poi;
    UWORD cnt;

    thistask = FindTask(NULL);

    nch = thistask->tc_UserData;
    nh = nch->nch_ClsBase;
    if(thistask->tc_Node.ln_Type != NT_PROCESS)
    {
        KPRINTF(10, ("Can't run as task!\n"));
        return(NULL);
    }
    do
    {
        if(!(nch->nch_Base = OpenLibrary("poseidon.library", 4)))
        {
            Alert(AG_OpenLib);
            break;
        }
        if(!(nch->nch_DOSBase = OpenLibrary("dos.library", 36)))
        {
            Alert(AG_OpenLib|AO_DOSLib);
            break;
        }
        nch->nch_MemPool = CreatePool(MEMF_CLEAR|MEMF_PUBLIC, 16384, 128);
        if(!nch->nch_MemPool)
        {
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
        nch->nch_EPInt = psdFindEndpoint(nch->nch_Interface, NULL,
                                         EA_IsIn, TRUE,
                                         EA_TransferType, USEAF_INTERRUPT,
                                         TAG_END);

        if(!(nch->nch_EPIn && nch->nch_EPOut))
        //if(!(nch->nch_EPIn && nch->nch_EPOut && nch->nch_EPInt))
        {
            KPRINTF(1, ("Ooops!?! No Endpoints defined?\n"));
            break;
        }
        psdGetAttrs(PGA_ENDPOINT, nch->nch_EPIn,
                    EA_MaxPktSize, &nch->nch_EPInPktSize,
                    TAG_END);

        // init root
        poi = &nch->nch_RootObject;
        KPRINTF(1, ("Root ObjectInfo %08lx\n", poi));
        NewList((struct List *) &poi->poi_Children);
        poi->poi_Parent = NULL;
        poi->poi_Flags = PTPF_NOPURGE|PTPF_FETCHED;
        poi->poi_Handle = 0xffffffff; // as root
        poi->poi_StorageID = 0xffffffff;
        poi->poi_ObjectFmt = POF_ASSOCIATION; // directory
        poi->poi_ProtFlags = FIBF_READ;
        poi->poi_ParentHandle = 0; // no parent
        poi->poi_Name = NULL;
        poi->poi_Keywords = NULL;

        // init charmap
        for(cnt = 0; cnt < 256; cnt++)
        {
            nch->nch_LowCharMap[cnt] = ToLower((ULONG) cnt);
        }

        if((nch->nch_DevEntry = nAllocVec(nch, sizeof(struct DosList) + 32)))
        {
            if((nch->nch_VolEntry = nAllocVec(nch, sizeof(struct DosList) + 32)))
            {
                nch->nch_DOSMsgPort = &((struct Process *) thistask)->pr_MsgPort;
                if((nch->nch_TaskMsgPort = CreateMsgPort()))
                {
                    if((nch->nch_EP0Pipe = psdAllocPipe(nch->nch_Device, nch->nch_TaskMsgPort, NULL)))
                    {
                        if((nch->nch_EPInPipe = psdAllocPipe(nch->nch_Device, nch->nch_TaskMsgPort, nch->nch_EPIn)))
                        {
                            if((nch->nch_EPOutPipe = psdAllocPipe(nch->nch_Device, nch->nch_TaskMsgPort, nch->nch_EPOut)))
                            {
                                //if((nch->nch_EPIntPipe = psdAllocPipe(nch->nch_Device, nch->nch_TaskMsgPort, nch->nch_EPInt)))
                                {
                                    psdSetAttrs(PGA_PIPE, nch->nch_EPInPipe,
                                                PPA_AllowRuntPackets, TRUE,
                                                PPA_NakTimeout, TRUE,
                                                PPA_NakTimeoutTime, 5000,
                                                TAG_END);
                                    psdSetAttrs(PGA_PIPE, nch->nch_EPOutPipe,
                                                PPA_NoZeroPktTerm, FALSE,
                                                PPA_NakTimeout, TRUE,
                                                PPA_NakTimeoutTime, 5000,
                                                TAG_END);
                                    nch->nch_Task = thistask;
                                    return(nch);
                                }
                                //psdFreePipe(nch->nch_EPOutPipe);
                            }
                            psdFreePipe(nch->nch_EPInPipe);
                        }
                        psdFreePipe(nch->nch_EP0Pipe);
                    }
                    DeleteMsgPort(nch->nch_TaskMsgPort);
                }
                nFreeVec(nch, nch->nch_VolEntry);
            }
            nFreeVec(nch, nch->nch_DevEntry);
        }
        DeletePool(nch->nch_MemPool);
    } while(FALSE);
    CloseLibrary(nch->nch_DOSBase);
    nch->nch_DOSBase = NULL;
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

/* /// "nFreePTP()" */
void nFreePTP(struct NepClassPTP *nch)
{
    struct PTPStorageInfo *psi;
    psi = (struct PTPStorageInfo *) nch->nch_Storages.lh_Head;
    while(psi->psi_Node.ln_Succ)
    {
        Remove(&psi->psi_Node);
        nPTPFreeStorageInfo(nch, psi);
        psi = (struct PTPStorageInfo *) nch->nch_Storages.lh_Head;
    }
    nch->nch_LastObject = NULL; // Invalidate last object
    nFreeVec(nch, nch->nch_LastBuffer);
    nch->nch_LastBuffer = NULL;

    psdFreePipe(nch->nch_EPInPipe);
    psdFreePipe(nch->nch_EPOutPipe);
    psdFreePipe(nch->nch_EPIntPipe);
    psdFreePipe(nch->nch_EP0Pipe);
    DeleteMsgPort(nch->nch_TaskMsgPort);
    nFreeVec(nch, nch->nch_VolEntry);
    nFreeVec(nch, nch->nch_DevEntry);
    DeletePool(nch->nch_MemPool);
    CloseLibrary(nch->nch_DOSBase);
    nch->nch_DOSBase = NULL;
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

/* /// "nGUITask()" */
AROS_UFH0(void, nGUITask)
{
    AROS_USERFUNC_INIT

    struct Task *thistask;
    struct NepPTPBase *nh;
    struct NepClassPTP *nch;
    APTR pic;

    thistask = FindTask(NULL);
#undef ps
#define ps nch->nch_PsdBase
#undef IntuitionBase
#define IntuitionBase nch->nch_IntBase
#undef MUIMasterBase
#define MUIMasterBase nch->nch_MUIBase
#undef DOSBase
#define DOSBase nch->nch_DOSBase

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
        MUIA_Application_Copyright  , "©2008-2009 Chris Hodges",
        MUIA_Application_Author     , "Chris Hodges <chrisly@platon42.de>",
        MUIA_Application_Description, "Settings for the ptp.class",
        MUIA_Application_Base       , "PTP",
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
                Child, VGroup, GroupFrameT("PTP Settings"),
                    Child, HGroup,
                        Child, Label((ULONG) "DOS Device Name:"),
                        Child, nch->nch_DOSNameObj = StringObject,
                            StringFrame,
                            MUIA_CycleChain, 1,
                            MUIA_String_AdvanceOnCR, TRUE,
                            MUIA_String_Contents, nch->nch_CDC->cdc_DOSName,
                            MUIA_String_Reject, "/ :?#*",
                            MUIA_String_MaxLen, 31,
                            End,
                        End,
                    Child, ColGroup(2),
                        Child, Label((ULONG) "Always fully load and cache objects:"),
                        Child, nch->nch_NoPartObjObj = ImageObject, ImageButtonFrame,
                            MUIA_Background, MUII_ButtonBack,
                            MUIA_CycleChain, 1,
                            MUIA_InputMode, MUIV_InputMode_Toggle,
                            MUIA_Image_Spec, MUII_CheckMark,
                            MUIA_Image_FreeVert, TRUE,
                            MUIA_Selected, nch->nch_CDC->cdc_NoPartObj,
                            MUIA_ShowSelState, FALSE,
                            End,
                        Child, Label((ULONG) "Try to detect MTP devices:"),
                        Child, nch->nch_EnableMTPObj = ImageObject, ImageButtonFrame,
                            MUIA_Disabled, nch->nch_Interface ? TRUE : FALSE,
                            MUIA_Background, MUII_ButtonBack,
                            MUIA_CycleChain, 1,
                            MUIA_InputMode, MUIV_InputMode_Toggle,
                            MUIA_Image_Spec, MUII_CheckMark,
                            MUIA_Image_FreeVert, TRUE,
                            MUIA_Selected, nch->nch_CDC->cdc_EnableMTP,
                            MUIA_ShowSelState, FALSE,
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
        sigmask = 0;
        do
        {
            retid = DoMethod(nch->nch_App, MUIM_Application_NewInput, &sigs);
            switch(retid)
            {
                case ID_DEF_CONFIG:
                case ID_STORE_CONFIG:
                case MUIV_Application_ReturnID_Quit:
                {
                    STRPTR tmpstr;

                    get(nch->nch_EnableMTPObj, MUIA_Selected, &nch->nch_CDC->cdc_EnableMTP);
                    get(nch->nch_NoPartObjObj, MUIA_Selected, &nch->nch_CDC->cdc_NoPartObj);
                    get(nch->nch_DOSNameObj, MUIA_String_Contents, &tmpstr);
                    strncpy(nch->nch_CDC->cdc_DOSName, tmpstr, 31);

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
                    if(nch->nch_Interface)
                    {
                        pic = psdGetUsbDevCfg(libname, nch->nch_DevIDString, nch->nch_IfIDString);
                        if(!pic)
                        {
                            psdSetUsbDevCfg(libname, nch->nch_DevIDString, nch->nch_IfIDString, NULL);
                            pic = psdGetUsbDevCfg(libname, nch->nch_DevIDString, nch->nch_IfIDString);
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
                }

                case ID_ABOUT:
                    MUI_RequestA(nch->nch_App, nch->nch_MainWindow, 0, NULL, "Moo!", VERSION_STRING, NULL);
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
        set(nch->nch_MainWindow, MUIA_Window_Open, FALSE);
    }
    nGUITaskCleanup(nch);
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nGUITaskCleanup()" */
void nGUITaskCleanup(struct NepClassPTP *nch)
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

