/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#ifndef LAYOUT_H
#define LAYOUT_H

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#ifndef DOS_DOS_H
#    include <dos/dos.h>
#endif

#ifndef DOS_EXALL_H
#    include <dos/exall.h>
#endif

#ifndef GRAPHICS_RASTPORT_H
#   include <graphics/rastport.h>
#endif

#ifndef GRAPHICS_TEXT_H
#   include <graphics/text.h>
#endif

#ifndef LIBRARIES_GADTOOLS_H
#   include <libraries/gadtools.h>
#endif

/*****************************************************************************************/

struct AslBase_intern;

/*****************************************************************************************/

/* Options */

/* AROS gadgetclass checks notification loops, while AmigaOS gadgetclass does not.
   By setting USE_SAFE_NOTIFYING to 1, icclass objects will be used for connection
   between listview gadget and scrollergadget (instead of direct connection through
   gadget objects only), which is always safe, as icclass in both AmigaOS and AROS
   checks for notification loops */
   
#define USE_SAFE_NOTIFYING  	1

/*****************************************************************************************/

/* spacing and sizes for layout */

#define BORDERPROPSPACINGX 	4
#define BORDERPROPSPACINGY 	4

#define BORDERSTRINGSPACINGX 	6
#define BORDERSTRINGSPACINGY 	4

#define BORDERLVSPACINGX 	4
#define BORDERLVSPACINGY 	4

#define BORDERCYCLESPACINGX 	4

#define GADGETSPACINGX 		4
#define GADGETSPACINGY 		4

#define OUTERSPACINGX 		4
#define OUTERSPACINGY 		4

#define LABELSPACINGX 		8

#define BUTTONEXTRAWIDTH 	16
#define BUTTONEXTRAHEIGHT 	8

#define CYCLEEXTRAWIDTH  	32

#define IMAGEBUTTONEXTRAHEIGHT 	4
#define IMAGEBUTTONEXTRAWIDTH  	4
#define BORDERIMAGESPACINGX 	4

#define BORDERLVITEMSPACINGX 	2
#define BORDERLVITEMSPACINGY 	1

#define PROPSIZE 		16

#define MIN_SPACING 		10

/*****************************************************************************************/

/* defines for string edit hook */

#define STRINGCODE_CURSORUP	1
#define STRINGCODE_CURSORDOWN	2
#define STRINGCODE_NOP	    	3

/*****************************************************************************************/

/* tags */

/* ScrollerClass */
#define ASLSC_Dummy 		(PGA_Top + 100)

#define ASLSC_Inc 		(ASLSC_Dummy + 1)
#define ASLSC_Dec 		(ASLSC_Dummy + 2)
#define ASLSC_DeltaFactor   	(ASLSC_Dummy + 3)

/* ListviewClass */

#define ASLLV_Dummy 		(PGA_Top + 200)

#define ASLLV_ItemHeight	(ASLLV_Dummy + 1)
#define ASLLV_Spacing		(ASLLV_Dummy + 2)
#define ASLLV_Labels		(ASLLV_Dummy + 3)
#define ASLLV_Top		(ASLLV_Dummy + 4)
#define ASLLV_Active		(ASLLV_Dummy + 5)
#define ASLLV_CallBack		(ASLLV_Dummy + 6)
#define ASLLV_Visible		(ASLLV_Dummy + 7)
#define ASLLV_Total		(ASLLV_Dummy + 8)
#define ASLLV_DoMultiSelect	(ASLLV_Dummy + 9)
#define ASLLV_MakeVisible	(ASLLV_Dummy + 10)
#define ASLLV_ReadOnly		(ASLLV_Dummy + 11)
#define ASLLV_Font  	    	(ASLLV_Dummy + 12)
#define ASLLV_TopPixel	    	(ASLLV_Dummy + 13)
#define ASLLV_TotalPixels   	(ASLLV_Dummy + 14)
#define ASLLV_VisiblePixels 	(ASLLV_Dummy + 15)
#define ASLLV_DeltaFactor   	(ASLLV_Dummy + 16)

/* ButtonClass */

#define ASLBT_Dummy		(PGA_Top + 300)
#define ASLBT_CoolImage		(ASLBT_Dummy + 1)

/* CycleClass */

#define ASLCY_Dummy		(PGA_Top + 400)
#define ASLCY_Labels		(ASLCY_Dummy + 1)
#define ASLCY_Active		(ASLCY_Dummy + 2)
#define ASLCY_Font  	    	(ASLCY_Dummy + 3)

/* ColorPickerClass */

#define ASLCP_Dummy 	    	(PGA_Top + 500)
#define ASLCP_NumColors     	(ASLCP_Dummy + 1)
#define ASLCP_ColorTable    	(ASLCP_Dummy + 2)
#define ASLCP_Color 	    	(ASLCP_Dummy + 3)

/* FontPreviewClass */

#define ASLFP_APen  	    	IA_FGPen
#define ASLFP_BPen  	    	IA_BGPen
#define ASLFP_Style 	    	IA_Mode
#define ASLFP_Font  	    	IA_Font
#define ASLFP_SampleText    	GA_Text

/* FontStyleClass */

#define ASLFS_Style 	    	SYSIA_Style
#define ASLFS_LabelArray    	GA_Text

/*****************************************************************************************/

/* methods */

#define ASLLV_DRAW 0x202

#define ASLLVCB_OK      	0
#define ASLLVCB_UNKNOWN 	1

#define ASLLVR_NORMAL   	0
#define ASLLVR_SELECTED 	1

#define ASLLV_MAXCOLUMNS 	5
#define ASLLV_ALIGN_LEFT	0
#define ASLLV_ALIGN_RIGHT	1

/*****************************************************************************************/

struct ASLLVDrawMsg
{
    ULONG              lvdm_MethodID; /* LV_DRAW */
    struct RastPort  * lvdm_RastPort;
    struct DrawInfo  * lvdm_DrawInfo;
    struct Rectangle   lvdm_Bounds;
    ULONG              lvdm_State;
};

struct ASLLVFileReqNode
{
    struct Node 	node;
    char		*text[ASLLV_MAXCOLUMNS];
    struct DateStamp	date;
    APTR		userdata;
    ULONG		filesize;
    LONG		type;
    LONG		subtype;
    UWORD		dontfreetext;
};

/*****************************************************************************************/

#define ASLLV_FRNTYPE_DIRECTORY 1
#define ASLLV_FRNTYPE_VOLUMES   2

#define NODEPRIF_SELECTED 1
#define NODEPRIF_MULTISEL 2

#define MARK_SELECTED(x)    (((struct Node *)(x))->ln_Pri |=  NODEPRIF_SELECTED)
#define MARK_UNSELECTED(x)  (((struct Node *)(x))->ln_Pri &= ~NODEPRIF_SELECTED)
#define IS_SELECTED(x)      (((struct Node *)(x))->ln_Pri &   NODEPRIF_SELECTED)

#define MARK_DO_MULTISEL(x) (((struct Node *)(x))->ln_Pri |=  NODEPRIF_MULTISEL)
#define MARK_NO_MULTISEL(x) (((struct Node *)(x))->ln_Pri &= ~NODEPRIF_MULTISEL)
#define IS_MULTISEL(x)      (((struct Node *)(x))->ln_Pri &   NODEPRIF_MULTISEL)


/*****************************************************************************************/

/* gadget ids */

#define ID_ARROWDEC 			100
#define ID_ARROWINC			101
#define ID_MAINBUTTON_OK 		102
#define ID_MAINBUTTON_MIDDLELEFT	103
#define ID_MAINBUTTON_MIDDLERIGHT	104
#define ID_MAINBUTTON_CANCEL		105

#define SCROLLTICKER 4 /* arrow gadget -> prop gadget scroll repeat delay in 1/10 sec */

/* This command should calculate window's min size 
*/
#define LDCMD_INIT		0
#define LDCMD_WINDOWOPENED	1 
/* Layout the gadgets */
#define LDCMD_LAYOUT		2 

#define LDCMD_HANDLEEVENTS	3 
/* Cleanup anything requester specific allocated in INIT, LAYOUT or HANDLEEVENTS */
#define LDCMD_CLEANUP		4 

/* Special retuen value for HANDLEEVENTS to express that the
user has successfully requested something and hit "OK" */

#define LDRET_FINISHED 2

/*****************************************************************************************/

/* Structure for storing data between LayoutGadgets and HandleEvents type hooks */
struct LayoutData
{
    UWORD		ld_Command;
    APTR		ld_UserData;
    BOOL		ld_ScreenLocked;
    BOOL		ld_TrueColor;
    struct Gadget	*ld_GList;
    struct Window	*ld_Window;
    struct Window	*ld_Window2;
    struct Menu		*ld_Menu;
    struct Screen	*ld_Screen;
    struct DrawInfo	*ld_Dri;
    APTR		ld_VisualInfo;
    struct IntReq	*ld_IntReq;
    APTR		ld_Req;
    struct RastPort	ld_DummyRP;
	
    /* The font to use in the GUI */
    struct TextFont 	*ld_Font;
    
    /* Texattr describing the above font */
    struct TextAttr 	ld_TextAttr;

    WORD		ld_WBorLeft;
    WORD		ld_WBorTop;
    WORD		ld_WBorRight;
    WORD		ld_WBorBottom;
    
    /* Used for passing back info about minimum *inner* window size 
     * From LDCMD_INIT hook
    */
    UWORD		ld_MinWidth;
    UWORD		ld_MinHeight;
    UWORD		ld_ButWidth;
    UWORD		ld_NumButtons;
    
    /* Used for passing an inputevent to
     * LDCMD_HANDLEEVENTS hooks
    */
    struct IntuiMessage *ld_Event;

};

struct ScrollerGadget
{
    Object *prop;
    Object *arrow1;
    Object *arrow2;
#if USE_SAFE_NOTIFYING
    Object *prop_ic;
    Object *listview_ic;
#endif
};

#endif /* LAYOUT_H */
