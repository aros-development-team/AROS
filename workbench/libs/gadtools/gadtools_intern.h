#ifndef GADTOOLS_INTERN_H
#define GADTOOLS_INTERN_H

/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal definitions for gadtools.library.
    Lang: english
*/

#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef EXEC_SEMAPHORES_H
#   include <exec/semaphores.h>
#endif
#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif
#ifndef PROTO_GRAPHICS_H
#   include <proto/graphics.h>
#endif
#ifndef GRAPHICS_GFXBASE_H
#   include <graphics/gfxbase.h>
#endif
#ifndef PROTO_INTUITION_H
#   include <proto/intuition.h>
#endif
#ifndef INTUITION_CLASSES_H
#   include <intuition/classes.h>
#endif
#ifndef INTUITION_INTUITIONBASE_H
#   include <intuition/intuitionbase.h>
#endif
#ifndef INTUITION_SCREENS_H
#   include <intuition/screens.h>
#endif
#ifndef PROTO_UTILITY_H
#   include <proto/utility.h>
#endif
#ifndef LIBRARIES_GADTOOLS_H
#   include <libraries/gadtools.h>
#endif
#ifndef GADGETS_AROSMX_H
#   include <gadgets/arosmx.h>
#endif

/* Needed for aros_print_not_implemented macro */
#include <aros/debug.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>

/****************************************************************************************/

struct VisualInfo;

/* Some external stuff (gadtools_init.c) */
struct GadToolsBase_intern; /* prerefrence */

/* Internal prototypes */
struct IntuiText *makeitext(struct GadToolsBase_intern *GadToolsBase,
			    struct NewGadget *ng,
			    struct TagItem *taglist);
void freeitext(struct GadToolsBase_intern *GadToolsBase,
	       struct IntuiText *itext);
	       
BOOL renderlabel(struct GadToolsBase_intern *GadToolsBase,
		 struct Gadget *gad, struct RastPort *rport, LONG labelplace);
void DoDisabledPattern(struct RastPort *rp, WORD x1, WORD y1, WORD x2, WORD y2,
		       WORD pen, struct GadToolsBase_intern *GadToolsBase);

/****************************************************************************************/
	       
struct Gadget *makebutton(struct GadToolsBase_intern *GadToolsBase,
			  struct TagItem stdgadtags[],
			  struct VisualInfo *vi,
			  struct TagItem *taglist);

struct Gadget *makecheckbox(struct GadToolsBase_intern *GadToolsBase,
			    struct TagItem stdgadtags[],
			    struct VisualInfo *vi,
			    struct TagItem *taglist);

struct Gadget *makecycle(struct GadToolsBase_intern *GadToolsBase,
                         struct TagItem stdgadtags[],
                         struct VisualInfo *vi,
		      	 struct TextAttr *tattr,
                         struct TagItem *taglist);

struct Gadget *makemx(struct GadToolsBase_intern *GadToolsBase,
		      struct TagItem stdgadtags[],
		      struct VisualInfo *vi,
		      struct TextAttr *tattr,
		      struct TagItem *taglist);

struct Gadget *makepalette(struct GadToolsBase_intern *GadToolsBase,
		      struct TagItem stdgadtags[],
		      struct VisualInfo *vi,
		      const struct TagItem *taglist);

struct Gadget *maketext(struct GadToolsBase_intern *GadToolsBase,
		      	struct TagItem stdgadtags[],
		      	struct VisualInfo *vi,
		      	struct TextAttr *tattr,
		      	const struct TagItem *taglist);

struct Gadget *makenumber(struct GadToolsBase_intern *GadToolsBase,
		      	  struct TagItem stdgadtags[],
		      	  struct VisualInfo *vi,
		      	  struct TextAttr *tattr,
		      	  const struct TagItem *taglist);


struct Gadget *makeslider(struct GadToolsBase_intern *GadToolsBase,
		      	  struct TagItem stdgadtags[],
		      	  struct VisualInfo *vi,
		      	  struct TextAttr *tattr,
		      	  const struct TagItem *taglist);

struct Gadget *makescroller(struct GadToolsBase_intern *GadToolsBase,
		      	  struct TagItem stdgadtags[],
		      	  struct VisualInfo *vi,
		      	  const struct TagItem *taglist);

struct Gadget *makestring(struct GadToolsBase_intern *GadToolsBase,
		      	  struct TagItem stdgadtags[],
		      	  struct VisualInfo *vi,
		      	  struct TextAttr *tattr,
		      	  const struct TagItem *taglist);

struct Gadget *makeinteger(struct GadToolsBase_intern *GadToolsBase,
		      	  struct TagItem stdgadtags[],
		      	  struct VisualInfo *vi,
		      	  struct TextAttr *tattr,
		      	  const struct TagItem *taglist);

struct Gadget *makelistview(struct GadToolsBase_intern *GadToolsBase,
		      	  struct TagItem stdgadtags[],
		      	  struct VisualInfo *vi,
		      	  struct TextAttr *tattr,
		      	  const struct TagItem *taglist);

struct Gadget *makegeneric(struct GadToolsBase_intern *GadToolsBase,
		      	  struct TagItem stdgadtags[],
		      	  struct VisualInfo *vi,
		      	  struct TextAttr *tattr,
		      	  struct TagItem *taglist);

/****************************************************************************************/
		      	  
/* Tags for the private gadtools classes */

#define GT_Dummy 			(TAG_USER)

#define GTA_Text_DispFunc	  	(GT_Dummy + 1)
#define GTA_Text_Format		  	(GT_Dummy + 2)
#define GTA_Arrow_Type		  	(GT_Dummy + 3)
#define GTA_Arrow_Pulse		  	(GT_Dummy + 4)
#define GTA_Arrow_Scroller	  	(GT_Dummy + 5)
#define GTA_Scroller_Dec	  	(GT_Dummy + 6)
#define GTA_Scroller_Inc	  	(GT_Dummy + 7)
#define GTA_Listview_Scroller	  	(GT_Dummy + 8)
#define GTA_GadgetKind		  	(GT_Dummy + 9)
#define GTA_ChildGadgetKind	  	(GT_Dummy + 10)
#define GTA_Scroller_ScrollerKind 	(GT_Dummy + 11)
#define GTA_Scroller_ArrowKind	  	(GT_Dummy + 12)
#define GTA_Scroller_Arrow1       	(GT_Dummy + 13)
#define GTA_Scroller_Arrow2       	(GT_Dummy + 14)
#define GTA_Text_ParentGadget  	    	(GT_Dummy + 15)

/****************************************************************************************/

/* private gadget kinds */

#define _ARROW_KIND   			100

/****************************************************************************************/

/* Some listview specific constants */
#define LV_BORDER_X 			4
#define LV_BORDER_Y 			3

#define LV_DEF_INTERNAL_SPACING 	0

/****************************************************************************************/

/* Private MX tags */

#define GTMX_TickLabelPlace AROSMX_TickLabelPlace
#define GTMX_TickHeight     AROSMX_TickHeight

/****************************************************************************************/

#define	NEWMENUCODE	TRUE

#if NEWMENUCODE
struct Menu * makemenutitle(struct NewMenu * newmenu,
                            UBYTE		**MyMenuMemPtr,
                            struct TagItem * taglist);

struct MenuItem * makemenuitem(struct NewMenu * newmenu,
                            UBYTE		**MyMenuMemPtr,
                            struct Image	***MyBarTablePtr,
                               BOOL is_image,
                               struct TagItem * taglist,
                               struct GadToolsBase_intern * GadToolsBase);

ULONG	getmenutitlesize(struct NewMenu * newmenu,
                          struct TagItem * taglist);
ULONG	getmenuitemsize(struct NewMenu * newmenu,
                                   BOOL is_image,
                                   struct TagItem * taglist,
                                   struct GadToolsBase_intern * GadToolsBase);

#else

struct Menu * makemenutitle(struct NewMenu * newmenu,
                            struct TagItem * taglist);

struct MenuItem * makemenuitem(struct NewMenu * newmenu,
                               BOOL is_image,
                               struct TagItem * taglist,
                               struct GadToolsBase_intern * GadToolsBase);
#endif
void appendmenu(struct Menu * firstmenu,
                struct Menu * lastmenu);

void appenditem(struct Menu * curmenu,
                struct MenuItem * item);

void appendsubitem(struct MenuItem * curitem,
                   struct MenuItem * subitem);

void freeitems(struct MenuItem * mi, struct GadToolsBase_intern * GadToolsBase);

BOOL layoutmenuitems(struct MenuItem * menuitem,
                     struct VisualInfo * vi,
                     struct TagItem * taglist,
                     struct GadToolsBase_intern * GadToolsBase);

BOOL layoutsubitems(struct MenuItem * motheritem,
                    struct VisualInfo * vi,
                    struct TagItem * taglist,
                    struct GadToolsBase_intern * GadToolsBase);

BOOL is_menubarlabelclass_image(struct Image *im,
    	    	    	    	struct GadToolsBase_intern *GadToolsBase);

/****************************************************************************************/

struct GadToolsBase_intern
{
    struct Library                lib;

    Class 			* buttonclass;
    Class 			* textclass;
    Class 			* sliderclass;
    Class 			* scrollerclass;
    Class 			* arrowclass;
    Class 			* stringclass;
    Class 			* listviewclass;
    Class 			* checkboxclass;
    Class 			* cycleclass;
    Class 			* mxclass;
    Class 			* paletteclass;
    
    /* Semaphore to protect the bevel object. */
    struct SignalSemaphore   	bevelsema;
    /* Actually an Object *. The image used for bevel boxes. */
    struct Image           	* bevel;
    
    /* RenderHook for GTListView class */
    struct Hook                 lv_RenderHook;

    /* Seglist pointer */
    BPTR			gt_SegList;

    /* Required libraies */
    APTR			gt_IntuitionBase;
    APTR			gt_UtilityBase;
    APTR			gt_GfxBase;
    APTR			gt_LayersBase;
};

/* extended intuimsg as used by GT_GetIMsg, GT_FilterIMsg, ... */

struct GT_IntuiMessage
{
    struct ExtIntuiMessage 	imsg;
    struct IntuiMessage 	* origmsg;
    BOOL 			wasalloced;
};

#define VI(x) 			((struct VisualInfo *)x)

struct VisualInfo
{
    struct Screen  		* vi_screen;
    struct DrawInfo 		* vi_dri;
};

/* dummy gadget created by CreateContext */

struct GT_ContextGadget
{
    struct ExtGadget 		gad;
    IPTR			magic;
    IPTR			magic2;
    struct GT_IntuiMessage 	gtmsg;
    struct Gadget 		*activegadget;
    struct Gadget 		*parentgadget;
    IPTR 			gadget_value;
    IPTR 			gadgetkind;
    IPTR 			childgadgetkind;
    IPTR 			childinfo;
    ULONG 			getattrtag;
    ULONG 			setattrtag;
    WORD 			scrollticker;
};

struct GT_GenericGadget
{
    struct ExtGadget		gad;
    IPTR			magic;
    IPTR			magic2;
    struct IntuiText		*itext;
};

/****************************************************************************************/

#define CONTEXT_MAGIC		((IPTR)0x11223344)
#define CONTEXT_MAGIC2		((IPTR)0x44332211)

#define GENERIC_MAGIC		((IPTR)0x11335577)
#define GENERIC_MAGIC2		((IPTR)0x77553311)

/****************************************************************************************/

#define TAG_Left		0
#define TAG_Top 		1
#define TAG_Width		2
#define TAG_Height		3
#define TAG_IText		4
#define TAG_LabelPlace		5
#define TAG_Previous		6
#define TAG_ID			7
#define TAG_DrawInfo		8
#define TAG_UserData		9
#define TAG_Num        		10


#define TAG_Menu		0
#define TAG_TextAttr		1
#define TAG_NewLookMenus	2
#define TAG_CheckMark		3
#define TAG_AmigaKey		4
#define TAG_FrontPen		5

#define BORDERPROPSPACINGX 	2
#define BORDERPROPSPACINGY 	2

#define BORDERSTRINGSPACINGX 	4
#define BORDERSTRINGSPACINGY 	2

#define LV_SHOWSELECTED_NONE 	((struct Gadget *)~0)

/****************************************************************************************/

#define GTB(gtb)        	((struct GadToolsBase_intern *)gtb)

extern struct ExecBase *SysBase;
/* FIXME: Remove these #define xxxBase hacks
   Do not use this in new code !
*/
#define IntuitionBase	(GTB(GadToolsBase)->gt_IntuitionBase)
#define UtilityBase	(GTB(GadToolsBase)->gt_UtilityBase)
#define GfxBase		(GTB(GadToolsBase)->gt_GfxBase)
#define LayersBase	(GTB(GadToolsBase)->gt_LayersBase)


#ifdef __MORPHOS__
#define DeinitRastPort(x) ((void)0)

#define DoMethod(MyObject, tags...) \
	({ULONG _tags[] = { tags }; DoMethodA((MyObject), (APTR)_tags);})

#define CoerceMethod(MyClass, MyObject, tags...) \
	({ULONG _tags[] = { tags }; CoerceMethodA((MyClass), (MyObject), (APTR)_tags);})

#define DoSuperMethod(MyClass, MyObject, tags...) \
	({ULONG _tags[] = { tags }; DoSuperMethodA((MyClass), (MyObject), (APTR)_tags);})

/********************************************************************************/
/* imageclass.h AROS extensions */

#define SYSIA_WithBorder  IA_FGPen      /* default: TRUE */
#define SYSIA_Style       IA_BGPen      /* default: SYSISTYLE_NORMAL */

#define SYSISTYLE_NORMAL   0
#define SYSISTYLE_GADTOOLS 1            /* to get arrow images in gadtools look */

/********************************************************************************/
/* gadgetclass.h AROS extenstions */

/* This method is invoked to learn about the sizing requirements of your class,
   before an object is created. This is AROS specific. */
#define GM_DOMAIN 7
struct gpDomain
{
    STACKED ULONG          MethodID;   /* GM_DOMAIN */
    STACKED struct GadgetInfo * gpd_GInfo;  /* see <intuition/cghooks.h> */
    STACKED struct RastPort   * gpd_RPort;  /* RastPort to calculate dimensions for. */
    STACKED LONG           gpd_Which;  /* see below */
    STACKED struct IBox         gpd_Domain; /* Resulting domain. */
    STACKED struct TagItem    * gpd_Attrs;  /* Additional attributes. None defined,
                                       yet. */
};

/********************************************************************************/

/* gpd_Which */
#define GDOMAIN_MINIMUM 0 /* Calculate minimum size. */
#define GDOMAIN_NOMINAL 1 /* Calculate nominal size. */
#define GDOMAIN_MAXIMUM 2 /* Calculate maximum size. */

/********************************************************************************/

  /* [IS.] (struct TextAttr *) TextAttr structure (see <graphics/text.h>) to
     use for gadget rendering. This attribute is not directly supported by
     GadgetClass. */
#define GA_TextAttr      (GA_Dummy + 40)

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

/********************************************************************************/

#define MENUBARLABELCLASS "menubarlabelclass"

#define GTYP_GADTOOLS 0x0100

#define	CORRECT_LISTVIEWHEIGHT	TRUE

#else /*MorphOS*/

#define	CORRECT_LISTVIEWHEIGHT	FALSE

#endif

#define DEBUG_CREATECONTEXT(x)	;
#define DEBUG_CREATEGADGETA(x)	;
#define DEBUG_CREATEMENUSA(x)   ;
#define	DEBUG_CREATELISTVIEW(x)	;
#define	DEBUG_CREATESCROLLER(x)	;
#define DEBUG_FREEGADGETS(x)	;
#define DEBUG_DUMPMENUS(x)	;
#define DEBUG_ALLOCMENUS(x)	;
#define DEBUG_FREEMENUS(x)	;
#define DEBUG_REFRESHWINDOW(x)	;

void	DumpMenu(struct Menu *menu);

/* Structures for the class private data */

/* GTButton class */
struct ButtonData
{
    struct DrawInfo 	*dri;
    struct Image 	*frame;
};

/* GTText class */
struct TextData
{
    struct DrawInfo 	*dri;
    struct Gadget   	*parentgadget;
    Object 		*frame;
    STRPTR 		format;
    IPTR 		toprint;
    UBYTE 		frontpen;
    UBYTE 		backpen;
    UBYTE 		justification;
    UBYTE 		flags;
    struct TextFont 	*font;
    UWORD 		maxnumberlength;
    WORD 		gadgetkind;
    IPTR  		(*dispfunc)(struct Gadget *, WORD);
    UBYTE 		labelplace;
    
};

/* GTSlider class */
struct SliderData
{
    Object 	*frame;
    WORD  	min;
    WORD  	max;
    WORD  	level;
    WORD        freedom;
    UBYTE 	labelplace;
};

/* GTScroller class */
struct ScrollerData
{
    Object 		*frame;
    struct Gadget 	*arrow1;
    struct Gadget	*arrow2;
    WORD 		gadgetkind;
    UBYTE 		labelplace;
};

/* GTArrow class */
struct ArrowData
{
    Object 	*arrowimage;
    Object 	*frame;
    Object	*scroller;
    WORD 	gadgetkind;
    WORD 	arrowtype;
};

/* GTString class */
struct StringData
{
    Object		*frame;
    struct TextFont 	*font;
    WORD		gadgetkind;
    UBYTE		labelplace;
};

/* GTListView class */
struct LVData
{
    struct Hook 	*ld_CallBack;
    struct List		*ld_Labels;
    struct DrawInfo 	*ld_Dri;
    struct TextFont 	*ld_Font;
    Object		*ld_Frame;
    Object		*ld_Scroller;
    struct Gadget	*ld_ShowSelected;
    WORD		ld_Top;
    WORD		ld_Selected;
    WORD		ld_Spacing;
    WORD		ld_ItemHeight;
    
    /* The number of first damaged entry, counting from first visible.
    ** A value o -1 means "nothing has to be redrawn"
    */
    WORD		ld_FirstDamaged;
    UWORD		ld_NumDamaged;

    /* Number of entries the listview should scroll in GM_RENDER.
    ** Negative valu means scroll up.
    */
    WORD		ld_ScrollEntries;
    WORD		ld_NumEntries;

    UBYTE		ld_Flags;
    UBYTE		ld_LabelPlace;
};

/* GTCheckBox class */
struct CheckBoxData
{
    struct DrawInfo *dri;
    LONG    	    labelplace;
    LONG    	    flags;
};

/* GTCycle class */
struct CycleData
{
    struct TextFont *font;
    STRPTR  	    *labels;
    UWORD   	    active;
    UWORD   	    numlabels;
    UWORD   	    labelplace;
};

/* GTMX class */
struct MXData
{
    struct DrawInfo 	*dri;
    struct TextAttr 	*tattr;
    struct Image    	*mximage;
    struct TextFont 	*font;
    struct Rectangle	bbox;
    STRPTR  	    	*labels;
    ULONG   	    	active, newactive;  	    /* The active tick and the tick to be activated */
    ULONG   	    	numlabels;  	    	    /* The number of labels */
    LONG    	    	labelplace, ticklabelplace;
    UWORD   	    	fontheight;
    UWORD   	    	spacing;
    UWORD   	    	maxtextwidth;
};

/* GTPalette class */
struct PaletteData
{
    UWORD		pd_NumColors;
    UBYTE		*pd_ColorTable;
    UBYTE		pd_Color;
    /* For state info, to know what selected entry to delete in GM_RENDER, GREDRAW_UPDATE */
    UBYTE		pd_OldColor; 
    
    UBYTE		pd_ColorOffset;
    UWORD		pd_IndWidth;
    UWORD		pd_IndHeight;

    struct IBox 	pd_GadgetBox;	 /* Box surrounding whole palette 	*/
    struct IBox 	pd_PaletteBox;	 /* Box surrounding palette 		*/
    struct IBox 	pd_IndicatorBox; /* Box surrounding indicator		*/
    
    UWORD		pd_ColWidth;
    UWORD		pd_RowHeight;
    UBYTE		pd_NumCols;
    UBYTE		pd_NumRows;

    /* This one is used to backup pd_Color if left is clicked. This
    ** way the old state can be restored if the left is released
    ** outside the gadget.
    */
    UBYTE		pd_ColorBackup;
    struct TextAttr 	*pd_TAttr;
    LONG		pd_LabelPlace;
    Object		*pd_Frame;
};


#endif /* GADTOOLS_INTERN_H */
