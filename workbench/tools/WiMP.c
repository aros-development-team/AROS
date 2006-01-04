/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Window Manipulation Program
    Lang: English
*/

/*****************************************************************************

    NAME

        WiMP

    SYNOPSIS

    LOCATION

	Workbench:Tools

    FUNCTION

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

        11-Dec-2000     hkiel     Initial version
        01-May-2001	petah     Added CTRL-C checking (WiMP 0.8)

******************************************************************************/

static const char version[] = "$VER: WiMP 0.9 (10.08.2003)";


#include <stdio.h>
#include <stdlib.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>

struct Screen *Screen;
struct Window *Window;
struct Window *InfoWindow = NULL;
struct RastPort *iw_rp;
struct Menu *menus;
ULONG IDCMP;
ULONG lock;
ULONG w_sigbit, iw_sigbit;
APTR vi;

enum {
  None_type,
  Window_type,
  Screen_type,
  Max_type
};

#define ClearIDCMP()		\
{				\
  IDCMP = Window->IDCMPFlags;	\
  Window->IDCMPFlags = 0UL;	\
}
#define ResetIDCMP()		\
{				\
  Window->IDCMPFlags = IDCMP;	\
}

#define ID_SHOW 10
struct NewGadget showgad =
{
    520, 130, 50, 20,
    "Show", NULL,
    ID_SHOW, PLACETEXT_IN, NULL, NULL
};

#define ID_HIDE 11
struct NewGadget hidegad =
{
    470, 130, 50, 20,
    "Hide", NULL,
    ID_HIDE, PLACETEXT_IN, NULL, NULL
};

#define ID_ZIP 12
struct NewGadget zipgad =
{
    420, 130, 50, 20,
    "Zip", NULL,
    ID_ZIP, PLACETEXT_IN, NULL, NULL
};

#define ID_ACTIVATE 13
struct NewGadget activategad =
{
    340, 130, 80, 20,
    "Activate", NULL,
    ID_ACTIVATE, PLACETEXT_IN, NULL, NULL
};

#define ID_ORIGIN 14
struct NewGadget origingad =
{
    210, 130, 130, 20,
    "Move to Origin", NULL,
    ID_ORIGIN, PLACETEXT_IN, NULL, NULL
};

#define ID_BACK 15
struct NewGadget backgad =
{
    140, 130, 70, 20,
    "To Back", NULL,
    ID_BACK, PLACETEXT_IN, NULL, NULL
};

#define ID_FRONT 16
struct NewGadget frontgad =
{
    60, 130, 80, 20,
    "To Front", NULL,
    ID_FRONT, PLACETEXT_IN, NULL, NULL
};

#define ID_KILL 17
struct NewGadget killgad =
{
    10, 130, 50, 20,
    "Close", NULL,
    ID_KILL, PLACETEXT_IN, NULL, NULL
};

#define ID_ABOUT 18
struct NewGadget aboutgad =
{
    520, 150, 50, 20,
    "About", NULL,
    ID_ABOUT, PLACETEXT_IN, NULL, NULL
};

#define ID_RETHINK 19
struct NewGadget rethinkgad =
{
    400, 150, 120, 20,
    "RethinkDisplay", NULL,
    ID_RETHINK, PLACETEXT_IN, NULL, NULL
};

#define ID_SHOWALL 20
struct NewGadget showallgad =
{
    265, 150, 135, 20,
    "Show all Windows", NULL,
    ID_SHOWALL, PLACETEXT_IN, NULL, NULL
};

#define ID_RESCUE 21
struct NewGadget rescuegad =
{
    110, 150, 155, 20,
    "Rescue all Windows", NULL,
    ID_RESCUE, PLACETEXT_IN, NULL, NULL
};

#define ID_UPDATE 22
struct NewGadget updategad =
{
    10, 150, 100, 20,
    "Update List", NULL,
    ID_UPDATE, PLACETEXT_IN, NULL, NULL
};

#define ID_LISTVIEW 23
struct NewGadget listviewgad =
{
    10, 5, 561, 125,
    "Screen/Window List", NULL,
    ID_LISTVIEW, PLACETEXT_ABOVE, NULL, NULL
};


struct List lv_list, lv_infolist;

struct Gadget *glist = NULL;
struct Gadget *igads = NULL;
struct Gadget *screenlistg = NULL;
struct Gadget *actiong = NULL;
#define ACTIONGLENW 8
#define ACTIONGLENS 4
UWORD actionmenu[] =
{
	FULLMENUNUM(1,9,NOSUB),
	FULLMENUNUM(1,8,NOSUB),
	FULLMENUNUM(1,7,NOSUB),
	FULLMENUNUM(1,6,NOSUB),
	FULLMENUNUM(1,5,NOSUB),
	FULLMENUNUM(1,4,NOSUB),
	FULLMENUNUM(1,3,NOSUB),
	FULLMENUNUM(1,2,NOSUB)
};

static struct NewMenu nm[] =
{
  {NM_TITLE, "Project"},
    {NM_ITEM, "About..."},
    {NM_ITEM, NM_BARLABEL},
    {NM_ITEM, "Quit", "Q"},
  {NM_TITLE, "Window List"},
    {NM_ITEM, "Update List"},
    {NM_ITEM, NM_BARLABEL},
    {NM_ITEM, "Kill"},
    {NM_ITEM, "To Front"},
    {NM_ITEM, "To Back"},
    {NM_ITEM, "To Origin"},
    {NM_ITEM, "Activate"},
    {NM_ITEM, "Zip"},
    {NM_ITEM, "Hide"},
    {NM_ITEM, "Show"},
    {NM_ITEM, NM_BARLABEL},
    {NM_ITEM, "Info"},
  {NM_TITLE, "Generic"},
    {NM_ITEM, "Rescue All"},
    {NM_ITEM, "Show All"},
    {NM_ITEM, "RethinkDisplay"},
  {NM_END}
};

#define EASYTRUE 1
const STRPTR TITLE_TXT		= "WiMP - The Window Manipulation Program";
const STRPTR INFOTITLE_TXT	= "WiMP - InfoWindow";
const STRPTR CLOSESCREEN_TXT	= "Do you really want to Close the selected Screen?";
const STRPTR CLOSEWINDOW_TXT	= "Do you really want to Close the selected Window?";
const STRPTR ABOUT_TXT		= "WiMP - The Window Manipulation Program\n\nCopyright 2000-2001 by Henning Kiel\nhkiel@aros.org\n\nThis program is part of AROS - The Amiga Research OS";
const STRPTR YESNO_TXT		= "Yes.|No!";
const STRPTR CONTINUE_TXT	= "Continue";

/* Internal Prototypes */
VOID initlvnodes(struct List *list);
VOID freelvnodes(struct List *list);
VOID update_list();
struct Gadget *gt_init();
IPTR getsw(int *type);
VOID update_actionglist();
VOID makemenus();
struct Gadget *makegadgets(struct Gadget *gad);
VOID open_window();
VOID close_window();
VOID open_infowindow();
VOID close_infowindow();
VOID WindowInfo( struct Window *win );
VOID ScreenInfo( struct Screen *scr );
VOID rescue_all();
VOID show_all();


VOID initlvnodes(struct List *list)
{
struct Screen *scr;
struct Window *win;
struct Node *node;
char tmp[1024];
char *string;

  /* Get Intuition's first Screen */
  lock = LockIBase ( 0 );
  scr = IntuitionBase->FirstScreen;
  UnlockIBase ( lock );

  /* Traverse through all Screens */
  while ( scr )
  {
    sprintf (	tmp,
		"Screen:   %p %4dx%4d @%4d.%4d     \"%s\",\"%s\"",
		scr,
		scr->Width,
		scr->Height,
		scr->LeftEdge,
		scr->TopEdge,
		scr->Title,
		scr->DefaultTitle
	    );
    string = StrDup ( tmp );
    node = (struct Node *) AllocMem ( sizeof(struct Node), MEMF_CLEAR );
    AddTail ( list, node );
    SetNodeName ( node, string );

    /* Traverse through all Windows of current Screen */
    win = scr->FirstWindow;
    while ( win )
    {
      sprintf ( tmp,
		"  Window: %p %4dx%4d @%4d,%4d %c%c%c \"%s\"%c(%p)",
		win,
		win->Width,
		win->Height,
		win->LeftEdge,
		win->TopEdge,
		(IsWindowVisible(win)?' ':'H'),
		(IS_CHILD(win)?'C':' '),
		(HAS_CHILDREN(win)?'P':' '),
		win->Title,
		(IS_CHILD(win)?' ':0),
		win->parent
	      );
      string = StrDup ( tmp );
      node = (struct Node *) AllocMem ( sizeof(struct Node), MEMF_CLEAR );
      AddTail ( list, node );
      SetNodeName ( node, string );
      win = win->NextWindow;
    }
    scr = scr->NextScreen;
  }

return;
}

VOID freelvnodes(struct List *list)
{
struct Node *popnode;

  while ( IsListEmpty ( list ) != TRUE )
  {
    popnode = RemTail ( list );

    FreeVec ( popnode->ln_Name );
    FreeMem ( popnode, sizeof ( struct Node ) );
  }

return;
}

VOID update_list()
{
  /* Detach List from Gadget */
  GT_SetGadgetAttrs ( screenlistg, Window, NULL,
      GTLV_Labels, (IPTR)~0,
      TAG_DONE );

  /* Recalculate List */
  freelvnodes( &lv_list );
  initlvnodes( &lv_list );

  /* Attach List to Gadget */
  GT_SetGadgetAttrs ( screenlistg, Window, NULL,
      GTLV_Labels,	(IPTR)&lv_list,
      GTLV_Selected,	(IPTR)~0,
      TAG_DONE );
  update_actionglist();
  GT_RefreshWindow ( Window, NULL );

return;
}

struct Gadget *gt_init()
{
    struct Gadget *gad = NULL;
    
    Screen = LockPubScreen ( NULL );
    vi = GetVisualInfoA ( Screen, NULL );
    if ( vi != NULL )
	gad = CreateContext ( &glist );
	
    return gad;
}

IPTR getsw(int *type)
{
IPTR gadget;
IPTR xptr = 0;
struct Screen *scr;
struct Window *win = NULL;

  GT_GetGadgetAttrs ( screenlistg, Window, NULL,
      GTLV_Selected, (IPTR)&gadget,
      TAG_DONE );
  if ( gadget != -1 )
  {
  struct Node *xnode;
  char *xnodename;

    xnode = (struct Node *) GetHead ( &lv_list );
    for ( ; gadget > 0 ; gadget-- )
    {
      xnode = GetSucc ( xnode );
    }
    xnodename = GetNodeName ( xnode );
    switch ( xnodename[2] )
    {
      case 'r' : /* "Screen:" */
	  *type = Screen_type;
	  xptr = (IPTR) strtol ( &(xnodename[12]), NULL, 16 );
	  break;
      case 'W' : /* "  Window:" */
	  *type = Window_type;
	  xptr = (IPTR) strtol ( &(xnodename[12]), NULL, 16 );
	  break;
      default:
	  break;
    }
    lock = LockIBase ( 0 );
    scr = IntuitionBase->FirstScreen;
    UnlockIBase ( lock );
    /* Traverse through all Screens */
    while ( scr && scr != (struct Screen *)xptr )
    {
      /* Traverse through all Windows of current Screen */
      win = scr->FirstWindow;
      while ( win && win != (struct Window *)xptr )
      {
	win = win->NextWindow;
      }
      scr = scr->NextScreen;
    }
    if ( ( win == (struct Window *)xptr && *type == Window_type )
      || ( scr == (struct Screen *)xptr && *type == Screen_type ) )
    {
      return xptr;
    }
    else
    {
      update_list();
      *type = None_type;
      return 0;
    }
  }
  else
  {
    *type = None_type;
    return 0;
  }
}

VOID update_actionglist()
{
struct Gadget *gad;
int i, type, max;

  getsw ( &type );
  gad = actiong;

  if ( type == Screen_type )
  {
    max = ACTIONGLENS;
  }
  else if ( type == None_type )
  {
    max = ACTIONGLENW;
  }
  else
  {
    max = 0;
  }
  for ( i = ACTIONGLENW ; i > 0 ; i-- )
  {
    if ( i > max )
    {
      OnGadget ( gad, Window, NULL );
      OnMenu ( Window, actionmenu[i-1] );
    }
    else
    {
      OffGadget ( gad, Window, NULL );
      OffMenu ( Window, actionmenu[i-1] );
    }
    gad = gad->NextGadget;
  }
  if ( type == None_type )
  {
    OffMenu ( Window, FULLMENUNUM ( 1, 11, NOSUB ) );
  }
  else
  {
    OnMenu ( Window, FULLMENUNUM ( 1, 11, NOSUB ) );
  }

return;
}

VOID makemenus()
{
    menus = CreateMenusA ( nm, NULL );
    LayoutMenusA ( menus, vi, NULL );
    SetMenuStrip ( Window, menus );

return;
}

struct Gadget *makegadgets(struct Gadget *gad)
{
int i;
int topborder, leftborder;
struct TextAttr fixedfontattr;
struct NewGadget *buttons[] =
{
  &updategad,
  &rescuegad,
  &showallgad,
  &rethinkgad,
  &aboutgad,
  &killgad,
  &frontgad,
  &backgad,
  &origingad,
  &activategad,
  &zipgad,
  &hidegad,
  &showgad
};
#define BUTTONALEN	6
#define BUTTONBLEN	7
#define BUTTONLEN	( BUTTONALEN + BUTTONBLEN )

  topborder = Window->WScreen->WBorTop + Window->WScreen->Font->ta_YSize + 1;
  leftborder = Window->WScreen->WBorLeft;
  AskFont ( Window->RPort, &fixedfontattr );
  listviewgad.ng_TextAttr = &fixedfontattr;
  listviewgad.ng_VisualInfo = vi;
  listviewgad.ng_LeftEdge += leftborder;
  listviewgad.ng_TopEdge += topborder + Window->WScreen->Font->ta_YSize + 1;
  listviewgad.ng_Height -= Window->WScreen->Font->ta_YSize + 1;
  for ( i = 0 ; i < BUTTONLEN ; i++ )
  {
    buttons[i]->ng_VisualInfo = vi;
    buttons[i]->ng_LeftEdge += leftborder;
    buttons[i]->ng_TopEdge += topborder;
  }
    
  freelvnodes( &lv_list );
  initlvnodes ( &lv_list );
  gad = CreateGadget ( LISTVIEW_KIND, gad, &listviewgad,
		GTLV_Labels,		(IPTR)&lv_list,
		GTLV_ShowSelected,	(IPTR)NULL,
		GTLV_ReadOnly,		FALSE,
		TAG_DONE );
  screenlistg = gad;

  for ( i = 0 ; i < BUTTONALEN ; i++ )
  {
    gad = CreateGadget ( BUTTON_KIND, gad, buttons[i],
		TAG_DONE );
  }
  actiong = gad;

  for ( ; i < BUTTONLEN ; i++ )
  {
    gad = CreateGadget ( BUTTON_KIND, gad, buttons[i],
		TAG_DONE );
  }

  if ( gad == NULL )
  {
    FreeGadgets ( glist );
    printf ( "GTDemo: Error creating gadgets\n" );
  }

return gad;
}

VOID open_window()
{
  Window = OpenWindowTags ( NULL
	, WA_Title,	    (Tag)TITLE_TXT
	, WA_Left,	    0
	, WA_Top,	    0
	, WA_InnerWidth,    580
	, WA_InnerHeight,   180
	, WA_IDCMP,	    IDCMP_REFRESHWINDOW
			    | IDCMP_MOUSEBUTTONS
			    | IDCMP_GADGETUP
			    | IDCMP_MENUPICK
			    | IDCMP_CLOSEWINDOW
			    | LISTVIEWIDCMP
			    | BUTTONIDCMP
	, WA_Flags,	    WFLG_DRAGBAR
			    | WFLG_DEPTHGADGET
			    | WFLG_CLOSEGADGET
			    | WFLG_NOCAREREFRESH
			    | WFLG_SMART_REFRESH
			    | WFLG_ACTIVATE
	, WA_SimpleRefresh, TRUE
	, TAG_END
    );
    w_sigbit = 1 << Window->UserPort->mp_SigBit;

return;
}

VOID close_window()
{
  ClearMenuStrip ( Window );
  FreeMenus ( menus );
  CloseWindow ( Window );
  freelvnodes ( &lv_list );
  FreeGadgets ( glist );
  FreeVisualInfo ( vi );
  UnlockPubScreen ( NULL, Screen );

return;
}

VOID open_infowindow()
{
struct TextAttr fixedfontattr;
struct Gadget *gad = NULL;
int topborder, leftborder;
#define ID_SWINFO 30
struct NewGadget swinfogad =
{
    10, 5, 561, 220,
    "Info", NULL,
    ID_SWINFO, PLACETEXT_ABOVE, NULL, NULL
};

  InfoWindow = OpenWindowTags ( NULL
	, WA_Title,		(Tag)INFOTITLE_TXT
	, WA_Left,		0
	, WA_Top,		0
	, WA_InnerWidth,	580
	, WA_InnerHeight,	250
	, WA_IDCMP,		IDCMP_CLOSEWINDOW | LISTVIEWIDCMP
	, WA_Flags,		WFLG_DRAGBAR
				| WFLG_DEPTHGADGET
				| WFLG_CLOSEGADGET
				| WFLG_ACTIVATE
	, TAG_END
  );
  iw_sigbit = 1 << InfoWindow->UserPort->mp_SigBit;
  iw_rp = InfoWindow->RPort;

  topborder = InfoWindow->WScreen->WBorTop + InfoWindow->WScreen->Font->ta_YSize + 1;
  leftborder = InfoWindow->WScreen->WBorLeft;
  AskFont ( InfoWindow->RPort, &fixedfontattr );
  swinfogad.ng_TextAttr = &fixedfontattr;
  swinfogad.ng_VisualInfo = vi;
  swinfogad.ng_LeftEdge += leftborder;
  swinfogad.ng_TopEdge += topborder + InfoWindow->WScreen->Font->ta_YSize + 1;
  swinfogad.ng_Height -= InfoWindow->WScreen->Font->ta_YSize + 1;
  
  gad = CreateContext ( &igads );
  gad = CreateGadget ( LISTVIEW_KIND, igads, &swinfogad,
		GTLV_Labels,		(IPTR)&lv_infolist,
		GTLV_ShowSelected,	(IPTR)NULL,
		GTLV_ReadOnly,		FALSE,
		TAG_DONE );

  AddGList ( InfoWindow, igads, 0, -1, NULL );
  RefreshGList ( igads, InfoWindow, NULL, -1 );

return;
}

VOID close_infowindow()
{
  if ( InfoWindow != NULL)
  {
    freelvnodes ( &lv_infolist );
    FreeGadgets ( igads );
    CloseWindow ( InfoWindow );
    InfoWindow = NULL;
    iw_sigbit = 0L;
  }

return;
}

#define APPENDLIST()							\
    string = StrDup ( tmp );						\
    node = (struct Node *) AllocMem ( sizeof(struct Node), MEMF_CLEAR );\
    AddTail ( &lv_infolist, node );					\
    SetNodeName ( node, string );

VOID WindowInfo( struct Window *win )
{
char tmp[1024];
struct Node *node;
char *string;

  if ( InfoWindow != NULL )
  {
    /* Detach List from Gadget */
    GT_SetGadgetAttrs ( igads, InfoWindow, NULL,
	GTLV_Labels, (IPTR)~0,
	TAG_DONE);
  }

  freelvnodes ( &lv_infolist );

  sprintf ( tmp, "NextWindow    = %p", win->NextWindow );	APPENDLIST();
  sprintf ( tmp, "LeftEdge      = %d", win->LeftEdge );		APPENDLIST();
  sprintf ( tmp, "TopEdge       = %d", win->TopEdge );		APPENDLIST();
  sprintf ( tmp, "Width         = %d", win->Width );		APPENDLIST();
  sprintf ( tmp, "Height        = %d", win->Height );		APPENDLIST();
  sprintf ( tmp, "MinWidth      = %d", win->MinWidth );		APPENDLIST();
  sprintf ( tmp, "MinHeight     = %d", win->MinHeight );	APPENDLIST();
  sprintf ( tmp, "MaxWidth      = %d", win->MaxWidth );		APPENDLIST();
  sprintf ( tmp, "MaxHeight     = %d", win->MaxHeight );	APPENDLIST();
  sprintf ( tmp, "Flags         = 0x%08lx", win->Flags );	APPENDLIST();
  sprintf ( tmp, "IDCMPFlags    = 0x%08lx", win->IDCMPFlags );	APPENDLIST();
  sprintf ( tmp, "Title         = \"%s\"", win->Title );	APPENDLIST();
  sprintf ( tmp, "ReqCount      = %d", win->ReqCount );		APPENDLIST();
  sprintf ( tmp, "WScreen       = %p \"%s\"", win->WScreen,
				win->WScreen->Title );		APPENDLIST();
  sprintf ( tmp, "BorderLeft    = %d", win->BorderLeft );	APPENDLIST();
  sprintf ( tmp, "BorderTop     = %d", win->BorderTop );	APPENDLIST();
  sprintf ( tmp, "BorderRight   = %d", win->BorderRight );	APPENDLIST();
  sprintf ( tmp, "BorderBottom  = %d", win->BorderBottom );	APPENDLIST();
  if ( IS_CHILD ( win ) )
  {
    sprintf ( tmp, "Parent Win  = %p", win->parent );		APPENDLIST();
  }
  if ( HAS_CHILDREN ( win ) )
  {
    sprintf ( tmp, "First Child = %p", win->firstchild );	APPENDLIST();
  }
  sprintf ( tmp, "Parent        = %p", win->Parent );		APPENDLIST();
  sprintf ( tmp, "Descendant    = %p", win->Descendant );	APPENDLIST();

  if ( InfoWindow == NULL )
  {
    open_infowindow();
  }
  else
  {
    /* Attach List to Gadget */
    GT_SetGadgetAttrs ( igads, InfoWindow, NULL,
	GTLV_Labels,	(IPTR)&lv_infolist,
	GTLV_Selected,	(IPTR)~0,
	TAG_DONE );

    GT_RefreshWindow ( InfoWindow, NULL );
  }

return;
}

VOID ScreenInfo( struct Screen *scr )
{
char tmp[1024];
struct Node *node;
char *string;

  if ( InfoWindow != NULL )
  {
    /* Detach List from Gadget */
    GT_SetGadgetAttrs ( igads, InfoWindow, NULL,
	GTLV_Labels, (IPTR)~0,
	TAG_DONE );
  }

  freelvnodes ( &lv_infolist );

  sprintf ( tmp, "Screen: %p \"%s\"", scr,scr->Title );		APPENDLIST();
  sprintf ( tmp, "LeftEdge     = %d", scr->LeftEdge );		APPENDLIST();
  sprintf ( tmp, "TopEdge      = %d", scr->TopEdge );		APPENDLIST();
  sprintf ( tmp, "Width        = %d", scr->Width );		APPENDLIST();
  sprintf ( tmp, "Height       = %d", scr->Height );		APPENDLIST();
  sprintf ( tmp, "Flags        = 0x%08x", scr->Flags );		APPENDLIST();
  sprintf ( tmp, "Title        = \"%s\"", scr->Title );		APPENDLIST();
  sprintf ( tmp, "DefaultTitle = \"%s\"", scr->DefaultTitle );	APPENDLIST();
  sprintf ( tmp, "FirstWindow  = %p", scr->FirstWindow );	APPENDLIST();

  if ( InfoWindow == NULL )
  {
    open_infowindow();
  }
  else
  {
    /* Attach List to Gadget */
    GT_SetGadgetAttrs ( igads, InfoWindow, NULL,
	GTLV_Labels,	(IPTR)&lv_infolist,
	GTLV_Selected,	(IPTR)~0,
	TAG_DONE );

    GT_RefreshWindow ( InfoWindow, NULL );
  }

return;
}

VOID rescue_all()
{
struct Screen *scr;
struct Window *win;
WORD width, height;

  /* Get Intuition's first Screen */
  lock = LockIBase ( 0 );
  scr = IntuitionBase->FirstScreen;
  UnlockIBase ( lock );

  /* Traverse through all Screens */
  while ( scr )
  {
    win = scr->FirstWindow;
    /* Traverse through all Windows of current Screen */
    while ( win )
    {
      /* Move Window onto the Screen if outside */
      if ( win->parent == NULL )
      {
	width = scr->Width;
	height = scr->Height;
      }
      else
      {
	width = win->parent->Width;
	height = win->parent->Height;
      }
      /* TODO:	calculate reasonable values:
		eg. this way only the Close Gadget my be visible :-( */
      if ( win->RelLeftEdge < 0
	|| win->RelTopEdge  <= -(win->BorderTop)
	|| win->RelLeftEdge > width
	|| win->RelTopEdge  >= (height - win->BorderTop) )
      {
	MoveWindow ( win, - win->RelLeftEdge, - win->RelTopEdge );
      }
      win = win->NextWindow;
    }
    scr = scr->NextScreen;
  }

return;
}

VOID show_all()
{
struct Screen *scr;
struct Window *win;

  /* Get Intuition's first Screen */
  lock = LockIBase ( 0 );
  scr = IntuitionBase->FirstScreen;
  UnlockIBase ( lock );

  /* Traverse through all Screens */
  while ( scr )
  {
    win = scr->FirstWindow;
    /* Traverse through all Windows of current Screen */
    while ( win )
    {
      /* Show Window if hidden */
      if ( IsWindowVisible ( win ) != TRUE )
      {
	ShowWindow ( win );
      }
      win = win->NextWindow;
    }
    scr = scr->NextScreen;
  }

return;
}

int main()
{
struct Gadget *gad;
struct IntuiMessage *msg = NULL;
struct MenuItem *item;
struct EasyStruct es;
ULONG class;
UWORD code;
ULONG port;
IPTR object;
int type;
int quit = 0;
ULONG sec1, sec2, msec1, msec2, sel1, sel2;

  open_window();

  NewList ( &lv_list );
  NewList ( &lv_infolist );

  CurrentTime ( &sec1, &msec1 );
  sel1 = -1;

  gad = gt_init();
  gad = makegadgets ( gad );
  AddGList ( Window, glist, 0, -1, NULL );
  RefreshGList ( glist, Window, NULL, -1 );

  makemenus();
  
  update_actionglist();

  es.es_StructSize = sizeof ( es );
  es.es_Flags	= 0;
  es.es_Title	= TITLE_TXT;

  while ( quit == 0 )
  {
    port = Wait ( w_sigbit | iw_sigbit | SIGBREAKF_CTRL_C );

    if ( (port & SIGBREAKF_CTRL_C) )
     quit = 1;

    if ( ( port & iw_sigbit ) != 0L )
    {
      BOOL close_iw = FALSE;
      
      while((msg = GT_GetIMsg ( InfoWindow->UserPort )))
      {
	class = msg->Class;
	code = msg->Code;
	if ( class == IDCMP_CLOSEWINDOW )
	{
	  close_iw = TRUE;
	}
	GT_ReplyIMsg((struct Message *)msg);
      }
      
      if (close_iw) close_infowindow();
    }
    
    if ( ( port & w_sigbit ) != 0L )
    {
      while((msg = GT_GetIMsg ( Window->UserPort )))
      {
	class = msg->Class;
	code = msg->Code;
	switch ( class )
	{
	  case IDCMP_CLOSEWINDOW :
		  quit = 1;
		  break;

	  case IDCMP_MENUPICK :
		  while ( code != MENUNULL )
		  {
  //		  printf ( "Menu: %d %d %d\n", MENUNUM(code), ITEMNUM(code), SUBNUM(code) );
		    switch ( MENUNUM ( code ) )
		    {
		      case 0: /* Project */
			  switch ( ITEMNUM ( code ) )
			  {
			    case 0: /* About */
				  es.es_TextFormat = ABOUT_TXT;
				  es.es_GadgetFormat = CONTINUE_TXT;
				  ClearIDCMP();
				  EasyRequest ( Window, &es, NULL, (IPTR) NULL, (IPTR) NULL );
				  ResetIDCMP();
				  break;
			    case 2: /* Quit */
				  quit = 1;
				  break;
			  }
			  break;
		      case 1: /* Window List */
			  object = getsw ( &type );
			  switch ( ITEMNUM ( code ) )
			  {
			    case 0: /* Update List */
				  /* Update will be done after this switch() */
				  break;
			    case 2: /* Kill */
				  if ( type == Screen_type || type == Window_type )
				  {
				  int killit;
				    switch ( type )
				    {
				      case Screen_type :
					  es.es_TextFormat = CLOSESCREEN_TXT;
					  es.es_GadgetFormat = YESNO_TXT;
					  ClearIDCMP();
					  killit = EasyRequest ( Window, &es, NULL, (IPTR) NULL, (IPTR) NULL );
					  ResetIDCMP();
					  if ( killit == EASYTRUE )
					  {
					    CloseScreen ( (struct Screen *)object );
					  }
					  break;
				      case Window_type :
					  es.es_TextFormat = CLOSEWINDOW_TXT;
					  es.es_GadgetFormat = YESNO_TXT;
					  ClearIDCMP();
					  killit = EasyRequest ( Window, &es, NULL, (IPTR) NULL, (IPTR) NULL );
					  ResetIDCMP();
					  if ( killit == EASYTRUE )
					  {
					    CloseWindow ( (struct Window *)object );
					  }
					  break;
				      default:
					  break;
				    }
				  }
				  break;
			    case 3: /* To Front */
				  if ( type == Screen_type || type == Window_type )
				  {
				    switch ( type )
				    {
				      case Screen_type :
					  ScreenToFront ( (struct Screen *)object );
					  break;
				      case Window_type :
					  WindowToFront ( (struct Window *) object );
					  break;
				      default:
					  break;
				    }
				  }
				  break;
			    case 4: /* To Back */
				  if ( type == Screen_type || type == Window_type )
				  {
				    switch ( type )
				    {
				      case Screen_type :
					  ScreenToBack ( (struct Screen *)object );
					  break;
				      case Window_type :
					  WindowToBack ( (struct Window *) object );
					  break;
				      default:
					  break;
				    }
				  }
				  break;
			    case 5: /* To Origin */
				  if ( type == Screen_type || type == Window_type )
				  {
				    switch ( type )
				    {
				      case Screen_type :
					  MoveScreen ( (struct Screen *)object,
						  -((struct Screen *)object)->LeftEdge,
						  -((struct Screen *)object)->TopEdge );
					  break;
				      case Window_type :
					  MoveWindow ( (struct Window *)object,
						  -((struct Window *)object)->RelLeftEdge,
						  -((struct Window *)object)->RelTopEdge );
					  break;
				      default:
					  break;
				    }
				    Delay ( 5 );
				  }
				  break;
			    case 6: /* Activate */
				  if ( type == Window_type )
				  {
				    ActivateWindow ( (struct Window *)object );
				  }
				  break;
			    case 7: /* Zip */
				  if ( type == Window_type )
				  {
				    ZipWindow ( (struct Window *)object );
				  }
				  break;
			    case 8: /* Hide */
				  if ( type == Window_type )
				  {
				    if ( (struct Window *)object == Window )
				    {
				      /* TODO: Iconify WiMP */
				    }
				    else
				    {
				      HideWindow ( (struct Window *)object );
				    }
				  }
				  Delay ( 5 );
				  break;
			    case 9: /* Show */
				  if ( type == Window_type )
				  {
				    ShowWindow ( (struct Window *)object );
				  }
				  Delay ( 5 );
				  break;
			    case 11: /* Info */
				  if ( type == Window_type )
				  {
				    WindowInfo ( (struct Window *)object );
				  }
				  else if ( type == Screen_type )
				  {
				    ScreenInfo ( (struct Screen *)object );
				  }
				  break;
			  }
			  update_list();
			  break;
		      case 2: /* Generic */
			  switch ( ITEMNUM ( code ) )
			  {
			    case 0: /* Rescue All */
				  rescue_all();
				  Delay ( 5 );
				  update_list();
				  break;

			    case 1: /* Show All */
				  show_all();
				  Delay ( 5 );
				  update_list();
				  break;

			    case 2: /* RethinkDisplay */
				  RethinkDisplay();
				  update_list();
				  break;

			    default:
				  break;
			  }
			  break;
		      default:
			  break;
		    }
		    if ( (item = ItemAddress ( menus, code ) ) != NULL )
		    {
		      code = item->NextSelect;
		    }
		    else
		    {
		      code = MENUNULL;
		    }
		  }

		  break;

	  case IDCMP_GADGETUP :
		  switch ( ((struct Gadget *)(msg->IAddress))->GadgetID )
		  {
		    case ID_UPDATE:
			  update_list();
			  break;

		    case ID_ABOUT:
			  es.es_TextFormat = ABOUT_TXT;
			  es.es_GadgetFormat = CONTINUE_TXT;
			  ClearIDCMP();
			  EasyRequest ( Window, &es, NULL, (IPTR) NULL, (IPTR) NULL );
			  ResetIDCMP();
			  break;

		    case ID_RETHINK:
			  RethinkDisplay();
			  update_list();
			  break;

		    case ID_SHOW:
			  object = getsw ( &type );
			  if ( type == Window_type )
			  {
			    ShowWindow ( (struct Window *)object );
			  }
			  Delay ( 5 );
			  update_list();
			  break;

		    case ID_HIDE:
			  object = getsw ( &type );
			  if ( type == Window_type )
			  {
			    if ( (struct Window *)object == Window )
			    {
			      /* TODO: Iconify WiMP */
			    }
			    else
			    {
			      HideWindow ( (struct Window *)object );
			    }
			  }
			  Delay ( 5 );
			  update_list();
			  break;

		    case ID_ZIP:
			  object = getsw ( &type );
			  if ( type == Window_type )
			  {
			    ZipWindow ( (struct Window *)object );
			  }
			  update_list();
			  break;

		    case ID_ACTIVATE:
			  object = getsw ( &type );
			  if ( type == Window_type )
			  {
			    ActivateWindow ( (struct Window *)object );
			  }
			  update_list();
			  break;

		    case ID_FRONT:
			  object = getsw ( &type );
			  if ( type == Screen_type || type == Window_type )
			  {
			    switch ( type )
			    {
			      case Screen_type :
				  ScreenToFront ( (struct Screen *)object );
				  break;
			      case Window_type :
				  WindowToFront ( (struct Window *)object );
				  break;
			      default:
				  break;
			    }
			  }
			  update_list();
			  break;

		    case ID_BACK:
			  object = getsw ( &type );
			  if ( type == Screen_type || type == Window_type )
			  {
			    switch ( type )
			    {
			      case Screen_type :
				  ScreenToBack ( (struct Screen *)object );
				  break;
			      case Window_type :
				  WindowToBack ( (struct Window *)object );
				  break;
			      default:
				  break;
			    }
			  }
			  update_list();
			  break;

		    case ID_ORIGIN:
			  object = getsw ( &type );
			  if ( type == Screen_type || type == Window_type )
			  {
			    switch ( type )
			    {
			      case Screen_type :
				  MoveScreen ( (struct Screen *)object,
					  -((struct Screen *)object)->LeftEdge,
					  -((struct Screen *)object)->TopEdge );
				  break;
			      case Window_type :
				  MoveWindow ( (struct Window *)object,
					  -((struct Window *)object)->RelLeftEdge,
					  -((struct Window *)object)->RelTopEdge );
				  break;
			      default:
				  break;
			    }
			    Delay ( 5 );
			  }
			  update_list();
			  break;

		    case ID_KILL:
			  object = getsw ( &type );
			  if ( type == Screen_type || type == Window_type )
			  {
			  int killit;
			    switch ( type )
			    {
			      case Screen_type :
				  es.es_TextFormat = CLOSESCREEN_TXT;
				  es.es_GadgetFormat = YESNO_TXT;
				  ClearIDCMP();
				  killit = EasyRequest ( Window, &es, NULL, (IPTR) NULL, (IPTR) NULL );
				  ResetIDCMP();
				  if ( killit == EASYTRUE )
				  {
				    CloseScreen ( (struct Screen *)object );
				  }
				  break;
			      case Window_type :
				  es.es_TextFormat = CLOSEWINDOW_TXT;
				  es.es_GadgetFormat = YESNO_TXT;
				  ClearIDCMP();
				  killit = EasyRequest ( Window, &es, NULL, (IPTR) NULL, (IPTR) NULL );
				  ResetIDCMP();
				  if ( killit == EASYTRUE )
				  {
				    CloseWindow ( (struct Window *)object );
				  }
				  break;
			      default:
				  break;
			    }
			  }
			  update_list();
			  break;

		    case ID_RESCUE:
			  rescue_all();
			  Delay(5);
			  update_list();
			  break;

		    case ID_SHOWALL:
			  show_all();
			  Delay(5);
			  update_list();
			  break;

		    case ID_LISTVIEW:
			  CurrentTime( &sec2, &msec2 );
			  GT_GetGadgetAttrs ( screenlistg,Window,NULL,
      					  GTLV_Selected, (IPTR)&sel2,
      					  TAG_DONE );
			  if ( sel1 == sel2
			       && DoubleClick ( sec1, msec1, sec2, msec2 )
      			     )
      			  {
			    object = getsw ( &type );
			    if ( type == Window_type )
			    {
			      WindowInfo ( (struct Window *)object );
			    }
			    else if ( type == Screen_type )
			    {
			      ScreenInfo ( (struct Screen *)object );
			    }
      			  }
      			  sec1 = sec2;
      			  msec1 = msec2;
      			  sel1 = sel2;
			  update_actionglist();
			  break;

		    default:
			  break;
		  }
		  break;

	  default :
		  break;
	}
      	GT_ReplyIMsg( (struct Message *)msg );
	
      } /* while((msg = GT_GetIMsg ( Window->UserPort ))) */
      
    } /* if ( ( port & w_sigbit ) != 0L ) */
    
  } /* while ( quit == 0 ) */

  close_infowindow();
  close_window();
  return ( 0 );
}
