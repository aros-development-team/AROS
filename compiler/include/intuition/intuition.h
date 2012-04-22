#ifndef INTUITION_INTUITION_H
#define INTUITION_INTUITION_H

/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Important defines and structures for intuition.library
    Lang: english
*/

#ifndef EXEC_PORTS_H
#   include <exec/ports.h>
#endif
#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef GRAPHICS_CLIP_H
#   include <graphics/clip.h>
#endif
#ifndef GRAPHICS_GFX_H
#   include <graphics/gfx.h>
#endif
#ifndef GRAPHICS_LAYERS_H
#   include <graphics/layers.h>
#endif
#ifndef GRAPHICS_RASTPORT_H
#   include <graphics/rastport.h>
#endif
#ifndef GRAPHICS_TEXT_H
#   include <graphics/text.h>
#endif
#ifndef GRAPHICS_VIEW_H
#   include <graphics/view.h>
#endif
#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif
#ifndef DEVICES_INPUTEVENT_H
#   include <devices/inputevent.h>
#endif
#ifndef INTUITION_PREFERENCES_H
#   include <intuition/preferences.h>
#endif
#ifndef INTUITION_SCREENS_H
#   include <intuition/screens.h>
#endif
#ifndef INTUITION_CLASSUSR_H
#   include <intuition/classusr.h>
#endif

#define INTUITIONNAME "intuition.library"

		       /***** Intuition Message *****/

struct IntuiMessage
{
    struct Message ExecMessage;

    ULONG Class;
    UWORD Code;
    UWORD Qualifier;
    APTR  IAddress;

    WORD  MouseX;
    WORD  MouseY;
    ULONG Seconds;
    ULONG Micros;

    struct Window	* IDCMPWindow;
    struct IntuiMessage * SpecialLink;
};

struct ExtIntuiMessage
{
    struct IntuiMessage eim_IntuiMessage;
    struct TabletData * eim_TabletData;
};

			    /***** IDCMP *****/

#define IDCMP_SIZEVERIFY     (1L<<0)
#define IDCMP_NEWSIZE	     (1L<<1)
#define IDCMP_REFRESHWINDOW  (1L<<2)
#define IDCMP_MOUSEBUTTONS   (1L<<3)
#define IDCMP_MOUSEMOVE      (1L<<4)
#define IDCMP_GADGETDOWN     (1L<<5)
#define IDCMP_GADGETUP	     (1L<<6)
#define IDCMP_REQSET	     (1L<<7)
#define IDCMP_MENUPICK	     (1L<<8)
#define IDCMP_CLOSEWINDOW    (1L<<9)
#define IDCMP_RAWKEY	     (1L<<10)
#define IDCMP_REQVERIFY      (1L<<11)
#define IDCMP_REQCLEAR	     (1L<<12)
#define IDCMP_MENUVERIFY     (1L<<13)
#define IDCMP_NEWPREFS	     (1L<<14)
#define IDCMP_DISKINSERTED   (1L<<15)
#define IDCMP_DISKREMOVED    (1L<<16)
#define IDCMP_WBENCHMESSAGE  (1L<<17)
#define IDCMP_ACTIVEWINDOW   (1L<<18)
#define IDCMP_INACTIVEWINDOW (1L<<19)
#define IDCMP_DELTAMOVE      (1L<<20)
#define IDCMP_VANILLAKEY     (1L<<21)
#define IDCMP_INTUITICKS     (1L<<22)
#define IDCMP_IDCMPUPDATE    (1L<<23)
#define IDCMP_MENUHELP	     (1L<<24)
#define IDCMP_CHANGEWINDOW   (1L<<25)
#define IDCMP_GADGETHELP     (1L<<26)
#define IDCMP_LONELYMESSAGE  (1L<<31)

#define CWCODE_MOVESIZE 0
#define CWCODE_DEPTH	1
#define MENUHOT 	1
#define MENUCANCEL	2
#define MENUWAITING	3
#define OKOK		1
#define OKCANCEL	2
#define OKABORT 	4
#define WBENCHOPEN	1
#define WBENCHCLOSE	2

			    /***** IntuiText *****/

struct IntuiText
{
    UBYTE FrontPen;
    UBYTE BackPen;
    UBYTE DrawMode;
    WORD  LeftEdge;
    WORD  TopEdge;

    struct TextAttr  * ITextFont;
    UBYTE	     * IText;
    struct IntuiText * NextText;
};

			     /***** Menu *****/

struct Menu
{
    struct Menu * NextMenu;

    WORD    LeftEdge;
    WORD    TopEdge;
    WORD    Width;
    WORD    Height;
    UWORD   Flags;    /* see below */
    BYTE  * MenuName;

    struct MenuItem * FirstItem;

    /* PRIVATE */
    WORD JazzX;
    WORD JazzY;
    WORD BeatX;
    WORD BeatY;
};

/* Flags */
#define MENUENABLED (1<<0)
/* The following flag is READ-ONLY */
#define MIDRAWN     (1<<8)

struct MenuItem
{
    struct MenuItem * NextItem;

    WORD  LeftEdge;
    WORD  TopEdge;
    WORD  Width;
    WORD  Height;
    UWORD Flags;	 /* see below */
    LONG  MutualExclude;
    APTR  ItemFill;
    APTR  SelectFill;
    BYTE  Command;

    struct MenuItem * SubItem;
    UWORD	      NextSelect;
};

/* Flags */
#define CHECKIT     (1<<0)
#define ITEMTEXT    (1<<1)
#define COMMSEQ     (1<<2)
#define MENUTOGGLE  (1<<3)
#define ITEMENABLED (1<<4)
#define HIGHIMAGE   0x0000
#define HIGHCOMP    (1<<6)
#define HIGHBOX     (1<<7)
#define HIGHNONE    0x00C0
#define HIGHFLAGS   0x00C0
#define CHECKED     (1<<8)
/* The following flags are READ-ONLY */
#define ISDRAWN     (1<<12)
#define HIGHITEM    (1<<13)
#define MENUTOGGLED (1<<14)

#define NOMENU	 0x001F
#define NOITEM	 0x003F
#define NOSUB	 0x001F
#define MENUNULL 0xFFFF

/* Macros */
#define MENUNUM(x) ((x) & 0x1F)
#define ITEMNUM(x) (((x)>>5)  & 0x003F)
#define SUBNUM(x)  (((x)>>11) & 0x001F)

#define SHIFTMENU(x) ((x) & 0x1F)
#define SHIFTITEM(x) (((x) & 0x3F)<<5)
#define SHIFTSUB(x)  (((x) & 0x1F)<<11)
#define FULLMENUNUM(m,i,s) (SHIFTMENU(m) | SHIFTITEM(i) | SHIFTSUB(s))

#define SRBNUM(x)  (0x08 - ((x)>>4))
#define SWBNUM(x)  (0x08 - ((x) & 0x0F))
#define SSBNUM(x)  (0x01 + ((x)>>4))
#define SPARNUM(x) ((x)>>4)
#define SHAKNUM(x) ((x) & 0x0F)

#define CHECKWIDTH    19
#define LOWCHECKWIDTH 13
#define COMMWIDTH     27
#define LOWCOMMWIDTH  16

			   /***** Gadgets *****/

struct Gadget
{
    struct Gadget * NextGadget;

    WORD LeftEdge;
    WORD TopEdge;
    WORD Width;
    WORD Height;

    UWORD Flags;      /* see below */
    UWORD Activation; /* see below */
    UWORD GadgetType; /* see below */

    APTR	       GadgetRender;
    APTR	       SelectRender;
    struct IntuiText * GadgetText;

    IPTR MutualExclude; /* OBSOLETE */

    APTR  SpecialInfo;
    UWORD GadgetID;
    APTR  UserData;
};

struct ExtGadget
{
    struct ExtGadget * NextGadget;

    WORD LeftEdge;
    WORD TopEdge;
    WORD Width;
    WORD Height;

    UWORD Flags;      /* see below */
    UWORD Activation; /* see below */
    UWORD GadgetType; /* see below */

    APTR	       GadgetRender;
    APTR	       SelectRender;
    struct IntuiText * GadgetText;

    IPTR MutualExclude; /* OBSOLETE */

    APTR  SpecialInfo;
    UWORD GadgetID;
    APTR  UserData;

/* ExtGadget specific fields */
    ULONG MoreFlags;	  /* see below */
    WORD  BoundsLeftEdge;
    WORD  BoundsTopEdge;
    WORD  BoundsWidth;
    WORD  BoundsHeight;
};

/* Flags */
#define GFLG_GADGHCOMP	    0x0000
#define GFLG_GADGHBOX	    (1<<0)
#define GFLG_GADGHIMAGE     (1<<1)
#define GFLG_GADGHNONE	    0x0003
#define GFLG_GADGHIGHBITS   0x0003
#define GFLG_GADGIMAGE	    (1<<2)
#define GFLG_RELBOTTOM	    (1<<3)
#define GFLG_RELRIGHT	    (1<<4)
#define GFLG_RELWIDTH	    (1<<5)
#define GFLG_RELHEIGHT	    (1<<6)
#define GFLG_SELECTED	    (1<<7)
#define GFLG_DISABLED	    (1<<8)
#define GFLG_TABCYCLE	    (1<<9)
#define GFLG_STRINGEXTEND   (1<<10)
#define GFLG_IMAGEDISABLE   (1<<11)
#define GFLG_LABELITEXT     0x0000
#define GFLG_LABELSTRING    (1<<12)
#define GFLG_LABELIMAGE     (1<<13)
#define GFLG_LABELMASK	    0x3000
#define GFLG_RELSPECIAL     (1<<14)
#define GFLG_EXTENDED	    (1<<15)

/* Activation */
#define GACT_RELVERIFY	  (1<<0)
#define GACT_IMMEDIATE	  (1<<1)
#define GACT_ENDGADGET	  (1<<2)
#define GACT_FOLLOWMOUSE  (1<<3)
#define GACT_RIGHTBORDER  (1<<4)
#define GACT_LEFTBORDER   (1<<5)
#define GACT_TOPBORDER	  (1<<6)
#define GACT_BOTTOMBORDER (1<<7)
#define GACT_TOGGLESELECT (1<<8)
#define GACT_STRINGLEFT   0
#define GACT_STRINGCENTER (1<<9)
#define GACT_STRINGRIGHT  (1<<10)
#define GACT_LONGINT	  (1<<11)
#define GACT_ALTKEYMAP	  (1<<12)
#define GACT_STRINGEXTEND (1<<13)
#define GACT_BOOLEXTEND   (1<<13)
#define GACT_ACTIVEGADGET (1<<14)
#define GACT_BORDERSNIFF  (1<<15)

/* GadgetType */
#define GTYP_GADGETTYPE   0xFC00
#define GTYP_SYSTYPEMASK  0x00F0
#define GTYP_SIZING	  0x0010
#define GTYP_WDRAGGING	  0x0020
#define GTYP_SDRAGGING	  0x0030
#define GTYP_WDEPTH	  0x0040
#define GTYP_SDEPTH	  0x0050
#define GTYP_WZOOM	  0x0060
#define GTYP_SUNUSED	  0x0070
#define GTYP_CLOSE	  0x0080
#define GTYP_REQGADGET	  (1<<12)
#define GTYP_GZZGADGET	  (1<<13)
#define GTYP_SCRGADGET	  (1<<14)
#define GTYP_SYSGADGET	  (1<<15)
#define GTYP_BOOLGADGET   0x0001
#define GTYP_GADGET0002   0x0002
#define GTYP_PROPGADGET   0x0003
#define GTYP_STRGADGET	  0x0004
#define GTYP_CUSTOMGADGET 0x0005
#define GTYP_GTYPEMASK	  0x0007

/* MoreFlags */
#define GMORE_BOUNDS	   (1L<<0)
#define GMORE_GADGETHELP   (1L<<1)
#define GMORE_SCROLLRASTER (1L<<2)
#define GMORE_BOOPSIGADGET (1L<<3) /* some internal boopsi classes changes the gadget type during execution (ie propgclass), so GTYP_CUSTOMGADGET doesn´t work (dariusb) */

/***** Bool Gadget *****/
struct BoolInfo
{
    UWORD   Flags;    /* see below */
    UWORD * Mask;
    ULONG   Reserved; /* must be NULL */
};

/* Flags */
#define BOOLMASK (1<<0)

/***** Proportional gadget *****/
struct PropInfo
{
    UWORD Flags;      /* see below */
    UWORD HorizPot;
    UWORD VertPot;
    UWORD HorizBody;
    UWORD VertBody;
    UWORD CWidth;
    UWORD CHeight;
    UWORD HPotRes;
    UWORD VPotRes;
    UWORD LeftBorder;
    UWORD TopBorder;
};

#define AUTOKNOB       (1<<0)
#define FREEHORIZ      (1<<1)
#define FREEVERT       (1<<2)
#define PROPBORDERLESS (1<<3)
#define PROPNEWLOOK    (1<<4)
#define KNOBHIT        (1<<8)

#define KNOBHMIN 6
#define KNOBVMIN 4
#define MAXBODY  0xFFFF
#define MAXPOT	 0xFFFF

/***** StringInfo *****/

struct StringInfo
{
    UBYTE * Buffer;
    UBYTE * UndoBuffer;
    WORD    BufferPos;
    WORD    MaxChars;
    WORD    DispPos;

    WORD UndoPos;
    WORD NumChars;
    WORD DispCount;
    WORD CLeft;
    WORD CTop;

    struct StringExtend * Extension;
    LONG		  LongInt;
    struct KeyMap	* AltKeyMap;
};

/**********************************************************************
 **                            Requesters                            **
 **********************************************************************/

/* The following struct is used for standard intuition requesters
   (not to be mixed up with asl or easy requesters).
   See intuition.library/Request() for more information. */
struct Requester
{
    struct Requester * OlderRequest;

    /* The dimensions of the requester */
    WORD LeftEdge;
    WORD TopEdge;
    WORD Width;
    WORD Height;
    WORD RelLeft;
    WORD RelTop;

    struct Gadget    * ReqGadget; /* First gadget of the requester */
    struct Border    * ReqBorder; /* First border of the requester */
    struct IntuiText * ReqText;   /* First intuitext of the requester */

    UWORD Flags;    /* see below */
    UBYTE BackFill; /* a pen to fill the background of the requester */

    struct Layer * ReqLayer; /* The layer on which the requester is based */

    UBYTE ReqPad1[32]; /* PRIVATE */

    struct BitMap * ImageBMap; /* you may use this to fill the requester
                                  with your own image */
    struct Window * RWindow;   /* window, which the requester belongs to */
    struct Image  * ReqImage;  /* corresponds to USEREQIMAGE (see below) */

    UBYTE ReqPad2[32]; /* PRIVATE */
};

/* Flags */
#define POINTREL      (1<<0) /* If set, LeftEdge and TopEdge are relative
                                to the coordinates of either the pointer
                                or the window */
#define PREDRAWN      (1<<1) /* If set, ImageBMap points to a custom bitmap */
#define NOISYREQ      (1<<2) /* Requester doesn't filter input */
#define SIMPLEREQ     (1<<4) /* If set, a SIMPLEREFRESH layer is used */
#define USEREQIMAGE   (1<<5) /* ReqImage points to an image, which is used
                                as background */
#define NOREQBACKFILL (1<<6) /* Ignore BackFill pen */
/* The following flags are READ-ONLY */
#define REQOFFWINDOW  (1<<12)
#define REQACTIVE     (1<<13) /* Requester is active */
#define SYSREQUEST    (1<<14) /* unused */
#define DEFERREFRESH  (1<<15)

/* This struct is passes as second parameter to EasyRequestArgs() and
   BuildEasyRequest(). It describes the general look of the requester. */
struct EasyStruct
{
    ULONG        es_StructSize;   /* Should be sizeof(struct EasyStruct). Note
                                     that this size may change, if you update the
                                     includes! Do not use absolute values as
                                     the size of pointers may vary on different
                                     platforms! */
    ULONG        es_Flags;        /* None defined, yet */
    CONST_STRPTR es_Title;        /* Text in the titlebar of the requester */
    CONST_STRPTR es_TextFormat;   /* Text in requester (printf-style). The
                                     arguments needed for that string are the
                                     fourth paramter to EasyRequestArgs() */
    CONST_STRPTR es_GadgetFormat; /* Text of the gadgets, separated by |'s */
};

			    /***** Window *****/

struct Window
{
    struct Window * NextWindow;

    WORD LeftEdge;
    WORD TopEdge;
    WORD Width;
    WORD Height;

    WORD MouseY;
    WORD MouseX;

    WORD MinWidth;
    WORD MinHeight;
    UWORD MaxWidth;
    UWORD MaxHeight;

    ULONG Flags;

    struct Menu      * MenuStrip;
    UBYTE	     * Title;
    struct Requester * FirstRequest;
    struct Requester * DMRequest;

    WORD ReqCount;

    struct Screen   * WScreen;
    struct RastPort * RPort;

    BYTE	      BorderLeft;
    BYTE	      BorderTop;
    BYTE	      BorderRight;
    BYTE	      BorderBottom;
    struct RastPort * BorderRPort;

    struct Gadget * FirstGadget;
    struct Window * Parent;
    struct Window * Descendant;

    UWORD * Pointer;
    BYTE    PtrHeight;
    BYTE    PtrWidth;
    BYTE    XOffset;
    BYTE    YOffset;

    ULONG		  IDCMPFlags;
    struct MsgPort	* UserPort;
    struct MsgPort	* WindowPort;
    struct IntuiMessage * MessageKey;

    UBYTE	   DetailPen;
    UBYTE	   BlockPen;
    struct Image * CheckMark;
    UBYTE	 * ScreenTitle;

    WORD GZZMouseX;
    WORD GZZMouseY;
    WORD GZZWidth;
    WORD GZZHeight;

    UBYTE * ExtData;
    BYTE  * UserData;

    struct Layer    * WLayer;
    struct TextFont * IFont;

    ULONG MoreFlags;
    
    WORD RelLeftEdge; // relative coordinates of the window
    WORD RelTopEdge;  // to its parent window. If it is 
                      // a window on the screen then these
                      // are the same as LeftEdge and TopEdge.
    
    struct Window * firstchild;  // pointer to first child
    struct Window * prevchild;   // if window is a child of a window
    struct Window * nextchild;   // then they are concatenated here.
    struct Window * parent;      // parent of this window
};

#define HAS_CHILDREN(w) (NULL != w->firstchild)
#define IS_CHILD(w) (NULL != w->parent)

struct NewWindow
{
    WORD LeftEdge;
    WORD TopEdge;
    WORD Width;
    WORD Height;

    UBYTE DetailPen;
    UBYTE BlockPen;

    ULONG IDCMPFlags;
    ULONG Flags;

    struct Gadget * FirstGadget;
    struct Image  * CheckMark;
    UBYTE	  * Title;
    struct Screen * Screen;    /* ignored if Type != CUSTOMSCREEN */
    struct BitMap * BitMap;

    WORD  MinWidth;
    WORD  MinHeight;
    UWORD MaxWidth;
    UWORD MaxHeight;

    UWORD Type;
};

struct ExtNewWindow
{
    WORD  LeftEdge;
    WORD  TopEdge;
    WORD  Width;
    WORD  Height;

    UBYTE DetailPen;
    UBYTE BlockPen;

    ULONG IDCMPFlags;
    ULONG Flags;

    struct Gadget * FirstGadget;
    struct Image  * CheckMark;
    UBYTE	  * Title;
    struct Screen * Screen;
    struct BitMap * BitMap;

    WORD  MinWidth;
    WORD  MinHeight;
    UWORD MaxWidth;
    UWORD MaxHeight;

    UWORD Type;

/* ExtNewWindow specific fields */
    struct TagItem *Extension;
};

/* Tags */
#define WA_Dummy	     (TAG_USER + 99)
#define WA_Left 	     (WA_Dummy + 1)
#define WA_Top		     (WA_Dummy + 2)
#define WA_Width	     (WA_Dummy + 3)
#define WA_Height	     (WA_Dummy + 4)
#define WA_DetailPen	     (WA_Dummy + 5)
#define WA_BlockPen	     (WA_Dummy + 6)
#define WA_IDCMP	     (WA_Dummy + 7)
#define WA_Flags	     (WA_Dummy + 8)
#define WA_Gadgets	     (WA_Dummy + 9)
#define WA_Checkmark	     (WA_Dummy + 10)
#define WA_Title	     (WA_Dummy + 11)
#define WA_ScreenTitle	     (WA_Dummy + 12)
#define WA_CustomScreen      (WA_Dummy + 13)
#define WA_SuperBitMap	     (WA_Dummy + 14)
#define WA_MinWidth	     (WA_Dummy + 15)
#define WA_MinHeight	     (WA_Dummy + 16)
#define WA_MaxWidth	     (WA_Dummy + 17)
#define WA_MaxHeight	     (WA_Dummy + 18)
#define WA_InnerWidth	     (WA_Dummy + 19)
#define WA_InnerHeight	     (WA_Dummy + 20)
#define WA_PubScreenName     (WA_Dummy + 21)
#define WA_PubScreen	     (WA_Dummy + 22)
#define WA_PubScreenFallBack (WA_Dummy + 23)
#define WA_WindowName	     (WA_Dummy + 24)
#define WA_Colors	     (WA_Dummy + 25)
#define WA_Zoom 	     (WA_Dummy + 26)
#define WA_MouseQueue	     (WA_Dummy + 27)
#define WA_BackFill	     (WA_Dummy + 28)
#define WA_RptQueue	     (WA_Dummy + 29)
#define WA_SizeGadget	     (WA_Dummy + 30)
#define WA_DragBar	     (WA_Dummy + 31)
#define WA_DepthGadget	     (WA_Dummy + 32)
#define WA_CloseGadget	     (WA_Dummy + 33)
#define WA_Backdrop	     (WA_Dummy + 34)
#define WA_ReportMouse	     (WA_Dummy + 35)
#define WA_NoCareRefresh     (WA_Dummy + 36)
#define WA_Borderless	     (WA_Dummy + 37)
#define WA_Activate	     (WA_Dummy + 38)
#define WA_RMBTrap	     (WA_Dummy + 39)
#define WA_WBenchWindow      (WA_Dummy + 40)
#define WA_SimpleRefresh     (WA_Dummy + 41)
#define WA_SmartRefresh      (WA_Dummy + 42)
#define WA_SizeBRight	     (WA_Dummy + 43)
#define WA_SizeBBottom	     (WA_Dummy + 44)
#define WA_AutoAdjust	     (WA_Dummy + 45)
#define WA_GimmeZeroZero     (WA_Dummy + 46)
#define WA_MenuHelp	     (WA_Dummy + 47)
#define WA_NewLookMenus      (WA_Dummy + 48)
#define WA_AmigaKey	     (WA_Dummy + 49)
#define WA_NotifyDepth	     (WA_Dummy + 50)
#define WA_Pointer	     (WA_Dummy + 52)
#define WA_BusyPointer	     (WA_Dummy + 53)
#define WA_PointerDelay      (WA_Dummy + 54)
#define WA_TabletMessages    (WA_Dummy + 55)
#define WA_HelpGroup	     (WA_Dummy + 56)
#define WA_HelpGroupWindow   (WA_Dummy + 57)
/* AmigaOS4 -compatible tags follow */
#define WA_Hidden            (WA_Dummy + 60)
#define WA_ToolBox           (WA_Dummy + 61)
#define WA_ShapeRegion 	     (WA_Dummy + 65)
#define WA_ShapeHook	     (WA_Dummy + 66)
#define WA_InFrontOf	     (WA_Dummy + 67)
/* AROS specific tags follow */
#define WA_Priority 	     (WA_Dummy + 100)
#define WA_Parent   	     (WA_Dummy + 101)
#define WA_Behind   	     (WA_Dummy + 103)

/* Flags */
#define WFLG_SIZEGADGET     (1L<<0)
#define WFLG_DRAGBAR	    (1L<<1)
#define WFLG_DEPTHGADGET    (1L<<2)
#define WFLG_CLOSEGADGET    (1L<<3)
#define WFLG_SIZEBRIGHT     (1L<<4)
#define WFLG_SIZEBBOTTOM    (1L<<5)

#define WFLG_SMART_REFRESH  0
#define WFLG_SIMPLE_REFRESH (1L<<6)
#define WFLG_SUPER_BITMAP   (1L<<7)
#define WFLG_OTHER_REFRESH  ((1L<<6) | (1L<<7))
#define WFLG_REFRESHBITS    WFLG_OTHER_REFRESH

#define WFLG_BACKDROP	    (1L<<8)
#define WFLG_REPORTMOUSE    (1L<<9)
#define WFLG_GIMMEZEROZERO  (1L<<10)
#define WFLG_BORDERLESS     (1L<<11)
#define WFLG_ACTIVATE	    (1L<<12)

/* PRIVATE */
#define WFLG_WINDOWACTIVE   (1L<<13)
#define WFLG_INREQUEST	    (1L<<14)
#define WFLG_MENUSTATE	    (1L<<15)

#define WFLG_RMBTRAP	    (1L<<16)
#define WFLG_NOCAREREFRESH  (1L<<17)
#define WFLG_NW_EXTENDED    (1L<<18)

#define WFLG_NEWLOOKMENUS   (1L<<21)

/* PRIVATE */
#define WFLG_WINDOWREFRESH  (1L<<24)
#define WFLG_WBENCHWINDOW   (1L<<25)
#define WFLG_WINDOWTICKED   (1L<<26)
#define WFLG_VISITOR	    (1L<<27)
#define WFLG_ZOOMED	    (1L<<28)
#define WFLG_HASZOOM	    (1L<<29)
#define WFLG_TOOLBOX        (1L<<30)

#define DEFAULTMOUSEQUEUE 5

#define HC_GADGETHELP 1

/* Magic values for ShowWindow() and WA_InFrontOf */
#define WINDOW_BACKMOST  (NULL)
#define WINDOW_FRONTMOST ((struct Window *)1)

			   /***** Images *****/

struct Image
{
    WORD LeftEdge;
    WORD TopEdge;
    WORD Width;
    WORD Height;

    WORD    Depth;
    UWORD * ImageData;
    UBYTE   PlanePick;
    UBYTE   PlaneOnOff;

    struct Image * NextImage;
};

			   /***** Border *****/

struct Border
{
    WORD   LeftEdge;
    WORD   TopEdge;
    UBYTE  FrontPen;
    UBYTE  BackPen;
    UBYTE  DrawMode;
    BYTE   Count;
    WORD * XY;

    struct Border * NextBorder;
};

			 /***** Tablets *****/

struct TabletData
{
    UWORD td_XFraction;
    UWORD td_YFraction;
    ULONG td_TabletX;
    ULONG td_TabletY;
    ULONG td_RangeX;
    ULONG td_RangeY;

    struct TagItem * td_TagList; /* see below */
};

/* Tags */
#define TABLETA_Dummy	    (TAG_USER + 0x3A000)
#define TABLETA_TabletZ     (TABLETA_Dummy + 0x01)
#define TABLETA_RangeZ	    (TABLETA_Dummy + 0x02)
#define TABLETA_AngleX	    (TABLETA_Dummy + 0x03)
#define TABLETA_AngleY	    (TABLETA_Dummy + 0x04)
#define TABLETA_AngleZ	    (TABLETA_Dummy + 0x05)
#define TABLETA_Pressure    (TABLETA_Dummy + 0x06)
#define TABLETA_ButtonBits  (TABLETA_Dummy + 0x07)
#define TABLETA_InProximity (TABLETA_Dummy + 0x08)
#define TABLETA_ResolutionX (TABLETA_Dummy + 0x09)
#define TABLETA_ResolutionY (TABLETA_Dummy + 0x0a)

struct TabletHookData
{
    struct Screen * thd_Screen;
    ULONG	    thd_Width;
    ULONG	    thd_Height;
    LONG	    thd_ScreenChanged;
};

			  /***** Keys *****/

#define SELECTDOWN (IECODE_LBUTTON)
#define SELECTUP   (IECODE_LBUTTON | IECODE_UP_PREFIX)
#define MENUDOWN   (IECODE_RBUTTON)
#define MENUUP	   (IECODE_RBUTTON | IECODE_UP_PREFIX)
#define MIDDLEDOWN (IECODE_MBUTTON)
#define MIDDLEUP   (IECODE_MBUTTON | IECODE_UP_PREFIX)
#define ALTLEFT    (IEQUALIFIER_LALT)
#define ALTRIGHT   (IEQUALIFIER_RALT)
#define AMIGALEFT  (IEQUALIFIER_LCOMMAND)
#define AMIGARIGHT (IEQUALIFIER_RCOMMAND)
#define AMIGAKEYS  (AMIGALEFT | AMIGARIGHT)

#define CURSORUP    0x4C
#define CURSORDOWN  0x4D
#define CURSORRIGHT 0x4E
#define CURSORLEFT  0x4F

#define KEYCODE_Q	0x10
#define KEYCODE_Z	0x31
#define KEYCODE_X	0x32
#define KEYCODE_V	0x34
#define KEYCODE_B	0x35
#define KEYCODE_N	0x36
#define KEYCODE_M	0x37
#define KEYCODE_LESS	0x38
#define KEYCODE_GREATER 0x39

			   /* Miscellaneous */

struct IBox
{
    WORD Left;
    WORD Top;
    WORD Width;
    WORD Height;
};

struct Remember
{
    struct Remember * NextRemember;
    ULONG	      RememberSize;
    UBYTE	    * Memory;
};

struct ColorSpec
{
    WORD  ColorIndex;
    UWORD Red;
    UWORD Green;
    UWORD Blue;
};

#define FOREVER for(;;)
#define SIGN(x) (((x)>0) - ((x)<0))
#define NOT !

#define ALERT_TYPE     0x80000000
#define RECOVERY_ALERT 0x00000000
#define DEADEND_ALERT  0x80000000

#define AUTOFRONTPEN  0
#define AUTOBACKPEN   1
#define AUTODRAWMODE  JAM2
#define AUTOLEFTEDGE  6
#define AUTORIGHTEDGE 3
#define AUTOITEXTFONT NULL
#define AUTONEXTTEXT  NULL

/* NewDecorator structure used by ChangeDecoration
   configure class pointers (nd_WindowClass, nd_ScreenClass and nd_MenuClass)
   to overide the default/internal decoration classes.
   the port is used for different issues and will be filled
   up with DecoratorMessages */

struct NewDecorator
{   struct  Node     nd_Node;
    struct  MsgPort *nd_Port;
            UWORD    nd_cnt;
            STRPTR   nd_Pattern;
            STRPTR   nd_IntPattern; /* Private, transformated Pattern be dos/ParsePattern() */
            struct IClass *nd_ScreenClass;
            struct TagItem *nd_ScreenTags;
            IPTR nd_ScreenObjOffset;
            struct IClass *nd_MenuClass;
            struct TagItem *nd_MenuTags;
            IPTR nd_ScreenMenuObjOffset;
            struct IClass *nd_WindowClass;
            struct TagItem *nd_WindowTags;
            IPTR nd_ScreenWindowObjOffset;
};

struct DecoratorMessage
{
    struct MagicMessage dm_Message;
    IPTR               dm_Class;
    ULONG               dm_Code;
    ULONG               dm_Flags;
    IPTR                dm_Object;
};

#define     DECORATOR_VERSION  0

/* there is only one Message in the initial decoration system
   it will be sent to the decorator port to signal that it´ll not be used any longer
   and may be destroyed, in that case the dm_Object contains the NewDecorator struct
   Intuition does not touch anything, the decorator have to destroy all objects as well as the
   NewDecorator struct. */

#define DM_CLASS_DESTROYDECORATOR       0x8001

struct ScreenNotifyMessage
{
    struct MagicMessage snm_Message;
    ULONG               snm_Class;           /* Notification Class ID same as SNA_Notify */
    ULONG               snm_Code;            /* Code only supported for ScreenDepth() and will put the Flags in */
    IPTR                snm_Object;	     /* Pointer to the Object that caused this message */
    IPTR                snm_UserData;        /* will be filled with SNA_UserData */
};

#define SCREENNOTIFY_VERSION 0

#define SNA_PubName             (TAG_USER + 0x01) /* public screen name of NULL for all screens */
#define SNA_Notify              (TAG_USER + 0x02) /* Flags to look for see below */
#define SNA_UserData            (TAG_USER + 0x03) /* this tag will be passed to the screennotify message */
#define SNA_SigTask             (TAG_USER + 0x04) /* if port == NULL, a sigbit will be set for this task */
#define SNA_SigBit              (TAG_USER + 0x05) /* signal bit to set if port == NULL*/
#define SNA_MsgPort             (TAG_USER + 0x06) /* if != NULL post mesage to this port */
#define SNA_Priority            (TAG_USER + 0x07) /*  */
#define SNA_Hook                (TAG_USER + 0x08)

/* SNA_Notify (all unassigned bits are reserved for system use) */
#define SNOTIFY_AFTER_OPENSCREEN        (1<<0)  /* screen has been opened */
#define SNOTIFY_BEFORE_CLOSESCREEN      (1<<1)  /* going to close screen */
#define SNOTIFY_AFTER_OPENWB            (1<<2)  /* Workbench is open */
#define SNOTIFY_BEFORE_CLOSEWB          (1<<3)  /* Workbench is going to be closed */
#define SNOTIFY_AFTER_OPENWINDOW        (1<<4)  /* new window */
#define SNOTIFY_BEFORE_CLOSEWINDOW      (1<<5)  /* window is going to be closed */
#define SNOTIFY_PUBSCREENSTATE          (1<<6)  /* PubScreenState() */
#define SNOTIFY_LOCKPUBSCREEN           (1<<7)  /* LockPubScreen() */
#define SNOTIFY_SCREENDEPTH             (1<<8)  /* ScreenDepth() */
#define SNOTIFY_AFTER_CLOSESCREEN       (1<<9)	/* notify after CloseScreen() */
#define SNOTIFY_AFTER_CLOSEWINDOW       (1<<10)	/* dto. CloseWindow() */
#define SNOTIFY_BEFORE_OPENSCREEN       (1<<11)	/* notify before OpenScreen() */
#define SNOTIFY_BEFORE_OPENWINDOW       (1<<12)	/* dto. OpenWindow() */
#define SNOTIFY_BEFORE_OPENWB           (1<<13)	/* like OPENSCREEN */
#define SNOTIFY_AFTER_CLOSEWB           (1<<14)	/* like CLOSESCREEN */
#define SNOTIFY_WAIT_REPLY              (1<<15) /* wait for reply before taking action */
#define SNOTIFY_UNLOCKPUBSCREEN         (1<<16) /* UnlockPubScreen() */
#define SNOTIFY_BEFORE_UPDATEINTUITION  (1<<17) /* Intuition is going to be updated */
#define SNOTIFY_AFTER_UPDATEINTUITION   (1<<18) /* Intuition is updated */
#endif /* INTUITION_INTUITION_H */
