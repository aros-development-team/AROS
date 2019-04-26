/*
    Copyright © 1995-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "sysmon_intern.h"

#include <clib/alib_protos.h>

/* Video information */
#include <proto/graphics.h>
#include <proto/oop.h>
#include <hidd/gfx.h>

#define DVMEM(x)

#undef HiddBitMapAttrBase
OOP_AttrBase HiddGfxAttrBase;
OOP_AttrBase HiddBitMapAttrBase;

struct Library * OOPBase = NULL;
OOP_Object * gfxhidd;

/* Videofunctions */
static BOOL InitVideo(struct SysMonData *smdata)
{
    struct OOP_ABDescr attrbases[] = 
    {
        { IID_Hidd_Gfx,         &HiddGfxAttrBase        },
        { IID_Hidd_BitMap,      &HiddBitMapAttrBase     },
        { NULL,                 NULL                    }
    };
    struct Screen * wbscreen;

    OOPBase = OpenLibrary("oop.library", 0L);

    if (!OOPBase)
        return FALSE;

    if (!OOP_ObtainAttrBases(attrbases))
        return FALSE;

    wbscreen = LockPubScreen(NULL);
    OOP_GetAttr(HIDD_BM_OBJ(wbscreen->RastPort.BitMap), aHidd_BitMap_GfxHidd, (APTR)&gfxhidd);
    D(bug("[SysMon:Video] %s: gfxhidd @ 0x%p\n", __func__, gfxhidd);)
    UnlockPubScreen(NULL, wbscreen);

    return TRUE;
}

static VOID DeInitVideo(struct SysMonData *smdata)
{
    OOP_ReleaseAttrBase(IID_Hidd_BitMap);

    CloseLibrary(OOPBase);
}

VOID UpdateVideoStaticInformation(struct SysMonData * smdata)
{
    TEXT buffer[64] = {0};
    struct TagItem memTags[] =
    {
        {tHidd_Gfx_MemTotal,            0       },
        {tHidd_Gfx_MemAddressableTotal, 0       },
        {TAG_DONE,                      0       }
    };

    DVMEM(bug("[SysMon:Video] %s: memTags @ 0x%p\n", __func__, memTags);)
    OOP_GetAttr(gfxhidd, aHidd_Gfx_MemoryAttribs, (IPTR *)memTags);

    __sprintf(buffer, "%ld kB", (ULONG)(memTags[0].ti_Data / 1024));
    set(smdata->memorysize[MEMORY_VMEM], MUIA_Text_Contents, buffer);
    __sprintf(buffer, "%ld kB", (ULONG)(memTags[1].ti_Data / 1024));
    set(smdata->memorysize[MEMORY_VMEMWINDOW], MUIA_Text_Contents, buffer);
}

VOID UpdateVideoInformation(struct SysMonData * smdata)
{
    TEXT buffer[64] = {0};
    struct TagItem memTags[] =
    {
        {tHidd_Gfx_MemFree,             0       },
        {tHidd_Gfx_MemAddressableFree,  0       },
        {TAG_DONE,                      0       }
    };

    DVMEM(bug("[SysMon:Video] %s: memTags @ 0x%p\n", __func__, memTags);)
    OOP_GetAttr(gfxhidd, aHidd_Gfx_MemoryAttribs, (IPTR *)memTags);

    __sprintf(buffer, "%ld kB", (ULONG)(memTags[0].ti_Data / 1024));
    set(smdata->memoryfree[MEMORY_VMEM], MUIA_Text_Contents, buffer);
    __sprintf(buffer, "%ld kB", (ULONG)(memTags[1].ti_Data / 1024));
    set(smdata->memoryfree[MEMORY_VMEMWINDOW], MUIA_Text_Contents, buffer);
}

struct SysMonModule videomodule =
{
    .Init = InitVideo,
    .DeInit = DeInitVideo,
};
