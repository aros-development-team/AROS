#ifndef INTUITION_IMAGECLASS_H
#define INTUITION_IMAGECLASS_H

/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Headerfile for Intuitions' IMAGECLASS
    Lang: english
*/

#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

/* Image.depth for IMAGECLASS objects */
#define CUSTOMIMAGEDEPTH	(-1)

/* Macros */
#define GADGET_BOX( g ) ( (struct IBox *) &((struct Gadget *)(g))->LeftEdge )
#define IM_BOX( im )    ( (struct IBox *) &((struct Image *)(im))->LeftEdge )
#define IM_FGPEN( im )  ( (im)->PlanePick )
#define IM_BGPEN( im )  ( (im)->PlaneOnOff )

/* Pack two UWORDs into one ULONG */
#define IAM_Resolution(x,y)     ((ULONG)(((UWORD)(x))<<16 | ((UWORD)(y))))

/* Attributes for IMAGECLASS */
#define IA_Dummy		(TAG_USER + 0x20000)
#define IA_Left 		(IA_Dummy + 0x01)
#define IA_Top			(IA_Dummy + 0x02)
#define IA_Width		(IA_Dummy + 0x03)
#define IA_Height		(IA_Dummy + 0x04)
#define IA_FGPen		(IA_Dummy + 0x05) /* Alias: PlanePick */
#define IA_BGPen		(IA_Dummy + 0x06) /* Alias: PlaneOnOff */
#define IA_Data 		(IA_Dummy + 0x07) /* Image data or similar */
#define IA_LineWidth		(IA_Dummy + 0x08)
#define IA_Pens 		(IA_Dummy + 0x0E) /* UWORD pens[] with ~0 as
						     the last element */
#define IA_Resolution		(IA_Dummy + 0x0F) /* Packed UWORDs with x/y
						     resolution ala
						     DrawInfo.Resolution */


/* Not all classes support these */
#define IA_APattern		(IA_Dummy + 0x10)
#define IA_APatSize		(IA_Dummy + 0x11)
#define IA_Mode 		(IA_Dummy + 0x12)
#define IA_Font 		(IA_Dummy + 0x13)
#define IA_Outline		(IA_Dummy + 0x14)
#define IA_Recessed		(IA_Dummy + 0x15)
#define IA_DoubleEmboss 	(IA_Dummy + 0x16)
/* to specify that the interior of a frame should not be cleared */
#define IA_EdgesOnly		(IA_Dummy + 0x17)

/* SYSICLASS attributes */
#define SYSIA_Size		(IA_Dummy + 0x0B) /* See #define's below */
#define SYSIA_Depth		(IA_Dummy + 0x0C)
#define SYSIA_Which		(IA_Dummy + 0x0D) /* See #define's below */


#define SYSIA_DrawInfo		(IA_Dummy + 0x18) /* Must be specified */

#define SYSIA_ReferenceFont	(IA_Dummy + 0x19)
#define IA_SupportsDisable	(IA_Dummy + 0x1a) /* Tell intuition to
						     use IDS_*DISABLED instead
						     of own code */
#define IA_FrameType		(IA_Dummy + 0x1b) /* See FRAME_* */

/* Private AROS sysiclass tags and defines*/

#define SYSIA_WithBorder  IA_FGPen	/* default: TRUE */
#define SYSIA_Style       IA_BGPen	/* default: SYSISTYLE_NORMAL */

#define SYSISTYLE_NORMAL   0
#define SYSISTYLE_GADTOOLS 1		/* to get arrow images in gadtools look */

/* next attribute: (IA_Dummy + 0x1c) */

/* Values for SYSIA_Size */
#define SYSISIZE_MEDRES (0)
#define SYSISIZE_LOWRES (1)
#define SYSISIZE_HIRES	(2)

/* Values for SYSIA_Which */
#define DEPTHIMAGE	(0x00L) /* Window depth gadget image */
#define ZOOMIMAGE	(0x01L) /* Window zoom gadget image */
#define SIZEIMAGE	(0x02L) /* Window sizing gadget image */
#define CLOSEIMAGE	(0x03L) /* Window close gadget image */
#define SDEPTHIMAGE	(0x05L) /* Screen depth gadget image */
#define LEFTIMAGE	(0x0AL) /* Left-arrow gadget image */
#define UPIMAGE 	(0x0BL) /* Up-arrow gadget image */
#define RIGHTIMAGE	(0x0CL) /* Right-arrow gadget image */
#define DOWNIMAGE	(0x0DL) /* Down-arrow gadget image */
#define CHECKIMAGE	(0x0EL) /* GadTools checkbox image */
#define MXIMAGE 	(0x0FL) /* GadTools mutual exclude "button" image */
#define MENUCHECK	(0x10L) /* Menu checkmark image */
#define AMIGAKEY	(0x11L) /* Menu Amiga-key image */

/* Values for IA_FrameType (FrameIClass)

    FRAME_DEFAULT: The standard V37-type frame, which has thin edges.
    FRAME_BUTTON:  Standard button gadget frames, having thicker
	sides and nicely edged corners.
    FRAME_RIDGE:  A ridge such as used by standard string gadgets.
	You can recess the ridge to get a groove image.
    FRAME_ICONDROPBOX: A broad ridge which is the standard imagery
	for areas in AppWindows where icons may be dropped.
*/
#define FRAME_DEFAULT		0
#define FRAME_BUTTON		1
#define FRAME_RIDGE		2
#define FRAME_ICONDROPBOX	3


/* image message id's   */
#define IM_DRAW       0x202L  /* draw yourself, with "state" */
#define IM_HITTEST    0x203L  /* return TRUE if click hits image */
#define IM_ERASE      0x204L  /* erase yourself */
#define IM_MOVE       0x205L  /* draw new and erase old, smoothly */
#define IM_DRAWFRAME  0x206L  /* draw with specified dimensions */
#define IM_FRAMEBOX   0x207L  /* get recommended frame around some box */
#define IM_HITFRAME   0x208L  /* hittest with dimensions */
#define IM_ERASEFRAME 0x209L  /* hittest with dimensions */

/* image draw states or styles, for IM_DRAW */
#define IDS_NORMAL	     (0L)
#define IDS_SELECTED	     (1L)   /* for selected gadgets */
#define IDS_DISABLED	     (2L)   /* for disabled gadgets */
#define IDS_BUSY	     (3L)   /* for future functionality */
#define IDS_INDETERMINATE    (4L)   /* for future functionality */
#define IDS_INACTIVENORMAL   (5L)   /* normal, in inactive window border */
#define IDS_INACTIVESELECTED (6L)   /* selected, in inactive border */
#define IDS_INACTIVEDISABLED (7L)   /* disabled, in inactive border */
#define IDS_SELECTEDDISABLED (8L)   /* disabled and selected */

/* IM_FRAMEBOX	*/
struct impFrameBox
{
    STACKED ULONG	      MethodID;
    STACKED struct IBox     * imp_ContentsBox;	/* in: relative box of contents */
    STACKED struct IBox     * imp_FrameBox;	/* out: rel. box of enclosing frame */
    STACKED struct DrawInfo * imp_DrInfo;	/* May be NULL */
    STACKED ULONG	      imp_FrameFlags;
};

#define FRAMEF_SPECIFY	(1<<0)

struct impPos
{
    STACKED WORD	 X;
    STACKED WORD	 Y;
};

struct impSize
{
    STACKED WORD	 Width;
    STACKED WORD	 Height;
};

/* IM_DRAW, IM_DRAWFRAME */
struct impDraw
{
    STACKED ULONG		MethodID;
    STACKED struct RastPort    *imp_RPort;
    STACKED struct impPos	imp_Offset;
    STACKED ULONG		imp_State;
    STACKED struct DrawInfo    *imp_DrInfo;    /* May be NULL */

    /* Only valid for IM_DRAWFRAME */
    STACKED struct impSize	imp_Dimensions;
};

/* IM_ERASE, IM_ERASEFRAME	*/
/* NOTE: This is a subset of impDraw	*/
struct impErase
{
    STACKED ULONG		MethodID;
    STACKED struct RastPort    *imp_RPort;
    STACKED struct impPos	imp_Offset;

    /* Only valid for IM_ERASEFRAME */
    STACKED struct impSize	imp_Dimensions;
};

/* IM_HITTEST, IM_HITFRAME	*/
struct impHitTest {
    STACKED ULONG		MethodID;
    STACKED struct impPos	imp_Point;

    /* Only valid for IM_HITFRAME */
    STACKED struct impSize	imp_Dimensions;
};

#endif /* INTUITION_IMAGECLASS_H */
