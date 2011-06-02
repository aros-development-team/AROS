
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
#include "timer.h"

#define GAYLE_BASE_4000 0xdd2022 /* 0xdd2020.W, 0xdd2026.B, 0xdd202a.B ... (argh!) */
#define GAYLE_IRQ_4000  0xdd3020

#define GAYLE_BASE_1200 0xda0000 /* 0xda0000.W, 0xda0004.B, 0xda0008.B ... */
#define GAYLE_IRQ_1200  0xda9000
#define GAYLE_INT_1200  0xdaa000

#define GAYLE_IRQ_IDE	0x80
#define GAYLE_INT_IDE	0x80

struct amiga_driverdata
{
    struct amiga_busdata *bus[2];
    struct Interrupt ideint;
    BOOL ideintdone;
    UBYTE *gaylebase;
    UBYTE *gayleirqbase;
    BOOL a4000;
    UBYTE doubler;
};

struct amiga_busdata
{
    struct amiga_driverdata *ddata;
    struct ata_Bus *bus;
    UBYTE *port;
    BOOL intena;
    BOOL reset;
};

static void ata_insw(APTR address, UWORD port, ULONG count, void *data)
{
    struct amiga_busdata *bdata = data;
    volatile UWORD *addr = (UWORD*)(bdata->port + (port & ~3));
    UWORD *dst = address;

    count /= 2;
    while (count-- != 0)
        *dst++ = *addr;
}

static void ata_outsw(APTR address, UWORD port, ULONG count, APTR data)
{
    struct amiga_busdata *bdata = data;
    volatile UWORD *addr = (UWORD*)(bdata->port + (port & ~3));
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
    struct amiga_busdata *bdata = data;
    volatile UBYTE *addr;

    /* IDE doubler hides Alternate Status/Device Control register */
    if (port == -1) {
	//bug("altout %x %x %d\n", bdata->port, port, offset);
    	bdata->intena = (val & 2) == 0;
	if (bdata->reset == 0 && (val & 4)) {
	    ata_out(0xa0, ata_DevHead, 0, bdata);
 	    ata_out(ATA_EXECUTE_DIAG, ata_Command, 0, bdata); 
 	    D(bug("[IDE] Emulating reset\n"));
	}
	bdata->reset = (val & 4) != 0;
    	//bug("intena=%d reset=%d\n", bdata->intena, bdata->reset);
	return;
    }
    //bug("out %x %x %x %d\n", bdata, bdata->port, port, offset);
    addr = (UBYTE*)(bdata->port + port);
    addr[offset * 4] = val;
}

static UBYTE ata_in(UWORD offset, IPTR port, APTR data)
{
    struct amiga_busdata *bdata = data;
    volatile UBYTE *addr;

    if (port == -1) {
    	//bug("altin %x %x %d\n", bdata->port, port, offset);
    	port = 0;
    	offset = ata_Status;
    }
    //bug("in %x %x %x %d\n", bdata, bdata->port, port, offset);
    addr = (UBYTE*)(bdata->port + port);
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
    volatile UBYTE *port, *altport;
    struct GfxBase *gfx;

    port = NULL;
    id = ReadGayle();
    if (id) {
        port = (UBYTE*)GAYLE_BASE_1200;
	ddata->gayleirqbase = (UBYTE*)GAYLE_IRQ_1200;
    } else {
    	gfx = (struct GfxBase*)TaggedOpenLibrary(TAGGEDOPEN_GRAPHICS);
    	// in AGA this area is never custom mirror but lets make sure..
    	if (!custom_check((APTR)0xdd2000) && (gfx->ChipRevBits0 & GFXF_AA_ALICE)) {
            port = (UBYTE*)GAYLE_BASE_4000;
	    ddata->a4000 = TRUE;
	    ddata->gayleirqbase = (UBYTE*)GAYLE_IRQ_4000;
        }
        CloseLibrary((struct Library*)gfx);
    }
    D(bug("[ATA] Gayle ID=%02x. Possible IDE port=%08x.\n", id, (ULONG)port & ~3));
    if (port == NULL)
    	return NULL;

    altport = port + 0x1010;
    status = port[ata_Status * 4];
    D(bug("[ATA] Status=%02x\n", status));
    // BUSY and DRDY both active or ERROR/DATAREQ = no drive(s) = do not install driver
    if (((status & (ATAF_BUSY | ATAF_DRDY)) == (ATAF_BUSY | ATAF_DRDY))
    	|| (status & (ATAF_ERROR | ATAF_DATAREQ))) {
    	D(bug("[ATA] Drives not detected\n"));
    	return NULL;
    }
    if (ddata->doubler) {
    	UBYTE v1, v2, v3;
    	/* check if AltControl is both readable and writable
    	 * It is either floating or DevHead if IDE doubler is connected.
    	 */
	v3 = altport[ata_AltControl * 4];
	altport[ata_AltControl * 4] = 0;
	v1 = altport[ata_AltControl * 4];
	altport[ata_AltControl * 4] = 2;
	v2 = altport[ata_AltControl * 4];
	altport[ata_AltControl * 4] = v3;
	if ((v1 == 0 && v2 == 2) || (v1 == 0xff && v2 == 0xff) || (v1 == 0 && v2 == 0)) {
    	    ddata->doubler = 2;
	    D(bug("[ATA] IDE doubler detected\n"));
	} else {
    	    ddata->doubler = 0;
	}
    }
    /* we may have connected drives */
    return (UBYTE*)port;
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
	if (ddata->bus[0] && ddata->bus[0]->intena)
	    ata_HandleIRQ(ddata->bus[0]->bus);
	if (ddata->bus[1] && ddata->bus[1]->intena)
	    ata_HandleIRQ(ddata->bus[1]->bus);
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
    if (irqmask & (GAYLE_IRQ_IDE << 8)) {
	if (ddata->bus[0] && ddata->bus[0]->intena)
	    ata_HandleIRQ(ddata->bus[0]->bus);
	if (ddata->bus[1] && ddata->bus[1]->intena)
	    ata_HandleIRQ(ddata->bus[1]->bus);
    }
    return 0;

    AROS_USERFUNC_EXIT
}

static APTR ata_CreateInterrupt(struct ata_Bus *bus, UBYTE num)
{
    struct amiga_busdata *bdata = bus->ab_DriverData;
    struct amiga_driverdata *ddata = bdata->ddata;
    struct Interrupt *irq = &ddata->ideint;
    volatile UBYTE *gayleintbase = NULL;

    bdata->bus = bus;
    ddata->bus[num] = bdata;

    if (ddata->ideintdone)
    	return bdata;
    ddata->ideintdone = TRUE;

    if (ddata->a4000) {
	irq->is_Code = (APTR)IDE_Handler_A4000;
    } else {
        gayleintbase = (UBYTE*)GAYLE_INT_1200;
	irq->is_Code = (APTR)IDE_Handler_A1200;
    }

    irq->is_Node.ln_Pri = 20;
    irq->is_Node.ln_Type = NT_INTERRUPT;
    irq->is_Node.ln_Name = "AT-IDE";
    irq->is_Data = ddata;
    AddIntServer(INTB_PORTS, irq);
    
    if (gayleintbase)
        *gayleintbase |= GAYLE_INT_IDE;

    return bdata;
}
static APTR ata_CreateInterrupt0(struct ata_Bus *bus)
{
    return ata_CreateInterrupt(bus, 0);
}
static APTR ata_CreateInterrupt1(struct ata_Bus *bus)
{
    return ata_CreateInterrupt(bus, 1);
}

static const struct ata_BusDriver amiga_driver0 = 
{
    ata_out,
    ata_in,
    ata_outl,
    ata_insw,
    ata_outsw,
    ata_insw,	/* These are intentionally the same as 16-bit routines */
    ata_outsw,
    ata_CreateInterrupt0
};
static const struct ata_BusDriver amiga_driver1 = 
{
    ata_out,
    ata_in,
    ata_outl,
    ata_insw,
    ata_outsw,
    ata_insw,	/* These are intentionally the same as 16-bit routines */
    ata_outsw,
    ata_CreateInterrupt1
};

static int ata_amiga_init(struct ataBase *LIBBASE)
{
    struct amiga_driverdata *ddata;
    struct amiga_busdata *bdata;

    ddata = AllocVec(sizeof(struct amiga_driverdata), MEMF_CLEAR);
    if (!ddata)
    	return FALSE;
    ddata->doubler = 1;

    ddata->gaylebase = getport(ddata);
    bdata = AllocVec(sizeof(struct amiga_busdata) * (ddata->doubler == 2 ? 2 : 1), MEMF_CLEAR);
    if (bdata && ddata->gaylebase) {
	LIBBASE->ata_NoDMA = TRUE;
	bdata->ddata = ddata;
	bdata->intena = 1;
	bdata->port = ddata->gaylebase;
	ata_RegisterBus(0, ddata->doubler ? -1 : 0x1010, 2, 0, 0, &amiga_driver0, bdata, LIBBASE);
	if (ddata->doubler == 2) {
	    bdata++;
	    bdata->ddata = ddata;
	    bdata->intena = 1;
	    bdata->port = ddata->gaylebase + 0x1000;
	    ata_RegisterBus(0, -1, 2, 0, 0, &amiga_driver1, bdata, LIBBASE);
	}
	return TRUE;
    }
    FreeVec(bdata);
    FreeVec(ddata);
    return FALSE;
}

ADD2INITLIB(ata_amiga_init, 20)
