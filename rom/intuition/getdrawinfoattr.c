/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <intuition/diattr.h>
#include <proto/graphics.h>

#include "intuition_intern.h"

#define ARG_MASK 0x0000000F

#define SetResult(x) if (resultPtr) *resultPtr = x;

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

    AROS_LH3(ULONG, GetDrawInfoAttr,

/*  SYNOPSIS */
         AROS_LHA(struct DrawInfo *, drawInfo, A0),
         AROS_LHA(ULONG            , attrID, D0),
         AROS_LHA(IPTR *           , resultPtr, A1),

/*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 156, Intuition)

/*  FUNCTION
         Gets value of the specified attribute from DrawInfo object, or
         system default value (for some attributes).
 
    INPUTS
         drawInfo - an object pointer to query. It is possible to set this
                    argument to NULL when querying GDIA_Color or GDIA_Pen
                    attributes. In this case values will be retrieved from
                    system preferences.
         attrID   - ID of the attribute you want. The following IDs are
                    currently defined:

           GDIA_Color       - 0RGB value of the color corresponding to a given pen.
                              It is possible to retrieve these values only from
                              DrawInfos belonging to direct-color screens. Pen ID
                              should be ORed with attribute ID.
           GDIA_Pen         - LUT color number corresponding to a given pen.
           GDIA_Version     - Version number of the DrawInfo object.
           GDIA_DirectColor - TRUE if the DrawInfo belongs to direct-color screen. Note
                              that in case of failure it also sets success indicator to
                              FALSE.
           GDIA_NumPens     - Number of pens or colors defined in this DrawInfo object.
           GDIA_Font        - Font specified in this DrawInfo.
           GDIA_Depth       - Depth of this DrawInfo. Note that this attribute will
                              return real depth of DrawInfo's screen, however dri_Depth
                              member will contain 8 for AmigaOS(tm) compatibility.
           GDIA_ResolutionX - X resolution in ticks
           GDIA_ResolutionY - Y resolution in ticks
           GDIA_CheckMark   - A pointer to CheckMark image object for the menu.
           GDIA_MenuKey     - A pointer to Menu (Amiga) key image object for the menu.

         resultPtr - an optional storage area for success indicator. You
                     can set this parameter to NULL.
 
    RESULT
         A value of the specified attribute. resultPtr, if supplied, gets
         TRUE for success and FALSE for failure.
 
    NOTES
         This function is compatible with MorphOS
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
    INTERNALS
 
*****************************************************************************/

/*
 * MorphOS (as of v2.6) quirks which are intentionally not reproduced in this
 * implementation:
 *
 * 1. MorphOS implementation does not accept struct DrawInfo with version
      different from OS own number.
 * 2. MorphOS implementation does not check dri_NumPens when reading dri_Pens,
 *    this is clearly a bug.
 * 3. In MorphOS GDIA_DirectColor and GDIA_Depth work only if DrawInfo has
 *    attached screen, consequently returning failure with handmade DrawInfos.
 */

{
    AROS_LIBFUNC_INIT 

    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    ULONG pen;

    SetResult(TRUE);

    if (drawInfo)
    {
        /* Most of IDs make sense only with nonzero DrawInfo */
        switch (attrID)
        {
        case GDIA_Version:
            return drawInfo->dri_Version;

        case GDIA_DirectColor:
            return (drawInfo->dri_Flags & DRIF_DIRECTCOLOR) ? TRUE : FALSE;

        case GDIA_NumPens:
            return drawInfo->dri_NumPens;

        case GDIA_Font:
            return (IPTR)drawInfo->dri_Font;

        case GDIA_Depth:
            return (drawInfo->dri_Version < 3) ? drawInfo->dri_Depth :
                    GetPrivScreen(drawInfo->dri_Screen)->realdepth;

        case GDIA_CheckMark:
            if (drawInfo->dri_Version < 2)
            {
                SetResult(FALSE);
                return 0;
            }
            return (IPTR)drawInfo->dri_CheckMark;

        case GDIA_MenuKey:
            if (drawInfo->dri_Version < 2)
            {
                SetResult(FALSE);
                return 0;
            }
            return (IPTR)drawInfo->dri_AmigaKey;

        case GDIA_ResolutionX:
            return drawInfo->dri_Resolution.X;

        case GDIA_ResolutionY:
            return drawInfo->dri_Resolution.Y;
        }

        /*
         * If we are here, this should be GDIA_Color or GDIA_Pen.
         * First we need to extract pen ID.
         */
        pen = attrID & ARG_MASK;
        if (pen < drawInfo->dri_NumPens)
        {
            switch (attrID & ~ARG_MASK)
            {
            case GDIA_Color:
                /*
                 * According to original MorphOS semantics we return RGB colors
                 * only for direct-color screens.
                 */
                if (drawInfo->dri_Flags & DRIF_DIRECTCOLOR)
                {
                    ULONG col[3];

                    GetRGB32(drawInfo->dri_Screen->ViewPort.ColorMap, drawInfo->dri_Pens[pen], 1, col);
                    return ((col[0] & 0xFF000000) >>  8) |
                           ((col[1] & 0xFF000000) >> 16) |
                            (col[2]               >> 24);
                }
                break;

            case GDIA_Pen:
                return drawInfo->dri_Pens[pen];
            }
        }
    }
    else
    {
        /*
         * No DrawInfo supplied.
         * In this case we return only default colors/pens from our preferences.
         */
        pen = attrID & ARG_MASK;
        if (pen < NUMDRIPENS)
        {
            struct Color32 *col;

            /* Extract color number corresponding to the pen */
            pen = GetPrivIBase(IntuitionBase)->DriPens8[pen];

            switch (attrID & ~ARG_MASK)
            {
            case GDIA_Color:
                /*
                 * In AROS we don't have separate preferences for direct-color
                 * screens. So we now decode color number into its RGB representation
                 * according to system palette.
                 */
                if (pen < 4)
                {
                    /* First 4 colors */
                    col = &GetPrivIBase(IntuitionBase)->Colors[pen];
                }
                else if ((pen > 16) && (pen < 20))
                {
                    /* These are pointer colors, indexes 8...10 in system palette */
                    col = &GetPrivIBase(IntuitionBase)->Colors[pen - 17 + 8];
                }
                else
                {
                    /*
                     * There's no known color value for other pens.
                     * We also have 4 last colors defined, however we don't know which
                     * pens correspond to them, because this depends on screen depth.
                     * Well, after all, we are not MOS. :)
                     */
                    break;
                }

                return ((col->red   & 0xFF000000) >>  8) |
                       ((col->green & 0xFF000000) >> 16) |
                       ( col->blue                >> 24);

            case GDIA_Pen:
                return pen;
            }
        }
    }

    /*
     * Unknown IDs as well as failed conditions end up here.
     * Report error and return zero.
     */
    SetResult(FALSE);
    return 0;

    AROS_LIBFUNC_EXIT
}
