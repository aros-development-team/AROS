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

#include "efifbgfx_support.h"
#include "efifbgfx_hidd.h"

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

/* These must stay in the same order as attrBases[] entries assignment in efifbgfx_intern.h */
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

/*
 * The hardware this driver writes to: the firmware framebuffer, and
 * nothing else. graphics.library matches it against the apertures of
 * native drivers as they arrive, so that this driver is taken down by
 * the one that claims the same hardware - and only by that one.
 *
 * That matters because the GOP surface is not necessarily memory the
 * firmware owns. On the Milk-V Titan it is the GM107's BAR1, so once
 * nouveau installs its own BAR1 page tables the pages behind this
 * pointer stop existing and every redraw lands in whatever the new
 * owner put there instead.
 *
 * Static because graphics.library keeps the pointer for the lifetime of
 * the driver.
 */
static struct DisplayRange efifbgfx_ranges[2];

static int EFIFBGfx_Init(LIBBASETYPEPTR LIBBASE)
{
    struct EFIFBGfx_staticdata *xsd = &LIBBASE->vsd;
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
        if (initEFIFBGfxHW(&xsd->data))
        {
            if (GetAttrBases(interfaces, xsd->attrBases, ATTRBASES_NUM))
            {
                xsd->basebm = OOP_FindClass(CLID_Hidd_BitMap);
                xsd->mid_Dispose = OOP_GetMethodID(IID_Root, moRoot_Dispose);
                D(bug("[EFIFBGfx] BitMap class @ 0x%p\n", xsd->basebm));

                InitSemaphore(&xsd->framebufferlock);
                InitSemaphore(&xsd->HW_acc);

                D(bug("[EFIFBGfx] Init: Everything OK, installing driver\n"));

                /*
                 * The framebuffer belongs to whatever display hardware the
                 * firmware drove. In order to avoid conflicts with native-mode
                 * hardware drivers the driver is removed from the system when
                 * a driver claiming that same hardware is installed.
                 * This is done by graphics.library, on behalf of the incoming
                 * driver, from DDRV_BootMode plus the ranges below.
                 */
                efifbgfx_ranges[0].dr_Base = xsd->data.framebuffer;
                efifbgfx_ranges[0].dr_Size = xsd->data.fbsize;
                efifbgfx_ranges[1].dr_Base = NULL;
                efifbgfx_ranges[1].dr_Size = 0;

                D(bug("[EFIFBGfx] framebuffer aperture @ 0x%p, %lu bytes\n",
                      efifbgfx_ranges[0].dr_Base, (unsigned long)efifbgfx_ranges[0].dr_Size));

                err = AddDisplayDriver(xsd->efifbgfxclass, NULL,
                                       DDRV_BootMode, TRUE,
                                       DDRV_HWRanges, (IPTR)efifbgfx_ranges,
                                       TAG_DONE);

                D(bug("[EFIFBGfx] AddDisplayDriver() result: %u\n", err));
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
        D(bug("[EFIFBGfx] Failed to open graphics.library!\n"));
    }
    return res;
}

ADD2INITLIB(EFIFBGfx_Init, 0)
