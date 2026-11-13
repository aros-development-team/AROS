/*
    Copyright (C) 2021-2026, The AROS Development Team. All rights reserved.

    Desc: Headless Gfx Hidd
*/

#define __OOP_NOATTRBASES__

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

#include "headlessgfx_hidd.h"

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

/* These must stay in the same order as attrBases[] entries assignment in headlessgfxclass.h */
static const STRPTR interfaces[ATTRBASES_NUM] =
{
    IID_Hidd_ChunkyBM,
    IID_Hidd_BitMap,
    IID_Hidd_Gfx,
    IID_Hidd_PixFmt,
    IID_Hidd_Sync,
    IID_Hidd,
    IID_Hidd_Display,
    IID_Hidd_DMEnum,
    IID_Hidd_Gfx_Headless
};

static int HeadlessGfx_Init(LIBBASETYPEPTR LIBBASE)
{
    struct HeadlessGfx_staticdata *xsd = &LIBBASE->vsd;

    if (!GetAttrBases(interfaces, xsd->attrBases, ATTRBASES_NUM))
        return FALSE;

    xsd->basebm = OOP_FindClass(CLID_Hidd_BitMap);
    D(bug("[HeadlessGfx] BitMap class @ 0x%p\n", xsd->basebm));

    /*
     * When this module is part of the kickstart there is no monitor
     * loader to register the driver, so it has to add itself. When it
     * lives on disk, DEVS:Monitors/Headless performs the registration
     * and passes the user's depth configuration from its icon; adding
     * ourselves here as well would create the driver object before the
     * loader could supply those tags.
     */
    if (FindResident(MOD_NAME_STRING))
    {
        struct GfxBase *GfxBase;
        ULONG err = (ULONG)-1;

        /*
         * Open graphics.library ourselves because we will close it
         * after adding the driver.
         * Autoinit code would close it only upon driver expunge.
         */
        GfxBase = (struct GfxBase *)TaggedOpenLibrary(TAGGEDOPEN_GRAPHICS);
        if (GfxBase)
        {
            /*
             * It is unknown (and no way to know) what hardware part this driver uses.
             * In order to avoid conflicts with disk-based native-mode hardware
             * drivers it needs to be removed from the system when some other driver
             * is installed.
             * This is done by graphics.library if DDRV_BootMode is set to TRUE.
             */
            err = AddDisplayDriver(xsd->headlessgfxclass, NULL, DDRV_BootMode, TRUE, TAG_DONE);

            D(bug("[HeadlessGfx] AddDisplayDriver() result: %u\n", err));
            CloseLibrary(&GfxBase->LibNode);
        }
        else
        {
            D(bug("[HeadlessGfx] Failed to open graphics.library!\n"));
        }

        if (err)
            return FALSE;

        /* expunge protection */
        LIBBASE->library.lib_OpenCnt = 1;
    }

    return TRUE;
}

ADD2INITLIB(HeadlessGfx_Init, 0)
