/*
 *----------------------------------------------------------------------------
 *                         usbaudio class for poseidon
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "debug.h"

#include "numtostr.h"
#include "usbaudio.class.h"

#include <proto/ahi.h>

#define SUBLIBNAME          "usbaudio.audio"

/* /// "Lib Stuff" */
static const STRPTR libname = MOD_NAME_STRING;

static
const APTR SubLibFuncTable[] =
{
    &AROS_SLIB_ENTRY(subLibOpen, nep, 1),
    &AROS_SLIB_ENTRY(subLibClose, nep, 2),
    &AROS_SLIB_ENTRY(subLibExpunge, nep, 3),
    &AROS_SLIB_ENTRY(subLibReserved, nep, 4),
    &AROS_SLIB_ENTRY(subLibAllocAudio, nep, 5),
    &AROS_SLIB_ENTRY(subLibFreeAudio, nep, 6),
    &AROS_SLIB_ENTRY(subLibDisable, nep, 7),
    &AROS_SLIB_ENTRY(subLibEnable, nep, 8),
    &AROS_SLIB_ENTRY(subLibStart, nep, 9),
    &AROS_SLIB_ENTRY(subLibUpdate, nep, 10),
    &AROS_SLIB_ENTRY(subLibStop, nep, 11),
    &AROS_SLIB_ENTRY(subLibSetVol, nep, 12),
    &AROS_SLIB_ENTRY(subLibSetFreq, nep, 13),
    &AROS_SLIB_ENTRY(subLibSetSound, nep, 14),
    &AROS_SLIB_ENTRY(subLibSetEffect, nep, 15),
    &AROS_SLIB_ENTRY(subLibLoadSound, nep, 16),
    &AROS_SLIB_ENTRY(subLibUnloadSound, nep, 17),
    &AROS_SLIB_ENTRY(subLibGetAttr, nep, 18),
    &AROS_SLIB_ENTRY(subLibHardwareControl, nep, 19),
    (APTR) -1,
};

static int libInit(LIBBASETYPEPTR nh)
{
    struct NepAudioBase *ret = NULL;

    KPRINTF(10, ("libInit nh: 0x%08lx SysBase: 0x%08lx\n", nh, SysBase));

    nh->nh_UtilityBase = OpenLibrary("utility.library", 39);

#define	UtilityBase	nh->nh_UtilityBase

    if(UtilityBase)
    {
        /* Create default config */
        nh->nh_CurrentCGC.cgc_ChunkID = AROS_LONG2BE(MAKE_ID('U','A','U','D'));
        nh->nh_CurrentCGC.cgc_Length = AROS_LONG2BE(sizeof(struct ClsGlobalCfg)-8);

        NewList(&nh->nh_Units);

        KPRINTF(1, ("Before MakeLibrary\n"));
        if((nh->nh_SubLibBase = (struct NepAudioSubLibBase *) MakeLibrary((APTR) SubLibFuncTable,
                                                                         NULL, 
                                                                         (APTR) subLibInit,
                                                                         sizeof(struct NepAudioSubLibBase), NULL)))
        {
            KPRINTF(1, ("After MakeLibrary\n"));

            nh->nh_SubLibBase->nas_ClsBase = nh;
            Forbid();
            AddLibrary((struct Library *) nh->nh_SubLibBase);
            nh->nh_SubLibBase->nas_Library.lib_OpenCnt++;
            Permit();
            ret = nh;
        } else {
            KPRINTF(20, ("failed to create usbaudio.audio\n"));
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
    struct NepClassAudio *nch;
    struct NepAudioMode *nam;

    KPRINTF(10, ("libExpunge nh: 0x%08lx\n", nh));

    if(nh->nh_SubLibBase->nas_Library.lib_OpenCnt == 1)
    {
        KPRINTF(1, ("libExpunge: closelibrary utilitybase 0x%08lx\n",
                    UtilityBase));
        CloseLibrary((struct Library *) UtilityBase);

        nch = (struct NepClassAudio *) nh->nh_Units.lh_Head;
        while(nch->nch_Node.ln_Succ)
        {
            Remove((struct Node *) nch);
            nam = (struct NepAudioMode *) nch->nch_AudioModes.lh_Head;
            while(nam->nam_Node.ln_Succ)
            {
                Remove(&nam->nam_Node);
                FreeVec(nam);
                nam = (struct NepAudioMode *) nch->nch_AudioModes.lh_Head;
            }

            FreeVec(nch);
            nch = (struct NepClassAudio *) nh->nh_Units.lh_Head;
        }

        nh->nh_SubLibBase->nas_Library.lib_OpenCnt--;
        RemLibrary((struct Library *) nh->nh_SubLibBase);
        KPRINTF(5, ("libExpunge: Unloading done! usbaudio.class expunged!\n\n"));
    } else {
        KPRINTF(5, ("libExpunge: Could not expunge, LIBF_DELEXP set!\n"));
        return(FALSE);
    }

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

#undef UtilityBase
#define	UtilityBase	nh->nh_UtilityBase

/* /// "nVerifyAudioStreamClass()" */
BOOL nVerifyAudioStreamDevice(struct Library *ps, struct PsdInterface *pif)
{
    struct PsdConfig *pc;
    struct PsdDevice *pd;
    struct PsdInterface *subpif;
    struct UsbAudioHeaderDesc10 *header = NULL;
    IPTR ifclass;
    IPTR subclass;
    IPTR proto;
    UWORD bcdvers;
    UWORD cnt;
    APTR desc;

    psdGetAttrs(PGA_INTERFACE, pif,
                    IFA_Config, &pc,
                    IFA_Class, &ifclass,
                    IFA_SubClass, &subclass,
                    IFA_Protocol, &proto,
                    TAG_DONE);

    psdGetAttrs(PGA_CONFIG, pc, CA_Device, &pd, TAG_END);

    if((ifclass == AUDIO_CLASSCODE) && (subclass == AUDIO_CTRL_SUBCLASS))
    {
        desc = psdFindDescriptor(pd, NULL,
                                 DDA_Interface, pif,
                                 DDA_DescriptorType, UDT_CS_INTERFACE,
                                 DDA_CS_SubType, UDST_AUDIO_CTRL_HEADER,
                                 TAG_END);

        if(desc)
        {
            psdGetAttrs(PGA_DESCRIPTOR, desc, DDA_DescriptorData, &header, TAG_END);
            bcdvers = (header->bcdADC0|(header->bcdADC1<<8));
            if(bcdvers == 0x0100)
            {
                for(cnt = 0; cnt < header->bInCollection; cnt++)
                {
                    subpif = psdFindInterface(pd, NULL,
                                              IFA_InterfaceNum, header->baInterfaceNr[cnt],
                                              IFA_Class, AUDIO_CLASSCODE,
                                              IFA_SubClass, AUDIO_STREAM_SUBCLASS,
                                              TAG_END);
                    if(subpif)
                    {
                        return(TRUE);
                    }
                }
            } else {
                psdAddErrorMsg(RETURN_WARN, (STRPTR) libname, "Unsupported audio class spec version %lx.%2lx", bcdvers>>8, bcdvers & 0xff);
            }
        }
    }
    return(FALSE);
}
/* \\\ */

/* /// "usbAttemptInterfaceBinding()" */
struct NepClassAudio * usbAttemptInterfaceBinding(struct NepAudioBase *nh, struct PsdInterface *pif)
{
    struct Library *ps;
    BOOL isaudio;

    KPRINTF(1, ("nepAudioAttemptInterfaceBinding(%08lx)\n", pif));
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        isaudio = nVerifyAudioStreamDevice(ps, pif);
        CloseLibrary(ps);
        if(isaudio)
        {
            return(usbForceInterfaceBinding(nh, pif));
        }
    }
    return(NULL);
}
/* \\\ */

/* /// "usbForceInterfaceBinding()" */
struct NepClassAudio * usbForceInterfaceBinding(struct NepAudioBase *nh, struct PsdInterface *pif)
{
    struct Library *ps;
    struct NepClassAudio *nch;
    struct PsdConfig *pc;
    struct PsdDevice *pd;
    STRPTR devname;
    IPTR altifnum;
    IPTR ifnum;
    IPTR cfgnum;
    IPTR prodid;
    IPTR vendid;
    ULONG unitno;
    BOOL  unitfound;
    UBYTE buf[64];
    struct Task *tmptask;

    KPRINTF(1, ("nepAudioAttemptInterfaceBinding(%08lx)\n", pif));
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        psdGetAttrs(PGA_INTERFACE, pif,
                    IFA_InterfaceNum, &ifnum,
                    IFA_AlternateNum, &altifnum,
                    IFA_Config, &pc,
                    TAG_DONE);
        psdGetAttrs(PGA_CONFIG, pc,
                    CA_Device, &pd,
                    CA_ConfigNum, &cfgnum,
                    TAG_END);
        psdGetAttrs(PGA_DEVICE, pd,
                    DA_ProductID, &prodid,
                    DA_VendorID, &vendid,
                    DA_ProductName, &devname,
                    TAG_END);
        unitno = 0;
        nch = (struct NepClassAudio *) nh->nh_Units.lh_Head;
        while(nch->nch_Node.ln_Succ)
        {
            if(nch->nch_UnitNo == unitno)
            {
                unitno++;
                nch = (struct NepClassAudio *) nh->nh_Units.lh_Head;
            } else {
                nch = (struct NepClassAudio *) nch->nch_Node.ln_Succ;
            }
        }
        unitfound = FALSE;
        nch = (struct NepClassAudio *) nh->nh_Units.lh_Head;
        while(nch->nch_Node.ln_Succ)
        {
            if((nch->nch_UnitAltIfNum == altifnum) && (nch->nch_UnitIfNum == ifnum) &&
               (nch->nch_UnitProdID == prodid) && (nch->nch_UnitVendorID == vendid))
            {
                unitno = nch->nch_UnitNo;
                unitfound = TRUE;
                break;
            }
            nch = (struct NepClassAudio *) nch->nch_Node.ln_Succ;
        }
        if(!unitfound)
        {
            /* as units are freed in the expunge-vector, the memory is
               outside the scope of the poseidon library */
            if(!(nch = AllocVec(sizeof(struct NepClassAudio), MEMF_PUBLIC|MEMF_CLEAR)))
            {
                Permit();
                CloseLibrary(ps);
                return(NULL);
            }
            nch->nch_ClsBase = nh;
            nch->nch_Device = NULL;
            nch->nch_Interface = pif;
            nch->nch_UnitNo = unitno;
            NewList(&nch->nch_AudioModes);
            NewList(&nch->nch_AudioUnits);
            Forbid();
            AddTail(&nh->nh_Units, &nch->nch_Node);
            Permit();
        }
        nch->nch_DenyRequests = TRUE;
 
        nLoadClassConfig(nh);

        psdSafeRawDoFmt(buf, 64, "usbaudio.class<%08lx>", nch);
        nch->nch_ReadySignal = SIGB_SINGLE;
        nch->nch_ReadySigTask = FindTask(NULL);
        SetSignal(0, SIGF_SINGLE);
        if((tmptask = psdSpawnSubTask(buf, nAudioTask, nch)))
        {
            psdBorrowLocksWait(tmptask, 1UL<<nch->nch_ReadySignal);
            if(nch->nch_Task)
            {
                nch->nch_ReadySigTask = NULL;
                //FreeSignal(nch->nch_ReadySignal);

                psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                               "Play it again, '%s'!",
                               devname);
                CloseLibrary(ps);
                return(nch);
            }
        }
        nch->nch_ReadySigTask = NULL;
        //FreeSignal(nch->nch_ReadySignal);
        CloseLibrary(ps);
    }
    return(NULL);
}
/* \\\ */

/* /// "usbReleaseInterfaceBinding()" */
void usbReleaseInterfaceBinding(struct NepAudioBase *nh, struct NepClassAudio *nch)
{
    struct Library *ps;
    struct PsdConfig *pc;
    struct PsdDevice *pd;
    STRPTR devname;

    KPRINTF(1, ("nepAudioReleaseInterfaceBinding(%08lx)\n", nch));
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        Forbid();
        nch->nch_DenyRequests = TRUE;
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

    KPRINTF(1, ("nepAudioGetAttrsA(%ld, %08lx, %08lx)\n", type, usbstruct, tags));
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
                 *((STRPTR *) ti->ti_Data) = "USB Audio Streaming Interface class";
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
             if((ti = FindTagItem(UCCA_SupportsSuspend, tags)))
             {
                 *((IPTR *) ti->ti_Data) = TRUE;
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

    struct NepClassAudio *nch;
    KPRINTF(10, ("Do Method %ld\n", methodid));
    switch(methodid)
    {
        case UCM_AttemptInterfaceBinding:
            return((IPTR) usbAttemptInterfaceBinding(nh, (struct PsdInterface *) methoddata[0]));

        case UCM_ForceInterfaceBinding:
            return((IPTR) usbForceInterfaceBinding(nh, (struct PsdInterface *) methoddata[0]));

        case UCM_ReleaseInterfaceBinding:
            usbReleaseInterfaceBinding(nh, (struct NepClassAudio *) methoddata[0]);
            return(TRUE);

        case UCM_OpenCfgWindow:
            return(nOpenCfgWindow(nh));

        case UCM_ConfigChangedEvent:
            nLoadClassConfig(nh);
            return(TRUE);

        case UCM_AttemptSuspendDevice:
            nch = (struct NepClassAudio *) methoddata[0];
            return(nch->nch_CurrentMode ? FALSE : TRUE);

        case UCM_AttemptResumeDevice:
            nch = (struct NepClassAudio *) methoddata[0];
            Signal(nch->nch_Task, (1L<<nch->nch_TaskMsgPort->mp_SigBit));
            return(TRUE);

        default:
            break;
    }
    return(0);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/**************************************************************************/

#undef  ps
#define ps nch->nch_Base
#undef AHIBase
#define AHIBase nch->nch_AHIBase

static const ULONG commonFreqs[] =
{
     4410,  4800,  5513,  6000,  7350,  8000,  9600,
    11025, 12000, 14700, 16000, 17640, 18900, 19200,
    22050, 24000, 29400, 32000, 33075, 37800, 44100,
    48000, 52000, 56000, 60000, 64000,
    // AHI doesn't support freqs > 65535 Hz
    /*72000, 80000, 88200, 96000, */
    0
};

/*static UWORD volumeTableLog[256] =
{
    0x0000, 0xFFE9, 0xFFD1, 0xFFBA, 0xFFA2, 0xFF8B, 0xFF72, 0xFF5A,
    0xFF41, 0xFF28, 0xFF0F, 0xFEF5, 0xFEDB, 0xFEC1, 0xFEA7, 0xFE8C,
    0xFE71, 0xFE56, 0xFE3A, 0xFE1E, 0xFE02, 0xFDE5, 0xFDC8, 0xFDAB,
    0xFD8E, 0xFD70, 0xFD52, 0xFD33, 0xFD14, 0xFCF5, 0xFCD5, 0xFCB5,
    0xFC95, 0xFC74, 0xFC53, 0xFC32, 0xFC10, 0xFBEE, 0xFBCC, 0xFBA9,
    0xFB85, 0xFB62, 0xFB3E, 0xFB19, 0xFAF4, 0xFACF, 0xFAA9, 0xFA83,
    0xFA5D, 0xFA36, 0xFA0E, 0xF9E6, 0xF9BE, 0xF995, 0xF96C, 0xF942,
    0xF918, 0xF8ED, 0xF8C2, 0xF897, 0xF86B, 0xF83E, 0xF811, 0xF7E3,
    0xF7B5, 0xF787, 0xF758, 0xF728, 0xF6F8, 0xF6C7, 0xF696, 0xF664,
    0xF632, 0xF5FF, 0xF5CB, 0xF597, 0xF563, 0xF52D, 0xF4F8, 0xF4C1,
    0xF48A, 0xF452, 0xF41A, 0xF3E1, 0xF3A8, 0xF36E, 0xF333, 0xF2F7,
    0xF2BB, 0xF27E, 0xF241, 0xF203, 0xF1C4, 0xF184, 0xF144, 0xF103,
    0xF0C1, 0xF07E, 0xF03B, 0xEFF7, 0xEFB3, 0xEF6D, 0xEF27, 0xEEE0,
    0xEE98, 0xEE4F, 0xEE06, 0xEDBB, 0xED70, 0xED24, 0xECD7, 0xEC8A,
    0xEC3B, 0xEBEC, 0xEB9C, 0xEB4A, 0xEAF8, 0xEAA5, 0xEA51, 0xE9FC,
    0xE9A7, 0xE950, 0xE8F8, 0xE89F, 0xE845, 0xE7EB, 0xE78F, 0xE732,
    0xE6D4, 0xE675, 0xE616, 0xE5B5, 0xE552, 0xE4EF, 0xE48B, 0xE426,
    0xE3BF, 0xE357, 0xE2EF, 0xE285, 0xE219, 0xE1AD, 0xE13F, 0xE0D0,
    0xE060, 0xDFEF, 0xDF7C, 0xDF09, 0xDE93, 0xDE1D, 0xDDA5, 0xDD2C,
    0xDCB2, 0xDC36, 0xDBB8, 0xDB3A, 0xDABA, 0xDA38, 0xD9B5, 0xD931,
    0xD8AB, 0xD824, 0xD79B, 0xD710, 0xD684, 0xD5F7, 0xD568, 0xD4D7,
    0xD445, 0xD3B1, 0xD31B, 0xD284, 0xD1EB, 0xD150, 0xD0B4, 0xD016,
    0xCF76, 0xCED4, 0xCE31, 0xCD8B, 0xCCE4, 0xCC3B, 0xCB90, 0xCAE3,
    0xCA34, 0xC984, 0xC8D1, 0xC81C, 0xC765, 0xC6AD, 0xC5F2, 0xC535,
    0xC476, 0xC3B5, 0xC2F1, 0xC22C, 0xC164, 0xC09A, 0xBFCE, 0xBF00,
    0xBE2F, 0xBD5C, 0xBC86, 0xBBAE, 0xBAD4, 0xB9F7, 0xB918, 0xB836,
    0xB752, 0xB66B, 0xB582, 0xB496, 0xB3A8, 0xB2B6, 0xB1C2, 0xB0CC,
    0xAFD2, 0xAED6, 0xADD7, 0xACD5, 0xABD1, 0xAAC9, 0xA9BE, 0xA8B1,
    0xA7A0, 0xA68C, 0xA576, 0xA45C, 0xA33F, 0xA21F, 0xA0FB, 0x9FD5,
    0x9EAB, 0x9D7D, 0x9C4D, 0x9B19, 0x99E1, 0x98A6, 0x9768, 0x9626,
    0x94E0, 0x9397, 0x924A, 0x90F9, 0x8FA5, 0x8E4D, 0x8CF1, 0x8B91,
    0x8A2D, 0x88C5, 0x8759, 0x85E9, 0x8475, 0x82FD, 0x8180, 0x8000
};*/

/* /// "nGetInputUnit()" */
struct NepAudioUnit * nGetInputUnit(struct NepAudioMode *nam)
{
    struct NepAudioUnit *nau = NULL;
    if(nam->nam_Sibling)
    {
        if(nam->nam_Sibling->nam_RootUnit->nau_SelectorUnit)
        {
            nau = nam->nam_Sibling->nam_RootUnit->nau_SelectorUnit->nau_InputUnit[nam->nam_CurrInput]->nau_RootUnit;
        } else {
            nau = nam->nam_Sibling->nam_RootUnit;
        }
    }
    return(nau);
}
/* \\\ */

/* /// "nAudioTask()" */
AROS_UFH0(void, nAudioTask)
{
    AROS_USERFUNC_INIT

    struct NepClassAudio *nch;
    struct NepAudioMode *nam;
    struct NepAudioUnit *innau;
    ULONG sigmask;
    ULONG sigs;
    UBYTE buf[2];
    LONG ioerr;

    if((nch = nAllocAudio()))
    {
        Forbid();
        if(nch->nch_ReadySigTask)
        {
            Signal(nch->nch_ReadySigTask, 1L<<nch->nch_ReadySignal);
        }
        Permit();
        sigmask = (1L<<nch->nch_TaskMsgPort->mp_SigBit)|SIGBREAKF_CTRL_C;
        do
        {
            //psdDoPipe(nch->nch_CurrentMode->nam_EPPipe, nch, 1024);
            sigs = Wait(sigmask);
            if(nch->nch_UpdateFlags)
            {
                nam = nch->nch_CurrentMode;
                if((nch->nch_UpdateFlags & UAF_SELECT_OUTPUT) && nam->nam_RootUnit->nau_SelectorUnit)
                {
                    struct NepAudioUnit *nau = nam->nam_RootUnit->nau_SelectorUnit;
                    KPRINTF(10, ("Setting output %ld to %08lx\n", nau->nau_UnitID, nam->nam_CurrOutput));
                    buf[0] = nam->nam_CurrOutput + 1;
                    psdPipeSetup(nch->nch_EP0Pipe, URTF_CLASS|URTF_INTERFACE, UAUDR_SET_CUR, 0, (nau->nau_UnitID<<8)|nch->nch_IfNum);
                    ioerr = psdDoPipe(nch->nch_EP0Pipe, buf, 1);
                    if(ioerr)
                    {
                        psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                       "Could not select output source: %s (%ld)!",
                                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                    }
                }
                if((nch->nch_UpdateFlags & UAF_SELECT_INPUT) && nam->nam_Sibling && nam->nam_Sibling->nam_RootUnit->nau_SelectorUnit)
                {
                    struct NepAudioUnit *nau = nam->nam_Sibling->nam_RootUnit->nau_SelectorUnit;
                    KPRINTF(10, ("Setting input %ld to %08lx\n", nau->nau_UnitID, nam->nam_CurrInput));
                    buf[0] = nam->nam_CurrInput + 1;
                    psdPipeSetup(nch->nch_EP0Pipe, URTF_CLASS|URTF_INTERFACE, UAUDR_SET_CUR, 0, (nau->nau_UnitID<<8)|nch->nch_IfNum);
                    ioerr = psdDoPipe(nch->nch_EP0Pipe, buf, 1);
                    if(ioerr)
                    {
                        psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                       "Could not select input source: %s (%ld)!",
                                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                    }
                }
                if((nch->nch_UpdateFlags & UAF_MASTER_VOLUME) && nam->nam_RootUnit->nau_VolumeControl)
                {
                    struct NepAudioUnit *nau = nam->nam_RootUnit->nau_VolumeUnit;
                    Fixed vol;
                    UWORD cnt;

                    vol = (nam->nam_MasterVol * (nau->nau_MaxVolDb - nau->nau_MinVolDb)>>16) + nau->nau_MinVolDb;
                    if(vol < nau->nau_MinVolDb)
                    {
                        // we should rather MUTE this channel completely.
                        vol = nau->nau_MinVolDb;
                    }
                    else if(vol > nau->nau_MaxVolDb)
                    {
                        vol = nau->nau_MaxVolDb;
                    }
                    KPRINTF(10, ("Setting master volume %ld to %08lx (%04lx)\n", nau->nau_UnitID, nam->nam_MasterVol, vol));
                    buf[0] = vol;
                    buf[1] = vol>>8;
                    for(cnt = 0; cnt < 3; cnt++)
                    {
                        if(nau->nau_VolCtrlMask & (1<<cnt))
                        {
                            psdPipeSetup(nch->nch_EP0Pipe, URTF_CLASS|URTF_INTERFACE, UAUDR_SET_CUR, (ULONG) (UAFUCS_VOLUME<<8)|cnt, (nau->nau_UnitID<<8)|nch->nch_IfNum);
                            ioerr = psdDoPipe(nch->nch_EP0Pipe, buf, 2);
                            if(ioerr)
                            {
                                psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                               "Could not set master volume (%ld): %s (%ld)!",
                                               cnt,
                                               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                            } else {
                                KPRINTF(10, ("Chan %ld okay!\n", cnt));
                            }
                        } else {
                            KPRINTF(10, ("Chan %ld ignored\n", cnt));
                        }
                    }
                }
                innau = nGetInputUnit(nam);
                if((nch->nch_UpdateFlags & UAF_INPUT_GAIN) && innau && innau->nau_VolumeControl)
                {
                    struct NepAudioUnit *nau = innau->nau_VolumeUnit;
                    Fixed vol;
                    UWORD cnt;

                    vol = (nam->nam_InputGain * (nau->nau_MaxVolDb - nau->nau_MinVolDb)>>16) + nau->nau_MinVolDb;
                    if(vol < nau->nau_MinVolDb)
                    {
                        // we should rather MUTE this channel completely.
                        vol = nau->nau_MinVolDb;
                    }
                    else if(vol > nau->nau_MaxVolDb)
                    {
                        vol = nau->nau_MaxVolDb;
                    }
                    KPRINTF(10, ("Setting input gain %ld to %08lx (%04lx)\n", nau->nau_UnitID, nam->nam_InputGain, vol));
                    buf[0] = vol;
                    buf[1] = vol>>8;
                    for(cnt = 0; cnt < 3; cnt++)
                    {
                        if(nau->nau_VolCtrlMask & (1<<cnt))
                        {
                            psdPipeSetup(nch->nch_EP0Pipe, URTF_CLASS|URTF_INTERFACE, UAUDR_SET_CUR, (ULONG) (UAFUCS_VOLUME<<8)|cnt, (nau->nau_UnitID<<8)|nch->nch_IfNum);
                            ioerr = psdDoPipe(nch->nch_EP0Pipe, buf, 2);
                            if(ioerr)
                            {
                                psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                               "Could not set input gain (%ld): %s (%ld)!",
                                               cnt,
                                               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                            } else {
                                KPRINTF(10, ("Chan %ld okay!\n", cnt));
                            }
                        } else {
                            KPRINTF(10, ("Chan %ld ignored\n", cnt));
                        }
                    }
                }
                if((nch->nch_UpdateFlags & UAF_MONITOR_VOLUME) && innau && innau->nau_Monitor)
                {
                    struct NepAudioUnit *nau = innau->nau_MonitorUnit;
                    Fixed vol;
                    UWORD cnt;

                    vol = (nam->nam_MonitorVol * (nau->nau_MaxVolDb - nau->nau_MinVolDb)>>16) + nau->nau_MinVolDb;
                    if(vol < nau->nau_MinVolDb)
                    {
                        // we should rather MUTE this channel completely.
                        vol = nau->nau_MinVolDb;
                    }
                    else if(vol > nau->nau_MaxVolDb)
                    {
                        vol = nau->nau_MaxVolDb;
                    }
                    KPRINTF(10, ("Setting monitor %ld to %08lx (%04lx)\n", nau->nau_UnitID, nam->nam_MonitorVol, vol));
                    buf[0] = vol;
                    buf[1] = vol>>8;
                    for(cnt = 0; cnt < 3; cnt++)
                    {
                        if(nau->nau_VolCtrlMask & (1<<cnt))
                        {
                            psdPipeSetup(nch->nch_EP0Pipe, URTF_CLASS|URTF_INTERFACE, UAUDR_SET_CUR, (ULONG) (UAFUCS_VOLUME<<8)|cnt, (nau->nau_UnitID<<8)|nch->nch_IfNum);
                            ioerr = psdDoPipe(nch->nch_EP0Pipe, buf, 2);
                            if(ioerr)
                            {
                                psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                               "Could not set monitor volume (%ld): %s (%ld)!",
                                               cnt,
                                               psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                            } else {
                                KPRINTF(10, ("Chan %ld okay!\n", cnt));
                            }
                        } else {
                            KPRINTF(10, ("Chan %ld ignored\n", cnt));
                        }
                    }
                }
                nch->nch_UpdateFlags = 0;
            }
        } while(!(sigs & SIGBREAKF_CTRL_C));
        if(nch->nch_CurrentMode)
        {
            psdAddErrorMsg(RETURN_WARN, (STRPTR) libname, "Cannot exit yet, audio playback still active!");
            while(nch->nch_CurrentMode)
            {
                psdDelayMS(500);
            }
        }
        KPRINTF(20, ("Going down the river!\n"));
        nFreeAudio(nch);
    }
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nFindAudioUnit()" */
struct NepAudioUnit * nFindAudioUnit(struct NepClassAudio *nch, ULONG unitid)
{
    struct NepAudioUnit *nau;

    nau = (struct NepAudioUnit *) nch->nch_AudioUnits.lh_Head;
    while(nau->nau_Node.ln_Succ)
    {
        if(nau->nau_UnitID == unitid)
        {
            return(nau);
        }
        nau = (struct NepAudioUnit *) nau->nau_Node.ln_Succ;
    }
    psdAddErrorMsg(RETURN_WARN, (STRPTR) libname, "Requested Unit %02lx not found.", unitid);
    return(NULL);
}
/* \\\ */

/* /// "nFindAndConnectAudioUnit()" */
void nFindAndConnectAudioUnit(struct NepClassAudio *nch, struct NepAudioUnit *targetnau, ULONG unitid)
{
    struct NepAudioUnit *sourcenau = nFindAudioUnit(nch, unitid);
    KPRINTF(10, ("Connecting unit %02lx to %02lx\n", unitid, targetnau->nau_UnitID));
    if(!sourcenau)
    {
        return;
    }
    if(sourcenau->nau_NumOutputs < 8)
    {
        sourcenau->nau_OutputUnit[sourcenau->nau_NumOutputs] = targetnau;
        sourcenau->nau_NumOutputs++;
    }
    if(targetnau->nau_NumInputs < 8)
    {
        targetnau->nau_InputUnit[targetnau->nau_NumInputs] = sourcenau;
        targetnau->nau_NumInputs++;
    }
}
/* \\\ */

/* /// "nFlowDown()" */
void nFlowDown(struct NepClassAudio *nch, struct NepAudioUnit *nau)
{
    UWORD cnt;

    KPRINTF(10, ("FlowDown %02lx\n", nau->nau_UnitID));

    if(nau->nau_InputUnit[0]->nau_RootUnit && ((nau->nau_Type != UDST_AUDIO_CTRL_OUTPUT_TERMINAL) || (nau->nau_TermType != UAUTT_STREAMING)))
    {
        // use parent audio mode for flow down
        nau->nau_RootUnit = nau->nau_InputUnit[0]->nau_RootUnit;
    }
    for(cnt = 0; cnt < nau->nau_NumOutputs; cnt++)
    {
        nFlowDown(nch, nau->nau_OutputUnit[cnt]);
    }
}
/* \\\ */

/* /// "nFlowDownToUSBSink()" */
struct NepAudioUnit * nFlowDownToUSBSink(struct NepClassAudio *nch, struct NepAudioUnit *nau)
{
    UWORD cnt;
    struct NepAudioUnit *resnau = NULL;
    KPRINTF(10, ("FlowDownToUSBSink %02lx\n", nau->nau_UnitID));

    if((nau->nau_Type == UDST_AUDIO_CTRL_OUTPUT_TERMINAL) && (nau->nau_TermType == UAUTT_STREAMING))
    {
        KPRINTF(10, ("USB Outterm found\n"));
        nau->nau_SinkUnit = nau;
        return nau;
    }
    for(cnt = 0; cnt < nau->nau_NumOutputs; cnt++)
    {
        nau->nau_SinkUnit = nFlowDownToUSBSink(nch, nau->nau_OutputUnit[cnt]);
        if((resnau = nau->nau_SinkUnit))
        {
            break;
        }
    }
    return(resnau);
}
/* \\\ */

/* /// "nFlowUpToUSBSource()" */
struct NepAudioUnit * nFlowUpToUSBSource(struct NepClassAudio *nch, struct NepAudioUnit *nau)
{
    UWORD cnt;
    struct NepAudioUnit *resnau = NULL;
    KPRINTF(10, ("FlowsDownToUSB %02lx\n", nau->nau_UnitID));

    if((nau->nau_Type == UDST_AUDIO_CTRL_INPUT_TERMINAL) && (nau->nau_TermType == UAUTT_STREAMING))
    {
        KPRINTF(10, ("USB interm found\n"));
        nau->nau_SourceUnit = nau;
        return nau;
    }
    for(cnt = 0; cnt < nau->nau_NumInputs; cnt++)
    {
        nau->nau_SourceUnit = nFlowUpToUSBSource(nch, nau->nau_InputUnit[cnt]);
        if((resnau = nau->nau_SourceUnit))
        {
            break;
        }
    }
    return(resnau);
}
/* \\\ */

/* /// "nFlowUp()" */
void nFlowUp(struct NepClassAudio *nch, struct NepAudioUnit *nau)
{
    UWORD cnt;
    UBYTE *descdata = nau->nau_Descriptor;

    KPRINTF(10, ("FlowUp %02lx\n", nau->nau_UnitID));

    if(nau->nau_Name)
    {
        return;
    }

    // use downlink audio mode for flow up
    for(cnt = 0; cnt < nau->nau_NumInputs; cnt++)
    {
        if(nau->nau_RootUnit)
        {
            if(!nau->nau_InputUnit[cnt]->nau_RootUnit)
            {
                nau->nau_InputUnit[cnt]->nau_RootUnit = nau->nau_RootUnit;
            }
        }
        nFlowUp(nch, nau->nau_InputUnit[cnt]);
    }

    switch(nau->nau_Type)
    {

        case UDST_AUDIO_CTRL_INPUT_TERMINAL:
        {
            struct UsbAudioInputTermDesc10 *uait = (struct UsbAudioInputTermDesc10 *) descdata;
            ULONG termtype = AROS_WORD2LE(uait->wTerminalType);
            UWORD channelcfg = AROS_WORD2LE(uait->wChannelConfig);
            STRPTR spacial = nConcatBitsStr(nch, NTS_SPATIALLOCATION, (ULONG) channelcfg);
            nau->nau_Name = psdCopyStrFmt("%s (%s)", nNumToStr(nch, NTS_TERMINALTYPE, termtype, "Unknown"), spacial);
            psdFreeVec(spacial);
            break;
        }

        case UDST_AUDIO_CTRL_OUTPUT_TERMINAL:
            nau->nau_Name = psdCopyStrFmt("%s-Output from %s",
                                          nNumToStr(nch, NTS_TERMINALTYPE, (ULONG) nau->nau_TermType, "Unknown"),
                                          nau->nau_InputUnit[0]->nau_Name);
            nau->nau_OutChannels = nau->nau_InputUnit[0]->nau_OutChannels;
            nau->nau_ChannelCfg = nau->nau_InputUnit[0]->nau_ChannelCfg;
            break;

        case UDST_AUDIO_CTRL_FEATURE_UNIT:
        {
            UWORD features = 0;
            UWORD featsize = descdata[5];
            UWORD feat;
            UBYTE *featptr = descdata + 6;
            UWORD featsizecnt;
            STRPTR featstr;

            nau->nau_TermType = nau->nau_InputUnit[0]->nau_TermType;
            nau->nau_OutChannels = nau->nau_InputUnit[0]->nau_OutChannels;
            nau->nau_ChannelCfg = nau->nau_InputUnit[0]->nau_ChannelCfg;

            for(cnt = 0; cnt <= nau->nau_InputUnit[0]->nau_OutChannels; cnt++)
            {
                feat = 0;
                for(featsizecnt = 0; featsizecnt < featsize; featsizecnt++)
                {
                    feat |= (*featptr++)<<(featsizecnt<<3);
                }
                KPRINTF(10, ("Feature Unit %02lx, Channel %ld: %04lx\n", nau->nau_UnitID, cnt, feat));
                features |= feat;
            }
            featstr = nConcatBitsStr(nch, NTS_FEATURE, (ULONG) features);
            nau->nau_Name = psdCopyStrFmt("%s for %s", featstr, nau->nau_InputUnit[0]->nau_Name);
            psdFreeVec(featstr);
            break;
        }

        case UDST_AUDIO_CTRL_SELECTOR_UNIT:
            for(cnt = 0; cnt < nau->nau_NumInputs; cnt++)
            {
                UWORD taroutch = nau->nau_InputUnit[cnt]->nau_OutChannels;
                if(cnt)
                {
                    if(nau->nau_OutChannels != taroutch)
                    {
                        psdAddErrorMsg(RETURN_WARN, (STRPTR) libname, "Selector channels %ld != %ld conflict.",
                                       nau->nau_OutChannels, taroutch);
                    }
                }
                nau->nau_OutChannels = taroutch;
                nau->nau_ChannelCfg = nau->nau_InputUnit[cnt]->nau_ChannelCfg;
            }
            // fall through

        case UDST_AUDIO_CTRL_MIXER_UNIT:
        case UDST_AUDIO_CTRL_PROCESSING_UNIT:
        case UDST_AUDIO_CTRL_EXTENSION_UNIT:
        {
            UBYTE buf[64];
            UBYTE *bufptr = buf;
            ULONG buflen = 63;
            UBYTE *srcptr;

            if(nau->nau_NumInputs == 1)
            {
                // not ambigious, so lets copy the input.
                nau->nau_TermType = nau->nau_InputUnit[0]->nau_TermType;
            }
            for(cnt = 0; buflen && (cnt < nau->nau_NumInputs); cnt++)
            {
                if(cnt && buflen)
                {
                    if(nau->nau_Type == UDST_AUDIO_CTRL_SELECTOR_UNIT)
                    {
                        *bufptr++ = '/';
                    } else {
                        *bufptr++ = '+';
                    }
                    if(!(--buflen))
                    {
                        break;
                    }
                }
                /*if(nau->nau_InputUnit[cnt]->nau_TermType == UAUTT_STREAMING)
                {
                    srcptr = "Loopback";
                } else {*/
                    srcptr = nNumToStr(nch, NTS_TERMINALTYPE, (ULONG) nau->nau_InputUnit[cnt]->nau_TermType, "Unknown");
                //}
                while((*bufptr++ = *srcptr++))
                {
                    if(!(--buflen))
                    {
                        break;
                    }
                }
                bufptr--;
            }
            *bufptr = 0;
            nau->nau_Name = psdCopyStrFmt("%s (%s)", nNumToStr(nch, NTS_UNITTYPE, (ULONG) nau->nau_Type, "Unknown"), buf);
            break;
        }
    }
}
/* \\\ */

/* /// "nExamineAudioDescriptors()" */
void nExamineAudioDescriptors(struct NepClassAudio *nch)
{
    APTR desc;
    UBYTE *descdata;
    IPTR ifnum;
    struct NepAudioMode *nam;
    struct PsdInterface *streampif;
    APTR subdesc;
    UWORD cnt;
    struct UsbAudioHeaderDesc10 *header;
    struct UsbAudioGeneralIFDesc *uagi;
    struct UsbAudioType1FormatDesc *uat1f;
    struct UsbAudioGeneralEPDesc *uage;
    ULONG audioformat;
    ULONG ahisamtype = 0;
    BOOL typegood;
    BOOL freqctrl = FALSE;
    BOOL pitchctrl = FALSE;
    ULONG ahimodeid = AHI_USB_MODE_BASE + (nch->nch_UnitNo<<12);

    KPRINTF(10, ("ExamineAudioDesc Stage 1\n"));

    desc = psdFindDescriptor(nch->nch_Device, NULL,
                                   DDA_Interface, nch->nch_Interface,
                                   DDA_DescriptorType, UDT_CS_INTERFACE,
                                   DDA_CS_SubType, UDST_AUDIO_CTRL_HEADER,
                                   TAG_END);
    if(!desc)
    {
        return;
    }
    psdGetAttrs(PGA_DESCRIPTOR, desc,
                DDA_DescriptorData, &descdata,
                TAG_END);
    header = (struct UsbAudioHeaderDesc10 *) descdata;

    for(cnt = 0; cnt < header->bInCollection; cnt++)
    {
        streampif = NULL;
        while((streampif = psdFindInterface(nch->nch_Device, streampif,
                                            IFA_InterfaceNum, header->baInterfaceNr[cnt],
                                            IFA_AlternateNum, 0xffffffff,
                                            IFA_Class, AUDIO_CLASSCODE,
                                            IFA_SubClass, AUDIO_STREAM_SUBCLASS,
                                            TAG_END)))
        {
            psdGetAttrs(PGA_INTERFACE, streampif,
                        IFA_InterfaceNum, &ifnum,
                        TAG_END);
            subdesc = psdFindDescriptor(nch->nch_Device, NULL,
                                        DDA_Interface, streampif,
                                        DDA_DescriptorType, UDT_CS_INTERFACE,
                                        DDA_CS_SubType, UDST_AUDIO_STREAM_GENERAL,
                                        TAG_END);
            if(subdesc)
            {
                psdGetAttrs(PGA_DESCRIPTOR, subdesc,
                            DDA_DescriptorData, &uagi,
                            TAG_END);
                audioformat = uagi->wFormatTag0|(uagi->wFormatTag1<<8);
                psdAddErrorMsg(RETURN_OK, (STRPTR) libname, "Terminal %02lx (%ld delay) of Format %s",
                               uagi->bTerminalLink,
                               uagi->bDelay,
                               nNumToStr(nch, NTS_AUDIOFORMAT, audioformat, "Unknown"));
                if(audioformat == UAADF_PCM)
                {
                    subdesc = psdFindDescriptor(nch->nch_Device, NULL,
                                                DDA_Interface, streampif,
                                                DDA_DescriptorType, UDT_CS_INTERFACE,
                                                DDA_CS_SubType, UDST_AUDIO_STREAM_FMT_TYPE,
                                                TAG_END);
                    if(subdesc)
                    {
                        psdGetAttrs(PGA_DESCRIPTOR, subdesc,
                                    DDA_DescriptorData, &uat1f,
                                    TAG_END);

                        subdesc = psdFindDescriptor(nch->nch_Device, NULL,
                                                    DDA_Interface, streampif,
                                                    DDA_DescriptorType, UDT_CS_ENDPOINT,
                                                    TAG_END);
                        pitchctrl = freqctrl = FALSE;
                        if(subdesc)
                        {
                            psdGetAttrs(PGA_DESCRIPTOR, subdesc,
                                        DDA_DescriptorData, &uage,
                                        TAG_END);

                            if(uage->bmAttributes & UAECS_SAMPLE_FREQ) // actually, should check the bit, but the control selector has the right value by incident ;)
                            {
                                freqctrl = TRUE;
                                /*psdAddErrorMsg(RETURN_OK, (STRPTR) libname, "Terminal %02lx has frequency control.",
                                               uagi->bTerminalLink);*/
                            }
                            if(uage->bmAttributes & UAECS_PITCH)  // actually, should check the bit, but the control selector has the right value by incident ;)
                            {
                                pitchctrl = TRUE;
                                /*psdAddErrorMsg(RETURN_OK, (STRPTR) libname, "Terminal %02lx has pitch control.",
                                               uagi->bTerminalLink);*/
                            }
                        }

                        typegood = TRUE;
                        if(uat1f->bNrChannels == 1)
                        {
                            if(uat1f->bBitResolution == 8)
                            {
                                ahisamtype = AHIST_M8S;
                            }
                            else if(uat1f->bBitResolution == 16)
                            {
                                ahisamtype = AHIST_M16S;
                            }
                            else if((uat1f->bBitResolution > 16) && (uat1f->bBitResolution <= 24))
                            {
                                ahisamtype = 0; // no AHI representation of this mode
                            }
                            else if((uat1f->bBitResolution > 24) && (uat1f->bBitResolution <= 32))
                            {
                                ahisamtype = AHIST_M32S;
                            } else {
                                typegood = FALSE;
                            }
                        }
                        else if(uat1f->bNrChannels == 2)
                        {
                            if(uat1f->bBitResolution == 8)
                            {
                                ahisamtype = AHIST_S8S;
                            }
                            else if(uat1f->bBitResolution == 16)
                            {
                                ahisamtype = AHIST_S16S;
                            }
                            else if((uat1f->bBitResolution > 16) && (uat1f->bBitResolution <= 24))
                            {
                                ahisamtype = 0; // no AHI representation of this mode
                            }
                            else if((uat1f->bBitResolution > 24) && (uat1f->bBitResolution <= 32))
                            {
                                ahisamtype = AHIST_S32S;
                            } else {
                                typegood = FALSE;
                            }
                        } else {
                            typegood = FALSE;
                        }
                        if(!typegood)
                        {
                            psdAddErrorMsg(RETURN_WARN, (STRPTR) libname, "Unsupported mode '%s' with %ld bits x %ld channels ignored.",
                                           nNumToStr(nch, NTS_AUDIOFORMAT, audioformat, "Unknown"),
                                           uat1f->bBitResolution,
                                           uat1f->bNrChannels);
                        } else {
                            if((nam = AllocVec(sizeof(struct NepAudioMode), MEMF_CLEAR|MEMF_PUBLIC)))
                            {
                                nam->nam_SubLibBase = nch->nch_ClsBase->nh_SubLibBase;
                                nam->nam_Unit = nch;
                                nam->nam_Interface = streampif;
                                nam->nam_ZeroBWIF = psdFindInterface(nch->nch_Device, NULL,
                                                                     IFA_InterfaceNum, ifnum,
                                                                     IFA_AlternateNum, 0xffffffff,
                                                                     IFA_Class, AUDIO_CLASSCODE,
                                                                     IFA_SubClass, AUDIO_STREAM_SUBCLASS,
                                                                     IFA_NumEndpoints, 0,
                                                                     TAG_END);
                                if(!nam->nam_ZeroBWIF)
                                {
                                    psdAddErrorMsg(RETURN_WARN, (STRPTR) libname, "Zero Bandwidth endpoint not found!");
                                }

                                nam->nam_IfNum = ifnum;
                                nam->nam_SampleType = ahisamtype;
                                nam->nam_NumChannels = uat1f->bNrChannels;
                                nam->nam_FrameSize = uat1f->bSubframeSize * nam->nam_NumChannels;
                                nam->nam_SampleSize = uat1f->bSubframeSize;
                                nam->nam_Resolution = uat1f->bBitResolution;
                                nam->nam_TerminalID = uagi->bTerminalLink;
                                nam->nam_HasFreqCtrl = freqctrl;
                                nam->nam_HasPitchCtrl = pitchctrl;

                                AddTail(&nch->nch_AudioModes, &nam->nam_Node);
                                if(uat1f->bSamFreqType)
                                {
                                    UWORD cnt;
                                    UWORD cnt2;
                                    ULONG freq;
                                    UBYTE *fptr = uat1f->tSamFreq0;
                                    nam->nam_MinFreq = 100000;
                                    nam->nam_MaxFreq = 1;
                                    nam->nam_NumFrequencies = (uat1f->bSamFreqType < 64) ? uat1f->bSamFreqType : 64;
                                    for(cnt = 0; cnt < nam->nam_NumFrequencies; cnt++)
                                    {
                                        freq = fptr[0]|(fptr[1]<<8)|(fptr[2]<<16);
                                        if(freq < nam->nam_MinFreq)
                                        {
                                            nam->nam_MinFreq = freq;
                                        }
                                        if(freq > 64000)
                                        {
                                            psdAddErrorMsg(RETURN_WARN, (STRPTR) libname, "AHI does not support %ld Hz as it is above 65 KHz, sorry.", freq);
                                            nam->nam_NumFrequencies--;
                                            cnt--;
                                            fptr += 3;
                                            continue;
                                        }
                                        if(freq > nam->nam_MaxFreq)
                                        {
                                            nam->nam_MaxFreq = freq;
                                        }
                                        nam->nam_FreqArray[cnt] = freq;
                                        fptr += 3;
                                    }
                                    /* sort array (yeah, lazy bubble sort) */
                                    for(cnt = 0; cnt < nam->nam_NumFrequencies; cnt++)
                                    {
                                        for(cnt2 = cnt + 1; cnt2 < nam->nam_NumFrequencies; cnt2++)
                                        {
                                            if(nam->nam_FreqArray[cnt] > nam->nam_FreqArray[cnt2])
                                            {
                                                freq = nam->nam_FreqArray[cnt2];
                                                nam->nam_FreqArray[cnt2] = nam->nam_FreqArray[cnt];
                                                nam->nam_FreqArray[cnt] = freq;
                                            }
                                        }
                                    }
                                } else {
                                    const ULONG *freqtab = commonFreqs;
                                    nam->nam_MinFreq = uat1f->tSamFreq0[0]|(uat1f->tSamFreq0[1]<<8)|(uat1f->tSamFreq0[2]<<16);
                                    nam->nam_MaxFreq = uat1f->tSamFreq0[3]|(uat1f->tSamFreq0[4]<<8)|(uat1f->tSamFreq0[5]<<16);
                                    if(nam->nam_MaxFreq > 64000)
                                    {
                                        nam->nam_MaxFreq = 64000;
                                        psdAddErrorMsg(RETURN_WARN, (STRPTR) libname, "AHI does not support frequencies above 65 KHz, sorry.");
                                    }
                                    nam->nam_NumFrequencies = 0;
                                    while(*freqtab && (nam->nam_NumFrequencies < 64))
                                    {
                                        if((*freqtab >= nam->nam_MinFreq) && (*freqtab <= nam->nam_MaxFreq))
                                        {
                                            nam->nam_FreqArray[nam->nam_NumFrequencies++] = *freqtab;
                                        }
                                        freqtab++;
                                    }
                                }
                                nam->nam_AHIModeID = ++ahimodeid;
                                psdAddErrorMsg(RETURN_OK, (STRPTR) libname, "Found mode with %ld channel(s) (%ld byte(s) per frame) and %ld bits",
                                               uat1f->bNrChannels,
                                               uat1f->bSubframeSize,
                                               uat1f->bBitResolution);

                            }
                        }

                        /*if(!uat1f->bSamFreqType)
                        {
                            ULONG lowfreq = uat1f->tSamFreq0[0]|(uat1f->tSamFreq0[1]<<8)|(uat1f->tSamFreq0[2]<<16);
                            ULONG highfreq = uat1f->tSamFreq0[3]|(uat1f->tSamFreq0[4]<<8)|(uat1f->tSamFreq0[5]<<16);
                            psdAddErrorMsg(RETURN_OK, (STRPTR) libname, "Continuous frequency from %ld Hz to %ld Hz",
                                           lowfreq, highfreq);
                        } else {
                            ULONG freq = uat1f->tSamFreq0[0]|(uat1f->tSamFreq0[1]<<8)|(uat1f->tSamFreq0[2]<<16);
                            psdAddErrorMsg(RETURN_OK, (STRPTR) libname, "%ld frequencies supported (first one being %ld Hz)",
                                           uat1f->bSamFreqType, freq);

                        }*/
                    } else {
                        psdAddErrorMsg(RETURN_ERROR, (STRPTR) libname, "Interface %ld is missing type 1 format descriptor!", header->baInterfaceNr[cnt]);
                    }
                } else {
                    psdAddErrorMsg(RETURN_ERROR, (STRPTR) libname, "Terminal %02lx with Format '%s' is not supported!",
                                   uagi->bTerminalLink,
                                   nNumToStr(nch, NTS_AUDIOFORMAT, audioformat, "Unknown"));
                }
            }
        }
    }
}
/* \\\ */

/* /// "nFindLogVolume()" */
void nFindLogVolume(struct NepClassAudio *nch, struct NepAudioUnit *nau)
{
    /*WORD cnt;
    WORD *tab = (WORD *) volumeTableLog;
    if((nau->nau_MinVolDb < 0) && (nau->nau_MaxVolDb <= 0))
    {
        KPRINTF(10, ("negative DB range\n"));
        nau->nau_MinVolume = 0x10000;
        for(cnt = 0; cnt < 256; cnt++)
        {
            if(nau->nau_MinVolDb > tab[cnt])
            {
                break;
            }
            nau->nau_MinVolume = (255 - cnt)<<8;
        }
        KPRINTF(10, ("Minimum volume linearized: %04lx\n", nau->nau_MinVolume));
        for(cnt = 255; cnt >= 0; cnt--)
        {
            nau->nau_MaxVolume = (255 - cnt)<<8;
            if(nau->nau_MaxVolDb < tab[cnt])
            {
                break;
            }
        }
        KPRINTF(10, ("Maximum volume linearized: %04lx\n", nau->nau_MaxVolume));
    }
    else if((nau->nau_MinVolDb >= 0) && (nau->nau_MaxVolDb > 0))
    {
        KPRINTF(10, ("positive DB range\n"));
        nau->nau_MinVolume = 0x20000;
        for(cnt = 255; cnt >= 0; cnt--)
        {
            if(-nau->nau_MinVolDb < tab[cnt])
            {
                break;
            }
            nau->nau_MinVolume = (cnt+256)<<8;
        }
        KPRINTF(10, ("Minimum volume linearized: %04lx\n", nau->nau_MinVolume));
        for(cnt = 0; cnt < 256; cnt++)
        {
            nau->nau_MaxVolume = (cnt+256)<<8;
            if(-nau->nau_MaxVolDb > tab[cnt])
            {
                break;
            }
        }
        KPRINTF(10, ("Maximum volume linearized: %04lx\n", nau->nau_MaxVolume));
    } else {
        KPRINTF(10, ("unsupported DB range\n"));
    }*/
    nau->nau_MinVolume = 0;//(((Fixed) nau->nau_MinVolDb) + 0x8000)<<1;
    nau->nau_MaxVolume = 0x10000;//(((Fixed) nau->nau_MaxVolDb) + 0x8000)<<1;
}
/* \\\ */

/* /// "nExamineAudioUnits()" */
void nExamineAudioUnits(struct NepClassAudio *nch)
{
    APTR desc;
    UBYTE *descdata;
    IPTR subtype;
    struct NepAudioMode *nam;
    struct NepAudioMode *sibnam;
    struct NepAudioUnit *nau;
    KPRINTF(10, ("ExamineAudioDesc Stage 2\n"));

    desc = NULL;
    while((desc = psdFindDescriptor(nch->nch_Device, desc,
                                   DDA_Interface, nch->nch_Interface,
                                   DDA_DescriptorType, UDT_CS_INTERFACE,
                                   TAG_END)))
    {
        psdGetAttrs(PGA_DESCRIPTOR, desc,
                    DDA_DescriptorData, &descdata,
                    DDA_CS_SubType, &subtype,
                    TAG_END);
        switch(subtype)
        {
            case UDST_AUDIO_CTRL_INPUT_TERMINAL:
            case UDST_AUDIO_CTRL_OUTPUT_TERMINAL:
            case UDST_AUDIO_CTRL_MIXER_UNIT:
            case UDST_AUDIO_CTRL_SELECTOR_UNIT:
            case UDST_AUDIO_CTRL_FEATURE_UNIT:
            case UDST_AUDIO_CTRL_PROCESSING_UNIT:
            case UDST_AUDIO_CTRL_EXTENSION_UNIT:
                nau = psdAllocVec(sizeof(struct NepAudioUnit));
                if(!nau)
                {
                    break;
                }
                nau->nau_Type = subtype;
                nau->nau_UnitID = descdata[3];
                nau->nau_Descriptor = descdata;
                AddTail(&nch->nch_AudioUnits, &nau->nau_Node);
                break;
        }
    }

    // connect units together
    nau = (struct NepAudioUnit *) nch->nch_AudioUnits.lh_Head;
    while(nau->nau_Node.ln_Succ)
    {
        descdata = nau->nau_Descriptor;
        switch(nau->nau_Type)
        {
            case UDST_AUDIO_CTRL_OUTPUT_TERMINAL:
                nFindAndConnectAudioUnit(nch, nau, (ULONG) ((struct UsbAudioOutputTermDesc10 *) descdata)->bSourceID);
                break;

            case UDST_AUDIO_CTRL_MIXER_UNIT:
            case UDST_AUDIO_CTRL_SELECTOR_UNIT:
            {
                struct UsbAudioMixerUnitDesc10 *uamu = (struct UsbAudioMixerUnitDesc10 *) descdata;
                UWORD inputpins = uamu->bNrInPins;
                UWORD cnt;

                if(inputpins > 8)
                {
                    psdAddErrorMsg(RETURN_WARN, (STRPTR) libname, "On Mixer / Selector Unit %02lx with %ld input pins only 8 are supported.",
                                   uamu->bUnitID,
                                   inputpins);
                    inputpins = 8;
                }

                for(cnt = 0; cnt < inputpins; cnt++)
                {
                    nFindAndConnectAudioUnit(nch, nau, (ULONG) descdata[5+cnt]);
                }
                break;
            }

            case UDST_AUDIO_CTRL_FEATURE_UNIT:
                nFindAndConnectAudioUnit(nch, nau, (ULONG) ((struct UsbAudioFeatureUnitDesc10 *) descdata)->bSourceID);
                break;

            case UDST_AUDIO_CTRL_PROCESSING_UNIT:
            case UDST_AUDIO_CTRL_EXTENSION_UNIT:
            {
                struct UsbAudioProcessingUnitDesc10 *uapu = (struct UsbAudioProcessingUnitDesc10 *) descdata;
                UWORD inputpins = uapu->bNrInPins;
                UWORD cnt;

                if(inputpins > 8)
                {
                    psdAddErrorMsg(RETURN_WARN, (STRPTR) libname, "On Processing / Extension Unit %02lx with %ld input pins only 8 are supported.",
                                   uapu->bUnitID,
                                   inputpins);
                    inputpins = 8;
                }

                for(cnt = 0; cnt < inputpins; cnt++)
                {
                    nFindAndConnectAudioUnit(nch, nau, (ULONG) descdata[7+cnt]);
                }
                break;
            }

        }
        nau = (struct NepAudioUnit *) nau->nau_Node.ln_Succ;
    }

    KPRINTF(10, ("ExamineAudioDesc Stage 3\n"));

    // obtain output channels and configs where known (the rest is done on flow recursion next step)
    nau = (struct NepAudioUnit *) nch->nch_AudioUnits.lh_Head;
    while(nau->nau_Node.ln_Succ)
    {
        descdata = nau->nau_Descriptor;
        switch(nau->nau_Type)
        {
            case UDST_AUDIO_CTRL_OUTPUT_TERMINAL:
            {
                struct UsbAudioOutputTermDesc10 *uaot = (struct UsbAudioOutputTermDesc10 *) descdata;
                ULONG termtype = AROS_WORD2LE(uaot->wTerminalType);
                nau->nau_TermType = termtype;
                if(termtype == UAUTT_STREAMING)
                {
                    nau->nau_RootUnit = nau;
                    // try to locate audio mode, which is obviously an input
                    nam = (struct NepAudioMode *) nch->nch_AudioModes.lh_Head;
                    while(nam->nam_Node.ln_Succ)
                    {
                        if(nam->nam_TerminalID == uaot->bTerminalID)
                        {
                            nam->nam_IsInput = TRUE;
                            nam->nam_RootUnit = nau;
                            //psdAddErrorMsg(RETURN_OK, (STRPTR) libname, "Terminal %02lx assigned to IN audio mode!", nam->nam_TerminalID);
                        }
                        nam = (struct NepAudioMode *) nam->nam_Node.ln_Succ;
                    }
                }
                break;
            }

            case UDST_AUDIO_CTRL_INPUT_TERMINAL:
            {
                struct UsbAudioInputTermDesc10 *uait = (struct UsbAudioInputTermDesc10 *) descdata;
                UWORD channelcfg = AROS_WORD2LE(uait->wChannelConfig);
                ULONG termtype = AROS_WORD2LE(uait->wTerminalType);

                nau->nau_OutChannels = uait->bNrChannels;
                nau->nau_ChannelCfg = channelcfg;
                nau->nau_TermType = termtype;
                nau->nau_RootUnit = nau;
                if(termtype == UAUTT_STREAMING)
                {
                    // try to locate audio mode, which is obviously an output
                    nam = (struct NepAudioMode *) nch->nch_AudioModes.lh_Head;
                    while(nam->nam_Node.ln_Succ)
                    {
                        if(nam->nam_TerminalID == uait->bTerminalID)
                        {
                            nam->nam_IsInput = FALSE;
                            nam->nam_RootUnit = nau;
                            //psdAddErrorMsg(RETURN_OK, (STRPTR) libname, "Terminal %02lx assigned to OUT audio mode!", nam->nam_TerminalID);
                        }
                        nam = (struct NepAudioMode *) nam->nam_Node.ln_Succ;
                    }
                }
                break;
            }

            case UDST_AUDIO_CTRL_MIXER_UNIT:
            {
                struct UsbAudioMixerUnitDesc10 *uamu = (struct UsbAudioMixerUnitDesc10 *) descdata;
                UWORD inputpins = uamu->bNrInPins;
                UWORD outchannels = descdata[5+inputpins];
                UWORD channelcfg = descdata[6+inputpins]|(descdata[7+inputpins]<<8);

                nau->nau_OutChannels = outchannels;
                nau->nau_ChannelCfg = channelcfg;
                break;
            }

            case UDST_AUDIO_CTRL_PROCESSING_UNIT:
            case UDST_AUDIO_CTRL_EXTENSION_UNIT:
            {
                struct UsbAudioProcessingUnitDesc10 *uapu = (struct UsbAudioProcessingUnitDesc10 *) descdata;
                UWORD inputpins = uapu->bNrInPins;
                UWORD outchannels = descdata[7+inputpins];
                UWORD channelcfg = descdata[8+inputpins]|(descdata[9+inputpins]<<8);

                nau->nau_OutChannels = outchannels;
                nau->nau_ChannelCfg = channelcfg;
                break;
            }
        }
        nau = (struct NepAudioUnit *) nau->nau_Node.ln_Succ;
    }

    KPRINTF(10, ("ExamineAudioDesc Stage 4\n"));

    // recurse over output terminals, flow up the chain
    nau = (struct NepAudioUnit *) nch->nch_AudioUnits.lh_Head;
    while(nau->nau_Node.ln_Succ)
    {
        descdata = nau->nau_Descriptor;
        if(nau->nau_Type == UDST_AUDIO_CTRL_OUTPUT_TERMINAL)
        {
            nFlowUp(nch, nau);
            nau->nau_SourceUnit = nFlowUpToUSBSource(nch, nau);
        }
        nau = (struct NepAudioUnit *) nau->nau_Node.ln_Succ;
    }

    // recurse over input terminals, flow down the chain
    nau = (struct NepAudioUnit *) nch->nch_AudioUnits.lh_Head;
    while(nau->nau_Node.ln_Succ)
    {
        descdata = nau->nau_Descriptor;
        if(nau->nau_Type == UDST_AUDIO_CTRL_INPUT_TERMINAL)
        {
            UWORD cnt;
            for(cnt = 0; cnt < nau->nau_NumOutputs; cnt++)
            {
                nFlowDown(nch, nau->nau_OutputUnit[cnt]);
            }
            nau->nau_SinkUnit = nFlowDownToUSBSink(nch, nau);
        }
        nau = (struct NepAudioUnit *) nau->nau_Node.ln_Succ;
    }

    KPRINTF(10, ("ExamineAudioDesc Stage 5\n"));

    // do the some more magic
    nau = (struct NepAudioUnit *) nch->nch_AudioUnits.lh_Head;
    while(nau->nau_Node.ln_Succ)
    {
        descdata = nau->nau_Descriptor;
        switch(nau->nau_Type)
        {
            case UDST_AUDIO_CTRL_MIXER_UNIT:
            {
                struct UsbAudioMixerUnitDesc10 *uamu = (struct UsbAudioMixerUnitDesc10 *) descdata;
                UWORD inputpins = uamu->bNrInPins;
                //UWORD outchannels = descdata[5+inputpins];
                UWORD channelcfg = descdata[6+inputpins]|(descdata[7+inputpins]<<8);
                STRPTR spacial = nConcatBitsStr(nch, NTS_SPATIALLOCATION, (ULONG) channelcfg);

                /*psdAddErrorMsg(RETURN_OK, (STRPTR) libname, "Mixer Unit %02lx with %ld input pins and %ld output channels (%s).",
                               uamu->bUnitID,
                               inputpins,
                               outchannels,
                               spacial);*/
                psdFreeVec(spacial);
                break;
            }

            case UDST_AUDIO_CTRL_SELECTOR_UNIT:
            {
                //struct UsbAudioSelectorUnitDesc10 *uasu = (struct UsbAudioSelectorUnitDesc10 *) descdata;
                //UWORD inputpins = uasu->bNrInPins;
                //UWORD cnt;

                /*psdAddErrorMsg(RETURN_OK, (STRPTR) libname, "Selector Unit %02lx with %ld input pins.",
                               uasu->bUnitID,
                               inputpins);*/

                if(nau->nau_SinkUnit)
                {
                    KPRINTF(10, ("Assigning Selector to Unit %02lx", nau->nau_SinkUnit->nau_UnitID));
                    nau->nau_SinkUnit->nau_SelectorUnit = nau;
                } else {
                    KPRINTF(10, ("Error: No Sink unit!\n"));
                }

                /*nam = (struct NepAudioMode *) nch->nch_AudioModes.lh_Head;
                while(nam->nam_Node.ln_Succ)
                {
                    if(nam->nam_TerminalID == nau->nau_RootUnit->nau_UnitID)
                    {
                        KPRINTF(10, ("Terminal ID %02lx matched\n", nam->nam_TerminalID));
                        if(nau->nau_RootUnit->nau_Type == UDST_AUDIO_CTRL_INPUT_TERMINAL)
                        {
                            for(cnt = 0; cnt < nau->nau_NumInputs; cnt++)
                            {
                                if(nam->nam_NumOutputs < 8)
                                {
                                    KPRINTF(10, ("Adding output selector %s\n", nNumToStr(nch, NTS_TERMINALTYPE, (ULONG) nau->nau_InputUnit[cnt]->nau_TermType, "Unknown")));
                                    nam->nam_OutputNames[nam->nam_NumOutputs++] = nNumToStr(nch, NTS_TERMINALTYPE, (ULONG) nau->nau_InputUnit[cnt]->nau_TermType, "Unknown");
                                }
                            }
                        } else {
                            for(cnt = 0; cnt < nau->nau_NumInputs; cnt++)
                            {
                                if(nam->nam_NumInputs < 8)
                                {
                                    KPRINTF(10, ("Adding input selector %s\n", nNumToStr(nch, NTS_TERMINALTYPE, (ULONG) nau->nau_InputUnit[cnt]->nau_TermType, "Unknown")));
                                    nam->nam_InputNames[nam->nam_NumInputs++] = nNumToStr(nch, NTS_TERMINALTYPE, (ULONG) nau->nau_InputUnit[cnt]->nau_TermType, "Unknown");
                                }
                            }
                        }
                    }
                    nam = (struct NepAudioMode *) nam->nam_Node.ln_Succ;
                }*/
                break;
            }

            case UDST_AUDIO_CTRL_FEATURE_UNIT:
            {
                UWORD featsize = descdata[5];
                UWORD feat;
                UWORD masterfeat = 0;
                UWORD leftfeat = 0;
                UWORD rightfeat = 0;
                UBYTE *featptr = descdata + 6;
                UWORD featsizecnt;
                UWORD cnt;
                UWORD firstchan = 0;

                nau->nau_TermType = nau->nau_InputUnit[0]->nau_TermType;
                nau->nau_OutChannels = nau->nau_InputUnit[0]->nau_OutChannels;
                nau->nau_ChannelCfg = nau->nau_InputUnit[0]->nau_ChannelCfg;

                KPRINTF(10, ("Feature Unit %ld, Root unit %ld\n", nau->nau_UnitID, nau->nau_RootUnit->nau_UnitID));

                for(cnt = 0; cnt <= nau->nau_InputUnit[0]->nau_OutChannels; cnt++)
                {
                    feat = 0;
                    for(featsizecnt = 0; featsizecnt < featsize; featsizecnt++)
                    {
                        feat |= (*featptr++)<<(featsizecnt<<3);
                    }
                    if(cnt == 0)
                    {
                        masterfeat = feat;
                    }
                    else if(cnt == 1)
                    {
                        leftfeat = feat;
                    }
                    else if(cnt == 2)
                    {
                        rightfeat = feat;
                    }
                }

                if((masterfeat & UAFUF_VOLUME) ||
                   ((nau->nau_OutChannels == 1) && (leftfeat & UAFUF_VOLUME)) ||
                   (leftfeat & rightfeat & UAFUF_VOLUME))
                {
                    KPRINTF(10, ("Master volume control supported\n"));
                    if(nau->nau_SourceUnit)
                    {
                        KPRINTF(10, ("Volume roots from USB input streaming, assuming Master Volume!\n"));
                        nau->nau_RootUnit->nau_VolumeControl = TRUE;
                        nau->nau_RootUnit->nau_VolumeUnit = nau;
                        if(leftfeat & rightfeat & UAFUF_VOLUME)
                        {
                            KPRINTF(10, ("Balance control supported\n"));
                            nau->nau_RootUnit->nau_Balance = TRUE;
                        }
                    }
                    else if(nau->nau_SinkUnit)
                    {
                        KPRINTF(10, ("Flows to USB stream, assuming input gain!\n"));
                        nau->nau_RootUnit->nau_VolumeControl = TRUE;
                        nau->nau_RootUnit->nau_VolumeUnit = nau;
                        if(leftfeat & rightfeat & UAFUF_VOLUME)
                        {
                            KPRINTF(10, ("Input Balance control supported\n"));
                            nau->nau_RootUnit->nau_Balance = TRUE;
                        }
                    } else {
                        KPRINTF(10, ("Flows to something else, assuming monitor!\n"));
                        nau->nau_RootUnit->nau_Monitor = TRUE;
                        nau->nau_RootUnit->nau_MonitorUnit = nau;
                    }
                }
                if(rightfeat & UAFUF_VOLUME)
                {
                    firstchan = 2;
                    nau->nau_VolCtrlMask |= 4;
                    nau->nau_VolCtrlCount++;
                }
                if(leftfeat & UAFUF_VOLUME)
                {
                    firstchan = 1;
                    nau->nau_VolCtrlMask |= 2;
                    nau->nau_VolCtrlCount++;
                }
                if((masterfeat & UAFUF_VOLUME))
                {
                    firstchan = 0;
                    nau->nau_VolCtrlMask |= 1;
                    nau->nau_VolCtrlCount++;
                }
                KPRINTF(10, ("Volume Feature Mask %04lx\n", nau->nau_VolCtrlMask));

                if(nau->nau_VolCtrlCount)
                {
                    UBYTE buf[2];
                    LONG ioerr;
                    psdPipeSetup(nch->nch_EP0Pipe, URTF_IN|URTF_CLASS|URTF_INTERFACE, UAUDR_GET_MIN, (ULONG) (UAFUCS_VOLUME<<8)|firstchan, (nau->nau_UnitID<<8)|nch->nch_IfNum);
                    ioerr = psdDoPipe(nch->nch_EP0Pipe, buf, 2);
                    if(ioerr)
                    {
                        psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                       "Could not get min volume: %s (%ld)!",
                                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                    } else {
                        KPRINTF(10, ("Minimum volume: %04lx\n", buf[0]|(buf[1]<<8)));
                        nau->nau_MinVolDb = (WORD) (buf[0]|(buf[1]<<8));
                    }
                    psdPipeSetup(nch->nch_EP0Pipe, URTF_IN|URTF_CLASS|URTF_INTERFACE, UAUDR_GET_MAX, (ULONG) (UAFUCS_VOLUME<<8)|firstchan, (nau->nau_UnitID<<8)|nch->nch_IfNum);
                    ioerr = psdDoPipe(nch->nch_EP0Pipe, buf, 2);
                    if(ioerr)
                    {
                        psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                                       "Could not get max volume: %s (%ld)!",
                                       psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
                    } else {
                        KPRINTF(10, ("Maximum volume: %04lx\n", buf[0]|(buf[1]<<8)));
                        nau->nau_MaxVolDb = (WORD) (buf[0]|(buf[1]<<8));
                    }
                    nFindLogVolume(nch, nau);
                }
                break;
            }
        }
        /*psdAddErrorMsg(RETURN_OK, (STRPTR) libname, "%s Unit (%s) ID=%02lx, In/Out=%ld/%ld, Chans=%ld, Cfg=%ld, TermType=%s, Root=%02lx, Source=%02lx, Sink=%02lx",
                       nNumToStr(nch, NTS_UNITTYPE, (ULONG) nau->nau_Type, "Unknown"),
                       nau->nau_Name,
                       nau->nau_UnitID,
                       nau->nau_NumInputs,
                       nau->nau_NumOutputs,
                       nau->nau_OutChannels,
                       nau->nau_ChannelCfg,
                       nNumToStr(nch, NTS_TERMINALTYPE, (ULONG) nau->nau_TermType, "Unknown"),
                       nau->nau_RootUnit->nau_UnitID,
                       nau->nau_SourceUnit ? nau->nau_SourceUnit->nau_UnitID : 0,
                       nau->nau_SinkUnit ? nau->nau_SinkUnit->nau_UnitID : 0);*/

        nau = (struct NepAudioUnit *) nau->nau_Node.ln_Succ;
    }

    // if no selectors were found, make sure we have names for at least one input source and output sink
    nam = (struct NepAudioMode *) nch->nch_AudioModes.lh_Head;
    while(nam->nam_Node.ln_Succ)
    {
        if(!nam->nam_NumOutputs)
        {
            nau = (struct NepAudioUnit *) nch->nch_AudioUnits.lh_Head;
            while(nau->nau_Node.ln_Succ)
            {
                if((nau->nau_Type == UDST_AUDIO_CTRL_OUTPUT_TERMINAL) && (nau->nau_TermType != UAUTT_STREAMING))
                {
                    nam->nam_OutputNames[nam->nam_NumOutputs++] = nNumToStr(nch, NTS_TERMINALTYPE, (ULONG) nau->nau_TermType, "Unknown");
                    //break;
                }
                nau = (struct NepAudioUnit *) nau->nau_Node.ln_Succ;
            }
        }
        if(!nam->nam_NumInputs)
        {
            nau = (struct NepAudioUnit *) nch->nch_AudioUnits.lh_Head;
            while(nau->nau_Node.ln_Succ)
            {
                if((nau->nau_Type == UDST_AUDIO_CTRL_INPUT_TERMINAL) && (nau->nau_TermType != UAUTT_STREAMING))
                {
                    nam->nam_InputNames[nam->nam_NumInputs++] = nNumToStr(nch, NTS_TERMINALTYPE, (ULONG) nau->nau_TermType, "Unknown");
                    //break;
                }
                nau = (struct NepAudioUnit *) nau->nau_Node.ln_Succ;
            }
        }
        nam = (struct NepAudioMode *) nam->nam_Node.ln_Succ;
    }

    // find recording modes and assign as sibling
    nam = (struct NepAudioMode *) nch->nch_AudioModes.lh_Head;
    while(nam->nam_Node.ln_Succ)
    {
        if(nam->nam_IsInput)
        {
            KPRINTF(10, ("Trying to find matching record mode for %ld channel(s) and %ld bits.\n", nam->nam_NumChannels, nam->nam_Resolution));
            sibnam = (struct NepAudioMode *) nch->nch_AudioModes.lh_Head;
            while(sibnam->nam_Node.ln_Succ)
            {
                if(!sibnam->nam_IsInput)
                {
                    if((sibnam->nam_Resolution == nam->nam_Resolution) &&
                       (sibnam->nam_NumChannels == nam->nam_NumChannels) &&
                       (sibnam->nam_FrameSize == nam->nam_FrameSize) &&
                       (sibnam->nam_NumFrequencies == nam->nam_NumFrequencies) &&
                       (sibnam->nam_MinFreq == nam->nam_MinFreq) &&
                       (sibnam->nam_MaxFreq == nam->nam_MaxFreq) &&
                       (sibnam->nam_FreqArray[0] == nam->nam_FreqArray[0]))
                    {
                        KPRINTF(10, ("Recording mode with %ld channel(s) and %ld bits matched.\n", nam->nam_NumChannels, nam->nam_Resolution));
                        if(sibnam->nam_Sibling)
                        {
                            psdAddErrorMsg(RETURN_WARN, (STRPTR) libname, "Matching playback mode with %ld channel(s) and %ld bits is already occupied.",
                                           sibnam->nam_NumChannels,
                                           sibnam->nam_Resolution);
                        } else {
                            nam->nam_Sibling = sibnam;
                            sibnam->nam_Sibling = nam;
                            break;
                        }
                    }
                }
                sibnam = (struct NepAudioMode *) sibnam->nam_Node.ln_Succ;
            }
            if(!sibnam->nam_Node.ln_Succ)
            {
                KPRINTF(10, ("Recording mode with %ld channel(s) and %ld bits could not be matched to an input mode.\n", nam->nam_NumChannels, nam->nam_Resolution));
                sibnam = (struct NepAudioMode *) nch->nch_AudioModes.lh_Head;
                while(sibnam->nam_Node.ln_Succ)
                {
                    if(!sibnam->nam_IsInput)
                    {
                        if((sibnam->nam_NumFrequencies == nam->nam_NumFrequencies) &&
                           (sibnam->nam_MinFreq == nam->nam_MinFreq) &&
                           (sibnam->nam_MaxFreq == nam->nam_MaxFreq) &&
                           (sibnam->nam_FreqArray[0] == nam->nam_FreqArray[0]))
                        {
                            KPRINTF(10, ("Recording mode partially matched.\n"));
                            if(sibnam->nam_Sibling)
                            {
                                psdAddErrorMsg(RETURN_WARN, (STRPTR) libname, "Partially matching playback mode with %ld channel(s) and %ld bits is already occupied.",
                                               sibnam->nam_NumChannels,
                                               sibnam->nam_Resolution);
                            } else {
                                nam->nam_Sibling = sibnam;
                                sibnam->nam_Sibling = nam;
                                break;
                            }
                        }
                    }
                    sibnam = (struct NepAudioMode *) sibnam->nam_Node.ln_Succ;
                }
                if(!sibnam->nam_Node.ln_Succ)
                {
                    psdAddErrorMsg(RETURN_WARN, (STRPTR) libname, "Recording mode with %ld channel(s) and %ld bits could not be matched to an output mode.",
                                   nam->nam_NumChannels,
                                   nam->nam_Resolution);
                } else {
                    psdAddErrorMsg(RETURN_WARN, (STRPTR) libname, "Recording mode with %ld ch@%ldb was assigned to %ld ch@%ldb output mode.",
                                   nam->nam_NumChannels,
                                   nam->nam_Resolution,
                                   sibnam->nam_NumChannels,
                                   sibnam->nam_Resolution);
                }
            }
        }
        nam = (struct NepAudioMode *) nam->nam_Node.ln_Succ;
    }

    KPRINTF(10, ("ExamineAudioUnits finished\n"));
}
/* \\\ */

/* /// "nAddAudioModes()" */
void nAddAudioModes(struct NepClassAudio *nch)
{
    struct NepAudioMode *nam;
    STRPTR devname;
    STRPTR ahimodename;

    psdGetAttrs(PGA_DEVICE, nch->nch_Device, DA_ProductName, &devname, TAG_END);
    nam = (struct NepAudioMode *) nch->nch_AudioModes.lh_Head;
    while(nam->nam_Node.ln_Succ)
    {
        if(!nam->nam_IsInput)
        {
            struct TagItem *tags = nam->nam_Tags;
            ahimodename = psdCopyStrFmt("%.20s: %s%ld bit %s (%ld bpf)",
                                        devname,
                                        (nam->nam_Resolution > 16) ? "HiFi " : "",
                                        nam->nam_Resolution,
                                        (nam->nam_NumChannels > 1) ? "stereo++" : "mono",
                                        nam->nam_FrameSize);

            KPRINTF(10, ("AHI Mode %s NumInput=%ld, NumOutput=%ld, Sib=%08lx\n",
                         ahimodename, nam->nam_NumInputs, nam->nam_NumOutputs, nam->nam_Sibling));

            // AHI samples are always 16 bit, except for HiFi mode, which returns 32 bit
            nam->nam_AHIFrameSize = nam->nam_NumChannels * ((nam->nam_Resolution > 16) ? 4 : 2);

            tags->ti_Tag = AHIDB_AudioID;
            tags->ti_Data = nam->nam_AHIModeID;
            tags++;
            tags->ti_Tag = AHIDB_Name;
            tags->ti_Data = (IPTR) ahimodename;
            tags++;
            tags->ti_Tag = AHIDB_Driver;
            tags->ti_Data = (IPTR) "usbaudio";
            tags++;
            /*tags->ti_Tag = AHIDB_DriverBaseName;
            tags->ti_Data = (ULONG) "";
            tags++;*/
            tags->ti_Tag = AHIDB_Volume;
            tags->ti_Data = nam->nam_RootUnit->nau_VolumeControl;
            tags++;
            tags->ti_Tag = AHIDB_Panning;
            tags->ti_Data = (nam->nam_NumChannels > 1) ? TRUE : FALSE;
            tags++;
            tags->ti_Tag = AHIDB_Stereo;
            tags->ti_Data = (nam->nam_NumChannels > 1) ? TRUE : FALSE;
            tags++;
            tags->ti_Tag = AHIDB_HiFi;
            tags->ti_Data = (nam->nam_Resolution > 16) ? TRUE : FALSE;
            tags++;
            tags->ti_Tag = AHIDB_MultTable;
            tags->ti_Data = FALSE;
            tags++;
            tags->ti_Tag = AHIDB_NepAudioMode;
            tags->ti_Data = (IPTR) nam;
            tags++;
            tags->ti_Tag = TAG_END;
            AHI_AddAudioMode(nam->nam_Tags);
            psdFreeVec(ahimodename);
        }
        nam = (struct NepAudioMode *) nam->nam_Node.ln_Succ;
    }
}
/* \\\ */

/* /// "nAllocAudio()" */
struct NepClassAudio * nAllocAudio(void)
{
    struct Task *thistask;
    struct NepClassAudio *nch;

    thistask = FindTask(NULL);
    nch = thistask->tc_UserData;
    do
    {
        if(!(nch->nch_Base = OpenLibrary("poseidon.library", 4)))
        {
            Alert(AG_OpenLib);
            break;
        }

        if(!(nch->nch_AHIMsgPort = CreateMsgPort()))
        {
            break;
        }

        if(!(nch->nch_AHIReq = (struct AHIRequest *) CreateIORequest(nch->nch_AHIMsgPort, sizeof(struct AHIRequest))))
        {
            break;
        }

        //if(OpenDevice("ahi.device", AHI_NO_UNIT, (struct IORequest *) nch->nch_AHIReq, AHIDF_NOMODESCAN))
        if(OpenDevice("ahi.device", AHI_NO_UNIT, (struct IORequest *) nch->nch_AHIReq, 0))
        {
            psdAddErrorMsg(RETURN_FAIL, (STRPTR) libname, "Unable to open AHI device");
            break;
        }

        AHIBase = (struct Library *) nch->nch_AHIReq->ahir_Std.io_Device;

        psdGetAttrs(PGA_INTERFACE, nch->nch_Interface,
                    IFA_Config, &nch->nch_Config,
                    IFA_InterfaceNum, &nch->nch_IfNum,
                    TAG_END);
        psdGetAttrs(PGA_CONFIG, nch->nch_Config,
                    CA_Device, &nch->nch_Device,
                    TAG_END);

        if(!nch->nch_AudioModes.lh_Head->ln_Succ)
        {
            nExamineAudioDescriptors(nch);
        }
        if(nch->nch_AudioModes.lh_Head->ln_Succ)
        {
            if((nch->nch_TaskMsgPort = CreateMsgPort()))
            {
                if((nch->nch_EP0Pipe = psdAllocPipe(nch->nch_Device, nch->nch_TaskMsgPort, NULL)))
                {
                    nExamineAudioUnits(nch);
                    nAddAudioModes(nch);

                    nch->nch_Task = thistask;
                    return(nch);
                }
                DeleteMsgPort(nch->nch_TaskMsgPort);
                nch->nch_TaskMsgPort = NULL;
            }
        } else {
            psdAddErrorMsg(RETURN_FAIL, (STRPTR) libname, "No suitable audio modes found for this audio device. Very sorry, dude.");
        }
    } while(FALSE);
    if(AHIBase)
    {
        CloseDevice((struct IORequest *) nch->nch_AHIReq);
        AHIBase = NULL;
    }
    DeleteIORequest((struct IORequest *) nch->nch_AHIReq);
    nch->nch_AHIReq = NULL;
    DeleteMsgPort(nch->nch_AHIMsgPort);
    nch->nch_AHIMsgPort = NULL;
    CloseLibrary(nch->nch_Base);
    nch->nch_Base = NULL;
    Forbid();
    nch->nch_Task = NULL;
    if(nch->nch_ReadySigTask)
    {
        Signal(nch->nch_ReadySigTask, 1L<<nch->nch_ReadySignal);
    }
    return(NULL);
}
/* \\\ */

/* /// "nFreeAudio()" */
void nFreeAudio(struct NepClassAudio *nch)
{
    struct NepAudioMode *nam;
    struct NepAudioUnit *nau;

    nam = (struct NepAudioMode *) nch->nch_AudioModes.lh_Head;
    while(nam->nam_Node.ln_Succ)
    {
        if(nam->nam_AHIModeID)
        {
            AHI_RemoveAudioMode(nam->nam_AHIModeID);
        }
        nam = (struct NepAudioMode *) nam->nam_Node.ln_Succ;
    }
    nau = (struct NepAudioUnit *) nch->nch_AudioUnits.lh_Head;
    while(nau->nau_Node.ln_Succ)
    {
        Remove((struct Node *) nau);
        psdFreeVec(nau->nau_Name);
        psdFreeVec(nau);
        nau = (struct NepAudioUnit *) nch->nch_AudioUnits.lh_Head;
    }
    psdFreePipe(nch->nch_EP0Pipe);
    DeleteMsgPort(nch->nch_TaskMsgPort);
    nch->nch_TaskMsgPort = NULL;

    CloseDevice((struct IORequest *) nch->nch_AHIReq);
    AHIBase = NULL;
    DeleteIORequest((struct IORequest *) nch->nch_AHIReq);
    nch->nch_AHIReq = NULL;
    DeleteMsgPort(nch->nch_AHIMsgPort);
    nch->nch_AHIMsgPort = NULL;
    CloseLibrary(nch->nch_Base);
    nch->nch_Base = NULL;
    Forbid();
    nch->nch_Task = NULL;
    if(nch->nch_ReadySigTask)
    {
        Signal(nch->nch_ReadySigTask, 1L<<nch->nch_ReadySignal);
    }
}
/* \\\ */

/**************************************************************************/

#undef  ps

/* /// "nLoadClassConfig()" */
BOOL nLoadClassConfig(struct NepAudioBase *nh)
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
LONG nOpenCfgWindow(struct NepAudioBase *nh)
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
    struct NepAudioBase *nh;
    APTR pic;

    thistask = FindTask(NULL);

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
        MUIA_Application_Copyright  , (IPTR)"©2008-2009 Chris Hodges",
        MUIA_Application_Author     , (IPTR)"Chris Hodges <chrisly@platon42.de>",
        MUIA_Application_Description, (IPTR)"Settings for the usbaudio.class",
        MUIA_Application_Base       , (IPTR)"USBAUDIO",
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
                Child, (IPTR)ColGroup(2), (IPTR)GroupFrameT((IPTR)"Global Settings"),
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
void nGUITaskCleanup(struct NepAudioBase *nh)
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

/**************************************************************************/

#undef UtilityBase
#undef ps
#define ps nam->nam_PsdBase

/* /// "subLib stuff" */
AROS_UFH3(SUBLIBBASETYPEPTR, subLibInit,
          AROS_UFHA(SUBLIBBASETYPEPTR, nas, D0),
          AROS_UFHA(BPTR, seglist, A0),
          AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    KPRINTF(10, ("subLibInit base: 0x%08lx seglist: 0x%08lx SysBase: 0x%08lx\n",
                 nas, seglist, SysBase));

    nas->nas_Library.lib_Node.ln_Type = NT_LIBRARY;
    nas->nas_Library.lib_Node.ln_Name = SUBLIBNAME;
    nas->nas_Library.lib_Flags        = LIBF_SUMUSED | LIBF_CHANGED;
    nas->nas_Library.lib_Version      = AHI_SUB_LIB_VERSION;
    nas->nas_Library.lib_Revision     = REVISION_NUMBER;
    nas->nas_Library.lib_IdString     = VERSION_STRING;

    /* Store segment */
    nas->nas_SegList = seglist;

#define	UtilityBase	nas->nas_UtilityBase
    if((nas->nas_UtilityBase = OpenLibrary("utility.library", 0)))
    {
        KPRINTF(10, ("subLibInit: Ok\n"));
        KPRINTF(10, ("subLibInit: openCnt = %ld\n", nas->nas_Library.lib_OpenCnt));
        return(nas);
    }
    else
    {
        return(NULL);
    }

    return(nas);
    
    AROS_USERFUNC_EXIT
}

AROS_LH1(SUBLIBBASETYPEPTR, subLibOpen,
         AROS_LHA(ULONG, version, D0),
         SUBLIBBASETYPEPTR, nas, 1, nep);
{
    AROS_LIBFUNC_INIT

    //struct NepAudioBase *nh = nas->nas_ClsBase;

    KPRINTF(10, ("subLibOpen base: 0x%08lx\n", nas));

    ++nas->nas_Library.lib_OpenCnt;
    nas->nas_Library.lib_Flags &= ~LIBF_DELEXP;

    KPRINTF(10, ("subLibOpen: openCnt = %ld\n", nas->nas_Library.lib_OpenCnt));

    return(nas);
    
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, subLibClose,
         SUBLIBBASETYPEPTR, nas, 2, nep);
{
    AROS_LIBFUNC_INIT

    BPTR ret;

    KPRINTF(10, ("subLibClose base: 0x%08lx\n", nas));

    ret = NULL;

    if(--nas->nas_Library.lib_OpenCnt == 0)
    {
        if(nas->nas_Library.lib_Flags & LIBF_DELEXP)
        {
            KPRINTF(5, ("subLibClose: calling expunge...\n"));
            ret = AROS_LC1(BPTR, subLibExpunge,
                           AROS_LCA(SUBLIBBASETYPEPTR, nas, D0),
                           SUBLIBBASETYPEPTR, nas, 3, dev);
        }
    }

    KPRINTF(5, ("subLibClose: lib_OpenCnt = %ld\n", nas->nas_Library.lib_OpenCnt));

    return(ret);
    
    AROS_LIBFUNC_EXIT
}

AROS_LH1(BPTR, subLibExpunge,
         AROS_LHA(SUBLIBBASETYPEPTR, extralh, D0),
         SUBLIBBASETYPEPTR, nas, 3, nep)
{
    AROS_LIBFUNC_INIT
    
    BPTR ret;

    KPRINTF(10, ("subLibExpunge base: 0x%08lx\n", nas));

    ret = NULL;

    if(nas->nas_Library.lib_OpenCnt == 0)
    {
        KPRINTF(5, ("subLibExpunge: Unloading...\n"));

        CloseLibrary(nas->nas_UtilityBase);

        ret = nas->nas_SegList;

        KPRINTF(5, ("subLibExpunge: removing library node 0x%08lx\n",
                    &nas->nas_Library.lib_Node));
        Remove(&nas->nas_Library.lib_Node);

        KPRINTF(5, ("subLibExpunge: FreeMem()...\n"));
        FreeMem((char *) nas - nas->nas_Library.lib_NegSize,
                (ULONG) (nas->nas_Library.lib_NegSize + nas->nas_Library.lib_PosSize));

        KPRINTF(5, ("subLibExpunge: Unloading done! usbaudio.audio expunged!\n\n"));

        return(ret);
    }
    else
    {
        KPRINTF(5, ("subLibExpunge: Could not expunge, LIBF_DELEXP set!\n"));
        nas->nas_Library.lib_Flags |= LIBF_DELEXP;
    }

    return(NULL);
    
    AROS_LIBFUNC_EXIT
}

AROS_LH0(SUBLIBBASETYPEPTR, subLibReserved,
         SUBLIBBASETYPEPTR, nas, 4, nep)
{
    AROS_LIBFUNC_INIT
    return NULL;
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "subLibPlayerIntV4()" */
AROS_INTH1(subLibPlayerIntV4, struct NepAudioMode *, nam)
{
    AROS_INTFUNC_INIT
    
    struct NepAudioSubLibBase *nas = nam->nam_SubLibBase;
    struct AHIAudioCtrlDrv *audioctrl = nam->nam_AudioCtrl;
    UWORD bufnum = nam->nam_NextBufW;

    CallHookPkt(audioctrl->ahiac_PlayerFunc, nam->nam_AudioCtrl, NULL);
    //KPRINTF(1, ("%ld F, %ld sam\n", nam->nam_PlayerMS, cnt));
    if(!nam->nam_Unit->nch_DenyRequests)
    {
        CallHookPkt(audioctrl->ahiac_MixerFunc, nam->nam_AudioCtrl, nam->nam_AHIBuffer);
        CallHookPkt(&nam->nam_SamConvHook, nam->nam_USBBuffer[bufnum], (APTR) (IPTR) nam->nam_AudioCtrl->ahiac_BuffSamples);
        nam->nam_NextBufR = bufnum;
        nam->nam_NextBufW = 1 - bufnum;
    }
    
    return FALSE;
    
    AROS_INTFUNC_EXIT
}
/* \\\ */

/* /// "subLibPlayerIntV6()" */
AROS_INTH1(subLibPlayerIntV6, struct NepAudioMode *, nam)
{
    AROS_INTFUNC_INIT
    
    struct NepAudioSubLibBase *nas = nam->nam_SubLibBase;
    struct AHIAudioCtrlDrv *audioctrl = nam->nam_AudioCtrl;
    UWORD bufnum = nam->nam_NextBufW;
    BOOL skipit;

    CallHookPkt(audioctrl->ahiac_PlayerFunc, nam->nam_AudioCtrl, NULL);
    skipit = CallHookPkt(audioctrl->ahiac_PreTimerFunc, nam->nam_AudioCtrl, NULL);
    //KPRINTF(1, ("%ld F, %ld sam\n", nam->nam_PlayerMS, cnt));
    if(!(skipit || nam->nam_Unit->nch_DenyRequests))
    {
        CallHookPkt(audioctrl->ahiac_MixerFunc, nam->nam_AudioCtrl, nam->nam_AHIBuffer);
        CallHookPkt(&nam->nam_SamConvHook, nam->nam_USBBuffer[bufnum], (APTR) (IPTR) nam->nam_AudioCtrl->ahiac_BuffSamples);
        nam->nam_NextBufR = bufnum;
        nam->nam_NextBufW = 1 - bufnum;
    }
    CallHookPkt(audioctrl->ahiac_PostTimerFunc, nam->nam_AudioCtrl, NULL);

    return FALSE;
    
    AROS_INTFUNC_EXIT
}
/* \\\ */

/* /// "subLibPlayerIntDummy()" */
AROS_INTH1(subLibPlayerIntDummy, struct NepAudioMode *, nam)
{
    AROS_INTFUNC_INIT

    struct NepAudioSubLibBase *nas = nam->nam_SubLibBase;
    struct AHIAudioCtrlDrv *audioctrl = nam->nam_AudioCtrl;

    CallHookPkt(audioctrl->ahiac_PlayerFunc, nam->nam_AudioCtrl, NULL);
    CallHookPkt(audioctrl->ahiac_MixerFunc, nam->nam_AudioCtrl, nam->nam_AHIBuffer);

    nam->nam_TimerIOReq->tr_node.io_Command = TR_ADDREQUEST;
    nam->nam_TimerIOReq->tr_time.tv_secs = 0;
    nam->nam_TimerIOReq->tr_time.tv_micro = 1000000 / (audioctrl->ahiac_PlayerFreq>>16);
    SendIO((struct IORequest *) nam->nam_TimerIOReq);

    return FALSE;
    
    AROS_INTFUNC_EXIT
}
/* \\\ */

/* /// "nOutReqHook()" */

AROS_UFH3(void, nOutReqHook,
          AROS_UFHA(struct Hook *, hook, A0),
          AROS_UFHA(struct IOUsbHWRTIso *, urti, A2),
          AROS_UFHA(struct IOUsbHWBufferReq *, ubr, A1))
{
    AROS_USERFUNC_INIT

    struct NepAudioMode *nam = (struct NepAudioMode *) hook->h_Data;

    nam->nam_PlayerTimer -= nam->nam_Interval;
    if(nam->nam_PlayerTimer < 0)
    {
        UWORD bufnum = nam->nam_NextBufW;
        Fixed cnt = nam->nam_USBBufCnt[bufnum];

        nam->nam_PlayerInterval &= 0xffff;
        nam->nam_PlayerInterval += nam->nam_PlayerFrac;
        nam->nam_PlayerMS = nam->nam_PlayerInterval>>16;
        nam->nam_PlayerTimer += nam->nam_PlayerMS;

        // calc number of samples to mix
        cnt += nam->nam_SampleFrac * nam->nam_PlayerMS;
        nam->nam_USBBufCnt[1 - bufnum] = cnt & 0xffff; // start of buffer count for next buffer
        cnt >>= 16;
        nam->nam_AudioCtrl->ahiac_BuffSamples = cnt;
        nam->nam_USBBufLen[bufnum] = cnt * nam->nam_FrameSize;

        Cause(&nam->nam_PlayerInt);
        /*if(nam->nam_USBCount)
        {
            KPRINTF(200, ("narf %ld - %ld, %lx, %lx %ld!\n", nam->nam_USBCount, ubr->ubr_Length, nam->nam_PlayerInterval, nam->nam_BufferCount, nam->nam_PlayerTimer));
        }*/
        nam->nam_USBCount = 0;
        //KPRINTF(1, ("NT %ld\n", nam->nam_PlayerTimer));
    }
    if(nam->nam_Unit->nch_DenyRequests)
    {
        ubr->ubr_Length = 0;
        return;
    }
 
    if(nam->nam_USBCount <= 0)
    {
        // switching is done via Mixer Int
        // ubr_Buffer automatically increases
        ubr->ubr_Buffer = (UBYTE *) nam->nam_USBBuffer[nam->nam_NextBufR];
        nam->nam_BufferCount = nam->nam_USBBufCnt[nam->nam_NextBufR];
        nam->nam_USBCount = nam->nam_USBBufLen[nam->nam_NextBufR];
    }
    nam->nam_BufferCount &= 0xffff;
    nam->nam_BufferCount += nam->nam_SampleFrac;
    ubr->ubr_Length = (nam->nam_BufferCount>>16) * nam->nam_FrameSize;
    nam->nam_USBCount -= ubr->ubr_Length;
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nInReqHook()" */
AROS_UFH3(void, nInReqHook,
          AROS_UFHA(struct Hook *, hook, A0),
          AROS_UFHA(struct IOUsbHWRTIso *, urti, A2),
          AROS_UFHA(struct IOUsbHWBufferReq *, ubr, A1))
{
    AROS_USERFUNC_INIT
    
    struct NepAudioMode *nam = (struct NepAudioMode *) hook->h_Data;
    //struct NepAudioSubLibBase *nas = nam->nam_SubLibBase;

    //KPRINTF(1, ("IR %ld\n", ubr->ubr_Length));
    ubr->ubr_Buffer = (UBYTE *) nam->nam_USBBuffer[0];
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nInDoneHook()" */
AROS_UFH3(void, nInDoneHook,
          AROS_UFHA(struct Hook *, hook, A0),
          AROS_UFHA(struct IOUsbHWRTIso *, urti, A2),
          AROS_UFHA(struct IOUsbHWBufferReq *, ubr, A1))
{
    AROS_USERFUNC_INIT
    
    struct NepAudioMode *nam = (struct NepAudioMode *) hook->h_Data;
    struct NepAudioSubLibBase *nas = nam->nam_SubLibBase;

    ULONG cnt = ubr->ubr_Length;

    //KPRINTF(1, ("ID %ld\n", cnt));
    if(cnt < nam->nam_FrameSize)
    {
        return;
    }
    switch(nam->nam_FrameSize)
    {
        case 2:
            cnt >>= 1;
            break;

        case 3:
            cnt /= 3;

        case 4:
            cnt >>= 2;
            break;

        case 6:
            cnt /= 6;
            break;

        case 8:
            cnt >>= 3;
            break;
    }
    // target format is always 16 bit stereo
    nam->nam_RecMsg.ahirm_Type = AHIST_S16S;
    nam->nam_RecMsg.ahirm_Buffer = nam->nam_AHIBuffer;
    nam->nam_RecMsg.ahirm_Length = cnt;
    CallHookPkt(&nam->nam_SamConvHook, nam->nam_AHIBuffer, (APTR) (IPTR) cnt);

    CallHookPkt(nam->nam_AudioCtrl->ahiac_SamplerFunc, nam->nam_AudioCtrl, &nam->nam_RecMsg);
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nReleaseHook()" */
AROS_UFH3(void, nReleaseHook,
          AROS_UFHA(struct Hook *, hook, A0),
          AROS_UFHA(APTR, prt, A2),
          AROS_UFHA(APTR, unused, A1))
{
    AROS_USERFUNC_INIT

    struct NepAudioMode *nam = (struct NepAudioMode *) hook->h_Data;
    struct NepClassAudio *nch = nam->nam_Unit;

    psdStopRTIso(nam->nam_RTIso);
    if(nam->nam_IsInput)
    {
        // we can only stop recording, we still need to call the player func until audio is done
        psdAddErrorMsg(RETURN_ERROR, (STRPTR) libname, "Violently stopped recording!");
    } else {
        nam->nam_PlayerInt.is_Code = (VOID_FUNC)subLibPlayerIntDummy;
        // start timer device
        nam->nam_FallbackTimer = TRUE;
        Cause(&nam->nam_PlayerInt);
    }
    nch->nch_DenyRequests = TRUE;
    psdAddErrorMsg(RETURN_ERROR, (STRPTR) libname,
                   "Removing the soundcard while playing is not very bright!");
    
    AROS_USERFUNC_EXIT

}
/* \\\ */

/* /// "nConv8BitMono()" */
AROS_UFH3(void, nConv8BitMono,
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(BYTE *, btarptr, A2),
          AROS_UFPA(ULONG, cnt, A1))
{
    AROS_USERFUNC_INIT

    WORD *srcptr = hook->h_Data;
    do
    {
        *btarptr++ = *((BYTE *) srcptr);
        srcptr++;
    } while(--cnt);
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nConv8BitStereo()" */
AROS_UFH3(void, nConv8BitStereo,
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(BYTE *, btarptr, A2),
          AROS_UFPA(ULONG, cnt, A1))
{
    AROS_USERFUNC_INIT

    WORD *srcptr = hook->h_Data;
    do
    {
        *btarptr++ = *((BYTE *) srcptr);
        srcptr++;
        *btarptr++ = *((BYTE *) srcptr);
        srcptr++;
    } while(--cnt);
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nConv16BitMono()" */
AROS_UFH3(void, nConv16BitMono,
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(UWORD *, wtarptr, A2),
          AROS_UFPA(ULONG, cnt, A1))
{
    AROS_USERFUNC_INIT

    UWORD *srcptr = hook->h_Data;
    do
    {
        *wtarptr++ = AROS_WORD2LE(*srcptr);
        srcptr++;
    } while(--cnt);
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nConv16BitStereo()" */
AROS_UFH3(void, nConv16BitStereo,
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(ULONG *, ltarptr, A2),
          AROS_UFPA(ULONG, cnt, A1))
{
    AROS_USERFUNC_INIT
    
    ULONG *lsrcptr = hook->h_Data;
    do
    {
        *ltarptr++ = ((*lsrcptr>>8) & 0x00ff00ff)|((*lsrcptr<<8) & 0xff00ff00);
        lsrcptr++;
    } while(--cnt);
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nConv24BitMono()" */
AROS_UFH3(void, nConv24BitMono,
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(UBYTE *, btarptr, A2),
          AROS_UFPA(ULONG, cnt, A1))
{
    AROS_USERFUNC_INIT

    ULONG *lsrcptr = hook->h_Data;
    ULONG tmp;
    do
    {
        tmp = *lsrcptr++;
        tmp >>= 8;
        *btarptr++ = tmp;
        tmp >>= 8;
        *btarptr++ = tmp;
        tmp >>= 8;
        *btarptr++ = tmp;
    } while(--cnt);
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nConv24BitStereo()" */
AROS_UFH3(void, nConv24BitStereo,
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(UBYTE *, btarptr, A2),
          AROS_UFPA(ULONG, cnt, A1))
{
    AROS_USERFUNC_INIT

    ULONG *lsrcptr = hook->h_Data;
    ULONG tmp;
    do
    {
        tmp = *lsrcptr++;
        tmp >>= 8;
        *btarptr++ = tmp;
        tmp >>= 8;
        *btarptr++ = tmp;
        tmp >>= 8;
        *btarptr++ = tmp;
        tmp = *lsrcptr++;
        tmp >>= 8;
        *btarptr++ = tmp;
        tmp >>= 8;
        *btarptr++ = tmp;
        tmp >>= 8;
        *btarptr++ = tmp;
    } while(--cnt);
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nConv32BitMono()" */
AROS_UFH3(void, nConv32BitMono,
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(ULONG *, ltarptr, A2),
          AROS_UFPA(ULONG, cnt, A1))
{
    AROS_USERFUNC_INIT

    UBYTE *bsrcptr = hook->h_Data;
    register ULONG tmp;
    do
    {
        tmp = *bsrcptr++;
        tmp |= (*bsrcptr++)<<8;
        tmp |= (*bsrcptr++)<<16;
        tmp |= (*bsrcptr++)<<24;
        *ltarptr++ = tmp;
    } while(--cnt);
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nConv32BitStereo()" */
AROS_UFH3(void, nConv32BitStereo,
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(ULONG *, ltarptr, A2),
          AROS_UFPA(ULONG, cnt, A1))
{
    AROS_USERFUNC_INIT

    UBYTE *bsrcptr = hook->h_Data;
    register ULONG tmp;
    do
    {
        tmp = *bsrcptr++;
        tmp |= (*bsrcptr++)<<8;
        tmp |= (*bsrcptr++)<<16;
        tmp |= (*bsrcptr++)<<24;
        *ltarptr++ = tmp;
        tmp = *bsrcptr++;
        tmp |= (*bsrcptr++)<<8;
        tmp |= (*bsrcptr++)<<16;
        tmp |= (*bsrcptr++)<<24;
        *ltarptr++ = tmp;
    } while(--cnt);
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nRec8BitMono()" */
AROS_UFH3(void, nRec8BitMono,
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(WORD *, tarptr, A2),
          AROS_UFPA(ULONG, cnt, A1))
{
    AROS_USERFUNC_INIT

    BYTE *srcptr = hook->h_Data;
    do
    {
        *tarptr++ = *srcptr;
        *tarptr++ = *srcptr++;
    } while(--cnt);
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nRec8BitStereo()" */
AROS_UFH3(void, nRec8BitStereo,
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(WORD *, tarptr, A2),
          AROS_UFPA(ULONG, cnt, A1))
{
    AROS_USERFUNC_INIT

    BYTE *srcptr = hook->h_Data;
    do
    {
        *tarptr++ = *srcptr++;
        *tarptr++ = *srcptr++;
    } while(--cnt);
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nRec16BitMono()" */
AROS_UFH3(void, nRec16BitMono,
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(WORD *, tarptr, A2),
          AROS_UFPA(ULONG, cnt, A1))
{
    AROS_USERFUNC_INIT

    UWORD *srcptr = hook->h_Data;
    do
    {
        WORD src = AROS_WORD2LE(*srcptr);
        *tarptr++ = src;
        *tarptr++ = src;
        srcptr++;
    } while(--cnt);
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nRec16BitStereo()" */
AROS_UFH3(void, nRec16BitStereo,
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(WORD *, tarptr, A2),
          AROS_UFPA(ULONG, cnt, A1))
{
    AROS_USERFUNC_INIT

    UWORD *srcptr = hook->h_Data;
    do
    {
        *tarptr++ = AROS_WORD2LE(*srcptr);
        srcptr++;
        *tarptr++ = AROS_WORD2LE(*srcptr);
        srcptr++;
    } while(--cnt);
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nRec24BitMono()" */
AROS_UFH3(void, nRec24BitMono,
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(WORD *, tarptr, A2),
          AROS_UFPA(ULONG, cnt, A1))
{
    AROS_USERFUNC_INIT

    UBYTE *srcptr = hook->h_Data;
    srcptr++;
    do
    {
        UWORD src = *srcptr|(srcptr[1]<<8);
        *tarptr++ = src;
        *tarptr++ = src;
        srcptr += 3;
    } while(--cnt);
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nRec24BitStereo()" */
AROS_UFH3(void, nRec24BitStereo,
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(WORD *, tarptr, A2),
          AROS_UFPA(ULONG, cnt, A1))
{
    AROS_USERFUNC_INIT

    UBYTE *srcptr = hook->h_Data;
    srcptr++;
    do
    {
        *tarptr++ = *srcptr|(srcptr[1]<<8);
        *tarptr++ = srcptr[3]|(srcptr[4]<<8);
        srcptr += 6;
    } while(--cnt);
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nRec32BitMono()" */
AROS_UFH3(void, nRec32BitMono,
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(WORD *, tarptr, A2),
          AROS_UFPA(ULONG, cnt, A1))
{
    AROS_USERFUNC_INIT

    UWORD *srcptr = hook->h_Data;
    srcptr++;
    do
    {
        UWORD src = AROS_WORD2LE(*srcptr);
        *tarptr++ = src;
        *tarptr++ = src;
        srcptr += 2;
    } while(--cnt);
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nRec32BitStereo()" */
AROS_UFH3(void, nRec32BitStereo,
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(WORD *, tarptr, A2),
          AROS_UFPA(ULONG, cnt, A1))
{
    AROS_USERFUNC_INIT

    UWORD *srcptr = hook->h_Data;
    srcptr++;
    do
    {
        *tarptr++ = AROS_WORD2LE(*srcptr);
        srcptr += 2;
        *tarptr++ = AROS_WORD2LE(*srcptr);
        srcptr += 2;
    } while(--cnt);
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "nSelectAudioMode()" */
BOOL nSelectAudioMode(struct NepAudioMode *nam)
{
    struct NepClassAudio *nch = nam->nam_Unit;
    UBYTE freqbuf[3];
    ULONG freq = nam->nam_AudioCtrl->ahiac_MixFreq;
    LONG ioerr;
    ULONG maxfreq;

    KPRINTF(1, ("SelectingAltInterface\n"));
    if(!(psdSetAltInterface(nam->nam_EP0Pipe, nam->nam_Interface)))
    {
        psdAddErrorMsg(RETURN_FAIL, (STRPTR) libname, "Cannot select alternate interface!");
        return(FALSE);
    }
    nam->nam_EP = psdFindEndpoint(nam->nam_Interface, NULL,
                                  EA_IsIn, nam->nam_IsInput,
                                  EA_TransferType, USEAF_ISOCHRONOUS,
                                  TAG_END);

    if(!nam->nam_EP)
    {
        KPRINTF(1, ("Ooops!?! No Endpoints defined?\n"));
        psdAddErrorMsg(RETURN_FAIL, (STRPTR) libname,
                       "No Iso Endpoint!");
        return(FALSE);
    }
    psdGetAttrs(PGA_ENDPOINT, nam->nam_EP,
                EA_EndpointNum, &nam->nam_EPNum,
                EA_MaxPktSize, &nam->nam_MaxPktSize,
                EA_Interval, &nam->nam_Interval,
                TAG_END);

    maxfreq = (1000 * nam->nam_MaxPktSize) / (nam->nam_FrameSize * nam->nam_Interval);
    if(maxfreq < nam->nam_MaxFreq)
    {
        psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                       "Maximum frequency was indicated as %ld Hz, but bandwidth-wise, only %ld Hz is possible.",
                       nam->nam_MaxFreq, maxfreq);
        nam->nam_MaxFreq = maxfreq;
    }

    if(nam->nam_HasFreqCtrl)
    {
        KPRINTF(1, ("Setting frequency\n"));
        freqbuf[0] = freq;
        freqbuf[1] = (freq>>8);
        freqbuf[2] = (freq>>16);
        psdPipeSetup(nam->nam_EP0Pipe, URTF_CLASS|URTF_ENDPOINT, UAUDR_SET_CUR, UAECS_SAMPLE_FREQ<<8, nam->nam_EPNum);
        ioerr = psdDoPipe(nam->nam_EP0Pipe, freqbuf, 3);
        if(ioerr)
        {
            psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                           "Could not set current frequency for EP %ld: %s (%ld)!",
                           nam->nam_EPNum,
                           psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
        }
    }
    if(nam->nam_HasPitchCtrl)
    {
        KPRINTF(1, ("Setting pitch control\n"));
        freqbuf[0] = 1;
        psdPipeSetup(nam->nam_EP0Pipe, URTF_CLASS|URTF_ENDPOINT, UAUDR_SET_CUR, UAECS_PITCH<<8, nam->nam_EPNum);
        ioerr = psdDoPipe(nam->nam_EP0Pipe, freqbuf, 1);
        if(ioerr)
        {
            psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                           "Could not enable pitch control: %s (%ld)!",
                           psdNumToStr(NTS_IOERR, ioerr, "unknown"), ioerr);
        }
    }
    nam->nam_ReleaseHook.h_Entry = (HOOKFUNC) nReleaseHook;
    nam->nam_ReleaseHook.h_Data = nam;

    if(nam->nam_IsInput)
    {
        KPRINTF(1, ("Input hook\n"));
        nch->nch_InReqHook.h_Entry = (HOOKFUNC) nInReqHook;
        nch->nch_InReqHook.h_Data = nam;
        nch->nch_InDoneHook.h_Entry = (HOOKFUNC) nInDoneHook;
        nch->nch_InDoneHook.h_Data = nam;

        nam->nam_RTIso = psdAllocRTIsoHandler(nam->nam_EP,
                                              RTA_InRequestHook, &nch->nch_InReqHook,
                                              RTA_InDoneHook, &nch->nch_InDoneHook,
                                              RTA_ReleaseHook, &nam->nam_ReleaseHook,
                                              TAG_END);
    } else {
        KPRINTF(1, ("Output hook\n"));

        nch->nch_OutReqHook.h_Entry = (HOOKFUNC) nOutReqHook;
        nch->nch_OutReqHook.h_Data = nam;

        nam->nam_RTIso = psdAllocRTIsoHandler(nam->nam_EP,
                                              RTA_OutRequestHook, &nch->nch_OutReqHook,
                                              RTA_ReleaseHook, &nam->nam_ReleaseHook,
                                              TAG_END);
    }
    if(!nam->nam_RTIso)
    {
        psdAddErrorMsg(RETURN_FAIL, (STRPTR) libname, "Could not allocate RT Iso Handler!");
        return(FALSE);
    }
    /*psdPipeSetup(nch->nch_EP0Pipe, URTF_CLASS|URTF_ENDPOINT, UAUDR_SET_CUR, UAECS_SAMPLE_FREQ, nam->nam_EPNum);
    freq[0] = 44100 & 0xff;
    freq[1] = (44100>>8);
    freq[2] = (44100>>16);
    ioerr = psdDoPipe(nch->nch_EP0Pipe, freq, 3);
    if(ioerr)
    {
        psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                       "Could not set current frequency!");
    }
    psdPipeSetup(nch->nch_EP0Pipe, URTF_IN|URTF_CLASS|URTF_ENDPOINT, UAUDR_GET_CUR, UAECS_SAMPLE_FREQ, nam->nam_EPNum);
    ioerr = psdDoPipe(nch->nch_EP0Pipe, freq, 3);
    if(ioerr)
    {
        psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                       "Could not get current frequency!");
    } else {
        psdAddErrorMsg(RETURN_WARN, (STRPTR) libname,
                       "Current frequency: %ld Hz",
                       freq[0]|(freq[1]<<8)|(freq[2]<<16));
    }*/

    nch->nch_DenyRequests = FALSE;
    return(TRUE);
}
/* \\\ */

/* /// "subLibAllocAudio()" */
AROS_LH2(ULONG, subLibAllocAudio,
         AROS_LHA(struct TagItem *, tags, A1),
         AROS_LHA(struct AHIAudioCtrlDrv *, audioctrl, A2),
         SUBLIBBASETYPEPTR, nas, 5, nep)
{
    AROS_LIBFUNC_INIT
    
    struct NepAudioMode *nam = (struct NepAudioMode *) GetTagData(AHIDB_NepAudioMode, (IPTR) NULL, tags);
    struct NepAudioMode *sibnam;
    struct NepClassAudio *nch;
    ULONG flags;
    UWORD cnt;
    STRPTR audiomode;

    KPRINTF(10, ("subLibAllocAudio(%08lx)\n", audioctrl));
    if(!nam)
    {
        KPRINTF(20, ("Could not find NepAudioMode in GetAttr call!\n"));
        audioctrl->ahiac_DriverData = NULL;
        return AHISF_ERROR;
    }
    if(nam->nam_RTIso)
    {
        KPRINTF(20, ("Already in use!\n"));
        return AHISF_ERROR;
    }
    audioctrl->ahiac_DriverData = nam;
    nam->nam_AudioCtrl = audioctrl;
    if(!(nam->nam_PsdBase = OpenLibrary("poseidon.library", 4)))
    {
        return AHISF_ERROR;
    }

    nam->nam_FallbackTimer = FALSE;
    nam->nam_TimerMsgPort.mp_Node.ln_Type = NT_MSGPORT;
    nam->nam_TimerMsgPort.mp_Flags = PA_SOFTINT;
    nam->nam_TimerMsgPort.mp_SigTask = &nam->nam_PlayerInt;
    NewList(&nam->nam_TimerMsgPort.mp_MsgList);
    if((nam->nam_TimerIOReq = (struct timerequest *) CreateIORequest(&nam->nam_TimerMsgPort, sizeof(struct timerequest))))
    {
        if(!OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *) nam->nam_TimerIOReq, 0))
        {
            nam->nam_TimerIOReq->tr_node.io_Message.mn_Node.ln_Type = NT_FREEMSG;
        } else {
            DeleteIORequest((struct IORequest *) nam->nam_TimerIOReq);
            nam->nam_TimerIOReq = NULL;
            return AHISF_ERROR;
        }
    } else {
        return AHISF_ERROR;
    }

    nch = nam->nam_Unit;
    KPRINTF(10, ("CreateMsgPort\n"));
    if(!(nam->nam_TaskMsgPort = CreateMsgPort()))
    {
        return AHISF_ERROR;
    }
    KPRINTF(10, ("AllocPipe\n"));
    if(!(nam->nam_EP0Pipe = psdAllocPipe(nch->nch_Device, nam->nam_TaskMsgPort, NULL)))
    {
        return AHISF_ERROR;
    }
    KPRINTF(10, ("SelectAudioMode\n"));
    if(!(nSelectAudioMode(nam)))
    {
        return AHISF_ERROR;
    }
    nch->nch_CurrentMode = nam;

    flags = AHISF_MIXING;
    if(nam->nam_NumChannels > 1)
    {
        flags |= AHISF_KNOWSTEREO;
    }
    if(nam->nam_Resolution > 16)
    {
        flags |= AHISF_KNOWHIFI;
    }
    KPRINTF(10, ("Freq=%ld, BuffSamples = %ld, BuffSize = %ld\n",
                 audioctrl->ahiac_MixFreq, audioctrl->ahiac_BuffSamples, audioctrl->ahiac_BuffSize));
    KPRINTF(10, ("NumChan=%ld, Sounds=%ld, BuffType=%04lx\n",
                 audioctrl->ahiac_Channels, audioctrl->ahiac_Sounds, audioctrl->ahiac_BuffType));
    KPRINTF(10, ("MinPlayerFreq=%08lx, MaxPlayerFreq=%08lx\n",
                 audioctrl->ahiac_MinPlayerFreq, audioctrl->ahiac_MaxPlayerFreq));

    if(audioctrl->ahiac_PlayerFreq < 0x10000)
    {
        audioctrl->ahiac_PlayerFreq <<= 16;
    }

    if(audioctrl->ahiac_MinPlayerFreq < 0x10000)
    {
        audioctrl->ahiac_MinPlayerFreq <<= 16;
    }
    if(audioctrl->ahiac_MinPlayerFreq < 10<<16)
    {
        // 100ms intervals max
        audioctrl->ahiac_MinPlayerFreq = 10<<16;
    }

    if(audioctrl->ahiac_MaxPlayerFreq < 0x10000)
    {
        audioctrl->ahiac_MaxPlayerFreq <<= 16;
    }

    audioctrl->ahiac_Channels = nam->nam_NumChannels;
    /*audioctrl->ahiac_BuffType = nam->nam_SampleType; */

    for(cnt = 0; cnt < nam->nam_NumFrequencies; cnt++)
    {
        if(audioctrl->ahiac_MixFreq == nam->nam_FreqArray[cnt])
        {
            break;
        }
    }
    if(cnt == nam->nam_NumFrequencies)
    {
        // need rate adaption
        ULONG target = audioctrl->ahiac_MixFreq;
        ULONG diff = 100000;
        for(cnt = 0; cnt < nam->nam_NumFrequencies; cnt++)
        {
            if(nam->nam_FreqArray[cnt] > target)
            {
                if(nam->nam_FreqArray[cnt] - target < diff)
                {
                    diff = nam->nam_FreqArray[cnt] - target;
                    audioctrl->ahiac_MixFreq = nam->nam_FreqArray[cnt];
                }
            } else {
                if(target - nam->nam_FreqArray[cnt] < diff)
                {
                    diff = target - nam->nam_FreqArray[cnt];
                    audioctrl->ahiac_MixFreq = nam->nam_FreqArray[cnt];
                }
            }
        }
    }

    // buffer for 100 ms max
    audioctrl->ahiac_BuffSamples = (audioctrl->ahiac_MixFreq * nam->nam_Interval) / 10;
    audioctrl->ahiac_BuffSize = audioctrl->ahiac_BuffSamples * nam->nam_AHIFrameSize;

    nam->nam_AHIBuffer = psdAllocVec(audioctrl->ahiac_BuffSize);
    nam->nam_USBBuffer[0] = psdAllocVec(audioctrl->ahiac_BuffSamples * nam->nam_FrameSize);
    nam->nam_USBBuffer[1] = psdAllocVec(audioctrl->ahiac_BuffSamples * nam->nam_FrameSize);
    if(!(nam->nam_AHIBuffer && nam->nam_USBBuffer[0] && nam->nam_USBBuffer[1]))
    {
        return AHISF_ERROR;
    }

    // make sure to play silence, if CPU is too slow on first frame
    nam->nam_NextBufR = 1;
    nam->nam_NextBufW = 0;
    nam->nam_BufferCount = 0;
    nam->nam_PlayerTimer = 0;
    nam->nam_USBCount = 0;
    nam->nam_USBBufLen[0] = 0;
    nam->nam_USBBufLen[1] = 0;

    nam->nam_MasterVol = 0x00010000;
    nam->nam_ChannelVol[0] = 0x00010000;
    nam->nam_ChannelVol[1] = 0x00010000;
    nam->nam_InputGain = 0x00010000;
    nam->nam_MonitorVol = 0x00010000;

    KPRINTF(10, ("Freq=%ld, BuffSamples = %ld, BuffSize = %ld\n",
                 audioctrl->ahiac_MixFreq, audioctrl->ahiac_BuffSamples, audioctrl->ahiac_BuffSize));

    nam->nam_PlayerInt.is_Node.ln_Type = NT_INTERRUPT;
    nam->nam_PlayerInt.is_Node.ln_Name = "Player Interrupt";
    nam->nam_PlayerInt.is_Node.ln_Pri  = 0;
    nam->nam_PlayerInt.is_Data = nam;
    if(nam->nam_Unit->nch_AHIBase->lib_Version < 6)
    {
        KPRINTF(10, ("Using V4 code\n"));
        nam->nam_PlayerInt.is_Code = (VOID_FUNC) subLibPlayerIntV4;
    } else {
        KPRINTF(10, ("Using V6 code\n"));
        nam->nam_PlayerInt.is_Code = (VOID_FUNC) subLibPlayerIntV6;
    }
    nam->nam_SamConvHook.h_Data = nam->nam_AHIBuffer;

    switch(nam->nam_NumChannels|(nam->nam_SampleSize<<8))
    {
        case 1|(1<<8): // 8 bit mono
            nam->nam_SamConvHook.h_Entry = (APTR) nConv8BitMono;
            break;

        case 2|(1<<8): // 8 bit stereo
            nam->nam_SamConvHook.h_Entry = (APTR) nConv8BitStereo;
            break;

        case 1|(2<<8): // 16 bit mono
            nam->nam_SamConvHook.h_Entry = (APTR) nConv16BitMono;
            break;

        case 2|(2<<8): // 16 bit stereo
            nam->nam_SamConvHook.h_Entry = (APTR) nConv16BitStereo;
            break;

        case 1|(3<<8): // 24 bit mono
            nam->nam_SamConvHook.h_Entry = (APTR) nConv24BitMono;
            break;

        case 2|(3<<8): // 24 bit stereo
            nam->nam_SamConvHook.h_Entry = (APTR) nConv24BitStereo;
            break;

        case 1|(4<<8): // 32 bit mono
            nam->nam_SamConvHook.h_Entry = (APTR) nConv32BitMono;
            break;

        case 2|(4<<8): // 32 bit stereo
            nam->nam_SamConvHook.h_Entry = (APTR) nConv32BitStereo;
            break;

        default:
            KPRINTF(10, ("Unknown sample format\n"));
            return AHISF_ERROR;
    }

    if((sibnam = nam->nam_Sibling))
    {
        sibnam->nam_PsdBase = nam->nam_PsdBase;
        sibnam->nam_AudioCtrl = audioctrl;
        if(!(sibnam->nam_TaskMsgPort = CreateMsgPort()))
        {
            return AHISF_ERROR;
        }
        if(!(sibnam->nam_EP0Pipe = psdAllocPipe(nch->nch_Device, sibnam->nam_TaskMsgPort, NULL)))
        {
            return AHISF_ERROR;
        }
        if(!(nSelectAudioMode(sibnam)))
        {
            return AHISF_ERROR;
        }
        sibnam->nam_AHIBuffer = psdAllocVec(audioctrl->ahiac_BuffSize);
        sibnam->nam_USBBuffer[0] = psdAllocVec(audioctrl->ahiac_BuffSamples * sibnam->nam_FrameSize);
        if(!(sibnam->nam_AHIBuffer && sibnam->nam_USBBuffer[0]))
        {
            return AHISF_ERROR;
        }
        sibnam->nam_SamConvHook.h_Data = sibnam->nam_USBBuffer[0];

        switch(sibnam->nam_NumChannels|(sibnam->nam_SampleSize<<8))
        {
            case 1|(1<<8): // 8 bit mono
                sibnam->nam_SamConvHook.h_Entry = (APTR) nRec8BitMono;
                break;

            case 2|(1<<8): // 8 bit stereo
                sibnam->nam_SamConvHook.h_Entry = (APTR) nRec8BitStereo;
                break;

            case 1|(2<<8): // 16 bit mono
                sibnam->nam_SamConvHook.h_Entry = (APTR) nRec16BitMono;
                break;

            case 2|(2<<8): // 16 bit stereo
                sibnam->nam_SamConvHook.h_Entry = (APTR) nRec16BitStereo;
                break;

            case 1|(3<<8): // 24 bit mono
                sibnam->nam_SamConvHook.h_Entry = (APTR) nRec24BitMono;
                break;

            case 2|(3<<8): // 24 bit stereo
                sibnam->nam_SamConvHook.h_Entry = (APTR) nRec24BitStereo;
                break;

            case 1|(4<<8): // 32 bit mono
                sibnam->nam_SamConvHook.h_Entry = (APTR) nRec32BitMono;
                break;

            case 2|(4<<8): // 32 bit stereo
                sibnam->nam_SamConvHook.h_Entry = (APTR) nRec32BitStereo;
                break;

            default:
                KPRINTF(10, ("Unknown sample format\n"));
                return AHISF_ERROR;
        }

        flags |= AHISF_CANRECORD;
        audiomode = "Full-Duplex";
    } else {
        audiomode = "Output-only";
    }
    psdAddErrorMsg(RETURN_OK, (STRPTR) libname, "Opening %s audio mode at %ld Hz with %ld channel%s and %ld bits each.",
                   audiomode,
                   audioctrl->ahiac_MixFreq,
                   nam->nam_NumChannels,
                   (nam->nam_NumChannels > 1) ? "s" : "",
                   nam->nam_Resolution);

    return flags;
    
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "subLibFreeAudio()" */
AROS_LH1(void, subLibFreeAudio,
         AROS_LHA(struct AHIAudioCtrlDrv *, audioctrl, A2),
         SUBLIBBASETYPEPTR, nas, 6, nep)
{
    AROS_LIBFUNC_INIT

    struct NepAudioMode *nam = (struct NepAudioMode *) audioctrl->ahiac_DriverData;
    struct NepAudioMode *sibnam;

    KPRINTF(10, ("subLibFreeAudio(%08lx)\n", audioctrl));
    if(!nam)
    {
        return;
    }
    if(!nam->nam_PsdBase)
    {
        return;
    }
    nam->nam_Unit->nch_CurrentMode = NULL;
    if((sibnam = nam->nam_Sibling))
    {
        if(sibnam->nam_RTIso)
        {
            psdStopRTIso(sibnam->nam_RTIso);
            psdFreeRTIsoHandler(sibnam->nam_RTIso);
            sibnam->nam_RTIso = NULL;
        }

        psdFreeVec(sibnam->nam_AHIBuffer);
        sibnam->nam_AHIBuffer = NULL;
        psdFreeVec(sibnam->nam_USBBuffer[0]);
        sibnam->nam_USBBuffer[0] = NULL;

        if(sibnam->nam_ZeroBWIF)
        {
            if(!(psdSetAltInterface(sibnam->nam_EP0Pipe, sibnam->nam_ZeroBWIF)))
            {
                psdAddErrorMsg(RETURN_ERROR, (STRPTR) libname, "Cannot restore zero bandwidth interface!");
            }
        }
        if(sibnam->nam_EP0Pipe)
        {
            psdFreePipe(sibnam->nam_EP0Pipe);
            sibnam->nam_EP0Pipe = NULL;
        }
        DeleteMsgPort(sibnam->nam_TaskMsgPort);
        sibnam->nam_TaskMsgPort = NULL;
        sibnam->nam_PsdBase = NULL;
    }
    if(nam->nam_FallbackTimer)
    {
        nam->nam_TimerMsgPort.mp_Flags = PA_IGNORE;
        AbortIO((struct IORequest *) nam->nam_TimerIOReq);
        WaitIO((struct IORequest *) nam->nam_TimerIOReq);
    }
    if(nam->nam_RTIso)
    {
        psdStopRTIso(nam->nam_RTIso);
        psdFreeRTIsoHandler(nam->nam_RTIso);
        nam->nam_RTIso = NULL;
    }

    psdFreeVec(nam->nam_AHIBuffer);
    nam->nam_AHIBuffer = NULL;
    psdFreeVec(nam->nam_USBBuffer[0]);
    nam->nam_USBBuffer[0] = NULL;
    psdFreeVec(nam->nam_USBBuffer[1]);
    nam->nam_USBBuffer[1] = NULL;

    if(nam->nam_ZeroBWIF)
    {
        if(!(psdSetAltInterface(nam->nam_EP0Pipe, nam->nam_ZeroBWIF)))
        {
            psdAddErrorMsg(RETURN_ERROR, (STRPTR) libname, "Cannot restore zero bandwidth interface!");
        }
    }

    if(nam->nam_EP0Pipe)
    {
        psdFreePipe(nam->nam_EP0Pipe);
        nam->nam_EP0Pipe = NULL;
    }
    DeleteMsgPort(nam->nam_TaskMsgPort);
    nam->nam_TaskMsgPort = NULL;

    if(nam->nam_TimerIOReq)
    {
        CloseDevice((struct IORequest *) nam->nam_TimerIOReq);
        DeleteIORequest((struct IORequest *) nam->nam_TimerIOReq);
        nam->nam_TimerIOReq = NULL;
    }

    CloseLibrary(nam->nam_PsdBase);
    nam->nam_PsdBase = NULL;
    
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "subLibDisable()" */
AROS_LH1(void, subLibDisable,
         AROS_LHA(struct AHIAudioCtrlDrv *, audioctrl, A2),
         SUBLIBBASETYPEPTR, nas, 7, nep)
{
    AROS_LIBFUNC_INIT
    //KPRINTF(10, ("subLibDisable(%08lx)\n", audioctrl));
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "subLibEnable()" */
AROS_LH1(void, subLibEnable,
         AROS_LHA(struct AHIAudioCtrlDrv *, audioctrl, A2),
         SUBLIBBASETYPEPTR, nas, 8, nep)
{
    AROS_LIBFUNC_INIT
    //KPRINTF(10, ("subLibEnable(%08lx)\n", audioctrl));
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "subLibStart()" */
AROS_LH2(ULONG, subLibStart,
         AROS_LHA(ULONG, flags, D0),
         AROS_LHA(struct AHIAudioCtrlDrv *, audioctrl, A2),
         SUBLIBBASETYPEPTR, nas, 9, nep)
{
    AROS_LIBFUNC_INIT

    struct NepAudioMode *nam = (struct NepAudioMode *) audioctrl->ahiac_DriverData;
    ULONG res = AHIE_UNKNOWN;

    KPRINTF(10, ("subLibStart(%08lx, %lx)\n", audioctrl, flags));
    if(!nam)
    {
        return res;
    }
    if(flags & AHISF_PLAY)
    {
        KPRINTF(10, ("Freq=%ld, BuffSamples = %ld, BuffSize = %ld, PlayerFreq = %08lx\n",
                     audioctrl->ahiac_MixFreq, audioctrl->ahiac_BuffSamples, audioctrl->ahiac_BuffSize,
                     audioctrl->ahiac_PlayerFreq));
        KPRINTF(10, ("NumChan=%ld, Sounds=%ld, BuffType=%04lx\n",
                     audioctrl->ahiac_Channels, audioctrl->ahiac_Sounds, audioctrl->ahiac_BuffType));
        KPRINTF(10, ("MinPlayerFreq=%08lx, MaxPlayerFreq=%08lx\n",
                     audioctrl->ahiac_MinPlayerFreq, audioctrl->ahiac_MaxPlayerFreq));
        if(nam->nam_AudioCtrl->ahiac_PlayerFreq < 0x10000)
        {
            nam->nam_AudioCtrl->ahiac_PlayerFreq <<= 16;
        }
        nam->nam_PlayerFrac = (1000UL<<20) / (nam->nam_AudioCtrl->ahiac_PlayerFreq>>12);
        KPRINTF(10, ("Player Frac = %08lx\n", nam->nam_PlayerFrac));
        nam->nam_SampleFrac = (((audioctrl->ahiac_MixFreq<<16)) / 1000) * nam->nam_Interval;
        if(nam->nam_FallbackTimer)
        {
            nam->nam_TimerMsgPort.mp_Flags = PA_SOFTINT;
            Cause(&nam->nam_PlayerInt);
        } else {
            psdStartRTIso(nam->nam_RTIso);
        }
        res = AHIE_OK;
    }
    if(flags & AHISF_RECORD)
    {
        struct NepAudioMode *sibnam = nam->nam_Sibling;
        if(sibnam)
        {
            sibnam->nam_SampleFrac = ((audioctrl->ahiac_MixFreq<<16) / 1000) * sibnam->nam_Interval;
            psdStartRTIso(sibnam->nam_RTIso);
            res = AHIE_OK;
        } else {
            res = AHIE_UNKNOWN;
        }
    }
    return res;
    
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "subLibUpdate()" */
AROS_LH2(ULONG, subLibUpdate,
         AROS_LHA(ULONG, flags, D0),
         AROS_LHA(struct AHIAudioCtrlDrv *, audioctrl, A2),
         SUBLIBBASETYPEPTR, nas, 10, nep)
{
    AROS_LIBFUNC_INIT

    struct NepAudioMode *nam = (struct NepAudioMode *) audioctrl->ahiac_DriverData;
    KPRINTF(10, ("subLibUpdate(%08lx, %lx)\n", audioctrl, flags));

    if(!nam)
    {
        return AHIE_UNKNOWN;
    }
    if(nam->nam_AudioCtrl->ahiac_PlayerFreq < 0x10000)
    {
        nam->nam_AudioCtrl->ahiac_PlayerFreq <<= 16;
    }
    nam->nam_PlayerFrac = (1000UL<<20) / (nam->nam_AudioCtrl->ahiac_PlayerFreq>>12);
    return AHIE_OK;
    
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "subLibStop()" */
AROS_LH2(ULONG, subLibStop,
         AROS_LHA(ULONG, flags, D0),
         AROS_LHA(struct AHIAudioCtrlDrv *, audioctrl, A2),
         SUBLIBBASETYPEPTR, nas, 11, nep)
{
    AROS_LIBFUNC_INIT

    struct NepAudioMode *nam = (struct NepAudioMode *) audioctrl->ahiac_DriverData;

    KPRINTF(10, ("subLibStop(%08lx, %lx)\n", audioctrl, flags));
    if(!nam)
    {
        return AHIE_UNKNOWN;
    }
    if(flags & AHISF_PLAY)
    {
        if(nam->nam_FallbackTimer)
        {
            nam->nam_TimerMsgPort.mp_Flags = PA_IGNORE;
            AbortIO((struct IORequest *) nam->nam_TimerIOReq);
        } else {
            psdStopRTIso(nam->nam_RTIso);
        }
    }
    if((flags & AHISF_RECORD) && nam->nam_Sibling)
    {
        psdStopRTIso(nam->nam_Sibling->nam_RTIso);
    }
    return AHIE_OK;
    
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "subLibSetVol()" */
AROS_LH5(ULONG, subLibSetVol,
         AROS_LHA(UWORD, channel, D0),
         AROS_LHA(Fixed, volume, D1),
         AROS_LHA(sposition, pan, D2),
         AROS_LHA(struct AHIAudioCtrlDrv *, audioctrl, A2),
         AROS_LHA(ULONG, flags, D3),
         SUBLIBBASETYPEPTR, nas, 12, nep)
{
    AROS_LIBFUNC_INIT
    return AHIS_UNKNOWN;
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "subLibSetFreq()" */
AROS_LH4(ULONG, subLibSetFreq,
         AROS_LHA(UWORD, channel, D0),
         AROS_LHA(ULONG, freq, D1),
         AROS_LHA(struct AHIAudioCtrlDrv *, audioctrl, A2),
         AROS_LHA(ULONG, flags, D2),
         SUBLIBBASETYPEPTR, nas, 13, nep)
{
    AROS_LIBFUNC_INIT
    return AHIS_UNKNOWN;
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "subLibSetSound()" */
AROS_LH6(ULONG, subLibSetSound,
         AROS_LHA(UWORD, channel, D0),
         AROS_LHA(UWORD, sound, D1),
         AROS_LHA(ULONG, offset, D2),
         AROS_LHA(LONG, length, D3),
         AROS_LHA(struct AHIAudioCtrlDrv *, audioctrl, A2),
         AROS_LHA(ULONG, flags, D4),
         SUBLIBBASETYPEPTR, nas, 14, nep)
{
    AROS_LIBFUNC_INIT
    return AHIS_UNKNOWN;
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "subLibSetEffect()" */
AROS_LH2(ULONG, subLibSetEffect,
         AROS_LHA(ULONG *, effect, A0),
         AROS_LHA(struct AHIAudioCtrlDrv *, audioctrl, A2),
         SUBLIBBASETYPEPTR, nas, 15, nep)
{
    AROS_LIBFUNC_INIT
    return AHIS_UNKNOWN;
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "subLibLoadSound()" */
AROS_LH4(ULONG, subLibLoadSound,
         AROS_LHA(UWORD, sound, D0),
         AROS_LHA(ULONG, type, D1),
         AROS_LHA(APTR, info, A0),
         AROS_LHA(struct AHIAudioCtrlDrv *, audioctrl, A2),
         SUBLIBBASETYPEPTR, nas, 16, nep)
{
    AROS_LIBFUNC_INIT
    return AHIS_UNKNOWN;
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "subLibUnloadSound()" */
AROS_LH2(ULONG, subLibUnloadSound,
         AROS_LHA(UWORD, sound, D0),
         AROS_LHA(struct AHIAudioCtrlDrv *, audioctrl, A2),
         SUBLIBBASETYPEPTR, nas, 17, nep)
{
    AROS_LIBFUNC_INIT
    return AHIS_UNKNOWN;
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "subLibGetAttr()" */
AROS_LH5(IPTR, subLibGetAttr,
         AROS_LHA(ULONG, attr, D0),
         AROS_LHA(LONG, arg, D1),
         AROS_LHA(LONG, defvalue, D2),
         AROS_LHA(struct TagItem *, tags, A1),
         AROS_LHA(struct AHIAudioCtrlDrv *, audioctrl, A2),
         SUBLIBBASETYPEPTR, nas, 18, nep)
{
    AROS_LIBFUNC_INIT

    UWORD cnt;
    struct NepAudioMode *nam = (struct NepAudioMode *) GetTagData(AHIDB_NepAudioMode, (IPTR) NULL, tags);
    struct NepAudioUnit *nau;

    KPRINTF(10, ("subLibGetAttr(%08lx, %08lx, %ld, %ld)\n", audioctrl, attr, arg, defvalue));

    if(!nam)
    {
        KPRINTF(20, ("Could not find NepAudioMode in GetAttr call!\n"));
        return defvalue;
    }

    switch(attr)
    {
        case AHIDB_Bits:
            return (IPTR) nam->nam_Resolution;

        /*case AHIDB_MaxChannels:
            return (IPTR) nam->nam_NumChannels;

        case AHIDB_MinMixFreq:
            return (IPTR) nam->nam_MinFreq;

        case AHIDB_MaxMixFreq:
            return (IPTR) nam->nam_MaxFreq;*/

        case AHIDB_Frequencies:
            return (IPTR) nam->nam_NumFrequencies;

        case AHIDB_Frequency: // Index->Frequency
            return (IPTR) nam->nam_FreqArray[arg];

        case AHIDB_Index: // Frequency->Index
        {
            ULONG diff = 100000;
            LONG bestfit = 0;
            for(cnt = 0; cnt < nam->nam_NumFrequencies; cnt++)
            {
                if(nam->nam_FreqArray[cnt] > arg)
                {
                    if(nam->nam_FreqArray[cnt] - arg < diff)
                    {
                        diff = nam->nam_FreqArray[cnt] - arg;
                        bestfit = cnt;
                    }
                } else {
                    if(arg - nam->nam_FreqArray[cnt] < diff)
                    {
                        diff = arg - nam->nam_FreqArray[cnt];
                        bestfit = cnt;
                    }
                }
            }
            return bestfit;
        }

        case AHIDB_Author:
            return (IPTR) "Chris Hodges";

        case AHIDB_Copyright:
            return (IPTR) "2008-2009 Chris Hodges";

        case AHIDB_Version:
            return (IPTR) VERSION_STRING;

        case AHIDB_Annotation:
            return (IPTR) "Bye...";

        case AHIDB_Record:
            return nam->nam_IsInput || nam->nam_Sibling;

        case AHIDB_FullDuplex:
            return nam->nam_Sibling ? TRUE : FALSE;

        case AHIDB_Realtime:
            return TRUE;

        case AHIDB_MinOutputVolume:
            if(nam->nam_RootUnit->nau_VolumeControl)
            {
                KPRINTF(10, ("MinOutputVolume %08lx\n", nam->nam_RootUnit->nau_VolumeUnit->nau_MinVolume));
                return nam->nam_RootUnit->nau_VolumeUnit->nau_MinVolume;
            }
            return 0;

        case AHIDB_MaxOutputVolume:
            if(nam->nam_RootUnit->nau_VolumeControl)
            {
                KPRINTF(10, ("MaxOutputVolume %08lx\n", nam->nam_RootUnit->nau_VolumeUnit->nau_MaxVolume));
                return nam->nam_RootUnit->nau_VolumeUnit->nau_MaxVolume;
            }
            return 0;

        case AHIDB_MinInputGain:
            nau = nGetInputUnit(nam);
            if(nau)
            {
                if(nau->nau_VolumeControl)
                {
                    KPRINTF(10, ("MinInputGain %08lx\n", nau->nau_VolumeUnit->nau_MinVolume));
                    return nau->nau_VolumeUnit->nau_MinVolume;
                }
            }
            return 0;

        case AHIDB_MaxInputGain:
            nau = nGetInputUnit(nam);
            if(nau)
            {
                if(nau->nau_VolumeControl)
                {
                    KPRINTF(10, ("MaxInputGain %08lx\n", nau->nau_VolumeUnit->nau_MaxVolume));
                    return nau->nau_VolumeUnit->nau_MaxVolume;
                }
            }
            return 0;

        case AHIDB_MinMonitorVolume:
            nau = nGetInputUnit(nam);
            if(nau)
            {
                if(nau->nau_Monitor)
                {
                    KPRINTF(10, ("MinMonitorVol %08lx\n", nau->nau_MonitorUnit->nau_MinVolume));
                    return nau->nau_MonitorUnit->nau_MinVolume;
                }
            }
            return 0;

        case AHIDB_MaxMonitorVolume:
            nau = nGetInputUnit(nam);
            if(nau)
            {
                if(nau->nau_Monitor)
                {
                    KPRINTF(10, ("MaxMonitorVol %08lx\n", nau->nau_MonitorUnit->nau_MaxVolume));
                    return nau->nau_MonitorUnit->nau_MaxVolume;
                }
            }
            return 0;

        case AHIDB_Inputs:
            if(nam->nam_Sibling && nam->nam_Sibling->nam_RootUnit->nau_SelectorUnit)
            {
                return (IPTR) nam->nam_Sibling->nam_RootUnit->nau_SelectorUnit->nau_NumInputs;
            }
            return 0;

        case AHIDB_Input:
            if(nam->nam_Sibling && nam->nam_Sibling->nam_RootUnit->nau_SelectorUnit)
            {
                return (IPTR) nam->nam_Sibling->nam_RootUnit->nau_SelectorUnit->nau_InputUnit[arg]->nau_RootUnit->nau_Name;
            }
            return (IPTR) NULL;

        case AHIDB_Outputs:
            if(nam->nam_RootUnit->nau_SelectorUnit)
            {
                return (IPTR) nam->nam_RootUnit->nau_SelectorUnit->nau_NumInputs;
            }
            return (IPTR) nam->nam_NumOutputs;

        case AHIDB_Output:
            if(nam->nam_RootUnit->nau_SelectorUnit)
            {
                return (IPTR) nam->nam_RootUnit->nau_SelectorUnit->nau_InputUnit[arg]->nau_RootUnit->nau_Name;
            }
            return (IPTR) nam->nam_OutputNames[arg];
    }
    return defvalue;
    
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "subLibHardwareControl()" */
AROS_LH3(IPTR, subLibHardwareControl,
         AROS_LHA(ULONG, attr, D0),
         AROS_LHA(LONG, arg, D1),
         AROS_LHA(struct AHIAudioCtrlDrv *, audioctrl, A2),
         SUBLIBBASETYPEPTR, nas, 19, nep)
{
    AROS_LIBFUNC_INIT

    struct NepAudioMode *nam = (struct NepAudioMode *) audioctrl->ahiac_DriverData;
    struct NepClassAudio *nch;
    KPRINTF(10, ("subLibHardwareControl(%08lx, %08lx, %ld)\n", audioctrl, attr, arg));

    if(!nam)
    {
        return FALSE;
    }
    nch = nam->nam_Unit;

    switch(attr)
    {
        case AHIC_MixFreq_Query:
            return (IPTR) audioctrl->ahiac_MixFreq;

        case AHIC_InputGain:
            nam->nam_InputGain = arg;
            nch->nch_UpdateFlags |= UAF_INPUT_GAIN;
            Signal(nch->nch_Task, 1UL<<nch->nch_TaskMsgPort->mp_SigBit);
            return TRUE;

        case AHIC_InputGain_Query:
            return nam->nam_InputGain;

        case AHIC_MonitorVolume:
            nam->nam_MonitorVol = arg;
            nch->nch_UpdateFlags |= UAF_MONITOR_VOLUME;
            Signal(nch->nch_Task, 1UL<<nch->nch_TaskMsgPort->mp_SigBit);
            return TRUE;

        case AHIC_MonitorVolume_Query:
            return nam->nam_MonitorVol;

        case AHIC_OutputVolume:
            nam->nam_MasterVol = arg;
            nch->nch_UpdateFlags |= UAF_MASTER_VOLUME;
            Signal(nch->nch_Task, 1UL<<nch->nch_TaskMsgPort->mp_SigBit);
            return TRUE;

        case AHIC_OutputVolume_Query:
            return nam->nam_MasterVol;

        case AHIC_Input:
            if(nam->nam_NumInputs < 2)
            {
                return TRUE;
            }
            nam->nam_CurrInput = arg;
            nch->nch_UpdateFlags |= UAF_SELECT_INPUT;
            Signal(nch->nch_Task, 1UL<<nch->nch_TaskMsgPort->mp_SigBit);
            return TRUE;

        case AHIC_Input_Query:
            return nam->nam_CurrInput;

        case AHIC_Output:
            if(nam->nam_NumOutputs < 2)
            {
                return TRUE;
            }
            nam->nam_CurrOutput = arg;
            nch->nch_UpdateFlags |= UAF_SELECT_OUTPUT;
            Signal(nch->nch_Task, 1UL<<nch->nch_TaskMsgPort->mp_SigBit);
            return TRUE;

        case AHIC_Output_Query:
            return nam->nam_CurrOutput;
    }
    return FALSE;
    
    AROS_LIBFUNC_EXIT
}
/* \\\ */

