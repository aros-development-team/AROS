/* uhwcmd.c - denebusb.device by Chris Hodges
*/

#include "uhwcmd.h"
//#include <strings.h>

#ifdef __MORPHOS__
#define USE_INLINE_STDARG
#define __NOLIBBASE__
#define NewList NEWLIST
#elif !defined(__AROS__)
static void NewList( struct MinList *list )
{

//;Prepare a list header for use
//NEWLIST     MACRO   ; list
//        MOVE.L  \1,LH_TAILPRED(\1)
//        ADDQ.L  #4,\1   ;Get address of LH_TAIL
//        CLR.L   (\1)    ;Clear LH_TAIL
//        MOVE.L  \1,-(\1)    ;Address of LH_TAIL to LH_HEAD
//        ENDM

    list->mlh_Tail = 0;
    list->mlh_TailPred = (struct MinNode *)list;
    list->mlh_Head = (struct MinNode *)&list->mlh_Tail;
}
#endif

#if !defined(__AROS__)
static ULONG shuffle(__reg("d0") ULONG in)="\tror.w\t#8,d0\t; * shuffle\n\tswap\td0\n\tror.w\t#8,d0";
#else
static ULONG shuffle(ULONG in)
{
    return in;
}
#endif

static void CopyMemShuffle( APTR source, APTR dest, unsigned long size )
{
    ULONG* src = source;
    ULONG* dst = dest;
    ULONG longs = size >> 2;
    while(longs--)
    {
        ULONG v = *src++;
        v = shuffle(v);
        *dst++ = v;
    }
}

#include <proto/exec.h>
#include <proto/expansion.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <stddef.h>

#if !defined(__AROS__)
DECLGATE(static const, uhwLevel6Int, LIB)
DECLGATE(static const, uhwSoftInt, LIBNR)
DECLGATE(static const, uhwDMATask, LIBNR)
DECLGATE(static const, uhwNakTimeoutInt, LIBNR)
#endif

/* /// "uhwOpenTimer()" */
BOOL uhwOpenTimer(struct DenebUnit *unit, struct DenebDevice *base)
{
    if(unit->hu_MsgPort = CreateMsgPort())
    {
        if(unit->hu_TimerReq = CreateIORequest(unit->hu_MsgPort, sizeof(struct timerequest)))
        {
            if(!OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *) unit->hu_TimerReq, 0))
            {
                unit->hu_TimerReq->tr_node.io_Message.mn_Node.ln_Name = "DenebUSB hardware";
                unit->hu_TimerReq->tr_node.io_Command = TR_ADDREQUEST;
                KPRINTF(1, ("opened timer device\n"));
                return(TRUE);
            }
            DeleteIORequest(unit->hu_TimerReq);
            unit->hu_TimerReq = NULL;
        }
        DeleteMsgPort(unit->hu_MsgPort);
        unit->hu_MsgPort = NULL;
    }
    KPRINTF(5, ("failed to open timer.device\n"));
    return(0);
}
/* \\\ */

/* /// "uhwDelayMS()" */
void uhwDelayMS(ULONG milli, struct DenebUnit *unit, struct DenebDevice *base)
{
    unit->hu_TimerReq->tr_time.tv_secs  = 0;
    unit->hu_TimerReq->tr_time.tv_micro = milli * 1000;
    DoIO((struct IORequest *) unit->hu_TimerReq);
}
/* \\\ */

/* /// "uhwCloseTimer()" */
void uhwCloseTimer(struct DenebUnit *unit, struct DenebDevice *base)
{
    if(unit->hu_MsgPort)
    {
        if(unit->hu_TimerReq)
        {
            KPRINTF(1, ("closing timer.device\n"));
            CloseDevice((struct IORequest *) unit->hu_TimerReq);
            DeleteIORequest(unit->hu_TimerReq);
            unit->hu_TimerReq = NULL;
        }
        DeleteMsgPort(unit->hu_MsgPort);
        unit->hu_MsgPort = NULL;
    }
}
/* \\\ */

/* /// "uhwHWInit()" */
WORD uhwHWInit(struct DenebUnit *unit)
{
    struct DenebDevice *base = unit->hu_Device;
    ULONG intpend;
    ULONG cnt;
    ULONG timeout;
    ULONG tmpval;

    KPRINTF(1, ("Reset\n"));
#ifndef ZORRO_II
    WRITEMACH(MACH_CTRL, MCF_RESET);
    uhwDelayMS(1, unit, base);
    WRITEMACH(MACH_CTRL, 0);
#else
    WRITEMACH(MACH_CTRL_Z2, MCF_RESET);
    uhwDelayMS(1, unit, base);
    WRITEMACH(MACH_CTRL_Z2, 0);
#endif
    uhwDelayMS(5, unit, base);
    KPRINTF(1, ("Mach nulled\n"));
    unit->hu_FrameCounter = 1;

    WRITEREG(EHCI_USBSTS, 0);
    WRITEREG(ISP_SWRESET, ISWRF_RESET_ALL);
    //WRITEREG(ISP_SWRESET, ISWRF_RESET_HC);
    WRITEREG(EHCI_USBCMD, EHCF_HCRESET);
    uhwDelayMS(50, unit, base);
    WRITEREG(EHCI_USBCMD, 0);
    WRITEREG(ISP_HWMODECTRL, IHWCF_DENEB);
    WRITEREG(ISP_HWMODECTRL, IHWCF_DENEB|IHWCF_ALL_ATX_RST);
    uhwDelayMS(15, unit, base);
    WRITEREG(ISP_HWMODECTRL, IHWCF_DENEB);
    WRITEREG(ISP_DCMODE, 0);

    // Set the force-host-mode bits, and clear the others (w/o touching reserved bits)
    tmpval = ( (OTGCF_FORCE_HOST << OTGCB_SET) & OTGCF_OTG_SET_MASK) | 
             ((~OTGCF_FORCE_HOST << OTGCB_CLR) & OTGCF_OTG_CLR_MASK);
    WRITEREG(ISP_OTGCTRL, tmpval);

    KPRINTF(100, ("OTG Control = %04lx\n", READREG(ISP_OTGCTRL) & 0xffff));
    KPRINTF(100, ("OTG Status  = %04lx\n", READREG(ISP_OTGSTATUS) & 0xffff));

    // clear PTDs (ISO, INT and ATLs) and memory
    for(cnt = MMAP_ISO_PTDS; cnt < MMAP_END; cnt++)
    {
        WRITEMEM(cnt, 0);
    }

    WRITEREG(EHCI_USBCMD, EHCF_HCRESET);
    uhwDelayMS(50, unit, base);

    // enable PTDs
    WRITEREG(ISP_ISOPTDSKIPMAP, 0xffffffff);
    WRITEREG(ISP_INTPTDSKIPMAP, 0xffffffff);
    WRITEREG(ISP_ATLPTDSKIPMAP, 0xffffffff);
    WRITEREG(ISP_ISOPTDDONEMAP, 0x00000000);
    WRITEREG(ISP_INTPTDDONEMAP, 0x00000000);
    WRITEREG(ISP_ATLPTDDONEMAP, 0x00000000);
    WRITEREG(ISP_ISOPTDLASTPTD, 0x00000001);
    WRITEREG(ISP_INTPTDLASTPTD, 0x00000001);
    WRITEREG(ISP_ATLPTDLASTPTD, 0x00000001);

    unit->hu_IntMask = IINTF_ATL_DONE|IINTF_INT_DONE|IINTF_ISO_DONE;//|IINTF_SOF;

    unit->hu_ATLDone = 0;
    unit->hu_IntDone = 0;
    unit->hu_IsoDone = 0;
    unit->hu_IsoRTDone = 0;
    unit->hu_IsoRTMask = 0;

    unit->hu_LastATL = 0;
    unit->hu_LastInt = 0;
    unit->hu_LastIso = 0;

    /* clear all interrupts */
    WRITEREG(EHCI_USBSTS, 0);
    WRITEREG(EHCI_USBINTR, 0);
    WRITEREG(ISP_INTR, IINTM_ALL);
    WRITEREG(ISP_INTEN, unit->hu_IntMask);
    WRITEREG(EHCI_USBINTR, EHSF_TDDONE|EHSF_PORTCHANGED|EHSF_FRAMECOUNTOVER);

    WRITEREG(ISP_ISOIRQMASKOR, 0xffffffff);
    WRITEREG(ISP_INTIRQMASKOR, 0xffffffff);
    WRITEREG(ISP_ATLIRQMASKOR, 0xffffffff);
    WRITEREG(ISP_ISOIRQMASKAND, 0x00000000);
    WRITEREG(ISP_INTIRQMASKAND, 0x00000000);
    WRITEREG(ISP_ATLIRQMASKAND, 0x00000000);

    WRITEREG(ISP_BUFFERSTATUS, IBSF_ATL_ENABLE|IBSF_INT_ENABLE|IBSF_ISO_ENABLE);

    WRITEREG(EHCI_USBCMD, EHCF_RUNSTOP|(8<<EHCS_INTTHRESHOLD));
    uhwDelayMS(10, unit, base);
    if(!(READREG(EHCI_USBCMD) & EHCF_RUNSTOP))
    {
        KPRINTF(20, ("ERROR: RunStop bit not set!\n"));
    }
    WRITEREG(EHCI_CONFIGFLAG, EHCF_CONFIGURED);
    uhwDelayMS(1, unit, base);
    if(!(READREG(EHCI_CONFIGFLAG) & EHCF_CONFIGURED))
    {
        KPRINTF(20, ("ERROR: Configured bit not set!\n"));
    }

    // reset root hub
    WRITEREG(EHCI_PORTSC1, EHPF_PORTPOWER);
    timeout = 1000;
    do
    {
        uhwDelayMS(1, unit, base);
        tmpval = READREG(EHCI_PORTSC1);
        if(!--timeout)
        {
            KPRINTF(10, ("HOST ERROR: Port not coming online\n"));
            return UHIOERR_HOSTERROR;
        }
    } while(!(tmpval & (EHPF_CONNECTCHANGE|EHPF_PORTCONNECTED)));
    KPRINTF(10, ("Port Status after Power after %ldms: %08lx\n", 1000-timeout, tmpval));
    ORREG(EHCI_PORTSC1, EHPF_PORTRESET);
    uhwDelayMS(50, unit, base);
    ANDREG(EHCI_PORTSC1, ~(EHPF_PORTRESET|EHPF_CONNECTCHANGE));
    timeout = 1000;
    do
    {
        uhwDelayMS(1, unit, base);
        tmpval = READREG(EHCI_PORTSC1);
        if(!--timeout)
        {
            KPRINTF(20, ("ERROR: Port not correctly enabled (%08lx)!\n", tmpval));
            return UHIOERR_HOSTERROR;
        }
    } while((tmpval & (EHPF_PORTPOWER|EHPF_PORTRESET|EHPF_PORTENABLE)) != (EHPF_PORTPOWER|EHPF_PORTENABLE));

    KPRINTF(10, ("Port enabled after %ldms: %08lx\n", 1000-timeout, tmpval));

    WRITEREG(ISP_HWMODECTRL, IHWCF_DENEB|IHWCF_GLOBAL_INT_EN);

    intpend = READREG(ISP_INTR);

    KPRINTF(1, ("Pre Int enable %08lx\n", intpend));
#ifndef ZORRO_II
#ifndef NODMA
    WRITEMACH(MACH_DMAINT, MIF_ISPINT|MIF_DMAINT);
    if(unit->hu_Buster9)
    {
        WRITEMACH(MACH_DMACTRL, (1<<MXS_TICKS)|(255<<MXS_QUANTUM));
    } else {
        WRITEMACH(MACH_DMACTRL, (1<<MXS_TICKS)|(255<<MXS_QUANTUM)|MXF_BUSTER11);
    }
    WRITEMACH(MACH_CTRL, MCF_INTEN|MCF_DMAINTEN);
#else
    WRITEMACH(MACH_DMAINT, MIF_ISPINT);
    WRITEMACH(MACH_CTRL, MCF_INTEN);
#endif
#else
    WRITEMACH(MACH_CTRL_Z2, MCF_INTEN);
#endif
    KPRINTF(1, ("Post Int enable\n"));

    unit->hu_FrameCounter = 1;
    return 0;
}
/* \\\ */

/* /// "Open_Unit()" */
struct Unit *Open_Unit(struct IOUsbHWReq *ioreq,
                       LONG unitnr,
                       struct DenebDevice *base)
{
    struct ConfigDev *confdev = NULL;
    LONG uno = 0;
    //ULONG loopcnt;
    struct DenebUnit *unit = NULL;
    BOOL selftestokay = FALSE;
    UWORD cnt;
#ifdef ZORRO_II
    volatile UWORD deadbeef;
#endif

#ifdef DEBUG
#define CHECKLONGOFFSET(x) if(offsetof(struct DenebUnit, x) & 15) KPRINTF(300, ("Offset of " #x " is %lx\n", offsetof(struct DenebUnit, x)));
    CHECKLONGOFFSET(hu_HWBase)
    CHECKLONGOFFSET(hu_ATLDone)
    CHECKLONGOFFSET(hu_LargeAreaFreeMap)
    CHECKLONGOFFSET(hu_ATLPTDs)
    CHECKLONGOFFSET(hu_DMATaskStack)
#endif

    if((unitnr < 0) || (unitnr > MAXUNITS))
    {
        KPRINTF(50, ("Unit number %ld out of range!\n", unitnr));
        return NULL;
    }
    if(base->hd_Units[unitnr])
    {
        ioreq->iouh_Req.io_Error = IOERR_UNITBUSY;
        KPRINTF(50, ("Unit %ld already open!\n", unitnr));
        return NULL;
    }


    while(confdev = FindConfigDev(confdev, 5060, 0x10)) /* Replay: 5060, USB/ETH 0x10 */
    {
        KPRINTF(100, ("Found Replay USB board at %08lx...\n", confdev->cd_BoardAddr));
        if(unitnr == uno++)
            break;
    }
    if(!confdev)
    {
        KPRINTF(50, ("No board found!\n"));
        return(NULL);
    }
    if(confdev->cd_Driver)
    {
        ioreq->iouh_Req.io_Error = IOERR_UNITBUSY;
        KPRINTF(50, ("Other driver uses this board!\n"));
        return(NULL);
    }
    confdev->cd_Driver = (APTR) base;
    base->hd_Units[unitnr] = unit = AllocVec(sizeof(struct DenebUnit), MEMF_PUBLIC|MEMF_CLEAR);
    if(unit)
    {
        unit->hu_ConfigDev = confdev;
        unit->hu_Device = base;
        unit->hu_UnitNo = unitnr;
        unit->hu_HWBase = confdev->cd_BoardAddr;
#ifndef ZORRO_II
        // detect flakey 030
        if((SysBase->AttnFlags & (AFF_68030|AFF_68040) == AFF_68030))
        {
            unit->hu_FastZorro2 = TRUE; // reuse this variable
            MC030FREEZE;
        }

        unit->hu_Buster9 = FindResident("Buster 9 ID Tag") ? TRUE : FALSE;

        if(!unit->hu_Buster9)
        {
            struct Library *DOSBase;
            if(DOSBase = OpenLibrary("dos.library", 37))
            {
                UBYTE buffer[4];
                LONG len;
                len = GetVar("Buster9", buffer, 4, 0);
                if(len > 0)
                {
                    if(*buffer != '0')
                    {
                        unit->hu_Buster9 = TRUE;
                    }
                }
                CloseLibrary(DOSBase);
            }
        }

        unit->hu_RegBase = unit->hu_HWBase + MACH_USBREGS;
#else
        unit->hu_RegBase = unit->hu_HWBase;
        deadbeef = READMACH(MACH_FASTZORROMAGIC);
        deadbeef = READMACH(MACH_FASTZORROMAGIC);
        deadbeef = READMACH(MACH_FASTZORROMAGIC);
        unit->hu_FastZorro2 = (deadbeef == 0xbeef) ? TRUE : FALSE;
#endif
        unit->hu_ReadBasePTDs = &unit->hu_RegBase[MMAP_ISO_PTDS];
        unit->hu_ReadBaseMem = &unit->hu_RegBase[MMAP_PAYLOAD];
        if(uhwOpenTimer(unit, base))
        {
            WRITEREG(ISP_SWRESET, ISWRF_RESET_HC);
            uhwDelayMS(5, unit, base);
            WRITEREG(ISP_SWRESET, 0);
            uhwDelayMS(50, unit, base);
            KPRINTF(100, ("HWMODECTRL = %08lx\n", READREG(ISP_HWMODECTRL)));
            WRITEREG(ISP_HWMODECTRL, IHWCF_DENEB);
            KPRINTF(100, ("Chip ID = %08lx\n", READREG(ISP_CHIPID)));
            do
            {
                if(READREG(ISP_CHIPID) != 0x00011761)
                {
                    KPRINTF(20, ("Chip ID failed!\n"));
                    break;
                }
                WRITEREG(ISP_SCRATCH, 0xdeadbeef);
                if(READREG(ISP_SCRATCH) != 0xdeadbeef)
                {
                    KPRINTF(20, ("Scratch W/R failed!\n"));
                    break;
                }
                selftestokay = TRUE;
            } while(FALSE);

            if(selftestokay)
            {
                struct Task *nt = &unit->hu_DMATask;
                /* Do some structure inits */
                NewMinList(&unit->hu_RHIOQueue);
                NewMinList(&unit->hu_IntXFerQueue);
                NewMinList(&unit->hu_CtrlXFerQueue);
                NewMinList(&unit->hu_IsoXFerQueue);
                NewMinList(&unit->hu_BulkXFerQueue);
                NewMinList(&unit->hu_RTIsoHandlers);
                NewMinList(&unit->hu_FreeRTIsoNodes);
                for(cnt = 0; cnt < 16; cnt++)
                {
                    AddTail((struct List *) &unit->hu_FreeRTIsoNodes, (struct Node *) &unit->hu_RTIsoNodes[cnt].rtn_Node);
                }

                unit->hu_LargeAreaFreeMap = 0x0001ffff; // 25 free 1KB pages
                unit->hu_ATLFree = 0xffffffff; // all free
                unit->hu_IntFree = 0xffffffff; // all free
                unit->hu_IsoFree = 0xffffffff; // all free

                unit->hu_ATLBusy = 0x00000000; // none active
                unit->hu_IntBusy = 0x00000000; // none active
                unit->hu_IsoBusy = 0x00000000; // none active

                unit->hu_Level6Int.is_Node.ln_Type = NT_INTERRUPT;
                unit->hu_Level6Int.is_Node.ln_Name = "DenebUSB Rocks";
#ifndef ZORRO_II
                unit->hu_Level6Int.is_Node.ln_Pri  = 127;
#else
                unit->hu_Level6Int.is_Node.ln_Pri  = 115;
#endif
                unit->hu_Level6Int.is_Data = unit;
                unit->hu_Level6Int.is_Code = (void (*)(void)) &uhwLevel6Int;

                unit->hu_SoftInt.is_Node.ln_Type = NT_INTERRUPT;
                unit->hu_SoftInt.is_Node.ln_Name = "DenebUSB Control";
                unit->hu_SoftInt.is_Node.ln_Pri  = 0;
                unit->hu_SoftInt.is_Data = unit;
                unit->hu_SoftInt.is_Code = (void (*)(void)) &uhwSoftInt;

                unit->hu_NakTimeoutInt.is_Node.ln_Type = NT_INTERRUPT;
                unit->hu_NakTimeoutInt.is_Node.ln_Name = "DenebUSB NakTimeout";
                unit->hu_NakTimeoutInt.is_Node.ln_Pri  = -16;
                unit->hu_NakTimeoutInt.is_Data = unit;
                unit->hu_NakTimeoutInt.is_Code = (void (*)(void)) &uhwNakTimeoutInt;

                CopyMem(unit->hu_TimerReq, (APTR) &unit->hu_NakTimeoutReq, sizeof(struct timerequest));
                unit->hu_NakTimeoutReq.tr_node.io_Message.mn_ReplyPort = &unit->hu_NakTimeoutMsgPort;
                unit->hu_NakTimeoutMsgPort.mp_Node.ln_Type = NT_MSGPORT;
                unit->hu_NakTimeoutMsgPort.mp_Flags = PA_SOFTINT;
                unit->hu_NakTimeoutMsgPort.mp_SigTask = &unit->hu_NakTimeoutInt;
                NewMinList((struct MinList *)&unit->hu_NakTimeoutMsgPort.mp_MsgList);
                Cause(&unit->hu_NakTimeoutInt);

                KPRINTF(1, ("Adding Interrupt Handler!\n"));
                AddIntServer(INTB_EXTER, &unit->hu_Level6Int);
                uhwHWInit(unit);

#ifndef NODMA
                nt->tc_Node.ln_Name = "DenebUSB DMA Handler";
#else
                nt->tc_Node.ln_Name = "DenebUSB PIO Transfer";
#endif
                nt->tc_Node.ln_Type = NT_TASK;
                nt->tc_Node.ln_Pri = 12;
                nt->tc_SPLower = unit->hu_DMATaskStack;
                nt->tc_SPUpper = nt->tc_SPReg = (APTR) ((ULONG) nt->tc_SPLower + 512);
                nt->tc_UserData = unit;
                NewMinList((struct MinList *)&nt->tc_MemEntry);
#ifndef __MORPHOS__
                AddTask(nt, (APTR) uhwDMATask, NULL);
#else
                AddTask(nt, (APTR) &uhwDMATask, NULL);
#endif
                //uhwTestStuff(ioreq, unit, base);
                MC030UNFREEZE;
                return(struct Unit *)(unit);
            } else {
                ioreq->iouh_Req.io_Error = IOERR_SELFTEST;
                KPRINTF(5, ("Hardware failure, Magic number failed!"));
            }
            uhwCloseTimer(unit, base);
        }
        MC030UNFREEZE;
        FreeVec(unit);
        base->hd_Units[unitnr] = NULL;
    } else {
        KPRINTF(5, ("No unit memory left!\n"));
    }
    confdev->cd_Driver = NULL;
    return(NULL);
}
/* \\\ */

/* /// "Close_Unit()" */
void Close_Unit(struct DenebDevice *base,
                struct DenebUnit *unit,
                struct IOUsbHWReq *ioreq)
{
    /* Disable all interrupts */
#ifndef ZORRO_II
    WRITEMACH(MACH_CTRL, 0);
#else
    WRITEMACH(MACH_CTRL_Z2, 0);
#endif
    WRITEREG(EHCI_USBCMD, 0);
    WRITEREG(ISP_HWMODECTRL, IHWCF_DENEB);
    unit->hu_NakTimeoutMsgPort.mp_Flags = PA_IGNORE;
    unit->hu_NakTimeoutInt.is_Node.ln_Type = NT_SOFTINT;
    AbortIO((struct IORequest *)&unit->hu_NakTimeoutReq);
    uhwDelayMS(10, unit, base);
    RemIntServer(INTB_EXTER, &unit->hu_Level6Int);
    Signal(&unit->hu_DMATask, SIGBREAKF_CTRL_C);
    uhwDelayMS(10, unit, base);
    /* disable chip */
    uhwCloseTimer(unit, base);
    base->hd_Units[unit->hu_UnitNo] = NULL;
    unit->hu_ConfigDev->cd_Driver = NULL;
    FreeVec(unit);
}
/* \\\ */

/* /// "uhwGetUsbState()" */
UWORD uhwGetUsbState(struct IOUsbHWReq *ioreq,
                     struct DenebUnit *unit,
                     struct DenebDevice *base)
{
    UWORD state = 0;
    ULONG cr = READREG(EHCI_PORTSC1);
    if(cr & EHPF_PORTCONNECTED)
    {
        state = UHSF_OPERATIONAL;
    }
    if(cr & EHPF_RESUMEDTX)
    {
        state |= UHSF_RESUMING;
    }
    if(cr & EHPF_PORTSUSPENDED)
    {
        state |= UHSF_SUSPENDED;
    }
    if(cr & EHPF_PORTRESET)
    {
        state |= UHSF_RESET;
    }
    return(ioreq->iouh_State = state);
}
/* \\\ */

/* /// "cmdReset()" */
/*
 *======================================================================
 * cmdReset(ioreq, unit, base)
 *======================================================================
 *
 * This is the device CMD_RESET routine.
 *
 * Resets the whole USB hardware. Goes into USBOperational mode right
 * after. Must NOT be called from an interrupt.
 *
 */

WORD cmdReset(struct IOUsbHWReq *ioreq,
              struct DenebUnit *unit,
              struct DenebDevice *base)
{
    KPRINTF(10, ("CMD_RESET ioreq: 0x%08lx\n", ioreq));
    uhwHWInit(unit);

    unit->hu_FrameCounter = 1;

    uhwGetUsbState(ioreq, unit, base);

    if(ioreq->iouh_State & UHSF_OPERATIONAL)
    {
        return RC_OK;
    }
    return UHIOERR_USBOFFLINE;
}
/* \\\ */

/* /// "cmdUsbReset()" */
/*
 *======================================================================
 * cmdUsbReset(ioreq, unit, base)
 *======================================================================
 *
 * This is the device UHCMD_USBRESET routine.
 *
 * Resets the USB bus. Goes into USBOperational mode right after. Must
 * NOT be called from an interrupt.
 *
 */

WORD cmdUsbReset(struct IOUsbHWReq *ioreq,
                 struct DenebUnit *unit,
                 struct DenebDevice *base)
{
    KPRINTF(10, ("UHCMD_USBRESET ioreq: 0x%08lx\n", ioreq));

    //uhwHWInit(unit);

    uhwGetUsbState(ioreq, unit, base);

    if(ioreq->iouh_State & UHSF_OPERATIONAL)
    {
        return RC_OK;
    }
    return UHIOERR_USBOFFLINE;
}
/* \\\ */

/* /// "cmdUsbResume()" */
/*
 *======================================================================
 * cmdUsbResume(ioreq, unit, base)
 *======================================================================
 *
 * This is the device UHCMD_USBRESUME routine.
 *
 * Tries to resume from USBSuspend mode into USBOperational.
 * Must NOT be called from an interrupt.
 *
 */

WORD cmdUsbResume(struct IOUsbHWReq *ioreq,
                  struct DenebUnit *unit,
                  struct DenebDevice *base)
{
    KPRINTF(10, ("UHCMD_USBRESUME ioreq: 0x%08lx\n", ioreq));

    /* FIXME */
    uhwGetUsbState(ioreq, unit, base);
    if(ioreq->iouh_State & UHSF_OPERATIONAL)
    {
        return RC_OK;
    }
    return UHIOERR_USBOFFLINE;
}
/* \\\ */

/* /// "cmdUsbSuspend()" */
/*
 *======================================================================
 * cmdUsbSuspend(ioreq, unit, base)
 *======================================================================
 *
 * This is the device UHCMD_USBSUSPEND routine.
 *
 * Sets the USB into USBSuspend mode.
 * Must NOT be called from an interrupt.
 *
 */

WORD cmdUsbSuspend(struct IOUsbHWReq *ioreq,
                   struct DenebUnit *unit,
                   struct DenebDevice *base)
{
    KPRINTF(10, ("UHCMD_USBSUSPEND ioreq: 0x%08lx\n", ioreq));

    /* FIXME */
    uhwGetUsbState(ioreq, unit, base);
    if(ioreq->iouh_State & UHSF_SUSPENDED)
    {
        return RC_OK;
    }
    return UHIOERR_USBOFFLINE;
}
/* \\\ */

/* /// "cmdUsbOper()" */
/*
 *======================================================================
 * cmdUsbOper(ioreq, unit, base)
 *======================================================================
 *
 * This is the device UHCMD_USBOPER routine.
 *
 * Sets the USB into USBOperational mode.
 * Must NOT be called from an interrupt.
 *
 */

WORD cmdUsbOper(struct IOUsbHWReq *ioreq,
                struct DenebUnit *unit,
                struct DenebDevice *base)
{
    KPRINTF(10, ("UHCMD_USBOPER ioreq: 0x%08lx\n", ioreq));

    /* FIXME */
    uhwGetUsbState(ioreq, unit, base);
    if(ioreq->iouh_State & UHSF_OPERATIONAL)
    {
        return RC_OK;
    }
    return UHIOERR_USBOFFLINE;
}
/* \\\ */

/* /// "cmdQueryDevice()" */
/*
 *======================================================================
 * cmdQueryDevice(ioreq, unit, base)
 *======================================================================
 *
 * This is the device UHCMD_QUERYDEVICE routine.
 *
 * Returns information about the hardware.
 *
 */

WORD cmdQueryDevice(struct IOUsbHWReq *ioreq,
                    struct DenebUnit *unit,
                    struct DenebDevice *base)
{
    struct TagItem *taglist = (struct TagItem *) ioreq->iouh_Data;
    struct TagItem *tag;
    ULONG count = 0;

    KPRINTF(10, ("UHCMD_QUERYDEVICE ioreq: 0x%08lx, taglist: 0x%08lx\n", ioreq, taglist));

    if(tag = FindTagItem(UHA_State, taglist))
    {
        *((ULONG *) tag->ti_Data) = (ULONG) uhwGetUsbState(ioreq, unit, base);
        count++;
    }
    if(tag = FindTagItem(UHA_Manufacturer, taglist))
    {
        *((STRPTR *) tag->ti_Data) = "Chris Hodges; FPGAArcade Replay adaptations by Erik Hemming";
        count++;
    }
    if(tag = FindTagItem(UHA_ProductName, taglist))
    {
#ifndef __MORPHOS__
        *((STRPTR *) tag->ti_Data) = "DenebUSB EHCI USB Host Controller";
#else
        *((STRPTR *) tag->ti_Data) = "DenebUSB EHCI USB Host Controller (MorphOS)";
#endif
        count++;
    }
    if(tag = FindTagItem(UHA_Description, taglist))
    {
#ifndef ZORRO_II
#ifdef NODMA
        *((STRPTR *) tag->ti_Data) = (/*unit->hu_FastZorro2 ? "USB 2.0 Highspeed Z3 PIO (030Kludge) driver for Deneb cards" :*/ "USB 2.0 Highspeed Z3 PIO driver for Deneb cards");
#else
        if(unit->hu_FastZorro2)
        {
            *((STRPTR *) tag->ti_Data) = (unit->hu_Buster9 ? "USB 2.0 Highspeed Z3 DMA (030Kludge/Buster9) driver for Deneb cards" : "USB 2.0 Highspeed Z3 DMA (030Kludge) driver for Deneb cards");
        } else {
            *((STRPTR *) tag->ti_Data) = (unit->hu_Buster9 ? "USB 2.0 Highspeed Z3 DMA (Buster9) driver for Deneb cards" : "USB 2.0 Highspeed Z3 DMA driver for Deneb cards");
        }
#endif
#else
        *((STRPTR *) tag->ti_Data) = (unit->hu_FastZorro2 ? "USB 2.0 Highspeed Fast-Zorro II driver for Deneb cards" : "USB 2.0 Highspeed Zorro II driver for Deneb cards");
#endif
        count++;
    }
    if(tag = FindTagItem(UHA_Copyright, taglist))
    {
        *((STRPTR *) tag->ti_Data) = "2007-2014 Chris Hodges";
        count++;
    }
#if (0)
    if(tag = FindTagItem(UHA_Version, taglist))
    {
        *((ULONG *) tag->ti_Data) = VERSION;
        count++;
    }
    if(tag = FindTagItem(UHA_Revision, taglist))
    {
        *((ULONG *) tag->ti_Data) = REVISION;
        count++;
    }
#endif
    if(tag = FindTagItem(UHA_DriverVersion, taglist))
    {
        *((ULONG *) tag->ti_Data) = 0x200;
        count++;
    }
    if(tag = FindTagItem(UHA_Capabilities, taglist))
    {
        *((ULONG *) tag->ti_Data) = UHCF_USB20|UHCF_ISO|UHCF_RT_ISO|UHCF_QUICKIO;
        count++;
    }
    ioreq->iouh_Actual = count;
    return RC_OK;
}
/* \\\ */

/* /// "cmdControlXFer()" */
/*
 *======================================================================
 * cmdControlXFer(ioreq, unit, base)
 *======================================================================
 *
 * This is the device UHCMD_CONTROLXFER routine.
 *
 * First it check if the usb is in proper state and if user passed arguments
 * are valid. If everything is ok, the request is linked to queue of
 * pending transfer requests.
 *
 */

WORD cmdControlXFer(struct IOUsbHWReq *ioreq,
                    struct DenebUnit *unit,
                    struct DenebDevice *base)
{
    KPRINTF(10, ("UHCMD_CONTROLXFER ioreq: 0x%08lx (Dev=%ld, EP=%ld, Flags=%04lx)\n", ioreq, ioreq->iouh_DevAddr, ioreq->iouh_Endpoint, ioreq->iouh_Flags));
    //uhwDelayMS(1000, unit, base); /* Wait 200 ms */
    uhwGetUsbState(ioreq, unit, base);
    if(!(ioreq->iouh_State & UHSF_OPERATIONAL))
    {
        return(UHIOERR_USBOFFLINE);
    }

    ioreq->iouh_Req.io_Flags &= ~IOF_QUICK;
    ioreq->iouh_Actual = 0;

    Disable();
    AddTail((struct List *) &unit->hu_CtrlXFerQueue, (struct Node*)ioreq);
    Enable();
    Cause(&unit->hu_SoftInt);

    KPRINTF(10, ("UHCMD_CONTROLXFER processed ioreq: 0x%08lx\n", ioreq));
    return(RC_DONTREPLY);
}
/* \\\ */

/* /// "cmdBulkXFer()" */
/*
 *======================================================================
 * cmdBulkXFer(ioreq, unit, base)
 *======================================================================
 *
 * This is the device UHCMD_BULKXFER routine.
 *
 * First it check if the usb is in proper state and if user passed arguments
 * are valid. If everything is ok, the request is linked to queue of
 * pending transfer requests.
 *
 */

WORD cmdBulkXFer(struct IOUsbHWReq *ioreq,
                 struct DenebUnit *unit,
                 struct DenebDevice *base)
{
    KPRINTF(10, ("UHCMD_BULKXFER ioreq: 0x%08lx (Dev=%ld, EP=%ld, Flags=%04lx)\n", ioreq, ioreq->iouh_DevAddr, ioreq->iouh_Endpoint, ioreq->iouh_Flags));
    uhwGetUsbState(ioreq, unit, base);
    if(!(ioreq->iouh_State & UHSF_OPERATIONAL))
    {
        return(UHIOERR_USBOFFLINE);
    }

    if(ioreq->iouh_Flags & UHFF_LOWSPEED)
    {
        return(UHIOERR_BADPARAMS);
    }
    ioreq->iouh_Req.io_Flags &= ~IOF_QUICK;
    ioreq->iouh_Actual = 0;

    Disable();
    AddTail((struct List *) &unit->hu_BulkXFerQueue, (struct Node*)ioreq);
    Enable();
    Cause(&unit->hu_SoftInt);

    KPRINTF(10, ("UHCMD_BULKXFER processed ioreq: 0x%08lx\n", ioreq));
    return(RC_DONTREPLY);
}
/* \\\ */

/* /// "cmdIsoXFer()" */
/*
 *======================================================================
 * cmdIsoXFer(ioreq, unit, base)
 *======================================================================
 *
 * This is the device UHCMD_ISOXFER routine.
 *
 * First it check if the usb is in proper state and if user passed arguments
 * are valid. If everything is ok, the request is linked to queue of
 * pending transfer requests.
 *
 */

WORD cmdIsoXFer(struct IOUsbHWReq *ioreq,
                 struct DenebUnit *unit,
                 struct DenebDevice *base)
{
    KPRINTF(10, ("UHCMD_ISOXFER ioreq: 0x%08lx\n", ioreq));
    uhwGetUsbState(ioreq, unit, base);
    if(!(ioreq->iouh_State & UHSF_OPERATIONAL))
    {
        return(UHIOERR_USBOFFLINE);
    }

    if(ioreq->iouh_Flags & UHFF_LOWSPEED)
    {
        return(UHIOERR_BADPARAMS);
    }

    ioreq->iouh_Req.io_Flags &= ~IOF_QUICK;
    ioreq->iouh_Actual = 0;
  
    Disable();
    AddTail((struct List *) &unit->hu_IsoXFerQueue, (struct Node*)ioreq);
    Enable();
    Cause(&unit->hu_SoftInt);

    KPRINTF(10, ("UHCMD_ISOXFER processed ioreq: 0x%08lx\n", ioreq));
    //return UHIOERR_HOSTERROR;
    return(RC_DONTREPLY);
}
/* \\\ */

/* /// "cmdIntXFer()" */
/*
 *======================================================================
 * cmdIntXFer(ioreq, unit, base)
 *======================================================================
 *
 * This is the device UHCMD_INTXFER routine.
 *
 * First it check if the usb is in proper state and if user passed arguments
 * are valid. If everything is ok, the request is linked to queue of
 * pending transfer requests.
 *
 */

WORD cmdIntXFer(struct IOUsbHWReq *ioreq,
                 struct DenebUnit *unit,
                 struct DenebDevice *base)
{
    KPRINTF(10, ("UHCMD_INTXFER ioreq: 0x%08lx\n", ioreq));
    //uhwDelayMS(1000, unit, base); /* Wait 200 ms */
    uhwGetUsbState(ioreq, unit, base);
    if(!(ioreq->iouh_State & UHSF_OPERATIONAL))
    {
        return(UHIOERR_USBOFFLINE);
    }

    ioreq->iouh_Req.io_Flags &= ~IOF_QUICK;
    ioreq->iouh_Actual = 0;

    Disable();
    AddTail((struct List *) &unit->hu_IntXFerQueue, (struct Node*)ioreq);
    Enable();
    Cause(&unit->hu_SoftInt);

    KPRINTF(10, ("UHCMD_INTXFER processed ioreq: 0x%08lx\n", ioreq));
    return(RC_DONTREPLY);
}
/* \\\ */

/* /// "cmdFlush()" */
/*
 *======================================================================
 * cmdFlush(ioreq, base)
 *======================================================================
 *
 * This is the device CMD_FLUSH routine.
 *
 * This routine abort all pending transfer requests.
 *
 */

WORD cmdFlush(struct IOUsbHWReq *ioreq,
               struct DenebUnit *unit,
               struct DenebDevice *base)
{
    struct IOUsbHWReq *pioreq;
    UWORD cnt;
    struct PTDNode *ptd;
    struct MinList *lists[5];

    KPRINTF(10, ("CMD_FLUSH ioreq: 0x%08lx\n", ioreq));

    lists[0] = &unit->hu_RHIOQueue;
    lists[1] = &unit->hu_CtrlXFerQueue;
    lists[2] = &unit->hu_IntXFerQueue;
    lists[3] = &unit->hu_IsoXFerQueue;
    lists[4] = &unit->hu_BulkXFerQueue;

    Disable();

    for(cnt = 0; cnt < 5; cnt++)
    {
        pioreq = (struct IOUsbHWReq *) lists[cnt]->mlh_Head;
        while(((struct Node *) pioreq)->ln_Succ)
        {
            Remove((struct Node *)pioreq);
            pioreq->iouh_Req.io_Error = IOERR_ABORTED;
            ReplyMsg((struct Message *)pioreq);
            pioreq = (struct IOUsbHWReq *) lists[cnt]->mlh_Head;
        }
    }

    // scan ATL PTDs
    for(cnt = 0; cnt < 32; cnt++)
    {
        ptd = &unit->hu_ATLPTDs[cnt];
        if(ptd->ptd_IOReq)
        {
            WRITEMEM(MMAP_ATL_PTDS + (cnt<<3), 0); // deactive PTD
            ptd->ptd_IOReq->iouh_Req.io_Error = IOERR_ABORTED;
            FreeATL(ptd, unit);
        }
    }
    // scan INT PTDs
    for(cnt = 0; cnt < 32; cnt++)
    {
        ptd = &unit->hu_IntPTDs[cnt];
        if(ptd->ptd_IOReq)
        {
            WRITEMEM(MMAP_INT_PTDS + (cnt<<3), 0); // deactive PTD
            ptd->ptd_IOReq->iouh_Req.io_Error = IOERR_ABORTED;
            FreeInt(ptd, unit);
        }
    }
    // scan ISO PTDs
    for(cnt = 0; cnt < 32; cnt++)
    {
        ptd = &unit->hu_IsoPTDs[cnt];
        if(ptd->ptd_IOReq)
        {
            WRITEMEM(MMAP_ISO_PTDS + (cnt<<3), 0); // deactive PTD
            ptd->ptd_IOReq->iouh_Req.io_Error = IOERR_ABORTED;
            FreeIso(ptd, unit);
        }
    }
    Enable();
    /* Return success
    */
    return RC_OK;
}
/* \\\ */

/* /// "cmdAddIsoHandler()" */
/*
 *======================================================================
 * cmdAddIsoHandler(ioreq, unit, base)
 *======================================================================
 *
 * This is the device UHCMD_ADDISOHANDLER routine.
 *
 * First it check if the usb is in proper state and if user passed arguments
 * are valid. If everything is ok, the request is linked to the list of
 * realtime iso handlers.
 *
 */

WORD cmdAddIsoHandler(struct IOUsbHWReq *ioreq,
                      struct DenebUnit *unit,
                      struct DenebDevice *base)
{
    struct RTIsoNode *rtn;
    UWORD cnt;
    UWORD ptdnum;
    ULONG mask;
    struct PTDNode *ptd0;
    struct PTDNode *ptd1;
    volatile ULONG *ispptd;
    UWORD memoffset;
    ULONG memmask;
    ULONG memsize;
    ULONG isomaxpktsize;
    ULONG slicesize;

    KPRINTF(10, ("UHCMD_ADDISOHANDLER ioreq: 0x%08lx\n", ioreq));

    if(!ioreq->iouh_Data)
    {
        return(UHIOERR_BADPARAMS);
    }

    ioreq->iouh_Actual = 0;

    Disable();
    // find free PTD
    ptdnum = 0;
    mask = 3;
    while(((unit->hu_IsoFree & mask) != mask))
    {
        mask <<= 1;
        ptdnum++;
    }
    if(ptdnum > 30)
    {
        Enable();
        return(UHIOERR_OUTOFMEMORY);
    }

    // allocate memory
    slicesize = (ioreq->iouh_MaxPktSize+7) & ~0x7;
    memsize = ((slicesize<<1) + MMAP_GRANULE_SIZE - 1)>>MMAP_GRANULE_BITS;
    // find free space in large data area
    memmask = (1<<memsize) - 1;
    cnt = MMAP_TOTAL_GRANULES - memsize;
    memoffset = MMAP_LARGEAREA;
    do
    {
        if((unit->hu_LargeAreaFreeMap & memmask) == memmask)
        {
            // free mem found!
            break;
        }
        memmask <<= 1;
        memoffset += MMAP_GRANULE_SIZE>>2; // longwords!
    } while(--cnt);
    if(!cnt)
    {
        KPRINTF(20, ("Out of memory for iso data %ld bytes\n", slicesize<<1));
        // no memory for setup
        Enable();
        return(UHIOERR_OUTOFMEMORY);
    }
    rtn = (struct RTIsoNode *) RemHead((struct List *) &unit->hu_FreeRTIsoNodes);

    /* copy some variables */
    rtn->rtn_IOReq.iouh_Flags = ioreq->iouh_Flags;
    rtn->rtn_IOReq.iouh_Dir = ioreq->iouh_Dir;
    rtn->rtn_IOReq.iouh_DevAddr = ioreq->iouh_DevAddr;
    rtn->rtn_IOReq.iouh_Endpoint = ioreq->iouh_Endpoint;
    rtn->rtn_IOReq.iouh_MaxPktSize = ioreq->iouh_MaxPktSize;
    rtn->rtn_IOReq.iouh_Interval = ioreq->iouh_Interval;
    rtn->rtn_IOReq.iouh_SplitHubAddr = ioreq->iouh_SplitHubAddr;
    rtn->rtn_IOReq.iouh_SplitHubPort = ioreq->iouh_SplitHubPort;

    rtn->rtn_RTIso = (struct IOUsbHWRTIso *) ioreq->iouh_Data;
    rtn->rtn_PTDs[0] = ptd0 = &unit->hu_IsoPTDs[ptdnum];
    rtn->rtn_PTDs[1] = ptd1 = &unit->hu_IsoPTDs[ptdnum+1];
    ptd0->ptd_Num = ptdnum;
    ptd1->ptd_Num = ptdnum+1;

    ptd1->ptd_RTIsoNode = ptd0->ptd_RTIsoNode = rtn;
    rtn->rtn_RTIso->urti_DriverPrivate1 = rtn; // backlink

    ptd0->ptd_BufStart = memoffset;
    ptd1->ptd_BufStart = memoffset + (slicesize>>2);
    ptd1->ptd_BufLen = ptd0->ptd_BufLen = ioreq->iouh_MaxPktSize;
    ptd1->ptd_AllocMap = ptd0->ptd_AllocMap = memmask;

    // allocate the memory
    unit->hu_LargeAreaFreeMap ^= memmask;

    ptd1->ptd_IOReq = ptd0->ptd_IOReq = NULL;

    // generate PTD
    ptd0->ptd_DW[0] = (ioreq->iouh_MaxPktSize<<QHA0S_TRANSLEN)|((ioreq->iouh_Endpoint & 1)<<QHA0B_ENDPOINT0);
    ptd0->ptd_DW[1] = ((ioreq->iouh_Endpoint>>1)<<QHA1S_ENDPOINT1_3)|(ioreq->iouh_DevAddr<<QHA1S_DEVADDR)|(QHA1F_TOKEN_OUT|QHA1F_TT_ISO);
    if(ioreq->iouh_Flags & UHFF_SPLITTRANS)
    {
        isomaxpktsize = ioreq->iouh_MaxPktSize;
        // only 188 bytes can be set per microframe in the best case
        if(ioreq->iouh_Dir == UHDIR_IN)
        {
            if(isomaxpktsize > 192)
            {
                isomaxpktsize = 192;
            }
            ptd0->ptd_DW[5] = 0xfc<<QHA5S_MUSOFCSPLIT;
            ptd0->ptd_IntSOFMap = 0x0f;
            ptd1->ptd_IntSOFMap = 0x0f;
            ptd0->ptd_DW[0] |= isomaxpktsize<<QHA0S_MAXPKTLEN;
        } else {
            ptd0->ptd_DW[5] = 0x00<<QHA5S_MUSOFCSPLIT;
        }

        KPRINTF(10, ("*** SPLIT TRANSACTION to HubPort %ld at Addr %ld, size=%ld\n", ioreq->iouh_SplitHubPort, ioreq->iouh_SplitHubAddr, isomaxpktsize));
        // full speed and low speed handling
        ptd0->ptd_DW[1] |= QHA1F_SPLITTRANS|(ioreq->iouh_SplitHubPort<<QHA1S_PORTNUMBER)|(ioreq->iouh_SplitHubAddr<<QHA1S_HUBADDRESS);
    } else {
        ptd0->ptd_DW[0] |= ioreq->iouh_MaxPktSize<<QHA0S_MAXPKTLEN;
        ptd0->ptd_DW[5] = 0;
        // obtain right polling interval
        if(ioreq->iouh_Interval < 2) // 0-1 Frames
        {
            ptd0->ptd_IntSOFMap = 0x55;
            ptd1->ptd_IntSOFMap = 0xaa;
        }
        else if(ioreq->iouh_Interval < 4) // 2-3 Frames
        {
            ptd0->ptd_IntSOFMap = 0x11;
            ptd1->ptd_IntSOFMap = 0x44;
        }
        else if(ioreq->iouh_Interval < 8) // 4-7 Frames
        {
            ptd0->ptd_IntSOFMap = 0x02;
            ptd1->ptd_IntSOFMap = 0x20;
        }
        else if(ioreq->iouh_Interval > 511) // 64ms and higher
        {
            ptd0->ptd_IntSOFMap = 0x10;
            ptd1->ptd_IntSOFMap = 0x10;
        }
        else //if(ioreq->iouh_Interval >= 8) // 1-64ms
        {
            ptd0->ptd_IntSOFMap = 0x01;
            ptd1->ptd_IntSOFMap = 0x01;
        }
    }

    if(ioreq->iouh_Dir == UHDIR_IN)
    {
        ptd0->ptd_DW[1] |= QHA1F_TOKEN_IN;
        ptd1->ptd_Type = ptd0->ptd_Type = PTDT_DATAIN;
    } else {
        //ptd->ptd_DW[1] |= QHA1F_TOKEN_OUT;
        ptd1->ptd_Type = ptd0->ptd_Type = PTDT_DATAOUT;
    }
    ptd1->ptd_DW[0] = ptd0->ptd_DW[0];
    ptd1->ptd_DW[1] = ptd0->ptd_DW[1];
    ptd0->ptd_DW[2] = (MMAP2IMAP(ptd0->ptd_BufStart)<<QHA2S_DATAADDR);
    ptd1->ptd_DW[2] = (MMAP2IMAP(ptd1->ptd_BufStart)<<QHA2S_DATAADDR);

    ptd1->ptd_DW[3] = ptd0->ptd_DW[3] = 0;
    ptd0->ptd_DW[4] = ptd0->ptd_IntSOFMap<<QHA4S_MUSOFACTIVE;
    ptd1->ptd_DW[4] = ptd1->ptd_IntSOFMap<<QHA4S_MUSOFACTIVE;
    ptd1->ptd_DW[5] = ptd0->ptd_DW[5];

    // all good, now fire the transfer
    unit->hu_IsoFree &= ~mask;

    // update HW PTD
    ispptd = &unit->hu_RegBase[MMAP_ISO_PTDS + (ptd0->ptd_Num<<3)];
    ispptd[1] = ptd0->ptd_DW[1];
    ispptd[2] = ptd0->ptd_DW[2];
    ispptd[3] = ptd0->ptd_DW[3];
    ispptd[4] = ptd0->ptd_DW[4];
    ispptd[5] = ptd0->ptd_DW[5];
    ispptd[6] = 0;
    ispptd[7] = 0;
    *ispptd = ptd0->ptd_DW[0];

    ispptd = &unit->hu_RegBase[MMAP_ISO_PTDS + (ptd1->ptd_Num<<3)];
    ispptd[1] = ptd1->ptd_DW[1];
    ispptd[2] = ptd1->ptd_DW[2];
    ispptd[3] = ptd1->ptd_DW[3];
    ispptd[4] = ptd1->ptd_DW[4];
    ispptd[5] = ptd1->ptd_DW[5];
    ispptd[6] = 0;
    ispptd[7] = 0;
    *ispptd = ptd1->ptd_DW[0];

    AddTail((struct List *) &unit->hu_RTIsoHandlers, (struct Node *) &rtn->rtn_Node);
    Enable();
    KPRINTF(10, ("UHCMD_ADDISOHANDLER processed ioreq: 0x%08lx\n", ioreq));
    return(RC_OK);
}
/* \\\ */

/* /// "cmdRemIsoHandler()" */
/*
 *======================================================================
 * cmdRemIsoHandler(ioreq, unit, base)
 *======================================================================
 *
 * This is the device UHCMD_REMISOHANDLER routine.
 *
 * Removes a previously added real time ISO handler.
 *
 */

WORD cmdRemIsoHandler(struct IOUsbHWReq *ioreq,
                      struct DenebUnit *unit,
                      struct DenebDevice *base)
{
    struct RTIsoNode *rtn;
    struct PTDNode *ptd;
    volatile ULONG *ispptd;

    KPRINTF(10, ("UHCMD_REMISOHANDLER ioreq: 0x%08lx\n", ioreq));
    Disable();
    rtn = (struct RTIsoNode *) unit->hu_RTIsoHandlers.mlh_Head;
    while(rtn->rtn_Node.mln_Succ)
    {
        if(rtn->rtn_RTIso == ioreq->iouh_Data)
        {
            break;
        }
        rtn = (struct RTIsoNode *) rtn->rtn_Node.mln_Succ;
    }
    if(!rtn->rtn_Node.mln_Succ)
    {
        Enable();
        KPRINTF(200, ("UHCMD_REMISOHANDLER could not find RTIso handler\n", ioreq));
        return(UHIOERR_BADPARAMS);
    }
    Remove((struct Node *) rtn);
    unit->hu_IsoRTMask &= ~((1<<rtn->rtn_PTDs[0]->ptd_Num)|(1<<rtn->rtn_PTDs[1]->ptd_Num));
    unit->hu_IsoBusy &= ~((1<<rtn->rtn_PTDs[0]->ptd_Num)|(1<<rtn->rtn_PTDs[1]->ptd_Num));
    unit->hu_IsoRTDone &= ~((1<<rtn->rtn_PTDs[0]->ptd_Num)|(1<<rtn->rtn_PTDs[1]->ptd_Num));
    rtn->rtn_RTIso->urti_DriverPrivate1 = NULL;
    rtn->rtn_RTIso = NULL;
    rtn->rtn_PTDs[1]->ptd_RTIsoNode = rtn->rtn_PTDs[0]->ptd_RTIsoNode = NULL;
    ptd = rtn->rtn_PTDs[0];
    ispptd = &unit->hu_RegBase[MMAP_ISO_PTDS + (ptd->ptd_Num<<3)];
    ispptd[3] = ptd->ptd_DW[3] = 0;
    ptd = rtn->rtn_PTDs[1];
    ispptd = &unit->hu_RegBase[MMAP_ISO_PTDS + (ptd->ptd_Num<<3)];
    ispptd[3] = ptd->ptd_DW[3] = 0;
    FreeIso(rtn->rtn_PTDs[0], unit);
    FreeIso(rtn->rtn_PTDs[1], unit);
    AddHead((struct List *) &unit->hu_FreeRTIsoNodes, (struct Node *) &rtn->rtn_Node);
    Enable();
    KPRINTF(10, ("UHCMD_REMISOHANDLER processed ioreq: 0x%08lx\n", ioreq));
    return(RC_OK);
}
/* \\\ */

/* /// "cmdStartRTIso()" */
/*
 *======================================================================
 * cmdStartRTIso(ioreq, unit, base)
 *======================================================================
 *
 * This is the device UHCMD_STARTRTISO routine.
 *
 * Enables a previously added realtime iso handler.
 *
 */

WORD cmdStartRTIso(struct IOUsbHWReq *ioreq,
                   struct DenebUnit *unit,
                   struct DenebDevice *base)
{
    struct RTIsoNode *rtn;
    struct PTDNode *ptd;
    struct IOUsbHWBufferReq *ubr;
    struct IOUsbHWRTIso *urti;
    volatile ULONG *ispptd;
    volatile ULONG *memptr;
    ULONG *srcptr;
    UWORD cnt;
    UWORD loopcnt = 2;
    ULONG framecnt;
    ULONG len;
    UWORD isomaxpktsize;
    UWORD toggle;
    KPRINTF(10, ("UHCMD_STARTRTISO ioreq: 0x%08lx\n", ioreq));

    Disable();
    rtn = (struct RTIsoNode *) unit->hu_RTIsoHandlers.mlh_Head;
    while(rtn->rtn_Node.mln_Succ)
    {
        if(rtn->rtn_RTIso == ioreq->iouh_Data)
        {
            break;
        }
        rtn = (struct RTIsoNode *) rtn->rtn_Node.mln_Succ;
    }
    if(!rtn->rtn_Node.mln_Succ)
    {
        Enable();
        return(UHIOERR_BADPARAMS);
    }
    ubr = &rtn->rtn_BufferReq;
    urti = rtn->rtn_RTIso;
    ioreq = &rtn->rtn_IOReq;
    framecnt = READREG(EHCI_FRINDEX);
    unit->hu_FrameCounter = (unit->hu_FrameCounter & ~0x1fff)|(framecnt & 0x1fff);
    framecnt = unit->hu_FrameCounter + (2<<3);
    toggle = rtn->rtn_NextPTD;
    do
    {
        ptd = rtn->rtn_PTDs[toggle];
        ispptd = &unit->hu_RegBase[MMAP_ISO_PTDS + (ptd->ptd_Num<<3)];

        ptd->ptd_DW[2] &= ~QHA2M_MUFRAME;
        if(ioreq->iouh_Flags & UHFF_SPLITTRANS)
        {
            ptd->ptd_NakTimeoutFrame = framecnt>>3;
            ptd->ptd_DW[2] |= (ptd->ptd_NakTimeoutFrame & 0x1f)<<(3+QHA2S_MUFRAME);
            framecnt += ioreq->iouh_Interval<<3;
        } else {
            ptd->ptd_NakTimeoutFrame = framecnt;
            ptd->ptd_DW[2] |= (ptd->ptd_NakTimeoutFrame & (0x1f<<3))<<(QHA2S_MUFRAME);
            framecnt += ioreq->iouh_Interval;
        }
        if(ptd->ptd_Type == PTDT_DATAOUT)
        {
            len = 0;
            if(urti->urti_OutReqHook)
            {
                ubr->ubr_Frame = ptd->ptd_NakTimeoutFrame;
                // Copy buffer to target
                /* FIXME in continuous mode, whenever ubr_Length was not aligned to longword, all kind of havoc can happen */
                memptr = &unit->hu_RegBase[ptd->ptd_BufStart];
                do
                {
                    CallHookPkt(urti->urti_OutReqHook, urti, ubr);
                    KPRINTF(200, ("Hook->%ld\n", ubr->ubr_Length));
                    if(cnt = (ubr->ubr_Length + 3)>>2)
                    {
                        srcptr = (ULONG *) ubr->ubr_Buffer;
                        do
                        {
                            *memptr++ = shuffle(*srcptr++);
                        }
                        while(--cnt);
                        len += ubr->ubr_Length;
                    }
                    if(urti->urti_OutDoneHook)
                    {
                        CallHookPkt(urti->urti_OutDoneHook, urti, ubr);
                    }
                    // automatically advance buffer
                    ubr->ubr_Buffer += ubr->ubr_Length;
                    if(!(ubr->ubr_Flags & UBFF_CONTBUFFER))
                    {
                        break;
                    }
                    // scatter gather rest of buffer
                    ubr->ubr_Flags &= ~UBFF_CONTBUFFER;
                } while(TRUE);
            }
            isomaxpktsize = ioreq->iouh_MaxPktSize;
            if(ioreq->iouh_Flags & UHFF_SPLITTRANS)
            {
                UWORD sbits = 0x01;
                // only 188 bytes can be set per microframe in the best case
                // on OUT, we start on the very first, and continue over the number of 188s packets.
                if(len > 5*188)
                {
                    sbits = 0x3f;
                }
                else if(len > 4*188)
                {
                    sbits = 0x1f;
                }
                else if(len > 3*188)
                {
                    sbits = 0x0f;
                }
                else if(len > 2*188)
                {
                    sbits = 0x07;
                }
                else if(len > 188)
                {
                    sbits = 0x03;
                }
                if(len > 150)
                {
                    isomaxpktsize = 188;
                }
                ptd->ptd_IntSOFMap = sbits;
            }

            ptd->ptd_DW[0] &= ~(QHA0M_TRANSLEN|QHA0M_MAXPKTLEN);
            ptd->ptd_DW[0] |= (len<<QHA0S_TRANSLEN)|(isomaxpktsize<<QHA0S_MAXPKTLEN);

            //WRITEREG(ISP_ISOPTDSKIPMAP, ~unit->hu_IsoBusy);
        }
        ptd->ptd_DW[0] |= QHA0F_VALID;
        // update HW PTD
        ispptd[2] = ptd->ptd_DW[2];
        ispptd[3] = ptd->ptd_DW[3] = QHA3F_ACTIVE;
        ispptd[4] = ptd->ptd_DW[4] = ptd->ptd_IntSOFMap<<QHA4S_MUSOFACTIVE;
        *ispptd = ptd->ptd_DW[0];
        KPRINTF(1, ("SF %ld\n", framecnt));
        unit->hu_IsoRTMask |= (1<<ptd->ptd_Num);
        unit->hu_IsoBusy |= (1<<ptd->ptd_Num);
        unit->hu_IsoRTDone &= ~(1<<ptd->ptd_Num);
        toggle ^= 1;
    } while(--loopcnt);

    WRITEREG(ISP_ISOPTDSKIPMAP, ~unit->hu_IsoBusy);
    KPRINTF(1, ("New IsoSkipMap %08lx\n", READREG(ISP_ISOPTDSKIPMAP)));

    if(ptd->ptd_Num > unit->hu_LastIso)
    {
        // Update Last PTD bit
        unit->hu_LastIso = ptd->ptd_Num;
        WRITEREG(ISP_ISOPTDLASTPTD, 1<<unit->hu_LastIso);
        KPRINTF(1, ("New IsoLastPtd %08lx\n", READREG(ISP_ISOPTDLASTPTD)));
    }
    Enable();
    return(RC_OK);
}
/* \\\ */

/* /// "cmdStopRTIso()" */
/*
 *======================================================================
 * cmdStopRTIso(ioreq, unit, base)
 *======================================================================
 *
 * This is the device UHCMD_STOPRTISO routine.
 *
 * Disables a previously added realtime iso handler.
 *
 */

WORD cmdStopRTIso(struct IOUsbHWReq *ioreq,
                  struct DenebUnit *unit,
                  struct DenebDevice *base)
{
    struct RTIsoNode *rtn;
    struct PTDNode *ptd;
    volatile ULONG *ispptd;
    UWORD cnt;
    KPRINTF(10, ("UHCMD_STOPRTISO ioreq: 0x%08lx\n", ioreq));

    Disable();
    rtn = (struct RTIsoNode *) unit->hu_RTIsoHandlers.mlh_Head;
    while(rtn->rtn_Node.mln_Succ)
    {
        if(rtn->rtn_RTIso == ioreq->iouh_Data)
        {
            break;
        }
        rtn = (struct RTIsoNode *) rtn->rtn_Node.mln_Succ;
    }
    if(!rtn->rtn_Node.mln_Succ)
    {
        Enable();
        return(UHIOERR_BADPARAMS);
    }

    for(cnt = 0; cnt < 2; cnt++)
    {
        ptd = rtn->rtn_PTDs[cnt];
        ispptd = &unit->hu_RegBase[MMAP_ISO_PTDS + (ptd->ptd_Num<<3)];
        ispptd[3] = ptd->ptd_DW[3] = 0;
        unit->hu_IsoRTMask &= ~(1<<ptd->ptd_Num);
        unit->hu_IsoBusy &= ~(1<<ptd->ptd_Num);
        unit->hu_IsoRTDone &= ~(1<<ptd->ptd_Num);
    }
    WRITEREG(ISP_ISOPTDSKIPMAP, ~unit->hu_IsoBusy);

    Enable();
    return(RC_OK);
}
/* \\\ */

/* /// "NSD stuff" */
static
const UWORD NSDSupported[] =
{
    CMD_FLUSH, CMD_RESET,
    UHCMD_QUERYDEVICE, UHCMD_USBRESET,
    UHCMD_USBRESUME, UHCMD_USBSUSPEND,
    UHCMD_USBOPER, UHCMD_CONTROLXFER ,
    UHCMD_ISOXFER, UHCMD_INTXFER,
    UHCMD_BULKXFER,
    UHCMD_ADDISOHANDLER, UHCMD_REMISOHANDLER,
    UHCMD_STARTRTISO, UHCMD_STOPRTISO,
    NSCMD_DEVICEQUERY, 0
};

WORD cmdNSDeviceQuery(struct IOStdReq *ioreq,
                       struct DenebUnit *unit,
                       struct DenebDevice *base)
{
    struct my_NSDeviceQueryResult *query;

    query = (struct my_NSDeviceQueryResult *) ioreq->io_Data;

    KPRINTF(10, ("NSCMD_DEVICEQUERY ioreq: 0x%08lx query: 0x%08lx\n", ioreq, query));

    /* NULL ptr?
       Enough data?
       Valid request?
    */
    if((!query) ||
       (ioreq->io_Length < sizeof(struct my_NSDeviceQueryResult)) ||
       (query->DevQueryFormat != 0) ||
       (query->SizeAvailable != 0))
    {
        /* Return error. This is special handling, since iorequest is only
           guaranteed to be sizeof(struct IOStdReq). If we'd let our
           devBeginIO dispatcher return the error, it would trash some
           memory past end of the iorequest (ios2_WireError field).
         */
         ioreq->io_Error = IOERR_NOCMD;
         TermIO((struct IOUsbHWReq *) ioreq, base);

         /* Don't reply, we already did.
         */
         return RC_DONTREPLY;
    }

    ioreq->io_Actual         = query->SizeAvailable
                             = sizeof(struct my_NSDeviceQueryResult);
    query->DeviceType        = NSDEVTYPE_USBHARDWARE;
    query->DeviceSubType     = 0;
    query->SupportedCommands = NSDSupported;

    /* Return success (note that this will NOT poke ios2_WireError).
    */
    return RC_OK;
}
/* \\\ */

/* /// "TermIO()" */
/*
 *===========================================================
 * TermIO(ioreq, base)
 *===========================================================
 *
 * Return completed ioreq to sender.
 *
 */

void TermIO(struct IOUsbHWReq *ioreq,
            struct DenebDevice *base)
{
    ioreq->iouh_Req.io_Message.mn_Node.ln_Type = NT_FREEMSG;

    /* If not quick I/O, reply the message
    */
    if(!(ioreq->iouh_Req.io_Flags & IOF_QUICK))
    {
        ReplyMsg(&ioreq->iouh_Req.io_Message);
    }
}
/* \\\ */

/* /// "FreeATL()" */
/*
 *===========================================================
 * FreeATL(ptd, base)
 *===========================================================
 *
 * Return completed ioreq to sender.
 *
 */
void FreeATL(struct PTDNode *ptd, struct DenebUnit *unit)
{
    struct DenebDevice *base = unit->hu_Device;
    struct IOUsbHWReq *ioreq = ptd->ptd_IOReq;
    ULONG mask;

    if(ioreq)
    {
        KPRINTF(10, ("%s Ready (Actual/Len = %ld/%ld, IOError=%ld, Dir=%s, toggle=%ld)\n",
                     (ioreq->iouh_Req.io_Command == UHCMD_BULKXFER) ? "Bulk" : "Ctrl",
                     ioreq->iouh_Actual, ioreq->iouh_Length, ioreq->iouh_Req.io_Error,
                     (ioreq->iouh_Dir == UHDIR_IN) ? "In" : "Out",
                     unit->hu_DevDataToggle[(ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0)]));
        unit->hu_DevBusyReq[(ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0)] = NULL;
        ptd->ptd_IOReq = NULL;
        ReplyMsg(&ioreq->iouh_Req.io_Message);
    }
    if(unit->hu_DMAPTD == ptd)
    {
        // free DMA
        unit->hu_DMAPTD = NULL;
#ifndef NODMA
        WRITEMACH(MACH_DMAADDRESS, 0);
        mask = ioreq->iouh_Length;
        CachePostDMA(ioreq->iouh_Data, &mask, (ioreq->iouh_Dir == UHDIR_IN) ? 0 : DMA_ReadFromRAM);
#endif
        //WRITEREG(ISP_DMACONFIG, 0);
    }
    ptd->ptd_Type = PTDT_FREE;
    unit->hu_LargeAreaFreeMap |= ptd->ptd_AllocMap;
    unit->hu_ATLFree |= (1<<ptd->ptd_Num);
    unit->hu_ATLBusy &= ~(1<<ptd->ptd_Num);
    unit->hu_ATLActive &= ~(1<<ptd->ptd_Num);
    unit->hu_ATLNakMap &= ~(1<<ptd->ptd_Num);
    WRITEREG(ISP_ATLPTDSKIPMAP, ~unit->hu_ATLBusy);

    if(ptd->ptd_Num == unit->hu_LastATL)
    {
        // Update Last PTD bit
        unit->hu_LastATL = 0;
        mask = unit->hu_ATLBusy >> 1;
        while(mask)
        {
            mask >>= 1;
            unit->hu_LastATL++;
        }
        WRITEREG(ISP_ATLPTDLASTPTD, 1<<unit->hu_LastATL);
        KPRINTF(1, ("Reduced ATLLastPtd %08lx\n", READREG(ISP_ATLPTDLASTPTD)));
    }
}
/* \\\ */

/* /// "FreeInt()" */
/*
 *===========================================================
 * FreeInt(ptd, base)
 *===========================================================
 *
 * Return completed ioreq to sender.
 *
 */
void FreeInt(struct PTDNode *ptd, struct DenebUnit *unit)
{
    struct DenebDevice *base = unit->hu_Device;
    struct IOUsbHWReq *ioreq = ptd->ptd_IOReq;
    ULONG mask;

    if(ioreq)
    {
        KPRINTF(10, ("Int Ready (Actual/Len = %ld/%ld, IOError=%ld, Dir=%s)\n",
                     ioreq->iouh_Actual, ioreq->iouh_Length, ioreq->iouh_Req.io_Error,
                     (ioreq->iouh_Dir == UHDIR_IN) ? "In" : "Out"));
        unit->hu_DevBusyReq[(ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0)] = NULL;
        ptd->ptd_IOReq = NULL;
        ReplyMsg(&ioreq->iouh_Req.io_Message);
    }
    ptd->ptd_Type = PTDT_FREE;
    unit->hu_LargeAreaFreeMap |= ptd->ptd_AllocMap;
    unit->hu_IntFree |= (1<<ptd->ptd_Num);
    unit->hu_IntBusy &= ~(1<<ptd->ptd_Num);
    unit->hu_IntNakMap &= ~(1<<ptd->ptd_Num);
    WRITEREG(ISP_INTPTDSKIPMAP, ~unit->hu_IntBusy);
    if(ptd->ptd_Num == unit->hu_LastInt)
    {
        // Update Last PTD bit
        unit->hu_LastInt = 0;
        mask = unit->hu_IntBusy >> 1;
        while(mask)
        {
            mask >>= 1;
            unit->hu_LastInt++;
        }
        WRITEREG(ISP_INTPTDLASTPTD, 1<<unit->hu_LastInt);
        KPRINTF(1, ("Reduced IntLastPtd %08lx\n", READREG(ISP_INTPTDLASTPTD)));
    }
}
/* \\\ */

/* /// "FreeIso()" */
/*
 *===========================================================
 * FreeIso(ptd, base)
 *===========================================================
 *
 * Return completed ioreq to sender.
 *
 */
void FreeIso(struct PTDNode *ptd, struct DenebUnit *unit)
{
    struct DenebDevice *base = unit->hu_Device;
    struct IOUsbHWReq *ioreq = ptd->ptd_IOReq;
    ULONG mask;

    if(ioreq)
    {
        KPRINTF(10, ("Iso Ready (Actual/Len = %ld/%ld, IOError=%ld, Dir=%s)\n",
                     ioreq->iouh_Actual, ioreq->iouh_Length, ioreq->iouh_Req.io_Error,
                     (ioreq->iouh_Dir == UHDIR_IN) ? "In" : "Out"));
        unit->hu_DevBusyReq[(ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0)] = NULL;
        ptd->ptd_IOReq = NULL;
        ReplyMsg(&ioreq->iouh_Req.io_Message);
    }
    ptd->ptd_Type = PTDT_FREE;
    unit->hu_LargeAreaFreeMap |= ptd->ptd_AllocMap;
    unit->hu_IsoFree |= (1<<ptd->ptd_Num);
    unit->hu_IsoBusy &= ~(1<<ptd->ptd_Num);
    WRITEREG(ISP_ISOPTDSKIPMAP, ~unit->hu_IsoBusy);
    if(ptd->ptd_Num == unit->hu_LastIso)
    {
        // Update Last PTD bit
        unit->hu_LastIso = 0;
        mask = unit->hu_IsoBusy >> 1;
        while(mask)
        {
            mask >>= 1;
            unit->hu_LastIso++;
        }
        WRITEREG(ISP_ISOPTDLASTPTD, 1<<unit->hu_LastIso);
        KPRINTF(1, ("Reduced IsoLastPtd %08lx\n", READREG(ISP_ISOPTDLASTPTD)));
    }
}
/* \\\ */

/* /// "cmdAbortIO()" */
BOOL cmdAbortIO(struct IOUsbHWReq *ioreq,
                struct DenebDevice *base)
{
    struct DenebUnit *unit = (struct DenebUnit *) ioreq->iouh_Req.io_Unit;
    struct PTDNode *ptd;
    UWORD cnt;
    struct IOUsbHWReq * cmpioreq;
    BOOL foundit = FALSE;
    struct MinList *lists[5];

    KPRINTF(10, ("CMD Abort on %08lx\n", ioreq));

    lists[0] = &unit->hu_RHIOQueue;
    lists[1] = &unit->hu_CtrlXFerQueue;
    lists[2] = &unit->hu_IntXFerQueue;
    lists[3] = &unit->hu_IsoXFerQueue;
    lists[4] = &unit->hu_BulkXFerQueue;

    Disable();
    for(cnt = 0; cnt < 5; cnt++)
    {
        cmpioreq = (struct IOUsbHWReq *) lists[cnt]->mlh_Head;
        while(((struct Node *) cmpioreq)->ln_Succ)
        {
            if(ioreq == cmpioreq)
            {
                foundit = TRUE;
                break;
            }
            cmpioreq = (struct IOUsbHWReq *) cmpioreq->iouh_Req.io_Message.mn_Node.ln_Succ;
        }
        if(foundit)
        {
            break;
        }
    }
    if(foundit)
    {
        Remove((struct Node *)ioreq);
    } else {
        KPRINTF(10, ("not found in waiting queues!"));
        // scan ATL PTDs
        for(cnt = 0; cnt < 32; cnt++)
        {
            ptd = &unit->hu_ATLPTDs[cnt];
            if(ptd->ptd_IOReq == ioreq)
            {
                WRITEMEM(MMAP_ATL_PTDS + (cnt<<3), 0); // deactive PTD
                unit->hu_DevBusyReq[(ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0)] = NULL;
                ptd->ptd_IOReq = NULL;
                FreeATL(ptd, unit);
                foundit = TRUE;
                break;
            }
        }
        if(!foundit)
        {
            // scan INT PTDs
            for(cnt = 0; cnt < 32; cnt++)
            {
                ptd = &unit->hu_IntPTDs[cnt];
                if(ptd->ptd_IOReq == ioreq)
                {
                    WRITEMEM(MMAP_INT_PTDS + (cnt<<3), 0); // deactive PTD
                    unit->hu_DevBusyReq[(ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0)] = NULL;
                    ptd->ptd_IOReq = NULL;
                    FreeInt(ptd, unit);
                    foundit = TRUE;
                    break;
                }
            }
        }
        if(!foundit)
        {
            // scan ISO PTDs
            for(cnt = 0; cnt < 32; cnt++)
            {
                ptd = &unit->hu_IsoPTDs[cnt];
                if(ptd->ptd_IOReq == ioreq)
                {
                    WRITEMEM(MMAP_ISO_PTDS + (cnt<<3), 0); // deactive PTD
                    unit->hu_DevBusyReq[(ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0)] = NULL;
                    ptd->ptd_IOReq = NULL;
                    FreeIso(ptd, unit);
                    foundit = TRUE;
                    break;
                }
            }
        }
    }
    Enable();
    return(foundit);
}
/* \\\ */

/* ---------------------------------------------------------------------- *
 *                    Service Interrupt Routines                          *
 * ---------------------------------------------------------------------- */

#ifdef DEBUG
/* /// "uhwDebugATLs()" */
void uhwDebugATLs(struct DenebUnit *unit)
{
    struct IOUsbHWReq *ioreq;
    UWORD ptdnum;
    struct PTDNode *ptd;
    volatile ULONG *ispptd;
    UWORD devadrep;
    ULONG tmpdw[5];

    KPRINTF(200, ("Free Memory %08lx\n", unit->hu_LargeAreaFreeMap));
    KPRINTF(200, ("ATL Free   %08lx\n", unit->hu_ATLFree));
    KPRINTF(200, ("ATL Busy   %08lx/%08lx\n", unit->hu_ATLBusy, READREG(ISP_ATLPTDSKIPMAP)));
    KPRINTF(200, ("ATL Active %08lx\n", unit->hu_ATLActive));
    KPRINTF(200, ("ATL NakMap %08lx\n", unit->hu_ATLNakMap));
    KPRINTF(200, ("ATL Done   %08lx\n", unit->hu_ATLDone));
    KPRINTF(200, ("ATL Last   %08lx/%ld\n", READREG(ISP_ATLPTDLASTPTD), unit->hu_LastATL));

    KPRINTF(200, ("DMA PTD    %08lx\n", unit->hu_DMAPTD));

    /* *** CTRL and BULK transfers *** */
    ptdnum = 0;
    do
    {
        ptd = &unit->hu_ATLPTDs[ptdnum];
        ioreq = ptd->ptd_IOReq;
        ispptd = &unit->hu_RegBase[MMAP_ATL_PTDS + (ptdnum<<3)];

        WRITEREG(ISP_MEMORY, (MMAP_ATL_PTDS<<2) + (ptdnum<<5));
        tmpdw[0] = *ispptd;
        tmpdw[1] = *ispptd;
        tmpdw[2] = *ispptd;
        tmpdw[3] = *ispptd;
        tmpdw[4] = *ispptd;

        if(tmpdw[0] | tmpdw[1] | tmpdw[2] | tmpdw[3] | tmpdw[4])
        {
            KPRINTF(200, ("PTD %ld (%08lx), IOReq=%08lx\n", ptdnum, ptd, ptd->ptd_IOReq));


            KPRINTF(200, ("0:%08lx 1:%08lx 2:%08lx 3:%08lx 4:%08lx\n",
                          tmpdw[0], tmpdw[1], tmpdw[2], tmpdw[3], tmpdw[4]));

            if(ioreq)
            {
                devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
                KPRINTF(200, ("Dev %04lx, Actual/Length=%ld/%ld, Toggle saved: %ld\n",
                              devadrep,
                              ioreq->iouh_Actual,
                              ioreq->iouh_Length,
                              unit->hu_DevDataToggle[devadrep]));
            }

            KPRINTF(200, ("Valid/Active=%ld/%ld, TransLen=%ld, MaxPkt=%ld\n",
                          (tmpdw[0] & QHA0F_VALID)>>QHA0B_VALID,
                          (tmpdw[3] & QHA3F_ACTIVE)>>QHA3B_ACTIVE,
                          (tmpdw[0] & QHA0M_TRANSLEN)>>QHA0S_TRANSLEN,
                          (tmpdw[0] & QHA0M_MAXPKTLEN)>>QHA0S_MAXPKTLEN));

            KPRINTF(200, ("TransCount=%ld, Cerr=%ld, NakCount=%ld, Toggle=%ld, Ping=%ld, CS=%ld\n",
                          (tmpdw[3] & QHA3M_TRANSCOUNT)>>QHA3S_TRANSCOUNT,
                          (tmpdw[3] & QHA3M_RETRYCOUNT)>>QHA3S_RETRYCOUNT,
                          (tmpdw[3] & QHA3M_NAKCOUNT)>>QHA3S_NAKCOUNT,
                          (tmpdw[3] & QHA3F_DATA1)>>QHA3B_DATA1,
                          (tmpdw[3] & QHA3F_PING)>>QHA3B_PING,
                          (tmpdw[3] & QHA3F_COMPLETESPLIT)>>QHA3B_COMPLETESPLIT));

            //KPRINTF(10, ("DUMP DW[3] = %08lx\n", ptd->ptd_DW[3]));
            if(tmpdw[3] & (QHA3F_PID_ERROR|QHA3F_BABBLE_ERROR|QHA3F_HALT))
            {
                if(tmpdw[3] & QHA3F_BABBLE_ERROR)
                {
                    KPRINTF(200, ("ATL BABBLE ERROR!\n"));
                }
                else if(tmpdw[3] & QHA3F_PID_ERROR)
                {
                    KPRINTF(200, ("ATL PID ERROR\n"));
                } else {
                    KPRINTF(200, ("ATL STALLED!\n"));
                }
            }
        }
        ptdnum++;
    } while(ptdnum< 32);
}
/* \\\ */
#endif

/* /// "uhwHandleFinishedATLs()" */
void uhwHandleFinishedATLs(struct DenebUnit *unit)
{
    struct DenebDevice *base = unit->hu_Device;
    struct IOUsbHWReq *ioreq;
    UWORD cnt;
    UWORD ptdnum;
    ULONG mask;
    struct PTDNode *ptd;
    volatile ULONG *ispptd;
    volatile ULONG *memptr;
    volatile ULONG *rmemptr = unit->hu_ReadBaseMem;
    ULONG *srcptr;
    ULONG tmpval;
    UWORD devadrep;
    BOOL nosetupdata;
    BOOL freeptd;
    ULONG len;

    ULONG busy;
    if(unit->hu_SafeATLCheck)
    {
        unit->hu_SafeATLCheck = FALSE;

        if(busy = unit->hu_ATLActive)
        {
            // this is a workaround for Errata 2 of ISP1760 1 Errata 070108
            ispptd = unit->hu_ReadBasePTDs;
            ptdnum = 0;
            do
            {
                if(busy & 1)
                {
                    ULONG dummy;
                    ispptd = &unit->hu_RegBase[MMAP_ATL_PTDS];
                    WRITEREG(ISP_MEMORY, (MMAP_ATL_PTDS<<2) + (ptdnum<<5));
                    if(!(*ispptd & 1)) // check valid bit 0
                    {
#ifdef DEBUG
                        //uhwDebugATLs(unit);
#endif
                        KPRINTF(100, ("*** Instant: Errata Valid bugfix on PTD %ld\n", ptdnum));
                        WRITEREG(ISP_ATLPTDDONEMAP, 1<<ptdnum);
                        unit->hu_ATLDone |= (1<<ptdnum)|READREG(ISP_ATLPTDDONEMAP);
                        KPRINTF(100, ("(Done=%08lx)\n", unit->hu_ATLDone));
                    } else {
                        dummy = *ispptd;
                        dummy = *ispptd;
                        //WRITEREG(ISP_MEMORY, ((MMAP_ATL_PTDS+3)<<2) + (ptdnum<<5));
                        if(((LONG) *ispptd) > 0) // check active bit 31
                        {
#ifdef DEBUG
                            //uhwDebugATLs(unit);
#endif
                            KPRINTF(100, ("*** Instant: Errata Active bugfix on PTD %ld\n", ptdnum));
                            WRITEREG(ISP_ATLPTDDONEMAP, 1<<ptdnum);
                            unit->hu_ATLDone |= (1<<ptdnum)|READREG(ISP_ATLPTDDONEMAP);
                            KPRINTF(100, ("(Done=%08lx)\n", unit->hu_ATLDone));
                        }
                    }
                }
                busy >>= 1;
                ptdnum++;
            } while(busy);
        }
    }

    /* *** CTRL and BULK transfers *** */
    ptdnum = 0;
    while(unit->hu_ATLDone)
    {
        mask = (1<<ptdnum);
        if(unit->hu_ATLDone & mask)
        {
            KPRINTF(1, ("ATL %ld DONE\n", ptdnum));
            //ULONG tmpdw[4];
            unit->hu_ATLDone &= ~mask;
            ptd = &unit->hu_ATLPTDs[ptdnum];
            ispptd = &unit->hu_RegBase[MMAP_ATL_PTDS + (ptdnum<<3)];
            ioreq = ptd->ptd_IOReq;
            if(!ioreq)
            {
                KPRINTF(20, ("No IOReq for PTD %ld\n", ptdnum));
                continue;
            }
            devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
            freeptd = TRUE;
            nosetupdata = FALSE;

            WRITEREG(ISP_MEMORY, ((MMAP_ATL_PTDS+3)<<2) + (ptdnum<<5));
            ptd->ptd_DW[3] = *ispptd;

            // update data toggle bit
            KPRINTF(1, ("Dev %04lx, Toggle old: %ld, toggle new: %ld, len=%ld\n",
                        devadrep,
                        unit->hu_DevDataToggle[devadrep],
                        (ptd->ptd_DW[3] & QHA3F_DATA1)>>QHA3B_DATA1,
                        (ptd->ptd_DW[3] & QHA3M_TRANSCOUNT)>>QHA3S_TRANSCOUNT));
            unit->hu_DevDataToggle[devadrep] = (ptd->ptd_DW[3] & (QHA3F_DATA1|QHA3F_PING))>>QHA3B_DATA1;

            //KPRINTF(10, ("DUMP DW[3] = %08lx\n", ptd->ptd_DW[3]));
            if((ptd->ptd_DW[3] & QHA3F_PID_ERROR) && (ptd->ptd_DW[3] & (QHA3M_RETRYCOUNT|QHA3M_TRANSCOUNT)))
            {
                KPRINTF(200, ("Workaround: Successful retry!\n"));
                ptd->ptd_DW[3] &= ~QHA3F_PID_ERROR;
            }
            if(ptd->ptd_DW[3] & (QHA3F_PID_ERROR|QHA3F_BABBLE_ERROR|QHA3F_HALT))
            {
                if(ptd->ptd_DW[3] & QHA3F_BABBLE_ERROR)
                {
                    KPRINTF(200, ("ATL BABBLE ERROR!\n"));
                    ioreq->iouh_Req.io_Error = UHIOERR_OVERFLOW;
                }
                else if(ptd->ptd_DW[3] & QHA3F_PID_ERROR)
                {
                    KPRINTF(200, ("ATL PID ERROR\n"));
                    ioreq->iouh_Req.io_Error = UHIOERR_TIMEOUT;
                }
                else //if(ptd->ptd_DW[3] & QHA3F_HALT)
                {
                    KPRINTF(200, ("ATL STALLED!\n"));
                    ioreq->iouh_Req.io_Error = UHIOERR_STALL;
                }
                FreeATL(ptd, unit);
            }
            else if(ptd->ptd_DW[3] & QHA3F_ACTIVE)
            {
                // Restarting transaction due to 15 NAKs
                if((unit->hu_ATLNakMap & mask) && (unit->hu_FrameCounter > ptd->ptd_NakTimeoutFrame))
                {
                    KPRINTF(20, ("Nak timeout %ld > %ld\n", unit->hu_FrameCounter, ptd->ptd_NakTimeoutFrame));
                    ioreq->iouh_Req.io_Error = UHIOERR_NAKTIMEOUT;
                    FreeATL(ptd, unit);
                } else {
#if 0
                    if(ioreq->iouh_Req.io_Command == UHCMD_CONTROLXFER)
                    {
                        KPRINTF(1, ("CTRL NAK\n"));
                        *ispptd = ptd->ptd_DW[0];
                    } else {
                        KPRINTF(100, ("BULK NAK %ld\n", unit->hu_DevDataToggle[devadrep]));
                        *ispptd = ptd->ptd_DW[0];
                        //ioreq->iouh_Req.io_Error = UHIOERR_NAKTIMEOUT;
                        //FreeATL(ptd, unit);
                    }
#else
                    // retry on NAK anyhow
                    /*if(ioreq->iouh_Dir == UHDIR_IN)
                    {
                        KPRINTF(100, ("%ld\n", unit->hu_FrameCounter));
                    }*/
                    ptd->ptd_DW[3] &= ~(QHA3M_RETRYCOUNT|QHA3M_NAKCOUNT);
                    ptd->ptd_DW[3] |= (2<<QHA3S_RETRYCOUNT)|(0<<QHA3S_NAKCOUNT);//QHA3F_PING;
                    ispptd[2] = ptd->ptd_DW[2];
                    ispptd[3] = ptd->ptd_DW[3];
                    *ispptd = ptd->ptd_DW[0];
                    //KPRINTF(100, ("BULK NAK %ld\n", unit->hu_DevDataToggle[devadrep]));
                    unit->hu_ATLActive |= 1<<ptdnum;
#endif
                }
            } else {
                // update nak timeout frame
                if(unit->hu_ATLNakMap & mask)
                {
                    ptd->ptd_NakTimeoutFrame = unit->hu_FrameCounter + (ioreq->iouh_NakTimeout<<3);
                }
                /*if(ioreq->iouh_Flags & UHFF_SPLITTRANS)
                {
                   len = (ptd->ptd_DW[3] & QHA3M_TRANSCOUNT_LS)>>QHA3S_TRANSCOUNT;
                } else {*/
                   len = (ptd->ptd_DW[3] & QHA3M_TRANSCOUNT)>>QHA3S_TRANSCOUNT;
                //}
                switch(ptd->ptd_Type)
                {
                    case PTDT_DATAINDMA:
                        KPRINTF(10, ("DATA IN DMASM=%ld (%ld)\n", unit->hu_DMAStateMachine, len));
                        ioreq->iouh_Actual += len;
                        if(len < ptd->ptd_BufLen)
                        {
                            // short packet
                            KPRINTF(20, ("Short packet %ld < %ld\n", len, ptd->ptd_BufLen));
                            KPRINTF(20, ("Short transfer! Actual (%ld) < Length (%ld)!\n", ioreq->iouh_Actual, ioreq->iouh_Length));
                            if(!(ioreq->iouh_Flags & UHFF_ALLOWRUNTPKTS))
                            {
                                ioreq->iouh_Req.io_Error = UHIOERR_RUNTPACKET;
                            }
                            ptd->ptd_LastIn = TRUE;
                        }
                        else if(ioreq->iouh_Length - ioreq->iouh_Actual < ptd->ptd_BufLen)
                        {
                            ptd->ptd_BufLen = ioreq->iouh_Length - ioreq->iouh_Actual;
                            KPRINTF(10, ("DMA Last one with %ld bytes\n", ptd->ptd_BufLen));
                            ptd->ptd_LastIn = (ioreq->iouh_Actual == ioreq->iouh_Length);
                        }
#ifndef NODMA
                        Disable(); // avoid race conditions
#endif
                        switch(unit->hu_DMAStateMachine)
                        {
                            case DMASM_IN_WAITING_LOW:
                                tmpval = (len + 3) & ~3;
                                if(tmpval >= MMAP_DMATHRES_SIZE)
                                {
                                    if(!ptd->ptd_LastIn)
                                    {
                                        KPRINTF(10, ("Activating HIGH PTD\n"));
                                        // activate PTD
                                        ptd->ptd_DW[0] &= ~QHA0M_TRANSLEN;
                                        ptd->ptd_DW[0] |= QHA0F_VALID|(ptd->ptd_BufLen<<QHA0S_TRANSLEN);
                                        ispptd[2] = ptd->ptd_DW[2] = IMAP_BULKDMA_HIGH<<QHA2S_DATAADDR;
                                        ptd->ptd_DW[3] &= ~(QHA3M_TRANSCOUNT|QHA3M_NAKCOUNT|QHA3M_RETRYCOUNT|QHA3F_COMPLETESPLIT|QHA3F_PID_ERROR|QHA3F_BABBLE_ERROR|QHA3F_HALT);
                                        ptd->ptd_DW[3] |= (2<<QHA3S_RETRYCOUNT)|QHA3F_ACTIVE;
                                        // update HW PTD
                                        ispptd[3] = ptd->ptd_DW[3];
                                        ispptd[4] = ptd->ptd_DW[4];
                                        *ispptd = ptd->ptd_DW[0];
                                        unit->hu_ATLActive |= 1<<ptdnum;
                                    }

                                    KPRINTF(10, ("Starting LOW DMA Transfer %ld bytes to %08lx from %04lx\n", tmpval, unit->hu_DMAAddr, MMAP_BULKDMA_LOW));
                                    unit->hu_DMAStateMachine = DMASM_IN_READING_LOW;
                                    unit->hu_DMALength = tmpval;

#ifndef NODMA
                                    WRITEREG(ISP_MEMORY, ((MMAP_BULKDMA_LOW<<2)|IMSF_BANK3)|(tmpval<<MMS_LENGTHBYTES));
                                    WRITEMACH(MACH_DMAADDRESS, (ULONG) unit->hu_DMAPhyAddr|MAF_ENABLE);
#else
                                    unit->hu_ISPAddr = MMAP_BULKDMA_LOW;
                                    Signal(&unit->hu_DMATask, SIGF_SINGLE); // start copy task
#endif
                                    break;
                                }
                                if(tmpval)
                                {
                                    KPRINTF(10, ("LOW done by PIO to %08lx from %04lx\n", tmpval, unit->hu_DMAAddr, MMAP_BULKDMA_LOW));
#ifndef ZORRO_II
                                    WRITEREG(ISP_MEMORY, MMAP_BULKDMA_LOW<<2);
                                    CopyMemShuffle((APTR) rmemptr, unit->hu_DMAAddr, tmpval);
#else
                                    WRITEREG(ISP_MEMORY, (MMAP_BULKDMA_LOW<<2)|IMSF_BANK1);
                                    CopyMemShuffle((APTR) &rmemptr[MMAP_BULKDMA_LOW], unit->hu_DMAAddr, tmpval);
#endif
                                    unit->hu_DMAOffset += unit->hu_DMALength;
                                    unit->hu_DMAAddr += unit->hu_DMALength;
#ifndef NODMA
                                    unit->hu_DMAPhyAddr += unit->hu_DMALength;
#endif
                                }
                                unit->hu_DMAStateMachine = DMASM_NOP;
                                FreeATL(ptd, unit);
                                break;

                            case DMASM_IN_READING_LOW:
                                KPRINTF(10, ("HIGH PTD done, waiting for LOW DMA to finish\n"));
                                unit->hu_DMAStateMachine = DMASM_IN_DONE_HIGH;
                                break;

                            case DMASM_IN_READING_HIGH:
                                KPRINTF(10, ("LOW PTD done, waiting for HIGH DMA to finish\n"));
                                unit->hu_DMAStateMachine = DMASM_IN_DONE_LOW;
                                break;

                            case DMASM_IN_WAITING_HIGH:
                                tmpval = (len + 3) & ~3;
                                if(tmpval >= MMAP_DMATHRES_SIZE)
                                {
                                    if(!ptd->ptd_LastIn)
                                    {
                                        KPRINTF(10, ("Activating LOW PTD\n"));
                                        // activate PTD
                                        ptd->ptd_DW[0] &= ~QHA0M_TRANSLEN;
                                        ptd->ptd_DW[0] |= QHA0F_VALID|(ptd->ptd_BufLen<<QHA0S_TRANSLEN);
                                        ispptd[2] = ptd->ptd_DW[2] = IMAP_BULKDMA_LOW<<QHA2S_DATAADDR;
                                        ptd->ptd_DW[3] &= ~(QHA3M_TRANSCOUNT|QHA3M_NAKCOUNT|QHA3M_RETRYCOUNT|QHA3F_COMPLETESPLIT|QHA3F_PID_ERROR|QHA3F_BABBLE_ERROR|QHA3F_HALT);
                                        ptd->ptd_DW[3] |= (2<<QHA3S_RETRYCOUNT)|QHA3F_ACTIVE;
                                        // update HW PTD
                                        ispptd[3] = ptd->ptd_DW[3];
                                        ispptd[4] = ptd->ptd_DW[4];
                                        *ispptd = ptd->ptd_DW[0];
                                        unit->hu_ATLActive |= 1<<ptdnum;
                                    }

                                    KPRINTF(10, ("Starting HIGH DMA Transfer %ld bytes to %08lx from %04lx\n", tmpval, unit->hu_DMAAddr, MMAP_BULKDMA_HIGH));
                                    unit->hu_DMAStateMachine = DMASM_IN_READING_HIGH;
                                    unit->hu_DMALength = tmpval;

#ifndef NODMA
                                    WRITEREG(ISP_MEMORY, ((MMAP_BULKDMA_HIGH<<2)|IMSF_BANK3)|(tmpval<<MMS_LENGTHBYTES));
                                    WRITEMACH(MACH_DMAADDRESS, (ULONG) unit->hu_DMAPhyAddr|MAF_ENABLE);
#else
                                    unit->hu_ISPAddr = MMAP_BULKDMA_HIGH;
                                    Signal(&unit->hu_DMATask, SIGF_SINGLE); // start copy task
#endif
                                    break;
                                }
                                if(tmpval)
                                {
                                    KPRINTF(10, ("HIGH done by PIO to %08lx from %04lx\n", tmpval, unit->hu_DMAAddr, MMAP_BULKDMA_HIGH));
#ifndef ZORRO_II
                                    WRITEREG(ISP_MEMORY, MMAP_BULKDMA_HIGH<<2);
                                    CopyMemShuffle((APTR) rmemptr, unit->hu_DMAAddr, tmpval);
#else
                                    WRITEREG(ISP_MEMORY, (MMAP_BULKDMA_HIGH<<2)|IMSF_BANK1);
                                    CopyMemShuffle((APTR) &rmemptr[MMAP_BULKDMA_HIGH], unit->hu_DMAAddr, tmpval);
#endif
                                    unit->hu_DMAOffset += unit->hu_DMALength;
                                    unit->hu_DMAAddr += unit->hu_DMALength;
#ifndef NODMA
                                    unit->hu_DMAPhyAddr += unit->hu_DMALength;
#endif
                                }
                                unit->hu_DMAStateMachine = DMASM_NOP;
                                FreeATL(ptd, unit);
                                break;

                            default:
                                KPRINTF(100, ("Illegal DMA state %ld with ATL ready\n", unit->hu_DMAStateMachine));
                        }
#ifndef NODMA
                        Enable();
#endif
                        break;

                    case PTDT_DATAOUTDMA:
                        KPRINTF(10, ("DATA OUT DMASM=%02lx\n", unit->hu_DMAStateMachine));
                        // out direction with DMA
                        ioreq->iouh_Actual += len;
                        if(ioreq->iouh_Actual < ioreq->iouh_Length)
                        {
                            KPRINTF(1, ("DMA TODO More data to transfer %ld < %ld\n", ioreq->iouh_Actual, ioreq->iouh_Length));
                            if(ioreq->iouh_Length - ioreq->iouh_Actual < ptd->ptd_BufLen)
                            {
                                ptd->ptd_BufLen = ioreq->iouh_Length - ioreq->iouh_Actual;
                                KPRINTF(1, ("DMA Last one with %ld bytes\n", ptd->ptd_BufLen));
                            }
                        } else if((ioreq->iouh_Actual == ioreq->iouh_Length) &&
                                  (len > 0) && // avoid infinite loop
                                  (!(ioreq->iouh_Flags & UHFF_NOSHORTPKT)) &&
                                  ((ioreq->iouh_Actual & (ioreq->iouh_MaxPktSize-1)) == 0))
                        {
                            KPRINTF(100, ("DMA Zero packet terminate\n", ptd->ptd_BufLen));
                            // send 0 byte packet
                            ptd->ptd_BufLen = 0;
                            ptd->ptd_DW[0] &= ~QHA0M_TRANSLEN;
                            ptd->ptd_DW[3] &= ~(QHA3M_TRANSCOUNT|QHA3M_NAKCOUNT|QHA3M_RETRYCOUNT|QHA3F_COMPLETESPLIT|QHA3F_PID_ERROR|QHA3F_BABBLE_ERROR|QHA3F_HALT);
                            ptd->ptd_DW[3] |= (2<<QHA3S_RETRYCOUNT)|QHA3F_ACTIVE;

                            // update HW PTD
                            ispptd[3] = ptd->ptd_DW[3];
                            ispptd[4] = ptd->ptd_DW[4];
                            *ispptd = ptd->ptd_DW[0];
                            unit->hu_ATLActive |= 1<<ptdnum;
                            break;
                        } else {
                            FreeATL(ptd, unit);
                            break;
                        }
#ifndef NODMA
                        Disable(); // avoid race conditions
#endif
                        switch(unit->hu_DMAStateMachine)
                        {
                            case DMASM_OUT_PREFETCH_HIGH:
                                unit->hu_DMAStateMachine = DMASM_OUT_PTD_DONE_LOW;
                                break;

                            case DMASM_OUT_PREFETCH_LOW:
                                unit->hu_DMAStateMachine = DMASM_OUT_PTD_DONE_HIGH;
                                break;

                            case DMASM_OUT_PREFETCH_HIGH_DONE:
                                // activate PTD (HIGH)
                                ptd->ptd_DW[0] &= ~QHA0M_TRANSLEN;
                                ptd->ptd_DW[0] |= QHA0F_VALID|(ptd->ptd_BufLen<<QHA0S_TRANSLEN);
                                ispptd[2] = ptd->ptd_DW[2] = IMAP_BULKDMA_HIGH<<QHA2S_DATAADDR;
                                ptd->ptd_DW[3] &= ~(QHA3M_TRANSCOUNT|QHA3M_NAKCOUNT|QHA3M_RETRYCOUNT|QHA3F_COMPLETESPLIT|QHA3F_PID_ERROR|QHA3F_BABBLE_ERROR|QHA3F_HALT);
                                ptd->ptd_DW[3] |= (2<<QHA3S_RETRYCOUNT)|QHA3F_ACTIVE;
                                // update HW PTD
                                ispptd[3] = ptd->ptd_DW[3];
                                ispptd[4] = ptd->ptd_DW[4];
                                *ispptd = ptd->ptd_DW[0];
                                unit->hu_ATLActive |= 1<<ptdnum;
                                if(unit->hu_DMAOffset >= ioreq->iouh_Length)
                                {
                                    unit->hu_DMAStateMachine = DMASM_NOP;
                                    break;
                                }
                                tmpval = (ptd->ptd_BufLen + 3) & ~3;
                                if(tmpval >= MMAP_DMATHRES_SIZE)
                                {
                                    unit->hu_DMAStateMachine = DMASM_OUT_PREFETCH_LOW;
                                    unit->hu_DMALength = tmpval;

#ifndef NODMA
                                    WRITEREG(ISP_MEMORY, ((MMAP_BULKDMA_LOW<<2)|IMSF_BANK3)|(tmpval<<MMS_LENGTHBYTES));
                                    WRITEMACH(MACH_DMAADDRESS, (ULONG) unit->hu_DMAPhyAddr|(MAF_READFROMRAM|MAF_ENABLE));
#else
                                    unit->hu_ISPAddr = MMAP_BULKDMA_LOW;
                                    Signal(&unit->hu_DMATask, SIGF_SINGLE); // start copy task
#endif
                                } else {
                                    unit->hu_DMAStateMachine = DMASM_OUT_PREFETCH_LOW_DONE;
                                    CopyMemShuffle(unit->hu_DMAAddr, (APTR) &unit->hu_RegBase[MMAP_BULKDMA_LOW], tmpval);
                                    unit->hu_DMAAddr += tmpval;
                                    unit->hu_DMAOffset += tmpval;
#ifndef NODMA
                                    unit->hu_DMAPhyAddr += tmpval;
#endif
                                }
                                break;

                            case DMASM_OUT_PREFETCH_LOW_DONE:
                                // activate PTD (LOW)
                                ptd->ptd_DW[0] &= ~QHA0M_TRANSLEN;
                                ptd->ptd_DW[0] |= QHA0F_VALID|(ptd->ptd_BufLen<<QHA0S_TRANSLEN);
                                ispptd[2] = ptd->ptd_DW[2] = IMAP_BULKDMA_LOW<<QHA2S_DATAADDR;
                                ptd->ptd_DW[3] &= ~(QHA3M_TRANSCOUNT|QHA3M_NAKCOUNT|QHA3M_RETRYCOUNT|QHA3F_COMPLETESPLIT|QHA3F_PID_ERROR|QHA3F_BABBLE_ERROR|QHA3F_HALT);
                                ptd->ptd_DW[3] |= (2<<QHA3S_RETRYCOUNT)|QHA3F_ACTIVE;
                                // update HW PTD
                                ispptd[3] = ptd->ptd_DW[3];
                                ispptd[4] = ptd->ptd_DW[4];
                                *ispptd = ptd->ptd_DW[0];
                                unit->hu_ATLActive |= 1<<ptdnum;
                                if(unit->hu_DMAOffset >= ioreq->iouh_Length)
                                {
                                    unit->hu_DMAStateMachine = DMASM_NOP;
                                    break;
                                }
                                tmpval = (ptd->ptd_BufLen + 3) & ~3;
                                if(tmpval >= MMAP_DMATHRES_SIZE)
                                {
                                    unit->hu_DMAStateMachine = DMASM_OUT_PREFETCH_HIGH;
                                    unit->hu_DMALength = tmpval;

#ifndef NODMA
                                    WRITEREG(ISP_MEMORY, ((MMAP_BULKDMA_HIGH<<2)|IMSF_BANK3)|(tmpval<<MMS_LENGTHBYTES));
                                    WRITEMACH(MACH_DMAADDRESS, (ULONG) unit->hu_DMAPhyAddr|(MAF_READFROMRAM|MAF_ENABLE));
#else
                                    unit->hu_ISPAddr = MMAP_BULKDMA_HIGH;
                                    Signal(&unit->hu_DMATask, SIGF_SINGLE); // start copy task
#endif
                                } else {
                                    unit->hu_DMAStateMachine = DMASM_OUT_PREFETCH_HIGH_DONE;
                                    CopyMemShuffle(unit->hu_DMAAddr, (APTR) &unit->hu_RegBase[MMAP_BULKDMA_HIGH], tmpval);
                                    unit->hu_DMAAddr += tmpval;
                                    unit->hu_DMAOffset += tmpval;
#ifndef NODMA
                                    unit->hu_DMAPhyAddr += tmpval;
#endif
                                }
                                break;

                            default:
                                KPRINTF(100, ("Illegal DMA state %ld with ATL ready\n", unit->hu_DMAStateMachine));
                        }
#ifndef NODMA
                        Enable();
#endif
                        break;

                    case PTDT_DATAIN:
                        // in direction
                        KPRINTF(1, ("BULK received %ld bytes\n", len));
                        if(len)
                        {
                            //KPRINTF(100, ("Before ISP 2 Read\n"));
                            WRITEREG(ISP_MEMORY, ptd->ptd_BufStart<<2);
                            //KPRINTF(100, ("ISP memory written\n"));
                            srcptr = (ULONG *) (((UBYTE *) ioreq->iouh_Data) + ioreq->iouh_Actual);
                            if(ioreq->iouh_Actual + len > ioreq->iouh_Length)
                            {
                                KPRINTF(200, ("BULK IN OVERFLOW %ld + %ld > %ld!\n", ioreq->iouh_Actual, len, ioreq->iouh_Length));
                                ioreq->iouh_Req.io_Error = UHIOERR_OVERFLOW;
                                len = ioreq->iouh_Length - ioreq->iouh_Actual;
                            }
                            if(cnt = len>>2)
                            {
                                do
                                {
                                    *srcptr++ = shuffle(*rmemptr);
                                } while(--cnt);
                            }

                            if(len & 3)
                            {
                                UBYTE *byteptr = (UBYTE *) srcptr;
                                KPRINTF(1, ("Remaining bytes %ld\n", len & 3));
                                tmpval = shuffle(*rmemptr);
                                switch(len & 3)
                                {
                                    case 1:
                                        *byteptr = tmpval>>24;
                                        break;

                                    case 2:
                                        *byteptr++ = tmpval>>24;
                                        *byteptr = tmpval>>16;
                                        break;

                                    case 3:
                                        *byteptr++ = tmpval>>24;
                                        *byteptr++ = tmpval>>16;
                                        *byteptr = tmpval>>8;
                                        break;
                                }
                            }
                            //KPRINTF(100, ("After ISP 2 Read\n"));
                        }
                        // drop through

                    case PTDT_DATAOUT:
                        // out direction
                        ioreq->iouh_Actual += len;
                        if(len < ptd->ptd_BufLen)
                        {
                            if(ptd->ptd_Type == PTDT_DATAOUT)
                            {
                                KPRINTF(200, ("Short packet %ld < %ld on OUT?!?!?\n", len, ptd->ptd_BufLen));
                                ioreq->iouh_Req.io_Error = UHIOERR_HOSTERROR;
                            }
                        } else {
                            if(ioreq->iouh_Actual < ioreq->iouh_Length)
                            {
                                KPRINTF(10, ("TODO More data to transfer %ld < %ld\n", ioreq->iouh_Actual, ioreq->iouh_Length));
                                if(ioreq->iouh_Length - ioreq->iouh_Actual < ptd->ptd_BufLen)
                                {
                                    ptd->ptd_BufLen = ioreq->iouh_Length - ioreq->iouh_Actual;
                                    KPRINTF(10, ("Last one with %ld bytes\n", ptd->ptd_BufLen));
                                }
                                ptd->ptd_DW[0] &= ~QHA0M_TRANSLEN;
                                ptd->ptd_DW[0] |= QHA0F_VALID|(ptd->ptd_BufLen<<QHA0S_TRANSLEN);
                                ptd->ptd_DW[3] &= ~(QHA3M_TRANSCOUNT|QHA3M_NAKCOUNT|QHA3M_RETRYCOUNT|QHA3F_COMPLETESPLIT|QHA3F_PID_ERROR|QHA3F_BABBLE_ERROR|QHA3F_HALT);
                                ptd->ptd_DW[3] |= (2<<QHA3S_RETRYCOUNT)|QHA3F_ACTIVE;

                                // Copy buffer to target
#if 1
                                CopyMemShuffle((((UBYTE *) ioreq->iouh_Data) + ioreq->iouh_Actual), (APTR) &unit->hu_RegBase[ptd->ptd_BufStart], (ULONG) ((ptd->ptd_BufLen + 3) & ~3));
#else
                                srcptr = (ULONG *) (((UBYTE *) ioreq->iouh_Data) + ioreq->iouh_Actual);
                                memptr = &unit->hu_RegBase[ptd->ptd_BufStart];
                                cnt = (ptd->ptd_BufLen + 3)>>2;
                                do
                                {
                                    *memptr++ = *srcptr++;
                                }
                                while(--cnt);
#endif
                                KPRINTF(1, ("DATA OUT part %ld bytes\n", ptd->ptd_BufLen));
                                // update HW PTD
                                ispptd[3] = ptd->ptd_DW[3];
                                ispptd[4] = ptd->ptd_DW[4];
                                *ispptd = ptd->ptd_DW[0];
                                unit->hu_ATLActive |= 1<<ptdnum;

                                //WRITEREG(ISP_INTPTDSKIPMAP, ~unit->hu_IntBusy);
                                freeptd = FALSE;
                            }
                            else if((ioreq->iouh_Actual == ioreq->iouh_Length) &&
                                    (ptd->ptd_Type == PTDT_DATAOUT) && // only for out
                                    (len > 0) && // avoid infinite loop
                                    (!(ioreq->iouh_Flags & UHFF_NOSHORTPKT)) &&
                                    ((ioreq->iouh_Actual % ioreq->iouh_MaxPktSize) == 0))
                            {
                                KPRINTF(100, ("Zero byte packet len %ld, nsp=%ld Flags=%04lx\n", len, (!(ioreq->iouh_Flags & UHFF_NOSHORTPKT)), ioreq->iouh_Flags));
                                // send 0 byte packet
                                ptd->ptd_BufLen = 0;
                                ptd->ptd_DW[0] &= ~QHA0M_TRANSLEN;
                                ptd->ptd_DW[3] &= ~(QHA3M_TRANSCOUNT|QHA3M_NAKCOUNT|QHA3M_RETRYCOUNT|QHA3F_COMPLETESPLIT|QHA3F_PID_ERROR|QHA3F_BABBLE_ERROR|QHA3F_HALT);
                                ptd->ptd_DW[3] |= (2<<QHA3S_RETRYCOUNT)|QHA3F_ACTIVE;

                                // update HW PTD
                                ispptd[3] = ptd->ptd_DW[3];
                                ispptd[4] = ptd->ptd_DW[4];
                                *ispptd = ptd->ptd_DW[0];
                                unit->hu_ATLActive |= 1<<ptdnum;
                                //WRITEREG(ISP_INTPTDSKIPMAP, ~unit->hu_IntBusy);
                                freeptd = FALSE;
                            }
                        }
                        if(freeptd)
                        {
                            FreeATL(ptd, unit);
                        }
                        break;

                    case PTDT_SETUP:
                        KPRINTF(1, ("Setup %ld done.\n", ptdnum));
                        if(len != 8)
                        {
                            ioreq->iouh_Req.io_Error = UHIOERR_PKTTOOLARGE;
                            FreeATL(ptd, unit);
                            break;
                        }

                        // optional data stage
                        if(ioreq->iouh_Length)
                        {
                            KPRINTF(1, ("Memory for setup data phase allocated at %4lx, %ld bytes\n", ptd->ptd_BufStart, ptd->ptd_BufLen));

                            ptd->ptd_DW[0] &= ~(QHA0M_TRANSLEN|QHA0M_MAXPKTLEN);
                            ptd->ptd_DW[0] |= (ptd->ptd_BufLen<<QHA0S_TRANSLEN)|(ioreq->iouh_MaxPktSize<<QHA0S_MAXPKTLEN);

                            ptd->ptd_DW[1] &= ~QHA1M_TOKEN;
                            if(ioreq->iouh_SetupData.bmRequestType & URTF_IN)
                            {
                                // in direction
                                ptd->ptd_DW[1] |= QHA1F_TOKEN_IN;
                                ptd->ptd_Type = PTDT_SDATAIN;
                                KPRINTF(1, ("Setup IN %ld bytes\n", ioreq->iouh_Length));
                            } else {
                                // out direction
                                //ptd->ptd_DW[1] |= QHA1F_TOKEN_OUT;
                                ptd->ptd_Type = PTDT_SDATAOUT;

                                // Copy buffer to target
                                srcptr = (ULONG *) ioreq->iouh_Data;
                                memptr = &unit->hu_RegBase[ptd->ptd_BufStart];
                                cnt = (ptd->ptd_BufLen + 3)>>2;
                                do
                                {
                                    *memptr++ = shuffle(*srcptr++);
                                }
                                while(--cnt);
                                KPRINTF(1, ("Setup OUT %ld bytes\n", ptd->ptd_BufLen));
                            }

                            //ptd->ptd_DW[4] = 0;

                            // update HW PTD
                            ispptd[1] = ptd->ptd_DW[1];
                            ispptd[2] = ptd->ptd_DW[2] = (MMAP2IMAP(ptd->ptd_BufStart)<<QHA2S_DATAADDR)|(0<<QHA2S_RELOAD);
                            ispptd[3] = ptd->ptd_DW[3] = (0<<QHA3S_NAKCOUNT)|(3<<QHA3S_RETRYCOUNT)|QHA3F_DATA1|QHA3F_ACTIVE;
                            //ispptd[4] = ptd->ptd_DW[4];
                            *ispptd = ptd->ptd_DW[0];
                            //WRITEREG(ISP_ATLPTDSKIPMAP, ~unit->hu_ATLBusy);
                            unit->hu_ATLActive |= 1<<ptdnum;

                            break;
                        }
                        nosetupdata = TRUE;
                        // fall through

                    case PTDT_SDATAIN:
                    case PTDT_SDATAOUT:
                        KPRINTF(1, ("Setup data %ld done.\n", ptdnum));
                        if(!nosetupdata)
                        {
                            if(ioreq->iouh_SetupData.bmRequestType & URTF_IN)
                            {
                                // in direction
                                KPRINTF(1, ("Setup Data received %ld bytes\n", len));
                                if(len)
                                {
                                    //KPRINTF(100, ("Before ISP 2 Read\n"));
                                    WRITEREG(ISP_MEMORY, ptd->ptd_BufStart<<2);
                                    //KPRINTF(100, ("ISP memory written\n"));
                                    srcptr = (ULONG *) (((UBYTE *) ioreq->iouh_Data) + ioreq->iouh_Actual);
                                    if(ioreq->iouh_Actual + len > ioreq->iouh_Length)
                                    {
                                        KPRINTF(20, ("OVERFLOW!\n"));
                                        ioreq->iouh_Req.io_Error = UHIOERR_OVERFLOW;
                                        len = ioreq->iouh_Length - ioreq->iouh_Actual;
                                    }
                                    if(cnt = len>>2)
                                    {
                                        do
                                        {
                                            *srcptr++ = shuffle(*rmemptr);
                                        } while(--cnt);
                                    }

                                    if(len & 3)
                                    {
                                        UBYTE *byteptr = (UBYTE *) srcptr;
                                        KPRINTF(1, ("Remaining bytes %ld\n", len & 3));
                                        tmpval = shuffle(*rmemptr);
                                        switch(len & 3)
                                        {
                                            case 1:
                                                *byteptr = tmpval>>24;
                                                break;

                                            case 2:
                                                *byteptr++ = tmpval>>24;
                                                *byteptr = tmpval>>16;
                                                break;

                                            case 3:
                                                *byteptr++ = tmpval>>24;
                                                *byteptr++ = tmpval>>16;
                                                *byteptr = tmpval>>8;
                                                break;
                                        }
                                    }
                                    //KPRINTF(100, ("After ISP 2 Read\n"));
                                }
                            }
                            ioreq->iouh_Actual += len;
                            if(len < ptd->ptd_BufLen)
                            {
                                // short packet
                                KPRINTF(1, ("Short packet %ld < %ld\n", len, ptd->ptd_BufLen));
                                if(!(ioreq->iouh_Flags & UHFF_ALLOWRUNTPKTS))
                                {
                                    KPRINTF(1, ("Short transfer! Actual (%ld) < Length (%ld)!\n", ioreq->iouh_Actual, ioreq->iouh_Length));
                                    ioreq->iouh_Req.io_Error = UHIOERR_RUNTPACKET;
                                }
                            } else {
                                if(ioreq->iouh_Actual < ioreq->iouh_Length)
                                {
                                    KPRINTF(1, ("TODO More data to transfer %ld < %ld\n", ioreq->iouh_Actual, ioreq->iouh_Length));
                                    if(ioreq->iouh_Length - ioreq->iouh_Actual < ptd->ptd_BufLen)
                                    {
                                        ptd->ptd_BufLen = ioreq->iouh_Length - ioreq->iouh_Actual;
                                        KPRINTF(1, ("Last one with %ld bytes\n", ptd->ptd_BufLen));
                                    }
                                    ptd->ptd_DW[0] &= ~QHA0M_TRANSLEN;
                                    ptd->ptd_DW[0] |= ptd->ptd_BufLen<<QHA0S_TRANSLEN;
                                    if(!(ioreq->iouh_SetupData.bmRequestType & URTF_IN))
                                    {
                                        // Copy buffer to target
                                        srcptr = (ULONG *) (((UBYTE *) ioreq->iouh_Data) + ioreq->iouh_Actual);
                                        memptr = &unit->hu_RegBase[ptd->ptd_BufStart];
                                        cnt = (ptd->ptd_BufLen + 3)>>2;
                                        do
                                        {
                                            *memptr++ = shuffle(*srcptr++);
                                        }
                                        while(--cnt);
                                        KPRINTF(1, ("Setup OUT part %ld bytes\n", ptd->ptd_BufLen));
                                    }

                                    ptd->ptd_DW[3] &= ~(QHA3M_TRANSCOUNT|QHA3M_NAKCOUNT|QHA3M_RETRYCOUNT|QHA3F_COMPLETESPLIT|QHA3F_PID_ERROR|QHA3F_BABBLE_ERROR|QHA3F_HALT);
                                    ptd->ptd_DW[3] |= QHA3F_ACTIVE;

                                    // update HW PTD
                                    //ispptd[1] = ptd->ptd_DW[1];
                                    //ispptd[2] = ptd->ptd_DW[2];
                                    ispptd[3] = ptd->ptd_DW[3];
                                    //ispptd[4] = ptd->ptd_DW[4];
                                    *ispptd = ptd->ptd_DW[0];
                                    //WRITEREG(ISP_ATLPTDSKIPMAP, ~unit->hu_ATLBusy);
                                    unit->hu_ATLActive |= 1<<ptdnum;

                                    break;
                                }
                            }
                        }
                        ptd->ptd_DW[0] &= ~QHA0M_TRANSLEN;
                        //ptd->ptd_DW[0] |= 0<<QHA0S_TRANSLEN;

                        ptd->ptd_DW[1] &= ~QHA1M_TOKEN;
                        if(ioreq->iouh_SetupData.bmRequestType & URTF_IN)
                        {
                            // in direction
                            ptd->ptd_DW[1] |= QHA1F_TOKEN_OUT;
                            ptd->ptd_Type = PTDT_STERMOUT;
                            KPRINTF(1, ("Setup OUT TERM\n"));
                        } else {
                            // out direction
                            ptd->ptd_DW[1] |= QHA1F_TOKEN_IN;
                            ptd->ptd_Type = PTDT_STERMIN;
                            KPRINTF(1, ("Setup IN TERM\n"));
                        }

                        //ptd->ptd_DW[2] |= (0<<QHA2S_RELOAD);
                        //ptd->ptd_DW[4] = 0;

                        // update HW PTD
                        ispptd[1] = ptd->ptd_DW[1];
                        ispptd[2] = ptd->ptd_DW[2];
                        ispptd[3] = ptd->ptd_DW[3] = (0<<QHA3S_NAKCOUNT)|(3<<QHA3S_RETRYCOUNT)|QHA3F_DATA1|QHA3F_ACTIVE;
                        //ispptd[4] = ptd->ptd_DW[4];
                        *ispptd = ptd->ptd_DW[0];
                        //WRITEREG(ISP_ATLPTDSKIPMAP, ~unit->hu_ATLBusy);
                        unit->hu_ATLActive |= 1<<ptdnum;

                        break;

                    case PTDT_STERMIN:
                    case PTDT_STERMOUT:
                        KPRINTF(1, ("Setup term %ld done.\n", ptdnum));
                        /* Clear Feature(Endpoint halt) */
                        if(*((ULONG *) &ioreq->iouh_SetupData) ==
                           (((URTF_STANDARD|URTF_ENDPOINT)<<24)|
                            (USR_CLEAR_FEATURE<<16)|
                            UFS_ENDPOINT_HALT))
                        {
                            KPRINTF(10, ("Resetting toggle bit for endpoint %ld\n", (ioreq->iouh_SetupData.wIndex>>8) & 0xf));
                            unit->hu_DevDataToggle[(ioreq->iouh_DevAddr<<5)|((ioreq->iouh_SetupData.wIndex>>8) & 0xf)|((ioreq->iouh_SetupData.wIndex>>15)<<4)] = 0;
                        }
                        else if((ioreq->iouh_SetupData.bmRequestType == (URTF_STANDARD|URTF_DEVICE)) &&
                                (ioreq->iouh_SetupData.bRequest == USR_SET_ADDRESS))
                        {
                            /* Set Address -> clear all endpoints */
                            ULONG epnum;
                            ULONG adr = ioreq->iouh_SetupData.wValue>>3;
                            KPRINTF(10, ("Resetting toggle bits for device address %ld\n", adr>>5));
                            for(epnum = 0; epnum < 31; epnum++)
                            {
                                unit->hu_DevDataToggle[adr+epnum] = 0;
                            }
                        }
                        FreeATL(ptd, unit);
                        break;

                    default:
                        KPRINTF(20, ("Unhandled PTDType %ld\n", ptd->ptd_Type));
                }
            }
        }
        ptdnum++;
        ptdnum &= 31;
    }
}
/* \\\ */

/* /// "uhwHandleFinishedInts()" */
void uhwHandleFinishedInts(struct DenebUnit *unit)
{
    struct IOUsbHWReq *ioreq;
    UWORD cnt;
    UWORD ptdnum;
    ULONG mask;
    struct PTDNode *ptd;
    volatile ULONG *ispptd;
    volatile ULONG *memptr;
    volatile ULONG *rmemptr = unit->hu_ReadBaseMem;
    ULONG *srcptr;
    ULONG tmpval;
    UWORD devadrep;
    BOOL freeptd;
    ULONG len;

    /* *** INT transfers *** */
    ptdnum = 0;
    /*if(unit->hu_IntBusy)
    {
        KPRINTF(1, ("Faking IntDone %08lx\n", unit->hu_IntBusy));
        unit->hu_IntDone |= unit->hu_IntBusy;
    }*/
    while(unit->hu_IntDone)
    {
        mask = (1<<ptdnum);
        if(unit->hu_IntDone & mask)
        {
            KPRINTF(1, ("INT %ld DONE\n", ptdnum));
            unit->hu_IntDone &= ~mask;
            ptd = &unit->hu_IntPTDs[ptdnum];
            ispptd = &unit->hu_RegBase[MMAP_INT_PTDS + (ptdnum<<3)];
            ioreq = ptd->ptd_IOReq;
            if(!ioreq)
            {
                KPRINTF(20, ("No IOReq for PTD %ld\n", ptdnum));
                continue;
            }
            devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
            freeptd = TRUE;

            WRITEREG(ISP_MEMORY, ((MMAP_INT_PTDS+3)<<2) + (ptdnum<<5));
            ptd->ptd_DW[3] = *ispptd;
            ptd->ptd_DW[4] = *ispptd;

            // update data toggle bit
            unit->hu_DevDataToggle[devadrep] = (ptd->ptd_DW[3] & QHA3F_DATA1)>>QHA3B_DATA1;

            if(ptd->ptd_DW[3] & QHA3F_HALT)
            {
#ifdef DEBUG
                ULONG status = ptd->ptd_DW[4]>>QHA4S_FIRSTSTATUS;
#endif
                if(ptd->ptd_DW[3] & QHA3F_BABBLE_ERROR)
                {
                    KPRINTF(200, ("INTERRUPT BABBLE ERROR!\n"));
                    ioreq->iouh_Req.io_Error = UHIOERR_OVERFLOW;
                }
                else if(ptd->ptd_DW[3] & QHA3F_PID_ERROR)
                {
                    KPRINTF(200, ("INTERRUPT PID ERROR\n"));
                    ioreq->iouh_Req.io_Error = UHIOERR_TIMEOUT;
                }
                else if(ptd->ptd_DW[3] & QHA3F_HALT)
                {
                    KPRINTF(200, ("INTERRUPT STALLED!\n"));
                    ioreq->iouh_Req.io_Error = UHIOERR_STALL;
                }
#ifdef DEBUG
                // debug
                for(cnt = 0; cnt < 8; cnt++)
                {
                    if(status & QHASF_BABBLE_ERROR)
                    {
                        KPRINTF(200, ("INTERRUPT BABBLE ERROR in SOF %ld!\n", cnt));
                    }
                    else if(status & QHASF_UNDERRUN)
                    {
                        KPRINTF(200, ("INTERRUPT UNDERRUN ERROR in SOF %ld!\n", cnt));
                    }
                    else if(status & QHASF_TRANS_ERROR)
                    {
                        KPRINTF(200, ("INTERRUPT TRANSACTION ERROR in SOF %ld!\n", cnt));
                    }
                    status >>= 3;
                }
#endif
                FreeInt(ptd, unit);
            }
            else if(ptd->ptd_DW[3] & QHA3F_ACTIVE)
            {
                // Restarting transaction due to 15 NAKs
                if((unit->hu_IntNakMap & mask) && (unit->hu_FrameCounter > ptd->ptd_NakTimeoutFrame))
                {
                    KPRINTF(10, ("Nak timeout %ld > %ld\n", unit->hu_FrameCounter, ptd->ptd_NakTimeoutFrame));
                    ioreq->iouh_Req.io_Error = UHIOERR_NAKTIMEOUT;
                    FreeInt(ptd, unit);
                } else {
                    KPRINTF(1, ("INT NAK\n"));
                    *ispptd = ptd->ptd_DW[0];
                }
            } else {
                KPRINTF(1, ("Interrupt %ld done.\n", ptdnum));

                // update nak timeout frame
                if(unit->hu_IntNakMap & mask)
                {
                    ptd->ptd_NakTimeoutFrame = unit->hu_FrameCounter + (ioreq->iouh_NakTimeout<<3);
                }

                if(ioreq->iouh_Flags & UHFF_SPLITTRANS)
                {
                   len = (ptd->ptd_DW[3] & QHA3M_TRANSCOUNT_LS)>>QHA3S_TRANSCOUNT;
                } else {
                   len = (ptd->ptd_DW[3] & QHA3M_TRANSCOUNT)>>QHA3S_TRANSCOUNT;
                }

                if(ptd->ptd_Type == PTDT_DATAIN)
                {
                    // in direction
                    KPRINTF(1, ("Interrupt received %ld bytes\n", len));
                    if(len)
                    {
                        WRITEREG(ISP_MEMORY, ptd->ptd_BufStart<<2);
                        srcptr = (ULONG *) (((UBYTE *) ioreq->iouh_Data) + ioreq->iouh_Actual);
                        if(ioreq->iouh_Actual + len > ioreq->iouh_Length)
                        {
                            KPRINTF(200, ("INT IN OVERFLOW %ld + %ld > %ld!\n", ioreq->iouh_Actual, len, ioreq->iouh_Length));
                            ioreq->iouh_Req.io_Error = UHIOERR_OVERFLOW;
                            len = ioreq->iouh_Length - ioreq->iouh_Actual;
                        }
                        if(cnt = len>>2)
                        {
                            do
                            {
                                *srcptr++ = shuffle(*rmemptr);
                            } while(--cnt);
                        }

                        if(len & 3)
                        {
                            UBYTE *byteptr = (UBYTE *) srcptr;
                            KPRINTF(1, ("Remaining bytes %ld\n", len & 3));
                            tmpval = shuffle(*rmemptr);
                            switch(len & 3)
                            {
                                case 1:
                                    *byteptr = tmpval>>24;
                                    break;

                                case 2:
                                    *byteptr++ = tmpval>>24;
                                    *byteptr = tmpval>>16;
                                    break;

                                case 3:
                                    *byteptr++ = tmpval>>24;
                                    *byteptr++ = tmpval>>16;
                                    *byteptr = tmpval>>8;
                                    break;
                            }
                        }
                    }
                }
                ioreq->iouh_Actual += len;
                if(len < ptd->ptd_BufLen)
                {
                    // short packet
                    KPRINTF(1, ("Short packet %ld < %ld\n", len, ptd->ptd_BufLen));
                    if(!(ioreq->iouh_Flags & UHFF_ALLOWRUNTPKTS))
                    {
                        KPRINTF(1, ("Short transfer! Actual (%ld) < Length (%ld)!\n", ioreq->iouh_Actual, ioreq->iouh_Length));
                        ioreq->iouh_Req.io_Error = UHIOERR_RUNTPACKET;
                    }
                } else {
                    if(ioreq->iouh_Actual < ioreq->iouh_Length)
                    {
                        KPRINTF(1, ("TODO More data to transfer %ld < %ld\n", ioreq->iouh_Actual, ioreq->iouh_Length));
                        if(ioreq->iouh_Length - ioreq->iouh_Actual < ptd->ptd_BufLen)
                        {
                            ptd->ptd_BufLen = ioreq->iouh_Length - ioreq->iouh_Actual;
                            KPRINTF(1, ("Last one with %ld bytes\n", ptd->ptd_BufLen));
                        }
                        ptd->ptd_DW[0] &= ~QHA0M_TRANSLEN;
                        ptd->ptd_DW[0] |= ptd->ptd_BufLen<<QHA0S_TRANSLEN;
                        if(ptd->ptd_Type == PTDT_DATAOUT)
                        {
                            // Copy buffer to target
                            srcptr = (ULONG *) (((UBYTE *) ioreq->iouh_Data) + ioreq->iouh_Actual);
                            memptr = &unit->hu_RegBase[ptd->ptd_BufStart];
                            cnt = (ptd->ptd_BufLen + 3)>>2;
                            do
                            {
                                *memptr++ = shuffle(*srcptr++);
                            }
                            while(--cnt);
                            KPRINTF(1, ("Setup OUT part %ld bytes\n", ptd->ptd_BufLen));
                        }

                        ptd->ptd_DW[3] &= ~(QHA3M_TRANSCOUNT|QHA3M_NAKCOUNT|QHA3M_RETRYCOUNT|QHA3F_COMPLETESPLIT|QHA3F_PID_ERROR|QHA3F_BABBLE_ERROR|QHA3F_HALT);
                        ptd->ptd_DW[3] |= QHA3F_ACTIVE;
                        // update HW PTD
                        ispptd[3] = ptd->ptd_DW[3];
                        ispptd[4] = ptd->ptd_DW[4] = ptd->ptd_IntSOFMap<<QHA4S_MUSOFACTIVE;
                        ispptd[5] = ptd->ptd_DW[5];
                        *ispptd = ptd->ptd_DW[0];
                        //WRITEREG(ISP_INTPTDSKIPMAP, ~unit->hu_IntBusy);
                        freeptd = FALSE;
                    }
                }
                if(freeptd)
                {
                    FreeInt(ptd, unit);
                }
            }
        }
        ptdnum++;
        ptdnum &= 31;
    }
}
/* \\\ */

/* /// "uhwHandleFinishedIsos()" */
void uhwHandleFinishedIsos(struct DenebUnit *unit)
{
    struct IOUsbHWReq *ioreq;
    UWORD cnt;
    UWORD ptdnum;
    ULONG mask;
    struct PTDNode *ptd;
    volatile ULONG *ispptd;
    volatile ULONG *memptr;
    volatile ULONG *rmemptr = unit->hu_ReadBaseMem;
    ULONG *srcptr;
    ULONG tmpval;
    BOOL freeptd;
    ULONG len;
#ifdef DEBUG
    ULONG dw4,dw5,dw6,dw7;
#endif

    /* *** ISO transfers *** */
    ptdnum = 0;
    while(unit->hu_IsoDone)
    {
        mask = (1<<ptdnum);
        if(unit->hu_IsoDone & mask)
        {
            KPRINTF(1, ("ISO %ld DONE\n", ptdnum));
            unit->hu_IsoDone &= ~mask;
            ptd = &unit->hu_IsoPTDs[ptdnum];
            ispptd = &unit->hu_RegBase[MMAP_ISO_PTDS + (ptdnum<<3)];
            ioreq = ptd->ptd_IOReq;
            if(!ioreq)
            {
                KPRINTF(20, ("No IOReq for PTD %ld\n", ptdnum));
                continue;
            }
            freeptd = TRUE;

            WRITEREG(ISP_MEMORY, ((MMAP_ISO_PTDS+3)<<2) + (ptdnum<<5));
            ptd->ptd_DW[3] = *ispptd;
#ifdef DEBUG
            dw4 = *ispptd;
            dw5 = *ispptd;
            dw6 = *ispptd;
            dw7 = *ispptd;
#endif
            if(ptd->ptd_DW[3] & QHA3F_HALT)
            {
#ifdef DEBUG
                ULONG status = dw4>>QHA4S_FIRSTSTATUS;
#endif
                if(ptd->ptd_DW[3] & QHA3F_BABBLE_ERROR)
                {
                    KPRINTF(200, ("ISO BABBLE ERROR!\n"));
                    ioreq->iouh_Req.io_Error = UHIOERR_OVERFLOW;
                }
                else if(ptd->ptd_DW[3] & QHA3F_PID_ERROR)
                {
                    KPRINTF(200, ("ISO PID ERROR\n"));
                    ioreq->iouh_Req.io_Error = UHIOERR_TIMEOUT;
                }
                else if(ptd->ptd_DW[3] & QHA3F_HALT)
                {
                    KPRINTF(200, ("ISO STALLED!\n"));
                    ioreq->iouh_Req.io_Error = UHIOERR_STALL;
#ifdef DEBUG
                // debug
                for(cnt = 0; cnt < 8; cnt++)
                {
                    len = 0;
                    if(ioreq->iouh_Flags & UHFF_SPLITTRANS)
                    {
                        switch(cnt)
                        {
                            case 0:
                                len = (dw5>>8) & 0xff;
                                break;
                            case 1:
                                len = (dw5>>16) & 0xff;
                                break;
                            case 2:
                                len = dw5>>24;
                                break;
                            case 3:
                                len = dw6 & 0xff;
                                break;
                            case 4:
                                len = (dw6>>8) & 0x0ff;
                                break;
                            case 5:
                                len = (dw6>>16) & 0xff;
                                break;
                            case 6:
                                len = (dw6>>24) & 0xff;
                                break;
                            case 7:
                                len = dw7 & 0xff;
                                break;
                        }
                    } else {
                        switch(cnt)
                        {
                            case 0:
                                len = dw5 & 0x0fff;
                                break;
                            case 1:
                                len = (dw5>>12) & 0x0fff;
                                break;
                            case 2:
                                len = ((dw5>>24)|(dw6<<8)) & 0x0fff;
                                break;
                            case 3:
                                len = (dw6>>4) & 0x0fff;
                                break;
                            case 4:
                                len = (dw6>>16) & 0x0fff;
                                break;
                            case 5:
                                len = ((dw6>>28)|(dw7<<4)) & 0x0fff;
                                break;
                            case 6:
                                len = (dw7>>8) & 0x0fff;
                                break;
                            case 7:
                                len = dw7>>20;
                                break;
                        }
                    }
                    if(len)
                    {
                        KPRINTF(200, ("SOF=%ld: Len=%ld\n", cnt, len));
                    }
                    if(status & QHASF_BABBLE_ERROR)
                    {
                        KPRINTF(200, ("ISO BABBLE ERROR in SOF %ld!\n", cnt));
                    }
                    else if(status & QHASF_UNDERRUN)
                    {
                        KPRINTF(200, ("ISO UNDERRUN ERROR in SOF %ld!\n", cnt));
                    }
                    else if(status & QHASF_TRANS_ERROR)
                    {
                        KPRINTF(200, ("ISO TRANSACTION ERROR in SOF %ld!\n", cnt));
                    }
                    status >>= 3;
                }
#endif
                }
                FreeIso(ptd, unit);
            }
            else if(ptd->ptd_DW[3] & QHA3F_ACTIVE)
            {
                // Restarting transaction due to 15 NAKs (should not happen?)
                KPRINTF(200, ("ISO NAK\n"));
                *ispptd = ptd->ptd_DW[0];
            } else {
#ifdef DEBUG
                // debug
                for(cnt = 0; cnt < 8; cnt++)
                {
                    len = 0;
                    if(ioreq->iouh_Flags & UHFF_SPLITTRANS)
                    {
                        switch(cnt)
                        {
                            case 0:
                                len = (dw5>>8) & 0xff;
                                break;
                            case 1:
                                len = (dw5>>16) & 0xff;
                                break;
                            case 2:
                                len = dw5>>24;
                                break;
                            case 3:
                                len = dw6 & 0xff;
                                break;
                            case 4:
                                len = (dw6>>8) & 0x0ff;
                                break;
                            case 5:
                                len = (dw6>>16) & 0xff;
                                break;
                            case 6:
                                len = (dw6>>24) & 0xff;
                                break;
                            case 7:
                                len = dw7 & 0xff;
                                break;
                        }
                    } else {
                        switch(cnt)
                        {
                            case 0:
                                len = dw5 & 0x0fff;
                                break;
                            case 1:
                                len = (dw5>>12) & 0x0fff;
                                break;
                            case 2:
                                len = ((dw5>>24)|(dw6<<8)) & 0x0fff;
                                break;
                            case 3:
                                len = (dw6>>4) & 0x0fff;
                                break;
                            case 4:
                                len = (dw6>>16) & 0x0fff;
                                break;
                            case 5:
                                len = ((dw6>>28)|(dw7<<4)) & 0x0fff;
                                break;
                            case 6:
                                len = (dw7>>8) & 0x0fff;
                                break;
                            case 7:
                                len = dw7>>20;
                                break;
                        }
                    }
                    if(len)
                    {
                        KPRINTF(200, ("SOF=%ld: Len=%ld\n", cnt, len));
                    }
                }
#endif
                KPRINTF(1, ("Iso %ld done.\n", ptdnum));

                len = (ptd->ptd_DW[3] & QHA3M_TRANSCOUNT)>>QHA3S_TRANSCOUNT;

                if(ptd->ptd_Type == PTDT_DATAIN)
                {
                    // in direction
                    KPRINTF(200, ("Iso received %ld bytes\n", len));
                    if(len)
                    {
                        WRITEREG(ISP_MEMORY, ptd->ptd_BufStart<<2);
                        srcptr = (ULONG *) (((UBYTE *) ioreq->iouh_Data) + ioreq->iouh_Actual);
                        if(ioreq->iouh_Actual + len > ioreq->iouh_Length)
                        {
                            KPRINTF(200, ("ISO IN OVERFLOW %ld + %ld > %ld!\n", ioreq->iouh_Actual, len, ioreq->iouh_Length));
                            ioreq->iouh_Req.io_Error = UHIOERR_OVERFLOW;
                            len = ioreq->iouh_Length - ioreq->iouh_Actual;
                        }
                        if(cnt = len>>2)
                        {
                            do
                            {
                                *srcptr++ = shuffle(*rmemptr);
                            } while(--cnt);
                        }

                        if(len & 3)
                        {
                            UBYTE *byteptr = (UBYTE *) srcptr;
                            //KPRINTF(1, ("Remaining bytes %ld\n", len & 3));
                            tmpval = shuffle(*rmemptr);
                            switch(len & 3)
                            {
                                case 1:
                                    *byteptr = tmpval>>24;
                                    break;

                                case 2:
                                    *byteptr++ = tmpval>>24;
                                    *byteptr = tmpval>>16;
                                    break;

                                case 3:
                                    *byteptr++ = tmpval>>24;
                                    *byteptr++ = tmpval>>16;
                                    *byteptr = tmpval>>8;
                                    break;
                            }
                        }
                    }
                }
                ioreq->iouh_Actual += len;
                if(len && (ioreq->iouh_Actual + ioreq->iouh_MaxPktSize < ioreq->iouh_Length))
                {
                    KPRINTF(10, ("TODO More data to transfer %ld < %ld\n", ioreq->iouh_Actual, ioreq->iouh_Length));
                    ioreq->iouh_Frame += (ioreq->iouh_Interval+7)>>3;
                    ptd->ptd_DW[0] &= ~QHA0M_TRANSLEN;
                    ptd->ptd_DW[0] |= ptd->ptd_BufLen<<QHA0S_TRANSLEN;
                    if(ptd->ptd_Type == PTDT_DATAOUT)
                    {
                        // Copy buffer to target
                        srcptr = (ULONG *) (((UBYTE *) ioreq->iouh_Data) + ioreq->iouh_Actual);
                        memptr = &unit->hu_RegBase[ptd->ptd_BufStart];
                        cnt = (ptd->ptd_BufLen + 3)>>2;
                        do
                        {
                            *memptr++ = shuffle(*srcptr++);
                        }
                        while(--cnt);
                        KPRINTF(1, ("Setup OUT part %ld bytes\n", ptd->ptd_BufLen));
                    }
                    ptd->ptd_DW[2] &= ~QHA2M_MUFRAME;
#if 0
                    ptd->ptd_DW[2] |= (ioreq->iouh_Frame & 0x1f)<<(3+QHA2S_MUFRAME);
#else
                    ptd->ptd_DW[2] |= ((READREG(EHCI_FRINDEX)+8) & 0xf8)<<QHA2S_MUFRAME;
#endif

                    // update HW PTD
                    ispptd[2] = ptd->ptd_DW[2];
                    ispptd[3] = ptd->ptd_DW[3] = QHA3F_ACTIVE;
                    ispptd[4] = ptd->ptd_DW[4];// = ptd->ptd_IntSOFMap<<QHA4S_MUSOFACTIVE;
                    ispptd[5] = ptd->ptd_DW[5];
                    *ispptd = ptd->ptd_DW[0];
                    //WRITEREG(ISP_ISOPTDSKIPMAP, ~unit->hu_IsoBusy);
                    freeptd = FALSE;
                    KPRINTF(1, ("%ld\n", READREG(EHCI_FRINDEX)>>3));
                }
                if(freeptd)
                {
                    FreeIso(ptd, unit);
                }
            }
        }
        ptdnum++;
        ptdnum &= 31;
    }
}
/* \\\ */

/* /// "uhwHandleFinishedRTIsos()" */
void uhwHandleFinishedRTIsos(struct DenebUnit *unit)
{
    struct DenebDevice *base = unit->hu_Device;
    struct IOUsbHWReq *ioreq;
    UWORD cnt;
    UWORD ptdnum;
    UWORD isomaxpktsize;
    ULONG mask;
    struct PTDNode *ptd;
    struct RTIsoNode *rtn;
    struct IOUsbHWBufferReq *ubr;
    struct IOUsbHWRTIso *urti;
    volatile ULONG *ispptd;
    volatile ULONG *memptr;
#ifndef ZORRO_II
    volatile ULONG *rmemptr = &unit->hu_ReadBaseMem[MMAP_BANK2];
#else
    volatile ULONG *rmemptr = unit->hu_ReadBaseMem;
#endif
    ULONG *srcptr;
    ULONG tmpval = 0;
    ULONG len;
    ULONG dw3;
#ifdef DEBUG
    ULONG dw4,dw5,dw6,dw7;
#endif

    /* *** RT ISO transfers *** */
    ptdnum = 0;
    while(unit->hu_IsoRTDone)
    {
        mask = (1<<ptdnum);
        if(unit->hu_IsoRTDone & mask)
        {
            KPRINTF(1, ("RT ISO %ld DONE\n", ptdnum));
            unit->hu_IsoRTDone &= ~mask;
            ptd = &unit->hu_IsoPTDs[ptdnum];
#ifndef ZORRO_II
            ispptd = &unit->hu_RegBase[(MMAP_ISO_PTDS|MMAP_BANK2) + (ptdnum<<3)];
#else
            ispptd = &unit->hu_RegBase[MMAP_ISO_PTDS + (ptdnum<<3)];
#endif
            if(!(ptd->ptd_DW[3] & QHA3F_ACTIVE))
            {
                KPRINTF(20, ("Ran into stopped PTD %ld\n", ptdnum));
                continue;
            }
            rtn = ptd->ptd_RTIsoNode;
            if(!rtn)
            {
                KPRINTF(20, ("No RTIsoNode for RT PTD %ld\n", ptdnum));
                continue;
            }
            ioreq = &rtn->rtn_IOReq;
            if(rtn->rtn_PTDs[rtn->rtn_NextPTD] != ptd)
            {
                KPRINTF(20, ("WO\n"));
                /* unit->hu_IsoRTDone |= mask;
                ptdnum++;
                ptdnum &= 31;
                continue;*/
            }

#ifndef ZORRO_II
            WRITEREG(ISP_MEMORY, (((MMAP_ISO_PTDS+3)<<2) + (ptdnum<<5))|IMSF_BANK2);
#else
            WRITEREG(ISP_MEMORY, (((MMAP_ISO_PTDS+3)<<2) + (ptdnum<<5)));
#endif
            dw3 = *ispptd;
#ifdef DEBUG
            dw4 = *ispptd;
            dw5 = *ispptd;
            dw6 = *ispptd;
            dw7 = *ispptd;
#endif
            if(dw3 & QHA3F_HALT)
            {
#ifdef DEBUG
                ULONG status = dw4>>QHA4S_FIRSTSTATUS;
#endif
                if(dw3 & QHA3F_BABBLE_ERROR)
                {
                    KPRINTF(200, ("ISO BABBLE ERROR!\n"));
                    ioreq->iouh_Req.io_Error = UHIOERR_OVERFLOW;
                }
                else if(dw3 & QHA3F_PID_ERROR)
                {
                    KPRINTF(200, ("ISO PID ERROR\n"));
                    ioreq->iouh_Req.io_Error = UHIOERR_TIMEOUT;
                }
                else if(dw3 & QHA3F_HALT)
                {
                    KPRINTF(200, ("ISO STALLED!\n"));
                    ioreq->iouh_Req.io_Error = UHIOERR_STALL;
                }
#ifdef DEBUG
                // debug
                for(cnt = 0; cnt < 8; cnt++)
                {
                    len = 0;
                    if(ioreq->iouh_Flags & UHFF_SPLITTRANS)
                    {
                        switch(cnt)
                        {
                            case 0:
                                len = (dw5>>8) & 0xff;
                                break;
                            case 1:
                                len = (dw5>>16) & 0xff;
                                break;
                            case 2:
                                len = dw5>>24;
                                break;
                            case 3:
                                len = dw6 & 0xff;
                                break;
                            case 4:
                                len = (dw6>>8) & 0xff;
                                break;
                            case 5:
                                len = (dw6>>16) & 0xff;
                                break;
                            case 6:
                                len = (dw6>>24) & 0xff;
                                break;
                            case 7:
                                len = dw7 & 0xff;
                                break;
                        }
                    } else {
                        switch(cnt)
                        {
                            case 0:
                                len = dw5 & 0x0fff;
                                break;
                            case 1:
                                len = (dw5>>12) & 0x0fff;
                                break;
                            case 2:
                                len = ((dw5>>24)|(dw6<<8)) & 0x0fff;
                                break;
                            case 3:
                                len = (dw6>>4) & 0x0fff;
                                break;
                            case 4:
                                len = (dw6>>16) & 0x0fff;
                                break;
                            case 5:
                                len = ((dw6>>28)|(dw7<<4)) & 0x0fff;
                                break;
                            case 6:
                                len = (dw7>>8) & 0x0fff;
                                break;
                            case 7:
                                len = dw7>>20;
                                break;
                        }
                    }
                    if(len)
                    {
                        KPRINTF(200, ("SOF=%ld: Len=%ld\n", cnt, len));
                    }
                    if(status & QHASF_BABBLE_ERROR)
                    {
                        KPRINTF(200, ("ISO BABBLE ERROR in SOF %ld!\n", cnt));
                    }
                    else if(status & QHASF_UNDERRUN)
                    {
                        KPRINTF(200, ("ISO UNDERRUN ERROR in SOF %ld!\n", cnt));
                    }
                    else if(status & QHASF_TRANS_ERROR)
                    {
                        KPRINTF(200, ("ISO TRANSACTION ERROR in SOF %ld!\n", cnt));
                    }
                    status >>= 3;
                }
#endif
            } else {
#ifdef DEBUG
                // debug
                for(cnt = 0; cnt < 8; cnt++)
                {
                    len = 0;
                    if(ioreq->iouh_Flags & UHFF_SPLITTRANS)
                    {
                        switch(cnt)
                        {
                            case 0:
                                len = (dw5>>8) & 0xff;
                                break;
                            case 1:
                                len = (dw5>>16) & 0xff;
                                break;
                            case 2:
                                len = dw5>>24;
                                break;
                            case 3:
                                len = dw6 & 0xff;
                                break;
                            case 4:
                                len = (dw6>>8) & 0xff;
                                break;
                            case 5:
                                len = (dw6>>16) & 0xff;
                                break;
                            case 6:
                                len = (dw6>>24) & 0xff;
                                break;
                            case 7:
                                len = dw7 & 0xff;
                                break;
                        }
                    } else {
                        switch(cnt)
                        {
                            case 0:
                                len = dw5 & 0x0fff;
                                break;
                            case 1:
                                len = (dw5>>12) & 0x0fff;
                                break;
                            case 2:
                                len = ((dw5>>24)|(dw6<<8)) & 0x0fff;
                                break;
                            case 3:
                                len = (dw6>>4) & 0x0fff;
                                break;
                            case 4:
                                len = (dw6>>16) & 0x0fff;
                                break;
                            case 5:
                                len = ((dw6>>28)|(dw7<<4)) & 0x0fff;
                                break;
                            case 6:
                                len = (dw7>>8) & 0x0fff;
                                break;
                            case 7:
                                len = dw7>>20;
                                break;
                        }
                    }
                    if(len)
                    {
                        KPRINTF(10, ("SOF=%ld: Len=%ld\n", cnt, len));
                    }
                }
#endif
                KPRINTF(1, ("Iso %ld done.\n", ptdnum));
                ubr = &rtn->rtn_BufferReq;
                urti = rtn->rtn_RTIso;

                ptd->ptd_DW[2] &= ~QHA2M_MUFRAME;
                isomaxpktsize = ioreq->iouh_MaxPktSize;
                if(ptd->ptd_Type == PTDT_DATAIN)
                {
                    KPRINTF(1, ("Iso received %ld bytes\n", (dw3 & QHA3M_TRANSCOUNT)>>QHA3S_TRANSCOUNT));
                    ubr->ubr_Frame = ptd->ptd_NakTimeoutFrame;

                    // in direction
                    if(urti->urti_InReqHook)
                    {
                        ULONG oddbytes = 0;
                        ULONG bytecnt;
                        len = (dw3 & QHA3M_TRANSCOUNT)>>QHA3S_TRANSCOUNT;
                        /* FIXME If ran in interrupt, this could clash with other reading routines */
#ifndef ZORRO_II
                        WRITEREG(ISP_MEMORY, (ptd->ptd_BufStart<<2)|IMSF_BANK2);
#else
                        WRITEREG(ISP_MEMORY, ptd->ptd_BufStart<<2);
#endif
                        do
                        {
                            ubr->ubr_Length = len;
                            CallHookPkt(urti->urti_InReqHook, urti, ubr);
                            KPRINTF(10, ("Hook<-%ld\n", ubr->ubr_Length));

                            if(bytecnt = ubr->ubr_Length)
                            {
                                if(oddbytes)
                                {
                                    UBYTE *byteptr = ubr->ubr_Buffer;
                                    if(oddbytes == 3)
                                    {
                                        *byteptr++ = tmpval>>16;
                                        oddbytes--;
                                        bytecnt--;
                                    }
                                    if((oddbytes == 2) && bytecnt)
                                    {
                                        *byteptr++ = tmpval>>8;
                                        oddbytes--;
                                        bytecnt--;
                                    }
                                    if(bytecnt)
                                    {
                                        *byteptr++ = tmpval;
                                        oddbytes--;
                                        bytecnt--;
                                    }
                                    srcptr = (ULONG *) byteptr;
                                } else {
                                    srcptr = (ULONG *) ubr->ubr_Buffer;
                                }
                                if(cnt = bytecnt>>2)
                                {
                                    do
                                    {
                                        *srcptr++ = shuffle(*rmemptr);
                                    } while(--cnt);
                                }
                                if(bytecnt & 3)
                                {
                                    UBYTE *byteptr = (UBYTE *) srcptr;
                                    //KPRINTF(1, ("Remaining bytes %ld\n", len & 3));
                                    tmpval = shuffle(*rmemptr);
                                    switch(bytecnt & 3)
                                    {
                                        case 1:
                                            *byteptr = tmpval>>24;
                                            break;

                                        case 2:
                                            *byteptr++ = tmpval>>24;
                                            *byteptr = tmpval>>16;
                                            break;

                                        case 3:
                                            *byteptr++ = tmpval>>24;
                                            *byteptr++ = tmpval>>16;
                                            *byteptr = tmpval>>8;
                                            break;
                                    }
                                    oddbytes = 4 - (bytecnt & 3);
                                }
                            }
                            if(urti->urti_InDoneHook)
                            {
                                CallHookPkt(urti->urti_InDoneHook, urti, ubr);
                            }
                            // automatically advance buffer
                            ubr->ubr_Buffer += ubr->ubr_Length;
                            if(!(ubr->ubr_Flags & UBFF_CONTBUFFER))
                            {
                                break;
                            }
                            // scatter gather rest of buffer
                            ubr->ubr_Flags &= ~UBFF_CONTBUFFER;
                            len -= ubr->ubr_Length;
                        } while(TRUE);
                    }
                    ptd->ptd_NakTimeoutFrame += ioreq->iouh_Interval<<1;
                    if(ioreq->iouh_Flags & UHFF_SPLITTRANS)
                    {
                        ptd->ptd_DW[2] |= (ptd->ptd_NakTimeoutFrame & 0x1f)<<(3+QHA2S_MUFRAME);
                    } else {
                        ptd->ptd_DW[2] |= (ptd->ptd_NakTimeoutFrame & (0x1f<<3))<<(QHA2S_MUFRAME);
                    }
                } else {
                    KPRINTF(10, ("Iso sent %ld bytes\n", (dw3 & QHA3M_TRANSCOUNT)>>QHA3S_TRANSCOUNT));
                    ptd->ptd_NakTimeoutFrame += ioreq->iouh_Interval<<1;
                    if(ioreq->iouh_Flags & UHFF_SPLITTRANS)
                    {
                        ptd->ptd_DW[2] |= (ptd->ptd_NakTimeoutFrame & 0x1f)<<(3+QHA2S_MUFRAME);
                    } else {
                        ptd->ptd_DW[2] |= (ptd->ptd_NakTimeoutFrame & (0x1f<<3))<<(QHA2S_MUFRAME);
                    }
                    len = 0;
                    if(urti->urti_OutReqHook)
                    {
                        ubr->ubr_Frame = ptd->ptd_NakTimeoutFrame;
                        // Copy buffer to target
                        /* FIXME in continuous mode, whenever ubr_Length was not aligned to longword, all kind of havoc can happen */
                        memptr = &unit->hu_RegBase[ptd->ptd_BufStart];
                        do
                        {
                            CallHookPkt(urti->urti_OutReqHook, urti, ubr);
                            KPRINTF(1, ("Hook->%ld\n", ubr->ubr_Length));
                            if(cnt = (ubr->ubr_Length + 3)>>2)
                            {
                                srcptr = (ULONG *) ubr->ubr_Buffer;
                                do
                                {
                                    *memptr++ = *srcptr++;
                                }
                                while(--cnt);
                                len += ubr->ubr_Length;
                            }
                            if(urti->urti_OutDoneHook)
                            {
                                CallHookPkt(urti->urti_OutDoneHook, urti, ubr);
                            }
                            // automatically advance buffer
                            ubr->ubr_Buffer += ubr->ubr_Length;
                            if(!(ubr->ubr_Flags & UBFF_CONTBUFFER))
                            {
                                break;
                            }
                            // scatter gather rest of buffer
                            ubr->ubr_Flags &= ~UBFF_CONTBUFFER;
                        } while(TRUE);
                    
                        if(ioreq->iouh_Flags & UHFF_SPLITTRANS)
                        {
                            UWORD sbits = 0x01;
                            // only 188 bytes can be set per microframe in the best case
                            // on OUT, we start on the very first, and continue over the number of 188s packets.
                            if(len > 5*188)
                            {
                                sbits = 0x7e;
                            }
                            else if(len > 4*188)
                            {
                                sbits = 0x3e;
                            }
                            else if(len > 3*188)
                            {
                                sbits = 0x1e;
                            }
                            else if(len > 2*188)
                            {
                                sbits = 0x0e;
                            }
                            else if(len > 188)
                            {
                                sbits = 0x06;
                            }
                            if(len > 150)
                            {
                                isomaxpktsize = 188;
                                /*cnt = len>>2;
                                memptr = &unit->hu_RegBase[ptd->ptd_BufStart];
                                do
                                {
                                    *memptr++ = 0x00000000;
                                } while(--cnt);
                                memptr = &unit->hu_RegBase[ptd->ptd_BufStart];
                                *memptr = 0x00550055;*/
                            }
                            ptd->ptd_IntSOFMap = sbits;
                        }
                    }
                    ptd->ptd_DW[0] &= ~(QHA0M_TRANSLEN|QHA0M_MAXPKTLEN);
                    ptd->ptd_DW[0] |= (len<<QHA0S_TRANSLEN)|(isomaxpktsize<<QHA0S_MAXPKTLEN);

                    //KPRINTF(100, ("%ld:%ld\n", READREG(EHCI_FRINDEX)>>3, (dw3 & QHA3M_TRANSCOUNT)>>QHA3S_TRANSCOUNT));
                }
                // update HW PTD
                ispptd[1] = ptd->ptd_DW[1];
                ispptd[2] = ptd->ptd_DW[2];
                ispptd[3] = ptd->ptd_DW[3] = QHA3F_ACTIVE;
                ispptd[4] = ptd->ptd_DW[4] = ptd->ptd_IntSOFMap<<QHA4S_MUSOFACTIVE;
                ispptd[5] = ptd->ptd_DW[5];
                *ispptd = ptd->ptd_DW[0];

                rtn->rtn_NextPTD ^= 1;
            }
        }
        ptdnum++;
        ptdnum &= 31;
    }
}
/* \\\ */

/* /// "uhwScheduleCtrlPTDs()" */
void uhwScheduleCtrlPTDs(struct DenebUnit *unit)
{
    struct DenebDevice *base = unit->hu_Device;
    struct IOUsbHWReq *ioreq;
    UWORD cnt;
    UWORD ptdnum;
    ULONG mask;
    struct PTDNode *ptd;
    volatile ULONG *ispptd;
    volatile ULONG *memptr;
    UWORD devadrep;
    UWORD memoffset;
    ULONG memmask;

    /* *** CTRL Transfers *** */
    ioreq = (struct IOUsbHWReq *) unit->hu_CtrlXFerQueue.mlh_Head;
    while(((struct Node *) ioreq)->ln_Succ)
    {
        devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint;
        /* is endpoint already in use or do we have to wait for next transaction */
        if(unit->hu_DevBusyReq[devadrep])
        {
            KPRINTF(10, ("Endpoint %02lx in use!\n", devadrep));
            ioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ;
            continue;
        }
        if(!unit->hu_ATLFree)
        {
            KPRINTF(20, ("Control Transfer: Out of free PTDs!\n"));
            // out of PTDs!
            break;
        }

        // find free PTD
        ptdnum = 0;
        mask = 1;
        while(!(unit->hu_ATLFree & mask))
        {
            mask <<= 1;
            ptdnum++;
        }

        ptd = &unit->hu_ATLPTDs[ptdnum];
        ptd->ptd_Num = ptdnum;
        ptd->ptd_AllocMap = 0; // no large memory

        // optional data stage
        if(ioreq->iouh_Length)
        {
            // allocate memory
            if(ioreq->iouh_Length > MMAP_SMALLATL_SIZE)
            {
                cnt = MMAP_TOTAL_GRANULES;
                memoffset = MMAP_LARGEAREA;
                memmask = 1;
                do
                {
                    if(unit->hu_LargeAreaFreeMap & memmask)
                    {
                        // free mem found!
                        break;
                    }
                    memmask <<= 1;
                    memoffset += MMAP_GRANULE_SIZE>>2; // longwords!
                } while(--cnt);
                if(!cnt)
                {
                    KPRINTF(20, ("Out of memory for setup data phase %ld bytes\n", ioreq->iouh_Length));
                    // no memory for setup
                    break;
                }
                ptd->ptd_BufStart = memoffset;
                ptd->ptd_BufLen = (MMAP_GRANULE_SIZE < ioreq->iouh_Length) ? MMAP_GRANULE_SIZE : ioreq->iouh_Length;
                ptd->ptd_AllocMap = memmask;
                // allocate the memory
                unit->hu_LargeAreaFreeMap ^= memmask;
            } else {
                ptd->ptd_BufStart = memoffset = MMAP_SMALLATL + (ptdnum<<(MMAP_SMALLATL_BITS-2));
                ptd->ptd_BufLen = ioreq->iouh_Length;
            }
        }

        // *** no going back after this point ***

        Remove((struct Node *)ioreq);

        ptd->ptd_Type = PTDT_SETUP;
        ptd->ptd_IOReq = ioreq;

        // generate PTD
        ispptd = &unit->hu_RegBase[MMAP_ATL_PTDS + (ptdnum<<3)];

        ptd->ptd_DW[0] = (QHA0F_VALID|(8<<QHA0S_TRANSLEN)|(8<<QHA0S_MAXPKTLEN)|QHA0F_MULTI_1)|((ioreq->iouh_Endpoint & 1)<<QHA0B_ENDPOINT0);

        ptd->ptd_DW[1] = ((ioreq->iouh_Endpoint>>1)<<QHA1S_ENDPOINT1_3)|(ioreq->iouh_DevAddr<<QHA1S_DEVADDR)|(QHA1F_TOKEN_SETUP|QHA1F_TT_CONTROL);
        if(ioreq->iouh_Flags & UHFF_SPLITTRANS)
        {
            KPRINTF(10, ("*** SPLIT TRANSACTION to HubPort %ld at Addr %ld\n", ioreq->iouh_SplitHubPort, ioreq->iouh_SplitHubAddr));
            // full speed and low speed handling
            ptd->ptd_DW[1] |= QHA1F_SPLITTRANS|(ioreq->iouh_SplitHubPort<<QHA1S_PORTNUMBER)|(ioreq->iouh_SplitHubAddr<<QHA1S_HUBADDRESS);
            if(ioreq->iouh_Flags & UHFF_LOWSPEED)
            {
                KPRINTF(10, ("*** LOW SPEED ***\n"));
                ptd->ptd_DW[1] |= QHA1F_LOWSPEED;
            }
        }

        ptd->ptd_DW[2] = ((IMAP_SETUPAREA + ptdnum)<<QHA2S_DATAADDR)|(0<<QHA2S_RELOAD);
        ptd->ptd_DW[3] = (0<<QHA3S_NAKCOUNT)|(3<<QHA3S_RETRYCOUNT)|QHA3F_DATA0|QHA3F_ACTIVE;
        ptd->ptd_DW[4] = 0;

        // copy setup data
        memptr = &unit->hu_RegBase[MMAP_SETUPAREA + (ptdnum<<1)];
        *memptr++ = shuffle(((ULONG *) &ioreq->iouh_SetupData)[0]);
        *memptr = shuffle(((ULONG *) &ioreq->iouh_SetupData)[1]);

        // manage endpoint going busy
        unit->hu_DevBusyReq[devadrep] = ioreq;

        // all good, now fire the transfer
        unit->hu_ATLBusy |= mask;
        unit->hu_ATLFree &= ~mask;

        if(ioreq->iouh_Flags & UHFF_NAKTIMEOUT)
        {
            unit->hu_ATLNakMap |= mask;
            ptd->ptd_NakTimeoutFrame = unit->hu_FrameCounter + (ioreq->iouh_NakTimeout<<3);
        }

        // update HW PTD
        ispptd[1] = ptd->ptd_DW[1];
        ispptd[2] = ptd->ptd_DW[2];
        ispptd[3] = ptd->ptd_DW[3];
        ispptd[4] = ptd->ptd_DW[4];
        *ispptd = ptd->ptd_DW[0];
        unit->hu_ATLActive |= mask;


        KPRINTF(1, ("DUMP DW[0] = %08lx\n", ptd->ptd_DW[0]));
        KPRINTF(1, ("DUMP DW[1] = %08lx\n", ptd->ptd_DW[1]));
        KPRINTF(1, ("DUMP DW[2] = %08lx\n", ptd->ptd_DW[2]));
        KPRINTF(1, ("DUMP DW[3] = %08lx\n", ptd->ptd_DW[3]));
        KPRINTF(1, ("DUMP DW[4] = %08lx\n", ptd->ptd_DW[4]));

        WRITEREG(ISP_ATLPTDSKIPMAP, ~unit->hu_ATLBusy);
        KPRINTF(1, ("New ATLSkipMap %08lx\n", READREG(ISP_ATLPTDSKIPMAP)));

        if(ptdnum > unit->hu_LastATL)
        {
            // Update Last PTD bit
            unit->hu_LastATL = ptdnum;
            WRITEREG(ISP_ATLPTDLASTPTD, 1<<unit->hu_LastATL);
            KPRINTF(1, ("New ATLLastPtd %08lx\n", READREG(ISP_ATLPTDLASTPTD)));
        }

        ioreq = (struct IOUsbHWReq *) unit->hu_CtrlXFerQueue.mlh_Head;
    }
}
/* \\\ */

/* /// "uhwScheduleIntPTDs()" */
void uhwScheduleIntPTDs(struct DenebUnit *unit)
{
    struct DenebDevice *base = unit->hu_Device;
    struct IOUsbHWReq *ioreq;
    UWORD cnt;
    UWORD ptdnum;
    ULONG mask;
    struct PTDNode *ptd;
    volatile ULONG *ispptd;
    volatile ULONG *memptr;
    ULONG *srcptr;
    UWORD devadrep;
    UWORD memoffset;
    ULONG memmask;

    /* *** INT Transfers *** */
    ioreq = (struct IOUsbHWReq *) unit->hu_IntXFerQueue.mlh_Head;
    while(((struct Node *) ioreq)->ln_Succ)
    {
        devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
        /* is endpoint already in use or do we have to wait for next transaction */
        if(unit->hu_DevBusyReq[devadrep])
        {
            KPRINTF(10, ("Endpoint %02lx in use!\n", devadrep));
            ioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ;
            continue;
        }
        if(!unit->hu_IntFree)
        {
            KPRINTF(20, ("Interrupt Transfer: Out of free PTDs!\n"));
            // out of PTDs!
            break;
        }

        // find free PTD
        ptdnum = 0;
        mask = 1;
        while(!(unit->hu_IntFree & mask))
        {
            mask <<= 1;
            ptdnum++;
        }

        ptd = &unit->hu_IntPTDs[ptdnum];
        ptd->ptd_Num = ptdnum;
        ptd->ptd_AllocMap = 0; // no large memory

        // allocate memory
        if(ioreq->iouh_Length > MMAP_SMALLINT_SIZE)
        {
            // find free space in large data area
            // only allow one page and allocate backwards
            cnt = MMAP_TOTAL_GRANULES;
            memoffset = MMAP_LARGEAREA + (MMAP_TOTAL_GRANULES-1)*(MMAP_GRANULE_SIZE>>2);
            // FIXME: High bandwidth endpoints will use up to 3 KB buffer!
            memmask = 1<<(MMAP_TOTAL_GRANULES-1);
            do
            {
                if(unit->hu_LargeAreaFreeMap & memmask)
                {
                    // free mem found!
                    break;
                }
                memmask >>= 1;
                memoffset -= MMAP_GRANULE_SIZE>>2; // longwords!
            } while(--cnt);
            if(!cnt)
            {
                KPRINTF(20, ("Out of memory for bigger interrupt with %ld bytes\n", ioreq->iouh_Length));
                // no memory for setup
                break;
            }
            ptd->ptd_BufStart = memoffset;
            ptd->ptd_BufLen = (ioreq->iouh_Length > MMAP_GRANULE_SIZE) ? MMAP_GRANULE_SIZE : ioreq->iouh_Length;
            ptd->ptd_AllocMap = memmask;
            // allocate the memory
            unit->hu_LargeAreaFreeMap ^= memmask;
        } else {
            ptd->ptd_BufStart = memoffset = MMAP_SMALLINT + (ptdnum<<(MMAP_SMALLINT_BITS-2));
            ptd->ptd_BufLen = ioreq->iouh_Length;
        }

        // *** no going back after this point ***

        Remove((struct Node *)ioreq);

        ptd->ptd_IOReq = ioreq;

        // generate PTD
        ispptd = &unit->hu_RegBase[MMAP_INT_PTDS + (ptdnum<<3)];

        ptd->ptd_DW[0] = QHA0F_VALID|(ptd->ptd_BufLen<<QHA0S_TRANSLEN)|(ioreq->iouh_MaxPktSize<<QHA0S_MAXPKTLEN)|((ioreq->iouh_Endpoint & 1)<<QHA0B_ENDPOINT0);
        ptd->ptd_DW[1] = ((ioreq->iouh_Endpoint>>1)<<QHA1S_ENDPOINT1_3)|(ioreq->iouh_DevAddr<<QHA1S_DEVADDR)|(QHA1F_TOKEN_OUT|QHA1F_TT_INTERRUPT);
        ptd->ptd_DW[2] = (MMAP2IMAP(ptd->ptd_BufStart)<<QHA2S_DATAADDR);
        if(ioreq->iouh_Flags & UHFF_SPLITTRANS)
        {
            KPRINTF(10, ("*** SPLIT TRANSACTION to HubPort %ld at Addr %ld\n", ioreq->iouh_SplitHubPort, ioreq->iouh_SplitHubAddr));
            // full speed and low speed handling
            ptd->ptd_DW[1] |= QHA1F_SPLITTRANS|(ioreq->iouh_SplitHubPort<<QHA1S_PORTNUMBER)|(ioreq->iouh_SplitHubAddr<<QHA1S_HUBADDRESS);
            if(ioreq->iouh_Flags & UHFF_LOWSPEED)
            {
                KPRINTF(10, ("*** LOW SPEED ***\n"));
                ptd->ptd_DW[1] |= QHA1F_LOWSPEED;
            }
            ptd->ptd_IntSOFMap = 0x01;
            ptd->ptd_DW[5] = 0x1c<<QHA5S_MUSOFCSPLIT;
            if(ioreq->iouh_Interval < 4)
            {
                ptd->ptd_DW[2] |= 1<<(3+QHA2S_MUFRAME)|(0x01<<QHA2S_MUFRAME);
            }
            else if(ioreq->iouh_Interval < 8)
            {
                ptd->ptd_DW[2] |= 2<<(3+QHA2S_MUFRAME)|(0x01<<QHA2S_MUFRAME);
            }
            else if(ioreq->iouh_Interval < 16)
            {
                ptd->ptd_DW[2] |= 4<<(3+QHA2S_MUFRAME)|(0x01<<QHA2S_MUFRAME);
            }
            else if(ioreq->iouh_Interval < 32)
            {
                ptd->ptd_DW[2] |= 8<<(3+QHA2S_MUFRAME)|(0x01<<QHA2S_MUFRAME);
            } else {
                ptd->ptd_DW[2] |= 16<<(3+QHA2S_MUFRAME)|(0x01<<QHA2S_MUFRAME);
            }
        } else {
            if(ioreq->iouh_Flags & UHFF_MULTI_3)
            {
                ptd->ptd_DW[0] |= QHA0F_MULTI_3;
            }
            else if(ioreq->iouh_Flags & UHFF_MULTI_2)
            {
                ptd->ptd_DW[0] |= QHA0F_MULTI_2;
            } else {
                ptd->ptd_DW[0] |= QHA0F_MULTI_1;
            }
            ptd->ptd_DW[5] = 0;
            // obtain right polling interval
            if(ioreq->iouh_Interval < 2) // 0-1 Frames
            {
                ptd->ptd_IntSOFMap = 0xff;
            }
            else if(ioreq->iouh_Interval < 4) // 2-3 Frames
            {
                ptd->ptd_IntSOFMap = 0x55;
            }
            else if(ioreq->iouh_Interval < 8) // 4-7 Frames
            {
                ptd->ptd_IntSOFMap = 0x22;
            }
            else if(ioreq->iouh_Interval > 511) // 64ms and higher
            {
                ptd->ptd_IntSOFMap = 0x10;
                ptd->ptd_DW[2] |= 16<<(3+QHA2S_MUFRAME);
            }
            else //if(ioreq->iouh_Interval >= 8) // 1-64ms
            {
                ptd->ptd_IntSOFMap = 0x01;
                ptd->ptd_DW[2] |= (ioreq->iouh_Interval>>1)<<QHA2S_MUFRAME;
            }
        }

        if(ioreq->iouh_Dir == UHDIR_IN)
        {
            ptd->ptd_DW[1] |= QHA1F_TOKEN_IN;
            ptd->ptd_Type = PTDT_DATAIN;
        } else {
            //ptd->ptd_DW[1] |= QHA1F_TOKEN_OUT;
            ptd->ptd_Type = PTDT_DATAOUT;

            if(ioreq->iouh_Length)
            {
                // Copy buffer to target
                srcptr = (ULONG *) ioreq->iouh_Data;
                memptr = &unit->hu_RegBase[ptd->ptd_BufStart];
                cnt = (ptd->ptd_BufLen + 3)>>2;
                do
                {
                    *memptr++ = shuffle(*srcptr++);
                }
                while(--cnt);
                KPRINTF(1, ("Interrupt OUT %ld bytes\n", ptd->ptd_BufLen));
            }
        }

        KPRINTF(1, ("Interrupt SOFMap %lx\n", ptd->ptd_IntSOFMap));

        // toggle bit
        ptd->ptd_DW[3] = (QHA3F_ACTIVE|(2<<QHA3S_RETRYCOUNT))|(unit->hu_DevDataToggle[devadrep]<<QHA3B_DATA1);
        ptd->ptd_DW[4] = ptd->ptd_IntSOFMap<<QHA4S_MUSOFACTIVE;

        // manage endpoint going busy
        unit->hu_DevBusyReq[devadrep] = ioreq;

        // all good, now fire the transfer
        unit->hu_IntBusy |= mask;
        unit->hu_IntFree &= ~mask;

        if(ioreq->iouh_Flags & UHFF_NAKTIMEOUT)
        {
            unit->hu_IntNakMap |= mask;
            ptd->ptd_NakTimeoutFrame = unit->hu_FrameCounter + (ioreq->iouh_NakTimeout<<3);
        }

        // update HW PTD
        ispptd[1] = ptd->ptd_DW[1];
        ispptd[2] = ptd->ptd_DW[2];
        ispptd[3] = ptd->ptd_DW[3];
        ispptd[4] = ptd->ptd_DW[4];
        ispptd[5] = ptd->ptd_DW[5];
        *ispptd = ptd->ptd_DW[0];

        KPRINTF(1, ("DUMP DW[0] = %08lx\n", ptd->ptd_DW[0]));
        KPRINTF(1, ("DUMP DW[1] = %08lx\n", ptd->ptd_DW[1]));
        KPRINTF(1, ("DUMP DW[2] = %08lx\n", ptd->ptd_DW[2]));
        KPRINTF(1, ("DUMP DW[3] = %08lx\n", ptd->ptd_DW[3]));
        KPRINTF(1, ("DUMP DW[4] = %08lx\n", ptd->ptd_DW[4]));

        WRITEREG(ISP_INTPTDSKIPMAP, ~unit->hu_IntBusy);
        KPRINTF(1, ("New IntSkipMap %08lx\n", READREG(ISP_INTPTDSKIPMAP)));

        if(ptdnum > unit->hu_LastInt)
        {
            // Update Last PTD bit
            unit->hu_LastInt = ptdnum;
            WRITEREG(ISP_INTPTDLASTPTD, 1<<unit->hu_LastInt);
            KPRINTF(1, ("New IntLastPtd %08lx\n", READREG(ISP_INTPTDLASTPTD)));
        }

        ioreq = (struct IOUsbHWReq *) unit->hu_IntXFerQueue.mlh_Head;
    }
}
/* \\\ */

/* /// "uhwScheduleBulkPTDs()" */
void uhwScheduleBulkPTDs(struct DenebUnit *unit)
{
    struct DenebDevice *base = unit->hu_Device;
    struct IOUsbHWReq *ioreq;
    UWORD cnt;
    UWORD ptdnum;
    ULONG mask;
    struct PTDNode *ptd;
    volatile ULONG *ispptd;
    //volatile ULONG *memptr;
    //ULONG *srcptr;
    UWORD devadrep;
    UWORD memoffset;
    ULONG memmask;
    ULONG memsize;
    BOOL usedma;
#ifndef NODMA
    ULONG tmplength;
#endif

    /* *** BULK Transfers *** */
    ioreq = (struct IOUsbHWReq *) unit->hu_BulkXFerQueue.mlh_Head;
    while(((struct Node *) ioreq)->ln_Succ)
    {
        devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
        /* is endpoint already in use or do we have to wait for next transaction */
        if(unit->hu_DevBusyReq[devadrep])
        {
            KPRINTF(10, ("Endpoint %02lx in use!\n", devadrep));
            ioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ;
            continue;
        }
        if(!unit->hu_ATLFree)
        {
            KPRINTF(20, ("Bulk Transfer: Out of free PTDs!\n"));
            // out of PTDs!
            break;
        }

        KPRINTF(1, ("Bulk transfer %ld bytes to dev=%04lx\n", ioreq->iouh_Length, devadrep));

        // find free PTD
        ptdnum = 0;
        memmask = 0;
        mask = 1;
        while(!(unit->hu_ATLFree & mask))
        {
            mask <<= 1;
            ptdnum++;
        }

        ptd = &unit->hu_ATLPTDs[ptdnum];
        ptd->ptd_Num = ptdnum;
        ptd->ptd_AllocMap = 0; // no large memory
        usedma = FALSE;
        // optional data stage
        if(ioreq->iouh_Length)
        {
            // allocate memory
            if(ioreq->iouh_Length > MMAP_SMALLATL_SIZE)
            {
                usedma = (ioreq->iouh_Length >= ((ioreq->iouh_Dir == UHDIR_IN) ? 4096 : 4096)) && (!unit->hu_DMAPTD);
                if(usedma)
                {
                    if((ioreq->iouh_Dir == UHDIR_IN) && (ioreq->iouh_Length & 3))
                    {
                        // on IN, length must be a long word aligned
                        usedma = FALSE;
                    }
                    if(((ULONG) ioreq->iouh_Data) & 3)
                    {
                        // data address must be long word aligned
                        usedma = FALSE;
                    }
                }
                //usedma = FALSE; // disable for now
                while(usedma)
                {
                    // Use DMA for this transfer, it's large enough
                    KPRINTF(10, ("Using DMA!!!\n"));
                    unit->hu_DMAPTD = ptd;
                    ptd->ptd_LastIn = FALSE;
                    // this is actually a double buffered transfer
                    ptd->ptd_BufStart = MMAP_BULKDMA_LOW;
                    ptd->ptd_BufLen = (MMAP_BULKDMA_SIZE < ioreq->iouh_Length) ? MMAP_BULKDMA_SIZE : ioreq->iouh_Length;
                    ptd->ptd_AllocMap = 0;
                    // initialize DMA transfer
                    unit->hu_DMAAddr = ioreq->iouh_Data;
                    unit->hu_DMAOffset = 0;
#ifndef NODMA
                    tmplength = ioreq->iouh_Length;
#endif
                    if(ioreq->iouh_Dir == UHDIR_IN)
                    {
#ifndef NODMA
                        unit->hu_DMAPhyAddr = CachePreDMA(unit->hu_DMAAddr, &tmplength, 0);
                        if(tmplength < ioreq->iouh_Length) // OS 4 workaround
                        {
                            tmplength = ioreq->iouh_Length;
                            CachePostDMA(unit->hu_DMAAddr, &tmplength, DMA_NoModify);
                            unit->hu_DMAPTD = NULL;
                            break;
                        }
#endif
                        unit->hu_DMAStateMachine = DMASM_IN_WAITING_LOW;
                        ptd->ptd_Type = PTDT_DATAINDMA;
                    } else {
#ifndef NODMA
                        unit->hu_DMAPhyAddr = CachePreDMA(unit->hu_DMAAddr, &tmplength, DMA_ReadFromRAM);
                        if(tmplength < ioreq->iouh_Length) // OS 4 workaround
                        {
                            tmplength = ioreq->iouh_Length;
                            CachePostDMA(unit->hu_DMAAddr, &tmplength, DMA_NoModify|DMA_ReadFromRAM);
                            unit->hu_DMAPTD = NULL;
                            break;
                        }
#endif
                        ptd->ptd_Type = PTDT_DATAOUTDMA;
                        // do the next DMA transfer right after finishing the first one
                        unit->hu_DMAStateMachine = (ptd->ptd_BufLen < ioreq->iouh_Length) ? DMASM_OUT_START_MULTIPLE : DMASM_OUT_START_ONE;

                        // initialize DMA transfer
                        unit->hu_DMALength = (ptd->ptd_BufLen + 3) & ~3;

#ifndef NODMA
                        WRITEREG(ISP_MEMORY, ((MMAP_BULKDMA_LOW<<2)|IMSF_BANK3)|(unit->hu_DMALength<<MMS_LENGTHBYTES));
                        WRITEMACH(MACH_DMAADDRESS, (ULONG) unit->hu_DMAPhyAddr|(MAF_READFROMRAM|MAF_ENABLE));
#else
                        unit->hu_ISPAddr = MMAP_BULKDMA_LOW;
                        Signal(&unit->hu_DMATask, SIGF_SINGLE); // start copy task
#endif
                        KPRINTF(10, ("DMA in progress...\n"));
                    }
                    break;
                }
                if(!usedma)
                {
                    memsize = (ioreq->iouh_Length + MMAP_GRANULE_SIZE - 1)>>MMAP_GRANULE_BITS;
                    if(memsize > MMAP_MAX_GRANULES)
                    {
                        memsize = MMAP_MAX_GRANULES;
                    }

                    // find free space in large data area
                    do
                    {
                        memmask = (1<<memsize) - 1;
                        cnt = MMAP_TOTAL_GRANULES - memsize;
                        memoffset = MMAP_LARGEAREA;
                        do
                        {
                            if((unit->hu_LargeAreaFreeMap & memmask) == memmask)
                            {
                                // free mem found!
                                break;
                            }
                            memmask <<= 1;
                            memoffset += MMAP_GRANULE_SIZE>>2; // longwords!
                        } while(--cnt);
                        if(cnt)
                        {
                            break;
                        }
                    } while(--memsize);

                    if(!memsize)
                    {
                        KPRINTF(20, ("Out of memory for setup data phase %ld bytes\n", ioreq->iouh_Length));
                        // no memory for setup
                        break;
                    }
                    ptd->ptd_BufStart = memoffset;
                    ptd->ptd_BufLen = ((memsize<<MMAP_GRANULE_BITS) < ioreq->iouh_Length) ? (memsize<<MMAP_GRANULE_BITS) : ioreq->iouh_Length;
                    ptd->ptd_AllocMap = memmask;
                    // allocate the memory
                    unit->hu_LargeAreaFreeMap ^= memmask;
                }
            } else {
                ptd->ptd_BufStart = memoffset = MMAP_SMALLATL + (ptdnum<<(MMAP_SMALLATL_BITS-2));
                ptd->ptd_BufLen = ioreq->iouh_Length;
            }
        }

        // *** no going back after this point ***

        Remove((struct Node *)ioreq);

        ptd->ptd_IOReq = ioreq;

        // generate PTD
        ispptd = &unit->hu_RegBase[MMAP_ATL_PTDS + (ptdnum<<3)];

        ptd->ptd_DW[0] = (ptd->ptd_BufLen<<QHA0S_TRANSLEN)|(ioreq->iouh_MaxPktSize<<QHA0S_MAXPKTLEN)|((ioreq->iouh_Endpoint & 1)<<QHA0B_ENDPOINT0);
        ptd->ptd_DW[1] = ((ioreq->iouh_Endpoint>>1)<<QHA1S_ENDPOINT1_3)|(ioreq->iouh_DevAddr<<QHA1S_DEVADDR)|QHA1F_TT_BULK;
        if(!usedma)
        {
            ptd->ptd_Type = (ioreq->iouh_Dir == UHDIR_IN) ? PTDT_DATAIN : PTDT_DATAOUT;
        }
        if(ioreq->iouh_Dir == UHDIR_IN)
        {
            ptd->ptd_DW[1] |= QHA1F_TOKEN_IN;
            unit->hu_DevDataToggle[devadrep] &= 1; // clear PING -- only relevant for OUT
        } else {
            //ptd->ptd_DW[1] |= QHA1F_TOKEN_OUT;

            if((!usedma) && ioreq->iouh_Length)
            {
                // Copy buffer to target
                /*srcptr = (ULONG *) ioreq->iouh_Data;
                memptr = &unit->hu_RegBase[ptd->ptd_BufStart];
                cnt = (ptd->ptd_BufLen + 3)>>2;
                do
                {
                    *memptr++ = *srcptr++;
                }
                while(--cnt);*/
                CopyMemShuffle(ioreq->iouh_Data, (APTR) &unit->hu_RegBase[ptd->ptd_BufStart], (ULONG) (ptd->ptd_BufLen + 3) & ~3);
                KPRINTF(1, ("BULK OUT %ld bytes %ld\n", ptd->ptd_BufLen, unit->hu_DevDataToggle[devadrep]));
            }
        }
        if(ioreq->iouh_Flags & UHFF_SPLITTRANS)
        {
            KPRINTF(10, ("*** SPLIT TRANSACTION to HubPort %ld at Addr %ld\n", ioreq->iouh_SplitHubPort, ioreq->iouh_SplitHubAddr));
            // full speed and low speed handling
            ptd->ptd_DW[1] |= QHA1F_SPLITTRANS|(ioreq->iouh_SplitHubPort<<QHA1S_PORTNUMBER)|(ioreq->iouh_SplitHubAddr<<QHA1S_HUBADDRESS);
            if(ioreq->iouh_Flags & UHFF_LOWSPEED)
            {
                KPRINTF(10, ("*** LOW SPEED ***\n"));
                ptd->ptd_DW[1] |= QHA1F_LOWSPEED;
            }
        } else {
            if(ioreq->iouh_Flags & UHFF_MULTI_3)
            {
                ptd->ptd_DW[0] |= QHA0F_MULTI_3;
            }
            else if(ioreq->iouh_Flags & UHFF_MULTI_2)
            {
                ptd->ptd_DW[0] |= QHA0F_MULTI_2;
            } else {
                ptd->ptd_DW[0] |= QHA0F_MULTI_1;
            }
        }

        ptd->ptd_DW[2] = (MMAP2IMAP(ptd->ptd_BufStart)<<QHA2S_DATAADDR);
        // toggle bit
        ptd->ptd_DW[3] = (QHA3F_ACTIVE|(2<<QHA3S_RETRYCOUNT))|(unit->hu_DevDataToggle[devadrep]<<QHA3B_DATA1); // also sets PING accordingly
        ptd->ptd_DW[4] = 0;

        KPRINTF(1, ("Toggle Bit: %ld\n", unit->hu_DevDataToggle[devadrep]));

        // manage endpoint going busy
        unit->hu_DevBusyReq[devadrep] = ioreq;

        // all good, now fire the transfer
        unit->hu_ATLBusy |= mask;
        unit->hu_ATLFree &= ~mask;

        if(ioreq->iouh_Flags & UHFF_NAKTIMEOUT)
        {
            unit->hu_ATLNakMap |= mask;
            ptd->ptd_NakTimeoutFrame = unit->hu_FrameCounter + (ioreq->iouh_NakTimeout<<3);
        }

        Disable(); // avoid DMA race condition
        if(usedma)
        {
            switch(unit->hu_DMAStateMachine)
            {
                case DMASM_OUT_START_ONE:
                case DMASM_OUT_START_MULTIPLE:
                    KPRINTF(10, ("Fire: DMASM_OUT_START_X\n"));
                    unit->hu_DMAStateMachine = DMASM_OUT_PTD_DONE_HIGH;
                    // start later
                    break;

                case DMASM_OUT_DONE_ONE:
                    KPRINTF(10, ("Fire: DMASM_OUT_DONE_ONE\n"));
                    ptd->ptd_DW[0] |= QHA0F_VALID;
                    unit->hu_DMAStateMachine = DMASM_NOP;
                    break;

                case DMASM_OUT_PREFETCH_1BUF_DONE:
                    KPRINTF(10, ("Fire: DMASM_OUT_PREFETCH_1BUF_DONE\n"));
                    ptd->ptd_DW[0] |= QHA0F_VALID;
                    unit->hu_DMAStateMachine = DMASM_OUT_PREFETCH_HIGH;
                    break;

                case DMASM_OUT_PREFETCH_2BUF_DONE:
                    KPRINTF(10, ("Fire: DMASM_OUT_PREFETCH_2BUF_DONE\n"));
                    ptd->ptd_DW[0] |= QHA0F_VALID;
                    unit->hu_DMAStateMachine = DMASM_OUT_PREFETCH_HIGH_DONE;
                    break;

                case DMASM_IN_WAITING_LOW:
                    ptd->ptd_DW[0] |= QHA0F_VALID;
                    break;

                default:
                    KPRINTF(100, ("Illegal DMA state %ld when attempting to fire PTD\n", unit->hu_DMAStateMachine));

            }
        } else {
            // if DMA is not used or has already finished, fire immediately.
            KPRINTF(1, ("Fire now!\n"));
            ptd->ptd_DW[0] |= QHA0F_VALID;
        }

        // update HW PTD
        ispptd[1] = ptd->ptd_DW[1];
        ispptd[2] = ptd->ptd_DW[2];
        ispptd[3] = ptd->ptd_DW[3];
        ispptd[4] = ptd->ptd_DW[4];
        *ispptd = ptd->ptd_DW[0];
        unit->hu_ATLActive |= mask;

        Enable();

        KPRINTF(1, ("DUMP DW[0] = %08lx\n", ptd->ptd_DW[0]));
        KPRINTF(1, ("DUMP DW[1] = %08lx\n", ptd->ptd_DW[1]));
        KPRINTF(1, ("DUMP DW[2] = %08lx\n", ptd->ptd_DW[2]));
        KPRINTF(1, ("DUMP DW[3] = %08lx\n", ptd->ptd_DW[3]));
        KPRINTF(1, ("DUMP DW[4] = %08lx\n", ptd->ptd_DW[4]));

        WRITEREG(ISP_ATLPTDSKIPMAP, ~unit->hu_ATLBusy);
        KPRINTF(1, ("New ATLSkipMap %08lx\n", READREG(ISP_ATLPTDSKIPMAP)));

        if(ptdnum > unit->hu_LastATL)
        {
            // Update Last PTD bit
            unit->hu_LastATL = ptdnum;
            WRITEREG(ISP_ATLPTDLASTPTD, 1<<unit->hu_LastATL);
            KPRINTF(1, ("New ATLLastPtd %08lx\n", READREG(ISP_ATLPTDLASTPTD)));
        }

        ioreq = (struct IOUsbHWReq *) unit->hu_BulkXFerQueue.mlh_Head;
    }
}
/* \\\ */

/* /// "uhwScheduleIsoPTDs()" */
void uhwScheduleIsoPTDs(struct DenebUnit *unit)
{
    struct DenebDevice *base = unit->hu_Device;
    struct IOUsbHWReq *ioreq;
    UWORD cnt;
    UWORD ptdnum;
    ULONG mask;
    struct PTDNode *ptd;
    volatile ULONG *ispptd;
    volatile ULONG *memptr;
    ULONG *srcptr;
    UWORD devadrep;
    UWORD memoffset;
    ULONG memmask;
    ULONG isomaxpktsize;

    /* *** ISO Transfers *** */
    ioreq = (struct IOUsbHWReq *) unit->hu_IsoXFerQueue.mlh_Head;
    while(((struct Node *) ioreq)->ln_Succ)
    {
        devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
        /* is endpoint already in use or do we have to wait for next transaction */
        if(unit->hu_DevBusyReq[devadrep])
        {
            KPRINTF(10, ("Endpoint %02lx in use!\n", devadrep));
            ioreq = (struct IOUsbHWReq *) ((struct Node *) ioreq)->ln_Succ;
            continue;
        }
        if(!unit->hu_IsoFree)
        {
            KPRINTF(20, ("ISO Transfer: Out of free PTDs!\n"));
            // out of PTDs!
            break;
        }

        // find free PTD
        ptdnum = 0;
        mask = 1;
        while(!(unit->hu_IsoFree & mask))
        {
            mask <<= 1;
            ptdnum++;
        }

        ptd = &unit->hu_IsoPTDs[ptdnum];
        ptd->ptd_Num = ptdnum;
        ptd->ptd_AllocMap = 0; // no large memory

        // allocate memory
        // find free space in large data area
        // only allow one page (ISO can only be up to 1 KB per transfer) and allocate backwards
        cnt = MMAP_TOTAL_GRANULES;
        memoffset = MMAP_LARGEAREA + (MMAP_TOTAL_GRANULES-1)*(MMAP_GRANULE_SIZE>>2);
        memmask = 1<<(MMAP_TOTAL_GRANULES-1);
        do
        {
            if(unit->hu_LargeAreaFreeMap & memmask)
            {
                // free mem found!
                break;
            }
            memmask >>= 1;
            memoffset -= MMAP_GRANULE_SIZE>>2; // longwords!
        } while(--cnt);
        if(!cnt)
        {
            KPRINTF(20, ("Out of memory for iso with %ld/%ld bytes\n", ioreq->iouh_Length, ioreq->iouh_MaxPktSize));
            // no memory for setup
            break;
        }
        ptd->ptd_BufStart = memoffset;
        ptd->ptd_BufLen = (ioreq->iouh_MaxPktSize > ioreq->iouh_Length) ? ioreq->iouh_Length : ioreq->iouh_MaxPktSize;
        ptd->ptd_AllocMap = memmask;
        // allocate the memory
        unit->hu_LargeAreaFreeMap ^= memmask;

        // *** no going back after this point ***

        Remove((struct Node *)ioreq);

        ptd->ptd_IOReq = ioreq;

        // generate PTD
        ispptd = &unit->hu_RegBase[MMAP_ISO_PTDS + (ptdnum<<3)];

        ptd->ptd_DW[0] = QHA0F_VALID|(ptd->ptd_BufLen<<QHA0S_TRANSLEN)|((ioreq->iouh_Endpoint & 1)<<QHA0B_ENDPOINT0);
        ptd->ptd_DW[1] = ((ioreq->iouh_Endpoint>>1)<<QHA1S_ENDPOINT1_3)|(ioreq->iouh_DevAddr<<QHA1S_DEVADDR)|(QHA1F_TOKEN_OUT|QHA1F_TT_ISO);
        if(ioreq->iouh_Flags & UHFF_SPLITTRANS)
        {
            isomaxpktsize = ptd->ptd_BufLen;
            // only 188 bytes can be set per microframe in the best case
            if(ioreq->iouh_Dir == UHDIR_IN)
            {
                if(isomaxpktsize > 192)
                {
                    isomaxpktsize = 192;
                }
                ptd->ptd_DW[5] = 0xfe<<QHA5S_MUSOFCSPLIT;
                ptd->ptd_IntSOFMap = 0x0f;
            } else {
                UWORD sbits = 0x01;
                // on OUT, we start on the very first, and continue over the number of 188s packets.
                if(isomaxpktsize > 5*188)
                {
                    sbits = 0x3f;
                }
                else if(isomaxpktsize > 4*188)
                {
                    sbits = 0x1f;
                }
                else if(isomaxpktsize > 3*188)
                {
                    sbits = 0x0f;
                }
                else if(isomaxpktsize > 2*188)
                {
                    sbits = 0x07;
                }
                else if(isomaxpktsize > 188)
                {
                    sbits = 0x03;
                }
                if(isomaxpktsize > 150)
                {
                    isomaxpktsize = 188;
                }
                ptd->ptd_DW[5] = 0x00<<QHA5S_MUSOFCSPLIT;
                ptd->ptd_IntSOFMap = sbits;
            }
            ptd->ptd_DW[0] |= isomaxpktsize<<QHA0S_MAXPKTLEN;

            KPRINTF(10, ("*** SPLIT TRANSACTION to HubPort %ld at Addr %ld, size=%ld\n", ioreq->iouh_SplitHubPort, ioreq->iouh_SplitHubAddr, isomaxpktsize));
            // full speed and low speed handling
            ptd->ptd_DW[1] |= QHA1F_SPLITTRANS|(ioreq->iouh_SplitHubPort<<QHA1S_PORTNUMBER)|(ioreq->iouh_SplitHubAddr<<QHA1S_HUBADDRESS);
        } else {
            ptd->ptd_DW[0] |= ptd->ptd_BufLen<<QHA0S_MAXPKTLEN;
            /* The Amiga system won't be able to keep up with this data rate anyway
               and we would need 3 KB buffers instead of 1 KB buffers, which would
               make things even more complicated. */
            /*if(ioreq->iouh_Flags & UHFF_MULTI_3)
            {
                ptd->ptd_DW[0] |= QHA0F_MULTI_3;
            }
            else if(ioreq->iouh_Flags & UHFF_MULTI_2)
            {
                ptd->ptd_DW[0] |= QHA0F_MULTI_2;
            } else {
                ptd->ptd_DW[0] |= QHA0F_MULTI_1;
            }*/
            ptd->ptd_DW[5] = 0;
            // obtain right polling interval
            if(ioreq->iouh_Interval < 2) // 0-1 Frames
            {
                ptd->ptd_IntSOFMap = 0xff;
            }
            else if(ioreq->iouh_Interval < 4) // 2-3 Frames
            {
                ptd->ptd_IntSOFMap = 0x55;
            }
            else if(ioreq->iouh_Interval < 8) // 4-7 Frames
            {
                ptd->ptd_IntSOFMap = 0x22;
            }
            else if(ioreq->iouh_Interval > 511) // 64ms and higher
            {
                ptd->ptd_IntSOFMap = 0x10;
            }
            else //if(ioreq->iouh_Interval >= 8) // 1-64ms
            {
                ptd->ptd_IntSOFMap = 0x01;
            }
        }
        // frame number
        ptd->ptd_DW[2] = (MMAP2IMAP(ptd->ptd_BufStart)<<QHA2S_DATAADDR)|(ioreq->iouh_Frame & 0x1f)<<(3+QHA2S_MUFRAME);

        if(ioreq->iouh_Dir == UHDIR_IN)
        {
            ptd->ptd_DW[1] |= QHA1F_TOKEN_IN;
            ptd->ptd_Type = PTDT_DATAIN;
        } else {
            //ptd->ptd_DW[1] |= QHA1F_TOKEN_OUT;
            ptd->ptd_Type = PTDT_DATAOUT;

            if(ioreq->iouh_Length)
            {
                // Copy buffer to target
                srcptr = (ULONG *) ioreq->iouh_Data;
                memptr = &unit->hu_RegBase[ptd->ptd_BufStart];
                cnt = (ptd->ptd_BufLen + 3)>>2;
                do
                {
                    *memptr++ = shuffle(*srcptr++);
                }
                while(--cnt);
                KPRINTF(1, ("Interrupt OUT %ld bytes\n", ptd->ptd_BufLen));
            }
        }

        KPRINTF(1, ("Interrupt SOFMap %lx\n", ptd->ptd_IntSOFMap));

        ptd->ptd_DW[3] = QHA3F_ACTIVE;
        ptd->ptd_DW[4] = ptd->ptd_IntSOFMap<<QHA4S_MUSOFACTIVE;

        // manage endpoint going busy
        unit->hu_DevBusyReq[devadrep] = ioreq;

        // all good, now fire the transfer
        unit->hu_IsoBusy |= mask;
        unit->hu_IsoFree &= ~mask;

        // update HW PTD
        ispptd[1] = ptd->ptd_DW[1];
        ispptd[2] = ptd->ptd_DW[2];
        ispptd[3] = ptd->ptd_DW[3];
        ispptd[4] = ptd->ptd_DW[4];
        ispptd[5] = ptd->ptd_DW[5];
        ispptd[6] = 0;
        ispptd[7] = 0;
        *ispptd = ptd->ptd_DW[0];

        KPRINTF(1, ("DUMP DW[0] = %08lx\n", ptd->ptd_DW[0]));
        KPRINTF(1, ("DUMP DW[1] = %08lx\n", ptd->ptd_DW[1]));
        KPRINTF(1, ("DUMP DW[2] = %08lx\n", ptd->ptd_DW[2]));
        KPRINTF(1, ("DUMP DW[3] = %08lx\n", ptd->ptd_DW[3]));
        KPRINTF(1, ("DUMP DW[4] = %08lx\n", ptd->ptd_DW[4]));

        WRITEREG(ISP_ISOPTDSKIPMAP, ~unit->hu_IsoBusy);
        KPRINTF(1, ("New IsoSkipMap %08lx\n", READREG(ISP_ISOPTDSKIPMAP)));

        if(ptdnum > unit->hu_LastIso)
        {
            // Update Last PTD bit
            unit->hu_LastIso = ptdnum;
            WRITEREG(ISP_ISOPTDLASTPTD, 1<<unit->hu_LastIso);
            KPRINTF(1, ("New IsoLastPtd %08lx\n", READREG(ISP_ISOPTDLASTPTD)));
        }

        ioreq = (struct IOUsbHWReq *) unit->hu_IsoXFerQueue.mlh_Head;
    }
}
/* \\\ */

/* /// "uhwSoftInt()" */
#if defined(__AROS__)
AROS_INTH1(uhwSoftInt, struct DenebUnit *,  unit)
{
    AROS_INTFUNC_INIT
#else
void DECLFUNC_1(uhwSoftInt, a1, struct DenebUnit *, unit)
{
    DECLARG_1(a1, struct DenebUnit *, unit)
#endif
    ULONG framecnt;

    MC030FLUSH;

    /* Update frame counter */
    framecnt = READREG(EHCI_FRINDEX);
    unit->hu_FrameCounter = (unit->hu_FrameCounter & ~0x1fff)|(framecnt & 0x1fff);

    KPRINTF(1, ("SoftInt %ld, ATLDone %08lx\n", unit->hu_FrameCounter, unit->hu_ATLDone));

    /* **************** HANDLE COMPLETED TRANSFERS **************** */

    if(unit->hu_ATLDone || unit->hu_SafeATLCheck)
    {
        uhwHandleFinishedATLs(unit);
    }

    if(unit->hu_IntDone)
    {
        uhwHandleFinishedInts(unit);
    }

    if(unit->hu_IsoDone)
    {
        uhwHandleFinishedIsos(unit);
    }

    /* **************** PROCESS NEW TRANSFERS **************** */

    if(unit->hu_CtrlXFerQueue.mlh_Head->mln_Succ)
    {
        uhwScheduleCtrlPTDs(unit);
    }

    if(unit->hu_IntXFerQueue.mlh_Head->mln_Succ)
    {
        uhwScheduleIntPTDs(unit);
    }

    if(unit->hu_BulkXFerQueue.mlh_Head->mln_Succ)
    {
        uhwScheduleBulkPTDs(unit);
    }

    if(unit->hu_IsoXFerQueue.mlh_Head->mln_Succ)
    {
        uhwScheduleIsoPTDs(unit);
    }

    KPRINTF(1, ("SoftDone\n"));
#if defined(__AROS__)
    return FALSE;

    AROS_INTFUNC_EXIT
#endif
}
/* \\\ */

/* /// "uhwNakTimeoutInt()" */
#if defined(__AROS__)
AROS_INTH1(uhwNakTimeoutInt, struct DenebUnit *,  unit)
{
    AROS_INTFUNC_INIT
#else
void DECLFUNC_1(uhwNakTimeoutInt, a1, struct DenebUnit *, unit)
{
    DECLARG_1(a1, struct DenebUnit *, unit)
#endif
    struct DenebDevice *base = unit->hu_Device;
    struct IOUsbHWReq *ioreq;
    struct PTDNode *ptd;
    volatile ULONG *ispptd;
    ULONG framecnt;
    ULONG busy;
    ULONG ptdnum;
    ULONG ints;
    UWORD devadrep;

    KPRINTF(1, ("NakTimeoutInt()\n"));

#ifdef DEBUG
    if(!((*(volatile UBYTE *) 0xBFE001) & (1<<7)))
    {
        uhwDebugATLs(unit);
    }
#endif
    framecnt = READREG(EHCI_FRINDEX) & 0x1fff;
    /* Update frame counter */
    if(ints = READREG(EHCI_USBSTS))
    {
        WRITEREG(EHCI_USBSTS, ints);
        KPRINTF(1, ("EHCI%08lx\n", ints));
        if(ints & EHSF_FRAMECOUNTOVER)
        {
            framecnt += 0x2000;
            KPRINTF(1, ("Framecount %08lx %ld\n", unit->hu_FrameCounter, unit->hu_FrameCounter));
        }
    }
    unit->hu_FrameCounter = (unit->hu_FrameCounter & ~0x1fff) + framecnt;

    Disable();
    if(busy = unit->hu_ATLActive)
    {
        // this is a workaround for Errata 2 of ISP1760 1 Errata 070108
        ispptd = unit->hu_ReadBasePTDs;
        ptdnum = 0;
        do
        {
            if(busy & 1)
            {
                ULONG dummy;
                WRITEREG(ISP_MEMORY, (MMAP_ATL_PTDS<<2) + (ptdnum<<5));
                if(!(*ispptd & 1)) // check valid bit 0
                {
#ifdef DEBUG
                    uhwDebugATLs(unit);
#endif
                    KPRINTF(100, ("Errata Valid bugfix on PTD %ld\n", ptdnum));
                    WRITEREG(ISP_ATLPTDDONEMAP, 1<<ptdnum);
                    unit->hu_ATLDone |= (1<<ptdnum)|READREG(ISP_ATLPTDDONEMAP);
                    unit->hu_ATLActive &= ~unit->hu_ATLDone;
                    KPRINTF(100, ("(Done=%08lx)\n", unit->hu_ATLDone));
                    Cause(&unit->hu_SoftInt);
                } else {
                    dummy = *ispptd;
                    dummy = *ispptd;
                    //WRITEREG(ISP_MEMORY, ((MMAP_ATL_PTDS+3)<<2) + (ptdnum<<5));
                    if(((LONG) *ispptd) > 0) // check active bit 31
                    {
#ifdef DEBUG
                        uhwDebugATLs(unit);
#endif
                        KPRINTF(100, ("Errata Active bugfix on PTD %ld\n", ptdnum));
                        WRITEREG(ISP_ATLPTDDONEMAP, 1<<ptdnum);
                        unit->hu_ATLDone |= (1<<ptdnum)|READREG(ISP_ATLPTDDONEMAP);
                        unit->hu_ATLActive &= ~unit->hu_ATLDone;
                        KPRINTF(100, ("(Done=%08lx)\n", unit->hu_ATLDone));
                        Cause(&unit->hu_SoftInt);
                    }
                }
            }
            busy >>= 1;
            ptdnum++;
        } while(busy);
    }
    if(unit->hu_ATLNakMap || unit->hu_IntNakMap)
    {
        ispptd = unit->hu_ReadBasePTDs;
        busy = unit->hu_ATLNakMap & unit->hu_ATLActive;
        ptdnum = 0;
        while(busy)
        {
            if(busy & 1)
            {
                ptd = &unit->hu_ATLPTDs[ptdnum];
                if(unit->hu_FrameCounter > ptd->ptd_NakTimeoutFrame)
                {
                    ioreq = ptd->ptd_IOReq;
                    devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
                    // update toggle bit
                    WRITEREG(ISP_MEMORY, ((MMAP_ATL_PTDS+3)<<2) + (ptdnum<<5));
                    unit->hu_DevDataToggle[devadrep] = (*ispptd & (QHA3F_DATA1|QHA3F_PING))>>QHA3B_DATA1;
                    WRITEMEM(MMAP_ATL_PTDS + (ptdnum<<3), 0); // deactive PTD

                    KPRINTF(100, ("ATL Nak timeout %ld > %ld Toggle %ld\n", unit->hu_FrameCounter, ptd->ptd_NakTimeoutFrame, unit->hu_DevDataToggle[devadrep]));
                    ioreq->iouh_Req.io_Error = UHIOERR_NAKTIMEOUT;
                    FreeATL(ptd, unit);
                }
            }
            busy >>= 1;
            ptdnum++;
        }
        busy = unit->hu_IntNakMap;
        ptdnum = 0;
        while(busy)
        {
            if(busy & 1)
            {
                ptd = &unit->hu_IntPTDs[ptdnum];
                if(unit->hu_FrameCounter > ptd->ptd_NakTimeoutFrame)
                {
                    ioreq = ptd->ptd_IOReq;
                    devadrep = (ioreq->iouh_DevAddr<<5) + ioreq->iouh_Endpoint + ((ioreq->iouh_Dir == UHDIR_IN) ? 0x10 : 0);
                    // update toggle bit
                    WRITEREG(ISP_MEMORY, ((MMAP_INT_PTDS+3)<<2) + (ptdnum<<5));
                    unit->hu_DevDataToggle[devadrep] = (*ispptd & QHA3F_DATA1)>>QHA3B_DATA1;
                    WRITEMEM(MMAP_INT_PTDS + (ptdnum<<3), 0); // deactive PTD

                    KPRINTF(10, ("INT Nak timeout %ld > %ld (ptd: %ld)\n", unit->hu_FrameCounter, ptd->ptd_NakTimeoutFrame, ptdnum));
                    KPRINTF(10, ("INTBusyMap %08lx, ATLBusyMap %08lx\n", unit->hu_IntBusy, unit->hu_ATLBusy));
                    KPRINTF(10, ("INTDoneMap %08lx, ATLDoneMap %08lx\n", unit->hu_IntDone, unit->hu_ATLDone));
                    ioreq->iouh_Req.io_Error = UHIOERR_NAKTIMEOUT;
                    FreeInt(ptd, unit);
                }
            }
            busy >>= 1;
            ptdnum++;
        }
    }
    Enable();

    unit->hu_NakTimeoutReq.tr_time.tv_micro = unit->hu_ATLActive ? (100*1000) : (250*1000);
    SendIO((struct IORequest *)&unit->hu_NakTimeoutReq);
#if defined(__AROS__)
    return FALSE;

    AROS_INTFUNC_EXIT
#endif
}
/* \\\ */

/* /// "uhwHandleDMAInt()" */
void uhwHandleDMAInt(struct DenebUnit *unit)
{
    struct DenebDevice *base = unit->hu_Device;
    ULONG tmpval;
    struct PTDNode *ptd = unit->hu_DMAPTD;
    volatile ULONG *ispptd;
#ifndef ZORRO_II
    volatile ULONG *rmemptr = &unit->hu_ReadBaseMem[MMAP_BANK1];
#else
    volatile ULONG *rmemptr = &unit->hu_ReadBaseMem[MMAP_BULKDMA_LOW];
#endif
    struct IOUsbHWReq *ioreq = ptd->ptd_IOReq;

    MC030FLUSH;

#ifdef NODMA
    if(ptd->ptd_Type == PTDT_DATAOUTDMA)
    {
        CopyMemShuffle(unit->hu_DMAAddr, (APTR) &unit->hu_RegBase[unit->hu_ISPAddr], unit->hu_DMALength);
    } else {
        WRITEREG(ISP_MEMORY, (unit->hu_ISPAddr<<2)|IMSF_BANK1);
        CopyMemShuffle((APTR) rmemptr, unit->hu_DMAAddr, unit->hu_DMALength);
    }
#endif
    unit->hu_DMAOffset += unit->hu_DMALength;
    unit->hu_DMAAddr += unit->hu_DMALength;
#ifndef NODMA
    unit->hu_DMAPhyAddr += unit->hu_DMALength;
#endif
    KPRINTF(10, ("DMA Done state=%02lx, len = %ld, new Addr %08lx, new Offset = %ld\n", unit->hu_DMAStateMachine, unit->hu_DMALength, unit->hu_DMAAddr, unit->hu_DMAOffset));

    ispptd = &unit->hu_RegBase[MMAP_ATL_PTDS + (ptd->ptd_Num<<3)];
    Disable();
    switch(unit->hu_DMAStateMachine)
    {
        case DMASM_OUT_START_ONE:
            unit->hu_DMAStateMachine = DMASM_OUT_DONE_ONE;
            break;

        case DMASM_OUT_START_MULTIPLE:
            unit->hu_DMAStateMachine = DMASM_OUT_PREFETCH_1BUF_DONE;
            tmpval = ((ioreq->iouh_Length + 3) & ~3) - unit->hu_DMAOffset;
            if(tmpval > MMAP_BULKDMA_SIZE)
            {
                tmpval = MMAP_BULKDMA_SIZE;
            }
            unit->hu_DMALength = tmpval;

#ifndef NODMA
            WRITEREG(ISP_MEMORY, ((MMAP_BULKDMA_HIGH<<2)|IMSF_BANK3)|(tmpval<<MMS_LENGTHBYTES));
            WRITEMACH(MACH_DMAADDRESS, (ULONG) unit->hu_DMAPhyAddr|(MAF_READFROMRAM|MAF_ENABLE));
#else
            unit->hu_ISPAddr = MMAP_BULKDMA_HIGH;
            Signal(&unit->hu_DMATask, SIGF_SINGLE); // start copy task
#endif
            break;

        case DMASM_OUT_PREFETCH_1BUF_DONE:
            unit->hu_DMAStateMachine = DMASM_OUT_PREFETCH_2BUF_DONE;
            break;

        case DMASM_OUT_PTD_DONE_LOW:
            // activate PTD (HIGH)
            ptd->ptd_DW[0] &= ~QHA0M_TRANSLEN;
            ptd->ptd_DW[0] |= QHA0F_VALID|(ptd->ptd_BufLen<<QHA0S_TRANSLEN);
            ispptd[2] = ptd->ptd_DW[2] = IMAP_BULKDMA_HIGH<<QHA2S_DATAADDR;
            ptd->ptd_DW[3] &= ~(QHA3M_TRANSCOUNT|QHA3M_NAKCOUNT|QHA3M_RETRYCOUNT|QHA3F_COMPLETESPLIT|QHA3F_PID_ERROR|QHA3F_BABBLE_ERROR|QHA3F_HALT);
            ptd->ptd_DW[3] |= (2<<QHA3S_RETRYCOUNT)|QHA3F_ACTIVE;
            // update HW PTD
            ispptd[3] = ptd->ptd_DW[3];
            ispptd[4] = ptd->ptd_DW[4];
            *ispptd = ptd->ptd_DW[0];
            unit->hu_ATLActive |= 1<<ptd->ptd_Num;

            tmpval = ((ioreq->iouh_Length + 3) & ~3) - unit->hu_DMAOffset;
            if(!tmpval)
            {
                // transfer complete
                unit->hu_DMAStateMachine = DMASM_NOP;
                break;
            }
            if(tmpval > MMAP_BULKDMA_SIZE)
            {
                tmpval = MMAP_BULKDMA_SIZE;
            }
            if(tmpval >= MMAP_DMATHRES_SIZE)
            {
                KPRINTF(10, ("DMA DONE: DMASM_OUT_PTD_DONE_LOW, starting LOW DMA\n"));
                unit->hu_DMAStateMachine = DMASM_OUT_PREFETCH_LOW;
                unit->hu_DMALength = tmpval;
#ifndef NODMA
                WRITEREG(ISP_MEMORY, ((MMAP_BULKDMA_LOW<<2)|IMSF_BANK3)|(tmpval<<MMS_LENGTHBYTES));
                WRITEMACH(MACH_DMAADDRESS, (ULONG) unit->hu_DMAPhyAddr|(MAF_READFROMRAM|MAF_ENABLE));
#else
                unit->hu_ISPAddr = MMAP_BULKDMA_LOW;
                Signal(&unit->hu_DMATask, SIGF_SINGLE); // start copy task
#endif
                break;
            }
            unit->hu_DMAStateMachine = DMASM_OUT_PREFETCH_LOW_DONE;
            Enable();
            KPRINTF(10, ("DMA DONE: DMASM_OUT_PTD_DONE_LOW, copying last %ld\n", tmpval));
            CopyMemShuffle(unit->hu_DMAAddr, (APTR) &unit->hu_RegBase[MMAP_BULKDMA_LOW], tmpval);
            unit->hu_DMAOffset += tmpval;
            unit->hu_DMAAddr += tmpval;
#ifndef NODMA
            unit->hu_DMAPhyAddr += tmpval;
#endif

            return;

        case DMASM_OUT_PREFETCH_LOW:
            unit->hu_DMAStateMachine = DMASM_OUT_PREFETCH_LOW_DONE;
            break;

        case DMASM_OUT_PTD_DONE_HIGH:
            // activate PTD (LOW)
            ptd->ptd_DW[0] &= ~QHA0M_TRANSLEN;
            ptd->ptd_DW[0] |= QHA0F_VALID|(ptd->ptd_BufLen<<QHA0S_TRANSLEN);
            ispptd[2] = ptd->ptd_DW[2] = IMAP_BULKDMA_LOW<<QHA2S_DATAADDR;
            ptd->ptd_DW[3] &= ~(QHA3M_TRANSCOUNT|QHA3M_NAKCOUNT|QHA3M_RETRYCOUNT|QHA3F_COMPLETESPLIT|QHA3F_PID_ERROR|QHA3F_BABBLE_ERROR|QHA3F_HALT);
            ptd->ptd_DW[3] |= (2<<QHA3S_RETRYCOUNT)|QHA3F_ACTIVE;
            // update HW PTD
            ispptd[3] = ptd->ptd_DW[3];
            ispptd[4] = ptd->ptd_DW[4];
            *ispptd = ptd->ptd_DW[0];
            unit->hu_ATLActive |= 1<<ptd->ptd_Num;

            tmpval = ((ioreq->iouh_Length + 3) & ~3) - unit->hu_DMAOffset;
            if(!tmpval)
            {
                // transfer complete
                unit->hu_DMAStateMachine = DMASM_NOP;
                break;
            }
            if(tmpval > MMAP_BULKDMA_SIZE)
            {
                tmpval = MMAP_BULKDMA_SIZE;
            }
            if(tmpval >= MMAP_DMATHRES_SIZE)
            {
                KPRINTF(10, ("DMA DONE: DMASM_OUT_PTD_DONE_HIGH, starting HIGH DMA\n"));
                unit->hu_DMAStateMachine = DMASM_OUT_PREFETCH_HIGH;
                unit->hu_DMALength = tmpval;
#ifndef NODMA
                WRITEREG(ISP_MEMORY, ((MMAP_BULKDMA_HIGH<<2)|IMSF_BANK3)|(tmpval<<MMS_LENGTHBYTES));
                WRITEMACH(MACH_DMAADDRESS, (ULONG) unit->hu_DMAPhyAddr|(MAF_READFROMRAM|MAF_ENABLE));
#else
                unit->hu_ISPAddr = MMAP_BULKDMA_HIGH;
                Signal(&unit->hu_DMATask, SIGF_SINGLE); // start copy task
#endif
                break;
            }
            unit->hu_DMAStateMachine = DMASM_OUT_PREFETCH_HIGH_DONE;
            Enable();
            KPRINTF(10, ("DMA DONE: DMASM_OUT_PTD_DONE_HIGH, copying last %ld\n", tmpval));
            CopyMemShuffle(unit->hu_DMAAddr, (APTR) &unit->hu_RegBase[MMAP_BULKDMA_HIGH], tmpval);
            unit->hu_DMAOffset += tmpval;
            unit->hu_DMAAddr += tmpval;
#ifndef NODMA
            unit->hu_DMAPhyAddr += tmpval;
#endif

            return;

        case DMASM_OUT_PREFETCH_HIGH:
            unit->hu_DMAStateMachine = DMASM_OUT_PREFETCH_HIGH_DONE;
            break;

        case DMASM_IN_READING_LOW:
            if(ptd->ptd_LastIn)
            {
                unit->hu_DMAStateMachine = DMASM_NOP;
                FreeATL(ptd, unit);
            } else {
                unit->hu_DMAStateMachine = DMASM_IN_WAITING_HIGH;
            }
            break;

        case DMASM_IN_DONE_HIGH:
            tmpval = ((ioreq->iouh_Actual + 3) & ~3) - unit->hu_DMAOffset;
            if(tmpval >= MMAP_DMATHRES_SIZE)
            {
                if(!ptd->ptd_LastIn)
                {
                    // activate PTD
                    ptd->ptd_DW[0] &= ~QHA0M_TRANSLEN;
                    ptd->ptd_DW[0] |= QHA0F_VALID|(ptd->ptd_BufLen<<QHA0S_TRANSLEN);
                    ispptd[2] = ptd->ptd_DW[2] = IMAP_BULKDMA_LOW<<QHA2S_DATAADDR;
                    ptd->ptd_DW[3] &= ~(QHA3M_TRANSCOUNT|QHA3M_NAKCOUNT|QHA3M_RETRYCOUNT|QHA3F_COMPLETESPLIT|QHA3F_PID_ERROR|QHA3F_BABBLE_ERROR|QHA3F_HALT);
                    ptd->ptd_DW[3] |= (2<<QHA3S_RETRYCOUNT)|QHA3F_ACTIVE;
                    // update HW PTD
                    ispptd[3] = ptd->ptd_DW[3];
                    ispptd[4] = ptd->ptd_DW[4];
                    *ispptd = ptd->ptd_DW[0];
                    unit->hu_ATLActive |= 1<<ptd->ptd_Num;
                }
                unit->hu_DMAStateMachine = DMASM_IN_READING_HIGH;
                unit->hu_DMALength = tmpval;
#ifndef NODMA
                WRITEREG(ISP_MEMORY, ((MMAP_BULKDMA_HIGH<<2)|IMSF_BANK3)|(tmpval<<MMS_LENGTHBYTES));
                WRITEMACH(MACH_DMAADDRESS, (ULONG) unit->hu_DMAPhyAddr|MAF_ENABLE);
#else
                unit->hu_ISPAddr = MMAP_BULKDMA_HIGH;
                Signal(&unit->hu_DMATask, SIGF_SINGLE); // start copy task
#endif
                break;
            }
            Enable();
            WRITEREG(ISP_MEMORY, (MMAP_BULKDMA_HIGH<<2)|IMSF_BANK1);
            CopyMemShuffle((APTR) rmemptr, unit->hu_DMAAddr, tmpval);
            unit->hu_DMAStateMachine = DMASM_NOP;
            FreeATL(ptd, unit);
            return;

        case DMASM_IN_READING_HIGH:
            if(ptd->ptd_LastIn)
            {
                unit->hu_DMAStateMachine = DMASM_NOP;
                FreeATL(ptd, unit);
            } else {
                unit->hu_DMAStateMachine = DMASM_IN_WAITING_LOW;
            }
            break;

        case DMASM_IN_DONE_LOW:
            tmpval = ((ioreq->iouh_Actual + 3) & ~3) - unit->hu_DMAOffset;
            if(tmpval >= MMAP_DMATHRES_SIZE)
            {
                if(!ptd->ptd_LastIn)
                {
                    // activate PTD
                    ptd->ptd_DW[0] &= ~QHA0M_TRANSLEN;
                    ptd->ptd_DW[0] |= QHA0F_VALID|(ptd->ptd_BufLen<<QHA0S_TRANSLEN);
                    ispptd[2] = ptd->ptd_DW[2] = IMAP_BULKDMA_HIGH<<QHA2S_DATAADDR;
                    ptd->ptd_DW[3] &= ~(QHA3M_TRANSCOUNT|QHA3M_NAKCOUNT|QHA3M_RETRYCOUNT|QHA3F_COMPLETESPLIT|QHA3F_PID_ERROR|QHA3F_BABBLE_ERROR|QHA3F_HALT);
                    ptd->ptd_DW[3] |= (2<<QHA3S_RETRYCOUNT)|QHA3F_ACTIVE;
                    // update HW PTD
                    ispptd[3] = ptd->ptd_DW[3];
                    ispptd[4] = ptd->ptd_DW[4];
                    *ispptd = ptd->ptd_DW[0];
                    unit->hu_ATLActive |= 1<<ptd->ptd_Num;
                }
                unit->hu_DMAStateMachine = DMASM_IN_READING_LOW;
                unit->hu_DMALength = tmpval;
#ifndef NODMA
                WRITEREG(ISP_MEMORY, ((MMAP_BULKDMA_LOW<<2)|IMSF_BANK3)|(tmpval<<MMS_LENGTHBYTES));
                WRITEMACH(MACH_DMAADDRESS, (ULONG) unit->hu_DMAPhyAddr|MAF_ENABLE);
#else
                unit->hu_ISPAddr = MMAP_BULKDMA_LOW;
                Signal(&unit->hu_DMATask, SIGF_SINGLE); // start copy task
#endif
                break;
            }
            Enable();
            WRITEREG(ISP_MEMORY, (MMAP_BULKDMA_LOW<<2)|IMSF_BANK1);
            CopyMemShuffle((APTR) rmemptr, unit->hu_DMAAddr, tmpval);
            unit->hu_DMAStateMachine = DMASM_NOP;
            FreeATL(ptd, unit);
            return;

        default:
            KPRINTF(100, ("Illegal DMA state %ld with incoming DMA INT\n", unit->hu_DMAStateMachine));
    }
    Enable();
    KPRINTF(10, ("DMA NEW State=%02lx\n", unit->hu_DMAStateMachine));
}
/* \\\ */

/* /// "uhwLevel6Int()" */
#if defined(__AROS__)
AROS_INTH1(uhwLevel6Int, struct DenebUnit *,  unit)
{
    AROS_INTFUNC_INIT
#else
ULONG DECLFUNC_1(uhwLevel6Int, a1, struct DenebUnit *, unit)
{
    DECLARG_1(a1, struct DenebUnit *, unit)
#endif
    struct DenebDevice *base = unit->hu_Device;
    ULONG tmpval;
    ULONG ints;

    MC030FLUSH;
    //KPRINTF(1, ("(Poff)\n"));
#ifndef NODMA
    if(ints = READMACH(MACH_DMAINT))
    {
        KPRINTF(5, ("MACH INT %08lx\n", ints));
        WRITEMACH(MACH_DMAINT, ints);
        if(ints & MIF_DMAINT)
        {
            // handle DMA
            if(unit->hu_DMAPTD)
            {
                Signal(&unit->hu_DMATask, SIGF_SINGLE);
#if defined(__AROS__)
                return TRUE;
#else
                return(1);
#endif
            }
        }
    }
#endif
    if(ints = READREG(ISP_INTR))
    {
        UWORD causeit = 0;
        if(ints & unit->hu_IntMask)
        {
            KPRINTF(1, ("INT%08lx\n", ints));
            if(ints & IINTF_INT_DONE)
            {
                ++causeit;
                tmpval = READREG(ISP_INTPTDDONEMAP);
                unit->hu_IntDone |= tmpval;
                KPRINTF(5, ("INTDoneMap %08lx\n", unit->hu_IntDone));
            }
            if(ints & IINTF_ATL_DONE)
            {
                ++causeit;
                tmpval = READREG(ISP_ATLPTDDONEMAP);
                unit->hu_ATLDone |= tmpval;
                unit->hu_ATLActive &= ~unit->hu_ATLDone;
                if(unit->hu_ATLActive)
                {
                    unit->hu_SafeATLCheck = TRUE;
                }
                KPRINTF(5, ("ATLDoneMap %08lx\n", unit->hu_ATLDone));
            }
            if(ints & IINTF_ISO_DONE)
            {
                tmpval = READREG(ISP_ISOPTDDONEMAP) & unit->hu_IsoBusy;
                if(tmpval & unit->hu_IsoRTMask)
                {
                    unit->hu_IsoRTDone |= tmpval & unit->hu_IsoRTMask;
                    KPRINTF(5, ("RTIsoDoneMap %08lx\n", unit->hu_IsoRTDone));
                    uhwHandleFinishedRTIsos(unit);
                }
                if(tmpval & ~unit->hu_IsoRTMask)
                {
                    ++causeit;
                    unit->hu_IsoDone |= tmpval & ~unit->hu_IsoRTMask;
                    KPRINTF(5, ("ISODoneMap %08lx\n", unit->hu_IsoDone));
                }
            }
            /*if(ints & IINTF_DMA_DONE)
            {
                // handle DMA
                if(unit->hu_DMAPTD)
                {
                    uhwHandleDMAInt(unit);
                }
            }*/
            if(causeit)
            {
                Cause(&unit->hu_SoftInt);
            }
        }
        WRITEREG(ISP_INTR, ints);
#if defined(__AROS__)
        return TRUE;
#else
        return(1);
#endif
    }
#if !defined(__AROS__)
    return(0);
#else
    return FALSE;

    AROS_INTFUNC_EXIT
#endif
}
/* \\\ */

/* /// "uhwDMATask()" */
#if defined(__AROS__)
void uhwDMATask()
#else
void DECLFUNC_0(uhwDMATask)
#endif
{
    struct Task *thistask;
    struct DenebUnit *unit;
    ULONG sigs;

#if !defined(__AROS__)
#undef SysBase
    struct Library *SysBase = (*(struct Library **) (4L));
#endif
    thistask = FindTask(NULL);
    unit = thistask->tc_UserData;

    do
    {
        sigs = Wait(SIGBREAKF_CTRL_C|SIGF_SINGLE);
        if(sigs & SIGF_SINGLE)
        {
            if(unit->hu_DMAPTD)
            {
                uhwHandleDMAInt(unit);
            }
        }
    } while(!(sigs & SIGBREAKF_CTRL_C));
}
/* \\\ */

