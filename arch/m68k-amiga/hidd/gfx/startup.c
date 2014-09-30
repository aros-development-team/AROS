/*
    Copyright � 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <graphics/driver.h>
#include <graphics/gfxbase.h>
#include <hidd/graphics.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/oop.h>

#include "amigavideogfx.h"
#include "amigavideobitmap.h"
#include "chipset.h"

#include LC_LIBDEFS_FILE

int Init_AmigaVideoClass(LIBBASETYPEPTR LIBBASE);

static int AmigaVideo_Init(LIBBASETYPEPTR LIBBASE)
{
    ULONG err;
    struct Library *GfxBase;

 	
    D(bug("************************* AmigaVideo_Init ******************************\n"));

    initcustom(&LIBBASE->csd);
    GfxBase = LIBBASE->csd.cs_GfxBase;
    Init_AmigaVideoClass(LIBBASE);
    LIBBASE->library.lib_OpenCnt = 1;

    err = AddDisplayDriver(LIBBASE->csd.amigagfxclass, NULL,
			   DDRV_KeepBootMode, TRUE,
			   DDRV_MonitorID   , 0,
			   DDRV_IDMask      , 0xF0000000,
			   TAG_DONE);

    CloseLibrary(GfxBase);

    D(bug("AMIGAGFXHIDD AddDisplayDriver() result: %u\n", err));
    return err ? FALSE : TRUE;
}

ADD2INITLIB(AmigaVideo_Init, 0)
