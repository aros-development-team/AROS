/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <workbench/icon.h>
#include <graphics/gfx.h>
#include <proto/arossupport.h>
#include "icon_intern.h"

/*#define OUTPUT_DATA*/

struct OldDrawerData
{
    struct NewWindow dd_NewWindow;
    LONG             dd_CurrentX;
    LONG             dd_CurrentY;
};


STATIC STRPTR StrDupPooled(APTR pool, STRPTR str)
{
    char *newstr;
    if (!str) return NULL;
    newstr = AllocPooled(pool,strlen(str)+1);
    if (newstr) strcpy(newstr,str);
    return newstr;
}

STATIC UBYTE *MemDupPooled(APTR pool, UBYTE *src, ULONG size)
{
    UBYTE *newmem;
    
    newmem = AllocPooled(pool, size);
    if (newmem) memcpy(newmem, src, size);
    
    return newmem;
}

STATIC struct Image *ImageDupPooled(APTR pool, struct Image *src)
{
    struct Image *dest;

    if (!src) return NULL;

    dest = (struct Image*)AllocPooled(pool,sizeof(struct Image));
    if (dest)
    {
	int data_size = 0;
	int plane_size;
	int i;
	int plane_pick = src->PlanePick;

	*dest = *src;
	dest->NextImage = NULL;

	/* Calc the size all used planes */
	plane_size = RASSIZE(src->Width,src->Height);
	for (i=0;i<8;i++)
	{
	    if (plane_pick & 1) data_size += plane_size;
	    plane_pick >>= 1;
	}

	if ((dest->ImageData = AllocPooled(pool,data_size)))
	{
	    memcpy(dest->ImageData,src->ImageData,data_size);
#ifdef OUTPUT_DATA
	    kprintf("plane_pick = %ld\n",src->PlanePick);
	    kprintf("width = %ld\n",src->Width);
	    kprintf("height = %ld\n",src->Height);

	    for (i=0;i<data_size;i++)
	    {
		kprintf("0x%02lx,",((unsigned char*)src->ImageData)[i]);
		if (i%16 == 15) kprintf("\n");
	    }
	     kprintf("\n");
#endif
	    return dest;
	}

	/* Something failed so we fail also */
	FreePooled(pool,dest,sizeof(struct Image));
    }
    return NULL;
}

/*****************************************************************************

    NAME */
#include <clib/icon_protos.h>

	AROS_LH2(struct DiskObject *, DupDiskObjectA,

/*  SYNOPSIS */
	AROS_LHA(struct DiskObject *, icon, A0),
	AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
	struct Library *, IconBase, 25, Icon)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IconBase)

    APTR pool;
    struct NativeIcon *mem;
    struct DiskObject *dobj;

    if (!icon) return NULL;

    pool = CreatePool(0,1024,1024);
    if (!pool) return NULL;

    /* AROS doesn't need the gfx to be placed in chip memory, so we can use pools */
    mem = (struct NativeIcon *)AllocPooled(pool, sizeof(struct NativeIcon));
    if (!mem) goto fail;

    memset(mem, 0, sizeof(*mem));
    
    mem->pool = pool;
    dobj = &mem->dobj;

    /* copy the contents */
    *dobj = *icon;

    if (GetTagData(ICONDUPA_JustLoadedFromDisk, FALSE, tags) != FALSE)
    {
    	struct NativeIcon *srcnativeicon;
	
	srcnativeicon = NATIVEICON(icon);
 
    	mem->icon35 = srcnativeicon->icon35;
	
	/* Clone image data */
	
	if (srcnativeicon->icon35.img1.imagedata)
	{
	    mem->icon35.img1.imagedata = MemDupPooled(pool,
	    	    	    	    	    	      srcnativeicon->icon35.img1.imagedata,
	    	    	    	    	    	      srcnativeicon->icon35.width * srcnativeicon->icon35.height);
						      
    	    if (!mem->icon35.img1.imagedata) goto fail;
	}

	if (srcnativeicon->icon35.img2.imagedata)
	{
	    mem->icon35.img2.imagedata = MemDupPooled(pool,
	    	    	    	    	    	      srcnativeicon->icon35.img2.imagedata,
	    	    	    	    	    	      srcnativeicon->icon35.width * srcnativeicon->icon35.height);
						      
    	    if (!mem->icon35.img2.imagedata) goto fail;
	}

    	/* Clone Image Mask */
	
	if (srcnativeicon->icon35.img1.mask)
	{
	    mem->icon35.img1.mask = MemDupPooled(pool,
	    	    	    	    	    	 srcnativeicon->icon35.img1.mask,
						 RASSIZE(srcnativeicon->icon35.width, srcnativeicon->icon35.height));
	}

	if (srcnativeicon->icon35.img2.mask)
	{
	    mem->icon35.img2.mask = MemDupPooled(pool,
	    	    	    	    	    	 srcnativeicon->icon35.img2.mask,
						 RASSIZE(srcnativeicon->icon35.width, srcnativeicon->icon35.height));
	}

    	/* Clone Palette */
		
	if (srcnativeicon->icon35.img1.palette)
	{
	    mem->icon35.img1.palette = MemDupPooled(pool,
	    	    	    	    	    	    srcnativeicon->icon35.img1.palette,
						    srcnativeicon->icon35.img1.numcolors * sizeof(struct ColorRegister));
	    if (!mem->icon35.img1.palette) goto fail;
	}
		
	if (srcnativeicon->icon35.img2.palette && (srcnativeicon->icon35.img2.flags & IMAGE35F_HASPALETTE))
	{
	    mem->icon35.img2.palette = MemDupPooled(pool,
	    	    	    	    	    	    srcnativeicon->icon35.img2.palette,
						    srcnativeicon->icon35.img2.numcolors * sizeof(struct ColorRegister));
	    if (!mem->icon35.img2.palette) goto fail;
	}
	else if (srcnativeicon->icon35.img1.palette)
	{
	    /* Both images use same palette which is kept in memory only once */
	    mem->icon35.img2.palette = mem->icon35.img1.palette;
	}
	
	/* Clone PNGIcon data */
	
	mem->iconPNG = srcnativeicon->iconPNG;
	
	if (srcnativeicon->iconPNG.img1)
	{
	    mem->iconPNG.img1 = MemDupPooled(pool,
	    	    	    	    	     srcnativeicon->iconPNG.img1,
					     srcnativeicon->iconPNG.width * srcnativeicon->iconPNG.height * sizeof(ULONG));
					     
    	    if (!mem->iconPNG.img1) goto fail;
	}
	
	if (srcnativeicon->iconPNG.img2)
	{
	    mem->iconPNG.img2 = MemDupPooled(pool,
	    	    	    	    	     srcnativeicon->iconPNG.img2,
					     srcnativeicon->iconPNG.width * srcnativeicon->iconPNG.height * sizeof(ULONG));
					     
    	    if (!mem->iconPNG.img2) goto fail;
	}
	
    } /* if (GetTagData(ICONDUPA_JustLoadedFromDisk, FALSE, tags) != FALSE) */
   
#ifdef OUTPUT_DATA
    kprintf("gadgetwidth = %ld\ngadgetheight = %ld\n",dobj->do_Gadget.Width,dobj->do_Gadget.Height);
#endif

    /* and now the pointers and the rest */
#warning TODO: check for errors here

    if (dobj->do_DefaultTool)
    {
    	dobj->do_DefaultTool = StrDupPooled(pool,icon->do_DefaultTool);
	if (!dobj->do_DefaultTool) goto fail;
    }
    
    if (dobj->do_ToolWindow)
    {
    	dobj->do_ToolWindow = StrDupPooled(pool,icon->do_ToolWindow);
    	if (!dobj->do_ToolWindow) goto fail;
    }
    
    if (icon->do_DrawerData)
    {
    	LONG size;
	
	dobj->do_DrawerData = AllocPooled(pool, sizeof(struct DrawerData));
    	if (!dobj->do_DrawerData) goto fail;
	
 	if (((LONG)icon->do_Gadget.UserData > 0) &&
	    ((LONG)icon->do_Gadget.UserData <= WB_DISKREVISION))
	{
	    size = sizeof(struct DrawerData);
	}
	else
	{
	    size = sizeof(struct OldDrawerData);
	}

	memset(dobj->do_DrawerData, 0, sizeof(struct DrawerData));
	memcpy(dobj->do_DrawerData, icon->do_DrawerData, size);
    }

    /* Duplicate the image data */
    
    if (icon->do_Gadget.GadgetRender)
    {
    	dobj->do_Gadget.GadgetRender = ImageDupPooled(pool, (struct Image*)icon->do_Gadget.GadgetRender);
	if (!dobj->do_Gadget.GadgetRender) goto fail;
    }
    
    if (icon->do_Gadget.SelectRender)
    {	    
    	dobj->do_Gadget.SelectRender = ImageDupPooled(pool, (struct Image*)icon->do_Gadget.SelectRender);
	if (!dobj->do_Gadget.SelectRender) goto fail;
    }

    /* Duplicate the tool types */
    if (icon->do_ToolTypes)
    {
	int num_tts;
	/* Get number of tool types */
	for (num_tts = 0;icon->do_ToolTypes[num_tts];num_tts++);

	if ((dobj->do_ToolTypes = (char**)AllocPooled(pool,sizeof(char*)*(num_tts+1))))
	{
	    int i;
	    for (i=0;i<num_tts;i++)
	    {
		dobj->do_ToolTypes[i] = StrDupPooled(pool,icon->do_ToolTypes[i]);
		if (!dobj->do_ToolTypes[i]) goto fail;
	    }
	    dobj->do_ToolTypes[i] = NULL;
	}
	else
	{
	    goto fail;
	}
    }

    AddIconToList(mem, LB(IconBase));
    
    return dobj;

fail:
    DeletePool(pool);
    return NULL;
    
    AROS_LIBFUNC_EXIT
} /* FreeDiskObject */
