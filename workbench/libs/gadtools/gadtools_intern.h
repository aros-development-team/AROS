#ifndef GADTOOLS_INTERN_H
#define GADTOOLS_INTERN_H

/*
    Copyright (C) 1997-2001 AROS - The Amiga Research OS
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
	       
Class *makebuttonclass(struct GadToolsBase_intern *GadToolsBase);
Class *maketextclass(struct GadToolsBase_intern *GadToolsBase);
Class *makesliderclass(struct GadToolsBase_intern *GadToolsBase);
Class *makescrollerclass(struct GadToolsBase_intern *GadToolsBase);
Class *makearrowclass(struct GadToolsBase_intern *GadToolsBase);
Class *makestringclass(struct GadToolsBase_intern *GadToolsBase);
Class *makelistviewclass(struct GadToolsBase_intern *GadToolsBase);
Class *makecheckboxclass(struct GadToolsBase_intern *GadToolsBase);
Class *makecycleclass(struct GadToolsBase_intern *GadToolsBase);
Class *makemxclass(struct GadToolsBase_intern *GadToolsBase);
Class *makepaletteclass(struct GadToolsBase_intern *GadToolsBase);

/* Listview class has some data that must be freed */
VOID freelistviewclass(Class *cl, struct GadToolsBase_intern *GadToolsBase);

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
		      struct TagItem *taglist);

struct Gadget *maketext(struct GadToolsBase_intern *GadToolsBase,
		      	struct TagItem stdgadtags[],
		      	struct VisualInfo *vi,
		      	struct TextAttr *tattr,
		      	struct TagItem *taglist);

struct Gadget *makenumber(struct GadToolsBase_intern *GadToolsBase,
		      	  struct TagItem stdgadtags[],
		      	  struct VisualInfo *vi,
		      	  struct TextAttr *tattr,
		      	  struct TagItem *taglist);


struct Gadget *makeslider(struct GadToolsBase_intern *GadToolsBase,
		      	  struct TagItem stdgadtags[],
		      	  struct VisualInfo *vi,
		      	  struct TextAttr *tattr,
		      	  struct TagItem *taglist);

struct Gadget *makescroller(struct GadToolsBase_intern *GadToolsBase,
		      	  struct TagItem stdgadtags[],
		      	  struct VisualInfo *vi,
		      	  struct TagItem *taglist);

struct Gadget *makestring(struct GadToolsBase_intern *GadToolsBase,
		      	  struct TagItem stdgadtags[],
		      	  struct VisualInfo *vi,
		      	  struct TextAttr *tattr,
		      	  struct TagItem *taglist);

struct Gadget *makeinteger(struct GadToolsBase_intern *GadToolsBase,
		      	  struct TagItem stdgadtags[],
		      	  struct VisualInfo *vi,
		      	  struct TextAttr *tattr,
		      	  struct TagItem *taglist);

struct Gadget *makelistview(struct GadToolsBase_intern *GadToolsBase,
		      	  struct TagItem stdgadtags[],
		      	  struct VisualInfo *vi,
		      	  struct TextAttr *tattr,
		      	  struct TagItem *taglist);

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

struct Menu * makemenutitle(struct NewMenu * newmenu,
                            struct TagItem * taglist);

struct MenuItem * makemenuitem(struct NewMenu * newmenu,
                               BOOL is_image,
                               struct TagItem * taglist,
                               struct GadToolsBase_intern * GadToolsBase);

void appendmenu(struct Menu * firstmenu,
                struct Menu * lastmenu);

void appenditem(struct Menu * curmenu,
                struct MenuItem * item);

void appendsubitem(struct MenuItem * curitem,
                   struct MenuItem * subitem);

void freeitems(struct Menu * m, struct GadToolsBase_intern * GadToolsBase);

BOOL layoutmenuitems(struct MenuItem * menuitem,
                     struct VisualInfo * vi,
                     struct TagItem * taglist,
                     struct GadToolsBase_intern * GadToolsBase);

BOOL layoutsubitems(struct MenuItem * motheritem,
                    struct VisualInfo * vi,
                    struct TagItem * taglist,
                    struct GadToolsBase_intern * GadToolsBase);

/****************************************************************************************/

struct GadToolsBase_intern
{
    struct Library    		library;
    struct ExecBase 		* sysbase;
    BPTR	      		seglist;

    struct IntuitionBase 	* intuibase;
    struct Library	 	* dosbase;
    struct GfxBase	 	* gfxbase;
    struct Library		* layersbase;
    struct Library	 	* utilitybase;

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
    struct SignalSemaphore   	classsema;
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

#define BORDERPROPSPACINGX 	4
#define BORDERPROPSPACINGY 	4

#define BORDERSTRINGSPACINGX 	4
#define BORDERSTRINGSPACINGY 	2

#define LV_SHOWSELECTED_NONE 	((struct Gadget *)~0)

/****************************************************************************************/

/* The following typedefs are necessary, because the names of the global
   variables storing the library base pointers	and the corresponding
   structs are equal.
   This is a hack, of course. */
typedef struct IntuitionBase IntuiBase;
typedef struct GfxBase GraphicsBase;

#define GTB(gtb)        	((struct GadToolsBase_intern *)gtb)
/*
#undef SysBase
#define SysBase (GTB(GadToolsBase)->sysbase)
*/
extern struct ExecBase * SysBase;
#undef IntuitionBase
#define IntuitionBase 		(GTB(GadToolsBase)->intuibase)
#undef DOSBase
#define DOSBase 		(GTB(GadToolsBase)->dosbase)
#undef GfxBase
#define GfxBase 		(GTB(GadToolsBase)->gfxbase)
#undef LayersBase
#define LayersBase 		(GTB(GadToolsBase)->layersbase)
#undef UtilityBase
#define UtilityBase 		(GTB(GadToolsBase)->utilitybase)

#define expunge() \
AROS_LC0(BPTR, expunge, struct GadToolsBase_intern *, GadToolsBase, 3, GadTools)





#endif /* GADTOOLS_INTERN_H */
