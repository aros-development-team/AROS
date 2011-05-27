
#define DEBUG 1

#include <aros/debug.h>
#include <exec/types.h>
#include <exec/exec.h>
#include <proto/exec.h>
#include <graphics/gfxbase.h>
#include <hardware/custom.h>
#include <hardware/intbits.h>
#include <aros/symbolsets.h>

#include "ata.h"

#define GAYLE_BASE_4000 0xdd2022 /* 0xdd2020.W, 0xdd2026.B, 0xdd202a.B ... (argh!) */
#define GAYLE_IRQ_4000  0xdd3020

#define GAYLE_BASE_1200 0xda0000 /* 0xda0000.W, 0xda0004.B, 0xda0008.B ... */
#define GAYLE_IRQ_1200  0xda9000
#define GAYLE_INT_1200  0xdaa000

#define GAYLE_IRQ_IDE	0x80
#define GAYLE_INT_IDE	0x80

struct amiga_driverdata
{
    struct ata_Bus *bus;
    struct Interrupt ideint;
    UBYTE *gaylebase;
    UBYTE *gayleirqbase;
    BOOL a4000;
};

static void ata_insw(APTR address, UWORD port, ULONG count, void *data)
{
    struct amiga_driverdata *ddata = data;
    volatile UWORD *addr = (UWORD*)(ddata->gaylebase + (port & ~3));
    UWORD *dst = address;

    count /= 2;
    while (count-- != 0)
        *dst++ = *addr;
}

static void ata_outsw(APTR address, UWORD port, ULONG count, APTR data)
{
    struct amiga_driverdata *ddata = data;
    volatile UWORD *addr = (UWORD*)(ddata->gaylebase + (port & ~3));
    UWORD *dst = address;

    count /= 2;
    while (count-- != 0)
        *addr = *dst++;
}

static void ata_outl(ULONG val, UWORD offset, IPTR port, APTR data)
{
}

static void ata_out(UBYTE val, UWORD offset, IPTR port, APTR data)
{
    struct amiga_driverdata *ddata = data;
    volatile UBYTE *addr = (UBYTE*)(ddata->gaylebase + port);

    addr[offset * 4] = val;
}

static UBYTE ata_in(UWORD offset, IPTR port, APTR data)
{
    struct amiga_driverdata *ddata = data;
    volatile UBYTE *addr = (UBYTE*)(ddata->gaylebase + port);

    return addr[offset * 4];
}

static BOOL custom_check(APTR addr)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    volatile struct Custom *maybe_custom = (struct Custom*)addr;
    UWORD intena;
    BOOL iscustom = TRUE;
    
    intena = custom->intenar;
    custom->intena = 0x7fff;
    custom->intena = 0xc000;
    maybe_custom->intena = 0x7fff;
    if (custom->intenar == 0x4000) {
    	maybe_custom->intena = 0x7fff;
    	if (custom->intenar == 0x4000)
    	    iscustom = FALSE;
    }
    custom->intena = 0x7fff;
    custom->intena = intena | 0x8000;
    return iscustom;
}  	

static UBYTE *getport(struct amiga_driverdata *ddata)
{
    UBYTE id, status;
    UBYTE *port = NULL;
    struct GfxBase *gfx;

    id = ReadGayle();
    if (id) {
        port = (UBYTE*)GAYLE_BASE_1200;
    } else {
    	gfx = (struct GfxBase*)TaggedOpenLibrary(TAGGEDOPEN_GRAPHICS);
    	// in AGA this area is never custom mirror but lets make sure..
    	if (!custom_check((APTR)0xdd2000) && (gfx->ChipRevBits0 & GFXF_AA_ALICE)) {
            port = (UBYTE*)GAYLE_BASE_4000;
        }
        CloseLibrary((struct Library*)gfx);
    }
    D(bug("[ATA] Gayle ID=%02x. Possible IDE port=%08x\n", id, (ULONG)port & ~3));
    if (port == NULL)
    	return NULL;

    status = ata_in(ata_Status, (IPTR)port, ddata);
    D(bug("[ATA] Status=%02x\n", status));
    // BUSY and DRDY both active or ERROR/DATAREQ = no drive(s) = do not install driver
    if (((status & (ATAF_BUSY | ATAF_DRDY)) == (ATAF_BUSY | ATAF_DRDY))
    	|| (status & (ATAF_ERROR | ATAF_DATAREQ))) {
    	D(bug("[ATA] Drives not detected\n"));
    	return NULL;
    }
    /* we may have connected drives */
    return port;
}

AROS_UFH4(APTR, IDE_Handler_A1200,
    AROS_UFHA(ULONG, dummy, A0),
    AROS_UFHA(void *, data, A1),
    AROS_UFHA(ULONG, dummy2, A5),
    AROS_UFHA(struct ExecBase *, mySysBase, A6))
{
    AROS_USERFUNC_INIT

    struct amiga_driverdata *ddata = data;
    UBYTE irqmask = *ddata->gayleirqbase;
    if (irqmask & GAYLE_IRQ_IDE) {
	ata_HandleIRQ(ddata->bus);
	/* Clear interrupt */
	*ddata->gayleirqbase = irqmask & ~GAYLE_IRQ_IDE;
    }
    return 0;

    AROS_USERFUNC_EXIT
}

AROS_UFH4(APTR, IDE_Handler_A4000,
    AROS_UFHA(ULONG, dummy, A0),
    AROS_UFHA(void *, data, A1),
    AROS_UFHA(ULONG, dummy2, A5),
    AROS_UFHA(struct ExecBase *, mySysBase, A6))
{
    AROS_USERFUNC_INIT

    struct amiga_driverdata *ddata = data;
    /* A4000 interrupt clears when register is read */
    UWORD irqmask = *((UWORD*)ddata->gayleirqbase);
    if (irqmask & (GAYLE_IRQ_IDE << 8))
	ata_HandleIRQ(ddata->bus);
    return 0;

    AROS_USERFUNC_EXIT
}

static APTR ata_CreateInterrupt(struct ata_Bus *bus)
{
    struct amiga_driverdata *ddata = bus->ab_DriverData;
    struct Interrupt *irq = &ddata->ideint;
    volatile UBYTE *gayleintbase = NULL;

    if (ddata->a4000) {
	irq->is_Code = (APTR)IDE_Handler_A4000;
    } else {
        gayleintbase = (UBYTE*)GAYLE_INT_1200;
	irq->is_Code = (APTR)IDE_Handler_A1200;
    }
    ddata->bus = bus;

    irq->is_Node.ln_Pri = 20;
    irq->is_Node.ln_Type = NT_INTERRUPT;
    irq->is_Node.ln_Name = "AT-IDE";
    irq->is_Data = ddata;
    AddIntServer(INTB_PORTS, irq);
    
    if (gayleintbase)
        *gayleintbase |= GAYLE_INT_IDE;

    return irq;
}

static const struct ata_BusDriver amiga_driver = 
{
    ata_out,
    ata_in,
    ata_outl,
    ata_insw,
    ata_outsw,
    ata_insw,	/* These are intentionally the same as 16-bit routines */
    ata_outsw,
    ata_CreateInterrupt
};

static int ata_amiga_init(struct ataBase *LIBBASE)
{
    struct amiga_driverdata *ddata;

    ddata = AllocMem(sizeof(struct amiga_driverdata), MEMF_CLEAR);
    if (!ddata)
    	return FALSE;

    ddata->gaylebase = getport(ddata);
    if (ddata->gaylebase) {
	if (ddata->gaylebase == (UBYTE*)GAYLE_BASE_4000) {
	    ddata->a4000 = TRUE;
	    ddata->gayleirqbase = (UBYTE*)GAYLE_IRQ_4000;
	} else {
	    ddata->gayleirqbase = (UBYTE*)GAYLE_IRQ_1200;
	}
	LIBBASE->ata_NoDMA = TRUE;
	ata_RegisterBus(0, 0x1010, 2, 0, 0, &amiga_driver, ddata, LIBBASE);
	return TRUE;
    }
    FreeMem(ddata, sizeof(struct amiga_driverdata));
    return FALSE;
}

ADD2INITLIB(ata_amiga_init, 20)
