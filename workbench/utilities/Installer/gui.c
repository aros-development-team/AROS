/* gui.c -- here are all functions for the gui */

/*
  Comment out the next line if do not you want to use the
  experimental Intuition based GUI
*/
#define INTUIGUI 1

#include "Installer.h"
#include "execute.h"

/* External variables */
extern BPTR inputfile;
extern char buffer[MAXARGSIZE];
extern char *filename;
extern InstallerPrefs preferences;
extern int error, grace_exit;

/* External function prototypes */
extern void cleanup();
extern char *get_var_arg( char * );
extern long int get_var_int( char * );
extern void execute_script( ScriptArg *, int );
extern void set_variable( char *, char *, long int );
#ifdef DEBUG
extern void dump_varlist();
#endif /* DEBUG */
extern void end_malloc();
extern void outofmem( void * );
extern int strtostrs ( char *, char *** );
extern char *addquotes( char * );

/* Internal function prototypes */
struct Border * genborder( int, int );
void init_gui();
void deinit_gui();
void clear_gui();
void show_abort( char * );
void show_complete( long int );
void show_exit( char * );
void show_parseerror( char *, int );
void show_working( char * );
void show_message( char * ,struct ParameterList * );
void show_help_userlevel();
void show_help_pretend();
void show_help_logfile();
void show_help_installer();
void request_userlevel();
long int request_bool( struct ParameterList * );
long int request_number( struct ParameterList * );
char *request_string( struct ParameterList * );
long int request_choice( struct ParameterList * );
char *request_dir( struct ParameterList * );
char *request_disk( struct ParameterList * );
char *request_file( struct ParameterList * );
long int request_options( struct ParameterList * );
int request_confirm( struct ParameterList *, long int );
void abort_install();
void final_report();
void traperr( char *, char * );


#include <proto/intuition.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <proto/graphics.h>
#include <graphics/gfxbase.h>
#include <proto/gadtools.h>
#include <libraries/gadtools.h>
#include <proto/asl.h>
#include <libraries/asl.h>


struct IntuitionBase *IntuitionBase = NULL;
struct Library *GadToolsBase = NULL;
struct GfxBase * GfxBase = NULL;
struct Library *AslBase = NULL;

struct Window *GuiWin;
struct RastPort *rp;
struct IntuiMessage *imsg;
ULONG class;
UWORD code;

const char GuiWinTitle[] ="AROS - Installer V43.3";

APTR vi;
struct Screen *scr;
struct Gadget *glist = NULL,
	      *stringglist = NULL,
	      *mxglist = NULL,
	      *stdglist = NULL,
	      *gad = NULL;

struct IntuiText itext = {
    1,		/* FrontPen */
    0,		/* BackPen */
    JAM1,	/* DrawMode */
    10,		/* LeftEdge */
    10,		/* TopEdge */
    NULL,	/* ITextFont */
    NULL,	/* IText */
    NULL	/* NextText */
};

#define ID_BOOLGADFALSE 0
struct NewGadget gt_boolgadfalse = {
  15,92, 30,30, /* ng_LeftEdge, ng_TopEdge, ng_Width, ng_Height */
  NULL,		/* ng_GadgetText */
  NULL,		/* ng_TextAttr */
  ID_BOOLGADFALSE, /* ng_GadgetID */
  PLACETEXT_IN, /* ng_Flags */
  NULL,		/* ng_VisualInfo */
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
  15,90, MX_WIDTH, MX_HEIGHT,
  NULL, NULL,
  ID_MXGAD, PLACETEXT_RIGHT, NULL, NULL
};
STRPTR *mxlabels;

#define ID_ABORTGAD 100
struct NewGadget gt_abortgad = {
  15,200, 74,25,
  "Abort", NULL,
  ID_ABORTGAD, PLACETEXT_IN, NULL, NULL
};

#define ID_SKIPGAD 101
struct NewGadget gt_skipgad = {
  90,200, 74,25,
  "Skip", NULL,
  ID_SKIPGAD, PLACETEXT_IN, NULL, NULL
};

#define ID_HELPGAD 102
struct NewGadget gt_helpgad = {
  165,200, 74,30,
  "Help", NULL,
  ID_HELPGAD, PLACETEXT_IN, NULL, NULL
};

struct TagItem bevel_tag[] = {
  { GT_VisualInfo, 0 },
  { GTBB_Recessed, TRUE },
  { GTBB_FrameType, BBFT_RIDGE },
  { TAG_DONE }
};

/*
 * Initialize the GUI
 */
void init_gui( )
{
struct TagItem windowtags[] =
{
  { WA_Width	, 400 },
  { WA_Height	, 250 },
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

struct TagItem ti1[] = {
  { GA_Immediate, TRUE },
  { TAG_DONE }
};

STRPTR defstring = "Blah";
struct TagItem stringti[] = {
  { GTST_String, (ULONG)defstring },
  { GTST_MaxChars, 8 },
  { GTTX_Border, TRUE },
  { GA_Immediate, TRUE },
  { TAG_DONE }
};
/*
struct TagItem ti2[] = {
  { GTTX_Text, (ULONG)"dummystring" },
  { GTTX_CopyText, TRUE },
  { GTTX_Border, TRUE },
  { GTTX_Justification, GTJ_LEFT },
  { TAG_DONE } };
*/

  IntuitionBase = (struct IntuitionBase *)OpenLibrary( "intuition.library", 37L );
  if (IntuitionBase == NULL)
  {
    cleanup();
    exit(-1);
  }

  GfxBase = (struct GfxBase *)OpenLibrary( "graphics.library", 37L );
  if (GfxBase == NULL)
  {
    cleanup();
    exit(-1);
  }

  GadToolsBase = OpenLibrary( "gadtools.library", 0L );
  if (GadToolsBase == NULL)
  {
    cleanup();
    exit(-1);
  }

  AslBase = OpenLibrary( "asl.library", 37L );
  if (AslBase == NULL)
  {
    cleanup();
    exit(-1);
  }

  if( NULL == ( GuiWin = OpenWindowTagList( NULL, windowtags ) ) )
  {
    cleanup();
    CloseLibrary( (struct Library *)IntuitionBase );
    exit(-1);
  }
  rp = GuiWin->RPort;

  scr = LockPubScreen( NULL );
  vi = GetVisualInfoA( scr, NULL );
  bevel_tag[0].ti_Data = (ULONG)vi;

  gt_boolgadtrue.ng_VisualInfo = vi;
  gt_boolgadfalse.ng_VisualInfo = vi;
  gt_textgad.ng_VisualInfo = vi;
  gt_stringgad.ng_VisualInfo = vi;
  gt_mxgad.ng_VisualInfo = vi;
  gt_abortgad.ng_VisualInfo = vi;
  gt_skipgad.ng_VisualInfo = vi;

  gad = CreateContext( &glist );
  if(gad==NULL)
    printf("CreateContext() failed\n");

  gad = CreateGadgetA( BUTTON_KIND, gad, &gt_boolgadtrue, ti1 );
  gad = CreateGadgetA( BUTTON_KIND, gad, &gt_boolgadfalse, ti1 );
//  gad = CreateGadgetA( TEXT_KIND, gad, &gt_textgad, ti2 );

  gad = CreateContext( &stringglist );
  if(gad==NULL)
    printf("CreateContext() failed\n");

  gad = CreateGadgetA( STRING_KIND, gad, &gt_stringgad, stringti );

  gad = CreateContext( &stdglist );
  if(gad==NULL)
    printf("CreateContext() failed\n");

  gad = CreateGadgetA( BUTTON_KIND, gad, &gt_abortgad, ti1 );
  gad = CreateGadgetA( BUTTON_KIND, gad, &gt_skipgad, ti1 );
  gad = CreateGadgetA( BUTTON_KIND, gad, &gt_helpgad, ti1 );
}


/*
 * Close GUI
 */
void deinit_gui( )
{
  FreeGadgets( glist );
  FreeGadgets( stringglist );
  FreeVisualInfo( vi );
  UnlockPubScreen( NULL, scr );
  CloseWindow( GuiWin );
  CloseLibrary( (struct Library *)AslBase );
  CloseLibrary( (struct Library *)GadToolsBase );
  CloseLibrary( (struct Library *)GfxBase );
  CloseLibrary( (struct Library *)IntuitionBase );
}


/*
 * Clean the GUI display
 */
void clear_gui()
{
  EraseRect( rp, 0, 0, GuiWin->Width, GuiWin->Height );
}


/*
 * Show user that we are going to "(abort)" install
 * Don't confuse NOVICE...
 */
void show_abort( char *msg )
{
char **out, *text;
int n, m;

  if( get_var_int( "@user-level" ) > _NOVICE )
  {
    clear_gui();

    text = strdup( "Aborting Installation:" );
    outofmem( text );
    itext.IText = text;
    PrintIText( rp, &itext, 10, 10 );
    free( text );
    out = malloc( sizeof( char * ) );
    outofmem( out );
    out[0] = NULL;
    m = strtostrs( msg, &out );
    for( n = 0 ; n < m ; n++ )
    {
#ifdef DEBUG
      printf( "%s\n", out[n] );
#endif /* DEBUG */
      itext.IText = out[n];
      PrintIText( rp, &itext, 10, 15*(n+2) );
    }
    printf( " Press Return to Proceed\n" );
    scanf("x");
  }
}


/*
 * Show user how much we have completed yet
 */
void show_complete( long int percent )
{
char *text;

  text = malloc( strlen( GuiWinTitle ) + 13);
  if( text == NULL )
  {
    end_malloc();
  }
  sprintf( text, "%s (Done %3ld%c)", GuiWinTitle, percent, PERCENT );
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

  text = strdup( "Aborting Installation:" );
  outofmem( text );
  itext.IText = text;
  PrintIText( rp, &itext, 10, 10 );
  free( text );
  out = malloc( sizeof( char * ) );
  outofmem( out );
  out[0] = NULL;
  m = strtostrs( msg, &out );
  for( n = 0 ; n < m ; n++ )
  {
#ifdef DEBUG
    printf( "%s\n", out[n] );
#endif /* DEBUG */
    itext.IText = out[n];
    PrintIText( rp, &itext, 10, 15*(n+2) );
  }
  text = strdup( "Done with Installation." );
  outofmem( text );
  itext.IText = text;
  PrintIText( rp, &itext, 10, 15*(n+2) );
  free( text );
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
  printf( "Syntax error in line %d:\n", errline );
  if( msg != NULL )
  {
    printf( "%s\n",msg );
  }
#endif /* DEBUG */
  inputfile = Open( filename, MODE_OLDFILE );
  if( inputfile == NULL )
  {
    PrintFault( IoErr(), "Installer" );
    cleanup();
    exit(-1);
  }
  errline--;
  while( count != 0 && errline > 0 )
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
  } while( buffer[i] != LINEFEED && count != 0 && i < MAXARGSIZE );
  buffer[i] = 0;
#ifdef DEBUG
  printf( "%s\n", buffer );
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

  text = strdup( "Working on Installation:" );
  outofmem( text );
  itext.IText = text;
  PrintIText( rp, &itext, 10, 10 );
#ifdef DEBUG
  printf( "%s\n", text );
#endif /* DEBUG */
  free( text );
  out = malloc( sizeof( char * ) );
  outofmem( out );
  out[0] = NULL;
  m = strtostrs( msg, &out );
  for( n = 0 ; n < m ; n++ )
  {
#ifdef DEBUG
    printf( "%s\n", out[n] );
#endif /* DEBUG */
    itext.IText = out[n];
    PrintIText( rp, &itext, 10, 15*(n+2) );
  }
}


/*
 * Display a "(message)" to the user
 * Don't confuse NOVICE unless "(all)" users want to get this info
 */
void show_message( char * msg ,struct ParameterList * pl )
{
char **out;
int n, m;
char c;
int finish = FALSE;

  if( GetPL( pl, _ALL ).used == 1 || get_var_int( "@user-level" ) > _NOVICE )
  {
    clear_gui();

    out = malloc( sizeof( char * ) );
    outofmem( out );
    out[0] = NULL;
    m = strtostrs( msg, &out );
    for( n = 0 ; n < m ; n++ )
    {
#ifdef DEBUG
      printf( "%s\n", out[n] );
#endif /* DEBUG */
      itext.IText = out[n];
      PrintIText( rp, &itext, 10, 15*(n+1) );
    }
    do
    {
      printf( " P - Proceed\n A - Abort\n H - Help\n" );
      scanf( "%c", &c );
      switch( tolower( c ) )
      {
        case 'p': /* Proceed */
                  finish = TRUE;
                  break;
        case 'a': /* abort */
                  abort_install();
                  break;
        case 'h': /* help */
                  m = GetPL( pl, _HELP ).intval;
                  for( n = 0 ; n < m ; n++ )
                  {
                    printf( "%s\n", GetPL( pl, _HELP ).arg[n] );
                  }
                  if( m == 0 )
                  {
#warning FIXME: What default help text is used?
                    printf( "Press Proceed to continue or Abort to abort\n" );
                  }
                  break;
        default	: break;
      }
    } while( !finish );
  }
}


/*
 * Show the help-window for topic: User-Level
 */
void show_help_userlevel( )
{
#warning TODO: help for userlevel-requester
#ifdef DEBUG
  printf( "\n NOVICE won't be asked anymore questions\n AVERAGE will have to interact\n EXPERT must confirm all actions\n" );
#endif /* DEBUG */
}


/*
 * Show the help-window for topic: Log-File
 */
void show_help_logfile()
{
#warning TODO: help for logfile-requester
#ifdef DEBUG
  printf( "\n Printer will go to PRT:\n Log File will be %s\n",preferences.transcriptfile );
#endif /* DEBUG */
}


/*
 * Show the help-window for topic: Pretend to install
 */
void show_help_pretend()
{
#warning TODO: help for pretend-requester
#ifdef DEBUG
  printf( "\n \"Pretend to Install\" will not install the application for real\n on your Disk, instead it will log all the actions to the log-file.\n However if the \"(safe)\" flag is set for functions in the script\n these will bedone even if in pretend mode.\n" );
#endif /* DEBUG */
}


/*
 * Show the help-window for topic: Installer
 */
void show_help_installer( )
{
#warning TODO: help/about for Installer
#ifdef DEBUG
  printf( "\nThis is AROS Installer V%d.%d\niIt is intended to be compatible to Installer V43.3\n\nThis program was written by Henning Kiel <hkiel@aros.org>\n\n", INSTALLER_VERSION, INSTALLER_REVISION );
#endif /* DEBUG */
}


/*
 * Ask user for his user-level
 */
void request_userlevel( char *msg )
{
int usrlevel, finish = FALSE;

#ifdef DEBUG
char c;

  usrlevel = preferences.defusrlevel;
  if( msg != NULL )
  {
    printf( "%s\n", msg );
  }
  else
  {
    printf( "Welcome to the %s App installation utility!\n", get_var_arg( "@app-name" ) );
  }

  do
  {
    printf( "Which user-level do you want?\n 0 - Novice\n 1 - Average\n 2 - Expert\n P - Proceed\n A - Abort\n H - Help\n O - About Installer\n\nMinimum user-level is %d\nDefault is %d\n", preferences.minusrlevel, usrlevel );
    scanf( "%c", &c );
    switch( tolower( c ) )
    {
      case 'a'	: /* abort */
                  abort_install();
                  break;
      case 'h'	: /* help */
                  show_help_userlevel();
                  break;
      case 'p'	: /* proceed */
                  finish = TRUE;
                  break;
      case 'o'	: /* about */
                  show_help_installer();
                  break;
      case '0'	: /* novice */
                  usrlevel = _NOVICE;
                  break;
      case '1'	: /* average */
                  usrlevel = _AVERAGE;
                  break;
      case '2'	: /* expert */
                  usrlevel = _EXPERT;
                  break;
      default	: break;
    }
    if( usrlevel < preferences.minusrlevel )
    {
      printf("Sorry, but minimum user-level is %d\n", preferences.minusrlevel );
      usrlevel = preferences.defusrlevel;
    }
  } while( finish == FALSE );
#endif /* DEBUG */

  set_variable( "@user-level", NULL, usrlevel );

  finish = FALSE;
  if( usrlevel > 0 )
  {
    /* Ask for logfile-creation */
#ifdef DEBUG
    do
    {
      printf( "Installer can log all actions.\nDo you want to\n F - write a log File\n P - log to Printer\n N - disable logging\n H - Help\n A - Abort installation ?\n" );
      scanf( "%c", &c );
      switch( tolower( c ) )
      {
        case 'a': /* abort */
                  abort_install();
                  break;
        case 'h': /* help */
                  show_help_logfile();
                  break;
        case 'f': /* log-file */
#if 0
                  /* fopen() log file */
#endif /* 0 */
                  finish = TRUE;
                  break;
        case 'p': /* printer-log */
#if 0
                  free( preferences.transcriptfile );
                  preferences.transcriptfile = strdup ("PRT:");
#endif /* 0 */
                  finish = TRUE;
                  break;
        case 'n': /* no log */
#if 0
/* fopen() printer log-"file" */
                  free( preferences.transcriptfile );
                  preferences.transcriptfile = NULL;
#endif /* 0 */
                  finish = TRUE;
                  break;
        default	: break;
      }
    } while( finish == FALSE );

    if(!preferences.nopretend)
    {
      finish = FALSE;
      do
      {
        printf( "Installer can pretend to install\n I - really Install\n P - Pretend to install\n H - Help\n A - Abort installation ?\n" );
        scanf( "%c", &c );
        switch( tolower( c ) )
        {
          case 'a': /* abort */
                    abort_install();
                    break;
          case 'h': /* help */
                    show_help_pretend();
                    break;
          case 'i': /* Really Install */
                    preferences.pretend = FALSE;
                    finish = TRUE;
                    break;
          case 'p': /* Only pretend to install */
                    preferences.pretend = TRUE;
                    finish = TRUE;
                    break;
          default : break;
        }
      } while( finish == FALSE );
    }
  }
#endif /* DEBUG */
}


/*
 * Ask user for a boolean
 */
long int request_bool( struct ParameterList *pl)
{
int i;
long int retval;
char yes[] = "Yes", no[] = "No", *yesstring, *nostring;
int finish = FALSE;
#ifndef INTUIGUI
char c;
#endif

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
#ifdef INTUIGUI
  {
    char **out;
    int j, n, m;

    clear_gui();

    AddGList(GuiWin,glist,-1,-1,NULL);
    RefreshGList(glist,GuiWin,NULL,-1);
    AddGList(GuiWin,stdglist,-1,-1,NULL);
    RefreshGList(stdglist,GuiWin,NULL,-1);
    GT_RefreshWindow(GuiWin,NULL);
    DrawBevelBoxA(rp, 5,5,GuiWin->Width-15-GuiWin->BorderLeft,GuiWin->Height-65-GuiWin->BorderTop,bevel_tag);
    DrawBevelBoxA(rp, 15,12,GuiWin->Width-35-GuiWin->BorderLeft,GuiWin->Height-160-GuiWin->BorderTop,bevel_tag);
        
#if 1

    itext.IText = nostring;
    PrintIText( rp, &itext, 40, 92 );
    itext.IText = yesstring;
    PrintIText( rp, &itext, 40, 132 );

    j = 0;
    for( i = 0 ; i < GetPL( pl, _PROMPT ).intval ; i ++ )
    {
#ifdef DEBUG
      printf( "%s\n", GetPL( pl, _PROMPT ).arg[i] );
#endif /* DEBUG */
      out = malloc( sizeof( char * ) );
      outofmem( out );
      out[0] = NULL;
      m = strtostrs( GetPL( pl, _PROMPT ).arg[i], &out );
      for( n = 0 ; n < m ; n++ )
      {
        itext.IText = out[n];
        PrintIText( rp, &itext, 15, 15*j+7 );
        free( out[n] );
        j++;
      }
      free( out );
    }
    do
    {
      WaitPort( GuiWin->UserPort );
      while((imsg = GT_GetIMsg( GuiWin->UserPort )))
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
                    abort_install();
                    break;
                  case ID_HELPGAD:
                    for( i = 0 ; i < GetPL( pl, _HELP ).intval ; i ++ )
                    {
                      printf( "%s\n", GetPL( pl, _HELP ).arg[i] );
                    }
                    if( i == 0 )
                    {
  #warning FIXME: What default help text is used?
                      printf( "%s\n", get_var_arg( "@asknumber-help" ) );
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
	
      } /* while((imsg = GT_GetIMsg( GuiWin->UserPort )) */
      
    } while( !finish );
    RemoveGList(GuiWin,glist,-1);
    RemoveGList(GuiWin,stdglist,-1);
    GT_RefreshWindow(GuiWin,NULL);
#else
    Delay( 100 );
#endif
  }
#else /* INTUIGUI */
  {
    do
    {
      for( i = 0 ; i < GetPL( pl, _PROMPT ).intval ; i ++ )
      {
        printf( "%s\n", GetPL( pl, _PROMPT ).arg[i] );
      }
      printf( "Default is %ld.\n", retval );
      printf( " 1 - %s\n 0 - %s\n A - Abort\n H - Help\n", yesstring, nostring );
      scanf( "%c", &c );
      switch( tolower( c ) )
      {
        case '1' : /* return TRUE */
                   retval = 1;
                   finish = TRUE;
                   break;
        case '0' : /* return FALSE */
                   retval = 0;
                   finish = TRUE;
                   break;
        case 'a' : /* abort */
                   abort_install();
                   break;
        case 'h' : /* help */
                   for( i = 0 ; i < GetPL( pl, _HELP ).intval ; i ++ )
                   {
                     printf( "%s\n", GetPL( pl, _HELP ).arg[i] );
                   }
                   if( i == 0 )
                   {
#warning FIXME: What default help text is used?
                     printf( "%s\n", get_var_arg( "@asknumber-help" ) );
                   }
                   break;
        default  : break;
      }
    } while( !finish );
  }
#endif /* INTUIGUI */

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
long int request_number( struct ParameterList *pl)
{
int i;
long int retval, min, max;
#ifdef DEBUG
char buffer[MAXARGSIZE];
int finish = FALSE;
char c;

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
#define INTMAX  32767
    max = INTMAX;
    min =( retval < 0 ) ?
         retval :
         0;
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
  retval = GetPL( pl, _DEFAULT ).intval;
  if( get_var_int( "@user-level" ) > _NOVICE )
  do
  {
    for( i = 0 ; i < GetPL( pl, _PROMPT ).intval ; i ++ )
    {
      printf( "%s\n", GetPL( pl, _PROMPT ).arg[i] );
    }
    printf( "Number [%ld,%ld] is %ld.\n", min, max, retval );
    printf( " V - Change value\n P - Proceed\n A - Abort\n H - Help\n" );
    scanf( "%c", &c );
    switch( tolower( c ) )
    {
      case 'v'	: /* change value */
                  scanf( "%s", buffer );
                  i = atol( buffer );
                  if( i < min || i > max )
                  {
                    printf( "Input out of range!\n" );
                  }
                  else
                  {
                    retval = i;
                  }
                  break;
      case 'p'	: /* proceed */
                  if( retval >= min && retval <= max )
                  {
                    finish = TRUE;
                  }
                  break;
      case 'a'	: /* abort */
                  abort_install();
                  break;
      case 'h'	: /* help */
                  for( i = 0 ; i < GetPL( pl, _HELP ).intval ; i ++ )
                  {
                    printf( "%s\n", GetPL( pl, _HELP ).arg[i] );
                  }
                  if( i == 0 )
                  {
                    printf( "%s\n", get_var_arg( "@asknumber-help" ) );
                  }
                  break;
      default	: break;
    }
  } while( !finish );

#endif /* DEBUG */
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
char *request_string( struct ParameterList *pl)
{
int i;
char *retval,*string;
#ifdef DEBUG
char buffer[MAXARGSIZE];
int finish = FALSE;
char c;

  if( GetPL( pl, _DEFAULT ).used == 1 )
  {
    string = strdup( GetPL( pl, _DEFAULT ).arg[0] );
  }
  else
  {
    string = strdup( "" );
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

  clear_gui();

  AddGList(GuiWin,stringglist,-1,-1,NULL);

  RefreshGList(stringglist,GuiWin,NULL,-1);

  GT_RefreshWindow(GuiWin,NULL);
  DrawBevelBoxA(rp, 5,5,GuiWin->Width-15-GuiWin->BorderLeft,GuiWin->Height-65-GuiWin->BorderTop,bevel_tag);
  DrawBevelBoxA(rp, 15,12,GuiWin->Width-35-GuiWin->BorderLeft,GuiWin->Height-160-GuiWin->BorderTop,bevel_tag);
  if( get_var_int( "@user-level" ) > _NOVICE )
  do
  {
    for( i = 0 ; i < GetPL( pl, _PROMPT ).intval ; i ++ )
    {
      printf( "%s\n", GetPL( pl, _PROMPT ).arg[i] );
    }
    printf( "String is %s.\n", string );
    printf( " V - Enter new string\n P - Proceed\n A - Abort\n H - Help\n" );
    scanf( "%c", &c );
    switch( tolower( c ) )
    {
      case 'v'	: /* enter new string */
                  scanf( "%s", buffer );
                  free( string );
                  string = strdup( buffer );
                  break;
      case 'p'	: /* proceed */
                  finish = TRUE;
                  break;
      case 'a'	: /* abort */
                  abort_install();
                  break;
      case 'h'	: /* help */
                  for( i = 0 ; i < GetPL( pl, _HELP ).intval ; i ++ )
                  {
                    printf( "%s\n", GetPL( pl, _HELP ).arg[i] );
                  }
                  if( i == 0 )
                  {
                    printf( "%s\n", get_var_arg( "@asknumber-help" ) );
                  }
                  break;
      default	: break;
    }
  } while( !finish );

  RemoveGList(GuiWin,stringglist,-1);
  GT_RefreshWindow(GuiWin,NULL);

#endif /* DEBUG */

  retval = addquotes( string );
  free( string );
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
long int request_choice( struct ParameterList *pl )
{
int i;
long int retval;
#ifdef DEBUG
int max, finish = FALSE;
char c;

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

  clear_gui();

  mxlabels = malloc( (max+1)*sizeof(STRPTR) );
  for( i = 0 ; i < max ; i ++ )
  {
    mxlabels[i] = strdup(GetPL( pl, _CHOICES ).arg[i] );
  }
  mxlabels[i] = NULL;

  gad = CreateContext( &mxglist );
  if(gad==NULL)
    printf("CreateContext() failed\n");

  gad = CreateGadget( MX_KIND, gad, &gt_mxgad,
			GTMX_Labels, mxlabels,
			GTMX_Scaled, TRUE,
			GTMX_TitlePlace, PLACETEXT_ABOVE,
			TAG_DONE );

  AddGList(GuiWin,mxglist,-1,-1,NULL);
  RefreshGList(mxglist,GuiWin,NULL,-1);
  GT_RefreshWindow(GuiWin,NULL);
  DrawBevelBoxA(rp, 5,5,GuiWin->Width-15-GuiWin->BorderLeft,GuiWin->Height-65-GuiWin->BorderTop,bevel_tag);
  DrawBevelBoxA(rp, 15,12,GuiWin->Width-35-GuiWin->BorderLeft,GuiWin->Height-160-GuiWin->BorderTop,bevel_tag);
  if( get_var_int( "@user-level" ) > _NOVICE )
  do
  {
    for( i = 0 ; i < GetPL( pl, _PROMPT ).intval ; i ++ )
    {
      printf( "%s\n", GetPL( pl, _PROMPT ).arg[i] );
    }
    for( i = 0 ; i < max ; i ++)
    {
      printf( "%2d: (%c) %s\n", ( i + 1 ), ( retval==(i+1) ? '*' : ' ' ), GetPL( pl, _CHOICES ).arg[i]);
    }
    printf( " V - Change Value\n P - Proceed\n A - Abort\n H - Help\n" );
    scanf( "%c", &c );
    switch( tolower( c ) )
    {
      case 'a'	: /* abort */
                  abort_install();
                  break;
      case 'p'	: /* proceed */
                  if( retval < max )
                  {
                    finish = TRUE;
                  }
                  break;
      case 'h'	: /* help */
                  for( i = 0 ; i < GetPL( pl, _HELP ).intval ; i ++ )
                  {
                    printf( "%s\n", GetPL( pl, _HELP ).arg[i] );
                  }
                  if( i == 0 )
                  {
                    printf( "%s\n", get_var_arg( "@asknumber-help" ) );
                  }
                  break;
      case 'v'	: /* change value */
                  scanf( "%s", buffer );
                  i = atol( buffer );
                  retval = ( i < max && i > 0 ) ? i : retval ;
                  break;
      default	: break;
    }
  } while( !finish );

#endif /* DEBUG */
  RemoveGList(GuiWin,mxglist,-1);
  GT_RefreshWindow(GuiWin,NULL);
  FreeGadgets( mxglist );

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
 * Ask user for a specific disk
 */
#warning TODO: write whole function
/* Ask user to insert a specific disk */
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
char *request_file( struct ParameterList *pl )
{
char *retval, *string;
struct TagItem frtags[] =
{
    { ASL_Hail,	      (ULONG)"Choose a file:" },
    { ASL_Height,     0 },
    { ASL_Width,      0 },
    { ASL_LeftEdge,   320 },
    { ASL_TopEdge,    40 },
    { ASL_OKText,     (ULONG)"Okay" },
    { ASL_CancelText, (ULONG)"Cancel" },
    { ASL_File,	      (ULONG)"asl.library" },
    { ASL_Dir,	      (ULONG)"libs:" },
    { TAG_DONE }
};

struct FileRequester *fr;


  if( GetPL( pl, _DEFAULT ).used == 0 )
  {
    error = SCRIPTERROR;
    traperr( "No default specified!", NULL );
  }

#if 0
  if ((fr = (struct FileRequester *) AllocAslRequest(ASL_FileRequest, frtags)))
  {
    if (AslRequest(fr, NULL))
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
long int request_options( struct ParameterList *pl )
{
long int retval;

  retval = GetPL( pl, _DEFAULT ).intval;

return retval;
}


/*
 * Ask user to confirm
 */
int request_confirm( struct ParameterList * pl, long int minuser )
{
int n, m, finish = FALSE;
int retval = 1;
char c;

  if( ( get_var_int( "@user-level" ) >= minuser ) || ( get_var_int( "@user-level" ) >= GetPL( pl, _CONFIRM).intval ) )
  {
    clear_gui();

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

    do
    {
      for( n = 0 ; n < m ; n++ )
      {
#ifdef DEBUG
        printf( "%s\n", GetPL( pl, _PROMPT ).arg[n] );
#endif /* DEBUG */
        itext.IText = GetPL( pl, _PROMPT ).arg[n];
        PrintIText( rp, &itext, 10, 15*(n+2) );
      }
#ifdef DEBUG 
      printf( " P - Proceed\n S - Skip this\n A - Abort Installation\n H - Help\n" );
#endif /* DEBUG */
      scanf( "%c", &c );
      switch( tolower( c ) )
      {
        case 'p': /* proceed */
                  finish = TRUE;
                  break;
        case 's': /* skip this */
                  finish = TRUE;
                  retval = 0;
                  break;
        case 'h': /* help */
                  m = GetPL( pl, _HELP ).intval;
                  for( n = 0 ; n < m ; n++ )
                  {
                    printf( "%s\n", GetPL( pl, _HELP ).arg[n] );
                  }
                  break;
        case 'a': /* abort */
                  abort_install();
                  break;
        default	: break;
      }
    } while( finish == FALSE );
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

void abort_install( )
{
  if( AutoRequest(GuiWin,&abortq_body,&p,&n,0L,0L,200,75) != 0 )
  {
    error = USERABORT;
    grace_exit = TRUE;
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
 * Execute "(traperr)" from preferences
 */
void traperr( char * msg, char * name )
{
char *outmsg;
int i, j;

#ifdef DEBUG
  i = ( msg != NULL ) ? strlen( msg ) : 0 ;
  j = ( name != NULL ) ? strlen( name ) : 0 ;
  outmsg = malloc( i + j + 1 );
  sprintf( outmsg, msg, name );
  printf( "ERROR:\n %s\n", outmsg );
#endif /* DEBUG */

  if( preferences.trap[ error - 1 ].cmd != NULL )
  {
    /* execute trap */
    execute_script( preferences.trap[ error - 1 ].cmd, -99 );
  }
  else
  {
    /* execute onerrors */
    if( preferences.onerror.cmd != NULL )
    {
      execute_script( preferences.onerror.cmd, -99 );
    }
  }

#ifdef DEBUG
  dump_varlist();
#endif /* DEBUG */

  cleanup();
  if( grace_exit == TRUE )
  {
    exit(0);
  }
  else
  {
    exit(-1);
  }
}


