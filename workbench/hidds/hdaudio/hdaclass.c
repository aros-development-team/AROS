/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    (C) Copyright 2009-2010 Stephen Jones.
    (C) Copyright xxxx-2009 Davy Wentzler.

    Desc: HD Audio controller hidd class

    Controller-level portions derived from the HDAudio AHI driver by
    Davy Wentzler / Stephen Jones.
*/

#include <aros/debug.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/interrupts.h>
#include <exec/execbase.h>
#include <devices/timer.h>
#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include <aros/asmcall.h>

#include <hidd/hidd.h>
#include <hidd/hda.h>

#include "hda_intern.h"

#include LC_LIBDEFS_FILE

#define HSD(cl) (&((LIBBASETYPEPTR)cl->UserData)->hsd)

static VOID HDA_Shutdown(OOP_Class *cl, OOP_Object *o);

#undef HiddHDAAttrBase
#define HiddHDAAttrBase (hsd->hdaAttrBase)
#undef UtilityBase
#define UtilityBase (hsd->utilityBase)

/*
 * Delay by waiting on timer.device; usable from task context only.
 */
VOID hda_usleep(ULONG usec)
{
    struct MsgPort *mp;
    struct timerequest *tr;

    mp = CreateMsgPort();
    if (!mp)
        return;

    tr = (struct timerequest *)CreateIORequest(mp, sizeof(struct timerequest));
    if (tr)
    {
        if (OpenDevice("timer.device", UNIT_MICROHZ,
                       (struct IORequest *)tr, 0) == 0)
        {
            tr->tr_node.io_Command = TR_ADDREQUEST;
            tr->tr_time.tv_secs = usec / 1000000;
            tr->tr_time.tv_micro = usec % 1000000;
            DoIO((struct IORequest *)tr);
            CloseDevice((struct IORequest *)tr);
        }
        DeleteIORequest((struct IORequest *)tr);
    }
    DeleteMsgPort(mp);
}

/* MMIO accessors */
static inline ULONG hda_readl(struct HDAData *hc, ULONG reg)
{
    return *(volatile ULONG *)(hc->hc_MMIO + reg);
}

static inline UWORD hda_readw(struct HDAData *hc, ULONG reg)
{
    return *(volatile UWORD *)(hc->hc_MMIO + reg);
}

static inline UBYTE hda_readb(struct HDAData *hc, ULONG reg)
{
    return *(volatile UBYTE *)(hc->hc_MMIO + reg);
}

static inline void hda_writel(struct HDAData *hc, ULONG reg, ULONG val)
{
    *(volatile ULONG *)(hc->hc_MMIO + reg) = val;
}

static inline void hda_writew(struct HDAData *hc, ULONG reg, UWORD val)
{
    *(volatile UWORD *)(hc->hc_MMIO + reg) = val;
}

static inline void hda_writeb(struct HDAData *hc, ULONG reg, UBYTE val)
{
    *(volatile UBYTE *)(hc->hc_MMIO + reg) = val;
}

static inline void hda_setb(struct HDAData *hc, ULONG reg, UBYTE bits)
{
    hda_writeb(hc, reg, hda_readb(hc, reg) | bits);
}

static inline void hda_clearb(struct HDAData *hc, ULONG reg, UBYTE bits)
{
    hda_writeb(hc, reg, hda_readb(hc, reg) & ~bits);
}

static inline void hda_setl(struct HDAData *hc, ULONG reg, ULONG bits)
{
    hda_writel(hc, reg, hda_readl(hc, reg) | bits);
}

static inline void hda_clearl(struct HDAData *hc, ULONG reg, ULONG bits)
{
    hda_writel(hc, reg, hda_readl(hc, reg) & ~bits);
}

/*
 * DMA cache maintenance. On coherent platforms these degenerate to
 * address translation; on non-coherent ones (e.g. riscv64 with Zicbom)
 * they perform the required cache operations. The address returned by
 * CachePreDMA is what the device must be given.
 */
static APTR hda_dma_todevice(APTR addr, ULONG len)
{
    ULONG l = len;
    return CachePreDMA(addr, &l, DMA_ReadFromRAM);
}

static APTR hda_dma_fromdevice_prepare(APTR addr, ULONG len)
{
    ULONG l = len;
    return CachePreDMA(addr, &l, 0);
}

static void hda_dma_fromdevice_done(APTR addr, ULONG len)
{
    ULONG l = len;
    CachePostDMA(addr, &l, 0);
}

/*
 * Allocate zeroed memory aligned to `boundary`; the raw allocation is
 * returned through `unaligned` for FreeVec().
 */
static APTR hda_alloc_aligned(ULONG size, APTR *unaligned, ULONG boundary)
{
    APTR mem = AllocVec(size + boundary, MEMF_PUBLIC | MEMF_CLEAR);

    *unaligned = mem;
    if (mem)
        mem = (APTR)(((IPTR)mem + boundary - 1) & ~((IPTR)boundary - 1));

    return mem;
}

/******************************************************************************
** Interrupt and reset handlers ***********************************************
******************************************************************************/

static AROS_INTH1(HDA_IntCode, struct HDAData *, hc)
{
    AROS_INTFUNC_INIT

    ULONG intsts = hda_readl(hc, HD_INTSTS);
    LONG handled = 0;
    int i;

    if (intsts & HD_INTSTS_GIS)
    {
        if (intsts & 0x3FFFFFFF)
        {
            for (i = 0; i < hc->hc_NumStreams; i++)
            {
                struct HDAStream *hs = &hc->hc_Streams[i];

                if (intsts & (1UL << i))
                {
                    hda_writeb(hc, hs->hs_SDOffset + HD_SD_OFFSET_STATUS,
                               HD_SD_STATUS_MASK);

                    if (hs->hs_InUse && hs->hs_ClientInt)
                        Cause(hs->hs_ClientInt);
                }
            }

            hda_writeb(hc, HD_INTSTS, 0xFF);
        }

        if (intsts & HD_INTSTS_CIS)
        {
            UBYTE rirbsts;

            hda_writeb(hc, HD_INTSTS + 3, 0x4); /* only byte access allowed */

            rirbsts = hda_readb(hc, HD_RIRBSTS);
            if (rirbsts & 0x5)
            {
                if (rirbsts & 0x1) /* RINTFL */
                    hc->hc_RIRBIrq++;

                hda_writeb(hc, HD_RIRBSTS, rirbsts);
            }
        }

        handled = 1;
    }

    return handled;

    AROS_INTFUNC_EXIT
}

static AROS_INTH1(HDA_ResetHandler, struct HDAData *, hc)
{
    AROS_INTFUNC_INIT

    int i;

    hda_writel(hc, HD_INTCTL, 0);

    for (i = 0; i < hc->hc_NumStreams; i++)
    {
        struct HDAStream *hs = &hc->hc_Streams[i];

        if (hs->hs_Running)
            hda_clearb(hc, hs->hs_SDOffset + HD_SD_OFFSET_CONTROL,
                       HD_SD_CONTROL_STREAM_RUN);
    }

    return 0;

    AROS_INTFUNC_EXIT
}

/******************************************************************************
** Controller bring-up ********************************************************
******************************************************************************/

static BOOL HDA_ResetChip(struct HDAData *hc)
{
    int counter;

    hda_writeb(hc, HD_CORBCTL, 0);
    hda_writeb(hc, HD_RIRBCTL, 0);

    /* After reset STATESTS holds the IDs of the connected codecs */
    hda_writeb(hc, HD_STATESTS, 0xFF);

    hda_clearl(hc, HD_GCTL, 1);

    for (counter = 0; counter < 1000; counter++)
    {
        if ((hda_readb(hc, HD_GCTL) & 0x1) == 0)
            break;
        hda_usleep(100);
    }

    if (counter == 1000)
    {
        D(bug("[HDA] Controller would not enter reset!\n"));
        return FALSE;
    }

    hda_usleep(100);
    hda_setl(hc, HD_GCTL, 1);

    for (counter = 0; counter < 1000; counter++)
    {
        if ((hda_readb(hc, HD_GCTL) & 0x1) == 1)
            break;
        hda_usleep(100);
    }

    if (counter == 1000)
    {
        D(bug("[HDA] Controller stuck in reset!\n"));
        return FALSE;
    }

    /* Wait for the codecs to request state change (min. 521us, be generous) */
    hda_usleep(1000);

    return TRUE;
}

static BOOL HDA_InitCORB(struct HDAData *hc)
{
    UBYTE sizereg;
    APTR phys;

    /* 4.4.1.3 Initialize the CORB */
    hda_clearb(hc, HD_CORBCTL, HD_CORBRUN);

    sizereg = hda_readb(hc, HD_CORBSIZE);
    if (sizereg & (1 << 6))
    {
        hda_writeb(hc, HD_CORBSIZE, 0x2);
        hc->hc_CORBEntries = 256;
    }
    else if (sizereg & (1 << 5))
    {
        hda_writeb(hc, HD_CORBSIZE, 0x1);
        hc->hc_CORBEntries = 16;
    }
    else
    {
        hda_writeb(hc, HD_CORBSIZE, 0x0);
        hc->hc_CORBEntries = 2;
    }

    hc->hc_CORB = hda_alloc_aligned(4 * hc->hc_CORBEntries,
                                    &hc->hc_CORBUnaligned, 128);
    if (!hc->hc_CORB)
        return FALSE;

    phys = hda_dma_todevice(hc->hc_CORB, 4 * hc->hc_CORBEntries);

    hda_writel(hc, HD_CORB_LOW, (ULONG)((IPTR)phys & 0xFFFFFFFF));
#if (__WORDSIZE == 64)
    hda_writel(hc, HD_CORB_HIGH, (ULONG)(((IPTR)phys >> 32) & 0xFFFFFFFF));
#else
    hda_writel(hc, HD_CORB_HIGH, 0);
#endif

    hda_writew(hc, HD_CORBWP, 0);
    hda_setb(hc, HD_CORBCTL, HD_CORBRUN);

    return TRUE;
}

static BOOL HDA_InitRIRB(struct HDAData *hc)
{
    UBYTE sizereg;
    APTR phys;

    /* 4.4.2.2 Initialize the RIRB */
    hda_clearb(hc, HD_RIRBCTL, HD_RIRBRUN);

    sizereg = hda_readb(hc, HD_RIRBSIZE);
    if (sizereg & (1 << 6))
    {
        hda_writeb(hc, HD_RIRBSIZE, 0x2);
        hc->hc_RIRBEntries = 256;
    }
    else if (sizereg & (1 << 5))
    {
        hda_writeb(hc, HD_RIRBSIZE, 0x1);
        hc->hc_RIRBEntries = 16;
    }
    else
    {
        hda_writeb(hc, HD_RIRBSIZE, 0x0);
        hc->hc_RIRBEntries = 2;
    }

    hc->hc_RIRBIrq = 0;
    hc->hc_RIRBReadPos = 0;

    hc->hc_RIRB = hda_alloc_aligned(4 * 2 * hc->hc_RIRBEntries,
                                    &hc->hc_RIRBUnaligned, 128);
    if (!hc->hc_RIRB)
        return FALSE;

    phys = hda_dma_fromdevice_prepare(hc->hc_RIRB, 4 * 2 * hc->hc_RIRBEntries);

    hda_writel(hc, HD_RIRB_LOW, (ULONG)((IPTR)phys & 0xFFFFFFFF));
#if (__WORDSIZE == 64)
    hda_writel(hc, HD_RIRB_HIGH, (ULONG)(((IPTR)phys >> 32) & 0xFFFFFFFF));
#else
    hda_writel(hc, HD_RIRB_HIGH, 0);
#endif

    /* Interrupt for every response */
    hda_writew(hc, HD_RINTCNT, 1);
    hda_writeb(hc, HD_RIRBSTS, 0x5);
    hda_setb(hc, HD_RIRBCTL, HD_RIRBRUN | HD_RINTCTL | 0x4);

    return TRUE;
}

static BOOL HDA_InitStreams(struct HDAData *hc)
{
    UWORD gcap = hda_readw(hc, HD_GCAP);
    int i;

    hc->hc_NumInStreams = (gcap & HD_GCAP_ISS_MASK) >> 8;
    hc->hc_NumOutStreams = (gcap & HD_GCAP_OSS_MASK) >> 12;
    hc->hc_NumStreams = hc->hc_NumInStreams + hc->hc_NumOutStreams;

    D(bug("[HDA] Streams: %u in, %u out\n",
          hc->hc_NumInStreams, hc->hc_NumOutStreams));

    if (!hc->hc_NumStreams)
        return FALSE;

    hc->hc_Streams = AllocVec(sizeof(struct HDAStream) * hc->hc_NumStreams,
                              MEMF_PUBLIC | MEMF_CLEAR);
    if (!hc->hc_Streams)
        return FALSE;

    for (i = 0; i < hc->hc_NumStreams; i++)
    {
        struct HDAStream *hs = &hc->hc_Streams[i];

        hs->hs_Ctrl = hc;
        hs->hs_SDOffset = HD_SD_BASE_OFFSET + HD_SD_DESCRIPTOR_SIZE * i;
        hs->hs_Index = i;
        hs->hs_Tag = i + 1;
        hs->hs_Direction = (i < hc->hc_NumInStreams) ?
            vHidd_HDA_StreamDir_In : vHidd_HDA_StreamDir_Out;

        hda_writeb(hc, hs->hs_SDOffset + HD_SD_OFFSET_STATUS,
                   HD_SD_STATUS_MASK);
    }

    return TRUE;
}

/*
 * Bring the controller up. Called by the subclass once from its
 * Root New, after its own instance data is in place: the hardware
 * mapping, bus quirks and interrupt installation are delegated to
 * the subclass through the HWInit override.
 */
BOOL HDA__Hidd_HDA__Setup(OOP_Class *cl, OOP_Object *o,
        struct pHidd_HDA_Setup *msg)
{
    struct hda_staticdata *hsd = HSD(cl);
    struct HDAData *hc = OOP_INST_DATA(cl, o);
    APTR mmio;

    if (hc->hc_HWInit)
        return TRUE;

    hc->hc_Interrupt.is_Node.ln_Type = NT_INTERRUPT;
    hc->hc_Interrupt.is_Node.ln_Pri = 0;
    hc->hc_Interrupt.is_Node.ln_Name = "HDA controller";
    hc->hc_Interrupt.is_Code = (VOID_FUNC)HDA_IntCode;
    hc->hc_Interrupt.is_Data = hc;

    mmio = HIDD_HDA_HWInit(o, &hc->hc_Interrupt);
    if (!mmio)
    {
        D(bug("[HDA] HWInit failed\n"));
        return FALSE;
    }
    hc->hc_MMIO = (volatile UBYTE *)mmio;

    D(bug("[HDA] registers @ 0x%p\n", hc->hc_MMIO));

    if (!HDA_ResetChip(hc))
        goto fail;

    /* 4.3 Codec discovery */
    hc->hc_CodecMask = hda_readw(hc, HD_STATESTS) & 0x7FFF;
    D(bug("[HDA] Codec mask %04x\n", hc->hc_CodecMask));

    if (!hc->hc_CodecMask)
    {
        D(bug("[HDA] No codecs present!\n"));
        goto fail;
    }

    if (!HDA_InitStreams(hc))
        goto fail;

    if (!HDA_InitCORB(hc))
        goto fail;

    if (!HDA_InitRIRB(hc))
        goto fail;

    /* DMA position buffer unused */
    hda_writel(hc, HD_DPLBASE, 0);
    hda_writel(hc, HD_DPUBASE, 0);

    hda_writew(hc, HD_WAKEEN, 0);
    hda_writeb(hc, HD_INTSTS, 0xFF);

    hda_writel(hc, HD_INTCTL, HD_INTCTL_CIE | HD_INTCTL_GIE);
    hda_usleep(200);

    hc->hc_ResetHandler.is_Node.ln_Type = NT_INTERRUPT;
    hc->hc_ResetHandler.is_Node.ln_Pri = 0;
    hc->hc_ResetHandler.is_Node.ln_Name = "HDA reset handler";
    hc->hc_ResetHandler.is_Code = (VOID_FUNC)HDA_ResetHandler;
    hc->hc_ResetHandler.is_Data = hc;
    hc->hc_ResetHandlerAdded = AddResetCallback(&hc->hc_ResetHandler);

    hc->hc_HWInit = TRUE;
    return TRUE;

fail:
    HDA_Shutdown(cl, o);
    return FALSE;
}

static VOID HDA_Shutdown(OOP_Class *cl, OOP_Object *o)
{
    struct HDAData *hc = OOP_INST_DATA(cl, o);
    int i;

    if (hc->hc_MMIO)
    {
        hda_writel(hc, HD_INTCTL, 0);

        if (hc->hc_Streams)
        {
            for (i = 0; i < hc->hc_NumStreams; i++)
            {
                struct HDAStream *hs = &hc->hc_Streams[i];

                if (hs->hs_Running)
                    hda_clearb(hc, hs->hs_SDOffset + HD_SD_OFFSET_CONTROL,
                               HD_SD_CONTROL_STREAM_RUN);
            }
        }

        hda_clearb(hc, HD_CORBCTL, HD_CORBRUN);
        hda_clearb(hc, HD_RIRBCTL, HD_RIRBRUN);
    }

    if (hc->hc_ResetHandlerAdded)
    {
        RemResetCallback(&hc->hc_ResetHandler);
        hc->hc_ResetHandlerAdded = FALSE;
    }

    if (hc->hc_MMIO)
        HIDD_HDA_HWExit(o, &hc->hc_Interrupt);

    if (hc->hc_CORBUnaligned)
    {
        FreeVec(hc->hc_CORBUnaligned);
        hc->hc_CORBUnaligned = NULL;
        hc->hc_CORB = NULL;
    }

    if (hc->hc_RIRBUnaligned)
    {
        FreeVec(hc->hc_RIRBUnaligned);
        hc->hc_RIRBUnaligned = NULL;
        hc->hc_RIRB = NULL;
    }

    if (hc->hc_Streams)
    {
        FreeVec(hc->hc_Streams);
        hc->hc_Streams = NULL;
        hc->hc_NumStreams = 0;
    }

    hc->hc_MMIO = NULL;
    hc->hc_HWInit = FALSE;
}

/*
 * Default hardware overrides: the base class is abstract, a subclass
 * must implement how its bus attaches the controller.
 */
APTR HDA__Hidd_HDA__HWInit(OOP_Class *cl, OOP_Object *o,
        struct pHidd_HDA_HWInit *msg)
{
    D(bug("[HDA] %s: no hardware implementation!\n", __func__));

    return NULL;
}

VOID HDA__Hidd_HDA__HWExit(OOP_Class *cl, OOP_Object *o,
        struct pHidd_HDA_HWExit *msg)
{
}

/******************************************************************************
** Root class interface *******************************************************
******************************************************************************/

OOP_Object *HDA__Root__New(OOP_Class *cl, OOP_Object *o,
        struct pRoot_New *msg)
{
    struct hda_staticdata *hsd = HSD(cl);

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        struct HDAData *hc = OOP_INST_DATA(cl, o);

        hc->hc_DeviceData = (APTR)GetTagData(aHidd_HDA_DeviceData, 0,
                                             msg->attrList);

        InitSemaphore(&hc->hc_VerbLock);
    }

    return o;
}

VOID HDA__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    HDA_Shutdown(cl, o);

    OOP_DoSuperMethod(cl, o, msg);
}

VOID HDA__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct hda_staticdata *hsd = HSD(cl);
    struct HDAData *hc = OOP_INST_DATA(cl, o);
    ULONG idx;

    Hidd_HDA_Switch(msg->attrID, idx)
    {
    case aoHidd_HDA_DeviceData:
        *msg->storage = (IPTR)hc->hc_DeviceData;
        return;

    case aoHidd_HDA_CodecMask:
        *msg->storage = hc->hc_CodecMask;
        return;

    case aoHidd_HDA_InputStreams:
        *msg->storage = hc->hc_NumInStreams;
        return;

    case aoHidd_HDA_OutputStreams:
        *msg->storage = hc->hc_NumOutStreams;
        return;
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/******************************************************************************
** Verb interface *************************************************************
******************************************************************************/

static ULONG HDA_GetResponse(struct HDAData *hc)
{
    const int timeout = 10000;
    int i;
    UBYTE rirb_wp = 0;

    hda_usleep(20);

    for (i = 0; i < timeout; i++)
    {
        if (hc->hc_RIRBIrq > 0)
        {
            hc->hc_RIRBIrq--;
            break;
        }
        hda_usleep(10);
    }

    if (i == timeout)
    {
        D(bug("[HDA] No response IRQ!\n"));
    }

    for (i = 0; i < timeout; i++)
    {
        rirb_wp = hda_readb(hc, HD_RIRBWP);

        if (rirb_wp == hc->hc_RIRBReadPos)
        {
            /* Strange, we expect the WP to have advanced */
            D(bug("[HDA] RIRB WP has not advanced! wp = %u, rp = %u\n",
                  rirb_wp, hc->hc_RIRBReadPos));
            hda_usleep(5000);
        }
        else
        {
            ULONG addr, response, response_ex;

            hc->hc_RIRBReadPos = rirb_wp;
            addr = hc->hc_RIRBReadPos * 2; /* 64-bit entries */

            hda_dma_fromdevice_done(&hc->hc_RIRB[addr], 8);
            response = hc->hc_RIRB[addr];
            response_ex = hc->hc_RIRB[addr + 1];

            if (response_ex & 0x10)
            {
                D(bug("[HDA] Unsolicited response, skipping\n"));
            }
            else
                return response;
        }
    }

    D(bug("[HDA] No response! rp = %u, wp = %u\n",
          hc->hc_RIRBReadPos, rirb_wp));
    return 0;
}

ULONG HDA__Hidd_HDA__SendCommand(OOP_Class *cl, OOP_Object *o,
        struct pHidd_HDA_SendCommand *msg)
{
    struct HDAData *hc = OOP_INST_DATA(cl, o);
    ULONG response;
    UWORD wp;

    if (!hc->hc_HWInit)
        return 0;

    ObtainSemaphore(&hc->hc_VerbLock);

    wp = hda_readw(hc, HD_CORBWP) & 0xFF;
    if (wp == hc->hc_CORBEntries - 1)
        wp = 0;
    else
        wp++;

    hc->hc_CORB[wp] = msg->command;
    hda_dma_todevice(&hc->hc_CORB[wp], 4);
    hda_writew(hc, HD_CORBWP, wp);

    response = HDA_GetResponse(hc);

    ReleaseSemaphore(&hc->hc_VerbLock);

    return response;
}

/******************************************************************************
** Stream interface ***********************************************************
******************************************************************************/

static BOOL HDA_StreamReset(struct HDAData *hc, struct HDAStream *hs)
{
    ULONG ctlreg = hs->hs_SDOffset + HD_SD_OFFSET_CONTROL;
    int i;

    hda_clearb(hc, ctlreg, HD_SD_CONTROL_STREAM_RUN);

    hda_setb(hc, ctlreg, HD_SD_STATUS_MASK);
    hda_setb(hc, hs->hs_SDOffset + HD_SD_OFFSET_STATUS, HD_SD_STATUS_MASK);
    hda_setb(hc, ctlreg, HD_SD_CONTROL_STREAM_RESET);

    for (i = 0; i < 1000; i++)
    {
        if (hda_readb(hc, ctlreg) & HD_SD_CONTROL_STREAM_RESET)
            break;
        hda_usleep(100);
    }

    if (i == 1000)
    {
        D(bug("[HDA] Stream %u would not enter reset\n", hs->hs_Index));
        return FALSE;
    }

    hda_clearb(hc, ctlreg, HD_SD_CONTROL_STREAM_RESET);
    hda_usleep(10);

    for (i = 0; i < 1000; i++)
    {
        if (!(hda_readb(hc, ctlreg) & HD_SD_CONTROL_STREAM_RESET))
            break;
        hda_usleep(100);
    }

    if (i == 1000)
    {
        D(bug("[HDA] Stream %u stuck in reset\n", hs->hs_Index));
        return FALSE;
    }

    hda_clearb(hc, ctlreg, HD_SD_CONTROL_STREAM_RUN);

    return !(hda_readb(hc, ctlreg) & HD_SD_CONTROL_STREAM_RESET);
}

static void HDA_FreeStreamBuffers(struct HDAStream *hs)
{
    ULONG i;

    for (i = 0; i < HDA_MAXSTREAMBUFS; i++)
    {
        if (hs->hs_BuffersUnaligned[i])
        {
            FreeVec(hs->hs_BuffersUnaligned[i]);
            hs->hs_BuffersUnaligned[i] = NULL;
        }
        hs->hs_Buffers[i] = NULL;
    }

    if (hs->hs_BDLUnaligned)
    {
        FreeVec(hs->hs_BDLUnaligned);
        hs->hs_BDLUnaligned = NULL;
        hs->hs_BDL = NULL;
    }

    hs->hs_BufferCount = 0;
    hs->hs_BufferSize = 0;
}

APTR HDA__Hidd_HDA__AllocStream(OOP_Class *cl, OOP_Object *o,
        struct pHidd_HDA_AllocStream *msg)
{
    struct HDAData *hc = OOP_INST_DATA(cl, o);
    int start, limit, i;

    if (!hc->hc_HWInit)
        return NULL;

    if (msg->direction == vHidd_HDA_StreamDir_In)
    {
        start = 0;
        limit = hc->hc_NumInStreams;
    }
    else
    {
        start = hc->hc_NumInStreams;
        limit = hc->hc_NumStreams;
    }

    ObtainSemaphore(&hc->hc_VerbLock);

    for (i = start; i < limit; i++)
    {
        struct HDAStream *hs = &hc->hc_Streams[i];

        if (!hs->hs_InUse)
        {
            hs->hs_InUse = TRUE;
            hs->hs_ClientInt = msg->streamInt;
            ReleaseSemaphore(&hc->hc_VerbLock);
            return hs;
        }
    }

    ReleaseSemaphore(&hc->hc_VerbLock);

    return NULL;
}

BOOL HDA__Hidd_HDA__SetupStream(OOP_Class *cl, OOP_Object *o,
        struct pHidd_HDA_SetupStream *msg)
{
    struct HDAData *hc = OOP_INST_DATA(cl, o);
    struct HDAStream *hs = (struct HDAStream *)msg->stream;
    struct HDA_StreamInfo *info = msg->info;
    APTR phys;
    ULONG i;

    if (!hs || !hs->hs_InUse || msg->bufferCount > HDA_MAXSTREAMBUFS)
        return FALSE;

    HDA_FreeStreamBuffers(hs);

    hs->hs_BDL = hda_alloc_aligned(
        sizeof(struct HDA_BDLE) * msg->bufferCount, &hs->hs_BDLUnaligned, 128);
    if (!hs->hs_BDL)
        return FALSE;

    for (i = 0; i < msg->bufferCount; i++)
    {
        APTR buffer = hda_alloc_aligned(msg->bufferSize,
                                        &hs->hs_BuffersUnaligned[i], 128);

        if (!buffer)
        {
            HDA_FreeStreamBuffers(hs);
            return FALSE;
        }

        hs->hs_Buffers[i] = buffer;

        if (hs->hs_Direction == vHidd_HDA_StreamDir_Out)
            phys = hda_dma_todevice(buffer, msg->bufferSize);
        else
            phys = hda_dma_fromdevice_prepare(buffer, msg->bufferSize);

        hs->hs_BDL[i].lower_address = (ULONG)((IPTR)phys & 0xFFFFFFFF);
#if (__WORDSIZE == 64)
        hs->hs_BDL[i].upper_address =
            (ULONG)(((IPTR)phys >> 32) & 0xFFFFFFFF);
#else
        hs->hs_BDL[i].upper_address = 0;
#endif
        hs->hs_BDL[i].length = msg->bufferSize;
        hs->hs_BDL[i].reserved_ioc = 1;
    }

    hs->hs_BufferCount = msg->bufferCount;
    hs->hs_BufferSize = msg->bufferSize;

    if (!HDA_StreamReset(hc, hs))
    {
        HDA_FreeStreamBuffers(hs);
        return FALSE;
    }

    /* 4.5.3 Starting Streams */
    hda_setb(hc, hs->hs_SDOffset + HD_SD_OFFSET_CONTROL + 2,
             hs->hs_Tag << 4);
    hda_writel(hc, hs->hs_SDOffset + HD_SD_OFFSET_CYCLIC_BUFFER_LEN,
               msg->bufferSize * msg->bufferCount);
    hda_writew(hc, hs->hs_SDOffset + HD_SD_OFFSET_LAST_VALID_INDEX,
               msg->bufferCount - 1);
    hda_writew(hc, hs->hs_SDOffset + HD_SD_OFFSET_FORMAT, msg->format);

    phys = hda_dma_todevice(hs->hs_BDL,
                            sizeof(struct HDA_BDLE) * msg->bufferCount);
    hda_writel(hc, hs->hs_SDOffset + HD_SD_OFFSET_BDL_ADDR_LOW,
               (ULONG)((IPTR)phys & 0xFFFFFFFF));
#if (__WORDSIZE == 64)
    hda_writel(hc, hs->hs_SDOffset + HD_SD_OFFSET_BDL_ADDR_HIGH,
               (ULONG)(((IPTR)phys >> 32) & 0xFFFFFFFF));
#else
    hda_writel(hc, hs->hs_SDOffset + HD_SD_OFFSET_BDL_ADDR_HIGH, 0);
#endif

    if (info)
    {
        info->si_Tag = hs->hs_Tag;
        info->si_HWIndex = hs->hs_Index;
        info->si_FIFOSize =
            hda_readw(hc, hs->hs_SDOffset + HD_SD_OFFSET_FIFO_SIZE);
        info->si_BufferCount = msg->bufferCount;
        for (i = 0; i < HDA_MAXSTREAMBUFS; i++)
            info->si_Buffers[i] =
                (i < msg->bufferCount) ? hs->hs_Buffers[i] : NULL;
    }

    return TRUE;
}

VOID HDA__Hidd_HDA__StartStream(OOP_Class *cl, OOP_Object *o,
        struct pHidd_HDA_StartStream *msg)
{
    struct HDAData *hc = OOP_INST_DATA(cl, o);
    struct HDAStream *hs = (struct HDAStream *)msg->stream;

    if (!hs || !hs->hs_InUse)
        return;

    hda_setl(hc, HD_INTCTL, 1UL << hs->hs_Index);
    hda_setb(hc, hs->hs_SDOffset + HD_SD_OFFSET_CONTROL,
             HD_SD_CONTROL_STREAM_RUN | HD_SD_STATUS_MASK);
    hs->hs_Running = TRUE;
}

VOID HDA__Hidd_HDA__StopStream(OOP_Class *cl, OOP_Object *o,
        struct pHidd_HDA_StopStream *msg)
{
    struct HDAData *hc = OOP_INST_DATA(cl, o);
    struct HDAStream *hs = (struct HDAStream *)msg->stream;

    if (!hs || !hs->hs_InUse)
        return;

    hda_clearb(hc, hs->hs_SDOffset + HD_SD_OFFSET_CONTROL,
               HD_SD_CONTROL_STREAM_RUN);
    hda_clearl(hc, HD_INTCTL, 1UL << hs->hs_Index);
    hs->hs_Running = FALSE;
}

VOID HDA__Hidd_HDA__FreeStream(OOP_Class *cl, OOP_Object *o,
        struct pHidd_HDA_FreeStream *msg)
{
    struct HDAData *hc = OOP_INST_DATA(cl, o);
    struct HDAStream *hs = (struct HDAStream *)msg->stream;

    if (!hs || !hs->hs_InUse)
        return;

    if (hs->hs_Running)
    {
        hda_clearb(hc, hs->hs_SDOffset + HD_SD_OFFSET_CONTROL,
                   HD_SD_CONTROL_STREAM_RUN);
        hda_clearl(hc, HD_INTCTL, 1UL << hs->hs_Index);
        hs->hs_Running = FALSE;
    }

    HDA_FreeStreamBuffers(hs);

    hs->hs_ClientInt = NULL;
    hs->hs_InUse = FALSE;
}

VOID HDA__Hidd_HDA__SyncStreamBuffer(OOP_Class *cl, OOP_Object *o,
        struct pHidd_HDA_SyncStreamBuffer *msg)
{
    struct HDAStream *hs = (struct HDAStream *)msg->stream;

    if (!hs || !hs->hs_InUse || msg->index >= hs->hs_BufferCount)
        return;

    if (hs->hs_Direction == vHidd_HDA_StreamDir_Out)
        hda_dma_todevice(hs->hs_Buffers[msg->index], hs->hs_BufferSize);
    else
        hda_dma_fromdevice_done(hs->hs_Buffers[msg->index],
                                hs->hs_BufferSize);
}

ULONG HDA__Hidd_HDA__GetStreamPosition(OOP_Class *cl, OOP_Object *o,
        struct pHidd_HDA_GetStreamPosition *msg)
{
    struct HDAData *hc = OOP_INST_DATA(cl, o);
    struct HDAStream *hs = (struct HDAStream *)msg->stream;

    if (!hs || !hs->hs_InUse)
        return 0;

    return hda_readl(hc, hs->hs_SDOffset + HD_SD_OFFSET_LINKPOS);
}
