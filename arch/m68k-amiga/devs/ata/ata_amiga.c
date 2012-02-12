
#define DEBUG 1
#define DEBUG2 0

#include <aros/debug.h>
#include <exec/types.h>
#include <exec/exec.h>
#include <proto/exec.h>
#include <proto/cardres.h>
#include <graphics/gfxbase.h>
#include <hardware/custom.h>
#include <hardware/intbits.h>
#include <resources/card.h>
#include <libraries/pccard.h>
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

struct amiga_pcmcia_driverdata
{
    struct amiga_busdata *bus[1];
    struct CardHandle cardhandle;
    struct Interrupt statusint;
    struct Interrupt insertint;
    struct Interrupt removalint;
    struct CardResource *CardResource;
    BOOL intena;
    BOOL poststatus;
    ULONG configbase, configmask;
    struct DeviceTData dtd;
    struct CardMemoryMap *cmm;
};


struct amiga_busdata
{
    void *ddata;
    struct ata_Bus *bus;
    UBYTE *port;
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

static void ata_pcmcia_insw(APTR address, UWORD port, ULONG count, void *data)
{
    struct amiga_busdata *bdata = data;
    volatile UWORD *addr = (UWORD*)(bdata->port + 8);
    UWORD *dst = address;

    count /= 2;
    while (count-- != 0)
        *dst++ = *addr;
}

static void ata_pcmcia_outsw(APTR address, UWORD port, ULONG count, APTR data)
{
    struct amiga_busdata *bdata = data;
    volatile UWORD *addr = (UWORD*)(bdata->port + 8);
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

#if DEBUG2
    bug("ata_out(%x,%x)=%x:%x\n", offset, port, (UBYTE*)(bdata->port + port) + offset * 4, val);
#endif
    /* IDE doubler hides Alternate Status/Device Control register */
    if (port == -1) {
	if (bdata->reset == 0 && (val & 4)) {
	    ata_out(0x40, ata_DevHead, 0, bdata);
 	    D(bug("[ATA] Emulating reset\n"));
 	    ata_out(ATA_EXECUTE_DIAG, ata_Command, 0, bdata); 
	}
	bdata->reset = (val & 4) != 0;
	return;
    }
    addr = (UBYTE*)(bdata->port + port);
    addr[offset * 4] = val;
}

static UBYTE ata_in(UWORD offset, IPTR port, APTR data)
{
    struct amiga_busdata *bdata = data;
    volatile UBYTE *addr;
    UBYTE v;

#if DEBUG2
    bug("ata_in(%x,%x)=%x\n", offset, port, (UBYTE*)(bdata->port + port) + offset * 4);
#endif
    if (port == -1) {
    	port = 0;
    	offset = ata_Status;
    }
    addr = (UBYTE*)(bdata->port + port);
    v = addr[offset * 4];
#if DEBUG2
    bug("=%x\n", v);
#endif
    return v;
}

static void ata_pcmcia_out(UBYTE val, UWORD offset, IPTR port, APTR data)
{
    volatile UBYTE *addr;

    if (offset == ata_Feature)
        offset = 13;

    addr = (UBYTE*)port;
    addr[offset] = val;
}

static UBYTE ata_pcmcia_in(UWORD offset, IPTR port, APTR data)
{
    volatile UBYTE *addr;

    if (offset == ata_Feature)
        offset = 13;

    addr = (UBYTE*)port;
    return addr[offset];
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
    UBYTE id, status1, status2;
    volatile UBYTE *port, *altport;
    struct GfxBase *gfx;

    port = NULL;
    gfx = (struct GfxBase*)TaggedOpenLibrary(TAGGEDOPEN_GRAPHICS);
    Disable();
    id = ReadGayle();
    if (id) {
        port = (UBYTE*)GAYLE_BASE_1200;
	ddata->gayleirqbase = (UBYTE*)GAYLE_IRQ_1200;
    } else {
    	// in AGA this area is never custom mirror but lets make sure..
    	if (!custom_check((APTR)0xdd4000) && (gfx->ChipRevBits0 & GFXF_AA_ALICE)) {
            port = (UBYTE*)GAYLE_BASE_4000;
	    ddata->a4000 = TRUE;
	    ddata->gayleirqbase = (UBYTE*)GAYLE_IRQ_4000;
        }
    }
    Enable();
    CloseLibrary((struct Library*)gfx);

    D(bug("[ATA] Gayle ID=%02x. Possible IDE port=%08x.\n", id, (ULONG)port & ~3));
    if (port == NULL)
    	return NULL;

    altport = port + 0x1010;
    Disable();
    port[atapi_DevSel * 4] = ATAF_ERROR;
    /* If nothing connected, we get back what we wrote, ATAF_ERROR set */
    status1 = port[ata_Status * 4];
    port[atapi_DevSel * 4] = ATAF_DATAREQ;
    status2 = port[ata_Status * 4];
    port[atapi_DevSel * 4] = 0;
    Enable();
    D(bug("[ATA] Status=%02x,%02x\n", status1, status2));
    // BUSY and DRDY both active or ERROR/DATAREQ = no drive(s) = do not install driver
    if (   (((status1 | status2) & (ATAF_BUSY | ATAF_DRDY)) == (ATAF_BUSY | ATAF_DRDY))
    	|| ((status1 | status2) & (ATAF_ERROR | ATAF_DATAREQ)))
    {
    	D(bug("[ATA] Drives not detected\n"));
    	return NULL;
    }
    if (ddata->doubler) {
    	UBYTE v1, v2;
    	/* check if AltControl is both readable and writable
    	 * It is either floating or DevHead if IDE doubler is connected.
    	 * AltControl = DevHead (R)
    	 * Device Control = DevHead (W)
    	 */
    	Disable();
	altport[ata_AltControl * 4] = 0;
    	port[atapi_DevSel * 4] = 1;
	v1 = altport[ata_AltControl * 4];
	altport[ata_AltControl * 4] = 2;
    	port[atapi_DevSel * 4] = 4;
	v2 = altport[ata_AltControl * 4];
	altport[ata_AltControl * 4] = 0;
    	port[atapi_DevSel * 4] = 0;
	Enable();
	if ((v1 == 0 && v2 == 2) || (v1 == 1 && v2 == 4) || (v1 == 0xff && v2 == 0xff)) {
    	    ddata->doubler = 2;
	} else {
    	    ddata->doubler = 0;
	}
	D(bug("[ATA] IDE doubler check (%02X, %02X) = %d\n", v1, v2, ddata->doubler));
    }
    /* we may have connected drives */
    return (UBYTE*)port;
}

static void callbusirq(struct amiga_driverdata *ddata)
{
    volatile UBYTE *port;
    UBYTE status1, status2;
    BOOL handled = FALSE;

    if (ddata->bus[0])
        handled |= ata_HandleIRQ(ddata->bus[0]->bus);
    if (ddata->bus[1])
        handled |= ata_HandleIRQ(ddata->bus[1]->bus);
    if (handled)
        return;

    /* Handle spurious interrupt */
    port = ddata->gaylebase;
    status1 = port[ata_Status * 4];
    status2 = 0;
    if (ddata->doubler == 2)
        status2 = port[0x1000 + ata_Status * 4];
    bug("[ATA] Spurious interrupt: %02X %02X\n", status1, status2);
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
	/* Clear interrupt */
	*ddata->gayleirqbase = 0x7c | (*ddata->gayleirqbase & 3);
	callbusirq(ddata);
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
	callbusirq(ddata);
    }
    return 0;

    AROS_USERFUNC_EXIT
}

AROS_UFH4(UBYTE, IDE_PCMCIA_Handler,
    AROS_UFHA(UBYTE, status, D0),
    AROS_UFHA(void *, data, A1),
    AROS_UFHA(ULONG, dummy2, A5),
    AROS_UFHA(struct ExecBase *, mySysBase, A6))
{ 
    AROS_USERFUNC_INIT

    struct amiga_pcmcia_driverdata *ddata = data;
    if (ddata->poststatus) {
        if (ddata->intena)
            ata_HandleIRQ(ddata->bus[0]->bus);
    } else if (status & CARD_INTF_IRQ) {
        ddata->poststatus = TRUE;
    }

    return status;

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
static APTR ata_CreateInterrupt_pcmcia(struct ata_Bus *bus)
{
    struct amiga_busdata *bdata = bus->ab_DriverData;
    struct amiga_pcmcia_driverdata *ddata = bdata->ddata;

    bdata->bus = bus;
    ddata->bus[0] = bdata;

    ddata->intena = 1;
    return bdata;
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
static const struct ata_BusDriver amiga_driver_pcmcia = 
{
    ata_pcmcia_out,
    ata_pcmcia_in,
    ata_outl,
    ata_pcmcia_insw,
    ata_pcmcia_outsw,
    ata_pcmcia_insw,
    ata_pcmcia_outsw,
    ata_CreateInterrupt_pcmcia
};

static BOOL ata_amiga_ide_init(struct ataBase *LIBBASE)
{
    struct amiga_driverdata *ddata;
    struct amiga_busdata *bdata;

    ddata = AllocVec(sizeof(struct amiga_driverdata), MEMF_CLEAR | MEMF_PUBLIC);
    if (!ddata)
    	return FALSE;
    ddata->doubler = 1;

    ddata->gaylebase = getport(ddata);
    bdata = AllocVec(sizeof(struct amiga_busdata) * (ddata->doubler == 2 ? 2 : 1), MEMF_CLEAR | MEMF_PUBLIC);
    if (bdata && ddata->gaylebase) {
	LIBBASE->ata_NoDMA = TRUE;
	bdata->ddata = ddata;
	bdata->port = ddata->gaylebase;
	ata_RegisterBus(0, ddata->doubler ? -1 : 0x1010, 2, 0, ARBF_EarlyInterrupt, &amiga_driver0, bdata, LIBBASE);
	if (ddata->doubler == 2) {
	    D(bug("[ATA] Adding secondary bus\n"));
	    bdata++;
	    bdata->ddata = ddata;
	    bdata->port = ddata->gaylebase + 0x1000;
	    ata_RegisterBus(0, -1, 2, 0, ARBF_EarlyInterrupt, &amiga_driver1, bdata, LIBBASE);
	}
	return TRUE;
    }
    FreeVec(bdata);
    FreeVec(ddata);
    return FALSE;
}

static BOOL detectcard(struct amiga_pcmcia_driverdata *ddata)
{
    APTR CardResource;
    struct CardHandle *ch;
    UBYTE tuple[256 + 2];
    WORD cnt1, cnt2;
    UBYTE *tp;
    BOOL got;

    ch = &ddata->cardhandle;
    CardResource = ddata->CardResource;

    ddata->configmask = 1;
    ddata->configbase = 0x0200;

    CardResetCard(ch);
    CardMiscControl(ch, CARD_ENABLEF_DIGAUDIO | CARD_DISABLEF_WP);

    got = FALSE;
    for (;;) {
        if (!CopyTuple(ch, tuple, PCCARD_TPL_DEVICE, sizeof(tuple) - 2))
            break;
        if (!DeviceTuple(tuple, &ddata->dtd))
            break;
        if (ddata->dtd.dtd_DTtype != PCCARD_DTYPE_FUNCSPEC)
            break;
        tuple[2] = 0;
        if (!CopyTuple(ch, tuple, PCCARD_TPL_FUNCID, sizeof(tuple) - 2))
            break;
        if (tuple[2] != PCCARD_FUNC_FIXED)
            break;
        got = FALSE;
        for (cnt1 = 0; TRUE; cnt1++) {
            if (!CopyTuple(ch, tuple, PCCARD_TPL_FUNCE | (cnt1 << 16), sizeof(tuple) - 2))
                break;
            if (tuple[2] != 1 || tuple[3] != 1)
                break;
            got = TRUE;
            break;
        }
        if (!got)
            break;
        got = FALSE;
        if (!CopyTuple(ch, tuple, PCCARD_TPL_CONFIG, sizeof(tuple) - 2))
            break;
        if (tuple[1] < 5)
            break;
        //lastindex = tuple[3] & 0x3f;
        tp = &tuple[4];
        cnt2 = (tuple[2] & 3) + 1;
        for (cnt1 = 0; cnt1 < cnt2; cnt1++) {
            ddata->configbase |= (*tp) << (cnt1 * 8);
            tp++;
        }
        cnt2 = ((tuple[2] >> 3) & 15) + 1;
        for (cnt1 = 0; cnt1 < cnt2; cnt1++) {
            ddata->configmask |= (*tp) << (cnt1 * 8);
            tp++;
        }
        return TRUE;
    }
    return FALSE;
}

static void initializecard(struct amiga_pcmcia_driverdata *ddata)
{
    struct CardHandle *ch;
    UBYTE tuple[256 + 2];
    UBYTE *tp;
    APTR CardResource;
    volatile UBYTE *attrbase;

    ch = &ddata->cardhandle;
    CardResource = ddata->CardResource;

    D(bug("Detected PCMCIA IDE. ConfigBase=%08x RMask=%08x\n", ddata->configbase, ddata->configmask);
    memset(tuple, 0, sizeof tuple);
    if (CopyTuple(ch, tuple, PCCARD_TPL_VERS1, sizeof(tuple) - 2)) {
        if (tuple[2] == 4) {
            tp = &tuple[4];
            while (*tp != 0xff) {
                bug("%s ", tp);
                tp += strlen(tp) + 1;
            }
            D(bug("\n"));
        }
    });
    CardAccessSpeed(ch, ddata->dtd.dtd_DTspeed);
    attrbase = ddata->cmm->cmm_AttributeMemory;
    attrbase[ddata->configbase + 2 * 3] = 0; /* Socket and copy. Must be written first. */
    attrbase[ddata->configbase + 2 * 2] = 0x0f; /* Pin replacement. */
    attrbase[ddata->configbase + 2 * 1] = 0; /* Configuration and Status. */
    attrbase[ddata->configbase + 2 * 0] = 0x41; /* Configure option. Configure as IO linear mode. */
    /* Now we have IDE registers at iobase */
}

static BOOL ata_amiga_pcmcia_init(struct ataBase *LIBBASE)
{
    struct CardResource *CardResource;
    struct amiga_pcmcia_driverdata *ddata;
    struct amiga_busdata *bdata;
    struct CardHandle *ch;
    
    CardResource = OpenResource("card.resource");
    if (!CardResource)
        return FALSE;
    if (CardInterface() != CARD_INTERFACE_AMIGA_0)
        return FALSE;

    ddata = AllocVec(sizeof(struct amiga_pcmcia_driverdata) + sizeof(struct amiga_busdata), MEMF_CLEAR | MEMF_PUBLIC);
    if (!ddata)
        return FALSE;
    bdata = (struct amiga_busdata*)(ddata + 1);

    ch = &ddata->cardhandle;
    ddata->CardResource = CardResource;
    ddata->cmm = GetCardMap();

    ch->cah_CardFlags = CARDF_IFAVAILABLE | CARDF_POSTSTATUS;
    ch->cah_CardNode.ln_Name = LIBBASE->ata_Device.dd_Library.lib_Node.ln_Name;
    ch->cah_CardStatus = &ddata->statusint;
    ch->cah_CardRemoved = &ddata->removalint;
    ch->cah_CardInserted = &ddata->insertint;
    ch->cah_CardStatus->is_Data = ddata;
    ch->cah_CardStatus->is_Code = (void*)IDE_PCMCIA_Handler;
#if 0
    ch->cah_CardRemoved->is_Data = ddata;
    ch->cah_CardRemoved->is_Code = (void*)IDE_PCMCIA_Removed;
    ch->cah_CardInserted->is_Data = ddata;
    ch->cah_CardInserted->is_Code = (void*)IDE_PCMCIA_Inserted
#endif

    if (!OwnCard(ch)) {
        BeginCardAccess(ch);

        if (detectcard(ddata)) {
            initializecard(ddata);
            bdata->ddata = ddata;
            bdata->port = (UBYTE*)ddata->cmm->cmm_IOMemory;
            LIBBASE->ata_NoDMA = TRUE;
            ata_RegisterBus((IPTR)ddata->cmm->cmm_IOMemory, (IPTR)(ddata->cmm->cmm_IOMemory + 14 - ata_AltControl), 2, 0, ARBF_EarlyInterrupt, &amiga_driver_pcmcia, bdata, LIBBASE);
            return TRUE;
        }

        EndCardAccess(ch);
        ReleaseCard(ch, CARDF_REMOVEHANDLE);
    }

    FreeVec(ddata);

    return FALSE;
}

static int ata_amiga_init(struct ataBase *LIBBASE)
{
    BOOL r_ide, r_pcmcia;

    r_ide = ata_amiga_ide_init(LIBBASE);
    r_pcmcia = ata_amiga_pcmcia_init(LIBBASE);
    return (r_ide || r_pcmcia) ? 1 : 0;
}

ADD2INITLIB(ata_amiga_init, 20)
