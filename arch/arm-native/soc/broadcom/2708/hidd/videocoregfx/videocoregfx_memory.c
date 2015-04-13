/*
    Copyright © 2013-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#define __OOP_NOATTRBASES__

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/mbox.h>
#include <proto/kernel.h>
#include <proto/utility.h>

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/memory.h> 
#include <hidd/graphics.h>
#include <hidd/pci.h>
#include <oop/oop.h>
#include <utility/utility.h>
#include <aros/symbolsets.h>

#include "videocoregfx_class.h"
#include "videocoregfx_hardware.h"

#include LC_LIBDEFS_FILE

#ifdef MBoxBase
#undef MBoxBase
#endif

#define MBoxBase      xsd->vcsd_MBoxBase

void *mh_Alloc(struct MemHeaderExt *mhe, IPTR size, ULONG *flags)
{
    D(bug("[VideoCoreGfx] %s(%d bytes, 0x%08x)\n", __PRETTY_FUNCTION__, size, flags));

    return NULL;
}

/*
    returns a handle on a block of gpu memory. you cannot directly access this
    memory unless it is locked, since it may be moved around by the gpu.
*/
void *FNAME_SUPPORT(Alloc)(struct MemHeaderExt *mhe, IPTR size, ULONG *flags)
{
    struct VideoCoreGfx_staticdata *xsd = (APTR)mhe->mhe_UserData;
    ULONG gpumemflags = VCMEM_NORMAL, gpumemalign = 0;

    D(bug("[VideoCoreGfx] %s(%d bytes, 0x%08x)\n", __PRETTY_FUNCTION__, size, flags));

    if (!(*flags & MEMF_PUBLIC))
        gpumemflags = VCMEM_L1NONALLOCATING | VCMEM_NOINIT | VCMEM_LAZYLOCK;
    if (*flags & MEMF_CLEAR)
        gpumemflags |= VCMEM_ZERO;
    if (*flags & MEMF_HWALIGNED)
        gpumemalign = 4096;

    xsd->vcsd_MBoxMessage[0] = 9 * 4;
    xsd->vcsd_MBoxMessage[1] = VCTAG_REQ;
    xsd->vcsd_MBoxMessage[2] = VCTAG_ALLOCMEM;
    xsd->vcsd_MBoxMessage[3] = 4 * 3;
    xsd->vcsd_MBoxMessage[4] = size;
    xsd->vcsd_MBoxMessage[5] = gpumemalign;
    xsd->vcsd_MBoxMessage[6] = gpumemflags;

    xsd->vcsd_MBoxMessage[7] = 0;

    xsd->vcsd_MBoxMessage[8] = 0; // terminate tag

    MBoxWrite(VCMB_BASE, VCMB_PROPCHAN, xsd->vcsd_MBoxMessage);
    if (MBoxRead(VCMB_BASE, VCMB_PROPCHAN) == xsd->vcsd_MBoxMessage)
    {
        D(bug("[VideoCoreGfx] %s: Allocated %d bytes, memhandle @ 0x%p\n", __PRETTY_FUNCTION__, xsd->vcsd_MBoxMessage[4], xsd->vcsd_MBoxMessage[7]));
        
        mhe->mhe_MemHeader.mh_Free -= size;
        
        return xsd->vcsd_MBoxMessage[7];
    }
    return NULL;
}

/*
    Lock the gpu memory represented by the memhandle,
    and return a physical pointer to it..
*/
void *FNAME_SUPPORT(LockMem)(struct MemHeaderExt *mhe, void *memhandle)
{
    struct VideoCoreGfx_staticdata *xsd = (APTR)mhe->mhe_UserData;

    D(bug("[VideoCoreGfx] %s(memhandle @ 0x%p)\n", __PRETTY_FUNCTION__, memhandle));
    
    xsd->vcsd_MBoxMessage[0] = 7 * 4;
    xsd->vcsd_MBoxMessage[1] = VCTAG_REQ;
    xsd->vcsd_MBoxMessage[2] = VCTAG_LOCKMEM;
    xsd->vcsd_MBoxMessage[3] = 4;
    xsd->vcsd_MBoxMessage[4] = memhandle;

    xsd->vcsd_MBoxMessage[5] = 0;

    xsd->vcsd_MBoxMessage[6] = 0; // terminate tag

    MBoxWrite(VCMB_BASE, VCMB_PROPCHAN, xsd->vcsd_MBoxMessage);
    if (MBoxRead(VCMB_BASE, VCMB_PROPCHAN) == xsd->vcsd_MBoxMessage)
    {
        D(bug("[VideoCoreGfx] %s: Memory locked @ 0x%p\n", __PRETTY_FUNCTION__, xsd->vcsd_MBoxMessage[5]));
        return xsd->vcsd_MBoxMessage[5];
    }
    return NULL;
}

/*
    Unlock the gpu memory represented by the memhandle.
*/
void *FNAME_SUPPORT(UnLockMem)(struct MemHeaderExt *mhe, void *memhandle)
{
    struct VideoCoreGfx_staticdata *xsd = (APTR)mhe->mhe_UserData;

    D(bug("[VideoCoreGfx] %s(memhandle @ 0x%p)\n", __PRETTY_FUNCTION__, memhandle));
    
    xsd->vcsd_MBoxMessage[0] = 7 * 4;
    xsd->vcsd_MBoxMessage[1] = VCTAG_REQ;
    xsd->vcsd_MBoxMessage[2] = VCTAG_UNLOCKMEM;
    xsd->vcsd_MBoxMessage[3] = 4;
    xsd->vcsd_MBoxMessage[4] = memhandle;

    xsd->vcsd_MBoxMessage[5] = 0;

    xsd->vcsd_MBoxMessage[6] = 0; // terminate tag

    MBoxWrite(VCMB_BASE, VCMB_PROPCHAN, xsd->vcsd_MBoxMessage);
    if (MBoxRead(VCMB_BASE, VCMB_PROPCHAN) == xsd->vcsd_MBoxMessage)
    {
        D(bug("[VideoCoreGfx] %s: Memory unlocked [status %08x]\n", __PRETTY_FUNCTION__, xsd->vcsd_MBoxMessage[5]));
        return xsd->vcsd_MBoxMessage[5];
    }
    return NULL;
}

void mh_Free(struct MemHeaderExt *mhe, APTR  mem,  IPTR size)
{
    D(bug("[VideoCoreGfx] %s(0x%p)\n", __PRETTY_FUNCTION__, mem));
}

/*
    Frees the gpu memory associated with the handle.
*/
void FNAME_SUPPORT(Free)(struct MemHeaderExt *mhe, APTR  memhandle, IPTR size)
{
    struct VideoCoreGfx_staticdata *xsd = (APTR)mhe->mhe_UserData;

    D(bug("[VideoCoreGfx] %s(memhandle @ 0x%p)\n", __PRETTY_FUNCTION__, memhandle));

    xsd->vcsd_MBoxMessage[0] = 7 * 4;
    xsd->vcsd_MBoxMessage[1] = VCTAG_REQ;
    xsd->vcsd_MBoxMessage[2] = VCTAG_FREEMEM;
    xsd->vcsd_MBoxMessage[3] = 4;
    xsd->vcsd_MBoxMessage[4] = memhandle;

    xsd->vcsd_MBoxMessage[5] = 0;

    xsd->vcsd_MBoxMessage[6] = 0; // terminate tag

    MBoxWrite(VCMB_BASE, VCMB_PROPCHAN, xsd->vcsd_MBoxMessage);
    if (MBoxRead(VCMB_BASE, VCMB_PROPCHAN) == xsd->vcsd_MBoxMessage)
    {
        mhe->mhe_MemHeader.mh_Free += size;

        D(bug("[VideoCoreGfx] %s: Memory freed [status %08x]\n", __PRETTY_FUNCTION__, xsd->vcsd_MBoxMessage[5]));
    }
}

void *mh_AllocAbs(struct MemHeaderExt *mhe, IPTR size, APTR  mem)
{
    D(bug("[VideoCoreGfx] %s(0x%p, %d bytes)\n", __PRETTY_FUNCTION__, mem, size));

    return NULL;
}

void *mh_ReAlloc(struct MemHeaderExt *mhe, APTR  old,  IPTR size)
{
    D(bug("[VideoCoreGfx] %s(0x%p, %d bytes)\n", __PRETTY_FUNCTION__, old, size));

    return NULL;
}

ULONG mh_Avail(struct MemHeaderExt *mhe, ULONG flags)
{
    struct VideoCoreGfx_staticdata *xsd = (APTR)mhe->mhe_UserData;

    D(bug("[VideoCoreGfx] %s(0x%08x)\n", __PRETTY_FUNCTION__, flags));

    return mhe->mhe_MemHeader.mh_Free;
}

int FNAME_SUPPORT(InitMem)(void *memstart, int memlength, struct VideoCoreGfxBase *VideoCoreGfxBase)
{
    struct VideoCoreGfx_staticdata *xsd = &LIBBASE->vsd;
//    struct MemChunk *mc = memstart;

    InitSemaphore(&xsd->vcsd_GPUMemLock);

    bug("[VideoCoreGfx] VideoCore GPU Memory @ 0x%p [%dKB]\n", memstart, memlength >> 10);

    xsd->vcsd_GPUMemManage.mhe_MemHeader.mh_Node.ln_Type = NT_MEMORY;
    xsd->vcsd_GPUMemManage.mhe_MemHeader.mh_Node.ln_Name = "VideoCore GPU Memory";
    xsd->vcsd_GPUMemManage.mhe_MemHeader.mh_Node.ln_Pri = -128;
    xsd->vcsd_GPUMemManage.mhe_MemHeader.mh_Attributes = MEMF_CHIP | MEMF_MANAGED | MEMF_PUBLIC;
//    xsd->vcsd_GPUMemManage.mhe_MemHeader.mh_First = mc;
    xsd->vcsd_GPUMemManage.mhe_MemHeader.mh_First = NULL;
    xsd->vcsd_GPUMemManage.mhe_MemHeader.mh_Lower = (APTR)memstart;
    xsd->vcsd_GPUMemManage.mhe_MemHeader.mh_Free = memlength;
    xsd->vcsd_GPUMemManage.mhe_MemHeader.mh_Upper = (APTR)(xsd->vcsd_GPUMemManage.mhe_MemHeader.mh_Free + (IPTR)memstart);

    xsd->vcsd_GPUMemManage.mhe_UserData = xsd;

    xsd->vcsd_GPUMemManage.mhe_Alloc = mh_Alloc;
    xsd->vcsd_GPUMemManage.mhe_Free = mh_Free;
    xsd->vcsd_GPUMemManage.mhe_AllocAbs = mh_AllocAbs;
    xsd->vcsd_GPUMemManage.mhe_ReAlloc = mh_ReAlloc;
    xsd->vcsd_GPUMemManage.mhe_Avail = mh_Avail;

//    mc->mc_Next = NULL;
//    mc->mc_Bytes = xsd->vcsd_GPUMemManage.mhe_MemHeader.mh_Free;

    Disable();
    AddTail(&SysBase->MemList, (struct Node *)&xsd->vcsd_GPUMemManage);
    Enable();

    bug("[VideoCoreGfx] Memory Manager Initialised\n");

    return TRUE;
}
