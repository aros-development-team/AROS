/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/****************************************************************************************/

#include <proto/dos.h>
#include <proto/alib.h>
#include <proto/png.h>

#include <aros/bigendianio.h>
#include <aros/asmcall.h>

#include "icon_intern.h"

#   define DEBUG 0
#   include <aros/debug.h>

/****************************************************************************************/

BOOL ReadIconPNG(struct DiskObject **ret, BPTR file, struct IconBase *IconBase)
{
    static STRPTR   	chunknames[] =
    {
    	"icOn",
	NULL
    };
    APTR     	    	chunkpointer[] =
    {
    	NULL,
	NULL
    };
    
    struct NativeIcon *icon;
    APTR    	    	pool;

    if (Seek(file, 0, OFFSET_BEGINNING) < 0) return FALSE;

    pool = CreatePool(MEMF_ANY | MEMF_CLEAR, 1024, 1024);
    if (!pool) return FALSE;
    
    icon = AllocPooled(pool, sizeof(struct NativeIcon));
    if (!icon)
    {
    	DeletePool(pool);
	return FALSE;
    }
    
    icon->pool = pool;
    
    
    icon->iconPNG.handle = PNG_LoadImageFH(file, chunknames, chunkpointer, TRUE);
    if (!icon->iconPNG.handle)
    {
    	FreeIconPNG(&icon->dobj, IconBase);
	return FALSE;
    }
    
    {
    	LONG width, height;
	
	PNG_GetImageInfo(icon->iconPNG.handle, &width, &height, NULL, NULL);

	icon->iconPNG.width  = width;
	icon->iconPNG.height = height;
	
	PNG_GetImageData(icon->iconPNG.handle, &icon->iconPNG.img1, NULL);
	
	#define DO(x) (&x->dobj)
	DO(icon)->do_Magic    	    = WB_DISKMAGIC;
	DO(icon)->do_Version  	    = (WB_DISKVERSION << 8) | WB_DISKREVISION;
	DO(icon)->do_Type     	    = WBPROJECT;
	DO(icon)->do_CurrentX 	    = NO_ICON_POSITION;
	DO(icon)->do_CurrentY 	    = NO_ICON_POSITION;
	DO(icon)->do_Gadget.Width   = width;
	DO(icon)->do_Gadget.Height  = height;
	DO(icon)->do_StackSize      = 0;
	
	#undef DO
	
	
    }
    
    *ret = &icon->dobj;
    
    return TRUE;
    
}

/****************************************************************************************/

VOID FreeIconPNG(struct DiskObject *dobj, struct IconBase *IconBase)
{
    if (dobj)
    {
    	struct NativeIcon *nativeicon = NATIVEICON(dobj);
    
    	if (nativeicon->iconPNG.handle)
	{
	    PNG_FreeImage(nativeicon->iconPNG.handle);
	}
	
	if (nativeicon->pool) DeletePool(nativeicon->pool);
    }    
}

/****************************************************************************************/
