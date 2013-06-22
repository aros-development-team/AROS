/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <intuition/diattr.h>
#include <proto/graphics.h>

#include "intuition_intern.h"

#define ARG_MASK 0x0000000F

static int GetDRIPen(struct DrawInfo *drawInfo, ULONG pen, IPTR *errorPtr, struct IntuitionBase *IntuitionBase)
{
    UWORD *pens;
    ULONG max;

    if (drawInfo)
    {
        pens = drawInfo->dri_Pens;
        max  = drawInfo->dri_NumPens;
    }
    else
    {
        pens = GetPrivIBase(IntuitionBase)->DriPens8;
        max  = NUMDRIPENS;
    }

    pen &= ARG_MASK;
    if (pen >= max)
    {
        *errorPtr = TRUE;
        return 0;
    }

    return pens[pen];
}

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

    AROS_LH3(ULONG, GetDrawInfoAttr,

/*  SYNOPSIS */
         AROS_LHA(struct DrawInfo *, drawInfo, A0),
         AROS_LHA(ULONG            , attrID, D0),
         AROS_LHA(IPTR *           , errorPtr, A1),

/*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 156, Intuition)

/*  FUNCTION
         Gets value of the specified attribute from DrawInfo object, or
         system default value of this attribute
 
    INPUTS
         drawInfo - an object pointer to query or NULL to get system default.
         attrID   - ID of the attribute you want. The following IDs are
                    currently defined:

           GDIA_Color       -
           GDIA_Pen         -
           GDIA_Version     -
           GDIA_NumPens     -
           GDIA_DirectColor -
           GDIA_Font        -
           GDIA_Depth       -
           GDIA_CheckMark   -
           GDIA_MenuKey     -
           GDIA_ResolutionX -
           GDIA_ResolutionY -

         errorPtr - an optional storage area for error indicator. You
                    can set this parameter to NULL.
 
    RESULT
         A value of the specified attribute
 
    NOTES
         This function is compatible with MorphOS
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
    INTERNALS
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT 

    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    int pen;
    IPTR err;
    struct Color32 col;

    if (!errorPtr) errorPtr = &err;
    *errorPtr = FALSE;

    switch (attrID & ~ARG_MASK)
    {
    case GDIA_Color:
        if (drawInfo)
        {
            if (!(drawInfo->dri_Flags & DRIF_DIRECTCOLOR))
            {
                *errorPtr = TRUE;
                return 0;
            }
        }

        pen = GetDRIPen(drawInfo, attrID, errorPtr, IntuitionBase);
        if (*errorPtr)
            return 0;

        if (drawInfo)
        {
            /* We have DrawInfo, decode the color using screen's ColorMap */
            GetRGB32(drawInfo->dri_Screen->ViewPort.ColorMap, pen, 1, &col.red);
        }
        else if (pen < 4)
        {
            /* System default, first 4 pens */
            col = GetPrivIBase(IntuitionBase)->Colors[pen];
        }
        else if ((pen > 16) && (pen < 20))
        {
            /* System default, pointer pens */
            col = GetPrivIBase(IntuitionBase)->Colors[pen - 9];
        }
        else
        {
            /* There's no known color value for other pens */
            return 0;
        }

        return ((col.red   & 0xFF000000) >>  8) |
               ((col.green & 0xFF000000) >> 16) |
               ( col.blue                >> 24);

    case GDIA_Pen:
        return GetDRIPen(drawInfo, attrID, errorPtr, IntuitionBase);

    case GDIA_Version:
        return drawInfo ? drawInfo->dri_Version : DRI_VERSION;

    case GDIA_NumPens:
        return drawInfo ? drawInfo->dri_NumPens : NUMDRIPENS;

    case GDIA_DirectColor:
        if (!drawInfo)
        {
            /* CHECKME: Is this correct ? */
            return FALSE;
        }

        return (drawInfo && drawInfo->dri_Flags & DRIF_DIRECTCOLOR) ? TRUE : FALSE;

    case GDIA_Font:
        return (IPTR)(drawInfo ? drawInfo->dri_Font : GfxBase->DefaultFont);

    case GDIA_Depth:
        if (!drawInfo)
        {
            /*
             * Default value for SA_Depth is OpenScreenTagList() is 1.
             * CHECKME: Is this correct ?
             */
            return 1;
        }

        return (drawInfo->dri_Version < 3) ? drawInfo->dri_Depth :
                GetPrivScreen(drawInfo->dri_Screen)->realdepth;

    case GDIA_CheckMark:
        if ((!drawInfo) || (drawInfo->dri_Version < 2))
        {
            *errorPtr = TRUE;
            return 0;
        }

        return (IPTR)drawInfo->dri_CheckMark;

    case GDIA_MenuKey:
        if ((!drawInfo) || (drawInfo->dri_Version < 2))
        {
            *errorPtr = TRUE;
            return 0;
        }

        return (IPTR)drawInfo->dri_AmigaKey;

    case GDIA_ResolutionX:
        if (!drawInfo)
        {
            *errorPtr = TRUE;
            return 0;
        }

        return drawInfo->dri_Resolution.X;

    case GDIA_ResolutionY:
        if (!drawInfo)
        {
            *errorPtr = TRUE;
            return 0;
        }

        return drawInfo->dri_Resolution.Y;
    }

    *errorPtr = TRUE;
    return 0;

    AROS_LIBFUNC_EXIT
}
