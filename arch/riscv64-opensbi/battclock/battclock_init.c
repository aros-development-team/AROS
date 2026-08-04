/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: opensbi battclock.resource - finding the RTC.

    SBI offers no wall clock service, so the time has to come from a real
    time clock on the board. Which part that is, and what it hangs off,
    is only known from the device tree the firmware handed over, so the
    resource looks itself up there rather than assuming a bus. A machine
    that describes no RTC - every virtual one - is not an error: the
    resource then only remembers what was written to it, which is what
    this port did before an RTC was supported at all.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <aros/kernel.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>

#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/utility.h>

#include <utility/tagitem.h>

#include <fdt.h>

#include "battclock_intern.h"
#include "battclock_i2c.h"
#include "m41t11.h"

#define RTC_COMPATIBLE  "st,m41t11"

/*
 * A bus that addresses its children with a single cell and gives them no
 * size is a chip select bus - i2c or spi - rather than the mmio bus it
 * hangs off itself. That is enough to tell one apart without having to
 * know every controller that might be fitted.
 */
static BOOL BattClock_IsChipBus(fdt_node_t node)
{
    if (FDT_GetPropU32(node, "#address-cells", 0) != 1)
        return FALSE;
    if (FDT_GetPropU32(node, "#size-cells", 1) != 0)
        return FALSE;

    return FDT_HasProp(node, "compatible");
}

/*
 * The FDT library has no way to ask for a node's parent, so the bus the
 * RTC sits on is found by walking the tree in order and remembering the
 * last such bus seen before reaching it. Nodes come out depth first, so
 * that node is the RTC's parent - which holds as long as one of these
 * buses does not sit inside another, something only a multiplexer does.
 */
static fdt_node_t BattClock_FindBus(fdt_node_t rtc)
{
    fdt_node_t node, bus = FDT_NONE;

    for (node = FDT_Root(); node != FDT_NONE; node = FDT_NextNode(node))
    {
        if (node == rtc)
            return bus;

        if (BattClock_IsChipBus(node))
            bus = node;
    }

    return FDT_NONE;
}

static BOOL BattClock_Discover(struct BattClockBase *BattClockBase)
{
    APTR KernelBase = OpenResource("kernel.resource");
    struct TagItem *bootinfo;
    APTR dtb;
    fdt_node_t rtc, bus;
    UQUAD base = 0, addr = M41T11_ADDR;

    if (!KernelBase)
        return FALSE;

    bootinfo = KrnGetBootInfo();
    dtb = (APTR)GetTagData(KRN_FlattenedDeviceTree, 0, bootinfo);

    if (!dtb || !FDT_Open(dtb))
    {
        D(bug("[BattClock] no device tree\n"));
        return FALSE;
    }

    rtc = FDT_FindCompatible(FDT_NONE, RTC_COMPATIBLE);
    if (rtc == FDT_NONE)
    {
        D(bug("[BattClock] no %s described\n", RTC_COMPATIBLE));
        return FALSE;
    }

    bus = BattClock_FindBus(rtc);
    if (bus == FDT_NONE || !FDT_GetReg(bus, 0, &base, NULL))
    {
        D(bug("[BattClock] %s is on no bus we can reach\n", RTC_COMPATIBLE));
        return FALSE;
    }

    /* On one of these buses "reg" is the slave address, in that one cell */
    if (!FDT_GetReg(rtc, 0, &addr, NULL))
        addr = M41T11_ADDR;

    BattClockBase->bb_BusBase = base;
    BattClockBase->bb_Addr    = (UWORD)addr;


    D(bug("[BattClock] %s at 0x%02x on the controller at %p\n",
          RTC_COMPATIBLE, (UWORD)addr, (APTR)(IPTR)base));

    return BattClock_I2COpen(BattClockBase);
}

static int BattClock_Init(struct BattClockBase *BattClockBase)
{
    InitSemaphore(&BattClockBase->bb_Lock);

    BattClockBase->bb_Bus     = NULL;
    BattClockBase->bb_BusBase = 0;
    BattClockBase->bb_Addr    = 0;
    BattClockBase->bb_Time    = 0;

    /*
     * Failing to find an RTC is not a failure to initialise - the
     * resource still has to answer, it just cannot answer with a time
     * that outlives the session.
     */
    if (!BattClock_Discover(BattClockBase))
    {
        D(bug("[BattClock] no RTC, the clock will not survive a reboot\n"));
    }

    return 1;
}

ADD2INITLIB(BattClock_Init, 0)
