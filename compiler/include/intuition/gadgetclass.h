#ifndef INTUITION_GADGETCLASS_H
#define INTUITION_GADGETCLASS_H

/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Headerfile for Intuitions' GadgetClass and ButtonGClass
    Lang: english
*/
#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef INTUITION_INTUITION_H
#   include <intuition/intuition.h>
#endif
#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

#ifdef _AMIGA
#   define STCKWORD	WORD
#   define STCKULONG	ULONG
#else
#   define STCKWORD	int
#   define STCKULONG	unsigned long
#endif

/* GadgetClass attributes */
#define GA_Dummy	    (TAG_USER +0x30000)
#define GA_Left 	    (GA_Dummy + 0x0001)
#define GA_RelRight	    (GA_Dummy + 0x0002)
#define GA_Top		    (GA_Dummy + 0x0003)
#define GA_RelBottom	    (GA_Dummy + 0x0004)
#define GA_Width	    (GA_Dummy + 0x0005)
#define GA_RelWidth	    (GA_Dummy + 0x0006)
#define GA_Height	    (GA_Dummy + 0x0007)
#define GA_RelHeight	    (GA_Dummy + 0x0008)
#define GA_Text 	    (GA_Dummy + 0x0009) /* ti_Data is (UBYTE *) */
#define GA_Image	    (GA_Dummy + 0x000A)
#define GA_Border	    (GA_Dummy + 0x000B)
#define GA_SelectRender     (GA_Dummy + 0x000C)
#define GA_Highlight	    (GA_Dummy + 0x000D)
#define GA_Disabled	    (GA_Dummy + 0x000E)
#define GA_GZZGadget	    (GA_Dummy + 0x000F)
#define GA_ID		    (GA_Dummy + 0x0010)
#define GA_UserData	    (GA_Dummy + 0x0011)
#define GA_SpecialInfo	    (GA_Dummy + 0x0012)
#define GA_Selected	    (GA_Dummy + 0x0013)
#define GA_EndGadget	    (GA_Dummy + 0x0014)
#define GA_Immediate	    (GA_Dummy + 0x0015)
#define GA_RelVerify	    (GA_Dummy + 0x0016)
#define GA_FollowMouse	    (GA_Dummy + 0x0017)
#define GA_RightBorder	    (GA_Dummy + 0x0018)
#define GA_LeftBorder	    (GA_Dummy + 0x0019)
#define GA_TopBorder	    (GA_Dummy + 0x001A)
#define GA_BottomBorder     (GA_Dummy + 0x001B)
#define GA_ToggleSelect     (GA_Dummy + 0x001C)
#define GA_SysGadget	    (GA_Dummy + 0x001D) /* internal */
#define GA_SysGType	    (GA_Dummy + 0x001E) /* internal */
#define GA_Previous	    (GA_Dummy + 0x001F) /* Previous Gadget [I.G] */
#define GA_Next 	    (GA_Dummy + 0x0020)
#define GA_DrawInfo	    (GA_Dummy + 0x0021) /* Some Gadgets need this */

/* You should use at most ONE of GA_Text, GA_IntuiText, and GA_LabelImage */
#define GA_IntuiText	    (GA_Dummy + 0x0022) /* ti_Data is (struct IntuiText *) */
#define GA_LabelImage	    (GA_Dummy + 0x0023) /* ti_Data is an image (object) */

#define GA_TabCycle	    (GA_Dummy + 0x0024) /* BOOL: Handle Tab/Shift-Tab */
#define GA_GadgetHelp	    (GA_Dummy + 0x0025) /* Send GADGETHELP message */
#define GA_Bounds	    (GA_Dummy + 0x0026) /* struct IBox * */
#define GA_RelSpecial	    (GA_Dummy + 0x0027) /* Special handling in GM_LAYOUT */

/* PROPGCLASS attributes */
#define PGA_Dummy	    (TAG_USER + 0x31000)
#define PGA_Freedom	    (PGA_Dummy + 0x0001) /* only one of FREEVERT or FREEHORIZ */
#define PGA_Borderless	    (PGA_Dummy + 0x0002)
#define PGA_HorizPot	    (PGA_Dummy + 0x0003)
#define PGA_HorizBody	    (PGA_Dummy + 0x0004)
#define PGA_VertPot	    (PGA_Dummy + 0x0005)
#define PGA_VertBody	    (PGA_Dummy + 0x0006)
#define PGA_Total	    (PGA_Dummy + 0x0007)
#define PGA_Visible	    (PGA_Dummy + 0x0008)
#define PGA_Top 	    (PGA_Dummy + 0x0009)
#define PGA_NewLook	    (PGA_Dummy + 0x000A)

/* STRGCLASS attributes */
#define STRINGA_Dummy	    (TAG_USER      +0x32000)
#define STRINGA_MaxChars    (STRINGA_Dummy + 0x0001)
#define STRINGA_Buffer	    (STRINGA_Dummy + 0x0002)
#define STRINGA_UndoBuffer  (STRINGA_Dummy + 0x0003)
#define STRINGA_WorkBuffer  (STRINGA_Dummy + 0x0004)
#define STRINGA_BufferPos   (STRINGA_Dummy + 0x0005)
#define STRINGA_DispPos     (STRINGA_Dummy + 0x0006)
#define STRINGA_AltKeyMap   (STRINGA_Dummy + 0x0007)
#define STRINGA_Font	    (STRINGA_Dummy + 0x0008)
#define STRINGA_Pens	    (STRINGA_Dummy + 0x0009)
#define STRINGA_ActivePens  (STRINGA_Dummy + 0x000A)
#define STRINGA_EditHook    (STRINGA_Dummy + 0x000B)
#define STRINGA_EditModes   (STRINGA_Dummy + 0x000C)

/* booleans */
#define STRINGA_ReplaceMode	(STRINGA_Dummy + 0x000D)
#define STRINGA_FixedFieldMode	(STRINGA_Dummy + 0x000E)
#define STRINGA_NoFilterMode	(STRINGA_Dummy + 0x000F)
#define STRINGA_Justification	(STRINGA_Dummy + 0x0010) /* GACT_STRINGCENTER,
							    GACT_STRINGLEFT,
							    GACT_STRINGRIGHT */
#define STRINGA_LongVal 	(STRINGA_Dummy + 0x0011)
#define STRINGA_TextVal 	(STRINGA_Dummy + 0x0012)
#define STRINGA_ExitHelp	(STRINGA_Dummy + 0x0013) /* Exit on "Help" */

#define SG_DEFAULTMAXCHARS	(128)

/* Gadget Layout related attributes	*/
#define LAYOUTA_Dummy		(TAG_USER  + 0x38000)
#define LAYOUTA_LayoutObj	(LAYOUTA_Dummy + 0x0001)
#define LAYOUTA_Spacing 	(LAYOUTA_Dummy + 0x0002)
#define LAYOUTA_Orientation	(LAYOUTA_Dummy + 0x0003)

/* orientation values	*/
#define LORIENT_NONE	0
#define LORIENT_HORIZ	1
#define LORIENT_VERT	2


/* Gadget Method ID's   */
#define GM_HITTEST	(0)     /* return GMR_GADGETHIT if you are hit
				   (works in disabled state, too). */
#define GM_RENDER	(1)     /* draw yourself in the right state */
#define GM_GOACTIVE	(2)     /* You will receive input */
#define GM_HANDLEINPUT	(3)     /* handle input */
#define GM_GOINACTIVE	(4)     /* no more input */
#define GM_HELPTEST	(5)     /* Will you send gadget help if the mouse is
				   at the specified coordinates?  See below
				   for possible GMR_ values. */
#define GM_LAYOUT	(6)     /* re-evaluate your size based on the GadgetInfo
				   Domain. Do NOT re-render yourself yet, you
				   will be called when it is time... */

/* Parameter "Messages" passed to gadget class methods  */

/* GM_HITTEST and GM_HELPTEST send this message.
 * For GM_HITTEST, gpht_Mouse are coordinates relative to the gadget
 * select box.	For GM_HELPTEST, the coordinates are relative to
 * the gadget bounding box (which defaults to the select box).
 */
struct gpHitTest
{
    ULONG		MethodID;
    struct GadgetInfo  *gpht_GInfo;
    struct
    {
	STCKWORD X;
	STCKWORD Y;
    }			gpht_Mouse;
};

/* For GM_HITTEST, return GMR_GADGETHIT if you were indeed hit,
 * otherwise return zero.
 *
 * For GM_HELPTEST, return GMR_NOHELPHIT (zero) if you were not hit.
 * Typically, return GMR_HELPHIT if you were hit.
 * It is possible to pass a UWORD to the application via the Code field
 * of the IDCMP_GADGETHELP message.  Return GMR_HELPCODE or'd with
 * the UWORD-sized result you wish to return.
 *
 * GMR_HELPHIT yields a Code value of ((UWORD) ~0), which should
 * mean "nothing particular" to the application.
 */

#define GMR_GADGETHIT	(0x00000004)    /* GM_HITTEST hit */

#define GMR_NOHELPHIT	(0x00000000)    /* GM_HELPTEST didn't hit */
#define GMR_HELPHIT	(0xFFFFFFFF)    /* GM_HELPTEST hit, return code = ~0 */
#define GMR_HELPCODE	(0x00010000)    /* GM_HELPTEST hit, return low word as code */

/* GM_RENDER	*/
struct gpRender
{
    ULONG		MethodID;
    struct GadgetInfo  *gpr_GInfo;	/* gadget context		*/
    struct RastPort    *gpr_RPort;	/* all ready for use		*/
#ifdef _AMIGA
    LONG		gpr_Redraw;	/* might be a "highlight pass"  */
#else
    int 		gpr_Redraw;	/* might be a "highlight pass"  */
#endif
};

/* values of gpr_Redraw */
#define GREDRAW_UPDATE	(2)     /* incremental update, e.g. prop slider */
#define GREDRAW_REDRAW	(1)     /* redraw gadget        */
#define GREDRAW_TOGGLE	(0)     /* toggle highlight, if applicable      */

/* GM_GOACTIVE, GM_HANDLEINPUT	*/
struct gpInput
{
    ULONG		MethodID;
    struct GadgetInfo  *gpi_GInfo;
    struct InputEvent  *gpi_IEvent;
    LONG	       *gpi_Termination;
    struct
    {
	STCKWORD X;
	STCKWORD Y;
    }			gpi_Mouse;

    /* (V39) Pointer to TabletData structure, if this event originated
     * from a tablet which sends IESUBCLASS_NEWTABLET events, or NULL if
     * not.
     *
     * DO NOT ATTEMPT TO READ THIS FIELD UNDER INTUITION PRIOR TO V39!
     * IT WILL BE INVALID!
     */
    struct TabletData  *gpi_TabletData;
};

/* GM_HANDLEINPUT and GM_GOACTIVE  return code flags	*/
/* return GMR_MEACTIVE (0) alone if you want more input.
   Otherwise, return ONE of GMR_NOREUSE and GMR_REUSE, and optionally
   GMR_VERIFY.
*/
#define GMR_MEACTIVE	(0)
#define GMR_NOREUSE	(1 << 1)
#define GMR_REUSE	(1 << 2)
#define GMR_VERIFY	(1 << 3)        /* you MUST set gpi_Termination */

/*
    You can end activation with one of GMR_NEXTACTIVE and GMR_PREVACTIVE,
    which instructs Intuition to activate the next or previous gadget
    that has GFLG_TABCYCLE set.
*/
#define GMR_NEXTACTIVE	(1 << 4)
#define GMR_PREVACTIVE	(1 << 5)

/* GM_GOINACTIVE */
struct gpGoInactive
{
    ULONG		MethodID;
    struct GadgetInfo  *gpgi_GInfo;

    /* V37 field only!	DO NOT attempt to read under V36! */
    STCKULONG		gpgi_Abort;	/* gpgi_Abort=1 if gadget was aborted
					 * by Intuition and 0 if gadget went
					 * inactive at its own request
					 */
};


/* New for V39: Intuition sends GM_LAYOUT to any GREL_ gadget when
 * the gadget is added to the window (or when the window opens, if
 * the gadget was part of the NewWindow.FirstGadget or the WA_Gadgets
 * list), or when the window is resized.  Your gadget can set the
 * GA_RelSpecial property to get GM_LAYOUT events without Intuition
 * changing the interpretation of your gadget select box.  This
 * allows for completely arbitrary resizing/repositioning based on
 * window size.
 */
/* GM_LAYOUT */
struct gpLayout
{
    ULONG		MethodID;
    struct GadgetInfo  *gpl_GInfo;
    STCKULONG		gpl_Initial;	/* non-zero if this method was invoked
					 * during AddGList() or OpenWindow()
					 * time.  zero if this method was invoked
					 * during window resizing.
					 */
};


#endif /* INTUITION_GADGETCLASS_H */
