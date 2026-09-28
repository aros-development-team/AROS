/*
    Copyright (C) 2010-2026, The AROS Development Team. All rights reserved.

    Desc: hidd.i2c driven by the BCM283x/BCM2711 BSC controller.
          The BSC runs whole transactions, so only WriteRead/ProbeAddress work.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/gpio.h>
#include <proto/mbox.h>

#include <hidd/i2c.h>
#include <utility/tagitem.h>

#include <hardware/bcm2708.h>
#include <hardware/videocore.h>

#include "i2c-bcm2708.h"

#include LC_LIBDEFS_FILE

#define GPIO_FSEL_IN            0
#define GPIO_FSEL_OUT           1
#define GPIO_FSEL_ALT0          4

APTR KernelBase __attribute__((used)) = NULL;

static const struct
{
    ULONG   offset;
    UBYTE   sda, scl;
} bsc_units[BSC_UNITS] =
{
    { 0x205000, 0, 1 },     /* BSC0 */
    { 0x804000, 2, 3 },     /* BSC1 */
};

/* Core (VPU) clock feeds the BSC divider; 0 if the firmware won't say. */
static ULONG bsc_core_clock(IPTR peri)
{
    APTR MBoxBase = OpenResource("mbox.resource");
    ULONG *raw, *msg, rate = 0;

    if (!MBoxBase)
        return 0;

    raw = AllocMem(MBOX_MSG_ALIGN + (MBOX_MSG_ALIGN - 1), MEMF_PUBLIC | MEMF_CLEAR);
    if (!raw)
        return 0;
    msg = (ULONG *)(((IPTR)raw + (MBOX_MSG_ALIGN - 1)) & ~(IPTR)(MBOX_MSG_ALIGN - 1));

    msg[0] = AROS_LONG2LE(8 * sizeof(ULONG));
    msg[1] = AROS_LONG2LE(VCTAG_REQ);
    msg[2] = AROS_LONG2LE(VCTAG_GETCLKRATE);
    msg[3] = AROS_LONG2LE(2 * sizeof(ULONG));
    msg[4] = 0;
    msg[5] = AROS_LONG2LE(VCCLOCK_CORE);
    msg[6] = 0;
    msg[7] = 0;

    if (MBoxCall((void *)(peri + VCMB_OFFSET), VCMB_PROPCHAN, msg) == (volatile unsigned int *)msg &&
        (AROS_LE2LONG(msg[4]) & VCTAG_RESP))
        rate = AROS_LE2LONG(msg[6]);

    FreeMem(raw, MBOX_MSG_ALIGN + (MBOX_MSG_ALIGN - 1));
    return rate;
}

/* A microsecond or more; i2c has no lower speed limit. */
static void bsc_delay(IPTR base)
{
    int i;

    for (i = 0; i < 100; i++)
        (void)bsc_rd(base, BSC_S);
}

/*
 * A slave cut off mid-byte holds SDA low, and a battery-backed RTC keeps
 * doing so across reboots. Clock SCL until it lets go, then send a STOP
 * (UM10204 3.1.16).
 */
static void bsc_bus_clear(struct bsc_unit *u)
{
    APTR GPIOBase = OpenResource("gpio.resource");
    int i;

    if (!GPIOBase || GPIOGet(u->sda))
        return;

    GPIOSetFunc(u->sda, GPIO_FSEL_IN);
    GPIOSet(u->scl, 1);
    GPIOSetFunc(u->scl, GPIO_FSEL_OUT);
    for (i = 0; i < 9 && !GPIOGet(u->sda); i++)
    {
        GPIOSet(u->scl, 0);
        bsc_delay(u->base);
        GPIOSet(u->scl, 1);
        bsc_delay(u->base);
    }

    /* STOP: SDA rises while SCL is high */
    GPIOSet(u->scl, 0);
    GPIOSet(u->sda, 0);
    GPIOSetFunc(u->sda, GPIO_FSEL_OUT);
    bsc_delay(u->base);
    GPIOSet(u->scl, 1);
    bsc_delay(u->base);
    GPIOSetFunc(u->sda, GPIO_FSEL_IN);
    bsc_delay(u->base);

    D(bug("[I2C-BCM2708] SDA was stuck low; %d clocks, now %u\n",
          i, GPIOGet(u->sda)));

    GPIOSetFunc(u->sda, GPIO_FSEL_ALT0);
    GPIOSetFunc(u->scl, GPIO_FSEL_ALT0);
}

static void bsc_unit_setup(struct bsc_staticdata *psd, ULONG n)
{
    struct bsc_unit *u = &psd->unit[n];
    APTR GPIOBase;
    ULONG core, div;

    if (u->ready)
        return;

    GPIOBase = OpenResource("gpio.resource");
    if (GPIOBase)
    {
        GPIOSetFunc(u->sda, GPIO_FSEL_ALT0);
        GPIOSetFunc(u->scl, GPIO_FSEL_ALT0);
    }
    bsc_bus_clear(u);

    /* SCL = core / DIV, DIV even. Keep the firmware's value if unknown. */
    core = bsc_core_clock(psd->periiobase);
    if (core)
    {
        div = (core + BSC_BUS_HZ - 1) / BSC_BUS_HZ;
        div = (div + 1) & ~1;
        bsc_wr(u->base, BSC_DIV, div);
    }

    D(bug("[I2C-BCM2708] BSC%u @ 0x%p, core %u Hz, DIV %u\n",
          n, (APTR)u->base, core, bsc_rd(u->base, BSC_DIV)));
    /* ALT0 on both pins = 0x24 at the pin pair's FSEL bits; idle lines read 1 */
    D(bug("[I2C-BCM2708] GPFSEL0 %08x, SDA %u SCL %u\n",
          bsc_rd(psd->periiobase + 0x200000, 0),
          GPIOBase ? GPIOGet(u->sda) : 9, GPIOBase ? GPIOGet(u->scl) : 9));

    u->ready = TRUE;
}

/*
 * The BSC has no repeated START, so a write-then-read is a write, STOP,
 * then a read. RTCs, EEPROMs and most sensors keep their register
 * pointer across the STOP.
 */
static BOOL bsc_transfer(struct bsc_unit *u, UBYTE addr,
                         const UBYTE *wbuf, ULONG wlen,
                         UBYTE *rbuf, ULONG rlen)
{
    IPTR b = u->base;
    ULONG s = 0, spins, got = 0;
    BOOL ok = TRUE;

    if ((wlen == 0 && rlen == 0) || wlen > 0xffff || rlen > 0xffff)
        return FALSE;

    /* A NACK leaves TA stuck; only clearing I2CEN aborts it. */
    bsc_wr(b, BSC_C, BSC_CONTROL_CLEAR);
    bsc_wr(b, BSC_S, BSC_CLEAR);
    bsc_wr(b, BSC_C, BSC_CONTROL_I2CEN | BSC_CONTROL_CLEAR);
    bsc_wr(b, BSC_A, addr);

    if (wlen)
    {
        bsc_wr(b, BSC_DLEN, wlen);
        while (wlen && (bsc_rd(b, BSC_S) & BSC_STATUS_TXD))
        {
            bsc_wr(b, BSC_FIFO, *wbuf++);
            wlen--;
        }

        bsc_wr(b, BSC_C, BSC_CONTROL_I2CEN | BSC_CONTROL_ST);
        for (spins = 0; ok; )
        {
            s = bsc_rd(b, BSC_S);
            if (s & (BSC_STATUS_ERR | BSC_STATUS_CLKT))
                ok = FALSE;
            else if (s & BSC_STATUS_DONE)
                break;
            else if (wlen && (s & BSC_STATUS_TXD))
            {
                bsc_wr(b, BSC_FIFO, *wbuf++);
                wlen--;
                spins = 0;
            }
            else if (++spins >= BSC_SPIN_LIMIT)
                ok = FALSE;
        }
        ok = ok && wlen == 0;
        bsc_wr(b, BSC_S, BSC_CLEAR);
    }

    if (ok && rlen)
    {
        bsc_wr(b, BSC_DLEN, rlen);
        bsc_wr(b, BSC_C, BSC_CONTROL_I2CEN | BSC_CONTROL_ST | BSC_CONTROL_READ);
    }

    /* DONE may come before the last bytes are drained; count them instead. */
    for (spins = 0; ok && got < rlen; )
    {
        s = bsc_rd(b, BSC_S);
        if (s & BSC_STATUS_RXD)
        {
            rbuf[got++] = (UBYTE)bsc_rd(b, BSC_FIFO);
            spins = 0;
        }
        else if (s & (BSC_STATUS_ERR | BSC_STATUS_CLKT | BSC_STATUS_DONE))
            ok = FALSE;
        else if (++spins >= BSC_SPIN_LIMIT)
            ok = FALSE;
    }

    /* The last byte comes before NACK + STOP; disabling earlier drops the STOP */
    for (spins = 0; ok && rlen; )
    {
        s = bsc_rd(b, BSC_S);
        if (s & BSC_STATUS_DONE)
            break;
        if ((s & (BSC_STATUS_ERR | BSC_STATUS_CLKT)) || ++spins >= BSC_SPIN_LIMIT)
            ok = FALSE;
    }

    /* s is the status the failing check saw */
    if (!ok)
        D(bug("[I2C-BCM2708] %02x failed: wleft %u, got %u/%u, S=%08x DLEN=%u\n",
              addr, wlen, got, rlen, s, bsc_rd(b, BSC_DLEN)));

    bsc_wr(b, BSC_C, BSC_CONTROL_CLEAR);
    bsc_wr(b, BSC_S, BSC_CLEAR);

    if (!ok)
        bsc_bus_clear(u);

    return ok;
}

OOP_Object *I2CBCM2708__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct bsc_staticdata *psd = PSD(cl);
    static CONST_STRPTR names[BSC_UNITS] = { "bsc0", "bsc1" };
    struct pRoot_New superMsg;
    struct TagItem superTags[2];
    OOP_Object *bus;
    IPTR n;

    n = GetTagData(aHidd_I2C_BCM2708_Unit, 1, msg->attrList);
    if (n >= BSC_UNITS)
        return NULL;

    superTags[0].ti_Tag  = aHidd_I2C_Name;
    superTags[0].ti_Data = (IPTR)names[n];
    superTags[1].ti_Tag  = TAG_MORE;
    superTags[1].ti_Data = (IPTR)msg->attrList;
    superMsg.mID      = msg->mID;
    superMsg.attrList = superTags;

    bus = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&superMsg);
    if (bus)
    {
        struct bsc_busdata *data = OOP_INST_DATA(cl, bus);

        data->unit = &psd->unit[n];

        ObtainSemaphore(&data->unit->lock);
        bsc_unit_setup(psd, n);
        ReleaseSemaphore(&data->unit->lock);
    }

    return bus;
}

BOOL I2CBCM2708__Hidd_I2C__WriteRead(OOP_Class *cl, OOP_Object *o, struct pHidd_I2C_WriteRead *msg)
{
    struct bsc_busdata *data = OOP_INST_DATA(cl, o);
    IPTR address = 0;
    BOOL ok;

    if (msg->device)
        OOP_GetAttr(msg->device, aHidd_I2CDevice_Address, &address);

    /* hidd.i2c carries the address with the R/W bit; the BSC wants 7 bits */
    ObtainSemaphore(&data->unit->lock);
    ok = bsc_transfer(data->unit, (UBYTE)((address >> 1) & 0x7f),
                      msg->writeBuffer, msg->writeLength,
                      msg->readBuffer, msg->readLength);
    ReleaseSemaphore(&data->unit->lock);

    return ok;
}

BOOL I2CBCM2708__Hidd_I2C__ProbeAddress(OOP_Class *cl, OOP_Object *o, struct pHidd_I2C_ProbeAddress *msg)
{
    struct bsc_busdata *data = OOP_INST_DATA(cl, o);
    UBYTE dummy;
    BOOL ok;

    ObtainSemaphore(&data->unit->lock);
    ok = bsc_transfer(data->unit, (UBYTE)((msg->address >> 1) & 0x7f),
                      NULL, 0, &dummy, 1);
    ReleaseSemaphore(&data->unit->lock);

    return ok;
}

/* Byte-level methods can't be expressed on the BSC. */

BOOL I2CBCM2708__Hidd_I2C__Start(OOP_Class *cl, OOP_Object *o, struct pHidd_I2C_Start *msg)
{
    return FALSE;
}

void I2CBCM2708__Hidd_I2C__Stop(OOP_Class *cl, OOP_Object *o, struct pHidd_I2C_Stop *msg)
{
}

BOOL I2CBCM2708__Hidd_I2C__Address(OOP_Class *cl, OOP_Object *o, struct pHidd_I2C_Address *msg)
{
    return FALSE;
}

BOOL I2CBCM2708__Hidd_I2C__PutByte(OOP_Class *cl, OOP_Object *o, struct pHidd_I2C_PutByte *msg)
{
    return FALSE;
}

BOOL I2CBCM2708__Hidd_I2C__GetByte(OOP_Class *cl, OOP_Object *o, struct pHidd_I2C_GetByte *msg)
{
    return FALSE;
}

/* Init runs without a class pointer; use the bases directly. */
#undef OOPBase
#undef UtilityBase

/* The superclass must exist before the class is made. */
ADD2LIBS((STRPTR)"i2c.hidd", 0, static struct Library *, I2CBase);

static int I2CBCM2708_Init(LIBBASETYPEPTR LIBBASE)
{
    struct bsc_staticdata *psd = &LIBBASE->psd;
    struct Library *OOPBase = psd->OOPBase;
    ULONG n;

    KernelBase = OpenResource("kernel.resource");
    if (!KernelBase)
        return FALSE;

    /* The Pi 5 header bus is on RP1, not a BSC. */
    psd->periiobase = (IPTR)KrnGetSystemAttr(KATTR_PeripheralBase);
    if (!psd->periiobase || psd->periiobase == (IPTR)-1 ||
        psd->periiobase == BCM2712_PERIIOBASE)
        return FALSE;

    psd->utilityBase = TaggedOpenLibrary(TAGGEDOPEN_UTILITY);
    if (!psd->utilityBase)
        return FALSE;

    psd->hiddI2CAB        = OOP_ObtainAttrBase(IID_Hidd_I2C);
    psd->hiddI2CDeviceAB  = OOP_ObtainAttrBase(IID_Hidd_I2CDevice);
    psd->hiddI2CBCM2708AB = OOP_ObtainAttrBase(IID_Hidd_I2C_BCM2708);
    if (!psd->hiddI2CAB || !psd->hiddI2CDeviceAB || !psd->hiddI2CBCM2708AB)
        return FALSE;

    for (n = 0; n < BSC_UNITS; n++)
    {
        InitSemaphore(&psd->unit[n].lock);
        psd->unit[n].base  = psd->periiobase + bsc_units[n].offset;
        psd->unit[n].sda   = bsc_units[n].sda;
        psd->unit[n].scl   = bsc_units[n].scl;
        psd->unit[n].ready = FALSE;
    }

    return TRUE;
}

ADD2INITLIB(I2CBCM2708_Init, 0)
