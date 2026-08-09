/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: EFI framebuffer Gfx Hidd initialization.
*/

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

#include "efigfx_support.h"
#include "efigfx_hidd.h"

#include LC_LIBDEFS_FILE

#define DEBUG 0
#include <aros/debug.h>

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

/* These must stay in the same order as attrBases[] entries assignment in efigfx_intern.h */
static const STRPTR interfaces[ATTRBASES_NUM] =
{
    IID_Hidd_ChunkyBM,
    IID_Hidd_BitMap,
    IID_Hidd_Gfx,
    IID_Hidd_PixFmt,
    IID_Hidd_Sync,
    IID_Hidd,
    IID_Hidd_Display,
    IID_Hidd_DMEnum
};

static int EFIGfx_Init(LIBBASETYPEPTR LIBBASE)
{
    struct EFIGfx_staticdata *xsd = &LIBBASE->vsd;
    struct GfxBase *GfxBase;
    ULONG err;
    int res = FALSE;

    /*
     * Open graphics.library ourselves because we will close it
     * after adding the driver.
     * Autoinit code would close it only upon driver expunge.
     */
    GfxBase = (struct GfxBase *)TaggedOpenLibrary(TAGGEDOPEN_GRAPHICS);
    if (GfxBase)
    {
        if (initEFIGfxHW(&xsd->data))
        {
            if (GetAttrBases(interfaces, xsd->attrBases, ATTRBASES_NUM))
            {
                xsd->basebm = OOP_FindClass(CLID_Hidd_BitMap);
                xsd->mid_Dispose = OOP_GetMethodID(IID_Root, moRoot_Dispose);
                D(bug("[EFIGfx] BitMap class @ 0x%p\n", xsd->basebm));

                InitSemaphore(&xsd->framebufferlock);
                InitSemaphore(&xsd->HW_acc);

                D(bug("[EFIGfx] Init: Everything OK, installing driver\n"));

                /*
                 * The framebuffer belongs to whatever display hardware the
                 * firmware drove. In order to avoid conflicts with native-mode
                 * hardware drivers the driver is removed from the system when
                 * some other driver is installed.
                 * This is done by graphics.library if DDRV_BootMode is set to TRUE.
                 */
                err = AddDisplayDriver(xsd->efigfxclass, NULL, DDRV_BootMode, TRUE, TAG_DONE);

                D(bug("[EFIGfx] AddDisplayDriver() result: %u\n", err));
                if (!err)
                {
                    /* expunge protection */
                    LIBBASE->library.lib_OpenCnt = 1;
                    res = TRUE;
                }
            }
        }
        CloseLibrary(&GfxBase->LibNode);
    }
    else
    {
        D(bug("[EFIGfx] Failed to open graphics.library!\n"));
    }
    return res;
}

ADD2INITLIB(EFIGfx_Init, 0)
