#ifndef INTUITION_MENUDECORCLASS_H
#define INTUITION_MENUDECORCLASS_H

/*
    Copyright  1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Headerfile for Intuitions' MENUDECORCLASS
    Lang: english
*/

#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

#ifndef INTUITION_IMAGECLASS_H
#   include <intuition/imageclass.h>
#endif

#ifndef INTUITION_INTUITION_H
#   include <intuition/intuition.h>
#endif

#ifndef INTUITION_SCREENS_H
#   include <intuition/screens.h>
#endif

/* Attributes for MENUDECORCLASS */
#define MDA_Dummy                   (TAG_USER + 0x22000)
#define MDA_DrawInfo                (MDA_Dummy + 1)         /* I.G */
#define MDA_Screen                  (MDA_Dummy + 2)         /* I.G */
#define MDA_TrueColorOnly           (MDA_Dummy + 3)         /* ..G */
#define MDA_UserBuffer              (MDA_Dummy + 4)         /* I.G */


/* Methods for MENUDECORCLASS */
#define MDM_Dummy                   (MDA_Dummy + 500)

#define MDM_GETDEFSIZE_SYSIMAGE     (MDM_Dummy + 1)
#define MDM_DRAW_SYSIMAGE           (MDM_Dummy + 2)
#define MDM_GETMENUSPACES           (MDM_Dummy + 3)
#define MDM_DRAWBACKGROUND          (MDM_Dummy + 4)
#define MDM_INITMENU                (MDM_Dummy + 5)
#define MDM_EXITMENU                (MDM_Dummy + 6)

struct mdpGetDefSizeSysImage
{
    STACKED ULONG           MethodID;
    STACKED BYTE            mdp_TrueColor;
    STACKED struct DrawInfo *mdp_Dri;
    STACKED struct TextFont *mdp_ReferenceFont; /* In: */
    STACKED ULONG           mdp_Which;      /* In: One of CLOSEIMAGE, SIZEIMAGE, ... */
    STACKED ULONG           mdp_SysiSize;    /* In: lowres/medres/highres */
    STACKED ULONG           *mdp_Width;      /* Out */
    STACKED ULONG           *mdp_Height;     /* Out */
    STACKED ULONG           mdp_Flags;
};

struct mdpDrawSysImage
{
    STACKED ULONG           MethodID;
    STACKED BYTE            mdp_TrueColor;
    STACKED struct DrawInfo *mdp_Dri;
    STACKED struct RastPort *mdp_RPort;
    STACKED LONG            mdp_X;
    STACKED LONG            mdp_Y;
    STACKED LONG            mdp_Width;
    STACKED LONG            mdp_Height;
    STACKED ULONG           mdp_Which;
    STACKED ULONG           mdp_State;
    STACKED ULONG           mdp_Flags;
    STACKED IPTR            mdp_UserBuffer;
};

struct mdpGetMenuSpaces
{
    STACKED LONG        MethodID;
    STACKED BYTE        mdp_TrueColor;
    STACKED LONG        mdp_InnerLeft;      /* Out */
    STACKED LONG        mdp_InnerTop;     /* Out */
    STACKED LONG        mdp_InnerRight;
    STACKED LONG        mdp_InnerBottom;
    STACKED LONG        mdp_ItemInnerLeft;
    STACKED LONG        mdp_ItemInnerTop;
    STACKED LONG        mdp_ItemInnerRight;
    STACKED LONG        mdp_ItemInnerBottom;
    STACKED LONG        mdp_MinWidth;
    STACKED LONG        mdp_MinHeight;
};

/* The sdpDrawSysImage struct in scrdecorclass.h must match this one!!! */

struct mdpDrawBackground
{
    STACKED ULONG           MethodID;
    STACKED BYTE            mdp_TrueColor;
    STACKED struct RastPort *mdp_RPort;
    STACKED LONG            mdp_X;
    STACKED LONG            mdp_Y;
    STACKED LONG            mdp_Width;
    STACKED LONG            mdp_Height;
    STACKED LONG            mdp_ItemLeft;
    STACKED LONG            mdp_ItemTop;
    STACKED LONG            mdp_ItemWidth;
    STACKED LONG            mdp_ItemHeight;
    STACKED UWORD           mdp_Flags;
    STACKED IPTR            mdp_UserBuffer;
};

struct mdpInitMenu
{
    STACKED LONG            MethodID;
    STACKED BYTE            mdp_TrueColor;
    STACKED struct RastPort *mdp_RPort;
    STACKED struct Screen   *mdp_Screen;
    STACKED ULONG           mdp_Left;
    STACKED ULONG           mdp_Top;
    STACKED ULONG           mdp_Width;
    STACKED LONG            mdp_Height;
    STACKED IPTR            mdp_UserBuffer;
    STACKED IPTR            mdp_ScreenUserBuffer;
};

struct mdpExitMenu
{
    STACKED LONG        MethodID;
    STACKED BYTE        mdp_TrueColor;
    STACKED IPTR        mdp_UserBuffer;
    
};


#define   MDP_STATE_NORMAL      0
#define   MDP_STATE_SELECTED    1
#define   MDP_STATE_DISABLED    2

#endif /* INTUITION_MENUDECORCLASS_H */
