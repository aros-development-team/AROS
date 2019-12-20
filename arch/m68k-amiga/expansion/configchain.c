/*
    Copyright © 1995-2019, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/expansion.h>
#include <proto/kernel.h>

#include "expansion_intern.h"

#include <clib/expansion_protos.h>
#include <proto/exec.h>
#include <exec/resident.h>

#define ROMIMG_SIZE             0x80000

struct mbChunkNode
{
    struct MinNode              bmc_Node;
    const char                  *bmc_TypeStr;
    ULONG                       bmc_TypeFlags;
    APTR                        *bmc_Start;
    ULONG                       bmc_Size;
    BYTE                        bmc_Prio;
};

AROS_UFP3(ULONG, MemoryTest,
    AROS_UFPA(APTR, startaddr, A0),
    AROS_UFPA(APTR, endaddr, A1),
    AROS_UFPA(ULONG, block, D0));

const char strMemTypeExp[]      = "expansion.memory";
const char strMemTypeFast[]     = "Fast Memory";

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

static LONG scanmbregion(struct ExpansionBase *ExpansionBase, struct MinList *mbchunks, APTR *start, APTR *end, LONG step, BYTE prio)
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
        ret = scanmbregion(ExpansionBase, mbchunks, start, &tmpend, step, prio);
        if (ret >= 0)
            ret = scanmbregion(ExpansionBase, mbchunks, &tmpstart, end, step, prio);
        return ret;
    }
    else if (romatregionstart(romend, *start, *end, (step < 0) ? TRUE : FALSE))
    {
        if (step < 0)
            tmpstart = romstart;
        else
            tmpstart = romend + 1;
        
        return scanmbregion(ExpansionBase, mbchunks, &tmpstart, end, step, prio);
    }
    else if (romatregionend(romstart, *start, *end, (step < 0) ? TRUE : FALSE))
    {
        if (step < 0)
            tmpend = romend + 1;
        else
            tmpend = romstart;

        return scanmbregion(ExpansionBase, mbchunks, start, &tmpend, step, prio);
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
        ret = scanmbregion(ExpansionBase, mbchunks, start, &tmpend, step, prio);
        if (ret >= 0)
            ret = scanmbregion(ExpansionBase, mbchunks, &tmpstart, end, step, prio);
        return ret;
    }
    else if (romatregionstart(romend, *start, *end, (step < 0) ? TRUE : FALSE))
    {
        if (step < 0)
            tmpstart = romstart;
        else
            tmpstart = romend + 1;

        return scanmbregion(ExpansionBase, mbchunks, &tmpstart, end, step, prio);
    }
    else if (romatregionend(romstart, *start, *end, (step < 0) ? TRUE : FALSE))
    {
        if (step < 0)
            tmpend = romend + 1;
        else
            tmpend = romstart;

        return scanmbregion(ExpansionBase, mbchunks, start, &tmpend, step, prio);
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
                    break;
                rangestart = mbramstart;
            }
            else
            {
                tstep = step;
                if (mbramstart > *end)
                    break;
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
            if (step > 0)
                mbramstart += step;

            D(bug("[expansion:am68k] %s:     ret = %d\n", __func__, ret));
        }

        D(bug("[expansion:am68k] %s: ret = %d\n", __func__, ret));

        if (ret == 0)
            break;

        if (ret > 0)
        {
            struct mbChunkNode *memChunk;

            /* allocate the node in chip.. */
            if ((memChunk = (struct mbChunkNode *)AllocMem(sizeof(struct mbChunkNode), MEMF_CHIP)) != NULL)
            {
                struct mbChunkNode *nxtChunk;
                D(bug("[expansion:am68k] %s: mboard chunk 0x%p:0x%p (size=%08x) suitable for use\n", __func__, rangestart, rangestart + ret, ret));

                memChunk->bmc_TypeStr = strMemTypeExp;
                memChunk->bmc_TypeFlags = MEMF_KICK | MEMF_LOCAL | MEMF_FAST | MEMF_PUBLIC;
                memChunk->bmc_Start = rangestart;
                memChunk->bmc_Size = ret;
                memChunk->bmc_Prio = prio;
                ForeachNode(mbchunks, nxtChunk)
                {
                    if (memChunk->bmc_Start < nxtChunk->bmc_Start)
                        break;
                }
                if (nxtChunk)
                {
                    memChunk->bmc_Node.mln_Pred	                = nxtChunk->bmc_Node.mln_Pred;
                    memChunk->bmc_Node.mln_Succ	                = &nxtChunk->bmc_Node;
                    nxtChunk->bmc_Node.mln_Pred->mln_Succ       = &memChunk->bmc_Node;
                    nxtChunk->bmc_Node.mln_Pred	                = &memChunk->bmc_Node;
                }
                else
                    AddTail(mbchunks, &memChunk->bmc_Node);
                break;
            }
        }
        if (step >= 0)
            mbramstart += step;
    }
    return ret;
}

static void addmergedchunks(struct MinList *chunkList)
{
    struct mbChunkNode *currChunk, *tmpChunk;

    D(bug("[expansion:am68k] %s()\n", __func__));

    /* Merge detected chunks that can be merged ... */
    ForeachNodeSafe(chunkList, currChunk, tmpChunk)
    {
        if (currChunk->bmc_Node.mln_Succ && currChunk->bmc_Node.mln_Succ->mln_Succ)
        {
            if ((((struct mbChunkNode *)currChunk->bmc_Node.mln_Succ)->bmc_Start == (currChunk->bmc_Start + currChunk->bmc_Size)) &&
                (((struct mbChunkNode *)currChunk->bmc_Node.mln_Succ)->bmc_Prio == currChunk->bmc_Prio) &&
                (((struct mbChunkNode *)currChunk->bmc_Node.mln_Succ)->bmc_TypeFlags == currChunk->bmc_TypeFlags))
            {
                D(bug("[expansion:am68k] %s: merging 0x%p:0x%p with 0x%p:0x%p\n", __func__, currChunk->bmc_Start,
                                                                                   (IPTR)currChunk->bmc_Start + currChunk->bmc_Size,
                                                                                   ((struct mbChunkNode *)currChunk->bmc_Node.mln_Succ)->bmc_Start,
                                                                                   (IPTR)((struct mbChunkNode *)currChunk->bmc_Node.mln_Succ)->bmc_Start + ((struct mbChunkNode *)currChunk->bmc_Node.mln_Succ)->bmc_Size));
                ((struct mbChunkNode *)currChunk->bmc_Node.mln_Succ)->bmc_Start = currChunk->bmc_Start;
                ((struct mbChunkNode *)currChunk->bmc_Node.mln_Succ)->bmc_Size = currChunk->bmc_Size;
                Remove(&currChunk->bmc_Node);
                FreeMem(currChunk, sizeof(struct mbChunkNode));
            }
        }
    }

    /* Now register what has been found in the system ... */
    ForeachNodeSafe(chunkList, currChunk, tmpChunk)
    {
        Remove(&currChunk->bmc_Node);
        D(bug("[expansion:am68k] %s: adding 0x%p:0x%p (size=%08x) to the expansion memlist\n", __func__, currChunk->bmc_Start, ((IPTR)currChunk->bmc_Start + currChunk->bmc_Size), currChunk->bmc_Size));
        AddMemList(currChunk->bmc_Size, currChunk->bmc_TypeFlags, currChunk->bmc_Prio, currChunk->bmc_Start, currChunk->bmc_TypeStr);
        FreeMem(currChunk, sizeof(struct mbChunkNode));
    }
}

static void findmbram(struct ExpansionBase *ExpansionBase)
{
    D(bug("[expansion:am68k] %s()\n", __func__));

    if (!(SysBase->AttnFlags & AFF_68020))
        return;
    if ((SysBase->AttnFlags & AFF_68020) && !(SysBase->AttnFlags & AFF_ADDR32))
        return;

    LONG ret;
    LONG step;
    APTR start, end;
    struct MinList mbchunks;

    NEWLIST(&mbchunks);

    /* Scan High MBRAM */
    step =  (LONG)0x00100000;
    start = (APTR)0x08000000; // 128MB mark
    end =   (APTR)0x7f000000; // 2GB mark
    ret = scanmbregion(ExpansionBase, &mbchunks, &start, &end, step, 40);

    /* Scan Low MBRAM, reversed detection needed */
    step =  -step;
    start = (APTR)0x08000000; // 128MB mark
    end =   (APTR)0x01000000; // 16MB mark
    ret = scanmbregion(ExpansionBase, &mbchunks, &start, &end, step, 30);

    addmergedchunks(&mbchunks);
}

static void detectexpram(struct ExpansionBase *ExpansionBase)
{
    findmbram(ExpansionBase);

    D(bug("[expansion:am68k] %s: adding ram boards\n", __func__));

    struct Node *node;
    struct List boardChunks;
    NEWLIST(&boardChunks);

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
                struct mbChunkNode *memChunk;
                /* allocate the node in chip.. */
                if ((memChunk = (struct mbChunkNode *)AllocMem(sizeof(struct mbChunkNode), MEMF_CHIP)) != NULL)
                {
                    struct mbChunkNode *nxtChunk;
                    D(bug("[expansion:am68k] %s: ram board chunk 0x%p:0x%p (size=%08x) suitable for use\n", __func__, rangestart, rangestart + ret, ret));

                    memChunk->bmc_TypeStr = strMemTypeFast;
                    memChunk->bmc_TypeFlags = attr;
                    memChunk->bmc_Start = addr;
                    memChunk->bmc_Size = size;
                    memChunk->bmc_Prio = pri;
                    ForeachNode(&boardChunks, nxtChunk)
                    {
                        if (memChunk->bmc_Start < nxtChunk->bmc_Start)
                            break;
                    }
                    if (nxtChunk)
                    {
                        memChunk->bmc_Node.mln_Pred             = nxtChunk->bmc_Node.mln_Pred;
                        memChunk->bmc_Node.mln_Succ             = &nxtChunk->bmc_Node;
                        nxtChunk->bmc_Node.mln_Pred->mln_Succ   = &memChunk->bmc_Node;
                        nxtChunk->bmc_Node.mln_Pred             = &memChunk->bmc_Node;
                    }
                    else
                        AddTail(&boardChunks, &memChunk->bmc_Node);
                    break;
                }
            }
            configDev->cd_Flags |= CDF_PROCESSED;
        }
    }

    addmergedchunks(&boardChunks);

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

    detectexpram(ExpansionBase);

    AROS_LIBFUNC_EXIT
} /* ConfigChain */
