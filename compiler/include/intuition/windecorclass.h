#ifndef INTUITION_WINDECORCLASS_H
#define INTUITION_WINDECORCLASS_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: imageclass.h 12757 2001-12-08 22:23:57Z chodorowski $

    Desc: Headerfile for Intuitions' WINDECORCLASS
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

/* Attributes for WINDECORCLASS */
#define WDA_Dummy		    (TAG_USER + 0x22000)
#define WDA_DrawInfo	    	    (WDA_Dummy + 1) 	    /* I.G */
#define WDA_Screen  	    	    (WDA_Dummy + 2) 	    /* I.G */
#define WDA_TrueColorOnly	    (WDA_Dummy + 3) 	    /* ..G */


/* Methods for WINDECORCLASS */
#define WDM_Dummy   	    	    (WDA_Dummy + 500)

#define WDM_SETUP   	    	    (WDM_Dummy + 1)
#define WDM_CLEANUP 	    	    (WDM_Dummy + 2)
#define WDM_GETDEFSIZE_SYSIMAGE     (WDM_Dummy + 3)
#define WDM_DRAW_SYSIMAGE   	    (WDM_Dummy + 4)
#define WDM_DRAW_WINBORDER  	    (WDM_Dummy + 5)
#define WDM_DRAW_WINTITLE   	    (WDM_Dummy + 6)
#define WDM_LAYOUT_BORDERGADGETS    (WDM_Dummy + 7)
#define WDM_DRAW_BORDERPROPBACK     (WDM_Dummy + 8)
#define WDM_DRAW_BORDERPROPKNOB     (WDM_Dummy + 9)

struct wdpGetDefSizeSysImage
{
    STACKULONG	     MethodID;
    STACKULONG	     wdp_Which;  	/* In: One of CLOSEIMAGE, SIZEIMAGE, ... */
    STACKULONG	     wdp_SysiSize;	/* In: lowres/medres/highres */
    struct TextFont *wdp_ReferenceFont; /* In: */
    STACKULONG	    *wdp_Width;  	/* Out */
    STACKULONG	    *wdp_Height; 	/* Out */
    STACKULONG	     wdp_Flags;
    
};

/* The sdpDrawSysImage struct in scrdecorclass.h must match this one!!! */

struct wdpDrawSysImage
{
    STACKULONG	     MethodID;
    struct RastPort *wdp_RPort;
    STACKLONG	     wdp_X;
    STACKLONG	     wdp_Y;
    STACKLONG	     wdp_Width;
    STACKLONG	     wdp_Height;
    STACKULONG	     wdp_Which;
    STACKULONG	     wdp_State;
    STACKULONG	     wdp_Flags;
};

struct wdpDrawWinBorder
{
    STACKULONG	     MethodID;
    struct Window   *wdp_Window;
    struct RastPort *wdp_RPort;
    STACKULONG	     wdp_Flags;
};

struct wdpDrawWinTitle
{
    STACKULONG	     MethodID;
    struct Window   *wdp_Window;
    struct RastPort *wdp_RPort;
    STACKULONG	     wdp_TitleAlign;
    STACKULONG	     wdp_Flags;
};

struct wdpLayoutBorderGadgets
{
    STACKULONG	     MethodID;
    struct Window   *wdp_Window;
    struct Gadget   *wdp_Gadgets;
    STACKULONG	     wdp_Flags;
};

struct wdpDrawBorderPropBack
{
    STACKULONG	      MethodID;
    struct Window    *wdp_Window;
    struct RastPort  *wdp_RPort;
    struct Gadget    *wdp_Gadget;
    struct Rectangle *wdp_RenderRect;
    struct Rectangle *wdp_PropRect;
    struct Rectangle *wdp_KnobRect;
    STACKULONG	      wdp_Flags;
};

struct wdpDrawBorderPropKnob
{
    STACKULONG	      MethodID;
    struct Window    *wdp_Window;
    struct RastPort  *wdp_RPort;
    struct Gadget    *wdp_Gadget;
    struct Rectangle *wdp_RenderRect;
    struct Rectangle *wdp_PropRect;
    STACKULONG	      wdp_Flags;
};

/* WinDecor DrawWindowBorder Flags */ 
#define  WDF_DWB_TOP_ONLY   	1   /* Draw top border only */

/* WinDecor DrawWinTitle Title Align */
#define WD_DWTA_LEFT 	    	0
#define WD_DWTA_RIGHT 	    	1
#define WD_DWTA_CENTER      	2

/* WinDecor LayourBorderGadgets Flags */
#define WDF_LBG_INITIAL     	1   /* First time == During OpenWindow */
#define WDF_LBG_SYSTEMGADGET	2   /* Is a system gadget (close/depth/zoom) */
#define WDF_LBG_INGADLIST   	4   /* Gadget is already in window gadget list */
#define WDF_LBG_MULTIPLE    	8   /* There may be multiple gadgets (linked
                                       together through NextGadget. Follow it) */
/* WinDecor DrawBorderPropKnob Flags */
#define WDF_DBPK_HIT	    	1   /* Knob is hit / in use by user*/

#endif /* INTUITION_WINDECORCLASS_H */
