/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef __RENDERINFO_H__
#define __RENDERINFO_H__

#ifndef INTUITION_CLASSES_H
#include <intuition/classes.h>
#endif
#ifndef INTUITION_SCREENS_H
#include <intuition/screens.h>
#endif

#include <pen.h>
#include <gdk/gdktypes.h>

#ifndef _AROS

enum _dripens {
    TEXTPEN = 0,
    SHINEPEN,
    SHADOWPEN,
    BARDETAILPEN,
    BARBLOCKPEN,
    BARTRIMPEN,
};

/* nothing to do with the original structure .. */
struct DrawInfo
{
    gulong          *dri_Pens; /* lookup table for mri_Pens */
    gulong           dri_Pixels[MPEN_COUNT+2]; /* storage for pens */    
};

#endif

/* Information on display environment */
/* located in Window instance, but all Areas have a reference to it */
struct MUI_RenderInfo
{
    Object          *mri_WindowObject;  /* valid between MUIM_Setup/MUIM_Cleanup */

#ifndef _AROS
    struct DrawInfo *mri_DrawInfo;
    gulong          *mri_Pens;
    GdkWindow       *mri_Window;        /* valid between MUIM_Show/MUIM_Hide */
    GdkGC           *mri_GC;
#define mri_RastPort mri_GC
#else
    struct Screen   *mri_Screen;        /* valid between MUIM_Setup/MUIM_Cleanup */
    struct DrawInfo *mri_DrawInfo;      /* valid between MUIM_Setup/MUIM_Cleanup */
    UWORD           *mri_Pens;          /* valid between MUIM_Setup/MUIM_Cleanup */
    struct Window   *mri_Window;        /* valid between MUIM_Show/MUIM_Hide */
    struct RastPort *mri_RastPort;      /* valid between MUIM_Show/MUIM_Hide */
#endif

    ULONG            mri_Flags;         /* valid between MUIM_Setup/MUIM_Cleanup */

    /* ... private data follows ... */
#ifndef _AROS
    GdkColormap     *mri_Colormap;
    GdkVisual       *mri_Visual;
    gint             mri_ScreenWidth;
    gint             mri_ScreenHeight;
    gulong           mri_Pixels[MPEN_COUNT]; /* storage for pens */
    GdkPixmap       *mri_PatternStipple;
    GdkRectangle     mri_ClipRect;
    GdkWindow       *mri_FocusWin[4];
    ULONG            mri_FocusPixel;
    struct DrawInfo  mri_DriHack;
#else
    struct ColorMap *mri_Colormap;
    UWORD            mri_ScreenWidth;
    UWORD            mri_ScreenHeight;
    UWORD            mri_Pixels[MPEN_COUNT]; /* storage for pens */

    /* this is for AddClipping/AddClipRegion */
    struct Region   *mri_rArray[10];
    int              mri_rCount;

    GdkRectangle     mri_ClipRect;
#endif

};

#define MUIMRI_RECTFILL (1<<0)
#define MUIMRI_TRUECOLOR (1<<1)
#define MUIMRI_THINFRAMES (1<<2)
#define MUIMRI_REFRESHMODE (1<<3)

#endif
