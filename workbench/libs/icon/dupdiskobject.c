/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <aros/debug.h>

#include <workbench/icon.h>
#include <graphics/gfx.h>
#include <proto/arossupport.h>
#include "icon_intern.h"

/*#define OUTPUT_DATA*/

STATIC STRPTR StrDupIcon(struct DiskObject *dobj, STRPTR str, APTR IconBase)
{
    char *newstr;
    if (!str) return NULL;
    newstr = AllocMemIcon(dobj, strlen(str)+1, MEMF_PUBLIC);
    if (newstr) strcpy(newstr,str);
    return newstr;
}

STATIC APTR MemDupIcon(struct DiskObject *dobj, CONST_APTR src, ULONG size, APTR IconBase)
{
    UBYTE *newmem;
    
    newmem = AllocMemIcon(dobj, size, MEMF_PUBLIC);
    if (newmem) CopyMem(src, newmem, size);
    
    return newmem;
}

STATIC struct Image *ImageDupIcon(struct DiskObject *dobj, struct Image *src, BOOL dupImage, BOOL dupImageData, APTR IconBase)
{
    struct Image *dest;

    if (!src) return NULL;

    if (dupImageData)
    	dupImage = TRUE;

    if (!dupImage)
    	return src;

    dest = (struct Image*)AllocMemIcon(dobj, sizeof(struct Image), MEMF_PUBLIC);
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
    #if 0
	for (i=0;i<8;i++)
	{
	    if (plane_pick & 1) data_size += plane_size;
	    plane_pick >>= 1;
	}
    #else
    	/* It seems planepick must be ignored. See drawer icon in MCC_TheBar-26.7.lha
	   which seems to contain a SelectRender image with planepick == 0, but the image
	   data is still there. */
	   
    	(void)plane_pick;
	(void)i;
	
    	data_size = plane_size * src->Depth;
    #endif

	if (!dupImageData)
	    return dest;
	
	if ((dest->ImageData = AllocMemIcon(dobj, data_size, MEMF_PUBLIC | MEMF_CHIP)))
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
	struct IconBase *, IconBase, 25, Icon)

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

    struct NativeIcon *mem;
    struct DiskObject *dobj;
    struct NativeIcon *srcnativeicon;

    if (!icon) return NULL;

    dobj = NewDiskObject(icon->do_Type);
    if (dobj == NULL)
        return NULL;

    mem = NATIVEICON(dobj);
    /* We can't cast this, since it could be a hand-build one */
    srcnativeicon = GetNativeIcon(icon, IconBase);

    /* copy the contents */
    *dobj = *icon;

    if (srcnativeicon && GetTagData(ICONDUPA_DuplicateImageData, TRUE, tags) == TRUE)
    {
    	int i;
	
        mem->ni_IsDefault = srcnativeicon->ni_IsDefault;
        mem->ni_Frameless = srcnativeicon->ni_Frameless;
        mem->ni_ScaleBox  = srcnativeicon->ni_ScaleBox;
        mem->ni_Face      = srcnativeicon->ni_Face;

        /* The duplicate will *not* be laid out to a specific screen */
        mem->ni_Screen = NULL;
        mem->ni_Width  = 0;
        mem->ni_Height = 0;

        /* Duplicate any extra data */
        if (srcnativeicon->ni_Extra.Data && srcnativeicon->ni_Extra.Size > 0) {
            mem->ni_Extra = srcnativeicon->ni_Extra;
            mem->ni_Extra.Data = MemDupIcon(dobj, srcnativeicon->ni_Extra.Data, srcnativeicon->ni_Extra.Size,IconBase);
            if (!mem->ni_Extra.Data) goto fail;
        }

        for (i = 0; i < 2; i++)
        {
            struct NativeIconImage *src, *dst;

            src = &srcnativeicon->ni_Image[i];
            dst = &mem->ni_Image[i];

            if (src->ARGB) {
                dst->ARGB = MemDupIcon(dobj, src->ARGB, srcnativeicon->ni_Face.Width * srcnativeicon->ni_Face.Height * 4, IconBase);
                if (!dst->ARGB)
                    goto fail;
            }

            dst->Pens = src->Pens;
            dst->TransparentColor = src->TransparentColor;

            /* Clone Palette, if we have pens allocated */
            if (src->Palette && src->Pens)
            {
                dst->Palette = MemDupIcon(dobj, src->Palette, src->Pens * sizeof(struct ColorRegister),IconBase);
                if (!dst->Palette) goto fail;
            }

            if (src->ImageData) {
                dst->ImageData = MemDupIcon(dobj, src->ImageData, srcnativeicon->ni_Face.Width * srcnativeicon->ni_Face.Height * sizeof(UBYTE), IconBase);
                if (!dst->ImageData) goto fail;
            }

            /* Do *not* clone the screen specific layout */
            dst->BitMap = NULL;
            dst->BitMask = NULL;
            dst->Pen = NULL;
        }
    } /* if (srcnativeicon && GetTagData(ICONDUPA_DuplicateImageData, TRUE, tags) == TRUE) */
   
#ifdef OUTPUT_DATA
    kprintf("gadgetwidth = %ld\ngadgetheight = %ld\n",dobj->do_Gadget.Width,dobj->do_Gadget.Height);
#endif

    /* and now the pointers and the rest */
    /* TODO: check for errors here */

    if (GetTagData(ICONDUPA_DuplicateDefaultTool, TRUE, tags) &&
        dobj->do_DefaultTool)
    {
    	dobj->do_DefaultTool = StrDupIcon(dobj,icon->do_DefaultTool,IconBase);
	if (!dobj->do_DefaultTool) goto fail;
    }
    
    if (GetTagData(ICONDUPA_DuplicateToolWindow, TRUE, tags) &&
        dobj->do_ToolWindow)
    {
    	dobj->do_ToolWindow = StrDupIcon(dobj,icon->do_ToolWindow,IconBase);
    	if (!dobj->do_ToolWindow) goto fail;
    }
   
    if (GetTagData(ICONDUPA_DuplicateDrawerData, TRUE, tags) &&
        icon->do_DrawerData)
    {
    	LONG size;
	
	dobj->do_DrawerData = AllocMemIcon(dobj, sizeof(struct DrawerData), MEMF_PUBLIC);
    	if (!dobj->do_DrawerData) goto fail;
	
 	if (((SIPTR)icon->do_Gadget.UserData > 0) &&
	    ((SIPTR)icon->do_Gadget.UserData <= WB_DISKREVISION))
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
    	dobj->do_Gadget.GadgetRender = ImageDupIcon(dobj, (struct Image*)icon->do_Gadget.GadgetRender,
    		GetTagData(ICONDUPA_DuplicateImages, TRUE, tags),
    		GetTagData(ICONDUPA_DuplicateImageData, TRUE, tags),IconBase);
	if (!dobj->do_Gadget.GadgetRender) goto fail;
    }
    
    if (icon->do_Gadget.SelectRender)
    {	    
    	dobj->do_Gadget.SelectRender = ImageDupIcon(dobj, (struct Image*)icon->do_Gadget.SelectRender,
    		GetTagData(ICONDUPA_DuplicateImages, TRUE, tags),
    		GetTagData(ICONDUPA_DuplicateImageData, TRUE, tags),IconBase);
	if (!dobj->do_Gadget.SelectRender) goto fail;
    }

    /* Duplicate the tool types */
    if (GetTagData(ICONDUPA_DuplicateToolTypes, TRUE, tags) &&
        icon->do_ToolTypes)
    {
	int num_tts;
	/* Get number of tool types */
	for (num_tts = 0;icon->do_ToolTypes[num_tts];num_tts++);

	if ((dobj->do_ToolTypes = (STRPTR *)AllocMemIcon(dobj,sizeof(char*)*(num_tts+1), MEMF_PUBLIC)))
	{
	    int i;
	    for (i=0;i<num_tts;i++)
	    {
		dobj->do_ToolTypes[i] = StrDupIcon(dobj,icon->do_ToolTypes[i],IconBase);
		if (!dobj->do_ToolTypes[i]) goto fail;
	    }
	    dobj->do_ToolTypes[i] = NULL;
	}
	else
	{
	    goto fail;
	}
    }

    if (GetTagData(ICONDUPA_ActivateImageData, FALSE, tags))
        LayoutIconA(dobj, LB(IconBase)->ib_Screen, NULL);

    D(bug("%s: Icon %p => %p\n", __func__, icon, dobj));
    
    return dobj;

fail:
    FreeDiskObject(dobj);
    return NULL;
    
    AROS_LIBFUNC_EXIT
} /* FreeDiskObject */
