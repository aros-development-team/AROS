/* gui.c -- here are all functions for the gui */
#include "Installer.h"

/* External variables */
extern FILE *inputfile;
extern char buffer[MAXARGSIZE];
extern char *filename;

/* External function prototypes */

/* Internal function prototypes */
void show_complete( int );
void show_parseerror( int );


void show_complete( int percent )
{
#ifdef DEBUG
  printf( "Done %d%c!\n", percent, PERCENT );
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

