/*
    Copyright (C) 1995-2022, The AROS Development Team. All rights reserved.

    ROMTag scanner.
*/

#include <aros/asmcall.h>
#include <exec/execbase.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/memheaderext.h>
#include <proto/alib.h>
#include <proto/exec.h>

#include <string.h>

#include <kernel_base.h>
#include <kernel_debug.h>
#include <kernel_romtags.h>

#define D(x)

/* The following define prevents the initial
 * scan to count residents */
//#define ROMTAG_FLATALLOC

/*
 * Currently the ROMTag code tries to allocate a 1MB
 * chunk to store the Resident info.
 * TODO: 1MB seems a bit excessive .. look at using a more conservative value
 */
#define ROMTAG_CHUNKSIZE    (1024 * 1024)

static LONG findname(struct Resident **list, ULONG len, CONST_STRPTR name)
{
    ULONG i;
    
    for (i = 0; i < len; i++)
    {
        if (!strcmp(name, list[i]->rt_Name))
            return i;
    }

    return -1;
}

/*
 * Allocate memory space for boot-time usage. Returns address and size of the usable area.
 */
static APTR krnSysMemAlloc(struct MemHeader *mh, IPTR *size)
{
    APTR smAlloc = NULL;

    if (mh->mh_Attributes & MEMF_MANAGED)
    {
        struct MemHeaderExt *mhe = (struct MemHeaderExt *)mh;
        
        if (mhe->mhe_Alloc)
        {
            smAlloc = mhe->mhe_Alloc(mhe, *size, NULL);
        }
    }
    else
    {
        struct MemChunk *mc = mh->mh_First;
        APTR *nxtPtr = (APTR *)&mh->mh_First;

        while (mc->mc_Bytes < *size)
        {
            if (mc->mc_Next)
            {
                nxtPtr = (APTR *)&mc->mc_Next;
                mc = mc->mc_Next;
                continue;
            }
            break;
        }
        if (mc->mc_Bytes >= *size)
        {
            D(bug("[Kernel] %s: Using chunk 0x%p of %lu bytes\n", __func__, mc, mc->mc_Bytes));

            *nxtPtr = mc->mc_Next;
            mh->mh_Free -= mc->mc_Bytes;
            *size = mc->mc_Bytes;
            smAlloc = mc;
        }
    }
    return smAlloc;
}

static APTR krnGetSysMem(struct MemHeader **mh, IPTR *size)
{
    APTR smAlloc;

    if (*size == 0)
        *size = ROMTAG_CHUNKSIZE;

    while ((smAlloc = krnSysMemAlloc(*mh, size)) == NULL)
    {
        if ((*mh)->mh_Node.ln_Pred && (*mh)->mh_Node.ln_Pred->ln_Pred)
        {
            *mh = (struct MemHeader *)((*mh)->mh_Node.ln_Pred);
            D(bug("[Kernel] %s: trying with mh @ 0x%p\n", __func__, *mh);)
        }
        else break;
    }
    if (!smAlloc)
        *size = 0;
    D(
    else
        {
            bug("[Kernel] %s: Allocated %lu bytes from mh @ 0x%p\n", __func__, *size, *mh);
        }
    )
    return smAlloc;
}

/* Release unused boot-time memory */
static void krnReleaseSysMem(struct MemHeader *mh, APTR addr, IPTR chunkSize, IPTR allocSize)
{
    if (mh->mh_Attributes & MEMF_MANAGED)
    {
        struct MemHeaderExt *mhe = (struct MemHeaderExt *)mh;

        if (mhe->mhe_ReAlloc)
            mhe->mhe_ReAlloc(mhe, addr, allocSize);
    }
    else
    {
        struct MemChunk *mc;

        allocSize = AROS_ROUNDUP2(allocSize, MEMCHUNK_TOTAL);
        chunkSize -= allocSize;

        D(bug("[Kernel] %s: Chunk 0x%p, %lu of %lu bytes used\n", __func__, addr, allocSize, chunkSize));

        if (chunkSize < MEMCHUNK_TOTAL)
            return;

        mc = addr + allocSize;

        mc->mc_Next  = mh->mh_First;
        mc->mc_Bytes = chunkSize;

        mh->mh_First = mc;
        mh->mh_Free += mc->mc_Bytes;
    }
}

/*
 * RomTag scanner.
 *
 * This function scans kernel for existing Resident modules. If two modules
 * with the same name are found, the one with higher version or priority
 * wins.
 *
 * After building the list of kernel modules, KickTagPtr and KickMemPtr are
 * checksummed. If the checksum is correct, and all memory pointed to by KickMemPtr
 * may be allocated, then all modules from KickTagPtr are added to RT list
 *
 * Afterwards the proper RomTagList is created (see InitCode() for details)
 * and memory after list and nodes is freed.
 *
 * The array ranges give a [ start, end ] pair combination to scan, with  -1 used
 * as the terminating value.
 */

struct RomScanData
{
    struct Resident **RomTag;
    ULONG           limit;
    ULONG           num;
};

typedef BOOL (*RESSCANCALLBACK)(struct Resident *, struct RomScanData *, void *);

/* Callback to count residents */
static BOOL krnRomResidentCount(struct Resident *res, struct RomScanData *scanData, ULONG *count)
{
    *count += 1;

    return TRUE;
}

/* Callback to add a found resident to the romtag list, or update the entry if it already exists in the list */
static BOOL krnRomResidentRegister(struct Resident *res, struct RomScanData *scanData, UWORD **ptr)
{
    ULONG           i;

    /* Check if there is an existing module with the same name already */
    i = findname(scanData->RomTag, scanData->num, res->rt_Name);
    if (i != -1)
    {
        struct Resident *old = scanData->RomTag[i];
        /*
            Rules for replacing modules:
            1. Higher version always wins.
            2. If the versions are equal, then lower priority
                looses.
        */
        if ((old->rt_Version < res->rt_Version) ||
            (old->rt_Version == res->rt_Version && old->rt_Pri <= res->rt_Pri))
        {
            scanData->RomTag[i] = res;
        }
    }
    else
    {
        /* 'limit' is the max number or pointers we can safely store in the RomTag array. */
        if (scanData->num == scanData->limit)
            return FALSE;

        /* New module */
        scanData->RomTag[scanData->num++] = res;
    }
    return TRUE;
}

/*
 * Scan provided regions looking for residents, and call the provided callback for each found.
 * If a param is provided, pass that to the callback, otherwise pass a pointer to the scan pointer.
 */
static VOID krnScanResidents(UWORD *ranges[], struct RomScanData *scanData, RESSCANCALLBACK scanCallback, void *param)
{
    struct Resident *res;
    UWORD           *end;
    UWORD           *ptr;

    /* Scan the requested ranges for resident modules */
    while (*ranges != (UWORD *)~0)
    {
        ptr = *ranges++;
        end = *ranges++;

        /* Ensure addresses are UWORD-aligned. */
        ptr = (UWORD *)(((IPTR)ptr + 1) & ~1);
        end = (UWORD *)((IPTR)end & ~1);

        D(bug("[Kernel] %s: Scan range start = %p, end = %p\n", __func__, ptr, end));
        do
        {
            res = (struct Resident *)ptr;

            /* Is there a Resident module? */
            if (res->rt_MatchWord == RTC_MATCHWORD && res->rt_MatchTag == res)
            {
                if (!param)
                    scanCallback(res, scanData, &ptr);
                else
                    scanCallback(res, scanData, param);

                /* Get address of EndOfResident from RomTag but only when
                 * it's higher then present one - this avoids strange locks
                 * when not all modules have Resident structure in .text
                 * section */
                ptr = ((IPTR)res->rt_EndSkip > (IPTR)ptr)
                        ?   (UWORD *)res->rt_EndSkip - 2
                        :   ptr;

                if ((IPTR)ptr & 0x01)
                   ptr = (UWORD *)((IPTR)ptr+1);
            }

            /* Get next address... */
            ptr++;
        } while (ptr < (UWORD*)end);
    }
}

APTR krnRomTagScanner(struct MemHeader *mh, UWORD *ranges[])
{
    struct MemHeader    *usedmh = mh;
    struct RomScanData  scanData;
    IPTR                chunkSize = 0;

#if !defined(ROMTAG_FLATALLOC)
    scanData.limit = 0;
    scanData.num = 0;
    /* Perform an initial scan to determine how much space we need to allocate .. */
    krnScanResidents(ranges, &scanData, (RESSCANCALLBACK)krnRomResidentCount, &scanData.limit);

    D(bug("[Kernel] %s: Counted %u Residents\n", __func__, scanData.limit);)
    chunkSize = (scanData.limit + 1) * sizeof(APTR);
#else
    /*
     * We don't know resident list size until it's created. Because of this, we use two-step memory allocation
     * for this purpose.
     * First we dequeue some space from the MemHeader, and remember its starting address and size. Then we
     * construct resident list in this area. After it's done, we return part of the used space to the system.
     */
#endif
    scanData.RomTag = krnGetSysMem(&usedmh, &chunkSize);
    scanData.limit = (chunkSize / sizeof(APTR)) - 1;
    scanData.num = 0;

    D(bug("[Kernel] %s: Array allocated @ %p (%u bytes)\n", __func__, scanData.RomTag, chunkSize);)

    if (scanData.RomTag)
    {
        BOOL sorted;

        /* Populate the list of Residents, and terminate */
        krnScanResidents(ranges, &scanData, (RESSCANCALLBACK)krnRomResidentRegister, NULL);
        scanData.RomTag[scanData.num] = NULL;

        /* Finalise the used memory, releasing unused space if necessary */
        krnReleaseSysMem(usedmh, scanData.RomTag, chunkSize, (scanData.num + 1) * sizeof(APTR));

        /* Resident list is built - BubbleSort RomTags according to their priority */
        do
        {
            ULONG           i;
            sorted = TRUE;

            for (i = 0; i < scanData.num - 1; i++)
            {
                if (scanData.RomTag[i]->rt_Pri < scanData.RomTag[i+1]->rt_Pri)
                {
                    struct Resident *tmp;

                    tmp = scanData.RomTag[i+1];
                    scanData.RomTag[i+1] = scanData.RomTag[i];
                    scanData.RomTag[i] = tmp;

                    sorted = FALSE;
                }
            }
        } while (!sorted);
    }
    return scanData.RomTag;
}

struct Resident *krnFindResident(struct Resident **resList, const char *name)
{
    ULONG i;

    for (i = 0; resList[i]; i++)
    {
        if (!strcmp(resList[i]->rt_Name, name))
            return resList[i];
    }
    return NULL;
}
