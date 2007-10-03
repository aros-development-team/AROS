#ifndef INTUITION_WINDECORCLASS_H
#define INTUITION_WINDECORCLASS_H

/*
    Copyright  1995-2001, The AROS Development Team. All rights reserved.
    $Id: windecorclass.h 12757 2001-12-08 22:23:57Z dariusb $

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
#define WDA_UserBuffer              (WDA_Dummy + 4)         /* I.G */


/* Methods for WINDECORCLASS */
#define WDM_Dummy   	    	    (WDA_Dummy + 500)

#define WDM_SETUP   	    	    (WDM_Dummy + 1)
#define WDM_CLEANUP 	    	    (WDM_Dummy + 2)
#define WDM_GETDEFSIZE_SYSIMAGE     (WDM_Dummy + 3)
#define WDM_DRAW_SYSIMAGE   	    (WDM_Dummy + 4)
#define WDM_DRAW_WINBORDER  	    (WDM_Dummy + 5)
#define WDM_LAYOUT_BORDERGADGETS    (WDM_Dummy + 6)
#define WDM_DRAW_BORDERPROPBACK     (WDM_Dummy + 7)
#define WDM_DRAW_BORDERPROPKNOB     (WDM_Dummy + 8)
#define WDM_INITWINDOW              (WDM_Dummy + 9)
#define WDM_EXITWINDOW              (WDM_Dummy + 10)
#define WDM_WINDOWSHAPE             (WDM_Dummy + 11)


struct wdpGetDefSizeSysImage
{
    STACKED ULONG	     MethodID;
    STACKED BYTE        wdp_TrueColor;
    STACKED struct DrawInfo *wdp_Dri;
    STACKED struct TextFont *wdp_ReferenceFont; /* In: */
    STACKED ULONG	     wdp_Which;  	/* In: One of CLOSEIMAGE, SIZEIMAGE, ... */
    STACKED ULONG	     wdp_SysiSize;	/* In: lowres/medres/highres */
    STACKED ULONG	        *wdp_Width;  	/* Out */
    STACKED ULONG	        *wdp_Height; 	/* Out */
    STACKED ULONG	     wdp_Flags;
    STACKED IPTR        wdp_UserBuffer;
    
};

/* The sdpDrawSysImage struct in scrdecorclass.h must match this one!!! */

struct wdpDrawSysImage
{
    STACKED ULONG	     MethodID;
    STACKED BYTE        wdp_TrueColor;
    STACKED struct DrawInfo *wdp_Dri;
    STACKED struct RastPort *wdp_RPort;
    STACKED LONG	     wdp_X;
    STACKED LONG	     wdp_Y;
    STACKED LONG	     wdp_Width;
    STACKED LONG	     wdp_Height;
    STACKED ULONG	     wdp_Which;
    STACKED ULONG	     wdp_State;
    STACKED ULONG	     wdp_Flags;
    STACKED IPTR        wdp_UserBuffer;
};

struct wdpDrawWinBorder
{
    STACKED ULONG	     MethodID;
    STACKED BYTE        wdp_TrueColor;
    STACKED struct DrawInfo *wdp_Dri;
    STACKED struct Window   *wdp_Window;
    STACKED struct RastPort *wdp_RPort;
    STACKED ULONG	     wdp_Flags;
    STACKED IPTR        wdp_UserBuffer;
};

struct wdpLayoutBorderGadgets
{
    STACKED ULONG	     MethodID;
    STACKED BYTE        wdp_TrueColor;
    STACKED struct DrawInfo *wdp_Dri;
    STACKED struct Window   *wdp_Window;
    STACKED struct Gadget   *wdp_Gadgets;
    STACKED ULONG	     wdp_Flags;
    STACKED ULONG       wdp_ExtraButtons;
    STACKED IPTR        wdp_UserBuffer;
};

struct wdpDrawBorderPropBack
{
    STACKED ULONG	      MethodID;
    STACKED BYTE         wdp_TrueColor;
    STACKED struct DrawInfo  *wdp_Dri;
    STACKED struct Window    *wdp_Window;
    STACKED struct RastPort  *wdp_RPort;
    STACKED struct Gadget    *wdp_Gadget;
    STACKED struct Rectangle *wdp_RenderRect;
    STACKED struct Rectangle *wdp_PropRect;
    STACKED struct Rectangle *wdp_KnobRect;
    STACKED ULONG	      wdp_Flags;
    STACKED IPTR         wdp_UserBuffer;
};

struct wdpDrawBorderPropKnob
{
    STACKED ULONG	      MethodID;
    STACKED BYTE         wdp_TrueColor;
    STACKED struct DrawInfo  *wdp_Dri;
    STACKED struct Window    *wdp_Window;
    STACKED struct RastPort  *wdp_RPort;
    STACKED struct Gadget    *wdp_Gadget;
    STACKED struct Rectangle *wdp_RenderRect;
    STACKED struct Rectangle *wdp_PropRect;
    STACKED ULONG	      wdp_Flags;
    STACKED IPTR         wdp_UserBuffer;
};

struct wdpInitWindow
{
    STACKED ULONG	     MethodID;
    STACKED BYTE        wdp_TrueColor;
    STACKED IPTR        wdp_UserBuffer;
    STACKED struct Screen   *wdp_Screen;
    STACKED IPTR        wdp_ScreenUserBuffer;
};

struct wdpExitWindow
{
    STACKED ULONG       MethodID;
    STACKED BYTE        wdp_TrueColor;
    STACKED IPTR	     wdp_UserBuffer;
};

struct wdpWindowShape
{
    STACKED ULONG       MethodID;
    STACKED BYTE        wdp_TrueColor;
    STACKED struct Window   *wdp_Window;
    STACKED LONG        wdp_Width;
    STACKED LONG        wdp_Height;
    STACKED IPTR        wdp_UserBuffer;
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
