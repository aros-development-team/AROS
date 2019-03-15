/* some utility functions */

#include "common.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>


  char*
  ft_basename( const char*  name )
  {
    const char*  base;
    const char*  current;
    char         c;

    base    = name;
    current = name;

    c = *current;

    while ( c )
    {
#ifndef macintosh
      if ( c == '/' || c == '\\' )
#else
      if ( c == ':' )
#endif
        base = current + 1;

      current++;
      c = *current;
    }

    return (char*)base;
  }


  void
  Panic( const char*  fmt,
         ... )
  {
    va_list  ap;


    va_start( ap, fmt );
    vprintf( fmt, ap );
    va_end( ap );

    exit( 1 );
  }


  extern int
  utf8_next( const char**  pcursor,
             const char*   end )
  {
    const unsigned char*  p = (const unsigned char*)*pcursor;
    int                   ch;


    if ( (const char*)p >= end ) /* end of stream */
      return -1;

    ch = *p++;
    if ( ch >= 0x80 )
    {
      int  len;


      if ( ch < 0xc0 )  /* malformed data */
        goto BAD_DATA;
      else if ( ch < 0xe0 )
      {
        len = 1;
        ch &= 0x1f;
      }
      else if ( ch < 0xf0 )
      {
        len = 2;
        ch &= 0x0f;
      }
      else
      {
        len = 3;
        ch &= 0x07;
      }

      while ( len > 0 )
      {
        if ( (const char*)p >= end || ( p[0] & 0xc0 ) != 0x80 )
          goto BAD_DATA;

        ch   = ( ch << 6 ) | ( p[0] & 0x3f );
        p   += 1;
        len -= 1;
      }
    }

    *pcursor = (const char*)p;

    return ch;

  BAD_DATA:
    return -1;
  }


/* End */
