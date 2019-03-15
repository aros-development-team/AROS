/*
 *  This is a cheap replacement for getopt() because that routine is not
 *  available on some platforms and behaves differently on other platforms.
 *  This code was written from scratch without looking at any other
 *  implementation.
 *
 *  This code is hereby expressly placed in the public domain.
 *  mleisher@crl.nmsu.edu (Mark Leisher)
 *  10 October 1997
 *
 *  Last update 2009-03-11.
 */

#include "mlgetopt.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#ifdef __STDC__
#define CONST  const
#else
#define CONST
#endif

  /*
   *  Externals visible to programs.
   */

  int    opterr = 1;
  int    optind = 1;
  char*  optarg;

  /*
   *  Internal variables that are used to detect when the global values
   *  need to be reset.
   */

  static int  cmdac;
  static CONST char*   cmdname;
  static char* CONST*  cmdav;

  int
#ifdef __STDC__
  getopt( int  ac, char* const*  av, const char*  pat )
#else
  getopt( ac, av, pat )
    int     ac;
    char**  av;
    char*   pat;
#endif
  {
    int  opt;
    CONST char*  p;
    CONST char*  pp;

    /*
     *  If there is no pattern, indicate the parsing is done.
     */
    if ( pat == 0 || *pat == 0 )
      return -1;

    /*
     *  Always reset the option argument to NULL.
     */
    optarg = 0;

    /*
     *  If the number of arguments or argument list do not match the last
     *  values seen, reset the internal pointers and the globals.
     */
    if ( ac != cmdac || av != cmdav )
    {
      optind = 1;
      cmdac = ac;
      cmdav = av;

      /*
       *  Determine the command name in case it is needed for warning
       *  messages.
       */
      for ( cmdname = 0, p = av[0]; *p; p++ )
      {
        if ( *p == '/' || *p == '\\' )
          cmdname = p;
      }
      /*
       *  Skip the path separator if the name was assigned.
       */
      if ( cmdname )
        cmdname++;
      else
        cmdname = av[0];
    }

    /*
     *  If the next index is greater than or equal to the number of
     *  arguments, then the command line is done.
     */
    if ( optind >= ac )
      return -1;

    /*
     *  Test the next argument for one of three cases:
     *    1. The next argument does not have an initial '-'.
     *    2. The next argument is '-'.
     *    3. The next argument is '--'.
     *
     *  In either of these cases, command line processing is done.
     */
    if ( av[optind][0] != '-'            ||
         strcmp( av[optind], "-" ) == 0  ||
         strcmp( av[optind], "--" ) == 0 )
      return -1;

    /*
     *  Point at the next command line argument and increment the
     *  command line index.
     */
    p = av[optind++];

    /*
     *  Look for the first character of the command line option.
     */
    for ( opt = *(p + 1), pp = pat; *pp && *pp != opt; pp++ )
      ;

    /*
     *  If nothing in the pattern was recognized, then issue a warning
     *  and return a '?'.
     */
    if ( *pp == 0 )
    {
      if ( opterr )
        fprintf( stderr, "%s: invalid option -- %c\n", cmdname, opt );
      return '?';
    }

    /*
     *  If the option expects an argument, get it.
     */
    if ( *(pp + 1) == ':' && (optarg = av[optind]) == 0 )
    {
      /*
       *  If the option argument is NULL, issue a warning and return a '?'.
       */
      if ( opterr )
        fprintf( stderr, "%s: option requires an argument -- %c\n",
                         cmdname, opt );
      opt = '?';
    }
    else if ( optarg )
    /*
     *  Increment the option index past the argument.
     */
      optind++;

    /*
     *  Return the option character.
     */
    return opt;
  }


/* End */
