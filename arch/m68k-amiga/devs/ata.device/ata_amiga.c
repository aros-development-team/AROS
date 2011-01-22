
#define DEBUG 1

#include <aros/debug.h>
#include <exec/types.h>
#include <exec/exec.h>
#include <proto/exec.h>
#include <graphics/gfxbase.h>

#include "ata.h"

#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT) && defined(__mc68000)

#include <hardware/intbits.h>

void ata_insw(APTR address, IPTR port, ULONG count)
{
    volatile UWORD *addr = (UWORD*)port;
    UWORD *dst = address;
    count /= 2;
    while (count-- != 0)
        *dst++ = *addr;
}

void ata_insl(APTR address, IPTR port, ULONG count)
{
}

void ata_outsw(APTR address, IPTR port, ULONG count)
{
    volatile UWORD *addr = (UWORD*)port;
    UWORD *dst = address;
    count /= 2;
    while (count-- != 0)
        *addr = *dst++;
}

void ata_outsl(APTR address, IPTR port, ULONG count)
{
}

void ata_out(UBYTE val, UWORD offset, IPTR port)
{
    volatile UBYTE *addr = (UBYTE*)port;
    addr[offset * 4] = val;
}

UBYTE ata_in(UWORD offset, IPTR port)
{
    volatile UBYTE *addr = (UBYTE*)port;
    return addr[offset * 4];
}

void ata_outl(ULONG val, UWORD offset, IPTR port)
{
}

static UBYTE *getport(void)
{
    UBYTE id, status;
    UBYTE *port = NULL;
    struct GfxBase *gfx;

    id = ReadGayle();
    if (id) {
        port = (UBYTE*)GAYLE_BASE_1200;
    } else {
    	gfx = (struct GfxBase*)OpenLibrary("graphics.library", 0);
    	// in AGA this area is never custom mirror
    	if (gfx->ChipRevBits0 & GFXF_AA_ALICE) {
            port = (UBYTE*)GAYLE_BASE_4000;
        }
        CloseLibrary(gfx);
    }
    D(bug("[ATA--] Gayle ID=%02x. Possible IDE port=%08x\n", id, port));
    if (port == NULL)
    	return NULL;

    status = ata_in(ata_Status, (IPTR)port);
    D(bug("[ATA--] Status=%02x\n", status));
    // BUSY and DRDY both active or ERROR/DATAREQ = no drive(s) = do not install driver
    if (((status & (ATAF_BUSY | ATAF_DRDY)) == (ATAF_BUSY | ATAF_DRDY))
    	|| (status & (ATAF_ERROR | ATAF_DATAREQ))) {
    	D(bug("[ATA--] Drives not detected\n"));
    	return NULL;
    }
    /* we may have drives */
    return port;
}

BOOL ata_ishardware(void)
{
    return getport() != NULL;
}

void ata_Scan(struct ataBase *base)
{
    UBYTE *port;
    EnumeratorArgs Args=
    {
        base,
        0
    };

    port = getport();
    if (!port)
    	return;

    if (port == (UBYTE*)GAYLE_BASE_4000)
    	base->a4000 = TRUE;
    base->gaylebase = port;

    ata_RegisterBus((IPTR)base->gaylebase, (IPTR)(base->gaylebase + 0x1010), 2, 0, 0, &Args);
    ata_scanstart(base);
}

void ata_configure(struct ataBase *base)
{
    base->ata_32bit = FALSE;
    base->ata_NoMulti = FALSE;
    base->ata_NoDMA = TRUE;
    base->ata_Poll = FALSE;
}

AROS_UFH4(APTR, IDE_Handler,
    AROS_UFHA(ULONG, dummy, A0),
    AROS_UFHA(void *, data, A1),
    AROS_UFHA(ULONG, dummy2, A5),
    AROS_UFHA(struct ExecBase *, mySysBase, A6))
{
    AROS_USERFUNC_INIT

    struct ataBase *base = data;
    UBYTE irqmask = *base->gayleirqbase;
    if (irqmask & GAYLE_IRQ_IDE) {
     	struct ata_Bus *b = (struct ata_Bus*)base->ata_Buses.mlh_Head;
    	while (b->ab_Node.mln_Succ) {
    	    ata_HandleIRQ(b);
    	    b = (struct ata_Bus*)b->ab_Node.mln_Succ;
    	}
    	if (base->a4000)
    	    irqmask = *base->gayleirqbase;
    	else
    	    *base->gayleirqbase = irqmask & ~GAYLE_IRQ_IDE;
    }
    return 0;

    AROS_USERFUNC_EXIT
}


int ata_CreateInterrupt(struct ata_Bus *bus)
{
    struct ataBase *base = bus->ab_Base;
    struct Interrupt *irq = &base->ideint;
    volatile UBYTE *gayleintbase = NULL;

    if (base->a4000) {
        base->gaylebase = (UBYTE*)GAYLE_BASE_4000;
        base->gayleirqbase = (UBYTE*)GAYLE_BASE_4000 + GAYLE_IRQ_4000;
    } else {
        gayleintbase = (UBYTE*)GAYLE_BASE_1200 + GAYLE_INT_1200;
        base->gaylebase = (UBYTE*)GAYLE_BASE_1200;
        base->gayleirqbase = (UBYTE*)GAYLE_BASE_1200 + GAYLE_IRQ_1200;
    }
    	
    irq->is_Node.ln_Pri = 20;
    irq->is_Node.ln_Type = NT_INTERRUPT;
    irq->is_Node.ln_Name = "AT-IDE";
    irq->is_Code = (APTR)IDE_Handler;
    irq->is_Data = base;
    AddIntServer(INTB_PORTS, irq);
    
    if (gayleintbase)
        *gayleintbase |= GAYLE_INT_IDE;

    return 1;
}

#endif
