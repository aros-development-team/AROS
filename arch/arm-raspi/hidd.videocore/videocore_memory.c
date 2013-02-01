/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#define __OOP_NOATTRBASES__

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/vcmbox.h>
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

#include "videocore_class.h"
#include "videocore_hardware.h"

#include LC_LIBDEFS_FILE

#ifdef VCMBoxBase
#undef VCMBoxBase
#endif

#define VCMBoxBase      xsd->vcsd_VCMBoxBase

void *mh_Alloc(struct MemHeaderExt *mhe, IPTR size, ULONG *flags)
{
    D(bug("[VideoCore] mh_Alloc(%d bytes, 0x%08x)\n", size, flags));

    return NULL;
}

/*
    returns a handle on a block of gpu memory. you cannot directly access this
    memory unless it is locked, since it may be moved around by the gpu.
*/
void *videocore_Alloc(struct MemHeaderExt *mhe, IPTR size, ULONG *flags)
{
    struct VideoCore_staticdata *xsd = (APTR)mhe->mhe_UserData;
    ULONG gpumemflags = VCMEM_NORMAL, gpumemalign = 0;

    D(bug("[VideoCore] videocore_Alloc(%d bytes, 0x%08x)\n", size, flags));

    if (!(*flags & MEMF_PUBLIC))
        gpumemflags = VCMEM_L1NONALLOCATING | VCMEM_NOINIT | VCMEM_LAZYLOCK;
    if (*flags & MEMF_CLEAR)
        gpumemflags |= VCMEM_ZERO;
    if (*flags & MEMF_HWALIGNED)
        gpumemalign = 4096;

    xsd->vcsd_VCMBoxMessage[0] = 9 * 4;
    xsd->vcsd_VCMBoxMessage[1] = VCTAG_REQ;
    xsd->vcsd_VCMBoxMessage[2] = VCTAG_ALLOCMEM;
    xsd->vcsd_VCMBoxMessage[3] = 4 * 3;
    xsd->vcsd_VCMBoxMessage[4] = size;
    xsd->vcsd_VCMBoxMessage[5] = gpumemalign;
    xsd->vcsd_VCMBoxMessage[6] = gpumemflags;

    xsd->vcsd_VCMBoxMessage[7] = 0;

    xsd->vcsd_VCMBoxMessage[8] = 0; // terminate tag

    VCMBoxWrite(VCMB_BASE, VCMB_FBCHAN, xsd->vcsd_VCMBoxMessage);
    if (VCMBoxRead(VCMB_BASE, VCMB_FBCHAN) == xsd->vcsd_VCMBoxMessage)
    {
        D(bug("[VideoCore] videocore_Alloc: Allocated %d bytes, memhandle @ 0x%p\n", xsd->vcsd_VCMBoxMessage[4], xsd->vcsd_VCMBoxMessage[7]));
        
        mhe->mhe_MemHeader.mh_Free -= size;
        
        return xsd->vcsd_VCMBoxMessage[7];
    }
    return NULL;
}

/*
    Lock the gpu memory represented by the memhandle,
    and return a physical pointer to it..
*/
void *videocore_LockMem(struct MemHeaderExt *mhe, void *memhandle)
{
    struct VideoCore_staticdata *xsd = (APTR)mhe->mhe_UserData;

    D(bug("[VideoCore] videocore_LockMem(memhandle @ 0x%p)\n", memhandle));
    
    xsd->vcsd_VCMBoxMessage[0] = 7 * 4;
    xsd->vcsd_VCMBoxMessage[1] = VCTAG_REQ;
    xsd->vcsd_VCMBoxMessage[2] = VCTAG_LOCKMEM;
    xsd->vcsd_VCMBoxMessage[3] = 4;
    xsd->vcsd_VCMBoxMessage[4] = memhandle;

    xsd->vcsd_VCMBoxMessage[5] = 0;

    xsd->vcsd_VCMBoxMessage[6] = 0; // terminate tag

    VCMBoxWrite(VCMB_BASE, VCMB_FBCHAN, xsd->vcsd_VCMBoxMessage);
    if (VCMBoxRead(VCMB_BASE, VCMB_FBCHAN) == xsd->vcsd_VCMBoxMessage)
    {
        D(bug("[VideoCore] videocore_LockMem: Memory locked @ 0x%p\n", xsd->vcsd_VCMBoxMessage[5]));
        return xsd->vcsd_VCMBoxMessage[5];
    }
    return NULL;
}

/*
    Unlock the gpu memory represented by the memhandle.
*/
void *videocore_UnLockMem(struct MemHeaderExt *mhe, void *memhandle)
{
    struct VideoCore_staticdata *xsd = (APTR)mhe->mhe_UserData;

    D(bug("[VideoCore] videocore_UnLockMem(memhandle @ 0x%p)\n", memhandle));
    
    xsd->vcsd_VCMBoxMessage[0] = 7 * 4;
    xsd->vcsd_VCMBoxMessage[1] = VCTAG_REQ;
    xsd->vcsd_VCMBoxMessage[2] = VCTAG_UNLOCKMEM;
    xsd->vcsd_VCMBoxMessage[3] = 4;
    xsd->vcsd_VCMBoxMessage[4] = memhandle;

    xsd->vcsd_VCMBoxMessage[5] = 0;

    xsd->vcsd_VCMBoxMessage[6] = 0; // terminate tag

    VCMBoxWrite(VCMB_BASE, VCMB_FBCHAN, xsd->vcsd_VCMBoxMessage);
    if (VCMBoxRead(VCMB_BASE, VCMB_FBCHAN) == xsd->vcsd_VCMBoxMessage)
    {
        D(bug("[VideoCore] videocore_UnLockMem: Memory unlocked [status %08x]\n", xsd->vcsd_VCMBoxMessage[5]));
        return xsd->vcsd_VCMBoxMessage[5];
    }
    return NULL;
}

void mh_Free(struct MemHeaderExt *mhe, APTR  mem,  IPTR size)
{
    D(bug("[VideoCore] mh_Free(0x%p)\n", mem));
}

/*
    Frees the gpu memory associated with the handle.
*/
void videocore_Free(struct MemHeaderExt *mhe, APTR  memhandle, IPTR size)
{
    struct VideoCore_staticdata *xsd = (APTR)mhe->mhe_UserData;

    D(bug("[VideoCore] videocore_Free(memhandle @ 0x%p)\n", memhandle));

    xsd->vcsd_VCMBoxMessage[0] = 7 * 4;
    xsd->vcsd_VCMBoxMessage[1] = VCTAG_REQ;
    xsd->vcsd_VCMBoxMessage[2] = VCTAG_FREEMEM;
    xsd->vcsd_VCMBoxMessage[3] = 4;
    xsd->vcsd_VCMBoxMessage[4] = memhandle;

    xsd->vcsd_VCMBoxMessage[5] = 0;

    xsd->vcsd_VCMBoxMessage[6] = 0; // terminate tag

    VCMBoxWrite(VCMB_BASE, VCMB_FBCHAN, xsd->vcsd_VCMBoxMessage);
    if (VCMBoxRead(VCMB_BASE, VCMB_FBCHAN) == xsd->vcsd_VCMBoxMessage)
    {
        mhe->mhe_MemHeader.mh_Free += size;

        D(bug("[VideoCore] videocore_Free: Memory freed [status %08x]\n", xsd->vcsd_VCMBoxMessage[5]));
    }
}

void *mh_AllocAbs(struct MemHeaderExt *mhe, IPTR size, APTR  mem)
{
    D(bug("[VideoCore] mh_AllocAbs(0x%p, %d bytes)\n", mem, size));

    return NULL;
}

void *mh_ReAlloc(struct MemHeaderExt *mhe, APTR  old,  IPTR size)
{
    D(bug("[VideoCore] mh_ReAlloc(0x%p, %d bytes)\n", old, size));

    return NULL;
}

ULONG mh_Avail(struct MemHeaderExt *mhe, ULONG flags)
{
    struct VideoCore_staticdata *xsd = (APTR)mhe->mhe_UserData;

    D(bug("[VideoCore] mh_Avail(0x%08x)\n", flags));

    return mhe->mhe_MemHeader.mh_Free;
}

int videocore_InitMem(void *memstart, int memlength, struct VideoCoreBase *VideoCoreBase)
{
    struct VideoCore_staticdata *xsd = &LIBBASE->vsd;
    struct MemChunk *mc = memstart;

    InitSemaphore(&xsd->vcsd_GPUMemLock);

    D(bug("[VideoCore] videocore_MemoryInit: VideoCore GPU Memory @ 0x%p [%dKB]\n", memstart, memlength >> 10));

    xsd->vcsd_GPUMemManage.mhe_MemHeader.mh_Node.ln_Type = NT_MEMORY;
    xsd->vcsd_GPUMemManage.mhe_MemHeader.mh_Node.ln_Name = "VideoCore GPU Memory";
    xsd->vcsd_GPUMemManage.mhe_MemHeader.mh_Node.ln_Pri = -128;
    xsd->vcsd_GPUMemManage.mhe_MemHeader.mh_Attributes = MEMF_CHIP | MEMF_MANAGED | MEMF_PUBLIC;
    xsd->vcsd_GPUMemManage.mhe_MemHeader.mh_First = mc;
    xsd->vcsd_GPUMemManage.mhe_MemHeader.mh_Lower = (APTR)mc;
    xsd->vcsd_GPUMemManage.mhe_MemHeader.mh_Free = memlength;
    xsd->vcsd_GPUMemManage.mhe_MemHeader.mh_Upper = (APTR)(xsd->vcsd_GPUMemManage.mhe_MemHeader.mh_Free + (IPTR)mc);

    xsd->vcsd_GPUMemManage.mhe_UserData = xsd;

    xsd->vcsd_GPUMemManage.mhe_Alloc = mh_Alloc;
    xsd->vcsd_GPUMemManage.mhe_Free = mh_Free;
    xsd->vcsd_GPUMemManage.mhe_AllocAbs = mh_AllocAbs;
    xsd->vcsd_GPUMemManage.mhe_ReAlloc = mh_ReAlloc;
    xsd->vcsd_GPUMemManage.mhe_Avail = mh_Avail;

    mc->mc_Next = NULL;
    mc->mc_Bytes = xsd->vcsd_GPUMemManage.mhe_MemHeader.mh_Free;

    Disable();
    AddTail(&SysBase->MemList, (struct Node *)&xsd->vcsd_GPUMemManage);
    Enable();

    D(bug("[VideoCore] videocore_MemoryInit: Memory Manager Initialised\n"));

    return TRUE;
}
