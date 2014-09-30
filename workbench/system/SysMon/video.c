/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "sysmon_intern.h"

#include <clib/alib_protos.h>

/* Video information */
#if defined(VRAM_HACK)

/* This code is a HACK which uses private nouveau.hidd API to get the VRAM/GART
   information. This will work only on nVidia cards running nouveau driver.
   The reason for this hack is that there is no public API that would return
   the needed values. */

#include <proto/graphics.h>
#include <proto/oop.h>
#include <hidd/graphics.h>
struct BitMap * bm = NULL;
#undef HiddGfxNouveauAttrBase
#undef HiddBitMapAttrBase
OOP_AttrBase HiddGfxNouveauAttrBase;
OOP_AttrBase HiddBitMapAttrBase;
#define IID_Hidd_Gfx_Nouveau            "hidd.gfx.nouveau"
enum
{
    aoHidd_Gfx_Nouveau_VRAMSize,        /* [G..] The amount of total VRAM in bytes */
    aoHidd_Gfx_Nouveau_GARTSize,        /* [G..] The amount of total GART in bytes */
    aoHidd_Gfx_Nouveau_VRAMFree,        /* [G..] The amount of free VRAM in bytes */
    aoHidd_Gfx_Nouveau_GARTFree,        /* [G..] The amount of free GART in bytes */
    
    num_Hidd_Gfx_Nouveau_Attrs
};

#define aHidd_Gfx_Nouveau_VRAMSize      (HiddGfxNouveauAttrBase + aoHidd_Gfx_Nouveau_VRAMSize)
#define aHidd_Gfx_Nouveau_GARTSize      (HiddGfxNouveauAttrBase + aoHidd_Gfx_Nouveau_GARTSize)
#define aHidd_Gfx_Nouveau_VRAMFree      (HiddGfxNouveauAttrBase + aoHidd_Gfx_Nouveau_VRAMFree)
#define aHidd_Gfx_Nouveau_GARTFree      (HiddGfxNouveauAttrBase + aoHidd_Gfx_Nouveau_GARTFree)
struct Library * OOPBase = NULL;

#endif

/* Videofunctions */
static BOOL InitVideo(struct SysMonData *smdata)
{
#if defined(VRAM_HACK)
    struct OOP_ABDescr attrbases[] = 
    {
    { IID_Hidd_BitMap,          &HiddBitMapAttrBase },
    { IID_Hidd_Gfx_Nouveau,     &HiddGfxNouveauAttrBase },
    { NULL, NULL }
    };
    struct Screen * wbscreen;
    
    OOPBase = OpenLibrary("oop.library", 0L);
    
    if (!OOPBase)
        return FALSE;

    if (!OOP_ObtainAttrBases(attrbases))
        return FALSE;

    wbscreen = LockPubScreen(NULL);
    bm = AllocBitMap(32, 32, 0, BMF_DISPLAYABLE, wbscreen->RastPort.BitMap);
    UnlockPubScreen(NULL, wbscreen);
#endif
    return TRUE;
}

static VOID DeInitVideo(struct SysMonData *smdata)
{
#if defined(VRAM_HACK)
    FreeBitMap(bm);
    
    OOP_ReleaseAttrBase(IID_Hidd_BitMap);
    OOP_ReleaseAttrBase(IID_Hidd_Gfx_Nouveau);
    
    CloseLibrary(OOPBase);
#endif
}

VOID UpdateVideoStaticInformation(struct SysMonData * smdata)
{
#if defined(VRAM_HACK)
    TEXT buffer[64] = {0};
    IPTR vram_size, gart_size;
    OOP_Object * gfxhidd;
    OOP_Object * oopbm = HIDD_BM_OBJ(bm);

    OOP_GetAttr(oopbm, aHidd_BitMap_GfxHidd, (APTR)&gfxhidd);
    OOP_GetAttr(gfxhidd, aHidd_Gfx_Nouveau_VRAMSize, &vram_size);
    OOP_GetAttr(gfxhidd, aHidd_Gfx_Nouveau_GARTSize, &gart_size);
    __sprintf(buffer, "%ld kB", (ULONG)(vram_size / 1024));
    set(smdata->memorysize[MEMORY_VRAM], MUIA_Text_Contents, buffer);
    __sprintf(buffer, "%ld kB", (ULONG)(gart_size / 1024));
    set(smdata->memorysize[MEMORY_GART], MUIA_Text_Contents, buffer);
#endif
}

VOID UpdateVideoInformation(struct SysMonData * smdata)
{
#if defined(VRAM_HACK)
    TEXT buffer[64] = {0};
    IPTR vram_free, gart_free;
    OOP_Object * gfxhidd;
    OOP_Object * oopbm = HIDD_BM_OBJ(bm);

    OOP_GetAttr(oopbm, aHidd_BitMap_GfxHidd, (APTR)&gfxhidd);
    OOP_GetAttr(gfxhidd, aHidd_Gfx_Nouveau_VRAMFree, &vram_free);
    OOP_GetAttr(gfxhidd, aHidd_Gfx_Nouveau_GARTFree, &gart_free);
    __sprintf(buffer, "%ld kB", (ULONG)(vram_free / 1024));
    set(smdata->memoryfree[MEMORY_VRAM], MUIA_Text_Contents, buffer);
    __sprintf(buffer, "%ld kB", (ULONG)(gart_free / 1024));
    set(smdata->memoryfree[MEMORY_GART], MUIA_Text_Contents, buffer);
#endif
}

struct SysMonModule videomodule =
{
    .Init = InitVideo,
    .DeInit = DeInitVideo,
};

