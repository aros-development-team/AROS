
#define DEBUG 0

#define __OOP_NOATTRBASES__

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
    OOP_Object *gfxhidd;
 	
    D(bug("************************* AmigaVideo_Init ******************************\n"));

    initcustom(&LIBBASE->csd);
    Init_AmigaVideoClass(LIBBASE);
    LIBBASE->library.lib_OpenCnt = 1;
    gfxhidd = OOP_NewObject(LIBBASE->csd.amigagfxclass, NULL, NULL);
    D(bug("AMIGAGFXHIDD=0x%p\n", gfxhidd));
    if (gfxhidd) {
	ULONG err = AddDisplayDriver(gfxhidd, DDRV_KeepBootMode, TRUE, DDRV_MonitorID, 0, DDRV_IDMask, 0xF0000000, TAG_DONE);
	D(bug("AMIGAGFXHIDD AddDisplayDriver() result: %u\n", err));
	if (err) {
	    OOP_DisposeObject(gfxhidd);
	    gfxhidd = NULL;
	}
    }

    D(bug("AmigaVideo_Init=0x%p\n", gfxhidd));
    return gfxhidd ? TRUE : FALSE;
}
ADD2INITLIB(AmigaVideo_Init, 0)
