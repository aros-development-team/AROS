/*
    Copyright © 2015-2018, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#include <proto/utility.h>
#include <proto/intuition.h>

#include <hidd/gfx.h>

#include <stdio.h>

#include "gallium_intern.h"

#undef HiddGalliumAttrBase
#define HiddGalliumAttrBase GB(GalliumBase)->galliumAttrBase

/*****************************************************************************

    NAME */

      AROS_LH1(PipeHandle_t, CreatePipe,

/*  SYNOPSIS */ 
      AROS_LHA(struct TagItem *, tags, A0),

/*  LOCATION */
      struct Library *, GalliumBase, 5, Gallium)

/*  FUNCTION
        Instantiates a gallium pipe.

    INPUTS
        tags - a pointer to tags to be used during creation.

    TAGS
        CPS_PipeFriendBitmap - a bitmap our pipe screen will target.
        CPS_PipeScreenDriver * - where to store the driver.
        CPS_GalliumInterfaceVersion - Indicates a version of gallium interface
            that a client is expected to receive. The client expected version
            must ideally match with the version that the driver provides,
            because gallium interface is not backwards compatible. This tag is
            required. Unless otherwise needed, the value
            GALLIUM_INTERFACE_VERSION should be passed.
            See also CreatePipeV.

    RESULT
        A valid pipe instance or NULL if creation was not successful.

    BUGS

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem galliumTags[] = 
    {
        { aHidd_Gallium_InterfaceVersion,       0 },
        { TAG_DONE,                             0 }
    };
    OOP_Object                                  *_driver = NULL;
    OOP_Object                                  **driver;
    struct BitMap                               *friendbm;
    struct Screen                               *pubscreen = NULL;

    galliumTags[0].ti_Data = GetTagData(CPS_GalliumInterfaceVersion, -1, tags);
    friendbm = (struct BitMap *)GetTagData(CPS_PipeFriendBitMap, 0, tags);
    driver = (OOP_Object **)GetTagData(CPS_PipeScreenDriver, (IPTR)&_driver, tags);

    /* The tag is missing */
    if (galliumTags[0].ti_Data == -1)
        return NULL;

    if (!friendbm)
    {
        if ((pubscreen = LockPubScreen(NULL)) != NULL)
            friendbm = pubscreen->RastPort.BitMap;
    }

    D(bug("[Gallium] %s: friendbm @ 0x%p\n", __PRETTY_FUNCTION__, friendbm));

    if (friendbm && IS_HIDD_BM(friendbm))
    {
        OOP_Object *bmObj = HIDD_BM_OBJ(friendbm);
        if (bmObj)
        {
            OOP_Object *gfxhidd;
            OOP_GetAttr(bmObj, aHidd_BitMap_GfxHidd, (IPTR *)&gfxhidd);

            if (gfxhidd)
            {
                *driver = HIDD_Gfx_CreateObject(gfxhidd, GB(GalliumBase)->basegallium, galliumTags);
            }
        }
    }

    if (pubscreen)
        UnlockPubScreen(NULL, pubscreen);

    if (!*driver)
    {
        char tmpname[128];
        if (!GB(GalliumBase)->fallbackmodule)
        {
            sprintf(tmpname, "%s.hidd", GB(GalliumBase)->fallback);

            D(bug("[Gallium] %s: trying fallback '%s' ...\n", __PRETTY_FUNCTION__, tmpname));

            GB(GalliumBase)->fallbackmodule = OpenLibrary(tmpname, 9);

            D(bug("[Gallium] %s: '%s' @ 0x%p\n", __PRETTY_FUNCTION__, tmpname, GB(GalliumBase)->fallbackmodule));
        }

        if (GB(GalliumBase)->fallbackmodule)
        {
            sprintf(tmpname, "hidd.gallium.%s", GB(GalliumBase)->fallback);

            *driver = OOP_NewObject(NULL, tmpname, galliumTags);

            D(bug("[Gallium] %s: '%s' @ 0x%p\n", __PRETTY_FUNCTION__, tmpname, *driver));
        }
    }

    return *driver;

    AROS_LIBFUNC_EXIT
}

