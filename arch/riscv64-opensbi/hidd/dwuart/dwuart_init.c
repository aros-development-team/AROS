/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Library housekeeping and UART discovery for the DesignWare
          APB UART driver.

    There is no probing to be done here - a memory mapped UART cannot be
    found by poking at fixed addresses the way a PC's can. Everything the
    driver needs (where the registers are, how they are laid out, which
    interrupt source they raise and what clocks them) comes from the
    device tree the firmware handed over.
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/kernel.h>
#include <proto/utility.h>

#include <aros/symbolsets.h>
#include <aros/kernel.h>

#include <exec/types.h>
#include <utility/tagitem.h>

#include <fdt.h>

#include "serial_intern.h"

#include LC_LIBDEFS_FILE

/*
 * Bindings a 16550-alike is described by. A part normally names both
 * itself and the generic register layout it follows, so the same node
 * can turn up under more than one of these.
 */
static CONST_STRPTR const dwuart_compatible[] =
{
    "snps,dw-apb-uart",
    "ns16550a",
    NULL
};

/*
 * A node the firmware has turned off must be left alone - its clocks or
 * pinmux may not be set up, and touching the registers can hang the bus.
 */
static BOOL dwuart_enabled(fdt_node_t node)
{
    CONST_STRPTR status = FDT_GetPropStr(node, "status");

    /* Absent means enabled; otherwise only "ok"/"okay" count */
    if (!status)
        return TRUE;

    return (status[0] == 'o' && status[1] == 'k');
}

static BOOL dwuart_known(struct class_static_data *csd, IPTR base)
{
    ULONG i;

    for (i = 0; i < csd->nports; i++)
    {
        if (csd->ports[i].base == base)
            return TRUE;
    }

    return FALSE;
}

static void dwuart_discover(struct class_static_data *csd)
{
    APTR KernelBase = csd->kernelBase;
    ULONG i;

    for (i = 0; dwuart_compatible[i]; i++)
    {
        fdt_node_t node = FDT_NONE;

        while ((node = FDT_FindCompatible(node, dwuart_compatible[i]))
               != FDT_NONE)
        {
            struct dwuart_port *port;
            const ULONG *ints;
            UQUAD addr = 0, size = 0;
            ULONG len = 0;

            if (csd->nports >= SER_MAX_UNITS)
                return;

            if (!dwuart_enabled(node))
                continue;

            if (!FDT_GetReg(node, 0, &addr, &size))
                continue;

            if (dwuart_known(csd, (IPTR)addr))
                continue;

            port = &csd->ports[csd->nports];

            port->base = (IPTR)addr;
            port->size = (IPTR)size;

            /*
             * A bus that declares no size cells leaves us without one.
             * A page covers every 16550 layout there is.
             */
            if (!port->size)
                port->size = 4096;

            /*
             * "reg-shift" and "reg-io-width" describe how far apart the
             * registers sit and how wide an access has to be. Their
             * defaults are the plain 8-bit layout.
             */
            port->regshift = (UBYTE)FDT_GetPropU32(node, "reg-shift", 0);
            port->regwidth = (UBYTE)FDT_GetPropU32(node, "reg-io-width", 1);

            /*
             * The reference clock. Boards that describe it through a
             * clock controller instead leave this absent, and then the
             * divisor cannot be worked out - see set_baudrate().
             */
            port->clock = FDT_GetPropU32(node, "clock-frequency", 0);

            /*
             * The interrupt controller is the PLIC, whose specifier is
             * a single cell holding the source number.
             */
            port->irq = 0;
            ints = FDT_GetProp(node, "interrupts", &len);
            if (ints && len >= sizeof(ULONG))
                port->irq = (ULONG)FDT_ReadCells(ints, 1);

            D(bug("[Serial:DW] %s: %s @ 0x%p+0x%p shift %u width %u"
                  " clk %u irq %u\n", __func__, FDT_NodeName(node),
                  port->base, port->size, port->regshift, port->regwidth,
                  port->clock, port->irq);)

            /*
             * The boot page tables cover RAM only, so the registers
             * have to be brought into the address space before anything
             * can be read from them.
             */
            if (!KrnMapGlobal((APTR)port->base, (APTR)port->base,
                              (ULONG)port->size,
                              MAP_Readable | MAP_Writable))
            {
                D(bug("[Serial:DW] %s: could not map 0x%p\n", __func__,
                      port->base);)
                continue;
            }

            csd->nports++;
        }
    }
}

static int DWSer_Init(LIBBASETYPEPTR LIBBASE)
{
    struct class_static_data *csd = &LIBBASE->hdg_csd;
    struct TagItem *bootinfo;
    APTR KernelBase;
    APTR dtb;

    D(bug("[Serial:DW] %s()\n", __func__);)

    KernelBase = OpenResource("kernel.resource");
    csd->kernelBase = KernelBase;
    if (!KernelBase)
        return FALSE;

    __IHidd = OOP_ObtainAttrBase(IID_Hidd);
    __IHidd_SerialUnitAB = OOP_ObtainAttrBase(IID_Hidd_SerialUnit);

    if (!__IHidd || !__IHidd_SerialUnitAB)
    {
        bug("[Serial:DW] %s: ObtainAttrBases failed\n", __func__);
        return FALSE;
    }

    bootinfo = KrnGetBootInfo();
    dtb = (APTR)GetTagData(KRN_FlattenedDeviceTree, 0, bootinfo);
    if (!dtb || !FDT_Open(dtb))
    {
        D(bug("[Serial:DW] %s: no device tree, nothing to do\n", __func__);)
        return FALSE;
    }

    dwuart_discover(csd);

    if (!csd->nports)
    {
        D(bug("[Serial:DW] %s: no UART described\n", __func__);)
        return FALSE;
    }

    D(bug("[Serial:DW] %s: %u port(s)\n", __func__, csd->nports);)

    return TRUE;
}

ADD2INITLIB(DWSer_Init, 0)

static int DWSer_Expunge(LIBBASETYPEPTR LIBBASE)
{
    struct class_static_data *csd = &LIBBASE->hdg_csd;

    D(bug("[Serial:DW] %s()\n", __func__);)

    OOP_ReleaseAttrBase(IID_Hidd_SerialUnit);
    OOP_ReleaseAttrBase(IID_Hidd);

    __IHidd_SerialUnitAB = 0;
    __IHidd = 0;

    return TRUE;
}

ADD2EXPUNGELIB(DWSer_Expunge, 0)
