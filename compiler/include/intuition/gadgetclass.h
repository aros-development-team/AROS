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
#   define STCKLONG	LONG
#else
#   define STCKWORD	int
#   define STCKULONG	unsigned long
#   define STCKLONG	long
#endif

/* GadgetClass attributes */
#define GA_Dummy	    (TAG_USER + 0x30000)
#define GA_Left 	    (GA_Dummy +  1) /* (LONG) Left edge */
#define GA_RelRight	    (GA_Dummy +  2) /* (LONG) Left=Win->Width-this-1 */
#define GA_Top		    (GA_Dummy +  3) /* (LONG) Top edge */
#define GA_RelBottom	    (GA_Dummy +  4) /* (LONG) Top=Win->Height-this-1 */
#define GA_Width	    (GA_Dummy +  5) /* (LONG) Width */
#define GA_RelWidth	    (GA_Dummy +  6) /* (LONG) Width=Win->Width-this */
#define GA_Height	    (GA_Dummy +  7) /* (LONG) Height */
#define GA_RelHeight	    (GA_Dummy +  8)
#define GA_Text 	    (GA_Dummy +  9) /* (UBYTE *) */
#define GA_Image	    (GA_Dummy + 10)
#define GA_Border	    (GA_Dummy + 11)
#define GA_SelectRender     (GA_Dummy + 12)
#define GA_Highlight	    (GA_Dummy + 13)
#define GA_Disabled	    (GA_Dummy + 14)
#define GA_GZZGadget	    (GA_Dummy + 15)
#define GA_ID		    (GA_Dummy + 16)
#define GA_UserData	    (GA_Dummy + 17)
#define GA_SpecialInfo	    (GA_Dummy + 18)
#define GA_Selected	    (GA_Dummy + 19)
#define GA_EndGadget	    (GA_Dummy + 20)
#define GA_Immediate	    (GA_Dummy + 21)
#define GA_RelVerify	    (GA_Dummy + 22)
#define GA_FollowMouse	    (GA_Dummy + 23)
#define GA_RightBorder	    (GA_Dummy + 24)
#define GA_LeftBorder	    (GA_Dummy + 25)
#define GA_TopBorder	    (GA_Dummy + 26)
#define GA_BottomBorder     (GA_Dummy + 27)
#define GA_ToggleSelect     (GA_Dummy + 28)
#define GA_SysGadget	    (GA_Dummy + 29) /* internal */
#define GA_SysGType	    (GA_Dummy + 30) /* internal */
#define GA_Previous	    (GA_Dummy + 31) /* Previous Gadget [I.G] */
#define GA_Next 	    (GA_Dummy + 32)
#define GA_DrawInfo	    (GA_Dummy + 33) /* Some Gadgets need this */

/* You should use at most ONE of GA_Text, GA_IntuiText, and GA_LabelImage */
#define GA_IntuiText	    (GA_Dummy + 34) /* ti_Data is (struct IntuiText *) */
#define GA_LabelImage	    (GA_Dummy + 35) /* ti_Data is an image (object) */

#define GA_TabCycle	    (GA_Dummy + 36) /* BOOL: Handle Tab/Shift-Tab */
#define GA_GadgetHelp	    (GA_Dummy + 37) /* Send GADGETHELP message */
#define GA_Bounds	    (GA_Dummy + 38) /* struct IBox * */
#define GA_RelSpecial	    (GA_Dummy + 39) /* Special handling in GM_LAYOUT */
#define GA_TextAttr	    (GA_Dummy + 40) /* (struct TextAttr *) */
#define GA_ReadOnly	    (GA_Dummy + 41) /* (BOOL) */


/* PROPGCLASS attributes */
#define PGA_Dummy	    (TAG_USER + 0x31000)
#define PGA_Freedom	    (PGA_Dummy +  1) /* only one of FREEVERT or FREEHORIZ */
#define PGA_Borderless	    (PGA_Dummy +  2)
#define PGA_HorizPot	    (PGA_Dummy +  3)
#define PGA_HorizBody	    (PGA_Dummy +  4)
#define PGA_VertPot	    (PGA_Dummy +  5)
#define PGA_VertBody	    (PGA_Dummy +  6)
#define PGA_Total	    (PGA_Dummy +  7)
#define PGA_Visible	    (PGA_Dummy +  8)
#define PGA_Top 	    (PGA_Dummy +  9)
#define PGA_NewLook	    (PGA_Dummy + 10)

/* STRGCLASS attributes */
#define STRINGA_Dummy	    (TAG_USER      +0x32000)
#define STRINGA_MaxChars    (STRINGA_Dummy +  1)
#define STRINGA_Buffer	    (STRINGA_Dummy +  2)
#define STRINGA_UndoBuffer  (STRINGA_Dummy +  3)
#define STRINGA_WorkBuffer  (STRINGA_Dummy +  4)
#define STRINGA_BufferPos   (STRINGA_Dummy +  5)
#define STRINGA_DispPos     (STRINGA_Dummy +  6)
#define STRINGA_AltKeyMap   (STRINGA_Dummy +  7)
#define STRINGA_Font	    (STRINGA_Dummy +  8)
#define STRINGA_Pens	    (STRINGA_Dummy +  9)
#define STRINGA_ActivePens  (STRINGA_Dummy + 10)
#define STRINGA_EditHook    (STRINGA_Dummy + 11)
#define STRINGA_EditModes   (STRINGA_Dummy + 12)

/* booleans */
#define STRINGA_ReplaceMode	(STRINGA_Dummy + 13)
#define STRINGA_FixedFieldMode	(STRINGA_Dummy + 14)
#define STRINGA_NoFilterMode	(STRINGA_Dummy + 15)
#define STRINGA_Justification	(STRINGA_Dummy + 16) /* GACT_STRINGCENTER,
							GACT_STRINGLEFT,
							GACT_STRINGRIGHT */
#define STRINGA_LongVal 	(STRINGA_Dummy + 17)
#define STRINGA_TextVal 	(STRINGA_Dummy + 18)
#define STRINGA_ExitHelp	(STRINGA_Dummy + 19) /* Exit on "Help" */

#define SG_DEFAULTMAXCHARS	(128)

/* Gadget Layout related attributes	*/
#define LAYOUTA_Dummy		(TAG_USER  + 0x38000)
#define LAYOUTA_LayoutObj	(LAYOUTA_Dummy + 1)
#define LAYOUTA_Spacing 	(LAYOUTA_Dummy + 2)
#define LAYOUTA_Orientation	(LAYOUTA_Dummy + 3)
#define LAYOUTA_ChildMaxWidth	(LAYOUTA_Dummy + 4)
#define LAYOUTA_ChildMaxHeight	(LAYOUTA_Dummy + 5)

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
#define GM_DOMAIN	(7)     /* Query the sizing requirements */

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


/*
    The GM_DOMAIN method is used to obtain the sizing requirements of an
    object for a class before ever creating an object.
*/
/* GM_DOMAIN */
struct gpDomain
{
    ULONG		 MethodID;
    struct GadgetInfo	*gpd_GInfo;
    struct RastPort	*gpd_RPort;	/* RastPort to layout for */
    STCKLONG		 gpd_Which;
    struct IBox 	 gpd_Domain;	/* Resulting domain */
    struct TagItem	*gpd_Attrs;	/* Additional attributes */
};

#define GDOMAIN_MINIMUM 	(0) /* Minimum size */
#define GDOMAIN_NOMINAL 	(1) /* Nominal size */
#define GDOMAIN_MAXIMUM 	(2) /* Maximum size */

#endif /* INTUITION_GADGETCLASS_H */
