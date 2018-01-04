#ifndef INTUITION_GADGETCLASS_H
#define INTUITION_GADGETCLASS_H

/*
    Copyright © 1995-2018, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Header file for Intuition's gadget classes.
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

/**********************************************************************
 ***************************** GadgetClass ****************************
 **********************************************************************/

/* GadgetClass is the topmost class for all other gadget classes. In the
   current implementation it subclasses RootClass directly, but this may change
   in the future.

   GadgetClass does not perform any rendering or input handling itself, its
   subclasses are responsible for doing that. The only thing GadgetClass does,
   is preparing the structure Gadget (see <intuition/intuition.h>), which is
   supplied as immediate data at the address, the object pointer points to. In
   other words, objects of GadgetClass are struct Gadget pointers. */

/* *************************** Attributes *****************************/

/* Most subclasses of GadgetClass implement a subset of these attributes. Note
   also that even if an attribute is signed as settable, some subclasses may
   ignore this or even behave strange, if such an attribute is set, after they
   were added to a window. Read the class documentation of the subclasses to
   learn about such caveats.

   Many of these attributes correspond directly to a field of the Gadget
   structure or to one flag for this structure. See <intuition/intuition.h> for
   more information. */

#define GA_Dummy	(TAG_USER + 0x30000)

/* Gadget placing (in pixels). Of course, all GA_Rel attributes are mutually
   exclusive with their non relative equivalents. */
  /* [ISG] (LONG) Left edge of gadget. */
#define GA_Left 	(GA_Dummy +  1)
  /* [ISG] (LONG) Left edge of gadget, depending on right window border:
     Left=Win->Width-this-1 */
#define GA_RelRight	(GA_Dummy +  2)
  /* [ISG] (LONG) Top edge of gadget. */
#define GA_Top		(GA_Dummy +  3)
  /* [ISG] (LONG) Top edge of gadget, depending on bottom window border:
     Top=Win->Height-this-1 */
#define GA_RelBottom	(GA_Dummy +  4)
  /* [ISG] (LONG) Width of gadget. */
#define GA_Width	(GA_Dummy +  5)
  /* [ISG] (LONG) Width of gadget, depending on window width:
     Width=Win->Width-this */
#define GA_RelWidth	(GA_Dummy +  6)
  /* [ISG] (LONG) Height of gadget. */
#define GA_Height	(GA_Dummy +  7)
  /* [ISG] (LONG) Height of gadget, depending on window height:
     Height=Win->Height-this */
#define GA_RelHeight	(GA_Dummy +  8)

/* Gadget rendering. */
  /* [IS.] (UBYTE *) Label text. This is mutually exclusive with GA_IntuiText
     and GA_LabelImage. */
#define GA_Text 	(GA_Dummy +  9)
/* The next two attributes are mutually exclusive. */
  /* [IS.] (struct Image *) Gadgets' image. */
#define GA_Image	(GA_Dummy + 10)
  /* [IS.] (struct Border *) Gadgets' border. */
#define GA_Border	(GA_Dummy + 11)
  /* [IS.] (struct Image *) Gadgets' image in selected state. Note that if
     this is NULL and GA_Image is in fact an image object, GA_Image may be
     tried to be drawn with IDS_SELECTED. So you do not need to fill this in,
     if you wish to have a special selected image and GA_Image is an image
     object that supports the selected state. */
#define GA_SelectRender (GA_Dummy + 12)
  /* [IS.] (ULONG) Takes GFLG_GADGH* flags (see <intuition/intuition.h>) as
     argument. Used to specify the highlighting technique. */
#define GA_Highlight	(GA_Dummy + 13)
  /* [ISG] (BOOL) If this is set to true, the gadget is not selectable. Often
     this is visually represented by using a special disabled pattern. */
#define GA_Disabled	(GA_Dummy + 14)

/* Additional information. */
  /* [IS.] (BOOL) The Gadget is a GimmeZeroZero gadget. */
#define GA_GZZGadget	(GA_Dummy + 15)
  /* [ISG] (LONG) The gadget ID to identify that gadget. Choose whatever number
     you want, but try to avoid to use the same number twice in an application.
     If you are sending an OM_UPDATE method from a gadget dispatcher, you
     should fill in your own gadget ID here. */
#define GA_ID		(GA_Dummy + 16)
  /* [ISG] (IPTR) Fill with whatever you want to. This field is ignored by
     the system. */
#define GA_UserData	(GA_Dummy + 17)
  /* [IS.] (APTR) Pointer to additional information, needed by some gadgets
     (like string or integer gadgets). This field should generally only be set
     by subclasses of GadgetClass. Applications should keep their hands off it.
  */
#define GA_SpecialInfo	(GA_Dummy + 18)
  /* [ISG] (BOOL) Determines the selected state of a gadget. */

/* Gadget activation. */
#define GA_Selected	(GA_Dummy + 19)
  /* [IS.] (BOOL) Only used for requester gadgets. This tells intuition that
     the requester is to be closed, when the gadget is released. */
#define GA_EndGadget	(GA_Dummy + 20)
  /* [IS.] (BOOL) If set the gadget responds immediatly, when the gadget is
     selected. */
#define GA_Immediate	(GA_Dummy + 21)
  /* [IS.] (BOOL) If set the gadget responds, when it is released from selected
     state. */
#define GA_RelVerify	(GA_Dummy + 22)
  /* [IS.] (BOOL) If this is set, the gadget receives information about the
     movement of the mouse as long as it is activated. */
#define GA_FollowMouse	(GA_Dummy + 23)

/* The (boolean) border attributes mean that the gadget is to be included in a
   window border, when the window containing it is opened. */
#define GA_RightBorder	(GA_Dummy + 24) /* [IS.] (BOOL) */
#define GA_LeftBorder	(GA_Dummy + 25) /* [IS.] (BOOL) */
#define GA_TopBorder	(GA_Dummy + 26) /* [IS.] (BOOL) */
#define GA_BottomBorder (GA_Dummy + 27) /* [IS.] (BOOL) */

  /* [IS.] (BOOL) Set this to turn on the toggle-select mode. */
#define GA_ToggleSelect (GA_Dummy + 28)

/* The following two attributes are PRIVATE! */
  /* [IS.] (BOOL) Set, if gadget is a system-gadget (eg a standard window
     border gadget. */
#define GA_SysGadget	(GA_Dummy + 29)
  /* [I..] (ULONG) Flag to indicate, which kind of system gadget this is (see
     <intuition/intuition.h> for more information). */
#define GA_SysGType	(GA_Dummy + 30)

/* Gadget linking. */
  /* [I..] (struct Gadget *) Pointer to previous gadget. This is used to link
     the current gadget into a gadget list, before this list is used. It can
     not be used to add a gadget to a list of an open window or requester! */
#define GA_Previous	(GA_Dummy + 31)
  /* [I..] (struct Gadget *) Currently not implemented. */
#define GA_Next 	(GA_Dummy + 32)

  /* [I..] Some gadgets need a DrawInfo structure (see <intuition/screens.h>)
     to be able to perform correct rendering. Read the documentation of the
     subclasses to learn, which need this attribute. To be on the safe side,
     you can always supply it. */
#define GA_DrawInfo	(GA_Dummy + 33)

/* You should use at most ONE of GA_Text, GA_IntuiText, and GA_LabelImage! */
  /* [IS.] (struct IntuiText *) The label of the gadget expressed as IntuiText
     structure (see <intuition/intuition.h>). */
#define GA_IntuiText	 (GA_Dummy + 34)
  /* [IS.] (struct Object *) Use this image object as label. */
#define GA_LabelImage	 (GA_Dummy + 35)

  /* [IS.] (BOOL) If set to true that gadget participates in TAB handling, ie
     if tab is pressed, the next gadget is activated. */
#define GA_TabCycle	 (GA_Dummy + 36)
  /* [..G] (BOOL) If this is set by the gadget, the sends GADGETHELP messages.
  */
#define GA_GadgetHelp	 (GA_Dummy + 37)
  /* [IS.] (struct IBox *) Bounds to be copied into the ExtGadget structure.
     (see <intuition/intuition.h> for more information). */
#define GA_Bounds	 (GA_Dummy + 38)
  /* [IS.] (BOOL) This attribute should only be set by subclasses of
     GadgetClass. Applications should keep their hands away!
     If set this means, that GM_LAYOUT is called, when the window it is in is
     opened or its size changes. This allows gadgets to make their own size
     dependent on the size of the window. */
#define GA_RelSpecial	 (GA_Dummy + 39)

#define GA_TextAttr	 (GA_Dummy + 40)
  /* [] (BOOL) */
#define GA_ReadOnly	 (GA_Dummy + 41)
#define GA_Underscore	 (GA_Dummy + 42)
#define GA_ActivateKey   (GA_Dummy + 43)
#define GA_BackFill	 (GA_Dummy + 44)
#define GA_GadgetHelpText (GA_Dummy + 45)
#define GA_UserInput	 (GA_Dummy + 46)

/* The following attributes are AROS specific. */
  /* [IS.] (struct TextAttr *) TextAttr structure (see <graphics/text.h>) to
     use for gadget rendering. This attribute is not directly supported by
     GadgetClass. */

  /* [I..] (LONG) Choose the placing of the label. GadgetClass does not support
     this directly. Its subclasses have to take care of that. For possible
     values see below. */
#define GA_LabelPlace    (GA_Dummy + 100)


/* Placetext values for GA_LabelPlace. */
#define GV_LabelPlace_In    1
#define GV_LabelPlace_Left  2
#define GV_LabelPlace_Right 3
#define GV_LabelPlace_Above 4
#define GV_LabelPlace_Below 5


/************* Methods for GadgetClass and its subclasses. ************/

/* This method is used to test, if a mouse-click hit the gadget. You return
   GMR_GADGETHIT (see below), if you were hit and 0UL otherwise. Note that you
   have to test, if you were hit, no matter if you are disabled or not. */
#define GM_HITTEST 0UL
struct gpHitTest
{
    STACKED ULONG		MethodID;   /* GM_HITEST or GM_HELPTEST */
    STACKED struct GadgetInfo  *gpht_GInfo; /* see <intuition/cghooks.h> */

      /* These values are relative to the gadget select box for GM_HITTEST. For
         GM_HELPTEST they are relative to the bounding box (which is often
         equal to the select box). */
    STACKED struct
    {
	STACKED WORD X;
	STACKED WORD Y;
    }			gpht_Mouse;
};
#define GMR_GADGETHIT 0x00000004


/* This method is invoked to draw the gadget into a rastport. */
#define GM_RENDER 1     /* draw yourself in the right state */
struct gpRender
{
    STACKED ULONG		MethodID;   /* GM_RENDER */
    STACKED struct GadgetInfo  *gpr_GInfo;  /* see <intuition/cghooks.h> */
    STACKED struct RastPort    *gpr_RPort;  /* RastPort (see <graphics/rastport.h>) to
                                       render into. */
    STACKED LONG		gpr_Redraw; /* see below */
};
/* gpr_Redraw. Not all of these values make sense for all gadgets. */
#define GREDRAW_TOGGLE 0 /* Just toggle the status. */
#define GREDRAW_REDRAW 1 /* Redraw the whole gadget. */
#define GREDRAW_UPDATE 2 /* Some data (eg the level of a slider) was updated.
                            Just redraw the necessary parts. */


/* GM_GOACTIVE tells the gadget that it has become active and will receive
   input from now on from the method GM_HANDLEINPUT. This is stopped by using
   GM_GOINACTIVE (see below).
   GM_GOACTIVE and GM_HANDLEINPUT both use the same structure and return the
   same values, as defined below. */
#define GM_GOACTIVE    2
#define GM_HANDLEINPUT 3
struct gpInput
{
    STACKED ULONG		 MethodID;        /* GM_GOACTIVE or GM_HANDLEINPUT */
    STACKED struct GadgetInfo  * gpi_GInfo;       /* see <intuition/cghooks.h> */
      /* Pointer to the InputEvent (see <devices/inputevent.h>) that caused
         the method to be invoked. */
    STACKED struct InputEvent  * gpi_IEvent;
      /* Pointer to a variable that is to be set by the gadget class, if
         GMR_VERIFY is returned. The lower 16 bits of this value are returned
         in the Code field of the IntuiMessage (see <intuition/intuition.h>)
         passed back to the application. */
    STACKED LONG	       * gpi_Termination;

      /* This struct defines the current mouse position, relative to the
         gadgets' bounding box. */
    STACKED struct
    {
	STACKED WORD X;
	STACKED WORD Y;
    }			gpi_Mouse;
      /* Pointer to TabletData structure (see <intuition/intuition.h>) or NULL,
         if this input event did not originate from a tablet that is capable of
         sending IESUBCLASS_NEWTABLET events. */
    STACKED struct TabletData * gpi_TabletData;
};

/* Return codes for GM_GOACTIVE and GM_HANDLEINPUT. These are actually flags
   and may be or'ed. */
  /* Gadget is still alive. */
#define GMR_MEACTIVE      0
  /* Gadget has become inactive, but the input event may not be used again. */
#define GMR_NOREUSE       (1L<<1)
  /* Gadget has become inactive, and the input event may be reused by
     intuition. */
#define GMR_REUSE         (1L<<2)
  /* Gadget was selected. Generate IDCMP_GADGETUP message. gpi_Termination must
     be set. */
#define GMR_VERIFY        (1L<<3)

/* If one of the following two flags is returned, the gadget has become
   inactive, but the next or previous gadget, which has the GFLG_TABCYCLE flag
   set is to be activated. */
#define GMR_NEXTACTIVE    (1L<<4) /* Activate next gadget. */
#define GMR_PREVACTIVE    (1L<<5) /* Activate previous gadget. */

/* See GM_GOACTIVE for explanation. */
#define GM_GOINACTIVE 4
struct gpGoInactive
{
    STACKED ULONG		MethodID;   /* GM_GOINACTIVE */
    STACKED struct GadgetInfo  *gpgi_GInfo; /* see <intuition/cghooks.h> */
      /* Boolean field to indicate, who wanted the gadget to go inactive. If
         this is 1 this method was sent, because intution wants the gadget to
         go inactive, if it is 0, it was the gadget itself that wanted it. */
    STACKED ULONG		gpgi_Abort;
};


/* This method is used to determine, if the gadget sends gadget help, if the
   mouse is at the coordinates specified in the message. Uses gpHitTest (see
   above) as message structure. Returns one of the value below. */
#define GM_HELPTEST 5

/* Return codes. */
  /* The gadget was not hit. */
#define GMR_NOHELPHIT ((ULONG)0UL)
  /* The gadget was hit. The lower word of the Code field of the IntuiMessage
     (see <intuition/intuition.h>) will be set to -1. */
#define GMR_HELPHIT   ((ULONG)0xFFFFFFFF)
  /* The gadget was hit. Pass the lower word, returned by this method to the
     application by using the Code field of the IntuiMessage. */
#define GMR_HELPCODE  ((ULONG)0x00010000)


/* This method is called by intuition, if one of the GFLG_REL flags or one of
   the GA_Rel attributes is set and the window size changes or you are added to
   a window. In this method you should re-evaluate the size of yourself. You
   are not allowed to do any rendering operation during this method! */
#define GM_LAYOUT 6
struct gpLayout
{
    STACKED ULONG		 MethodID;    /* GM_LAYOUT */
    STACKED struct GadgetInfo  * gpl_GInfo;   /* see <intuition/cghooks.h> */
      /* Boolean that indicated, if this method was invoked, when you are added
         to a window (TRUE) or if it is called, because the window was resized
         (FALSE). */
    STACKED ULONG		 gpl_Initial;
};


/* This method is invoked to learn about the sizing requirements of your class,
   before an object is created. */
#define GM_DOMAIN 7
struct gpDomain
{
    STACKED ULONG	        MethodID;   /* GM_DOMAIN */
    STACKED struct GadgetInfo * gpd_GInfo;  /* see <intuition/cghooks.h> */
    STACKED struct RastPort   * gpd_RPort;  /* RastPort to calculate dimensions for. */
    STACKED LONG	        gpd_Which;  /* see below */
    STACKED struct IBox         gpd_Domain; /* Resulting domain. */
    STACKED struct TagItem    * gpd_Attrs;  /* Additional attributes. None defined,
                                       yet. */
};

/* gpd_Which */
#define GDOMAIN_MINIMUM 0 /* Calculate minimum size. */
#define GDOMAIN_NOMINAL 1 /* Calculate nominal size. */
#define GDOMAIN_MAXIMUM 2 /* Calculate maximum size. */

/**********************************************************************
 **************************** ButtonGClass ****************************
 **********************************************************************/

/* ButtonGClass is a subclass of GadgetClass that renders itself as a standard
   "button" gadget. You may either supply a label attribute (GA_Text,
   GA_IntuiText or GA_LabelImage) or use the image attributes, but not both
   kind of attributes together. To use that kind of rendering, use the
   FrButtonClass instead.

   This class requires a DrawInfo structure to be passed in with GA_DrawInfo!
*/

/**********************************************************************
 **************************** FrButtonClass ***************************
 **********************************************************************/

/* FrButtonClass is a subclass of ButtonGClass. It can be used to render both,
   a label and an image. This is normally used to display a standard "bevelled
   button". To do this, you pass in your label text either as GA_Text or as
   GA_IntuiText and additionally an image object of FrameIClass (see
   <intuition/imageclass.h>) as GA_Image. Note that the supplied image must not
   render at the place the label is rendered in. To accomplish this with
   FrameIClass, supply the attribute IA_EdgesOnly with the value TRUE to
   FrameIClass.

   Note that all images that are passed in actually have to be image objects!

   This class requires a DrawInfo structure to be passed in with GA_DrawInfo!
*/

/**********************************************************************
 ***************************** PropGClass *****************************
 **********************************************************************/

/* This class defines a standard proportional gadget. */

/* Attributes. */
#define PGA_Dummy      (TAG_USER + 0x31000)

  /* [IS.] (ULONG) Define in which the direction gadget should stretch.
     Possible values are FREEVERT and FREEHORIZ (see <intuition/intuition.h>).
  */
#define PGA_Freedom    (PGA_Dummy +  1)
  /* [IS.] (BOOL) If set, no border will be rendered. */
#define PGA_Borderless (PGA_Dummy +  2)

/* The following four attributes should not be used with PGA_Total, PGA_Visible
   and PGA_Top. */
  /* [ISG] (UWORD) */
#define PGA_HorizPot   (PGA_Dummy +  3)
  /* [ISG] (UWORD) */
#define PGA_HorizBody  (PGA_Dummy +  4)
  /* [ISG] (UWORD) */
#define PGA_VertPot    (PGA_Dummy +  5)
  /* [ISG] (UWORD) */
#define PGA_VertBody   (PGA_Dummy +  6)

/* The following three attributes should not be used with the PGA_*Pot and
   PGA_*Body attributes. */
  /* [IS.] (UWORD) The total number of positions in the gadget. */
#define PGA_Total      (PGA_Dummy +  7)
  /* [IS.] (UWORD) The number of visible positions in the gadget. */
#define PGA_Visible    (PGA_Dummy +  8)
  /* [ISG] (UWORD) The first visible position. */
#define PGA_Top        (PGA_Dummy +  9)

  /* [IS.] (BOOL) If set, this indicated that the new look should be used for
     rendering. */
#define PGA_NewLook    (PGA_Dummy + 10)

  /* (I.G) (struct Hook) Use this Hook to render the Gadget visuals
     */

#define PGA_DisplayHook (PGA_Dummy + 11)
/* AROS extensions */

 /* [I] (UWORD) If set to PG_BEHAVIOUR_NICE OM_NOTIFY messages are sent
    also during OM_SET/OM_UPDATE, not just when user drags the knob,
    which is the default behaviour (PG_BEHAVIOUR_COMPATIBLE) */
#define PGA_NotifyBehaviour (PGA_Dummy + 30)

 /* [I] (UWORD) If set to PG_BEHAVIOUR_NICE the gadget is re-rendered
    during OM_SET/OM_UPDATE even when being a subclass of propgclass.
    The default behaviour (PG_BEHAVIOUR_COMPATIBLE) is that subclasses
    of propgclass don't render in OM_SET/OM_UPDATE */
#define PGA_RenderBehaviour (PGA_Dummy + 31)

#define PG_BEHAVIOUR_COMPATIBLE 0
#define PG_BEHAVIOUR_NICE       1

/**********************************************************************
 ****************************** StrGClass *****************************
 **********************************************************************/

/* StringGClass is just a normal "string" gadget. */

/* Attributes. */
#define STRINGA_Dummy          (TAG_USER + 0x32000)

  /* [I..] (WORD) Maximum number of characters the string gadget accepts.
     Default defined below. */
#define STRINGA_MaxChars       (STRINGA_Dummy +  1)
  /* [I..] (STRPTR) Buffer for storing the current string of the gadget. */
#define STRINGA_Buffer	       (STRINGA_Dummy +  2)
  /* [I..] (STRPTR) Buffer for storing the old (undo) string of the gadget. */
#define STRINGA_UndoBuffer     (STRINGA_Dummy +  3)
  /* [I..] (STRPTR) Buffer for the class to work with. */
#define STRINGA_WorkBuffer     (STRINGA_Dummy +  4)
  /* [IS.] (WORD) Current position of cursor (relative to the beginning of the
     buffer). */
#define STRINGA_BufferPos      (STRINGA_Dummy +  5)
  /* [IS.] (WORD) FIXME */
#define STRINGA_DispPos        (STRINGA_Dummy +  6)
  /* [IS.] (struct KeyMap *) KeyMap to use (see <devices/keymaps.h>). */
#define STRINGA_AltKeyMap      (STRINGA_Dummy +  7)
  /* [IS.] (struct TextFont *) Font to use for displaying the string (see
     <graphics/text.h>). */
#define STRINGA_Font	       (STRINGA_Dummy +  8)
  /* [IS.] (LONG) The lower 16 bits specify the background-pen, the upper 16
     bits the foreground-pen. The gadget is rendered, using these pens, if the
     gadget is inactive */
#define STRINGA_Pens	       (STRINGA_Dummy +  9)
  /* [IS.] (LONG) Like STRINGA_Pens. These pens are used, if the gadget is
     active. */
#define STRINGA_ActivePens     (STRINGA_Dummy + 10)
  /* [I..] (struct Hook *) FIXME */
#define STRINGA_EditHook       (STRINGA_Dummy + 11)
  /* [IS.] (ULONG) FIXME */
#define STRINGA_EditModes      (STRINGA_Dummy + 12)
  /* [IS.] (BOOL) If this is TRUE, the current character is overwritten, if the
     use presses a key. Otherwise, the new character is inserted. */
#define STRINGA_ReplaceMode    (STRINGA_Dummy + 13)
  /* [IS.] (BOOL) FIXME */
#define STRINGA_FixedFieldMode (STRINGA_Dummy + 14)
  /* [IS.] (BOOL) FIXME */
#define STRINGA_NoFilterMode   (STRINGA_Dummy + 15)
  /* [IS.] (UWORD) Where should the text be justified? Use one of
     GACT_STRINGCENTER, GACT_STRINGLEFT and GACT_STRINGRIGHT (defined in
     <intuition/intuition.h>). */
#define STRINGA_Justification  (STRINGA_Dummy + 16)
  /* [ISG] (LONG) If this is set, the string gadget will only accept numeric
     values. Argument is the number, the string gadget is to be set to. When
     getting this attribute, this number is returned. */
#define STRINGA_LongVal        (STRINGA_Dummy + 17)
  /* [ISG] (STRPTR) If this is set, the string gadget will accept strings.
     Argument is a string that is to be copied into the string gadget and its
     buffer. */
#define STRINGA_TextVal        (STRINGA_Dummy + 18)
  /* [IS.] (BOOL) If this is set, pressing the "Help" key, while the gadget is
     active, will unselect the gadget. */
#define STRINGA_ExitHelp       (STRINGA_Dummy + 19) /* Exit on "Help" */

/* Default, if STRINGA_MaxChars is not set. */
#define SG_DEFAULTMAXCHARS (128)

/* Gadget layout related attributes. */
#define LAYOUTA_Dummy	       (TAG_USER + 0x38000)
  /* FIXME */
#define LAYOUTA_LayoutObj      (LAYOUTA_Dummy + 1)
  /* FIXME */
#define LAYOUTA_Spacing        (LAYOUTA_Dummy + 2)
  /* FIXME (see below) */
#define LAYOUTA_Orientation    (LAYOUTA_Dummy + 3)
  /* FIXME */
#define LAYOUTA_ChildMaxWidth  (LAYOUTA_Dummy + 4)
  /* FIXME */
#define LAYOUTA_ChildMaxHeight (LAYOUTA_Dummy + 5)

/* Orientation values. */
#define LORIENT_NONE  0
#define LORIENT_HORIZ 1
#define LORIENT_VERT  2

#endif /* INTUITION_GADGETCLASS_H */
