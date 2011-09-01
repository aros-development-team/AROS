
#define DEBUG 0

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <graphics/driver.h>
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/oop.h>

#include "uaegfx.h"
#include "uaegfxbitmap.h"

#include LC_LIBDEFS_FILE

BOOL Init_UAEGFXClass(LIBBASETYPEPTR LIBBASE);

static int UAEGFX_Init(LIBBASETYPEPTR LIBBASE)
{
    ULONG err;
 	
    D(bug("************************* UAEGFX_Init ******************************\n"));

    if (!Init_UAEGFXClass(LIBBASE))
    	return FALSE;
    LIBBASE->library.lib_OpenCnt = 1;

    err = AddDisplayDriver(LIBBASE->csd.gfxclass, NULL,
			   DDRV_KeepBootMode, TRUE,
			   DDRV_IDMask      , 0xF0000000,
			   TAG_DONE);

    D(bug("UAEGFXHIDD AddDisplayDriver() result: %u\n", err));
    return err ? FALSE : TRUE;
}
ADD2INITLIB(UAEGFX_Init, 0)
