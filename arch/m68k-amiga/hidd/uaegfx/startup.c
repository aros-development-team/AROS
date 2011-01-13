
#define DEBUG 0

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <graphics/driver.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/oop.h>

#include "uaegfx.h"
#include "uaegfxbitmap.h"

#include LC_LIBDEFS_FILE

BOOL Init_UAEGFXClass(LIBBASETYPEPTR LIBBASE);

static int UAEGFX_Init(LIBBASETYPEPTR LIBBASE)
{
    struct GfxBase *GfxBase;
    OOP_Object *gfxhidd;
 	
    D(bug("************************* UAEGFX_Init ******************************\n"));
    if (!Init_UAEGFXClass(LIBBASE))
    	return FALSE;
    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 41);
    if (!GfxBase)
        return FALSE;
    LIBBASE->library.lib_OpenCnt = 1;
    gfxhidd = OOP_NewObject(LIBBASE->csd.gfxclass, NULL, NULL);
    D(bug("UAEGFX=0x%p\n", gfxhidd));
    if (gfxhidd) {
	ULONG err = AddDisplayDriver(gfxhidd, DDRV_BootMode, TRUE, TAG_DONE);
	D(bug("UAEGFX AddDisplayDriver() result: %u\n", err));
	if (err) {
	    OOP_DisposeObject(gfxhidd);
	    gfxhidd = NULL;
	}
    }

    CloseLibrary(&GfxBase->LibNode);
    D(bug("UAEGFX_Init=0x%p\n", gfxhidd));
    return gfxhidd ? TRUE : FALSE;
}
ADD2INITLIB(UAEGFX_Init, 0)
