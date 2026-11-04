/*
    Copyright (C) 2010-2026, The AROS Development Team. All rights reserved.
*/

#include "nvgfx_intern.h"

#define DEBUG 0
#include <aros/debug.h>

#include <proto/oop.h>
#include <proto/exec.h>
#include <aros/symbolsets.h>

/* GLOBALS */
APTR NVGfxMemPool;

/* kernel.resource, for the DRM spinlock emulation (include/linux/spinlock.h) */
APTR KernelBase;
struct SignalSemaphore globalLock;
/* GLOBALS END */

static ULONG NVGfx_Init(LIBBASETYPEPTR LIBBASE)
{
    struct OOP_ABDescr attrbases[] = 
    {
    { IID_Hidd,                 &LIBBASE->sd.hiddAttrBase },
    { IID_Hidd_BitMap,          &LIBBASE->sd.bitMapAttrBase },
    { IID_Hidd_PixFmt,	        &LIBBASE->sd.pixFmtAttrBase },
    { IID_Hidd_Sync,            &LIBBASE->sd.syncAttrBase },
    { IID_Hidd_Gfx,             &LIBBASE->sd.gfxAttrBase },
    { IID_Hidd_Display,         &LIBBASE->sd.displayAttrBase },
    { IID_Hidd_DMEnum,          &LIBBASE->sd.dmenumAttrBase },
    { IID_Hidd_Gfx_NVGfx,     &LIBBASE->sd.gfxNVGfxAttrBase },
    { IID_Hidd_PlanarBM,        &LIBBASE->sd.planarAttrBase },
    { IID_Hidd_I2C_NVGfx,       &LIBBASE->sd.i2cNVGfxAttrBase },
    { IID_Hidd_GC,              &LIBBASE->sd.gcAttrBase },
    { IID_Hidd_BitMap_NVGfx,  &LIBBASE->sd.bitMapNVGfxAttrBase },
    { NULL, NULL }
    };

    InitSemaphore(&globalLock);

    /* the DRM spinlocks (include/linux/spinlock.h) are kernel spinlocks */
    KernelBase = OpenResource("kernel.resource");

    D(bug("[Nouveau] %s: library init entered\n", __func__));

    if (!OOP_ObtainAttrBases(attrbases))
    {
        D(bug("[Nouveau] %s: OOP_ObtainAttrBases failed\n", __func__));
        return FALSE;
    }

    LIBBASE->sd.basegc = OOP_FindClass(CLID_Hidd_GC);
    LIBBASE->sd.basebm = OOP_FindClass(CLID_Hidd_BitMap);
    LIBBASE->sd.basei2c = OOP_FindClass(CLID_Hidd_I2C);
    
    LIBBASE->sd.mid_CopyMemBox16    = OOP_GetMethodID((STRPTR)IID_Hidd_BitMap, moHidd_BitMap_CopyMemBox16);
    LIBBASE->sd.mid_CopyMemBox32    = OOP_GetMethodID((STRPTR)IID_Hidd_BitMap, moHidd_BitMap_CopyMemBox32);
    LIBBASE->sd.mid_PutMem32Image16 = OOP_GetMethodID((STRPTR)IID_Hidd_BitMap, moHidd_BitMap_PutMem32Image16);
    LIBBASE->sd.mid_GetMem32Image16 = OOP_GetMethodID((STRPTR)IID_Hidd_BitMap, moHidd_BitMap_GetMem32Image16);
    LIBBASE->sd.mid_PutMemTemplate16= OOP_GetMethodID((STRPTR)IID_Hidd_BitMap, moHidd_BitMap_PutMemTemplate16);
    LIBBASE->sd.mid_PutMemTemplate32= OOP_GetMethodID((STRPTR)IID_Hidd_BitMap, moHidd_BitMap_PutMemTemplate32);
    LIBBASE->sd.mid_PutMemPattern16 = OOP_GetMethodID((STRPTR)IID_Hidd_BitMap, moHidd_BitMap_PutMemPattern16);
    LIBBASE->sd.mid_PutMemPattern32 = OOP_GetMethodID((STRPTR)IID_Hidd_BitMap, moHidd_BitMap_PutMemPattern32);
    LIBBASE->sd.mid_ConvertPixels   = OOP_GetMethodID((STRPTR)IID_Hidd_BitMap, moHidd_BitMap_ConvertPixels);
    LIBBASE->sd.mid_GetPixFmt       = OOP_GetMethodID((STRPTR)IID_Hidd_DMEnum, moHidd_DMEnum_GetPixFmt);

    InitSemaphore(&LIBBASE->sd.multibitmapsemaphore);
    InitSemaphore(&LIBBASE->sd.nvddLock);

    NVGfxMemPool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR | MEMF_SEM_PROTECTED, 32 * 1024, 16 * 1024);

    D(bug("[Nouveau] Nouveau_Init: done, pool 0x%p\n", NVGfxMemPool));

    return TRUE;
}

static VOID NVGfx_Exit(LIBBASETYPEPTR LIBBASE)
{
    /* the 3D parts: the gallium and Vulkan libraries */
    HIDDNVGfxNVDDExit(&LIBBASE->sd);
    if (NVGfxMemPool)
    {
        DeletePool(NVGfxMemPool);
        NVGfxMemPool = NULL;
    }
}


#if defined(MEM_GUAD_DEBUG)
#include <aros/debug.h>

#define BUFF_GUARD (4 * 1024)

APTR HIDDNVGfxAlloc(ULONG size)
{
    ULONG guarded_size = size + (2 * BUFF_GUARD);
    char *memory = (char *)AllocVecPooled(NVGfxMemPool, guarded_size);
    memory += BUFF_GUARD;

    char *start = (char *)memory - BUFF_GUARD;

    ULONG *sizestore = (ULONG *)start;
    *sizestore = size; /* store in first 4 bytes of front guard */
    sizestore = (ULONG *)memory;
    sizestore--;
    *sizestore = size; /* store in last 4 bytes of front guard */

    ULONG *patternstore = (ULONG *)start;
    patternstore++;

    /* Write pattern to front guard except for fields holding size */
    for (int i = 0; i < (BUFF_GUARD / 4) - 2; i++)
    {
        *patternstore = 0xFEAB1381;
        patternstore++;
    }

    /* Write patter to back guard */
    patternstore = (ULONG *)(memory + size);
    for (int i = 0; i < (BUFF_GUARD / 4); i++)
    {
        *patternstore = 0xFEAB1381;
        patternstore++;
    }

    return memory;
}

VOID HIDDNVGfxFree(APTR memory)
{
    if (memory == NULL) return;

    char *start = (char *)memory - BUFF_GUARD;

    ULONG *sizestore = (ULONG *)start;
    ULONG size1 = *sizestore;
    sizestore = (ULONG *)memory;
    sizestore--;
    ULONG size2 = *sizestore;

    if (size1 != size2) bug("---- MEM CHECK: Size overwritten! %u <> %u", size1, size2);

    ULONG *patternstore = (ULONG *)start;
    patternstore++;

    for (int i = 0; i < (BUFF_GUARD / 4) - 2; i++)
    {
        ULONG pattern = *patternstore;
        if (pattern != 0xFEAB1381) bug("---- MEM CHECK: Damaged front guard pattern %08x!\n", pattern);
        patternstore++;
    }

    patternstore = (ULONG *)(memory + size2);

    LONG first = -1, last = 0;

    for (int i = 0; i < (BUFF_GUARD / 4); i++)
    {
        ULONG pattern = *patternstore;
        if (pattern != 0xFEAB1381)
        {
            if (first == -1) first = i;
            if (last < i) last = i;
        }
        patternstore++;
    }

    if (first != -1 && last != 0)
        bug("---- MEM CHECK: Damaged back guard for mem=%p(%d) from %d to %d\n", memory, size2, first * 4, last * 4);

    FreeVecPooled(NVGfxMemPool, start);
}
#else
/*
 * Straight exec allocations rather than a semaphore-protected pool: the
 * drm code allocates under its "spinlocks" (Forbid sections), and a pool
 * semaphore that has to wait there would let other tasks into the section.
 * The 16-byte header keeps the size and the alignment kmalloc callers expect.
 */
#define NOUVEAU_ALLOC_HEADER    16

APTR HIDDNVGfxAlloc(ULONG size)
{
    IPTR total = (IPTR)size + NOUVEAU_ALLOC_HEADER;
    IPTR *memory = AllocMem(total, MEMF_PUBLIC | MEMF_CLEAR);

    if (memory == NULL)
        return NULL;

    memory[0] = total;
    return (APTR)((UBYTE *)memory + NOUVEAU_ALLOC_HEADER);
}

VOID HIDDNVGfxFree(APTR memory)
{
    if (memory != NULL)
    {
        IPTR *real = (IPTR *)((UBYTE *)memory - NOUVEAU_ALLOC_HEADER);
        FreeMem(real, real[0]);
    }
}

IPTR HIDDNVGfxAllocSize(CONST_APTR memory)
{
    if (memory != NULL)
    {
        IPTR *real = (IPTR *)((UBYTE *)memory - NOUVEAU_ALLOC_HEADER);
        return real[0] - NOUVEAU_ALLOC_HEADER;
    }
    return 0;
}
#endif

ADD2INITLIB(NVGfx_Init, 0);
ADD2EXPUNGELIB(NVGfx_Exit, 0);

ADD2LIBS((STRPTR)"pci.hidd", 0, static struct Library *, PciHiddBase);
