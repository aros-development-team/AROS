/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    vulkan.library: library set-up and the drivers.

    A driver is a vulkan.hidd object: the one the display of the default
    public screen makes when its CreateObject is asked for the hidd.vulkan
    class, and the software driver (lavapipe.hidd) when there is none, or
    when the VK_DRIVER variable asks for it:

        VK_DRIVER=software  the software driver only
        VK_DRIVER=all       the display's driver and the software driver

    The objects are made once, on the first use of the API, and kept until
    the library is expunged.
*/

#include <aros/debug.h>
#include <aros/symbolsets.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/oop.h>
#include <proto/intuition.h>

#include <intuition/screens.h>
#include <hidd/gfx.h>
#include <hidd/vulkan_hidd.h>

#include <string.h>

#include "vulkan_wsi.h"

#include LC_LIBDEFS_FILE

struct vkl_icd vkl_icds[VKL_MAX_ICDS];
ULONG vkl_icd_count;

static struct SignalSemaphore vkl_icd_lock;
static BOOL vkl_icd_done;
static struct Library *vkl_hiddbase;    /* vulkan.hidd, for its class */
static struct Library *vkl_swbase;      /* lavapipe.hidd */

APTR vkl_alloc(ULONG size)
{
    return AllocVec(size, MEMF_PUBLIC | MEMF_CLEAR);
}

void vkl_free(APTR mem)
{
    FreeVec(mem);
}

static void vkl_icd_add(OOP_Object *obj, OOP_Object *display, BOOL software)
{
    struct pHidd_Vulkan_GetInstanceProcAddr msg;
    PFN_vkGetInstanceProcAddr gipa;

    msg.mID = OOP_GetMethodID(IID_Hidd_Vulkan, moHidd_Vulkan_GetInstanceProcAddr);
    msg.instance = NULL;
    msg.pName = "vkGetInstanceProcAddr";
    gipa = (PFN_vkGetInstanceProcAddr)OOP_DoMethod(obj, (OOP_Msg)&msg);

    if (!gipa || vkl_icd_count >= VKL_MAX_ICDS)
    {
        D(bug("[Vulkan] driver object %p has no vkGetInstanceProcAddr\n", obj));
        OOP_DisposeObject(obj);
        return;
    }

    vkl_icds[vkl_icd_count].obj = obj;
    vkl_icds[vkl_icd_count].display = display;
    vkl_icds[vkl_icd_count].gipa = gipa;
    vkl_icds[vkl_icd_count].software = software;
    vkl_icd_count++;
}

BOOL vkl_icds_init(void)
{
    struct TagItem tags[] =
    {
        { 0, VULKAN_INTERFACE_VERSION },
        { TAG_DONE, 0 }
    };
    OOP_AttrBase vkAttrBase, bmAttrBase;
    OOP_Class *vkclass;
    char mode[16];
    BOOL wanthw = TRUE, wantsw = FALSE;

    ObtainSemaphore(&vkl_icd_lock);
    if (vkl_icd_done)
    {
        ReleaseSemaphore(&vkl_icd_lock);
        return vkl_icd_count > 0;
    }
    vkl_icd_done = TRUE;

    mode[0] = 0;
    if (GetVar("VK_DRIVER", mode, sizeof(mode), 0) > 0)
    {
        if (!stricmp(mode, "software"))
        {
            wanthw = FALSE;
            wantsw = TRUE;
        }
        else if (!stricmp(mode, "all"))
            wantsw = TRUE;
    }

    if (!vkl_hiddbase)
        vkl_hiddbase = OpenLibrary("vulkan.hidd", 0);
    vkclass = vkl_hiddbase ? OOP_FindClass(CLID_Hidd_Vulkan) : NULL;
    vkAttrBase = OOP_ObtainAttrBase(IID_Hidd_Vulkan);
    bmAttrBase = OOP_ObtainAttrBase(IID_Hidd_BitMap);

    if (vkclass && vkAttrBase && bmAttrBase)
    {
        tags[0].ti_Tag = vkAttrBase + aoHidd_Vulkan_InterfaceVersion;

        if (wanthw)
        {
            struct Screen *scr = LockPubScreen(NULL);

            if (scr)
            {
                struct BitMap *bm = scr->RastPort.BitMap;
                OOP_Object *display = NULL;

                if (bm && IS_HIDD_BM(bm) && HIDD_BM_OBJ(bm))
                    OOP_GetAttr(HIDD_BM_OBJ(bm), bmAttrBase + aoHidd_BitMap_Display, (IPTR *)&display);
                UnlockPubScreen(NULL, scr);

                if (display)
                {
                    struct pHidd_Display_CreateObject msg;
                    OOP_Object *obj;

                    msg.mID = OOP_GetMethodID(IID_Hidd_Display, moHidd_Display_CreateObject);
                    msg.cl = vkclass;
                    msg.attrList = tags;
                    obj = (OOP_Object *)OOP_DoMethod(display, (OOP_Msg)&msg);
                    if (obj)
                        vkl_icd_add(obj, display, FALSE);
                }
            }
        }

        /* no Vulkan from the display: the software driver, as gallium.library does */
        if (wantsw || vkl_icd_count == 0)
        {
            if (!vkl_swbase)
                vkl_swbase = OpenLibrary("lavapipe.hidd", 0);
            if (vkl_swbase)
            {
                OOP_Object *obj = OOP_NewObject(NULL, "hidd.vulkan.lavapipe", tags);

                if (obj)
                    vkl_icd_add(obj, NULL, TRUE);
            }
        }
    }

    if (bmAttrBase)
        OOP_ReleaseAttrBase(IID_Hidd_BitMap);
    if (vkAttrBase)
        OOP_ReleaseAttrBase(IID_Hidd_Vulkan);

    D(bug("[Vulkan] %lu driver(s)\n", (unsigned long)vkl_icd_count));

    ReleaseSemaphore(&vkl_icd_lock);
    return vkl_icd_count > 0;
}

void vkl_icds_exit(void)
{
    ULONG i;

    for (i = 0; i < vkl_icd_count; i++)
        OOP_DisposeObject(vkl_icds[i].obj);
    vkl_icd_count = 0;
    vkl_icd_done = FALSE;

    if (vkl_swbase)
        CloseLibrary(vkl_swbase);
    vkl_swbase = NULL;
    if (vkl_hiddbase)
        CloseLibrary(vkl_hiddbase);
    vkl_hiddbase = NULL;
}

static int Vulkan_Init(LIBBASETYPEPTR LIBBASE)
{
    InitSemaphore(&vkl_icd_lock);
    InitSemaphore(&vkl_wsi_lock);
    NEWLIST((struct List *)&vkl_wsi_surfaces);
    vkl_flip_init();
    return TRUE;
}

static int Vulkan_Expunge(LIBBASETYPEPTR LIBBASE)
{
    vkl_flip_exit();
    vkl_icds_exit();
    return TRUE;
}

ADD2INITLIB(Vulkan_Init, 0)
ADD2EXPUNGELIB(Vulkan_Expunge, 0)
