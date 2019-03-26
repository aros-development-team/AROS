/*
    Copyright © 1995-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <graphics/driver.h>
#include <graphics/gfxbase.h>
#include <hidd/gfx.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/oop.h>

#include "amigavideo_hidd.h"
#include "amigavideo_bitmap.h"
#include "chipset.h"

#include LC_LIBDEFS_FILE

int Init_AmigaVideoClass(LIBBASETYPEPTR LIBBASE);

static int AmigaVideo_Init(LIBBASETYPEPTR LIBBASE)
{
    ULONG err;
    struct Library *GfxBase, *OOPBase = LIBBASE->csd.cs_OOPBase;

    D(
      bug("[AmigaVideo] %s() *****************************\n", __func__);
      bug("[AmigaVideo] %s: libbase @ 0x%p\n", __func__, LIBBASE);
      bug("[AmigaVideo] %s: csd @ 0x%p\n", __func__, &LIBBASE->csd);
      bug("[AmigaVideo] %s: OOPBase @ 0x%p\n", __func__, OOPBase);
     )

    initcustom(&LIBBASE->csd);
    GfxBase = LIBBASE->csd.cs_GfxBase;

    D(bug("[AmigaVideo] %s: GfxBase @ 0x%p\n", __func__, GfxBase);)

    LIBBASE->csd.cs_basebm = OOP_FindClass(CLID_Hidd_BitMap);
    
    Init_AmigaVideoClass(LIBBASE);
    LIBBASE->library.lib_OpenCnt = 1;

    err = AddDisplayDriver(LIBBASE->csd.amigagfxclass, NULL,
			   DDRV_KeepBootMode, TRUE,
			   DDRV_MonitorID   , 0,
			   DDRV_IDMask      , 0xF0000000,
			   TAG_DONE);

    D(bug("[AmigaVideo] %s: AddDisplayDriver() result: %u\n", __func__, err);)

    return err ? FALSE : TRUE;
}

ADD2INITLIB(AmigaVideo_Init, 0)
