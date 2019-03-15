/* grfont.h */

#ifndef GRFONT_H_
#define GRFONT_H_

#include "graph.h"

  extern const unsigned char  font_8x8[];


  void
  grGotobitmap( grBitmap*  bitmap );

  void
  grSetMargin( int  right,
               int  top );

  void
  grSetPixelMargin( int  right,
                    int  top );

  void
  grSetLineHeight( int  height );

  void
  grGotoxy ( int  x,
             int  y );


  void
  grWrite( const char*  string );

  void
  grWriteln( const char*  string );

  void
  grLn( void );

#endif /* GRFONT_H_ */


/* eof */
