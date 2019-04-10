/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  Copyright (C) 2015-2019 by                                              */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/*                                                                          */
/*  output.c - string output routines for the FreeType demo programs.       */
/*                                                                          */
/****************************************************************************/


#include "output.h"


  static char hexdigit[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'A', 'B', 'C', 'D', 'E', 'F'
  };


  void
  put_ascii_string( char*     out,
                    FT_Byte*  string,
                    FT_UInt   string_len,
                    FT_UInt   indent )
  {
    FT_UInt  i, j;


    for ( j = 0; j < indent; j++ )
      *out++ = ' ';
    *out++ = '"';

    for ( i = 0; i < string_len; i++ )
    {
      switch ( string[i] )
      {
      case '\n':
        *out++ = '\\';
        *out++ = 'n';
        *out++ = '"';

        if ( i + 1 < string_len )
        {
          *out++ = '\n';
          for ( j = 0; j < indent; j++ )
            *out++ = ' ';
          *out++ = '"';
        }
        break;

      case '\r':
        *out++ = '\\';
        *out++ = 'r';
        break;

      case '\t':
        *out++ = '\\';
        *out++ = 't';
        break;

      case '\\':
        *out++ = '\\';
        *out++ = '\\';
        break;

      case '"':
        *out++ = '\\';
        *out++ = '"';
        break;

      case 0xA9:
        *out++ = '(';
        *out++ = 'c';
        *out++ = ')';
        break;

      default:
        if ( string[i] < 0x80 )
          *out++ = (char)string[i];
        else
        {
          *out++ = '\\';
          *out++ = 'x';
          *out++ = hexdigit[string[i] >> 4];
          *out++ = hexdigit[string[i] & 0xF];
        }
        break;
      }
    }
    if ( string[i - 1] != '\n' )
      *out++ = '"';

    *out++ = '\0';
  }


  FT_UInt
  put_ascii_string_size( FT_Byte*  string,
                         FT_UInt   string_len,
                         FT_UInt   indent )
  {
    FT_UInt  i, len = 0;


    /* the code formatting follows `put_ascii_string' */

    len += indent;
    len += 1;

    for ( i = 0; i < string_len; i++ )
    {
      switch ( string[i] )
      {
      case '\n':
        len += 3;

        if ( i + 1 < string_len )
        {
          len += 1;
          len += indent;
          len += 1;
        }
        break;

      case '\r':
      case '\t':
      case '\\':
      case '"':
        len += 2;
        break;

      case 0xA9:
        len += 3;
        break;

      default:
        if ( string[i] < 0x80 )
          len += 1;
        else
          len += 4;
        break;
      }
    }
    if ( string[i - 1] != '\n' )
      len += 1;

    len += 1;

    return len;
  }


  void
  put_ascii( FT_Byte*  string,
             FT_UInt   string_len,
             FT_UInt   indent )
  {
    FT_UInt  len;
    char*    s;


    len = put_ascii_string_size( string, string_len, indent );
    s = (char*)malloc( len );
    if ( !s )
      printf( "allocation error for name string" );
    else
    {
      put_ascii_string( s, string, string_len, indent );
      fputs( s, stdout );
      free( s );
    }
  }


  void
  put_unicode_be16_string( char*     out,
                           FT_Byte*  string,
                           FT_UInt   string_len,
                           FT_UInt   indent,
                           FT_Int    as_utf8 )
  {
    FT_Int   ch = 0;
    FT_UInt  i, j;


    for ( j = 0; j < indent; j++ )
      *out++ = ' ';
    *out++ = '"';

    for ( i = 0; i < string_len; i += 2 )
    {
      ch = ( string[i] << 8 ) | string[i + 1];

      switch ( ch )
      {
      case '\n':
        *out++ = '\\';
        *out++ = 'n';
        *out++ = '"';

        if ( i + 2 < string_len )
        {
          *out++ = '\n';
          for ( j = 0; j < indent; j++ )
            *out++ = ' ';
          *out++ = '"';
        }
        continue;

      case '\r':
        *out++ = '\\';
        *out++ = 'r';
        continue;

      case '\t':
        *out++ = '\\';
        *out++ = 't';
        continue;

      case '\\':
        *out++ = '\\';
        *out++ = '\\';
        continue;

      case '"':
        *out++ = '\\';
        *out++ = '"';
        continue;

      default:
        break;
      }

      if ( as_utf8 )
      {
        /*
         * UTF-8 encoding
         *
         *   0x00000080 - 0x000007FF:
         *        110xxxxx 10xxxxxx
         *
         *   0x00000800 - 0x0000FFFF:
         *        1110xxxx 10xxxxxx 10xxxxxx
         */

        if ( ch < 0x80 )
          *out++ = (char)ch;
        else if ( ch < 0x800 )
        {
          *out++ = (char)( 0xC0 | ( (FT_UInt)ch >> 6 ) );
          *out++ = (char)( 0x80 | ( (FT_UInt)ch & 0x3F ) );
        }
        else
        {
          /* we don't handle surrogates */
          *out++ = (char)( 0xE0 | ( (FT_UInt)ch >> 12 ) );
          *out++ = (char)( 0x80 | ( ( (FT_UInt)ch >> 6 ) & 0x3F ) );
          *out++ = (char)( 0x80 | ( (FT_UInt)ch & 0x3F ) );
        }

        continue;
      }

      switch ( ch )
      {
      case 0x00A9:
        *out++ = '(';
        *out++ = 'c';
        *out++ = ')';
        continue;

      case 0x00AE:
        *out++ = '(';
        *out++ = 'r';
        *out++ = ')';
        continue;

      case 0x2013:
        *out++ = '-';
        *out++ = '-';
        continue;

      case 0x2019:
        *out++ = '\'';
        continue;

      case 0x2122:
        *out++ = '(';
        *out++ = 't';
        *out++ = 'm';
        *out++ = ')';
        continue;

      default:
        if ( ch < 128 )
          *out++ = (char)ch;
        else
        {
          *out++ = '\\';
          *out++ = 'U';
          *out++ = '+';
          *out++ = hexdigit[( ch >> 12 ) & 0xF];
          *out++ = hexdigit[( ch >> 8  ) & 0xF];
          *out++ = hexdigit[( ch >> 4  ) & 0xF];
          *out++ = hexdigit[  ch         & 0xF];
        }
        continue;
      }
    }

    if ( ch != '\n' )
      *out++ = '"';

    *out++ = '\0';
  }


  FT_UInt
  put_unicode_be16_string_size( FT_Byte*  string,
                                FT_UInt   string_len,
                                FT_UInt   indent,
                                FT_Int    as_utf8 )
  {
    FT_Int   ch = 0;
    FT_UInt  i, len = 0;


    /* the code formatting follows `put_unicode_be16_string' */

    len += indent;
    len += 1;

    for ( i = 0; i < string_len; i += 2 )
    {
      ch = ( string[i] << 8 ) | string[i + 1];

      switch ( ch )
      {
      case '\n':
        len += 3;

        if ( i + 2 < string_len )
        {
          len += 1;
          len += indent;
          len += 1;
        }
        continue;

      case '\r':
      case '\t':
      case '\\':
      case '"':
        len += 2;
        continue;

      default:
        break;
      }

      if ( as_utf8 )
      {
        if ( ch < 0x80 )
          len += 1;
        else if ( ch < 0x800 )
          len += 2;
        else
          len += 3;

        continue;
      }

      switch ( ch )
      {
      case 0x00A9:
      case 0x00AE:
        len += 3;
        continue;

      case 0x2013:
        len += 2;
        continue;

      case 0x2019:
        len += 1;
        continue;

      case 0x2122:
        len += 4;
        continue;

      default:
        if ( ch < 128 )
          len += 1;
        else
          len += 7;
        continue;
      }
    }

    if ( ch != '\n' )
      len += 1;

    len += 1;

    return len;
  }


  void
  put_unicode_be16( FT_Byte*  string,
                    FT_UInt   string_len,
                    FT_UInt   indent,
                    FT_Int    utf8 )
  {
    FT_UInt  len;
    char*    s;


    len = put_unicode_be16_string_size( string, string_len, indent, utf8 );
    s = (char*)malloc( len );
    if ( !s )
      printf( "allocation error for name string" );
    else
    {
      put_unicode_be16_string( s, string, string_len, indent, utf8 );
      fputs( s, stdout );
      free( s );
    }
  }


/* End */
