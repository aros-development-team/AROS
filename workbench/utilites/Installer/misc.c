/* misc.c -- here are all miscellaneous functions for global use */

#include "Installer.h"

/* External variables */

/* External function prototypes */
extern void outofmem( void * );

/* Internal function prototypes */
int strtostrs ( char *, char *** );
char *addquotes ( char * );
void freestrlist( STRPTR * );

/*
 * Break string into array of strings at LINEFEEDs
 * **outarr must be obtained via malloc!
 */
int strtostrs ( char * in, char ***outarr )
{
int i = 0, j = 0, k = 0;
char **out = *outarr;

  while( *in )
  {
    i++;
    /* malloc space for next string */
    out = realloc( out, ( i + 1 ) * sizeof( char *) );
    for( j = 0 ; in[j] && in[j]!=LINEFEED ; j++ );
    out[i-1] = malloc( ( j + 1 ) * sizeof( char ) );
    outofmem( out[i-1] );
    for( k = 0 ; k < j ; k++ )
    {
      /* save char to string */
      out[i-1][k] = *in;
      in++;
    }
    /* NULL-terminate string */
    out[i-1][j] = 0;
    if( *in )
      in++;
  }
  /* NULL-terminate array */
  out[i] = NULL;
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

/*
 * free() array of strings ( eg. allocated by strtostrs() )
 */
void freestrlist( STRPTR *array )
{
int i=0;

  while(array[i])
  {
    free(array[i]);
    i++;
  }
  free(array);
}

