#ifndef INTUITION_SCRDECORCLASS_H
#define INTUITION_SCRDECORCLASS_H

/*
    Copyright  1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Headerfile for Intuitions' SCRDECORCLASS
    Lang: english
*/

#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

#ifndef INTUITION_IMAGECLASS_H
#   include <intuition/imageclass.h>
#endif

#ifndef GRAPHICS_CLIP_H
#   include <graphics/clip.h>
#endif

#ifndef INTUITION_INTUITION_H
#   include <intuition/intuition.h>
#endif

#ifndef INTUITION_SCREENS_H
#   include <intuition/screens.h>
#endif

/* Attributes for SCRDECORCLASS */
#define SDA_Dummy		    (TAG_USER + 0x22100)
#define SDA_DrawInfo	    	    (SDA_Dummy + 1) 	    /* I.G */
#define SDA_Screen  	    	    (SDA_Dummy + 2) 	    /* I.G */
#define SDA_TrueColorOnly	    (SDA_Dummy + 3) 	    /* ..G */
#define SDA_UserBuffer              (SDA_Dummy + 4)         /* I.G */

/* Methods for SCRDECORCLASS */
#define SDM_Dummy   	    	    (SDA_Dummy + 500)

#define SDM_SETUP   	    	    (SDM_Dummy + 1)
#define SDM_CLEANUP 	    	    (SDM_Dummy + 2)
#define SDM_GETDEFSIZE_SYSIMAGE     (SDM_Dummy + 3)
#define SDM_DRAW_SYSIMAGE   	    (SDM_Dummy + 4)
#define SDM_DRAW_SCREENBAR  	    (SDM_Dummy + 5)
#define SDM_LAYOUT_SCREENGADGETS    (SDM_Dummy + 6)
#define SDM_INITSCREEN              (SDM_Dummy + 7)
#define SDM_EXITSCREEN              (SDM_Dummy + 8)

struct sdpGetDefSizeSysImage
{
    STACKED ULONG	     MethodID;
    STACKED BYTE        sdp_TrueColor;
    STACKED struct DrawInfo *sdp_Dri;
    STACKED struct TextFont *sdp_ReferenceFont; /* In: */
    STACKED ULONG	     sdp_Which;  	/* In: SDEPTHIMAGE */
    STACKED ULONG	     sdp_SysiSize;	/* In: lowres/medres/highres */
    STACKED ULONG	        *sdp_Width;  	/* Out */
    STACKED ULONG	        *sdp_Height; 	/* Out */
    STACKED ULONG	     sdp_Flags;
    STACKED IPTR	     sdp_UserBuffer;
};

/* This struct must match wdpDrawSysImage struct in windecorclass.h! */

struct sdpDrawSysImage
{
    STACKED ULONG	     MethodID;
    STACKED BYTE        sdp_TrueColor;
    STACKED struct DrawInfo *sdp_Dri;
    STACKED struct RastPort *sdp_RPort;
    STACKED LONG	     sdp_X;
    STACKED LONG	     sdp_Y;
    STACKED LONG	     sdp_Width;
    STACKED LONG	     sdp_Height;
    STACKED ULONG	     sdp_Which;
    STACKED ULONG	     sdp_State;
    STACKED ULONG	     sdp_Flags;
    STACKED IPTR	     sdp_UserBuffer;
};

struct sdpDrawScreenBar
{
    STACKED ULONG	     MethodID;
    STACKED BYTE        sdp_TrueColor;
    STACKED struct DrawInfo *sdp_Dri;
    STACKED struct Layer    *sdp_Layer;
    STACKED struct RastPort *sdp_RPort;
    STACKED struct Screen   *sdp_Screen;
    STACKED ULONG	     sdp_Flags;
    STACKED IPTR	     sdp_UserBuffer;
};

struct sdpLayoutScreenGadgets
{
    STACKED ULONG	     MethodID;
    STACKED BYTE        sdp_TrueColor;
    STACKED struct DrawInfo *sdp_Dri;
    STACKED struct Layer    *sdp_Layer;
    STACKED struct Gadget   *sdp_Gadgets;
    STACKED ULONG	     sdp_Flags;
    STACKED IPTR	     sdp_UserBuffer;
};

struct sdpInitScreen
{
    STACKED ULONG	     MethodID;
    STACKED BYTE        sdp_TrueColor;
    STACKED struct DrawInfo *sdp_Dri;
    STACKED struct Screen   *sdp_Screen;
    STACKED ULONG       sdp_FontHeight;
    STACKED LONG        sdp_TitleHack;
    STACKED ULONG       sdp_BarHeight;
    STACKED ULONG       sdp_BarVBorder;
    STACKED ULONG       sdp_BarHBorder;
    STACKED ULONG       sdp_MenuVBorder;
    STACKED ULONG       spd_MenuHBorder;
    STACKED BYTE        sdp_WBorTop;
    STACKED BYTE        sdp_WBorLeft;
    STACKED BYTE        sdp_WBorRight;
    STACKED BYTE        sdp_WBorBottom;
    STACKED IPTR        sdp_UserBuffer;
};

struct sdpExitScreen
{
    STACKED ULONG       MethodID;
    STACKED BYTE        sdp_TrueColor;
    STACKED IPTR	     sdp_UserBuffer;
};
/* ScrDecor LayoutScreenGadgets Flags */
#define SDF_LSG_INITIAL     	1   /* First time == During OpenScreen */
#define SDF_LSG_SYSTEMGADGET	2   /* Is a system gadget (sdepth) */
#define SDF_LSG_INGADLIST   	4   /* Gadget is already in screen gadget list */
#define SDF_LSG_MULTIPLE    	8   /* There may be multiple gadgets (linked
                                       together through NextGadget. Follow it) */
				       
#endif /* INTUITION_SCRDECORCLASS_H */
