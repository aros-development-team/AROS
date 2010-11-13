
#define DEBUG 1

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <graphics/driver.h>
#include <hidd/keyboard.h>
#include <hidd/mouse.h>
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
    struct GfxBase *GfxBase;
    OOP_Object *gfxhidd;
 	
    bug("************************* AmigaVideo_Init ******************************\n");
    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 41);
    if (!GfxBase)
        return FALSE;
    initcustom(&LIBBASE->csd);
    Init_AmigaVideoClass(LIBBASE);
    LIBBASE->library.lib_OpenCnt = 1;
    gfxhidd = OOP_NewObject(LIBBASE->csd.gfxclass, NULL, NULL);
    D(bug("AMIGAGFXHIDD=0x%p\n", gfxhidd));
    if (gfxhidd) {
	ULONG err = AddDisplayDriver(gfxhidd, DDRV_BootMode, TRUE, DDRV_KeepBootMode, TRUE, TAG_DONE);
	D(bug("AMIGAGFXHIDD AddDisplayDriver() result: %u\n", err));
	if (err) {
	    OOP_DisposeObject(gfxhidd);
	    gfxhidd = NULL;
	}
    }

    CloseLibrary(&GfxBase->LibNode);
    D(bug("AmigaVideo_Init=0x%p\n", gfxhidd));
    return gfxhidd ? TRUE : FALSE;
}
ADD2INITLIB(AmigaVideo_Init, 0)
