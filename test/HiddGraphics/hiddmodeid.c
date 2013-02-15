#include <graphics/gfxbase.h>
#include <intuition/screens.h>
#include <hidd/graphics.h>
#include <proto/intuition.h>
#include <proto/oop.h>

#include <stdio.h>

OOP_AttrBase HiddBitMapAttrBase;
OOP_AttrBase HiddSyncAttrBase;

static void PrintMode(HIDDT_ModeID m, OOP_Object *sync, OOP_Object *pixfmt)
{
    STRPTR desc = "No description";

    if (m == vHidd_ModeID_Invalid)
        desc = "End of list";
    else {
        if (sync) {
	    OOP_GetAttr(sync, aHidd_Sync_Description, (IPTR *)&desc);
        } else
	    desc = "NULL sync";
    }

    printf("ModeID: 0x%08lX %s\n", m, desc);
}

int main(void)
{
    struct Screen *scr;
    struct BitMap *bm;

    HiddBitMapAttrBase = OOP_ObtainAttrBase(IID_Hidd_BitMap);
    if (!HiddBitMapAttrBase) {
        printf("Failed to obtain IID_Hidd_BitMap\n");
	return RETURN_FAIL;
    }

    HiddSyncAttrBase = OOP_ObtainAttrBase(IID_Hidd_Sync);
    if (!HiddSyncAttrBase) {
        printf("Failed to obtain IID_Hidd_Sync\n");
	OOP_ReleaseAttrBase(IID_Hidd_BitMap);
	return RETURN_FAIL;
    }

    scr = LockPubScreen(NULL);

    if (!scr) {
        printf("Failed to lock default public screen\n");
	OOP_ReleaseAttrBase(IID_Hidd_Sync);
	OOP_ReleaseAttrBase(IID_Hidd_BitMap);
	return RETURN_FAIL;
    }

    bm = scr->RastPort.BitMap;

    if (IS_HIDD_BM(bm)) { 
        OOP_Object *bmobj = HIDD_BM_OBJ(bm);
	OOP_Object *gfxhidd = NULL;

	OOP_GetAttr(bmobj, aHidd_BitMap_GfxHidd, (IPTR *)&gfxhidd);

	if (gfxhidd) {
	    HIDDT_ModeID *modes;
	    HIDDT_ModeID mode;
	    OOP_Object *sync, *pixfmt;

	    printf("Checking QueryModeIDs()...\n");
	    modes = HIDD_Gfx_QueryModeIDs(gfxhidd, NULL);
	    if (modes) {
	        HIDDT_ModeID *m = modes;
	        
		do {
		    sync = NULL;
		    pixfmt = NULL;
		    if ((*m == vHidd_ModeID_Invalid) ||
		        HIDD_Gfx_GetMode(gfxhidd, *m, &sync, &pixfmt))
		        PrintMode(*m, sync, pixfmt);
		    else
		        printf("ModeID 0x%08lX GetMode() failed\n", *m);
	        } while (*m++ != vHidd_ModeID_Invalid);

		HIDD_Gfx_ReleaseModeIDs(gfxhidd, modes);
	    } else
	        printf("Failed to obtain ModeID list\n");

	    printf("Checking NextModeID()...\n");
	    mode = vHidd_ModeID_Invalid;
	    
	    do {
	        sync = NULL;
		pixfmt = NULL;
	        mode = HIDD_Gfx_NextModeID(gfxhidd, mode, &sync, &pixfmt);
		PrintMode(mode, sync, pixfmt);
	    } while (mode != vHidd_ModeID_Invalid);
	    
	} else
	    printf("Public screen bitmap does not have aHidd_BitMap_GfxHidd, weird\n");
    } else
        printf("Public screen bitmap is not a HIDD bitmap, unsupported for now\n");

    UnlockPubScreen(NULL, scr);
    OOP_ReleaseAttrBase(IID_Hidd_Sync);
    OOP_ReleaseAttrBase(IID_Hidd_BitMap);
    return 0;
}
