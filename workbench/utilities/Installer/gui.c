/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/* gui.c -- here are all functions for the gui */


#include "Installer.h"
#include "cleanup.h"
#include "execute.h"
#include "texts.h"
#include "more.h"
#include "misc.h"
#include "gui.h"
#include "variables.h"

/* External variables */
extern BPTR inputfile;
extern char buffer[MAXARGSIZE];
extern char *filename;
extern InstallerPrefs preferences;
extern int error, grace_exit;
extern int doing_abort;

void setgadgetdisable( int );
void abort_install( VOID_FUNC );
void setaboutgaddisable( int );

#include <proto/intuition.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <proto/graphics.h>
#include <graphics/gfxbase.h>
#include <proto/gadtools.h>
#include <libraries/gadtools.h>
#include <proto/asl.h>
#include <libraries/asl.h>


struct Window *GuiWin;
struct RastPort *rp;
struct IntuiMessage *imsg;
ULONG class;
UWORD code;

const char GuiWinTitle[] ="AROS - Installer V43.3";

#define _MX_HEIGHT 15

APTR vi;
struct Screen *scr;
struct Gadget
	/* Lists */
	      *glist = NULL,
	      *stringglist = NULL,
	      *integerglist = NULL,
	      *mxglist = NULL,
	      *stdglist = NULL,
	/* Gadgets */
	      *proceedgad = NULL,
	      *abortgad = NULL,
	      *skipgad = NULL,
	      *helpgad = NULL,
	      *aboutgad = NULL,
	/* Special */
	      *stringgad = NULL,
	      *integergad = NULL,
	      *gad = NULL;


#define ID_BOOLGADFALSE 0
struct NewGadget gt_boolgadfalse = {
  15,92, 30,30, /* ng_LeftEdge, ng_TopEdge, ng_Width, ng_Height */
  NULL, 	/* ng_GadgetText */
  NULL, 	/* ng_TextAttr */
  ID_BOOLGADFALSE, /* ng_GadgetID */
  PLACETEXT_IN, /* ng_Flags */
  NULL, 	/* ng_VisualInfo */
  NULL		/* ng_UserData */
};

#define ID_BOOLGADTRUE 1
struct NewGadget gt_boolgadtrue = {
  15,132, 30,30,
  NULL, NULL,
  ID_BOOLGADTRUE, PLACETEXT_IN, NULL, NULL
};

struct NewGadget gt_textgad = {
  15,90, 100,70,
  NULL, NULL,
  0, 0, NULL, NULL
};

#define ID_STRINGGAD 2
struct NewGadget gt_stringgad = {
  15,132, 100,30,
  "Input", NULL,
  ID_STRINGGAD, PLACETEXT_ABOVE, NULL, NULL
};

#define ID_MXGAD 3
struct NewGadget gt_mxgad = {
  15,90, MX_WIDTH, _MX_HEIGHT,
  NULL, NULL,
  ID_MXGAD, PLACETEXT_RIGHT, NULL, NULL
};
STRPTR *mxlabels;


#define PROCEEDGAD	1
#define ABORTGAD	2
#define SKIPGAD		4
#define HELPGAD		8
#define ABOUTGAD		8

#define ID_INTEGERGAD 4
struct NewGadget gt_integergad = {
  15,132, 100,30,
  "Input", NULL,
  ID_INTEGERGAD, PLACETEXT_ABOVE, NULL, NULL
};

#define ID_PROCEEDGAD 100
struct NewGadget gt_proceedgad = {
  10,200, 74,25,
  "Proceed", NULL,
  ID_PROCEEDGAD, PLACETEXT_IN, NULL, NULL
};

#define ID_ABORTGAD 101
struct NewGadget gt_abortgad = {
  85,200, 74,25,
  "Abort", NULL,
  ID_ABORTGAD, PLACETEXT_IN, NULL, NULL
};

#define ID_SKIPGAD 102
struct NewGadget gt_skipgad = {
  160,200, 74,25,
  "Skip", NULL,
  ID_SKIPGAD, PLACETEXT_IN, NULL, NULL
};

#define ID_HELPGAD 103
struct NewGadget gt_helpgad = {
  235,200, 74,25,
  "Help", NULL,
  ID_HELPGAD, PLACETEXT_IN, NULL, NULL
};

#define ID_ABOUTGAD 104
struct NewGadget gt_aboutgad = {
  310,200, 74,25,
  "About", NULL,
  ID_ABOUTGAD, PLACETEXT_IN, NULL, NULL
};

struct TagItem bevel_tag[] = {
  { GT_VisualInfo, 0 },
  { GTBB_Recessed, TRUE },
  { GTBB_FrameType, BBFT_RIDGE },
  { TAG_DONE }
};


#define WINDOWWIDTH  400
#define WINDOWHEIGHT 250

/*
 * Initialize the GUI
 */
void init_gui( )
{
struct TagItem windowtags[] =
{
  { WA_Width	, WINDOWWIDTH },
  { WA_Height	, WINDOWHEIGHT },
  { WA_Left	, 0 },
  { WA_Top	, 0 },
  { WA_Title	, (ULONG)GuiWinTitle },
  { WA_IDCMP	, IDCMP_MOUSEMOVE
		  | IDCMP_GADGETUP
		  | IDCMP_GADGETDOWN
		  | MXIDCMP
		  | BUTTONIDCMP
		  | CHECKBOXIDCMP },
  { WA_Flags	,   WFLG_ACTIVATE
		  | WFLG_DEPTHGADGET
		  | WFLG_DRAGBAR
		  | WFLG_GIMMEZEROZERO },

  { TAG_DONE }
};


  scr = LockPubScreen( NULL );
  windowtags[2].ti_Data = ( scr->Width - WINDOWWIDTH ) / 2 ;
  windowtags[3].ti_Data = ( scr->Height - WINDOWHEIGHT ) / 2 ;
  if( NULL == ( GuiWin = OpenWindowTagList( NULL, windowtags ) ) )
  {
    cleanup();
    exit(-1);
  }
  rp = GuiWin->RPort;

  vi = GetVisualInfoA( scr, NULL );
  bevel_tag[0].ti_Data = (ULONG)vi;

  gt_boolgadtrue.ng_VisualInfo = vi;
  gt_boolgadfalse.ng_VisualInfo = vi;
  gt_textgad.ng_VisualInfo = vi;
  gt_stringgad.ng_VisualInfo = vi;
  gt_mxgad.ng_VisualInfo = vi;
  gt_integergad.ng_VisualInfo = vi;
  gt_proceedgad.ng_VisualInfo = vi;
  gt_abortgad.ng_VisualInfo = vi;
  gt_skipgad.ng_VisualInfo = vi;
  gt_helpgad.ng_VisualInfo = vi;
  gt_aboutgad.ng_VisualInfo = vi;

  gad = CreateContext( &glist );
  if(gad==NULL)
    fprintf( stderr, "CreateContext() failed\n");

  gad = CreateGadget( BUTTON_KIND, gad, &gt_boolgadtrue, TAG_DONE );
  gad = CreateGadget( BUTTON_KIND, gad, &gt_boolgadfalse, TAG_DONE );

  gad = CreateContext( &stringglist );
  if(gad==NULL)
    fprintf( stderr, "CreateContext() failed\n");

  stringgad = gad = CreateGadget( STRING_KIND, gad, &gt_stringgad,
			GTST_String, (IPTR)"Blah",
			GTST_MaxChars, 128,
			GTTX_Border, TRUE,
			TAG_DONE );

  gad = CreateContext( &integerglist );
  if(gad==NULL)
    fprintf( stderr, "CreateContext() failed\n");

  integergad = gad = CreateGadget( INTEGER_KIND, gad, &gt_integergad,
			GTIN_MaxChars, 16,
			GTTX_Border, TRUE,
			TAG_DONE );

  gad = CreateContext( &stdglist );
  if(gad==NULL)
    fprintf( stderr, "CreateContext() failed\n");

  proceedgad = gad = CreateGadget( BUTTON_KIND, gad, &gt_proceedgad, TAG_DONE );
  abortgad   = gad = CreateGadget( BUTTON_KIND, gad, &gt_abortgad, TAG_DONE );
  skipgad    = gad = CreateGadget( BUTTON_KIND, gad, &gt_skipgad, TAG_DONE );
  helpgad    = gad = CreateGadget( BUTTON_KIND, gad, &gt_helpgad, TAG_DONE );

  AddGList( GuiWin, stdglist, -1, -1, NULL );
  RefreshGList( stdglist, GuiWin, NULL, -1 );
}


/*
 * Close GUI
 */
void deinit_gui( )
{
  RemoveGList( GuiWin, stdglist, -1 );
  FreeGadgets( glist );
  FreeGadgets( stringglist );
  FreeGadgets( integerglist );
  FreeVisualInfo( vi );
  UnlockPubScreen( NULL, scr );
  CloseWindow( GuiWin );
}


/*
 * Clean the GUI display
 */
void clear_gui()
{
//  GT_BeginRefresh( GuiWin );
  EraseRect( rp, 0, 0, GuiWin->Width, GuiWin->Height-25 );
//  GT_EndRefresh( GuiWin, TRUE );
}


/*
 * Show user that we are going to "(abort)" install
 * Don't confuse NOVICE...
 */
void show_abort( char *msg )
{
char **out, *text;
int n, m;
int finish = FALSE;

  if( get_var_int( "@user-level" ) > _NOVICE )
  {

    clear_gui();

    setgadgetdisable( ABORTGAD|SKIPGAD|HELPGAD );

    RefreshGList(stdglist,GuiWin,NULL,-1);
    GT_RefreshWindow(GuiWin,NULL);

    text = StrDup( "Aborting Installation:" );
    outofmem( text );
    Move( rp, 15, 25 );
    Text( rp, text, strlen(text) );
    FreeVec( text );
    out = AllocVec( sizeof( char * ), MEMF_PUBLIC );
    outofmem( out );
    out[0] = NULL;
    m = strtostrs( msg, &out );
    for( n = 0 ; n < m ; n++ )
    {
      Move( rp, 15, 15*n+45 );
      Text( rp, out[n], strlen(out[n]) );
      FreeVec( out[n] );
    }
    FreeVec( out );
#ifdef DEBUG
      printf( "%s\n", msg );
#endif /* DEBUG */
    while (!finish)
    {
      WaitPort( GuiWin->UserPort );
      while ((imsg = GT_GetIMsg( GuiWin->UserPort )))
      {
	class = imsg->Class;
	code = imsg->Code;
	switch( class )
	{
	  case IDCMP_GADGETUP:
	      switch( ( (struct Gadget *)(imsg->IAddress) )->GadgetID )
	      {
		case ID_PROCEEDGAD:
		    finish = TRUE;
		    break;
		default:
		    break;
	      }
	      break;
	  default:
	      break;
	}
	GT_ReplyIMsg( imsg );

      } /* while ((imsg = GT_GetIMsg( GuiWin->UserPort )) */
    }

  }
}


/*
 * Show user how much we have completed yet
 */
void show_complete( long int percent )
{
static char *text = NULL;

  if(text == NULL)
  {
    text = AllocVec( strlen( GuiWinTitle ) + 13, MEMF_PUBLIC );
  }
  if( text == NULL )
  {
    end_alloc();
  }
  sprintf( text, "%s (Done %3ld%%)", GuiWinTitle, percent );
  SetWindowTitles( GuiWin, text, NULL);
}


/*
 * Show user that we "(exit)" the installation
 */
void show_exit( char *msg )
{
char **out, *text;
int n, m;

  clear_gui();

  setgadgetdisable( ABORTGAD|SKIPGAD );
  RefreshGList(stdglist,GuiWin,NULL,-1);
  GT_RefreshWindow(GuiWin,NULL);

  text = StrDup( "Aborting Installation:" );
  outofmem( text );
  Move( rp, 15, 25 );
  Text( rp, text, strlen(text) );
  FreeVec( text );
  out = AllocVec( sizeof( char * ), MEMF_PUBLIC );
  outofmem( out );
  out[0] = NULL;
  m = strtostrs( msg, &out );
  for( n = 0 ; n < m ; n++ )
  {
    Move( rp, 15, 15*n+45 );
    Text( rp, out[n], strlen(out[n]) );
    FreeVec( out[n] );
  }
  FreeVec( out );
#ifdef DEBUG
  printf( "%s\n", msg );
#endif /* DEBUG */
  text = StrDup( "Done with Installation." );
  outofmem( text );
  Move( rp, 15, 15*n+45 );
  Text( rp, text, strlen(text) );
  FreeVec( text );
#ifdef DEBUG
  printf( "\nDone with installation.\n\n" );
#endif /* DEBUG */
  printf( " Press Return to Proceed\n" );
  scanf("x");
}


/*
 * Show the line which caused the parse-error
 */
void show_parseerror( char * msg, int errline )
{
int count = 1, i = -1;

#ifdef DEBUG
  fprintf( stderr, "Syntax error in line %d:\n", errline );
  if( msg != NULL )
  {
    fprintf( stderr, "%s\n",msg );
  }
#endif /* DEBUG */
  inputfile = Open( filename, MODE_OLDFILE );
  if( inputfile == NULL )
  {
    PrintFault( IoErr(), INSTALLER_NAME );
    cleanup();
    exit(-1);
  }
  errline--;
  while ( count != 0 && errline > 0 )
  {
    count = Read( inputfile, buffer, 1 );
    if( buffer[0] == LINEFEED )
    {
      errline--;
    }
  }
  do
  {
    i++;
    count = Read( inputfile, &buffer[i], 1 );
  } while ( buffer[i] != LINEFEED && count != 0 && i < MAXARGSIZE );
  buffer[i] = 0;
#ifdef DEBUG
  fprintf( stderr, "%s\n", buffer );
#endif /* DEBUG */

}


/*
 * Tell user that some big task is to be done
 * "Be patient..."
 */
void show_working( char *msg )
{
char *text, **out;
int n, m;

  clear_gui();

  setgadgetdisable( 0 );
  RefreshGList(stdglist,GuiWin,NULL,-1);
  GT_RefreshWindow(GuiWin,NULL);

  text = StrDup( "Working on Installation:" );
  outofmem( text );
  Move( rp, 15, 25 );
  Text( rp, text, strlen(text) );
#ifdef DEBUG
  printf( "%s\n", text );
#endif /* DEBUG */
  FreeVec( text );
  out = AllocVec( sizeof( char * ), MEMF_PUBLIC );
  outofmem( out );
  out[0] = NULL;
  m = strtostrs( msg, &out );
  for( n = 0 ; n < m ; n++ )
  {
    Move( rp, 15, 15*n+45 );
    Text( rp, out[n], strlen(out[n]) );
    FreeVec( out[n] );
  }
  FreeVec( out );
#ifdef DEBUG
  printf( "%s\n", msg );
#endif /* DEBUG */
}


/*
 * Display a "(message)" to the user
 * Don't confuse NOVICE unless "(all)" users want to get this info
 */
void show_message( char * msg ,struct ParameterList * pl )
{
char **out;
int n, m;
int finish = FALSE;

  if( GetPL( pl, _ALL ).used == 1 || get_var_int( "@user-level" ) > _NOVICE )
  {

    clear_gui();

    setgadgetdisable( SKIPGAD
		    | ( (GetPL( pl, _HELP ).used == 1) ? 0 : HELPGAD ) );
    RefreshGList(stdglist,GuiWin,NULL,-1);
    GT_RefreshWindow(GuiWin,NULL);

    out = AllocVec( sizeof( char * ), MEMF_PUBLIC );
    outofmem( out );
    out[0] = NULL;
    m = strtostrs( msg, &out );
    for( n = 0 ; n < m ; n++ )
    {
      Move( rp, 15, 15*n+30 );
      Text( rp, out[n], strlen(out[n]) );
      FreeVec( out[n] );
    }
    FreeVec( out );
#ifdef DEBUG
    printf( "%s\n", msg );
#endif /* DEBUG */

    while (!finish)
    {
      WaitPort( GuiWin->UserPort );
      while ((imsg = GT_GetIMsg( GuiWin->UserPort )))
      {
	class = imsg->Class;
	code = imsg->Code;
	switch( class )
	{
	  case IDCMP_GADGETUP:
		switch( ( (struct Gadget *)(imsg->IAddress) )->GadgetID )
		{
		  case ID_PROCEEDGAD:
		    finish = TRUE;
		    break;
		  case ID_ABORTGAD:
		    abort_install( NULL );
		    break;
		  case ID_HELPGAD:
		    m = GetPL( pl, _HELP ).intval;
#ifdef DEBUG
		    for( n = 0 ; n < m ; n++ )
		    {
		      printf( "%s\n", GetPL( pl, _HELP ).arg[n] );
		    }
#endif /* DEBUG */
		    setgadgetdisable( PROCEEDGAD|ABORTGAD|SKIPGAD|HELPGAD );
		    morenmain( HELP_ON_MESSAGE, m, GetPL( pl, _HELP ).arg );
		    setgadgetdisable( SKIPGAD
				    | ( (GetPL( pl, _HELP ).used == 1) ? 0 : HELPGAD ) );
		    break;
		  default:
		    break;
		}
		break;
	  default:
		break;
	}
	GT_ReplyIMsg(imsg);
	
      } /* while ((imsg = GT_GetIMsg( GuiWin->UserPort )) */
    } /* !finish */

  }
}


/*
 * Show the help-window for topic: User-Level
 */
void show_help_userlevel( )
{
#warning TODO: help for userlevel-requester

  moremain( HELP_ON_USERLEVEL, USERLEVEL_HELP );
}


/*
 * Show the help-window for topic: Log-File
 */
void show_help_logfile()
{
char *helptext;

#warning TODO: help for logfile-requester
  helptext = AllocVec( 512 * sizeof(char), MEMF_PUBLIC );
  sprintf( helptext, LOG_HELP, preferences.transcriptfile );
  moremain( HELP_ON_LOGFILES, helptext );
  FreeVec(helptext);
}


/*
 * Show the help-window for topic: Pretend to install
 */
void show_help_pretend()
{
#warning TODO: help for pretend-requester
  moremain( HELP_ON_PRETEND, PRETEND_HELP );
}


/*
 * Show the help-window for topic: Installer
 */
void show_help_installer( )
{
char *helptext;

#warning TODO: help/about for Installer
  helptext = AllocVec( 512 * sizeof(char), MEMF_PUBLIC );
  sprintf( helptext, ABOUT_INSTALLER, INSTALLER_VERSION, INSTALLER_REVISION );
  moremain( ABOUT_ON_INSTALLER, helptext );
  FreeVec(helptext);
}


/*
 * Ask user for his user-level
 */
void request_userlevel( char *msg )
{
int usrlevel, finish = FALSE;
char welcome[1024];

  usrlevel = preferences.defusrlevel;

  if( msg != NULL )
  {
  int n,m;
  char **out;

    out = AllocVec( sizeof( char * ), MEMF_PUBLIC );
    outofmem( out );
    out[0] = NULL;
    m = strtostrs( msg, &out );
    for( n = 0 ; n < m ; n++ )
    {
      Move( rp, 15, 15*n+30 );
      Text( rp, out[n], strlen(out[n]) );
      FreeVec( out[n] );
    }
    FreeVec( out );
#ifdef DEBUG
    printf( "%s\n", msg );
#endif /* DEBUG */
  }
  else
  {
    sprintf( welcome, "Welcome to the %s App installation utility!\n", get_var_arg( "@app-name" ) );
    Move( rp, 15, 25 );
    Text( rp, welcome, strlen(welcome) );
#ifdef DEBUG
    printf( welcome );
#endif /* DEBUG */
  }

  mxlabels = AllocVec( 4 * sizeof(STRPTR), MEMF_PUBLIC );
  mxlabels[0] = StrDup( NOVICE_NAME );
  mxlabels[1] = StrDup( ADVANCED_NAME );
  mxlabels[2] = StrDup( EXPERT_NAME );
  mxlabels[3] = NULL;

  gad = CreateContext( &mxglist );
  if(gad==NULL)
    fprintf( stderr, "CreateContext() failed\n");

  gt_mxgad.ng_GadgetText = StrDup( USERLEVEL_REQUEST );
  gt_mxgad.ng_LeftEdge = 150;

  gad = CreateGadget( MX_KIND, gad, &gt_mxgad,
			GTMX_Labels, (IPTR)mxlabels,
			GTMX_Scaled, TRUE,
			GTMX_Spacing, 2,
			GTMX_TitlePlace, PLACETEXT_ABOVE,
			GTMX_Active, usrlevel,
			GA_Immediate, TRUE,
			TAG_DONE );
  aboutgad = gad = CreateGadget( BUTTON_KIND, gad, &gt_aboutgad, TAG_DONE );

#warning FIXME: Disable MX-Items < preferences.minusrlevel
/* A Hack would be to use:
			GTMX_Labels, &mxlabels[preferences.minusrlevel],
   or add a disabled transparent gadget over the unselectable items
   Does this really work? Can you switch the selected item via keyboard?
*/

  setgadgetdisable( SKIPGAD );
  AddGList( GuiWin, mxglist, -1, -1, NULL );
  RefreshGList( mxglist, GuiWin, NULL ,-1 );
  GT_RefreshWindow( GuiWin, NULL );

  while (!finish)
  {
    WaitPort( GuiWin->UserPort );
    while ((imsg = GT_GetIMsg( GuiWin->UserPort )))
    {
      class = imsg->Class;
      code = imsg->Code;
      switch( class )
      {
	case IDCMP_GADGETDOWN:
	    switch( ( (struct Gadget *)(imsg->IAddress) )->GadgetID )
	    {
	      case ID_MXGAD:
		  usrlevel = code;
		  break;
	      default:
		  break;
	    }
	    break;
	case IDCMP_GADGETUP:
	    switch( ( (struct Gadget *)(imsg->IAddress) )->GadgetID )
	    {
	      case ID_PROCEEDGAD:
		  finish = TRUE;
		  break;
	      case ID_ABORTGAD:
		  abort_install( NULL );
		  break;
	      case ID_HELPGAD:
		  setgadgetdisable( PROCEEDGAD|ABORTGAD|SKIPGAD|HELPGAD );
		  setaboutgaddisable( TRUE );
		  show_help_userlevel();
		  setaboutgaddisable( FALSE );
		  setgadgetdisable( SKIPGAD );
		  break;
	      case ID_ABOUTGAD:
		  setgadgetdisable( PROCEEDGAD|ABORTGAD|SKIPGAD|HELPGAD );
		  setaboutgaddisable( TRUE );
		  show_help_installer();
		  setaboutgaddisable( FALSE );
		  setgadgetdisable( SKIPGAD );
		  break;
	      default:
		  break;
	    }
	    break;
	default:
	    break;
      }
      GT_ReplyIMsg( imsg );

    } /* while ((imsg = GT_GetIMsg( GuiWin->UserPort )) */
  }

  RemoveGList( GuiWin, mxglist, -1 );
  RefreshGList( stdglist, GuiWin, NULL, -1 );

  clear_gui();

  GT_RefreshWindow( GuiWin, NULL );
  FreeGadgets( mxglist );
  freestrlist( mxlabels );
  FreeVec( gt_mxgad.ng_GadgetText );
  gt_mxgad.ng_GadgetText = NULL;

  set_variable( "@user-level", NULL, usrlevel );

  finish = FALSE;
  if( usrlevel > 0 )
  {
    /* Ask for logfile-creation */
    Move( rp, 15, 50 );
    Text( rp, LOG_QUESTION, strlen(LOG_QUESTION) );

    mxlabels = AllocVec( 4 * sizeof(STRPTR), MEMF_PUBLIC );
    mxlabels[0] = StrDup( LOG_FILE_TEXT );
    mxlabels[1] = StrDup( LOG_PRINT_TEXT );
    mxlabels[2] = StrDup( LOG_NOLOG_TEXT );
    mxlabels[3] = NULL;

    gad = CreateContext( &mxglist );
    if(gad==NULL)
      fprintf( stderr, "CreateContext() failed\n");

    gt_mxgad.ng_LeftEdge = 80;
    gad = CreateGadget( MX_KIND, gad, &gt_mxgad,
			GTMX_Labels, (IPTR)mxlabels,
			GTMX_Scaled, TRUE,
			GTMX_TitlePlace, PLACETEXT_ABOVE,
			GA_Immediate, TRUE,
			TAG_DONE );

    AddGList( GuiWin, mxglist ,-1 ,-1 ,NULL );
    RefreshGList( stdglist ,GuiWin ,NULL ,-1 );
    GT_RefreshWindow( GuiWin ,NULL );
    while (!finish)
    {
      WaitPort( GuiWin->UserPort );
      while ((imsg = GT_GetIMsg( GuiWin->UserPort )))
      {
	class = imsg->Class;
	code = imsg->Code;
	switch( class )
	{
	  case IDCMP_GADGETDOWN:
		switch( ( (struct Gadget *)(imsg->IAddress) )->GadgetID )
		{
		  case ID_MXGAD:
		    switch(code)
		    {
#warning TODO: Handle Logging output selection
		      case 0: /* Log to file */
			break;
		      case 1: /* Log to printer */
			FreeVec( preferences.transcriptfile );
			preferences.transcriptfile = StrDup ("PRT:");
			break;
		      case 2: /* No Log */
			FreeVec( preferences.transcriptfile );
			preferences.transcriptfile = NULL;
			break;
		      default:
			break;
		    }
		    break;
		  default:
		    break;
		}
		break;
	  case IDCMP_GADGETUP:
		switch( ( (struct Gadget *)(imsg->IAddress) )->GadgetID )
		{
		  case ID_PROCEEDGAD:
		    finish = TRUE;
		    break;
		  case ID_ABORTGAD:
		    abort_install( NULL );
		    break;
		  case ID_HELPGAD:
		    setgadgetdisable( PROCEEDGAD|ABORTGAD|SKIPGAD|HELPGAD );
		    show_help_logfile();
		    setgadgetdisable( SKIPGAD );
		    break;
		  default:
		    break;
		}
		break;
	  default:
		break;
	}
	GT_ReplyIMsg(imsg);
	
      } /* while ((imsg = GT_GetIMsg( GuiWin->UserPort )) */
    } /* !finish */

    if(!preferences.nopretend)
    {
      RemoveGList( GuiWin, mxglist, -1 );
      RefreshGList( stdglist, GuiWin, NULL, -1 );
      clear_gui();
      GT_RefreshWindow( GuiWin, NULL );
      FreeGadgets( mxglist );
      freestrlist( mxlabels );
      FreeVec( gt_mxgad.ng_GadgetText );
      gt_mxgad.ng_GadgetText = NULL;
      Move( rp, 15, 50 );
      Text( rp, PRETEND_QUESTION, strlen(PRETEND_QUESTION) );

      mxlabels = AllocVec( 3 * sizeof(STRPTR), MEMF_PUBLIC );
      mxlabels[0] = StrDup( NOPRETEND_TEXT );
      mxlabels[1] = StrDup( PRETEND_TEXT );
      mxlabels[2] = NULL;

      gad = CreateContext( &mxglist );
      if(gad==NULL)
	fprintf( stderr, "CreateContext() failed\n");

      gt_mxgad.ng_LeftEdge = 80;
      gad = CreateGadget( MX_KIND, gad, &gt_mxgad,
			  GTMX_Labels, (IPTR)mxlabels,
			  GTMX_Scaled, TRUE,
			  GTMX_TitlePlace, PLACETEXT_ABOVE,
			  GA_Immediate, TRUE,
			  TAG_DONE );

      AddGList( GuiWin, mxglist ,-1 ,-1 ,NULL );
      RefreshGList( stdglist ,GuiWin ,NULL ,-1 );
      GT_RefreshWindow( GuiWin ,NULL );

      finish = FALSE;
      while (!finish)
      {
	WaitPort( GuiWin->UserPort );
	while ((imsg = GT_GetIMsg( GuiWin->UserPort )))
	{
	  class = imsg->Class;
	  code = imsg->Code;
	  switch( class )
	  {
	    case IDCMP_GADGETDOWN:
		  switch( ( (struct Gadget *)(imsg->IAddress) )->GadgetID )
		  {
		    case ID_MXGAD:
		      switch(code)
		      {
			case 0: /* Really Install */
			  preferences.pretend = FALSE;
			  break;
			case 1: /* Only pretend to install */
			  preferences.pretend = TRUE;
			  break;
			default:
			  break;
		      }
		      break;
		    default:
		      break;
		  }
		  break;
	    case IDCMP_GADGETUP:
		  switch( ( (struct Gadget *)(imsg->IAddress) )->GadgetID )
		  {
		    case ID_PROCEEDGAD:
		      finish = TRUE;
		      break;
		    case ID_ABORTGAD:
		      abort_install( NULL );
		      break;
		    case ID_HELPGAD:
		      setgadgetdisable( PROCEEDGAD|ABORTGAD|SKIPGAD|HELPGAD );
		      show_help_pretend();
		      setgadgetdisable( SKIPGAD );
		      break;
		    default:
		      break;
		  }
		  break;
	    default:
		  break;
	  }
	  GT_ReplyIMsg(imsg);
	
	} /* while ((imsg = GT_GetIMsg( GuiWin->UserPort )) */
      } /* !finish */
    } /* nopretend */

    RemoveGList(GuiWin,mxglist,-1);
    FreeGadgets( mxglist );
    freestrlist( mxlabels );

  } /* if usrlevel > 0 */ 

  gt_mxgad.ng_LeftEdge = 15;
  clear_gui();

  setgadgetdisable( 0 );
  RefreshGList(stdglist,GuiWin,NULL,-1);
  GT_RefreshWindow(GuiWin,NULL);
}


/*
 * Ask user for a boolean
 */
void request_bool_destruct()
{
  RemoveGList(GuiWin,glist,-1);
  GT_RefreshWindow(GuiWin,NULL);
}
long int request_bool( struct ParameterList *pl)
{
int i;
long int retval;
char yes[] = "Yes", no[] = "No", *yesstring, *nostring;
int finish = FALSE;

  retval = ( GetPL( pl, _DEFAULT ).intval != 0 );
  yesstring = yes;
  nostring = no;
  if( GetPL( pl, _CHOICES ).used == 1 )
  {
    i = GetPL( pl, _CHOICES ).intval;
    if( i > 0 )
      yesstring = GetPL( pl, _CHOICES ).arg[0];
    if( i > 1 )
      nostring = GetPL( pl, _CHOICES ).arg[1];
  }
  for( i = 0 ; i < GetPL( pl, _PROMPT ).intval ; i ++ )
  {
    if( preferences.transcriptstream != NULL )
    {
    char tmpbuffer[MAXARGSIZE];
      sprintf( tmpbuffer, ">%s\n", GetPL( pl, _PROMPT ).arg[i] );
      Write( preferences.transcriptstream, tmpbuffer, strlen( tmpbuffer ) );
    }
  }
  if( get_var_int( "@user-level" ) > _NOVICE )
  {
    char **out;
    int j, n, m;

    clear_gui();

    setgadgetdisable( PROCEEDGAD|SKIPGAD );
    AddGList(GuiWin,glist,-1,-1,NULL);
    RefreshGList(stdglist,GuiWin,NULL,-1);
    GT_RefreshWindow(GuiWin,NULL);
    DrawBevelBoxA(rp, 5,5,GuiWin->Width-15-GuiWin->BorderLeft,GuiWin->Height-65-GuiWin->BorderTop,bevel_tag);
    DrawBevelBoxA(rp, 15,12,GuiWin->Width-35-GuiWin->BorderLeft,GuiWin->Height-160-GuiWin->BorderTop,bevel_tag);
	
    Move( rp, 50, 107 );
    Text( rp, nostring, strlen(nostring) );
    Move( rp, 50, 147 );
    Text( rp, yesstring, strlen(yesstring) );

    j = 0;
    for( i = 0 ; i < GetPL( pl, _PROMPT ).intval ; i ++ )
    {
#ifdef DEBUG
      printf( "%s\n", GetPL( pl, _PROMPT ).arg[i] );
#endif /* DEBUG */
      out = AllocVec( sizeof( char * ), MEMF_PUBLIC );
      outofmem( out );
      out[0] = NULL;
      m = strtostrs( GetPL( pl, _PROMPT ).arg[i], &out );
      for( n = 0 ; n < m ; n++ )
      {
	Move( rp, 18, 15*j+22 );
	Text( rp, out[n], strlen(out[n]) );
	FreeVec( out[n] );
	j++;
      }
      FreeVec( out );
    }
    do
    {
      WaitPort( GuiWin->UserPort );
      while ((imsg = GT_GetIMsg( GuiWin->UserPort )))
      {
	class = imsg->Class;
	code = imsg->Code;
	switch( class )
	{
	  case IDCMP_GADGETUP:
		switch( ( (struct Gadget *)(imsg->IAddress) )->GadgetID )
		{
		  case ID_BOOLGADFALSE:
		    retval = 0;
		    finish = TRUE;
		    break;
		  case ID_BOOLGADTRUE:
		    retval = 1;
		    finish = TRUE;
		    break;
		  case ID_ABORTGAD:
		    abort_install( &request_bool_destruct );
		    break;
		  case ID_HELPGAD:
#warning FIXME: What is this help like?
		    setgadgetdisable( PROCEEDGAD|ABORTGAD|SKIPGAD|HELPGAD );
		    m = GetPL( pl, _HELP ).intval;
		    if(m)
		    {
#ifdef DEBUG
		      for( n = 0 ; n < m ; n++ )
		      {
			printf( "%s\n", GetPL( pl, _HELP ).arg[n] );
		      }
#endif /* DEBUG */
		      morenmain( HELP_ON_ASKCHOICE, m, GetPL( pl, _HELP ).arg );
		    }
		    else
		    {
#ifdef DEBUG
		      printf( "%s\n", get_var_arg( "@askchoice-help" ) );
#endif /* DEBUG */
		      moremain( HELP_ON_ASKCHOICE, get_var_arg( "@askchoice-help" ) );
		    }
		    setgadgetdisable( SKIPGAD );
		    break;
		  default:
		    break;
		}
		break;
	  default:
		break;
	}
	GT_ReplyIMsg(imsg);

      } /* while ((imsg = GT_GetIMsg( GuiWin->UserPort )) */

    } while (!finish);

    request_bool_destruct();
  }

  if( preferences.transcriptstream != NULL )
  {
  char tmpbuffer[MAXARGSIZE];
    sprintf( tmpbuffer, "Ask Question: Result was \"%s\".\n\n", ( retval ? yesstring : nostring ) );
    Write( preferences.transcriptstream, tmpbuffer, strlen( tmpbuffer ) );
  }

return retval;
}


/*
 * Ask user for a number
 */
void request_number_destruct()
{
  RemoveGList(GuiWin,integerglist,-1);
  GT_RefreshWindow(GuiWin,NULL);
}
long int request_number( struct ParameterList *pl)
{
int i, j, m, n;
long int retval, min, max;
char **out;
int finish = FALSE;

  retval = GetPL( pl, _DEFAULT ).intval;
  if( GetPL( pl, _RANGE ).used == 1 )
  {
    min = GetPL( pl, _RANGE ).intval;
    max = GetPL( pl, _RANGE ).intval2;
    /* Wrong order ? Change order */
    if( max < min )
    {
      i = min;
      min = max;
      max = i;
    }
  }
  else
  {
#define INTMAX	32767
    max = INTMAX;
    min = ( retval < 0 ) ? retval : 0;
  }
  for( i = 0 ; i < GetPL( pl, _PROMPT ).intval ; i ++ )
  {
    if( preferences.transcriptstream != NULL )
    {
    char tmpbuffer[MAXARGSIZE];
      sprintf( tmpbuffer, ">%s\n", GetPL( pl, _PROMPT ).arg[i] );
      Write( preferences.transcriptstream, tmpbuffer, strlen( tmpbuffer ) );
    }
  }

  if( get_var_int( "@user-level" ) > _NOVICE )
  {
    clear_gui();
    setgadgetdisable( SKIPGAD );

    GT_SetGadgetAttrs( integergad, GuiWin, NULL,
			GTIN_Number, retval,
			TAG_DONE );

    AddGList(GuiWin,integerglist,-1,-1,NULL);
    RefreshGList(stdglist,GuiWin,NULL,-1);
    GT_RefreshWindow(GuiWin,NULL);
    DrawBevelBoxA(rp, 5,5,GuiWin->Width-15-GuiWin->BorderLeft,GuiWin->Height-65-GuiWin->BorderTop,bevel_tag);
    DrawBevelBoxA(rp, 15,12,GuiWin->Width-35-GuiWin->BorderLeft,GuiWin->Height-160-GuiWin->BorderTop,bevel_tag);
    j = 0;
    for( i = 0 ; i < GetPL( pl, _PROMPT ).intval ; i ++ )
    {
#ifdef DEBUG
      printf( "%s\n", GetPL( pl, _PROMPT ).arg[i] );
#endif /* DEBUG */
      out = AllocVec( sizeof( char * ), MEMF_PUBLIC );
      outofmem( out );
      out[0] = NULL;
      m = strtostrs( GetPL( pl, _PROMPT ).arg[i], &out );
      for( n = 0 ; n < m ; n++ )
      {
	Move( rp, 18, 15*j+22 );
	Text( rp, out[n], strlen(out[n]) );
	FreeVec( out[n] );
	j++;
      }
      FreeVec( out );
    }

    finish = FALSE;
    while (!finish)
    {
      WaitPort( GuiWin->UserPort );
      while ((imsg = GT_GetIMsg( GuiWin->UserPort )))
      {
	class = imsg->Class;
	code = imsg->Code;
	switch( class )
	{
	  case IDCMP_GADGETUP:
		switch( ( (struct Gadget *)(imsg->IAddress) )->GadgetID )
		{
		  case ID_PROCEEDGAD:
		    GT_GetGadgetAttrs( integergad, GuiWin, NULL,
					GTIN_Number, (IPTR)&m,
					TAG_DONE );
		    if( m < min )
		    {
		      GT_SetGadgetAttrs( integergad, GuiWin, NULL,
					GTIN_Number, min,
					TAG_DONE );
		      ActivateGadget( integergad, GuiWin, NULL );
		    }
		    else if( m > max )
		    {
		      GT_SetGadgetAttrs( integergad, GuiWin, NULL,
					GTIN_Number, max,
					TAG_DONE );
		      ActivateGadget( integergad, GuiWin, NULL );
		    }
		    else
		    {
		      retval = m;
		      finish = TRUE;
		    }
		    break;
		  case ID_ABORTGAD:
		    abort_install( &request_number_destruct );
		    break;
		  case ID_HELPGAD:
#warning FIXME: What is this help like?
		    m = GetPL( pl, _HELP ).intval;
		    for( n = 0 ; n < m ; n++ )
		    {
		      printf( "%s\n", GetPL( pl, _HELP ).arg[n] );
		    }
		    if( n == 0 )
		    {
		      printf( "%s\n", get_var_arg( "@asknumber-help" ) );
		    }
		    break;
		  case ID_INTEGERGAD:
		    GT_GetGadgetAttrs( integergad, GuiWin, NULL,
					GTIN_Number, (IPTR)&m,
					TAG_DONE );
		    if( m < min )
		    {
		      GT_SetGadgetAttrs( integergad, GuiWin, NULL,
					GTIN_Number, min,
					TAG_DONE );
		      ActivateGadget( integergad, GuiWin, NULL );
		    }
		    else if( m > max )
		    {
		      GT_SetGadgetAttrs( integergad, GuiWin, NULL,
					GTIN_Number, max,
					TAG_DONE );
		      ActivateGadget( integergad, GuiWin, NULL );
		    }
		    else
		    {
		      retval = m;
		    }
		    break;
		  default:
		    break;
		}
		break;
	  default:
		break;
	}
	GT_ReplyIMsg(imsg);

      } /* while ((imsg = GT_GetIMsg( GuiWin->UserPort )) */
    } /* !finish */

    request_number_destruct();
  }

  if( preferences.transcriptstream != NULL )
  {
  char tmpbuffer[MAXARGSIZE];
    sprintf( tmpbuffer, "Ask Number: Result was \"%ld\".\n\n", retval );
    Write( preferences.transcriptstream, tmpbuffer, strlen( tmpbuffer ) );
  }

return retval;
}


/*
 * Ask user for a string
 */
void request_string_destruct()
{
  RemoveGList(GuiWin,stringglist,-1);
  GT_RefreshWindow(GuiWin,NULL);
}
char *request_string( struct ParameterList *pl)
{
int i, j, m, n;
char *retval,*string;
char **out;
int finish = FALSE;

  if( GetPL( pl, _DEFAULT ).used == 1 )
  {
    string = GetPL( pl, _DEFAULT ).arg[0];
  }
  else
  {
    string = EMPTY_STRING;
  }
  for( i = 0 ; i < GetPL( pl, _PROMPT ).intval ; i ++ )
  {
    if( preferences.transcriptstream != NULL )
    {
    char tmpbuffer[MAXARGSIZE];
      sprintf( tmpbuffer, ">%s\n", GetPL( pl, _PROMPT ).arg[i] );
      Write( preferences.transcriptstream, tmpbuffer, strlen( tmpbuffer ) );
    }
  }

  if( get_var_int( "@user-level" ) > _NOVICE )
  {
    clear_gui();
    setgadgetdisable( SKIPGAD );

    GT_SetGadgetAttrs( stringgad, GuiWin, NULL,
			GTST_String, (IPTR)string,
			TAG_DONE );

    AddGList(GuiWin,stringglist,-1,-1,NULL);
    RefreshGList(stdglist,GuiWin,NULL,-1);
    GT_RefreshWindow(GuiWin,NULL);
    DrawBevelBoxA(rp, 5,5,GuiWin->Width-15-GuiWin->BorderLeft,GuiWin->Height-65-GuiWin->BorderTop,bevel_tag);
    DrawBevelBoxA(rp, 15,12,GuiWin->Width-35-GuiWin->BorderLeft,GuiWin->Height-160-GuiWin->BorderTop,bevel_tag);
    j = 0;
    for( i = 0 ; i < GetPL( pl, _PROMPT ).intval ; i ++ )
    {
#ifdef DEBUG
      printf( "%s\n", GetPL( pl, _PROMPT ).arg[i] );
#endif /* DEBUG */
      out = AllocVec( sizeof( char * ), MEMF_PUBLIC );
      outofmem( out );
      out[0] = NULL;
      m = strtostrs( GetPL( pl, _PROMPT ).arg[i], &out );
      for( n = 0 ; n < m ; n++ )
      {
	Move( rp, 18, 15*j+22 );
	Text( rp, out[n], strlen(out[n]) );
	FreeVec( out[n] );
	j++;
      }
      FreeVec( out );
    }

    finish = FALSE;
    while (!finish)
    {
      WaitPort( GuiWin->UserPort );
      while ((imsg = GT_GetIMsg( GuiWin->UserPort )))
      {
	class = imsg->Class;
	code = imsg->Code;
	switch( class )
	{
	  case IDCMP_GADGETUP:
		switch( ( (struct Gadget *)(imsg->IAddress) )->GadgetID )
		{
		  case ID_PROCEEDGAD:
		    finish = TRUE;
		    break;
		  case ID_ABORTGAD:
		    abort_install( &request_string_destruct );
		    break;
		  case ID_HELPGAD:
#warning FIXME: What is this help like?
		    setgadgetdisable( PROCEEDGAD|ABORTGAD|SKIPGAD|HELPGAD );
		    m = GetPL( pl, _HELP ).intval;
		    if(m)
		    {
#ifdef DEBUG
		      for( n = 0 ; n < m ; n++ )
		      {
			printf( "%s\n", GetPL( pl, _HELP ).arg[n] );
		      }
#endif /* DEBUG */
		      morenmain( HELP_ON_ASKSTRING, m, GetPL( pl, _HELP ).arg );
		    }
		    else
		    {
#ifdef DEBUG
		      printf( "%s\n", get_var_arg( "@askstring-help" ) );
#endif /* DEBUG */
		      moremain( HELP_ON_ASKSTRING, get_var_arg( "@askstring-help" ) );
		    }
		    setgadgetdisable( SKIPGAD );
		    break;
		  default:
		    break;
		}
		break;
	  default:
		break;
	}
	GT_ReplyIMsg(imsg);

      } /* while ((imsg = GT_GetIMsg( GuiWin->UserPort )) */
    } /* !finish */

    GT_GetGadgetAttrs(stringgad,GuiWin,NULL,GTST_String,(IPTR)&string,TAG_DONE);

    request_string_destruct();
  }

  retval = addquotes( string );
  if( preferences.transcriptstream != NULL )
  {
  char tmpbuffer[MAXARGSIZE];
    sprintf( tmpbuffer, "Ask String: Result was %s.\n\n", retval );
    Write( preferences.transcriptstream, tmpbuffer, strlen( tmpbuffer ) );
  }

return retval;
}

/*
 * Ask user to choose one of N items
 */
void request_choice_destruct()
{
}
long int request_choice( struct ParameterList *pl )
{
int i,j,m,n;
long int retval;
char **out;
int max, finish = FALSE;

  retval = GetPL( pl, _DEFAULT ).intval;
  if( preferences.transcriptstream != NULL )
  {
    for( i = 0 ; i < GetPL( pl, _PROMPT ).intval ; i ++ )
    {
    char tmpbuffer[MAXARGSIZE];
      sprintf( tmpbuffer, ">%s\n", GetPL( pl, _PROMPT ).arg[i] );
      Write( preferences.transcriptstream, tmpbuffer, strlen( tmpbuffer ) );
    }
  }
  max = GetPL( pl, _CHOICES ).intval;
  if( max > 32 )
  {
    error = SCRIPTERROR;
    traperr( "More than 32 choices given!\n", NULL );
  }
  else if( max < 1 )
  {
    error = SCRIPTERROR;
    traperr( "No choices given!\n", NULL );
  }

  if( get_var_int( "@user-level" ) > _NOVICE )
  {
    clear_gui();

    RefreshGList(stdglist,GuiWin,NULL,-1);
    GT_RefreshWindow(GuiWin,NULL);

    mxlabels = AllocVec( (max+1)*sizeof(STRPTR), MEMF_PUBLIC );
    for( i = 0 ; i < max ; i ++ )
    {
      mxlabels[i] = StrDup(GetPL( pl, _CHOICES ).arg[i] );
    }
    mxlabels[i] = NULL;

    gad = CreateContext( &mxglist );
    if(gad==NULL)
      fprintf( stderr, "CreateContext() failed\n");

    gad = CreateGadget( MX_KIND, gad, &gt_mxgad,
			GTMX_Labels, (IPTR)mxlabels,
			GTMX_Scaled, TRUE,
			GTMX_TitlePlace, PLACETEXT_ABOVE,
			GTMX_Active, retval,
			GA_Immediate, TRUE,
			TAG_DONE );

    AddGList( GuiWin, mxglist ,-1 ,-1 ,NULL );
    RefreshGList( stdglist ,GuiWin ,NULL ,-1 );
    GT_RefreshWindow( GuiWin ,NULL );
    DrawBevelBoxA(rp, 5,5,GuiWin->Width-15-GuiWin->BorderLeft,GuiWin->Height-65-GuiWin->BorderTop,bevel_tag);
    DrawBevelBoxA(rp, 15,12,GuiWin->Width-35-GuiWin->BorderLeft,GuiWin->Height-160-GuiWin->BorderTop,bevel_tag);
    j = 0;
    for( i = 0 ; i < GetPL( pl, _PROMPT ).intval ; i ++ )
    {
#ifdef DEBUG
      printf( "%s\n", GetPL( pl, _PROMPT ).arg[i] );
#endif /* DEBUG */
      out = AllocVec( sizeof( char * ), MEMF_PUBLIC );
      outofmem( out );
      out[0] = NULL;
      m = strtostrs( GetPL( pl, _PROMPT ).arg[i], &out );
      for( n = 0 ; n < m ; n++ )
      {
	Move( rp, 18, 15*j+22 );
	Text( rp, out[n], strlen(out[n]) );
	FreeVec( out[n] );
	j++;
      }
      FreeVec( out );
    }

    finish = FALSE;
    while (!finish)
    {
      WaitPort( GuiWin->UserPort );
      while ((imsg = GT_GetIMsg( GuiWin->UserPort )))
      {
	class = imsg->Class;
	code = imsg->Code;
	switch( class )
	{
	  case IDCMP_GADGETUP:
		switch( ( (struct Gadget *)(imsg->IAddress) )->GadgetID )
		{
		  case ID_PROCEEDGAD:
		    finish = TRUE;
		    break;
		  case ID_ABORTGAD:
		    abort_install( &request_choice_destruct );
		    break;
		  case ID_HELPGAD:
		    setgadgetdisable( PROCEEDGAD|ABORTGAD|SKIPGAD|HELPGAD );
		    m = GetPL( pl, _HELP ).intval;
		    if(m)
		    {
#ifdef DEBUG
		      for( n = 0 ; n < m ; n++ )
		      {
			printf( "%s\n", GetPL( pl, _HELP ).arg[n] );
		      }
#endif /* DEBUG */
		      morenmain( HELP_ON_ASKCHOICE, m, GetPL( pl, _HELP ).arg );
		    }
		    else
		    {
#ifdef DEBUG
		      printf( "%s\n", get_var_arg( "@askchoice-help" ) );
#endif /* DEBUG */
		      moremain( HELP_ON_ASKCHOICE, get_var_arg( "@askchoice-help" ) );
		    }
		    setgadgetdisable( SKIPGAD );
		    break;
		  default:
		    break;
		}
		break;
	  default:
		break;
	}
	GT_ReplyIMsg(imsg);

      } /* while ((imsg = GT_GetIMsg( GuiWin->UserPort )) */
    } /* !finish */

    GT_GetGadgetAttrs( gad, GuiWin, NULL, GTMX_Active, (IPTR)&retval, TAG_DONE );

    request_choice_destruct();

    RemoveGList(GuiWin,mxglist,-1);
    GT_RefreshWindow(GuiWin,NULL);
    FreeGadgets( mxglist );
    freestrlist( mxlabels );
  }

  if( preferences.transcriptstream != NULL )
  {
  char tmpbuffer[MAXARGSIZE];
    sprintf( tmpbuffer, "Ask Choice: Result was \"%s\".\n\n", GetPL( pl, _CHOICES ).arg[retval] );
    Write( preferences.transcriptstream, tmpbuffer, strlen( tmpbuffer ) );
  }

return retval;
}


/*
 * Ask user for a directory
 */
#warning TODO: write whole function
void request_dir_destruct()
{
}
char *request_dir( struct ParameterList *pl )
{
char *retval, *string;

  if( GetPL( pl, _DEFAULT ).used == 0 )
  {
    error = SCRIPTERROR;
    traperr( "No default specified!", NULL );
  }

  string = GetPL( pl, _DEFAULT ).arg[0];
  retval = addquotes( string );

return retval;
}


/*
 * Ask user to insert a specific disk
 */
#warning TODO: write whole function
void request_disk_destruct()
{
}
char *request_disk( struct ParameterList *pl )
{
char *retval, *string;

  if( GetPL( pl, _DEST ).used == 0 )
  {
    error = SCRIPTERROR;
    traperr( "No dest specified!", NULL );
  }

  string = GetPL( pl, _DEST ).arg[0];
  retval = addquotes( string );

return retval;
}


/*
 * Ask user for a file
 */
#warning TODO: write whole function
void request_file_destruct()
{
}
char *request_file( struct ParameterList *pl )
{
char *retval, *string;
struct TagItem frtags[] =
{
    { ASL_Height,	  0 },
    { ASL_Width,	  0 },
    { ASL_LeftEdge,	320 },
    { ASL_TopEdge,	 40 },
    { ASLFR_TitleText,		(ULONG)"Choose a file:" },
    { ASLFR_PositiveText,	(ULONG)"Okay"		},
    { ASLFR_NegativeText,	(ULONG)"Cancel"		},
    { ASLFR_InitialFile,	(ULONG)"asl.library"	},
    { ASLFR_InitialDrawer,	(ULONG)"libs:"		},
    { ASLFR_PubScreenName,	NULL },
    { TAG_DONE }
};

struct FileRequester *fr;


  if( GetPL( pl, _DEFAULT ).used == 0 )
  {
    error = SCRIPTERROR;
    traperr( "No default specified!", NULL );
  }

#if 1
  if ((fr = (struct FileRequester *) AllocAslRequest(ASL_FileRequest, frtags)))
  {
    if (AslRequest(fr, NULL) != FALSE)
    {
      printf("PATH=%s  FILE=%s\n", fr->rf_Dir, fr->rf_File);
/*
	To combine the path and filename, copy the path to a buffer,
	add the filename with Dos AddPart().
*/
    }
    FreeAslRequest(fr);
  }
  else printf("User Cancelled\n");
#endif

  string = GetPL( pl, _DEFAULT ).arg[0];
  retval = addquotes( string );

return retval;
}


/*
 * Ask user for a selection of multiple items (choose m of n items)
 */
#warning TODO: write whole function
void request_options_destruct()
{
}
long int request_options( struct ParameterList *pl )
{
long int retval;

  retval = GetPL( pl, _DEFAULT ).intval;

return retval;
}


/*
 * Ask user to confirm
 */
void request_confirm_destruct()
{
}
int request_confirm( struct ParameterList * pl )
{
int m, n, i, j, finish = FALSE;
int retval = 1;
char **out;

  if( get_var_int( "@user-level" ) >= GetPL( pl, _CONFIRM).intval )
  {

    setgadgetdisable( (GetPL( pl, _HELP ).used == 1) ? 0 : HELPGAD );
    clear_gui();
    RefreshGList(stdglist,GuiWin,NULL,-1);
    GT_RefreshWindow(GuiWin,NULL);

    m = GetPL( pl, _PROMPT ).intval;
    if( m == 0 )
    {
      error = SCRIPTERROR;
      traperr( "Missing prompt!\n", NULL );
    }
    if( GetPL( pl, _HELP ).intval == 0 )
    {
      error = SCRIPTERROR;
      traperr( "Missing help!\n", NULL );
    }

    j = 0;
    for( i = 0 ; i < GetPL( pl, _PROMPT ).intval ; i ++ )
    {
#ifdef DEBUG
      printf( "%s\n", GetPL( pl, _PROMPT ).arg[i] );
#endif /* DEBUG */
      out = AllocVec( sizeof( char * ), MEMF_PUBLIC );
      outofmem( out );
      out[0] = NULL;
      m = strtostrs( GetPL( pl, _PROMPT ).arg[i], &out );
      for( n = 0 ; n < m ; n++ )
      {
	Move( rp, 15, 15*j+22 );
	Text( rp, out[n], strlen(out[n]) );
	FreeVec( out[n] );
	j++;
      }
      FreeVec( out );
    }

    finish = FALSE;
    while (!finish)
    {
	WaitPort( GuiWin->UserPort );
	while ((imsg = GT_GetIMsg( GuiWin->UserPort )))
	{
	  class = imsg->Class;
	  code = imsg->Code;
	  switch( class )
	  {
	    case IDCMP_GADGETUP:
		  switch( ( (struct Gadget *)(imsg->IAddress) )->GadgetID )
		  {
		    case ID_PROCEEDGAD:
		      finish = TRUE;
		      break;
		    case ID_SKIPGAD:
		      retval = 0;
		      finish = TRUE;
		      break;
		    case ID_ABORTGAD:
		      abort_install( &request_confirm_destruct );
		      break;
		    case ID_HELPGAD:
		      m = GetPL( pl, _HELP ).intval;
#ifdef DEBUG
		      for( n = 0 ; n < m ; n++ )
		      {
		        printf( "%s\n", GetPL( pl, _HELP ).arg[n] );
		      }
#endif /* DEBUG */
		      setgadgetdisable( PROCEEDGAD|ABORTGAD|SKIPGAD|HELPGAD );
		      morenmain( HELP_ON_CONFIRM, m, GetPL( pl, _HELP ).arg );
		      setgadgetdisable( 0 );
		      break;
		    default:
		      break;
		  }
		  break;
	    default:
		  break;
	  }
	  GT_ReplyIMsg(imsg);
	
	} /* while ((imsg = GT_GetIMsg( GuiWin->UserPort )) */
      } /* !finish */

  }
  else
  {
    return 1;
  }

return retval;
}


/*
 * Ask user if he really wants to abort
 */
struct IntuiText p = {1,0,JAM1,10,30,NULL,(UBYTE *)"OK",NULL };
struct IntuiText n = {1,0,JAM1,70,30,NULL,(UBYTE *)"Cancel",NULL };
struct IntuiText abortq_body = {1,0,JAM1,0,0,NULL,(UBYTE *)"Do you really want to quit Installer?",NULL };
struct IntuiText displaytext = {1,0,JAM1,0,0,NULL,NULL,NULL };

void abort_install( VOID_FUNC destructor )
{
  if( AutoRequest(GuiWin,&abortq_body,&p,&n,(scr->Width-200)/2,(scr->Height-75)/2,200,75) != 0 )
  {
    error = USERABORT;
    grace_exit = TRUE;
    if( destructor )
    {
      destructor();
    }
    /* Execute trap(1) */
    traperr( "User aborted!\n", NULL );
  }
}


/*
 * Give a short summary on what was done
 */
void final_report( )
{
#ifdef DEBUG
  printf( "Application has been installed in %s.\n", get_var_arg( "@default-dest" ) );
#endif /* DEBUG */
}


/*
 * Enable/Disable Gadgets in stdglist
 */
void setgadgetdisable( int list )
{
    GT_SetGadgetAttrs( proceedgad, GuiWin, NULL,
			GA_Disabled, (list & PROCEEDGAD) ? TRUE : FALSE,
			TAG_DONE );
    GT_SetGadgetAttrs( abortgad, GuiWin, NULL,
			GA_Disabled, (list & ABORTGAD) ? TRUE : FALSE,
			TAG_DONE );
    GT_SetGadgetAttrs( skipgad, GuiWin, NULL,
			GA_Disabled, (list & SKIPGAD) ? TRUE : FALSE,
			TAG_DONE );
    GT_SetGadgetAttrs( helpgad, GuiWin, NULL,
			GA_Disabled, (list & HELPGAD) ? TRUE : FALSE,
			TAG_DONE );
}
void setaboutgaddisable( int disable )
{
    GT_SetGadgetAttrs( aboutgad, GuiWin, NULL,
			GA_Disabled, disable,
			TAG_DONE );
}


void display_text( char * msg )
{
  displaytext.IText = msg;
#ifdef DEBUG
  printf( "ERROR: %s\n", msg );
#endif /* DEBUG */
  AutoRequest(GuiWin,&displaytext,&p,NULL,0L,0L,200,75);
}


