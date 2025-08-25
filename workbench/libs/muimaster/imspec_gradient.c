/*
    Copyright  2003-2025, The AROS Development Team.
    All rights reserved.

*/

#include <math.h>
#include <intuition/imageclass.h>
#include <cybergraphx/cybergraphics.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>

#include <stdio.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include "mui.h"
#include "imspec_intern.h"
#include "support.h"
#define MYDEBUG 1
#include "debug.h"

extern struct Library *MUIMasterBase;

/*************************************************************************
 Converts a gradient string to a internal image spec

 The format of the string is:
  (h|H|v|V|<angle>),<start_r>,<start_g>,<start_b>-<end_r>,<end_g>,<end_b>
 where <angle> is given decimal. The rest represents hexadecimal 32 bit
 numbers.
**************************************************************************/
BOOL zune_gradient_string_to_intern(CONST_STRPTR str,
    struct MUI_ImageSpec_intern *spec)
{
    LONG converted;
    ULONG angle;
    ULONG start_r, start_g, start_b;
    ULONG end_r, end_g, end_b;

    if (!str || !spec)
        return FALSE;

    /* Find out about the gradient angle */
    switch (str[0])
    {
    case 'H':
    case 'h':
        angle = 90;
        converted = 1;
        break;

    case 'V':
    case 'v':
        angle = 0;
        converted = 1;
        break;

    default:
        converted = StrToLong((STRPTR) str, (LONG *) & angle);
        if (converted == -1)
            return FALSE;
        break;
    }

    str += converted;

    /* convert the color information */
    if (*str != ',')
        return FALSE;
    str++;
    converted = HexToLong((STRPTR) str, &start_r);
    if (converted == -1)
        return FALSE;
    str += converted;

    if (*str != ',')
        return FALSE;
    str++;
    converted = HexToLong((STRPTR) str, &start_g);
    if (converted == -1)
        return FALSE;
    str += converted;

    if (*str != ',')
        return FALSE;
    str++;
    converted = HexToLong((STRPTR) str, &start_b);
    if (converted == -1)
        return FALSE;
    str += converted;

    if (*str != '-')
        return FALSE;
    str++;
    converted = HexToLong((STRPTR) str, &end_r);
    if (converted == -1)
        return FALSE;
    str += converted;

    if (*str != ',')
        return FALSE;
    str++;
    converted = HexToLong((STRPTR) str, &end_g);
    if (converted == -1)
        return FALSE;
    str += converted;

    if (*str != ',')
        return FALSE;
    str++;
    converted = HexToLong((STRPTR) str, &end_b);
    if (converted == -1)
        return FALSE;

    /* Fill in the spec */
    spec->u.gradient.angle = angle;

    spec->u.gradient.start_rgb[0] = start_r >> 24;
    spec->u.gradient.start_rgb[1] = start_g >> 24;
    spec->u.gradient.start_rgb[2] = start_b >> 24;

    spec->u.gradient.end_rgb[0] = end_r >> 24;
    spec->u.gradient.end_rgb[1] = end_g >> 24;
    spec->u.gradient.end_rgb[2] = end_b >> 24;

    return TRUE;
}

VOID zune_scaled_gradient_intern_to_string(struct MUI_ImageSpec_intern *
    spec, STRPTR buf)
{
    sprintf(buf, "7:%d,%08x,%08x,%08x-%08x,%08x,%08x",
        (int)spec->u.gradient.angle,
        (unsigned int)spec->u.gradient.start_rgb[0] * 0x01010101,
        (unsigned int)spec->u.gradient.start_rgb[1] * 0x01010101,
        (unsigned int)spec->u.gradient.start_rgb[2] * 0x01010101,
        (unsigned int)spec->u.gradient.end_rgb[0] * 0x01010101,
        (unsigned int)spec->u.gradient.end_rgb[1] * 0x01010101,
        (unsigned int)spec->u.gradient.end_rgb[2] * 0x01010101);
}

VOID zune_tiled_gradient_intern_to_string(struct MUI_ImageSpec_intern *spec,
    STRPTR buf)
{
    sprintf(buf, "8:%d,%08x,%08x,%08x-%08x,%08x,%08x",
        (int)spec->u.gradient.angle,
        (unsigned int)spec->u.gradient.start_rgb[0] * 0x01010101,
        (unsigned int)spec->u.gradient.start_rgb[1] * 0x01010101,
        (unsigned int)spec->u.gradient.start_rgb[2] * 0x01010101,
        (unsigned int)spec->u.gradient.end_rgb[0] * 0x01010101,
        (unsigned int)spec->u.gradient.end_rgb[1] * 0x01010101,
        (unsigned int)spec->u.gradient.end_rgb[2] * 0x01010101);
}

BOOL zune_gradientspec_setup(struct MUI_ImageSpec_intern *spec,
    struct MUI_RenderInfo *mri)
{
    spec->u.gradient.mri = mri;

    if (!(mri->mri_Flags & MUIMRI_TRUECOLOR))
    {
        ULONG *rgbptr = spec->u.gradient.start_rgb;
        ULONG *penptr = &spec->u.gradient.start_pen;
        BOOL *flagptr = &spec->u.gradient.start_pen_is_allocated;
        LONG pen, i;

        struct TagItem obp_tags[] = {
            {OBP_FailIfBad, FALSE},
            {TAG_DONE}
        };

        for (i = 0; i < 2; i++)
        {
            ULONG r = rgbptr[0] * 0x01010101;
            ULONG g = rgbptr[1] * 0x01010101;
            ULONG b = rgbptr[2] * 0x01010101;

            pen = ObtainBestPenA(mri->mri_Colormap, r, g, b, obp_tags);

            if (pen == -1)
            {
                *flagptr = FALSE;
                *penptr = FindColor(mri->mri_Colormap, r, g, b, -1);
            }
            else
            {
                *flagptr = TRUE;
                *penptr = pen;
            }

            rgbptr = spec->u.gradient.end_rgb;
            penptr = &spec->u.gradient.end_pen;
            flagptr = &spec->u.gradient.end_pen_is_allocated;
        }

    }
    else
    {
        spec->u.gradient.start_pen_is_allocated = FALSE;
        spec->u.gradient.end_pen_is_allocated = FALSE;
    }

    return TRUE;
}

void zune_gradientspec_cleanup(struct MUI_ImageSpec_intern *spec)
{
    if (spec->u.gradient.start_pen_is_allocated)
    {
        ReleasePen(spec->u.gradient.mri->mri_Colormap,
            spec->u.gradient.start_pen);
        spec->u.gradient.start_pen_is_allocated = FALSE;
    }

    if (spec->u.gradient.end_pen_is_allocated)
    {
        ReleasePen(spec->u.gradient.mri->mri_Colormap,
            spec->u.gradient.end_pen);
        spec->u.gradient.end_pen_is_allocated = FALSE;
    }

    spec->u.gradient.mri = NULL;
}
