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

/* Attributes for WINDECORCLASS */
#define WDA_Dummy		    (TAG_USER + 0x22000)
#define WDA_DrawInfo	    	    (WDA_Dummy + 1)
#define WDA_Screen  	    	    (WDA_Dummy + 2)


/* Methods for WINDECORCLASS */
#define WDM_Dummy   	    	    (WDA_Dummy + 500)

#define WDM_SETUP   	    	    (WDM_Dummy + 1)
#define WDM_CLEANUP 	    	    (WDM_Dummy + 2)
#define WDM_GETDEFSIZE_SYSIMAGE     (WDM_Dummy + 3)
#define WDM_DRAW_SYSIMAGE   	    (WDM_Dummy + 4)
#define WDM_DRAW_WINBORDER  	    (WDM_Dummy + 5)
#define WDM_DRAW_WINTITLE   	    (WDM_Dummy + 6)
#define WDM_LAYOUT_BORDERGADGETS    (WDM_Dummy + 7)

struct wdpGetDefSizeSysImage
{
    STACKULONG	     MethodID;
    STACKULONG	     wdp_Which;  	/* In: One of CLOSEIMAGE, SIZEIMAGE, ... */
    STACKULONG	     wdp_SysiSize;	/* In: lowres/medres/highres */
    struct TextFont *wdp_ReferenceFont; /* In: */
    ULONG   	    *wdp_Width;  	/* Out */
    ULONG   	    *wdp_Height; 	/* Out */
};

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
};

struct wdpLayoutBorderGadgets
{
    STACKULONG	     MethodID;
    struct Window   *wdp_Window;
    struct Gadget   *wdp_Gadgets;
    STACKULONG	     wdp_Flags;
};

/* WinDecor DrawWindowBorderFlags */ 
#define  WD_DWBF_TOP_ONLY   	1   /* Draw top border only */

/* WinDecor DrawWinTitle Title Align */

#define WD_DWTA_LEFT 	    	0
#define WD_DWTA_RIGHT 	    	1
#define WD_DWTA_CENTER      	2

/* WinDecor LayourBorderGadgetsFlags */
#define WD_LBGF_INITIAL     	1   /* First time == During OpenWindow */
#define WD_LBGF_SYSTEMGADGET	2   /* Is a system gadget (close/depth/zoom) */
#define WD_LBGF_INGADLIST   	4   /* Gadget is already in window gadget list */
#define WD_LBGF_MULTIPLE    	8   /* There may be multiple gadgets (linked
                                       together through NextGadget. Follow it) */

#endif /* INTUITION_WINDECORCLASS_H */
