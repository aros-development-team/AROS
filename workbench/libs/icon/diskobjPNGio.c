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

#define DEBUG 0
#include <aros/debug.h>

#define ATTR_ICONX   	    0x80001001
#define ATTR_ICONY   	    0x80001002
#define ATTR_DRAWERX 	    0x80001003
#define ATTR_DRAWERY 	    0x80001004
#define ATTR_DRAWERWIDTH    0x80001005
#define ATTR_DRAWERHEIGHT   0x80001006
#define ATTR_DRAWERFLAGS    0x80001007
#define ATTR_UNKNOWN	    0x80001008 /* probably toolwindow? */
#define ATTR_STACKSIZE	    0x80001009
#define ATTR_DEFAULTTOOL    0x8000100a
#define ATTR_TOOLTYPE	    0x8000100b

/****************************************************************************************/

/*
     ATTR_DRAWERFLAGS: AAABC
     
     C  : 1-bit flag  : 0 = showonlyicons 1 = showallfiles
     B  : 1-bit flag  : 0 = viewastext    1 = view as icons
     AA : 2-bit value : 0 = viewbyname, 1 = viewbydata, 2 = viewbysize, 3 = viewbytype
*/

static ULONG flags_to_ddflags(ULONG flags)
{
    ULONG ret = 0;
    
    if (flags & 1)
    {
    	ret = DDFLAGS_SHOWALL;
    }
    else
    {
    	ret = DDFLAGS_SHOWICONS;
    }
    
    return ret;    
}

static ULONG flags_to_ddviewmodes(ULONG flags)
{
    ULONG ret = 0;
    
    if (flags & 2)
    {
    	ret = DDVM_BYICON;
    }
    else
    {
    	ret = (flags >> 2) + DDVM_BYNAME;
    }
    
    return ret;    
}

/****************************************************************************************/

BOOL ReadIconPNG(struct DiskObject **ret, BPTR file, struct IconBase *IconBase)
{
    static STRPTR chunknames[] =
    {
    	"icOn",
	NULL
    };
    APTR chunkpointer[] =
    {
    	NULL,
	NULL
    };
    
    struct NativeIcon  *icon;
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
	DO(icon)->do_StackSize      = AROS_STACKSIZE;
		
	if (chunkpointer[0])
	{
	    UBYTE *chunkdata;
	    ULONG  chunksize;
	    ULONG  ttnum = 0;
	    ULONG  ttarraysize = 0;
	    BOOL   ok = TRUE;
	    
	    PNG_GetChunkInfo(chunkpointer[0], &chunkdata, &chunksize);
	    
	    while(chunksize >= 4)
	    {
	    	ULONG attr;
		IPTR  val = 0;
		BOOL  need_drawerdata = FALSE;

		attr = (chunkdata[0] << 24) | (chunkdata[1] << 16) | (chunkdata[2] << 8) | chunkdata[3];
		chunksize -=4;
		chunkdata += 4;
		
		switch(attr)
		{
		    case ATTR_DRAWERX:
		    case ATTR_DRAWERY:
		    case ATTR_DRAWERWIDTH:
		    case ATTR_DRAWERHEIGHT:
		    case ATTR_DRAWERFLAGS:
		    	need_drawerdata = TRUE;
			/* Fall through */
			
		    case ATTR_ICONX:
		    case ATTR_ICONY:
		    case ATTR_STACKSIZE:
		    	if (chunksize >= 4)
			{
			    val = (chunkdata[0] << 24) | (chunkdata[1] << 16) | (chunkdata[2] << 8) | chunkdata[3];
			    chunksize -=4;
			    chunkdata += 4;			    
			}
			else
			{
			    ok = FALSE;
			}
			break;
			
		    /* case ATTR_UNKNOWN: */
		    case ATTR_DEFAULTTOOL:
		    case ATTR_TOOLTYPE:
		    	val = chunkdata;
			chunksize -= strlen((char *)val) + 1;
			chunkdata += strlen((char *)val) + 1;
			
			if (chunksize < 0)
			{
			    ok = FALSE;
			}
			break;
			
		    default:
		    	/* Unknown attribute/tag. Impossible to handle correctly
			   if we don't know if it's a string attribute or not. */
		    	ok = FALSE;
			break;
					    
		} /* switch(attr) */
		
		if (!ok) break;
		
		if (need_drawerdata && !(DO(icon)->do_DrawerData))
		{
		    DO(icon)->do_DrawerData = AllocPooled(pool, sizeof(struct DrawerData));
		    if (!(DO(icon)->do_DrawerData))
		    {
		    	ok = FALSE;
			break;
		    }
		    
		    DO(icon)->do_DrawerData->dd_NewWindow.LeftEdge = 20;
		    DO(icon)->do_DrawerData->dd_NewWindow.TopEdge  = 20;
		    DO(icon)->do_DrawerData->dd_NewWindow.Width    = 300;
		    DO(icon)->do_DrawerData->dd_NewWindow.Height   = 200;		    
		}
		
		switch(attr)
		{
		    case ATTR_ICONX:
		    	DO(icon)->do_CurrentX = val;
			break;
			
		    case ATTR_ICONY:
		    	DO(icon)->do_CurrentY = val;
			break;
			
		    case ATTR_STACKSIZE:
		    	DO(icon)->do_StackSize = val;
			break;
			
		    case ATTR_DRAWERX:
		    	DO(icon)->do_DrawerData->dd_NewWindow.LeftEdge = (WORD)val;
			break;

		    case ATTR_DRAWERY:
		    	DO(icon)->do_DrawerData->dd_NewWindow.TopEdge = (WORD)val;
			break;

		    case ATTR_DRAWERWIDTH:
		    	DO(icon)->do_DrawerData->dd_NewWindow.Width = (WORD)val;
			break;

		    case ATTR_DRAWERHEIGHT:
		    	DO(icon)->do_DrawerData->dd_NewWindow.Height = (WORD)val;
			break;
			
		    case ATTR_DRAWERFLAGS:
		    	DO(icon)->do_DrawerData->dd_Flags     = flags_to_ddflags(val);
			DO(icon)->do_DrawerData->dd_ViewModes = flags_to_ddviewmodes(val);
			break;

    	    	    case ATTR_DEFAULTTOOL:
		    	DO(icon)->do_DefaultTool = AllocPooled(pool, strlen((char *)val) + 1);
			if (DO(icon)->do_DefaultTool)
			{
			    strcpy(DO(icon)->do_DefaultTool, (char *)val);
			}
			else
			{
			    ok = FALSE;
			}
			break;
			
		    case ATTR_TOOLTYPE:
		    	ttnum++;
			if (ttarraysize < ttnum + 1)
			{
			    STRPTR old_tooltypes = DO(icon)->do_ToolTypes;
			    ULONG  old_ttarraysize = ttarraysize;
			    
			    ttarraysize += 10;
			    
			    DO(icon)->do_ToolTypes = AllocPooled(pool, ttarraysize * sizeof(APTR));
			    if (DO(icon)->do_ToolTypes)
			    {
			    	if (old_tooltypes)
				{
				    memcpy(DO(icon)->do_ToolTypes, old_tooltypes, (ttnum - 1) * sizeof(APTR));				    
				}
			    }
			    else
			    {
			    	ok = FALSE;
			    }
			    
			    if (old_tooltypes) FreePooled(pool, old_tooltypes, old_ttarraysize * sizeof(APTR));			    
			}
			
			if (!ok) break;
			
			DO(icon)->do_ToolTypes[ttnum - 1] = AllocPooled(pool, strlen((char *)val) + 1);
			if (DO(icon)->do_ToolTypes[ttnum - 1])
			{
			    strcpy(DO(icon)->do_ToolTypes[ttnum - 1], (char *)val);
			}
			else
			{
			    ok = FALSE;
			}
			break;
			
		} /* switch(attr) */
				
		if (!ok) break;
		
	    } /* while(chunksize >= 4) */
	    
	    PNG_FreeChunk(chunkpointer[0]);
	    
	    if (!ok)
	    {
    	    	D(bug("=== Failure during icOn chunk parsing ===\n"));
    	    	FreeIconPNG(&icon->dobj, IconBase);
	    	return FALSE;
	    }
	    
	    
	} /* if (chunkpointer[0]) */

	#undef DO
	
    } /**/
    
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
