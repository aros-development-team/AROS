/*
    Copyright © 1995-2019, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#define DEBUG 1
#include <aros/debug.h>

#include <proto/expansion.h>
#include <proto/kernel.h>

#include "expansion_intern.h"

#include <clib/expansion_protos.h>
#include <proto/exec.h>
#include <exec/resident.h>

#define ROMIMG_SIZE                     0x80000

AROS_UFP3(ULONG, MemoryTest,
    AROS_UFPA(APTR, startaddr, A0),
    AROS_UFPA(APTR, endaddr, A1),
    AROS_UFPA(ULONG, block, D0));

static ULONG autosize(struct ExpansionBase *ExpansionBase, struct ConfigDev *configDev)
{
    UBYTE sizebits = configDev->cd_Rom.er_Flags & ERT_Z3_SSMASK;
    ULONG maxsize = configDev->cd_BoardSize;
    ULONG size = 0;
    UBYTE *addr = (UBYTE*)configDev->cd_BoardAddr;

    D(bug("[expansion:am68k] %s: sizebits = %x\n", __func__, sizebits));
    if (sizebits >= 14) /* 14 and 15 = reserved */
        return 0;
    if (sizebits >= 2 && sizebits <= 8)
        return 0x00010000 << (sizebits - 2);
    if (sizebits >= 9)
        return 0x00600000 + (0x200000 * (sizebits - 9));
    size = AROS_UFC3(ULONG, MemoryTest,
            AROS_UFCA(APTR, addr, A0),
            AROS_UFCA(APTR, addr + maxsize, A1),
            AROS_UFCA(ULONG, 0x80000, D0));
    D(bug("[expansion:am68k] %s: addr = %lx size = %lx, maxsize = %lx\n", __func__, addr, size, maxsize));
    return size;
}

static APTR getromaddr(struct ExpansionBase *ExpansionBase, APTR romaddr)
{
    if (IntExpBase(ExpansionBase)->kernelBase)
        return KrnVirtualToPhysical(romaddr);
    return romaddr;
}

static BOOL romisinregion(APTR romstart, APTR romend, APTR regstart, APTR regend, BOOL rev)
{
    if (rev)
    {
        if (((romend < regstart) && (romend > regend)) && 
            ((romstart > regend ) && (romstart < regstart)))
            return TRUE;
    }
    else
    {
        if (((romend > regstart) && (romend < regend)) && 
            ((romstart < regend ) && (romstart > regstart)))
            return TRUE;
    }
    return FALSE;
}

static BOOL romatregionstart(APTR romend, APTR regstart, APTR regend, BOOL rev)
{
    if (rev)
    {
        if ((romend < regstart) && (romend > regend))
            return TRUE;
    }
    else
    {
        if ((romend > regstart) && (romend < regend))
            return TRUE;
    }
    return FALSE;
}

static BOOL romatregionend(APTR romstart, APTR regstart, APTR regend, BOOL rev)
{
    if (rev)
    {
        if ((romstart > regend) && (romstart < regstart))
            return TRUE;
    }
    else
    {
        if ((romstart < regend) && (romstart > regstart))
            return TRUE;
    }
    return FALSE;
}

static LONG scanmbram(struct ExpansionBase *ExpansionBase, APTR *start, APTR *end, LONG step, BYTE prio)
{
    APTR romstart, romend, tmpstart, tmpend;
    LONG ret;

    D(
      bug("[expansion:am68k] %s(", __func__);
      if (step < 0)
      {    
          bug("%08x:%08x)\n", *start - 1, *end);
      }
      else
      {
          bug("%08x:%08x)\n", *start, *end - 1);
      }
     )

    /* split the region around the system rom *******************************************/
    romstart = getromaddr(ExpansionBase, (APTR)0xF80000);
    romend = romstart + ROMIMG_SIZE - 1;
    D(bug("[expansion:am68k] %s: rom @ %08x-> %08x\n", __func__, romstart, romend));
    if (romisinregion(romstart, romend, *start, *end, (step < 0) ? TRUE : FALSE))
    {
        if (step < 0)
        {
            tmpstart = romstart;
            tmpend = romend + 1;
        }
        else
        {
            tmpend = romstart;
            tmpstart = romend + 1;
        }
        ret = scanmbram(ExpansionBase, start, &tmpend, step, prio);
        if (ret >= 0)
            ret = scanmbram(ExpansionBase, &tmpstart, end, step, prio);
        return ret;
    }
    else if (romatregionstart(romend, *start, *end, (step < 0) ? TRUE : FALSE))
    {
        if (step < 0)
            tmpstart = romstart;
        else
            tmpstart = romend + 1;
        
        return scanmbram(ExpansionBase, &tmpstart, end, step, prio);
    }
    else if (romatregionend(romstart, *start, *end, (step < 0) ? TRUE : FALSE))
    {
        if (step < 0)
            tmpend = romend + 1;
        else
            tmpend = romstart;

        return scanmbram(ExpansionBase, start, &tmpend, step, prio);
    }
    /* end splitting the region around the system rom ***********************************/

    /* split the region around the extended rom (cd32/aros/etc) *************************/
    romstart = getromaddr(ExpansionBase, (APTR)0xE00000);
    romend = romstart + ROMIMG_SIZE - 1;
    D(bug("[expansion:am68k] %s: ext-rom @ %08x-> %08x\n", __func__, romstart, romend));
    if (romisinregion(romstart, romend, *start, *end, (step < 0) ? TRUE : FALSE))
    {
        if (step < 0)
        {
            tmpstart = romstart;
            tmpend = romend + 1;
        }
        else
        {
            tmpend = romstart;
            tmpstart = romend + 1;
        }
        ret = scanmbram(ExpansionBase, start, &tmpend, step, prio);
        if (ret >= 0)
            ret = scanmbram(ExpansionBase, &tmpstart, end, step, prio);
        return ret;
    }
    else if (romatregionstart(romend, *start, *end, (step < 0) ? TRUE : FALSE))
    {
        if (step < 0)
            tmpstart = romstart;
        else
            tmpstart = romend + 1;

        return scanmbram(ExpansionBase, &tmpstart, end, step, prio);
    }
    else if (romatregionend(romstart, *start, *end, (step < 0) ? TRUE : FALSE))
    {
        if (step < 0)
            tmpend = romend + 1;
        else
            tmpend = romstart;

        return scanmbram(ExpansionBase, start, &tmpend, step, prio);
    }
    /* end splitting the region around the extended rom *********************************/

    APTR mbramstart = *start, mbramend;

    D(
      bug("[expansion:am68k] %s: performing test on region ", __func__);
      if (step < 0)
      {
          bug("%08x:%08x (-)\n", *end, *start - 1);
      }
      else
      {
          bug("%08x:%08x\n", *start, *end - 1);
      }    
     )

    for (;;)
    {
        APTR rangestart = mbramstart;
        ret = 0;
        for (;;)
        {   
            LONG tstep;
            
            if (step < 0)
            {
                tstep = -step;
                mbramend = mbramstart;
                if ((mbramstart += step) < *end)
                {
                    ret = 0;
                    break;
                }
                rangestart = mbramstart;
            }
            else
            {
                tstep = step;
                if ((mbramstart + step) > *end)
                {
                    ret = 0;
                    break;
                }
                if (step < (*end - mbramstart))
                    mbramend = mbramstart + step;
                else
                    mbramend = *end;
            }

            D(bug("[expansion:am68k] %s:     testing range %08x -> %08x\n", __func__, mbramstart, mbramend - 1));
            LONG tret = AROS_UFC3(LONG, MemoryTest,
                        AROS_UFCA(APTR, mbramstart, A0),
                        AROS_UFCA(APTR, mbramend, A1),
                        AROS_UFCA(ULONG, tstep, D0));
            if (tret < 0)
            {
                D(bug("[expansion:am68k] %s:     returning tret=%d\n", __func__, tret));
                return tret;
            }
            if (tret == 0)
            {
                D(bug("[expansion:am68k] %s:     tret == 0\n", __func__));
                if ((step < 0) && (ret > 0))
                    rangestart -= step;
                break;
            }
            ret += tret;
            D(bug("[expansion:am68k] %s:     ret = %d\n", __func__, ret));
        }

        D(bug("[expansion:am68k] %s: ret = %d\n", __func__, ret));

        if (ret == 0)
            break;

        if (ret > 0)
        {
            D(bug("[expansion:am68k] %s: Adding %08x;%08x (size=%08x) to the expansion memlist\n", __func__, rangestart, rangestart + ret, ret));
            AddMemList(ret, MEMF_KICK | MEMF_LOCAL | MEMF_FAST | MEMF_PUBLIC, prio, rangestart, "expansion.memory");
            if (((step >= 0) && ((rangestart + ret) == *end)) ||
                ((step < 0 ) && (rangestart == *end)))
                break;
        }
        if (step >= 0)
            mbramstart += step;
    }
    return ret;
}

static void findmbram(struct ExpansionBase *ExpansionBase)
{
    LONG ret;
    LONG step;
    APTR start, end;

    if (!(SysBase->AttnFlags & AFF_68020))
        return;
    if ((SysBase->AttnFlags & AFF_68020) && !(SysBase->AttnFlags & AFF_ADDR32))
        return;

    /* High MBRAM */
    step =  (LONG)0x00100000;
    start = (APTR)0x08000000;
    end =   (APTR)0x7f000000;
    ret = scanmbram(ExpansionBase, &start, &end, step, 40);

    /* Low MBRAM, reversed detection needed */
    step =  -step;
    start = (APTR)0x08000000;
    end =   (APTR)0x01000000;
    ret = scanmbram(ExpansionBase, &start, &end, step, 30);
}

static void allocram(struct ExpansionBase *ExpansionBase)
{
    struct Node *node;
    
    findmbram(ExpansionBase);

    // we should merge address spaces, later..
    D(bug("[expansion:am68k] %s: adding ram boards\n", __func__));

    ForeachNode(&ExpansionBase->BoardList, node) {
        struct ConfigDev *configDev = (struct ConfigDev*)node;
        if ((configDev->cd_Rom.er_Type & ERTF_MEMLIST) && !(configDev->cd_Flags & CDF_SHUTUP) && !(configDev->cd_Flags & CDF_PROCESSED)) {
            ULONG attr = MEMF_PUBLIC | MEMF_FAST | MEMF_KICK;
            ULONG size = configDev->cd_BoardSize;
            APTR addr = configDev->cd_BoardAddr;
            LONG pri = 20;
            if (configDev->cd_BoardAddr <= (APTR)0x00FFFFFF) {
                attr |= MEMF_24BITDMA;
                pri = 0;
            } else if ((configDev->cd_Rom.er_Flags & ERT_Z3_SSMASK) != 0) {
                size = autosize(ExpansionBase, configDev);
            }
            if (size && size <= configDev->cd_BoardSize) {
                D(bug("[expansion:am68k] %s: ram board at %08x, size %08x attr %08x\n", __func__, addr, size, attr));
                AddMemList(size, attr, pri, addr, "Fast Memory");
            }
            configDev->cd_Flags |= CDF_PROCESSED;
        }
    }
    D(bug("[expansion:am68k] %s: ram boards done\n", __func__));
}
        

AROS_LH1(void, ConfigChain,
        AROS_LHA(APTR, baseAddr, A0),
        struct ExpansionBase *, ExpansionBase, 11, Expansion)
{
    AROS_LIBFUNC_INIT

    struct ConfigDev *configDev = NULL;

    /* Try to guess if we have Z3 based machine.
     * Not all Z3 boards appear in Z2 region.
     *
     * Ignores original baseAddr by design.
     */
    BOOL maybeZ3 = (SysBase->AttnFlags & AFF_ADDR32);
    D(bug("[expansion:am68k] %s()\n", __func__));
    for(;;) {
        BOOL gotrom = FALSE;
        if (!configDev)
            configDev = AllocConfigDev();
        if (!configDev)
            break;
        if (maybeZ3) {
            baseAddr = (APTR)EZ3_EXPANSIONBASE;
            gotrom = ReadExpansionRom(baseAddr, configDev);
        }
        if (!gotrom) {
            baseAddr = (APTR)E_EXPANSIONBASE;
            gotrom = ReadExpansionRom(baseAddr, configDev);
        }
        if (!gotrom) {
            FreeConfigDev(configDev);
            break;
        }
        if (ConfigBoard(baseAddr, configDev)) {
            AddConfigDev(configDev);
            configDev = NULL;
        }
    }
    D(bug("[expansion:am68k] %s: configchain done\n", __func__));

    allocram(ExpansionBase);

    AROS_LIBFUNC_EXIT
} /* ConfigChain */
