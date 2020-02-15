/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: SAGA Gfx Hidd for V4 AROS
    Lang: english
*/

#define __OOP_NOATTRBASES__

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/oop.h>
#include <exec/types.h>
#include <exec/lists.h>
#include <graphics/driver.h>
#include <graphics/gfxbase.h>
#include <hidd/gfx.h>
#include <oop/oop.h>
#include <utility/utility.h>
#include <aros/symbolsets.h>

#include "sagagfx_hidd.h"
#include "sagagfx_hw.h"

#include LC_LIBDEFS_FILE

/*
 * The following two functions are candidates for inclusion into oop.library.
 * For slightly other implementation see incomplete Android-hosted graphics driver.
 */
static void FreeAttrBases(const STRPTR *iftable, OOP_AttrBase *bases, ULONG num)
{
    ULONG i;

    for (i = 0; i < num; i++)
    {
	if (bases[i])
	    OOP_ReleaseAttrBase(iftable[i]);
    }
}

static BOOL GetAttrBases(const STRPTR *iftable, OOP_AttrBase *bases, ULONG num)
{
    ULONG i;

    for (i = 0; i < num; i++)
    {
	bases[i] = OOP_ObtainAttrBase(iftable[i]);
	if (!bases[i])
	{
	    FreeAttrBases(iftable, bases, i);
	    return FALSE;
	}
    }

    return TRUE;
}

/* These must stay in the same order as attrBases[] entries assignment in sagagfx_hidd.h */
static const STRPTR interfaces[ATTRBASES_NUM] =
{
    IID_Hidd_ChunkyBM,
    IID_Hidd_BitMap,
    IID_Hidd_Gfx,
    IID_Hidd_PixFmt,
    IID_Hidd_Sync,
    IID_Hidd,
    IID_Hidd_ColorMap
};

static int SAGAGfx_Init(LIBBASETYPEPTR LIBBASE)
{
    struct SAGAGfx_staticdata *xsd = &LIBBASE->vsd;
    struct GfxBase *GfxBase;
    struct Library *IconBase;
    ULONG err;
    int res = FALSE;

    D(bug("[SAGA] SAGAGfx_Init() called\n"));

    /* Check if Vampire V4 is detected */
    if (SAGA_Init() == FALSE)
        return FALSE;

    xsd->mempool = CreatePool(MEMF_FAST | MEMF_CLEAR, 32768, 16384);
    if (xsd->mempool == NULL)
        return FALSE;

    xsd->visible = NULL;

#if 0
    IconBase = OpenLibrary("icon.library", 0);
    xsd->useHWSprite = FALSE;

    if (IconBase)
    {
        struct DiskObject *icon;
        STRPTR myName = FindTask(NULL)->tc_Node.ln_Name;

        bug("[SAGA] IconBase = %p\n", IconBase);
        bug("[SAGA] MyName='%s'\n", myName);

        icon = GetDiskObject(myName);

        bug("[SAGA] DiskObject: %p\n", icon);
        if (icon)
        {
            STRPTR hwSprite = FindToolType(icon->do_ToolTypes, "HWSPRITE");
            bug("[SAGA] hwSprite='%s'\n", hwSprite);
            if (hwSprite)
            {
                if (MatchToolValue(hwSprite, "Yes"))
                {
                    xsd->useHWSprite = TRUE;
                }
            }
            FreeDiskObject(icon);
        }

        CloseLibrary(IconBase);
    }
#endif

    /* Initialize lock */
//    InitSemaphore(&xsd->framebufferlock);

    /* Obtain AttrBases */
    if (!GetAttrBases(interfaces, xsd->attrBases, ATTRBASES_NUM))
        return FALSE;

    D(bug("[SAGA] SAGA detected. Installing the driver\n"));

    /*
        Open graphics.library ourselves because we will close it
        after adding the driver.
        Autoinit code would close it only upon driver expunge.
     */
    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 41);
    if (!GfxBase)
    {
        D(bug("[SAGA] Failed to open graphics.library!\n"));
        return FALSE;
    }

    LIBBASE->vsd.basebm = OOP_FindClass(CLID_Hidd_BitMap);

    /* We use ourselves, and no one else does */
    LIBBASE->library.lib_OpenCnt = 1;
    res = TRUE;

    CloseLibrary(&GfxBase->LibNode);

    return res;
}

ADD2LIBS((STRPTR)"gfx.hidd", 0, static struct Library *, __gfxbase);
ADD2INITLIB(SAGAGfx_Init, 0)
