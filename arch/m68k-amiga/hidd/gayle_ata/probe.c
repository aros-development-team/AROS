/*
    Copyright © 2013-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: A600/A1200/A4000 ATA HIDD hardware detection routine
    Lang: English
*/

#define DEBUG 1
#include <aros/debug.h>

#define __OOP_NOMETHODBASES__

#include <aros/asmcall.h>
#include <aros/symbolsets.h>
#include <asm/io.h>
#include <exec/lists.h>
#include <exec/rawfmt.h>
#include <hidd/ata.h>
#include <hidd/hidd.h>
#include <oop/oop.h>
#include <proto/exec.h>
#include <proto/oop.h>

#include <hardware/custom.h>
#include <graphics/gfxbase.h>
#include <hardware/ata.h>

#include <string.h>

#include "bus_class.h"
#include "interface_pio.h"

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

static BOOL isFastATA(struct ata_ProbedBus *ddata)
{
    return FALSE;
}

static UBYTE *getport(struct ata_ProbedBus *ddata)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;
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
        // AGA does not have custom mirror here but lets make sure..
        if (!custom_check((APTR)0xdd4000) && (custom->vposr & 0x7f00) >= 0x2200) {
            port = (UBYTE*)GAYLE_BASE_4000;
            ddata->a4000 = TRUE;
            ddata->gayleirqbase = (UBYTE*)GAYLE_IRQ_4000;
        }
    }
    Enable();
    CloseLibrary((struct Library*)gfx);

    // Detect FastATA... FIXME: the check is flawed for an a4000, disabled for now.
#if (0)
    if (ddata->gayleirqbase)
    {
        altport = (UBYTE*)GAYLE_BASE_FASTATA;
        Disable();
        status1 = altport[GAYLE_BASE_FASTATA_PIO0 + GAYLE_FASTATA_PIO_STAT];
        status2 = altport[GAYLE_BASE_FASTATA_PIO3 + GAYLE_FASTATA_STAT];
        Enable();
        D(bug("[ATA:Gayle] Status=%02x,%02x\n", status1, status2);)
        if ((status1 & 0xfd) == (status2 & 0xfd))
        {
            port = (UBYTE*)altport;
            ddata->gayleirqbase = (UBYTE*)GAYLE_IRQ_FASTATA;
        }
    }
#endif
    if (port == NULL)
    {
        D(bug("[ATA:Gayle] No Gayle ATA Detected (ID=%02x)\n", id);)
        return NULL;
    }

    ddata->port = (UBYTE*)port;
    if ((ddata->port == (UBYTE*)GAYLE_BASE_1200) || (ddata->port == (UBYTE*)GAYLE_BASE_4000))
    {
        D(bug("[ATA:Gayle] Possible Gayle IDE port @ %08x (ID=%02x)\n", (ULONG)port & ~3, id);)
        altport = port + 0x1010;
    }
    else
    {
        D(bug("[ATA:Gayle] Possible FastATA IDE port @ %08x (ID=%02x)\n", (ULONG)port & ~3, id);)
        altport = NULL;
    }
    ddata->altport = (UBYTE*)altport;

    Disable();
    port[ata_DevHead * 4] = ATAF_ERROR;
    /* If nothing connected, we get back what we wrote, ATAF_ERROR set */
    status1 = port[ata_Status * 4];
    port[ata_DevHead * 4] = ATAF_DATAREQ;
    status2 = port[ata_Status * 4];
    port[ata_DevHead * 4] = 0;
    Enable();

    D(bug("[ATA:Gayle] Status=%02x,%02x\n", status1, status2);)
    // BUSY and DRDY both active or ERROR/DATAREQ = no drive(s) = do not install driver
    if (   (((status1 | status2) & (ATAF_BUSY | ATAF_DRDY)) == (ATAF_BUSY | ATAF_DRDY))
        || ((status1 | status2) & (ATAF_ERROR | ATAF_DATAREQ)))
    {
        D(bug("[ATA:Gayle] No Devices detected\n");)
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
        port[ata_DevHead * 4] = 1;
        v1 = altport[ata_AltControl * 4];
        altport[ata_AltControl * 4] = 2;
        port[ata_DevHead * 4] = 4;
        v2 = altport[ata_AltControl * 4];
        altport[ata_AltControl * 4] = 0;
        port[ata_DevHead * 4] = 0;
        Enable();
        if ((v1 == 0 && v2 == 2) || (v1 == 1 && v2 == 4) || (v1 == 0xff && v2 == 0xff)) {
            ddata->doubler = 2;
        } else {
            ddata->doubler = 0;
        }
        D(bug("[ATA:Gayle] IDE doubler check (%02X, %02X) = %d\n", v1, v2, ddata->doubler);)
        ddata->altport = NULL;
    }
    /* we may have connected drives */
    return (UBYTE*)port;
}

static int ata_Scan(struct ataBase *base)
{
    struct ata_ProbedBus *probedbus;
    OOP_Class *busClass = base->GayleBusClass;

    probedbus = AllocVec(sizeof(struct ata_ProbedBus), MEMF_ANY | MEMF_CLEAR);
    if (probedbus && getport(probedbus)) {
        OOP_Object *ata = OOP_NewObject(NULL, CLID_HW_ATA, NULL);
        if (ata) {
            HWBase = OOP_GetMethodID(IID_HW, 0);
            struct TagItem attrs[] =
            {
                {aHidd_DriverData         , (IPTR)probedbus                    },
                {aHidd_ATABus_PIODataSize , sizeof(struct pio_data)            },
                {aHidd_ATABus_BusVectors  , (IPTR)bus_FuncTable                },
                {aHidd_ATABus_PIOVectors  , (IPTR)pio_FuncTable                },
                {aHidd_ATABus_KeepEmpty   , FALSE                              },
                {TAG_DONE                 , 0                                  }
            };
            OOP_Object *bus;

            /*
             * We use this field as ownership indicator.
             * The trick is that HW_AddDriver() fails if either object creation fails
             * or subsystem-side setup fails. In the latter case our object will be
             * disposed.
             * We need to know whether OOP_DisposeObject() or we should deallocate
             * this structure on failure.
             */
            probedbus->atapb_Node.ln_Succ = NULL;

            /*
             * Check if we have a FastATA adaptor
             */
            if (isFastATA(probedbus))
                busClass = base->FastATABusClass;

            bus = HW_AddDriver(ata, busClass, attrs);
            if (bus)
                return TRUE;
            D(bug("[ATA:Gayle] Failed to create object for device IO: %x:%x IRQ: %x\n",
                probedbus->port, probedbus->altport, probedbus->gayleirqbase);)

            /*
             * Free the structure only upon object creation failure!
             * In case of success it becomes owned by the driver object!
             */
            if (!probedbus->atapb_Node.ln_Succ)
                 FreeVec(probedbus);
            return TRUE;
        }
    }
    FreeVec(probedbus);

    return TRUE;
}

ADD2INITLIB(ata_Scan, 30)
