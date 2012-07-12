/*
    Copyright © 2004-2011, The AROS Development Team. All rights reserved.
    $Id: lowlevel.c 34191 2010-08-17 16:19:51Z neil $

    Desc: PCI bus driver for ata.device
    Lang: English
*/

#define DEBUG 1

#include <aros/asmcall.h>
#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <asm/io.h>
#include <asm/amcc440.h>
#include <exec/lists.h>
#include <hidd/irq.h>
#include <oop/oop.h>
#include <resources/processor.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/processor.h>

#include <string.h>

#include "ata.h"

typedef struct 
{
    struct ataBase *ATABase;
    ULONG	    ata__buscount;
    OOP_AttrBase    HiddPCIDeviceAttrBase;
    OOP_MethodID    HiddPCIDriverMethodBase;
} EnumeratorArgs;

static VOID ata460_out(UBYTE val, UWORD offset, IPTR port, APTR data)
{
    offset <<= 2;
    outb(val, (uint8_t *)(port + offset + data));
}

static UBYTE ata460_in(UWORD offset, IPTR port, APTR data)
{
    offset <<= 2;
    return inb((uint8_t *)(port + offset + data));
}

static VOID ata460_outl(ULONG val, UWORD offset, IPTR port, APTR data)
{
    offset <<= 2;
    outl(val, (uint32_t *)(port + offset + data));
}

static VOID ata460_insw(APTR address, UWORD port, ULONG count, APTR data)
{
    UWORD *addr = address;
    UWORD *p = (UWORD*)(port + data);
    
    while(count)
    {
        *addr++ = inw(p);
        count -= 2;
    }
}

static VOID ata460_insl(APTR address, UWORD port, ULONG count, APTR data)
{
    ata460_insw(address, port, count, data);
}

static VOID ata460_outsw(APTR address, UWORD port, ULONG count, APTR data)
{
    UWORD *addr = address;
    UWORD *p = (UWORD*)(port + data);
    
    while(count)
    {
        outw(*addr++, p);
        count -= 2;
    }
}

static VOID ata460_outsl(APTR address, UWORD port, ULONG count, APTR data)
{
    ata460_outsw(address, port, count, data);
}

static void ata460_Interrupt(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw)
{
    /*
     * Our interrupt handler should call this function.
     * It's our problem how to store bus pointer. Here we use h_Data for it.
     */
    ata_HandleIRQ(irq->h_Data);
}

/* Actually a quick hack. Proper implementation really needs HIDDizing this code. */
static APTR ata460_CreateInterrupt(struct ata_Bus *bus)
{
    HIDDT_IRQ_Handler *IntHandler = AllocMem(sizeof(HIDDT_IRQ_Handler), MEMF_PUBLIC);

    if (IntHandler)
    {
        OOP_Object *o;

        /*
            Prepare nice interrupt for our bus. Even if interrupt sharing is enabled,
            it should work quite well
        */
        IntHandler->h_Node.ln_Pri = 10;
        IntHandler->h_Node.ln_Name = "<ppc460ex ATA>";
        IntHandler->h_Code = ata460_Interrupt;
        IntHandler->h_Data = bus;

        o = OOP_NewObject(NULL, CLID_Hidd_IRQ, NULL);
        if (o)
        {
            struct pHidd_IRQ_AddHandler msg =
            {
                mID:            OOP_GetMethodID(IID_Hidd_IRQ, moHidd_IRQ_AddHandler),
                handlerinfo:    IntHandler,
                id:             bus->ab_IRQ,
            };
            int retval = OOP_DoMethod(o, &msg.mID);

            OOP_DisposeObject(o);
            
            if (retval)
            	return IntHandler;
        }
    }

    FreeMem(IntHandler, sizeof(HIDDT_IRQ_Handler));
    return NULL;
}

static const struct ata_BusDriver ppc460_driver = 
{
    ata460_out,
    ata460_in,
    ata460_outl,
    ata460_insw,
    ata460_outsw,
    ata460_insl,
    ata460_outsl,
    ata460_CreateInterrupt
};

static inline ULONG GetPVR(void)
{
    struct Library *ProcessorBase = OpenResource(PROCESSORNAME);
    ULONG pvr = 0;

    if (ProcessorBase) {
        struct TagItem tags[] = {
            { GCIT_Model, (IPTR)&pvr },
            { TAG_END }
        };
        GetCPUInfo(tags);
    }

    return pvr;
}

#define SDR0_PE0_PHY_CTL_RST 0x30f


/* Requires supervisor */
ULONG getSDR0_PE0_PHY_CTL_RST(VOID)
{
    wrdcr(SDR0_CFGADDR, SDR0_PE0_PHY_CTL_RST);
    return rddcr(SDR0_CFGDATA);
}

static int ata460_Scan(struct ataBase *base)
{
    if (GetPVR() == PVR_PPC460EX_B) {
        /* ppc460 */
        ULONG pe0_mode = Supervisor(getSDR0_PE0_PHY_CTL_RST) & 0x3;

        D(bug("[ATA] Sam460EX PE0 mode = 0x%08x\n", pe0_mode));
        if (pe0_mode == 1 && (inl(SATA0_SCR0) & 0xf) == 3) {
            D(bug("[ATA] Enabling Sam460EX SATA in ATA PIO mode\n"));
            ata_RegisterBus(0, 0x18, INTR_UIC3_BASE + INTR_UIC3_SATA, 0,
                            ARBF_EarlyInterrupt | ARBF_80Wire, &ppc460_driver, 
                            (APTR)SATA0_CDR0, base);
        }
    }

    return TRUE;
}

/*
 * ata.device main code has two init routines with 0 and 127 priorities.
 * All bus scanners must run between them.
 */
ADD2INITLIB(ata460_Scan, 40)
ADD2LIBS("irq.hidd", 0, static struct Library *, __irqhidd)
