/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/graphics.h>
#include <proto/layers.h>
#include "intuition_intern.h"
#include <intuition/classusr.h>
#include <proto/alib.h>

#include <aros/macros.h>

#undef DEBUG
#define DEBUG 0
#   include <aros/debug.h>
//#include <proto/arossupport.h>

#define USE_BLTBITMAPRASTPORT 1
#define USE_FASTPLANEPICK0    1

/*****************************************************************************
 
    NAME */
#include <graphics/rastport.h>
#include <graphics/rpattr.h>
#include <intuition/intuition.h>
#include <intuition/imageclass.h>
#include <proto/intuition.h>

AROS_LH6(void, DrawImageState,

         /*  SYNOPSIS */
         AROS_LHA(struct RastPort *, rp,         A0),
         AROS_LHA(struct Image    *, image,      A1),
         AROS_LHA(LONG             , leftOffset, D0),
         AROS_LHA(LONG             , topOffset,  D1),
         AROS_LHA(ULONG            , state,      D2),
         AROS_LHA(struct DrawInfo *, drawInfo,   A2),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 103, Intuition)

/*  FUNCTION
    This function renders an image in a certain state.
 
    INPUTS
    rp - Render in this RastPort
    image - Render this image
    leftOffset, topOffset - Add this offset to the position stored in the
        image.
    state - Which state (see intuition/imageclass.h for possible
        valued).
    drawInfo - The DrawInfo from the screen.
 
    RESULT
    None.
 
    NOTES
    DrawImageState(), handles both boopsi and conventional images.
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
    INTERNALS
 
    HISTORY
    29-10-95    digulla automatically created from
                intuition_lib.fd and clib/intuition_protos.h
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    ULONG apen, bpen, drmd, penmode;

    EXTENDWORD(leftOffset);
    EXTENDWORD(topOffset);

    SANITY_CHECK(rp)
    SANITY_CHECK(image)

    if (rp->Layer) LockLayer(0,rp->Layer);

    /* Store important variables of the RastPort */
#ifdef __MORPHOS__
    GetRPAttrs(rp,RPTAG_PenMode,(ULONG)&penmode,RPTAG_APen,(ULONG)&apen,
               RPTAG_BPen,(ULONG)&bpen,RPTAG_DrMd,(ULONG)&drmd,TAG_DONE);
#else
    GetRPAttrs(rp,RPTAG_APen,(ULONG)&apen,
               RPTAG_BPen,(ULONG)&bpen,RPTAG_DrMd,(ULONG)&drmd,TAG_DONE);
#endif

    while(image)
    {
        if (image->Depth == CUSTOMIMAGEDEPTH)
        {
            struct impDraw method;

            method.MethodID = IM_DRAW;
            method.imp_RPort = rp;
            method.imp_Offset.X = leftOffset;
            method.imp_Offset.Y = topOffset;
            method.imp_State = state;
            method.imp_DrInfo = drawInfo;
            DoMethodA ((Object *)image, (Msg)&method);
        }
        else
        {
            WORD    x, y, d;
            UWORD   shift;
            ULONG   planeonoff, planepick;

    	#if !USE_BLTBITMAPRASTPORT

            ULONG   lastPen = 0;
            ULONG   pen = 0;
            UWORD   offset;
            UWORD   bitmask;
            UWORD * bits[8];
            WORD    xoff, yoff, plane;

    	#define START_BITMASK   0x8000L

            /* Change RastPort to the mode I need */
            SetDrMd (rp, JAM1);

    	#endif

            /*  kprintf("*** Drawing Image %x. Next Image = %x\n widht = %d  height = %d  depth = %d  planepick = %d  planeonoff = %d\n",
            image,image->NextImage,
            image->Width,image->Height,image->Depth,image->PlanePick,image->PlaneOnOff);*/


            planepick  = image->PlanePick;
            planeonoff = image->PlaneOnOff & ~planepick;

    	#if USE_FASTPLANEPICK0

            if (planepick == 0)
            {
                SetAPen(rp, planeonoff);
                RectFill(rp, leftOffset + image->LeftEdge,
                         topOffset  + image->TopEdge,
                         leftOffset + image->LeftEdge + image->Width  - 1,
                         topOffset  + image->TopEdge  + image->Height - 1);

                image = image->NextImage;
                continue;
            }

    	#endif

            /* Use x to store size of one image plane */
            x = ((image->Width + 15) >> 4) * image->Height;
            y = 0;

            shift = 1;

    	#if USE_BLTBITMAPRASTPORT
            {
                struct BitMap bitmap;
                int depth;

    	    #if 0
                /* The "8" (instead of image->Depth) seems to be correct,
                   as for example DOpus uses prop gadget knob images with
                   a depth of 0 (planepick 0, planeonoff color) */

                depth = 8;
    	    #else
    	    	/* That makes far more sense than just a 8
                * R.Schmidt...still doesn`t resolve some weird icon problem i have
                */
                //depth = rp->BitMap->Depth;

                /*
                ** Be system friendly. - Piru
                */
                depth = GetBitMapAttr(rp->BitMap, BMA_DEPTH);
                if (depth > 8)
                    depth = 8;
    	    #endif
                InitBitMap(&bitmap, depth, image->Width, image->Height);

                for(d = 0; d < depth; d++)
                {
                    if (planepick & shift)
                    {
                        bitmap.Planes[d] = (PLANEPTR)(image->ImageData + (y++) * x);
                    }
                    else
                    {
                        bitmap.Planes[d] = (planeonoff & shift) ? (PLANEPTR)-1 : NULL;
                    }
                    shift <<= 1;
                }

                BltBitMapRastPort(&bitmap,
                                  0, 0,
                                  rp,
                                  leftOffset + image->LeftEdge, topOffset + image->TopEdge,
                                  image->Width, image->Height,
                                  0xC0);
            }

    	#else


            for(d = 0; d < image->Depth;d++)
            {
                bits[d] = (planepick & shift) ? image->ImageData + (y++) * x : NULL;
                shift <<= 1;
            }

            offset  = 0;

            yoff = image->TopEdge + topOffset;

            for (y=0; y < image->Height; y++, yoff++)
            {

                bitmask = START_BITMASK;

                xoff = image->LeftEdge + leftOffset;

                for (x=0; x < image->Width; x++, xoff++)
                {
                    pen = planeonoff;
                    shift = 1;
                    plane = 0;

                    for(d = 0; d < image->Depth; d++)
                    {
                        if (planepick & shift)
                        {
                            UWORD dat;

                            dat = bits[d][offset];
                            dat = AROS_WORD2BE(dat);

                            if (dat & bitmask)
                            {
                                pen |= shift;
                            }
                        }

                        shift <<= 1;
                    }

                    if (!x)
                    {
                        lastPen = pen;
                        Move (rp, xoff, yoff);
                    }

                    if (pen != lastPen)
                    {
                        SetAPen (rp, lastPen);
                        Draw (rp, xoff, yoff);
                        lastPen = pen;
                    }

                    bitmask >>= 1;

                    if (!bitmask)
                    {
                        bitmask = START_BITMASK;
                        offset ++;
                    }

                } /* for (x=0; x < image->Width; x++, xoff++) */

                SetAPen (rp, pen);
                Draw (rp, xoff-1, yoff);

                if (bitmask != START_BITMASK)
                    offset ++;

            } /* for (y=0; y < image->Height; y++, yoff++) */

    	#endif
        }

        image = image->NextImage;
    }

#ifdef __MORPHOS__
    SetRPAttrs(rp,RPTAG_APen,apen,RPTAG_BPen,bpen,RPTAG_DrMd,drmd,RPTAG_PenMode,penmode,TAG_DONE);
#else
    SetRPAttrs(rp,RPTAG_APen,apen,RPTAG_BPen,bpen,RPTAG_DrMd,drmd,TAG_DONE);
#endif

    if (rp->Layer) UnlockLayer(rp->Layer);

    AROS_LIBFUNC_EXIT
} /* DrawImageState */
