/* gui.c -- here are all functions for the gui */

#include "Installer.h"
#include "execute.h"

/* External variables */
extern FILE *inputfile;
extern char buffer[MAXARGSIZE];
extern char *filename;
extern InstallerPrefs preferences;
extern int error, grace_exit;

/* External function prototypes */
extern void cleanup();
extern char *get_var_arg( char * );
extern char *get_var_int( char * );
extern void execute_script( ScriptArg *, int );
extern void set_variable( char *, char *, int );
#ifdef DEBUG
extern void dump_varlist();
#endif /* DEBUG */
extern void end_malloc();
extern void outofmem( void * );

/* Internal function prototypes */
#ifndef LINUX
void init_gui();
void deinit_gui();
void clear_gui();
#endif /* !LINUX */
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
void abort_install();
void final_report();
void traperr( char *, char * );
int strtostrs ( char *, char *** );

#ifndef LINUX

#include <proto/intuition.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <proto/graphics.h>
#include <graphics/gfxbase.h>

struct IntuitionBase *IntuitionBase = NULL;
struct GfxBase * GfxBase = NULL;
struct Window *GuiWin;
struct RastPort *rp;
const char GuiWinTitle[] ="AROS - Installer V43.3";

void init_gui( )
{
struct TagItem tags[] =
{
  { WA_Width	, 300 },
  { WA_Height	, 150 },
  { WA_Title	, (ULONG)GuiWinTitle },
  { WA_IDCMP	, IDCMP_MOUSEMOVE },

  { 0,0}
};

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

  if( NULL == ( GuiWin = OpenWindowTagList( NULL, tags ) ) )
  {
    cleanup();
    CloseLibrary( (struct Library *)IntuitionBase );
    exit(-1);
  }
  rp = GuiWin->RPort;
}

void deinit_gui( )
{
  CloseWindow( GuiWin );
  CloseLibrary( (struct Library *)IntuitionBase );
  CloseLibrary( (struct Library *)GfxBase );
}

void clear_gui()
{
  EraseRect( rp, 0, 0, GuiWin->Width - 5, GuiWin->Height - 11 );
}

#endif /* !LINUX */

void show_abort( char *msg )
{
#ifndef LINUX
struct IntuiText itext;
char **out, *text;
int n, m;
#endif /* !LINUX */
char c;

  if( get_var_int( "@user-level" ) > _NOVICE )
  {
#ifndef LINUX
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
#else /* !LINUX */
  printf( "Aborting Installation:\n%s\n", msg );
#endif /* !LINUX */
    printf( " Press Return to Proceed\n" );
    c = getchar();
    while( getchar() != LINEFEED );
  }
}

void show_complete( long int percent )
{
#ifdef LINUX
  printf( "(Done %ld%c)\n", percent, PERCENT );
#else /* LINUX */
char *text;

  text = malloc( strlen( GuiWinTitle ) + 13);
  if( text == NULL )
  {
    end_malloc();
  }
  sprintf( text, "%s (Done %3ld%c)", GuiWinTitle, percent, PERCENT );
  SetWindowTitles( GuiWin, text, NULL);
#endif /* LINUX */
}

void show_exit( char *msg )
{
#ifndef LINUX
struct IntuiText itext;
char **out, *text;
int n, m;
#endif /* !LINUX */
char c;

#ifndef LINUX
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
#else /* !LINUX */
  printf( "%s\n", msg );
#endif /* !LINUX */
#ifdef DEBUG
  printf( "\nDone with installation.\n\n" );
#endif /* DEBUG */
  printf( " Press Return to Proceed\n" );
  c = getchar();
  while( getchar() != LINEFEED );
}

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
  inputfile = fopen( filename, "r" );
  if( inputfile == NULL )
  {
    PrintFault( IoErr(), "Installer" );
    cleanup();
    exit(-1);
  }
  errline--;
  while( count != 0 && errline > 0 )
  {
    count = fread( buffer, 1, 1, inputfile );
    if( buffer[0] == LINEFEED )
    {
      errline--;
    }
  }
  do
  {
    i++;
    count = fread( &buffer[i], 1, 1, inputfile );
  } while( buffer[i] != LINEFEED && count != 0 && i < MAXARGSIZE );
  buffer[i] = 0;
#ifdef DEBUG
  printf( "%s\n", buffer );
#endif /* DEBUG */

}

void show_working( char *msg )
{
#ifndef LINUX
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
#else /* !LINUX */
  printf( "Working on Installation:\n%s\n", msg );
#endif /* !LINUX */
}

void show_message( char * msg ,struct ParameterList * pl )
{
#ifndef LINUX
struct IntuiText itext;
char **out;
#endif /* !LINUX */
int n, m;
char c;
int finish = FALSE;

  if( GetPL( pl, _ALL ).used == 1 || get_var_int( "@user-level" ) > _NOVICE )
  {
#ifndef LINUX
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
#else /* !LINUX */
    printf( "Message:\n%s\n", msg );
#endif /* !LINUX */
    do
    {
      printf( " P - Proceed\n A - Abort\n H - Help\n" );
      c = getchar();
      while( getchar() != LINEFEED );
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

void show_help_userlevel( )
{
#warning TODO: help for userlevel-requester
#ifdef DEBUG
  printf( "\n NOVICE won't be asked anymore questions\n AVERAGE will have to interact\n EXPERT must confirm all actions\n" );
#endif /* DEBUG */
}

void show_help_logfile()
{
#warning TODO: help for logfile-requester
#ifdef DEBUG
  printf( "\n Printer will go to PRT:\n Log File will be %s\n",preferences.transcriptfile );
#endif /* DEBUG */
}

void show_help_installer( )
{
#warning TODO: help/about for Installer
#ifdef DEBUG
  printf( "\nThis is AROS Installer V%d.%d\niIt is intended to be compatible to Installer V43.3\n\nThis program was written by Henning Kiel <hkiel@aros.org>\n\n", INSTALLER_VERSION, INSTALLER_REVISION );
#endif /* DEBUG */
}

void request_userlevel( char *msg )
{
int usrlevel, finish = FALSE;

#ifdef DEBUG
int c;

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
    c = getchar();
    while( getchar() != LINEFEED );
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
      c = getchar();
      while( getchar() != LINEFEED );
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

long int request_bool( struct ParameterList *pl)
{
int i;
long int retval;
char yes[] = "Yes", no[] = "No", *yesstring, *nostring;
int c, finish = FALSE;

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
      fprintf( preferences.transcriptstream, ">%s\n", GetPL( pl, _PROMPT ).arg[i] );
    }
  }
#ifdef DEBUG
  if( get_var_int( "@user-level" ) > _NOVICE )
  do
  {
    for( i = 0 ; i < GetPL( pl, _PROMPT ).intval ; i ++ )
    {
      printf( "%s\n", GetPL( pl, _PROMPT ).arg[i] );
    }
    printf( "Default is %ld.\n", retval );
    printf( " 1 - %s\n 0 - %s\n A - Abort\n H - Help\n", yesstring, nostring );
    c = getchar();
    while( getchar() != LINEFEED );
    switch( tolower( c ) )
    {
      case '1'	: /* return TRUE */
                  retval = 1;
                  finish = TRUE;
                  break;
      case '0'	: /* return FALSE */
                  retval = 0;
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
#warning FIXME: What default help text is used?
                    printf( "%s\n", get_var_arg( "@asknumber-help" ) );
                  }
                  break;
      default	: break;
    }
  } while( !finish );

#endif /* DEBUG */
  if( preferences.transcriptstream != NULL )
  {
    fprintf( preferences.transcriptstream, "Ask Question: Result was \"%s\".\n\n", ( retval ? yesstring : nostring ) );
  }

return retval;
}

long int request_number( struct ParameterList *pl)
{
int i;
long int retval, min, max;
#ifdef DEBUG
char buffer[MAXARGSIZE];
int c, finish = FALSE;

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
      fprintf( preferences.transcriptstream, ">%s\n", GetPL( pl, _PROMPT ).arg[i] );
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
    c = getchar();
    while( getchar() != LINEFEED );
    switch( tolower( c ) )
    {
      case 'v'	: /* change value */
                  gets( buffer );
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
    fprintf( preferences.transcriptstream, "Ask Number: Result was \"%ld\".\n\n", retval );
  }

return retval;
}

char *request_string( struct ParameterList *pl)
{
int i;
char *retval,*string;
#ifdef DEBUG
char buffer[MAXARGSIZE];
int c, finish = FALSE;

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
      fprintf( preferences.transcriptstream, ">%s\n", GetPL( pl, _PROMPT ).arg[i] );
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
    c = getchar();
    while( getchar() != LINEFEED );
    switch( tolower( c ) )
    {
      case 'v'	: /* enter new string */
                  gets( buffer );
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
  c = strlen( string );
  retval = malloc( c + 3 );
  outofmem( retval );
  retval[0] = DQUOTE;
  strcpy( retval + 1, string );
  retval[c+1] = DQUOTE;
  retval[c+2] = 0;
  free( string );
  if( preferences.transcriptstream != NULL )
  {
    fprintf( preferences.transcriptstream, "Ask String: Result was %s.\n\n", retval );
  }

return retval;
}

#warning TODO: check whole function (supports only 1-9 in text-gui)
/* Ask user to choose one of N items */
long int request_choice( struct ParameterList *pl )
{
int i;
long int retval;
#ifdef DEBUG
int c, n = -1, finish = FALSE;

  retval = GetPL( pl, _DEFAULT ).intval;
  if( retval == 0 )
  {
    retval = 1;
  }
  for( i = 0 ; i < GetPL( pl, _PROMPT ).intval ; i ++ )
  {
    if( preferences.transcriptstream != NULL )
    {
      fprintf( preferences.transcriptstream, ">%s\n", GetPL( pl, _PROMPT ).arg[i] );
    }
  }
  for( i = 0; i < 32 ; i ++ )
  {
    if( retval & ( 1 << i ) )
    {
      n = i + 1;
    }
  }
  if( get_var_int( "@user-level" ) > _NOVICE )
  do
  {
    for( i = 0 ; i < GetPL( pl, _PROMPT ).intval ; i ++ )
    {
      printf( "%s\n", GetPL( pl, _PROMPT ).arg[i] );
    }
    if( GetPL( pl, _CHOICES ).intval > 32 )
    {
      error = SCRIPTERROR;
      traperr( "More than 32 choices given!\n", NULL );
    }
    if( GetPL( pl, _CHOICES ).intval > 1 )
    {
      for( i = 0 ; i < GetPL( pl, _CHOICES ).intval ; i ++)
      {
        printf( "%2d: (%c) %s\n", ( i + 1 ), ( retval&(1<<i) ? '*' : ' ' ), GetPL( pl, _CHOICES ).arg[i]);
      }
    }
    else
    {
      error = SCRIPTERROR;
      traperr( "No choices given!\n", NULL );
    }
    printf( " P - Proceed\n A - Abort\n H - Help\n" );
    c = getchar();
    while( getchar() != LINEFEED );
    if( isdigit(c) && c != '0' )
    {
      retval = 1 << ( c - '1' );
      n = ( c - '0' );
    }
    else
    {
      switch( tolower( c ) )
      {
        case 'a': /* abort */
                  abort_install();
                  break;
        case 'p': /* proceed */
                  finish = TRUE;
                  break;
        case 'h': /* help */
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
    }
  } while( !finish );

#endif /* DEBUG */
  if( preferences.transcriptstream != NULL )
  {
    fprintf( preferences.transcriptstream, "Ask Choice: User selected \"%s\".\n\n", GetPL( pl, _CHOICES ).arg[n] );
  }

return retval;
}

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

#warning TODO: write whole function
long int request_options( struct ParameterList *pl )
{
long int retval;

  retval = GetPL( pl, _DEFAULT ).intval;

return retval;
}

void abort_install( )
{
#ifdef DEBUG
int c, abort = -1;

  do
  {
    printf( "Do you really want to abort Installer? [y/n]" );
    c = getchar();
    while( getchar() != LINEFEED );
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

void final_report( )
{
#ifdef DEBUG
  printf( "Application has bee installed in %s\n", get_var_arg( "@default-dest" ) );
#endif /* DEBUG */
}

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

