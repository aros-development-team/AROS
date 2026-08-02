#define DEBUG 1
/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Finding the i2c controllers.

    Nothing here knows anything about i2c signalling; it reads the
    device tree and maps what it finds, so that the class in
    dwi2c_class.c has something to be a bus object for.
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/kernel.h>
#include <proto/utility.h>

#include <aros/symbolsets.h>
#include <aros/kernel.h>

#include <exec/types.h>
#include <hidd/hidd.h>
#include <hidd/i2c.h>
#include <utility/tagitem.h>

#include <fdt.h>

#include LC_LIBDEFS_FILE
#include "dwi2c_intern.h"

/*
 * Nothing in this file runs with a class pointer in scope, so the
 * shorthands that resolve through one have to go; each function takes
 * the bases it needs from the static data instead.
 */
#undef OOPBase
#undef UtilityBase
#undef HiddAttrBase
#undef HiddI2CAttrBase
#undef HiddI2CDeviceAttrBase
#undef HiddI2CDWAttrBase

/*
 * Which nodes are one of these controllers. "snps,designware-i2c" is
 * the generic name; a part that behaves like one names it in its
 * compatible list alongside its own, so matching the generic name
 * covers the family.
 */
static CONST_STRPTR dwi2c_compatible[] =
{
    "snps,designware-i2c",
    "snps,dw-apb-i2c"
};

/*
 * Follow a phandle. The device tree library has no index of them, so
 * this is a walk - which is fine for something done a couple of times
 * at init.
 */
static fdt_node_t dwi2c_findphandle(ULONG phandle)
{
    fdt_node_t n;

    if (!phandle || phandle == 0xffffffff)
        return FDT_NONE;

    for (n = FDT_Root(); n != FDT_NONE; n = FDT_NextNode(n))
    {
        if (FDT_GetPropU32(n, "phandle", 0) == phandle ||
            FDT_GetPropU32(n, "linux,phandle", 0) == phandle)
            return n;
    }

    return FDT_NONE;
}

/*
 * The rate the controller itself is clocked at, which is what the SCL
 * counts are derived from. It is not on the i2c node - "clock-frequency"
 * there is the speed of the bus, a different thing entirely - but on
 * whatever the node's "clocks" property points at.
 *
 * That only leads anywhere when the source is a fixed clock, which
 * states its rate outright. A clock behind a controller would need that
 * controller's driver to ask, and there is none, so this returns zero
 * and the timing registers are left as the firmware set them.
 */
static ULONG dwi2c_inputclock(fdt_node_t node)
{
    ULONG len = 0;
    const ULONG *clocks = FDT_GetProp(node, "clocks", &len);

    if (clocks && len >= 4)
    {
        fdt_node_t src = dwi2c_findphandle((ULONG)FDT_ReadCells(&clocks[0], 1));

        if (src != FDT_NONE)
            return FDT_GetPropU32(src, "clock-frequency", 0);
    }

    return 0;
}

static BOOL dwi2c_enabled(fdt_node_t node)
{
    CONST_STRPTR status = FDT_GetPropStr(node, "status");

    /* Absent means enabled; a board describes every controller it has
       and marks the ones that are not wired up */
    if (!status)
        return TRUE;

    return (status[0] == 'o' && status[1] == 'k') ||    /* "ok", "okay" */
           (status[0] == '\0');
}

static BOOL dwi2c_add(struct dwi2c_staticdata *psd, fdt_node_t node)
{
    APTR KernelBase = psd->kernelBase;
    struct dwi2c_ctrl *ctrl;
    UQUAD addr = 0, size = 0;
    ULONG len = 0;
    const ULONG *irq;
    ULONG unit = psd->ctrlCount;

    if (unit >= DWI2C_MAX_CTRL)
    {
        D(bug("[DWI2C] more controllers than we can hold\n");)
        return FALSE;
    }

    if (!dwi2c_enabled(node))
    {
        D(bug("[DWI2C] %s is disabled\n", FDT_NodeName(node));)
        return FALSE;
    }

    if (!FDT_GetReg(node, 0, &addr, &size) || !addr || !size)
    {
        D(bug("[DWI2C] %s has no registers\n", FDT_NodeName(node));)
        return FALSE;
    }

    ctrl = &psd->ctrl[unit];

    ctrl->base = (IPTR)addr;
    ctrl->size = (IPTR)size;

    /* On an i2c node this is the speed of the bus, not of the block */
    ctrl->busSpeed = FDT_GetPropU32(node, "clock-frequency", 100000);
    ctrl->inputClock = dwi2c_inputclock(node);
    ctrl->sdaHoldNs = FDT_GetPropU32(node, "i2c-sda-hold-time-ns", 0);

    /*
     * The first cell of "interrupts" is the source number at the
     * interrupt controller. Recorded for the day this driver grows an
     * interrupt path; the polled one has no use for it.
     */
    ctrl->irq = 0;
    irq = FDT_GetProp(node, "interrupts", &len);
    if (irq && len >= 4)
        ctrl->irq = (ULONG)FDT_ReadCells(&irq[0], 1);

    ctrl->name[0] = 'i';
    ctrl->name[1] = '2';
    ctrl->name[2] = 'c';
    ctrl->name[3] = (char)('0' + (unit % 10));
    ctrl->name[4] = '\0';

    /*
     * The boot page tables cover RAM and nothing else, so the register
     * block has to be mapped before it can be touched at all.
     */
    if (!KrnMapGlobal((APTR)ctrl->base, (APTR)ctrl->base, (ULONG)ctrl->size,
                      MAP_Readable | MAP_Writable))
    {
        D(bug("[DWI2C] %s: could not map %p+%p\n", FDT_NodeName(node),
              ctrl->base, ctrl->size);)
        ctrl->base = 0;
        return FALSE;
    }

    InitSemaphore(&ctrl->lock);

    DWI2C_HWProbeFIFO(ctrl);
    DWI2C_HWCalcTiming(ctrl);

    D(bug("[DWI2C] %s: %s @ %p+%p, bus %uHz, input %uHz, irq %u, "
          "FIFO %u/%u\n", ctrl->name, FDT_NodeName(node), ctrl->base,
          ctrl->size, ctrl->busSpeed, ctrl->inputClock, ctrl->irq,
          ctrl->txDepth, ctrl->rxDepth);)

    if (!ctrl->inputClock)
    {
        D(bug("[DWI2C] %s: no input clock in the tree - keeping the "
              "clock shape the firmware left\n", ctrl->name);)
    }

    psd->ctrlCount++;

    return TRUE;
}

static ULONG dwi2c_discover(struct dwi2c_staticdata *psd)
{
    ULONG i;

    for (i = 0; i < sizeof(dwi2c_compatible) / sizeof(dwi2c_compatible[0]);
         i++)
    {
        fdt_node_t node = FDT_NONE;

        while ((node = FDT_FindCompatible(node, dwi2c_compatible[i]))
               != FDT_NONE)
        {
            /*
             * A node usually names both the generic part and its own,
             * so the same controller can turn up under more than one of
             * the names above. Take it once.
             */
            if (i > 0 && FDT_IsCompatible(node, dwi2c_compatible[0]))
                continue;

            dwi2c_add(psd, node);
        }
    }

    return psd->ctrlCount;
}

/*****************************************************************************

    Library housekeeping.

*****************************************************************************/

/*
 * The base class has to exist before ours can be built on it, and
 * set_open_libraries() runs before the class is created - which the
 * init functions below do not. So it is opened here rather than by
 * hand. A failure aborts the module, which is right: without hidd.i2c
 * there is no class to subclass and nothing this driver could do.
 */
ADD2LIBS((STRPTR)"i2c.hidd", 0, static struct Library *, __dwi2c_i2cbase);

static int DWI2C_Init(LIBBASETYPEPTR LIBBASE)
{
    struct dwi2c_staticdata *psd = &LIBBASE->psd;
    struct Library *OOPBase = psd->OOPBase;
    struct Library *UtilityBase;
    APTR KernelBase;
    APTR bootinfo;
    APTR dtb;

    D(bug("[DWI2C] %s()\n", __func__);)

    psd->i2cBase = __dwi2c_i2cbase;

    KernelBase = OpenResource("kernel.resource");
    psd->kernelBase = KernelBase;
    if (!KernelBase)
        return FALSE;

    UtilityBase = TaggedOpenLibrary(TAGGEDOPEN_UTILITY);
    psd->utilityBase = UtilityBase;
    if (!UtilityBase)
        return FALSE;

    psd->hiddAB = OOP_ObtainAttrBase(IID_Hidd);
    psd->hiddI2CAB = OOP_ObtainAttrBase(IID_Hidd_I2C);
    psd->hiddI2CDeviceAB = OOP_ObtainAttrBase(IID_Hidd_I2CDevice);
    psd->hiddI2CDWAB = OOP_ObtainAttrBase(IID_Hidd_I2C_DW);

    if (!psd->hiddAB || !psd->hiddI2CAB || !psd->hiddI2CDeviceAB ||
        !psd->hiddI2CDWAB)
    {
        bug("[DWI2C] %s: ObtainAttrBases failed\n", __func__);
        return FALSE;
    }

    /* The firmware's device tree, as kernel.resource recorded it */
    bootinfo = KrnGetBootInfo();
    dtb = (APTR)GetTagData(KRN_FlattenedDeviceTree, 0,
                           (struct TagItem *)bootinfo);
    if (!dtb || !FDT_Open(dtb))
    {
        D(bug("[DWI2C] %s: no device tree, nothing to do\n", __func__);)
        return FALSE;
    }

    if (!dwi2c_discover(psd))
    {
        D(bug("[DWI2C] %s: no controllers described\n", __func__);)
        return FALSE;
    }

    D(bug("[DWI2C] %s: %u controller(s)\n", __func__, psd->ctrlCount);)

    return TRUE;
}

ADD2INITLIB(DWI2C_Init, 0)
