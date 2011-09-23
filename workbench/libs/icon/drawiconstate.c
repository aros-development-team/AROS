/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <workbench/icon.h>
#include <intuition/imageclass.h>

#include "icon_intern.h"

/* Define this in order to simulate LUT screen on hi- and truecolor displays.
   Useful for debugging.
#define FORCE_LUT_ICONS */

/*****************************************************************************

    NAME */
#include <proto/icon.h>

	AROS_LH7(void, DrawIconStateA,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *,   rp,       A0),
	AROS_LHA(struct DiskObject *, icon,     A1),
	AROS_LHA(STRPTR,              label,    A2),
	AROS_LHA(LONG,                leftEdge, D0),
	AROS_LHA(LONG,                topEdge,  D1),
	AROS_LHA(ULONG,               state,    D2),
	AROS_LHA(struct TagItem *,    tags,     A3),

/*  LOCATION */
	struct IconBase *, IconBase, 27, Icon)

/*  FUNCTION
	Draw an icon like an image.
	
    INPUTS
	rp       - rastport to draw into
	icon     - the icon
	label    - label string
	leftEdge, 
	topEdge  - drawing position 
	state    - drawing state, see intuition/imageclass.h
    
    RESULT

    NOTES
	Only very limited implemented.

    EXAMPLE

    BUGS

    SEE ALSO
	intuition/imageclass.h

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    LONG wDelta,textTop,textLeft;
    LONG iconWidth = 0, iconHeight = 0;
    struct NativeIcon *nativeicon;

    IconControl(icon, ICONCTRLA_GetWidth,  (IPTR)&iconWidth,
                      ICONCTRLA_GetHeight, (IPTR)&iconHeight,
                      TAG_DONE);
    nativeicon = GetNativeIcon(icon, LB(IconBase));

    wDelta = 0;
    textTop = 0;
    textLeft = 0;

    if (label != NULL) {
        struct TextExtent extent;

        wDelta = iconWidth;
        textTop = iconHeight;

        TextExtent(rp, label, strlen(label), &extent);

        /* wDelta will be the centering offset for the icon */
        /* textLeft is the horiz offset for the text */

        if (extent.te_Width > wDelta) {
            wDelta = (extent.te_Width - wDelta) / 2;
            textLeft = 0;
        } else {
            textLeft = (wDelta - extent.te_Width) / 2;
            wDelta = 0;
        }

        /* textTop is adjusted downwards by the size of the font */
        textTop += extent.te_Height;

        Move(rp, leftEdge + textLeft, topEdge + textTop);
        Text(rp, label, strlen(label));
    }

    if (nativeicon && GfxBase)
    {
#ifndef FORCE_LUT_ICONS
	ULONG bmdepth = GetBitMapAttr(rp->BitMap, BMA_DEPTH);

	D(bug("[Icon] Target bitmap depth: %d\n", bmdepth));

        if (CyberGfxBase && (bmdepth >= 8))
	{
	    /*
	     * If it's PNG iton, and target is >8bpp, use alpha blending.
	     * There's actually no sense to layout icon for such screens.
	     * Layout is basically remapping.
	     */
	    APTR img;

	    if ((state == IDS_SELECTED) && nativeicon->iconPNG.img2)
	        img = nativeicon->iconPNG.img2;
	    else
	        img = nativeicon->iconPNG.img1;

	    D(bug("[Icon] PNG alpha image: 0x%p\n", img));

	    if (img)
	    {
            	// OLD MODE
            	WritePixelArrayAlpha(img, 0, 0, nativeicon->iconPNG.width * sizeof(ULONG),
                		     rp, leftEdge + wDelta, topEdge,
                		     iconWidth,  iconHeight, 0);

	    	return;
	    }
	}
#endif

	if (nativeicon->iconscr)
	{
	    /* Already laid out (hopefully for this screen) ? */
            struct BitMap *bm;

            if ((state == IDS_SELECTED) && nativeicon->iconbm2)
                bm = nativeicon->iconbm2;
            else
                bm = nativeicon->iconbm1;

	    if (bm)
	    {
            	BltBitMapRastPort(bm, 0, 0, rp, leftEdge + wDelta, topEdge,
                              	  iconWidth, iconHeight, 0x00C0);
            	return;
            }
        }

        if (nativeicon->icon35.img1.imagedata && GfxBase && CyberGfxBase)
	{
	    if (bmdepth >= 4)
	    {
	        struct Image35 *img;
		ULONG	    	*cgfxcoltab;
		    
		if (state == IDS_SELECTED && nativeicon->icon35.img2.imagedata)
		    img = &nativeicon->icon35.img2;
		else
		    img = &nativeicon->icon35.img1;

		if ((cgfxcoltab = AllocVecPooled(POOL, img->numcolors * sizeof(ULONG))))
		{
		    struct ColorRegister *cr;
		    WORD i;
		            
		    cr = (struct ColorRegister *)img->palette;
		    for(i = 0; i < img->numcolors; i++)
		    {
                        struct ColorRegister color = *cr;
                        
                        if (state == IDS_SELECTED)
                            ChangeToSelectedIconColor(&color);
                            
		        cgfxcoltab[i] = (color.red << 16) | (color.green << 8) | color.blue;
			cr++;
		    }
		            
		    if (img->mask)
		    {
			struct BitMap *bm = AllocBitMap(nativeicon->icon35.width, nativeicon->icon35.height,
							0, 0, rp->BitMap);

			if (bm)
			{
		            struct RastPort bmrp;

			    InitRastPort(&bmrp);
			    bmrp.BitMap = bm;
			    WriteLUTPixelArray(img->imagedata, 0, 0, nativeicon->icon35.width,
					       &bmrp, cgfxcoltab, 0, 0, nativeicon->icon35.width, nativeicon->icon35.height,
					       CTABFMT_XRGB8);
    
			    BltMaskBitMapRastPort(bm, 0, 0, rp,
			    	    leftEdge + wDelta,
			    	    topEdge,
			    	    nativeicon->icon35.width,
			    	    nativeicon->icon35.height,
			    	    0xE0, img->mask);
		    
			    DeinitRastPort(&bmrp);
        
			    FreeBitMap(bm);
                            FreeVecPooled(POOL, cgfxcoltab);
			    return;
			} /* if (bm) */ 
		    } /* if (img->mask) */

		    WriteLUTPixelArray
                    (
                        img->imagedata, 0, 0, nativeicon->icon35.width,
                        rp, cgfxcoltab,
                        leftEdge + wDelta,
                        topEdge,
                        nativeicon->icon35.width,
                        nativeicon->icon35.height,
                        CTABFMT_XRGB8
                    );

		    FreeVecPooled(POOL, cgfxcoltab);
		    return;
		} /* if (cgfxcoltab != NULL) */

	    } /* if (bmdepth >= 4) */
	} /* if (nativeicon->icon35.img1.imagedata) */
    } /* if (nativeicon && GfxBase && CyberGfxBase) */

    if (state == IDS_SELECTED && icon->do_Gadget.SelectRender)
    {
        DrawImage
        (
            rp, (struct Image *)icon->do_Gadget.SelectRender,
            leftEdge + wDelta, topEdge
        );
    }
    else if (icon->do_Gadget.GadgetRender)
    {
        if (icon->do_Gadget.Flags & GFLG_GADGIMAGE)
        {
            DrawImage
            (
                rp, (struct Image *) icon->do_Gadget.GadgetRender,
                leftEdge + wDelta, topEdge
            );
        }
    }

    AROS_LIBFUNC_EXIT
} /* DrawIconStateA() */
