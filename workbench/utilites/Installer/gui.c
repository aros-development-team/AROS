/* gui.c -- here are all functions for the gui */

#include "Installer.h"

/* External variables */
extern FILE *inputfile;
extern char buffer[MAXARGSIZE];
extern char *filename;

/* External function prototypes */
char *get_var_arg( char * );

/* Internal function prototypes */
void show_abort( char * );
void show_complete( int );
void show_exit( char * );
void show_parseerror( int );
void show_working( char * );
int request_userlevel();
void abort_install();
void final_report();


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
  printf( "%s\n\nDone with installation.\n\n", msg );
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

void show_welcome( char *msg )
{
#ifdef DEBUG
  printf( "%s\n", msg );
#endif /* DEBUG */
}

void show_working( char *msg )
{
#ifdef DEBUG
  printf( "Working on Installation:\n%s\n", msg );
#endif /* DEBUG */
}

int request_userlevel()
{
int usrlevel = -1;

#ifdef DEBUG
int c;

  do
  {
    printf( "Which user-level do you want?\n 0 - Novice\n 1 - Average\n 2 - Expert\n A - Abort\n" );
    c = getchar();
    while( getchar() != LINEFEED );
    switch( tolower( c ) )
    {
      case 'a'	: abort_install( 0 );
                  break;
      case '0'	: usrlevel = _NOVICE;
                  break;
      case '1'	: usrlevel = _AVERAGE;
                  break;
      case '2'	: usrlevel = _EXPERT;
                  break;
      default	: break;
    }
  } while( usrlevel == -1 );
#else /* DEBUG */
  usrlevel = 2;
#endif /* DEBUG */

return usrlevel;
}

void abort_install( int xtrap )
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
      case 'y'	: /* if (xtrap != 0) : Execute trap */
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

