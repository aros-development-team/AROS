/* gui.c -- here are all functions for the gui */

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

/* Internal function prototypes */
struct Border * genborder( int, int );
void remborder( struct Border * );
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
int strtostrs ( char *, char *** );


#include <proto/intuition.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <proto/graphics.h>
#include <graphics/gfxbase.h>
#include <proto/gadtools.h>
#include <libraries/gadtools.h>

struct IntuitionBase *IntuitionBase = NULL;
struct Library *GadToolsBase = NULL;
struct GfxBase * GfxBase = NULL;
struct Window *GuiWin;
struct RastPort *rp;
struct IntuiMessage *msg;
ULONG class;
UWORD code;

const char GuiWinTitle[] ="AROS - Installer V43.3";

APTR vi;
struct Screen *scr;
struct Gadget *glist = NULL, *first = NULL, *gad = NULL;


#define INTUIGUI 1

#define ID_BOOLGAD 1
struct NewGadget gt_boolgad = {
  10,10, 30,30, /* ng_LeftEdge ng_TopEdge ng_Width ng_Height */
  NULL, /* ng_GadgetText */
  NULL, /* ng_TextAttr */
  ID_BOOLGAD, /* ng_GadgetID */
  PLACETEXT_IN, /* ng_Flags */
  NULL, /* ng_VisualInfo */
  NULL  /* ng_UserData */
};

#define ID_TEXTGAD 4
struct NewGadget gt_textgad = {
  10,50, 100,70, /* ng_LeftEdge ng_TopEdge ng_Width ng_Height */
  NULL, /* ng_GadgetText */
  NULL, /* ng_TextAttr */
  0, /* ng_GadgetID */
  0, /* ng_Flags */
  NULL, /* ng_VisualInfo */
  NULL  /* ng_UserData */
};


/*
 * Initialize the GUI
 */
void init_gui( )
{
struct TagItem tags[] =
{
  { WA_Width	, 400 },
  { WA_Height	, 200 },
  { WA_Title	, (ULONG)GuiWinTitle },
  { WA_IDCMP	, IDCMP_MOUSEMOVE
		  | IDCMP_GADGETUP
		  | IDCMP_GADGETDOWN },
  { WA_Flags	,   WFLG_ACTIVATE
		  | WFLG_DEPTHGADGET
		  | WFLG_DRAGBAR
		  | WFLG_GIMMEZEROZERO },

  { 0,0 }
};
struct Gadget *gad = NULL;

  IntuitionBase = (struct IntuitionBase *)OpenLibrary( "intuition.library", 37 );
  if (IntuitionBase == NULL)
  {
    cleanup();
    exit(-1);
  }

  GfxBase = (struct GfxBase *)OpenLibrary( "graphics.library", 37 );
  if (GfxBase == NULL)
  {
    cleanup();
    exit(-1);
  }

  GadToolsBase = OpenLibrary( "gadtools.library", 0 );
  if (GadToolsBase == NULL)
  {
    cleanup();
    exit(-1);
  }

  if( NULL == ( GuiWin = OpenWindowTagList( NULL, tags ) ) )
  {
    cleanup();
    CloseLibrary( (struct Library *)IntuitionBase );
    exit(-1);
  }
  rp = GuiWin->RPort;

  scr = LockPubScreen( NULL );
  vi = GetVisualInfoA( scr, NULL );
  gad = CreateContext( &glist );
  if(gad==NULL)
    printf("CreateContext() failed\n");
  gt_boolgad.ng_VisualInfo = vi;
  gt_textgad.ng_VisualInfo = vi;
}


/*
 * Close GUI
 */
void deinit_gui( )
{
  FreeGadgets( glist );
  FreeVisualInfo( vi );
  UnlockPubScreen( NULL, scr );
  CloseWindow( GuiWin );
  CloseLibrary( (struct Library *)IntuitionBase );
  CloseLibrary( (struct Library *)GadToolsBase );
  CloseLibrary( (struct Library *)GfxBase );
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
struct IntuiText itext;
char **out, *text;
int n, m;

  if( get_var_int( "@user-level" ) > _NOVICE )
  {
    clear_gui();
    itext.NextText = NULL;
    itext.FrontPen = 1;
    itext.BackPen = 0;
    itext.DrawMode = JAM1;
    itext.LeftEdge = 10;
    itext.TopEdge = 10;
    itext.ITextFont = NULL;

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
struct IntuiText itext;
char **out, *text;
int n, m;

  clear_gui();
  itext.NextText = NULL;
  itext.FrontPen = 1;
  itext.BackPen = 0;
  itext.DrawMode = JAM1;
  itext.LeftEdge = 10;
  itext.TopEdge = 10;
  itext.ITextFont = NULL;

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
struct IntuiText itext;
char *text, **out;
int n, m;

  clear_gui();
  itext.NextText = NULL;
  itext.FrontPen = 1;
  itext.BackPen = 0;
  itext.DrawMode = JAM1;
  itext.LeftEdge = 10;
  itext.TopEdge = 10;
  itext.ITextFont = NULL;

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
struct IntuiText itext;
char **out;
int n, m;
char c;
int finish = FALSE;

  if( GetPL( pl, _ALL ).used == 1 || get_var_int( "@user-level" ) > _NOVICE )
  {
    clear_gui();
    itext.NextText = NULL;
    itext.FrontPen = 1;
    itext.BackPen = 0;
    itext.DrawMode = JAM1;
    itext.LeftEdge = 10;
    itext.TopEdge = 10;
    itext.ITextFont = NULL;

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
    printf( "Which user-level do you want?\n 0 - Novice\n 1 - Average\n 2 - Expert\n P - Proceed\n A - Abort\n H - Help\n O - About Installer\n\nDefault is %d\n", usrlevel );
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
  } while( finish == FALSE );
#endif /* DEBUG */

  set_variable( "@user-level", NULL, usrlevel );

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
                  finish = TRUE;
                  break;
        case 'p': /* printer-log */
#ifdef 0
/* fopen() log file */
                  free( preferences.transcriptfile );
                  preferences.transcriptfile = strdup ("PRT:");
#endif /* 0 */
                  finish = TRUE;
                  break;
        case 'n': /* no log */
#ifdef 0
/* fopen() printer log-"file" */
                  free( preferences.transcriptfile );
                  preferences.transcriptfile = NULL;
#endif /* 0 */
                  finish = TRUE;
                  break;
        default	: break;
      }
    } while( finish == FALSE );
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
char c;

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
    struct IntuiText itext;
    char **out;
    int j, n, m;

    clear_gui();
    gad = CreateGadget( BUTTON_KIND, gad, &gt_boolgad,
		    	GA_Immediate, TRUE,
		    	TAG_DONE );
    first = gad;

    gad = CreateGadget( TEXT_KIND, gad, &gt_textgad,
		    	GTTX_Text, yesstring,
		    	GTTX_CopyText, TRUE,
		    	GTTX_Border, TRUE,
		    	GTTX_Justification, GTJ_CENTER,
		    	TAG_DONE );

    if(glist==NULL)
        printf("glist==NULL\n");
    else
    {
printf("Adding gads...\n");
        AddGList(GuiWin,glist,-1,-1,NULL);
printf("Refreshing glist...\n");
        RefreshGList(glist,GuiWin,NULL,-1);
printf("Refreshing win...\n");
        GT_RefreshWindow(GuiWin,NULL);
printf("Drawing Bevel...\n");
        DrawBevelBox(rp, 8,8,160,75,NULL);
printf("Finished...\n");
    }
        
#if 1
    itext.NextText = NULL;
    itext.FrontPen = 1;
    itext.BackPen = 0;
    itext.DrawMode = JAM1;
    itext.LeftEdge = 10;
    itext.TopEdge = 10;
    itext.ITextFont = NULL;

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
        PrintIText( rp, &itext, 10, 15*(j+5) );
        free( out[n] );
        j++;
      }
      free( out );
    }
    do
    {
      Wait( 1L<<GuiWin->UserPort->mp_SigBit );
      msg = (struct IntuiMessage *)GetMsg( GuiWin->UserPort );
      class = msg->Class;
      code = msg->Code;
      switch( class )
      {
        case IDCMP_GADGETUP:
              switch( ( (struct Gadget *)(msg->IAddress) )->GadgetID )
              {
                case 1:
                  retval = 0;
                  finish = TRUE;
                  break;
                case 4:
                  retval = 1;
                  finish = TRUE;
                  break;
                case 0:
                  abort_install();
                  break;
                case 2:
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
      ReplyMsg((struct Message *)msg);
    } while( !finish );
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
#endif /* DEBUG */

  /* Add surrounding quotes */
  i = strlen( string );
  retval = malloc( i + 3 );
  outofmem( retval );
  retval[0] = DQUOTE;
  strcpy( retval + 1, string );
  retval[i+1] = DQUOTE;
  retval[i+2] = 0;
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
  if( get_var_int( "@user-level" ) > _NOVICE )
  do
  {
    for( i = 0 ; i < GetPL( pl, _PROMPT ).intval ; i ++ )
    {
      printf( "%s\n", GetPL( pl, _PROMPT ).arg[i] );
    }
    if( GetPL( pl, _CHOICES ).intval > 1 )
    {
      for( i = 0 ; i < max ; i ++)
      {
        printf( "%2d: (%c) %s\n", ( i + 1 ), ( retval&(1<<i) ? '*' : ' ' ), GetPL( pl, _CHOICES ).arg[i]);
      }
    }
    else
    {
      error = SCRIPTERROR;
      traperr( "No choices given!\n", NULL );
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
                  retval = ( atol( buffer ) < max ) ? atol( buffer ) : retval ;
                  break;
      default	: break;
    }
  } while( !finish );

#endif /* DEBUG */
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
int c;

  if( GetPL( pl, _DEFAULT ).used == 0 )
  {
    error = SCRIPTERROR;
    traperr( "No default specified!", NULL );
  }

  string = GetPL( pl, _DEFAULT ).arg[0];
  /* Add surrounding quotes */
  c = ( string == NULL ) ? 0 : strlen( string );
  retval = malloc( c + 3 );
  outofmem( retval );
  retval[0] = DQUOTE;
  strcpy( retval + 1, string );
  retval[c+1] = DQUOTE;
  retval[c+2] = 0;

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
int c;

  if( GetPL( pl, _DEST ).used == 0 )
  {
    error = SCRIPTERROR;
    traperr( "No dest specified!", NULL );
  }

  string = GetPL( pl, _DEST ).arg[0];
  /* Add surrounding quotes */
  c = ( string == NULL ) ? 0 : strlen( string );
  retval = malloc( c + 3 );
  outofmem( retval );
  retval[0] = DQUOTE;
  strcpy( retval + 1, string );
  retval[c+1] = DQUOTE;
  retval[c+2] = 0;

return retval;
}


/*
 * Ask user for a file
 */
#warning TODO: write whole function
char *request_file( struct ParameterList *pl )
{
char *retval, *string;
int c;

  if( GetPL( pl, _DEFAULT ).used == 0 )
  {
    error = SCRIPTERROR;
    traperr( "No default specified!", NULL );
  }

  string = GetPL( pl, _DEFAULT ).arg[0];
  /* Add surrounding quotes */
  c = ( string == NULL ) ? 0 : strlen( string );
  retval = malloc( c + 3 );
  outofmem( retval );
  retval[0] = DQUOTE;
  strcpy( retval + 1, string );
  retval[c+1] = DQUOTE;
  retval[c+2] = 0;

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
struct IntuiText itext;
int n, m, finish = FALSE;
int retval = 1;
char c;

  if( get_var_int( "@user-level" ) >= minuser )
  {
    clear_gui();
    itext.NextText = NULL;
    itext.FrontPen = 1;
    itext.BackPen = 0;
    itext.DrawMode = JAM1;
    itext.LeftEdge = 10;
    itext.TopEdge = 10;
    itext.ITextFont = NULL;

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
void abort_install( )
{
#ifdef DEBUG
int abort = -1;
char c;

  do
  {
    printf( "Do you really want to abort Installer? [y/n]" );
    scanf( "%c", &c );
    switch( tolower( c ) )
    {
      case 'y'	: error = USERABORT;
                  grace_exit = TRUE;
                  /* Execute trap(1) */
                  traperr( "User aborted!\n", NULL );
                  break;
      case 'n'	: abort = 0;
                  break;
      default	: break;
    }
  } while( abort == -1 );
#endif /* DEBUG */
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


/*
 * Break string into array of strings at LINEFEEDs
 */
int strtostrs ( char * in, char ***outarr )
{
int i = 0, j = 0;
char **out = *outarr;

  if( *in )
  {
    i++;
  }
  while( *in )
  {
    out[i-1] = realloc( out[i-1], ( j + 1 ) * sizeof( char ) );
    outofmem( out[i-1] );
    if( *in == LINEFEED )
    {
      /* NULL-terminate string and malloc space for next string */
      out[i-1][j] = 0;
      out = realloc( out, ( i + 1 ) * sizeof( char *) );
      out[i] = NULL;
      j = 0;
      i++;
    }
    else
    {
      /* save char to string */
      out[i-1][j] = *in;
      j++;
    }
    in++;
  }
  /* NULL-terminate last string */
  out[i-1] = realloc( out[i-1], ( j + 1 ) * sizeof( char ) );
  outofmem( out[i-1] );
  out[i-1][j] = 0;
  *outarr = out;

return i;
}

