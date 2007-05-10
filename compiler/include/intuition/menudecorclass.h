#ifndef INTUITION_MENUDECORCLASS_H
#define INTUITION_MENUDECORCLASS_H

/*
    Copyright  1995-2001, The AROS Development Team. All rights reserved.
    $Id: menudecorclass.h 12757 2001-12-08 22:23:57Z dariusb $

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
#define MDA_Dummy		    (TAG_USER + 0x22000)
#define MDA_DrawInfo	    	    (WDA_Dummy + 1) 	    /* I.G */
#define MDA_Screen  	    	    (WDA_Dummy + 2) 	    /* I.G */
#define MDA_TrueColorOnly	    (WDA_Dummy + 3) 	    /* ..G */
#define MDA_UserBuffer              (WDA_Dummy + 4)         /* I.G */


/* Methods for MENUDECORCLASS */
#define MDM_Dummy   	    	    (MDA_Dummy + 500)

#define MDM_GETDEFSIZE_SYSIMAGE     (MDM_Dummy + 1)
#define MDM_DRAW_SYSIMAGE   	    (MDM_Dummy + 2)
#define MDM_GETMENUSPACES           (MDM_Dummy + 3)
#define MDM_DRAWBACKGROUND          (MDM_Dummy + 4)
#define MDM_INITMENU                (MDM_Dummy + 5)
#define MDM_EXITMENU                (MDM_Dummy + 6)

struct mdpGetDefSizeSysImage
{
    STACKULONG	     MethodID;
    BOOL             mdp_TrueColor;
    struct DrawInfo *mdp_Dri;
    struct TextFont *mdp_ReferenceFont; /* In: */
    STACKULONG	     mdp_Which;  	/* In: One of CLOSEIMAGE, SIZEIMAGE, ... */
    STACKULONG	     mdp_SysiSize;	/* In: lowres/medres/highres */
    STACKULONG	    *mdp_Width;  	/* Out */
    STACKULONG	    *mdp_Height; 	/* Out */
    STACKULONG	     mdp_Flags;
};

struct mdpDrawSysImage
{
    STACKULONG	     MethodID;
    BOOL             mdp_TrueColor;
    struct DrawInfo *mdp_Dri;
    struct RastPort *mdp_RPort;
    STACKLONG	     mdp_X;
    STACKLONG	     mdp_Y;
    STACKLONG	     mdp_Width;
    STACKLONG	     mdp_Height;
    STACKULONG	     mdp_Which;
    STACKULONG	     mdp_State;
    STACKULONG	     mdp_Flags;
    IPTR             mdp_UserBuffer;
};

struct mdpGetMenuSpaces
{
    STACKLONG	     MethodID;
    BOOL             mdp_TrueColor;
    STACKLONG	     mdp_InnerLeft;  	/* Out */
    STACKLONG	     mdp_InnerTop; 	/* Out */
    STACKLONG	     mdp_InnerRight;
    STACKLONG        mdp_InnerBottom;
    STACKLONG        mdp_ItemInnerLeft;
    STACKLONG        mdp_ItemInnerTop;
    STACKLONG        mdp_ItemInnerRight;
    STACKLONG        mdp_ItemInnerBottom;
    STACKLONG        mdp_MinWidth;
    STACKLONG        mdp_MinHeight;
};

/* The sdpDrawSysImage struct in scrdecorclass.h must match this one!!! */

struct mdpDrawBackground
{
    STACKULONG	     MethodID;
    BOOL             mdp_TrueColor;
    struct RastPort *mdp_RPort;
    STACKLONG	     mdp_X;
    STACKLONG	     mdp_Y;
    STACKLONG	     mdp_Width;
    STACKLONG	     mdp_Height;
    STACKLONG        mdp_ItemLeft;
    STACKLONG        mdp_ItemTop;
    STACKLONG        mdp_ItemWidth;
    STACKLONG        mdp_ItemHeight;
    STACKUWORD       mdp_Flags;
    STACKIPTR        mdp_UserBuffer;
};

struct mdpInitMenu
{
    STACKLONG	     MethodID;
    BOOL             mdp_TrueColor;
    struct RastPort *mdp_RPort;
    struct Screen   *mdp_Screen;
    STACKULONG       mdp_Left;
    STACKULONG       mdp_Top;
    STACKULONG       mdp_Width;
    STACKLONG        mdp_Height;
    STACKIPTR        mdp_UserBuffer;
    STACKIPTR        mdp_ScreenUserBuffer;
};

struct mdpExitMenu
{
    STACKLONG	     MethodID;
    BOOL             mdp_TrueColor;
    STACKIPTR        mdp_UserBuffer;
    
};


#define   MDP_STATE_NORMAL	0
#define   MDP_STATE_SELECTED    1
#define   MDP_STATE_DISABLED    2

#endif /* INTUITION_MENUDECORCLASS_H */
