/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: Applying a device tree node's pin multiplexing.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/kernel.h>
#include <aros/kernel.h>

#include <string.h>

#include <fdt.h>

#include "dp1000_pinctrl.h"

#define PINCTRL_COMPATIBLE  "ultrarisc,dp1000-pinctrl"

/*
 * The function select: one 32 bit word per port, the low half a bitmask
 * of the pins switched to FUNC0 and the high half the same for FUNC1,
 * bit n being pin n. Ports are A, B, C, D, LPC in that order.
 *
 * The firmware's tree uses the generic pinctrl binding - a group node
 * carries "pins", a list of pad names, and one "function" for all of
 * them - not the "pinctrl-pins = <port pin func>" triples the vendor's
 * Linux sources describe the same hardware with.
 */
#define PMX_FUNC_BASE       0x2c0
#define PMX_FUNC_REG(port)  (PMX_FUNC_BASE + ((port) * 4))
#define PMX_MAX_PORT        4
#define PMX_MAX_PIN         15

#define UR_FUNC_DEF         0
#define UR_FUNC0            1
#define UR_FUNC1            0x10000

/* "bias-pull-up" and "drive-strength" live in an array whose layout is
   not known, so they are left as the firmware had them */

static volatile UBYTE *pmxBase = NULL;
static BOOL pmxLookedFor = FALSE;

static volatile UBYTE *DP1000_PinctrlBase(APTR KernelBase)
{
    fdt_node_t node;
    UQUAD addr = 0, size = 0;

    if (pmxLookedFor)
        return pmxBase;

    pmxLookedFor = TRUE;

    node = FDT_FindCompatible(FDT_NONE, PINCTRL_COMPATIBLE);
    if (node == FDT_NONE)
    {
        D(bug("[DP1000:pinctrl] no %s described\n", PINCTRL_COMPATIBLE);)
        return NULL;
    }

    if (!FDT_GetReg(node, 0, &addr, &size) || !addr || !size)
    {
        D(bug("[DP1000:pinctrl] %s has no registers\n", PINCTRL_COMPATIBLE);)
        return NULL;
    }

    /* The boot page tables cover RAM and nothing else */
    if (!KrnMapGlobal((APTR)(IPTR)addr, (APTR)(IPTR)addr, (ULONG)size,
                      MAP_Readable | MAP_Writable))
    {
        D(bug("[DP1000:pinctrl] could not map %p+%p\n", (APTR)(IPTR)addr,
              (APTR)(IPTR)size);)
        return NULL;
    }

    pmxBase = (volatile UBYTE *)(IPTR)addr;

    D(bug("[DP1000:pinctrl] controller at %p\n", pmxBase);)

    return pmxBase;
}

/* "PA12", "PC0" - a port letter and a pin number, and nothing else */
static BOOL DP1000_PinParse(const char *s, ULONG *port, ULONG *pin)
{
    ULONG v = 0, digits = 0;

    if (!s || (s[0] != 'P' && s[0] != 'p'))
        return FALSE;

    if (s[1] >= 'A' && s[1] <= 'E')
        *port = (ULONG)(s[1] - 'A');
    else if (s[1] >= 'a' && s[1] <= 'e')
        *port = (ULONG)(s[1] - 'a');
    else
        return FALSE;

    for (s += 2; *s >= '0' && *s <= '9'; s++, digits++)
        v = (v * 10) + (ULONG)(*s - '0');

    if (!digits || *s)
        return FALSE;

    *pin = v;

    return TRUE;
}

/* "func0", "func1", or anything else - gpio, default - meaning neither */
static ULONG DP1000_FuncParse(CONST_STRPTR name)
{
    if (name && name[0] == 'f' && name[1] == 'u' && name[2] == 'n' &&
        name[3] == 'c' && name[5] == '\0')
    {
        if (name[4] == '0')
            return UR_FUNC0;
        if (name[4] == '1')
            return UR_FUNC1;
    }

    return UR_FUNC_DEF;
}

static void DP1000_PinctrlSet(volatile UBYTE *base, ULONG port, ULONG pin,
                              ULONG func)
{
    volatile ULONG *reg;
    ULONG was, now;

    if (port > PMX_MAX_PORT || pin > PMX_MAX_PIN)
    {
        D(bug("[DP1000:pinctrl] port %u pin %u is not a pad\n", port, pin);)
        return;
    }

    reg = (volatile ULONG *)(base + PMX_FUNC_REG(port));

    was = *reg;

    /* Out with whichever function the pad had, in with the one asked for */
    now = was & ~((UR_FUNC0 << pin) | (UR_FUNC1 << pin));
    if (func != UR_FUNC_DEF)
        now |= (func << pin);

    if (now != was)
    {
        *reg = now;

        D(bug("[DP1000:pinctrl] P%c%u func %x: %08x -> %08x\n",
              'A' + port, pin, func, was, now);)
    }
}

BOOL DP1000_PinctrlApply(APTR KernelBase, fdt_node_t node)
{
    volatile UBYTE *base;
    const ULONG *phandles;
    ULONG len = 0, i, applied = 0;

    phandles = FDT_GetProp(node, "pinctrl-0", &len);
    if (!phandles || len < 4)
        return FALSE;

    base = DP1000_PinctrlBase(KernelBase);
    if (!base)
        return FALSE;

    /* "pinctrl-0" is a list of phandles, each naming a group of pads */
    for (i = 0; i < len / 4; i++)
    {
        ULONG phandle = (ULONG)FDT_ReadCells(&phandles[i], 1);
        fdt_node_t group = FDT_NONE, n;
        const char *pins;
        ULONG plen = 0, o, func;

        if (!phandle || phandle == 0xffffffff)
            continue;

        /* The parser keeps no index of phandles, so this is a walk */
        for (n = FDT_Root(); n != FDT_NONE; n = FDT_NextNode(n))
        {
            if (FDT_GetPropU32(n, "phandle", 0) == phandle ||
                FDT_GetPropU32(n, "linux,phandle", 0) == phandle)
            {
                group = n;
                break;
            }
        }

        if (group == FDT_NONE)
        {
            D(bug("[DP1000:pinctrl] phandle %x names no node\n", phandle);)
            continue;
        }

        pins = FDT_GetProp(group, "pins", &plen);
        if (!pins || !plen)
        {
            D(bug("[DP1000:pinctrl] %s has no \"pins\"\n",
                  FDT_NodeName(group));)
            continue;
        }

        func = DP1000_FuncParse(FDT_GetPropStr(group, "function"));

        /* One pad per string, all taking the same function */
        for (o = 0; o < plen && pins[o]; o += strlen(&pins[o]) + 1)
        {
            ULONG port = 0, pin = 0;

            if (!DP1000_PinParse(&pins[o], &port, &pin))
            {
                D(bug("[DP1000:pinctrl] cannot read pad name \"%s\"\n",
                      &pins[o]);)
                continue;
            }

            DP1000_PinctrlSet(base, port, pin, func);
            applied++;
        }
    }

    D(bug("[DP1000:pinctrl] %s: %u pad(s)\n", FDT_NodeName(node), applied);)

    return applied != 0;
}
