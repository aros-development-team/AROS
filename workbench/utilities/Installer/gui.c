/* gui.c -- here are all functions for the gui */

#include "Installer.h"
#include "execute.h"

/* External variables */
extern FILE *inputfile;
extern char buffer[MAXARGSIZE];
extern char *filename;
extern InstallerPrefs preferences;
extern int error;

/* External function prototypes */
extern char *get_var_arg( char * );
extern char *get_var_int( char * );
extern void end_malloc();
extern void execute_script( ScriptArg *, int );
extern void set_variable( char *, char *, int );

/* Internal function prototypes */
void show_abort( char * );
void show_complete( int );
void show_exit( char * );
void show_parseerror( int );
void show_working( char * );
void show_help_userlevel();
void show_help_installer();
void request_userlevel();
int  request_bool( struct ParameterList *);
int  request_number( struct ParameterList *);
char *request_string( struct ParameterList *);
int request_choice( struct ParameterList * );
char *request_dir( struct ParameterList * );
char *request_disk( struct ParameterList * );
char *request_file( struct ParameterList * );
int request_options( struct ParameterList * );
void abort_install();
void final_report();
void traperr( char * );
void parseerror( char *, int );

void show_abort( char *msg )
{
#ifdef DEBUG
  printf( "Aborting Installation:\n%s\n", msg );
#endif /* DEBUG */
}

void show_complete( int percent )
{
#ifdef DEBUG
  printf( "Done %d%c!\n", percent, PERCENT );
#endif /* DEBUG */
}

void show_exit( char *msg )
{
#ifdef DEBUG
  if( msg != NULL )
  {
    printf( "%s\n", msg );
  }
  printf( "\nDone with installation.\n\n" );
#endif /* DEBUG */
}

void show_parseerror( int errline )
{
int count = 1, i = -1;

#ifdef DEBUG
  printf( "Syntax error in line %d:\n", errline );
#endif /* DEBUG */
  inputfile = fopen( filename, "r" );
  if( inputfile == NULL )
  {
    PrintFault( IoErr(), "Installer" );
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
#ifdef DEBUG
  printf( "Working on Installation:\n%s\n", msg );
#endif /* DEBUG */
}

void show_help_userlevel()
{
#warning TODO: help for userlevel-requester
#ifdef DEBUG
  printf( "\n NOVICE won't be asked anymore questions\n AVERAGE will have to interact\n EXPERT must confirm all actions\n" );
#endif /* DEBUG */
}

void show_help_installer()
{
#warning TODO: help/about for Installer
#ifdef DEBUG
  printf( "\nThis is AROS-Installer V43.3\nit is intended to be comaptible to Installer V43.3\n\nThis program was written by Henning Kiel <hkiel@aros.org>\n" );
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
}

int request_bool( struct ParameterList *pl)
{
int i, retval;
char yes[] = "Yes", no[] = "No", *yesstring, *nostring;
#ifdef DEBUG
int c, finish = FALSE;

  retval = ( GetPL( pl, _DEFAULT ).intval != 0 );
  if( get_var_int( "@user-level" ) > _NOVICE )
  do
  {
    for( i = 0 ; i < GetPL( pl, _PROMPT ).intval ; i ++ )
    {
      printf( "%s\n", GetPL( pl, _PROMPT ).arg[i] );
    }
    printf( "Default is %d.\n", retval );
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
                    printf( "%s\n", get_var_arg( "@asknumber-help" ) );
                  }
                  break;
      default	: break;
    }
  } while( !finish );

#endif /* DEBUG */

return retval;
}

int request_number( struct ParameterList *pl)
{
int i, retval, min, max;
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
#define INTMIN -32768
#define INTMAX  32767
    max = INTMAX;
    min =( retval < 0 ) ?
         INTMIN :
         0;
  }
  if( get_var_int( "@user-level" ) > _NOVICE )
  do
  {
    for( i = 0 ; i < GetPL( pl, _PROMPT ).intval ; i ++ )
    {
      printf( "%s\n", GetPL( pl, _PROMPT ).arg[i] );
    }
    retval = GetPL( pl, _DEFAULT ).intval;
    printf( "Number [%d,%d] is %d.\n", min, max, retval );
    printf( " V - Change value\n P - Proceed\n A - Abort\n H - Help\n" );
    c = getchar();
    while( getchar() != LINEFEED );
    switch( tolower( c ) )
    {
      case 'v'	: /* change value */
                  gets( buffer );
                  retval = atoi( buffer );
                  if( retval < min || retval > max )
                  {
                    printf( "Input out of range!\n" );
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
  if( get_var_int( "@user-level" ) > _NOVICE )
  do
  {
    for( i = 0 ; i < GetPL( pl, _PROMPT ).intval ; i ++ )
    {
      printf( "%s\n", GetPL( pl, _PROMPT ).arg[i] );
    }
    free( string );
    if( GetPL( pl, _DEFAULT ).used == 1 )
    {
      string = strdup( GetPL( pl, _DEFAULT ).arg[0] );
    }
    else
    {
      string = strdup( "" );
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
  if( retval == NULL )
  {
    end_malloc();
  }
  retval[0] = DQUOTE;
  strcpy( retval + 1, string );
  retval[c+1] = DQUOTE;
  retval[c+2] = 0;
  free( string );

return retval;
}

#warning TODO: check whole function (supports only 1-9 in text-gui)
/* Ask user to choose one of N items */
int request_choice( struct ParameterList *pl )
{
int i, retval;
#ifdef DEBUG
int c, finish = FALSE;

  retval = GetPL( pl, _DEFAULT ).intval;
  if( get_var_int( "@user-level" ) > _NOVICE )
  do
  {
    for( i = 0 ; i < GetPL( pl, _PROMPT ).intval ; i ++ )
    {
      printf( "%s\n", GetPL( pl, _PROMPT ).arg[i] );
    }
    if( GetPL( pl, _CHOICES ).used == 1 )
    {
      for( i = 0 ; i < GetPL( pl, _CHOICES ).intval ; i ++)
      {
        printf( "%2d: (%c) %s\n", ( i + 1 ), ( retval&(1<<i) ? '*' : ' ' ), GetPL( pl, _CHOICES ).arg[i]);
      }
    }
    printf( " P - Proceed\n A - Abort\n H - Help\n" );
    c = getchar();
    while( getchar() != LINEFEED );
    if( isdigit(c) )
    {
      retval = 1 << ( c - '1' );
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

return retval;
}

#warning TODO: write whole function
char *request_dir( struct ParameterList *pl )
{
char *retval;

  retval = GetPL( pl, _DEFAULT ).arg[0];

return retval;
}

#warning TODO: write whole function
/* Ask user to insert a specific disk */
char *request_disk( struct ParameterList *pl )
{
char *retval;

  retval = GetPL( pl, _DEST ).arg[0];

return retval;
}

#warning TODO: write whole function
char *request_file( struct ParameterList *pl )
{
char *retval;

  retval = GetPL( pl, _DEFAULT ).arg[0];

return retval;
}

#warning TODO: write whole function
int request_options( struct ParameterList *pl )
{
int retval;

  retval = GetPL( pl, _DEFAULT ).intval;

return retval;
}

void abort_install()
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
                  /* Execute trap(1) */
                  traperr( "User aborted!\n" );
                  /* execute onerrors */
                  if( preferences.onerror.cmd != NULL )
                  {
                    execute_script( preferences.onerror.cmd, -99 );
                  }
                  exit(0);
                  break;
      case 'n'	: abort = 0;
                  break;
      default	: break;
    }
  } while( abort == -1 );
#endif /* DEBUG */
}

void final_report()
{
#ifdef DEBUG
  printf( "Application has bee installed in %s\n", get_var_arg( "@default-dest" ) );
#endif /* DEBUG */
}

void traperr( char * msg )
{
#ifdef DEBUG
  printf( "ERROR:\n %s\n", msg );
#endif /* DEBUG */
}

void parseerror( char *msg, int ln )
{
#ifdef DEBUG
  printf( msg, ln );
#endif /* DEBUG */
}


