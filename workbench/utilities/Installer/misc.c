/* misc.c -- here are all miscellaneous functions for global use */

#include "Installer.h"

/* External variables */

/* External function prototypes */
extern void outofmem( void * );

/* Internal function prototypes */
int strtostrs ( char *, char *** );
char *addquotes ( char * );

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

/*
 * Add surrounding quotes to string
 * Creates a copy of input string
 */
char *addquotes ( char * string )
{
char *retval;
int c;

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

