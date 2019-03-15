#include <stdlib.h>

/*  Author:  David Turner  <david@freetype.org>
 *
 *  this filtering code is explicitly placed in the public domain !!
 */
#include <stdlib.h>
#include <memory.h>

#include "grswizzle.h"

/* technical note:
 *
 *   the following code is used to simulate the color display of an
 *   OLPC screen on a traditional LCD or CRT screen. First, here's more or
 *   less how the laptop's hardware works:
 *
 *   when in color mode, the screen uses the following colored pixels
 *   arrangement, where each pixel is square and can have its luminosity
 *   set between 0 and 255:
 *
 *     R G B R G B ....
 *     G B R G B R
 *     B R G B R G
 *     R G B R G B
 *     G B R G B R
 *     B R G B R G
 *     :
 *     :
 *
 *   in its normal mode of operation, the laptop's graphics chip gets its
 *   data from a normal frame buffer (where each pixel has three
 *   components: R, G and B) but only selects the red signal for the first
 *   pixel, the green signal for the second pixel, etc...
 *
 *   in other words, it ignores 2/3rd of the frame buffer data !
 *
 *   there is also another interesting mode of operation, so-called
 *   "anti-aliasing mode" where the value of displayed pixel is obtained
 *   by performing a simple 3x3 linear filter on the frame-buffer data.
 *   the filter's matrix being:
 *
 *       0   1/8   0
 *
 *      1/8  1/2  1/8
 *
 *       0   1/8   0
 *
 *   note that this filtering is per-color, so the *displayed* intensity of
 *   a given red pixel corresponds to an average of the pixel's red value and
 *   its four neighboring pixels' red values.
 *
 *   note that the code below uses a 3-lines work buffer, where each line
 *   in the buffer holds a copy of the source frame buffer.
 *
 *   more exactly, each line has 'width+2' pixels, where the first and last
 *   pixels are always set to 0. this allows us to ignore edge cases in the
 *   filtering code.
 *
 *   similarly, we artificially extend the source buffer with zero-ed lines
 *   above and below.
 */


/* define ANTIALIAS to perform anti-alias filtering before swizzling */
#define  ANTIALIAS

/* define POSTPROCESS to enhance the output for traditional displays,
 * otherwise, you'll get those ugly diagonals everywhere
 */
#define  POSTPROCESS

/************************************************************************/
/************************************************************************/
/*****                                                              *****/
/*****               G E N E R I C   F I L T E R I N G              *****/
/*****                                                              *****/
/************************************************************************/
/************************************************************************/

/* the type of a line filtering function, see below for usage */
typedef void
(*filter_func_t)( unsigned char**  lines,
                  unsigned char*   write,
                  int              width,
                  int              offset );

static void
copy_line_generic( unsigned char*    from,
                   unsigned char*    to,
                   int               x,
                   int               width,
                   int               buff_width,
                   int               pix_bytes )
{
  if (x > 0)
  {
    width += 1;
    from  -= pix_bytes;
  }
  else
    to += pix_bytes;

  if (x+width < buff_width)
    width += 1;

  memcpy( to, from, (unsigned int)( width * pix_bytes ) );
}



/* a generic function to perform 3x3 filtering of a given rectangle,
 * from a source bitmap into a destination one, the source *can* be
 * equal to the destination.
 *
 * IMPORTANT: this will read the rectangle (x-1,y-1,width+2,height+2)
 * from the source (clipping and edge cases are handled).
 *
 *  read_buff    :: first byte of source buffer
 *  read_pitch   :: source buffer bytes per row
 *  write_buff   :: first byte of target buffer
 *  write_pitch  :: target buffer bytes per row
 *  buff_width   :: width in pixels of both buffers
 *  buff_height  :: height in pixels of both buffers
 *  x            :: rectangle's left-most horizontal coordinate
 *  y            :: rectangle's top-most vertical coordinate
 *  width        :: rectangle width in pixels
 *  height       :: rectangle height in pixels
 *  pix_bytes    :: number of bytes per pixels in both buffer
 *  filter_func  :: line filtering function
 *  temp_lines   :: a work buffer of at least '3*(width+2)*pix_bytes' bytes
 */
static void
filter_rect_generic( unsigned char*   read_buff,
                     int              read_pitch,
                     unsigned char*   write_buff,
                     int              write_pitch,
                     int              buff_width,
                     int              buff_height,
                     int              x,
                     int              y,
                     int              width,
                     int              height,
                     int              pix_bytes,
                     filter_func_t    filter_func,
                     unsigned char*   temp_lines )
{
  unsigned char*  lines[3];
  int             offset   = (x+y) % 3;
  int             delta, height2;

  /* clip rectangle, just to be sure */
  if (x < 0)
  {
    width += x;
    x      = 0;
  }
  delta = x+width - buff_width;
  if (delta > 0)
    width -= delta;

  if (y < 0)
  {
    height += y;
    y       = 0;
  }
  delta = y+height - buff_height;
  if (delta > 0)
    height -= delta;

  if (width <= 0 || height <= 0)  /* nothing to do */
    return;

  /* now setup the three work lines */
  read_buff  += y*read_pitch  + pix_bytes*x;
  write_buff += y*write_pitch + pix_bytes*x;

  memset( temp_lines, 0, (unsigned int)(3 * pix_bytes * ( width + 2 ) ) );

  lines[0] = (unsigned char*) temp_lines;
  lines[1] = lines[0] + pix_bytes*(width+2);
  lines[2] = lines[1] + pix_bytes*(width+2);

  /* lines[0] correspond to the pixels of the line above
   */
   if (y > 0)
     copy_line_generic( read_buff - read_pitch, lines[0],
                        x, width, buff_width, pix_bytes );

  /* lines[1] correspond to the pixels of the current line
   */
   copy_line_generic( read_buff, lines[1],
                      x, width, buff_width, pix_bytes );

  /* process all lines, except the last one */
  for ( height2 = height; height2 > 1; height2-- )
  {
    unsigned char*   tmp;

    /* lines[2] correspond to the pixels of the line below */
    copy_line_generic( read_buff + read_pitch, lines[2],
                       x, width, buff_width, pix_bytes );

    filter_func( lines, write_buff, width, offset );

    if (++offset == 3)
      offset = 0;

    /* scroll the work lines */
    tmp      = lines[0];
    lines[0] = lines[1];
    lines[1] = lines[2];
    lines[2] = tmp;

    read_buff  += read_pitch;
    write_buff += write_pitch;
  }

  /* process last line */
  if (y+height == buff_height)
    memset( lines[2], 0, (unsigned int)( ( width + 2 ) * pix_bytes ) );
  else
    copy_line_generic( read_buff + read_pitch, lines[2],
                       x, width, buff_width, pix_bytes );

  filter_func ( lines, write_buff, width, offset );
}


/************************************************************************/
/************************************************************************/
/*****                                                              *****/
/*****               R G B 2 4   S U P P O R T                      *****/
/*****                                                              *****/
/************************************************************************/
/************************************************************************/


/* this function performs AA+swizzling of a given line from/to RGB24 buffers
 */
static void
swizzle_line_rgb24( unsigned char**  lines,
                    unsigned char*   write,
                    int              width,
                    int              offset )
{
  unsigned char*  above   = lines[0] + 3;
  unsigned char*  current = lines[1] + 3;
  unsigned char*  below   = lines[2] + 3;
  int             nn;

  width *= 3;
  for ( nn = 0; nn < width; nn += 3 )
  {
    unsigned int  sum;
    int           off = nn + offset;

#ifdef ANTIALIAS
    sum  = (unsigned int)current[off] << 2;

    sum += current[off-3] +
           current[off+3] +
           above  [off]   +
           below  [off]   ;

    /* performance trick: use shifts to avoid jumps */
    sum = (sum >> 3) << (offset*8);
#else /* !ANTIALIAS */
    sum = current[off] << (offset*8);
#endif

    write[nn]   = (unsigned char) sum;
    write[nn+1] = (unsigned char)(sum >> 8);
    write[nn+2] = (unsigned char)(sum >> 16);

    if ( ++offset == 3 )
      offset = 0;
  }
}


/* the following function is used to post-process the result of the
 * swizzling algorithm to provide a more pleasant output on normal
 * (LCD and CRT) display screens.
 *
 * that's because the normal processing creates images that are not
 * relevant to the display's true nature. For example, consider a 3x3
 * white square on the original frame buffer, after simple swizzling, this
 * will generate the following picture (represented by RGB triplets):
 *
 *  (255,0,0)(0,255,0)(0,0,255)
 *  (0,255,0)(0,0,255)(255,0,0)
 *  (0,0,255)(255,0,0)(0,255,0)
 *
 * the laptop's DCON chip ignores all the 0s above, and will essentially
 * display a *bright* white square
 *
 * a traditional display will not, and this will result in an image that
 * will be much darker (due to all the zeroes).
 *
 * moreover, on an typical LCD screen, this creates very visible
 * black diagonals. On a CRT, some thinner diagonals are also visible, but
 * this is mostly due to the fact that the human eye is much more sensitive
 * to green than red and blue
 */


/* in this algorithm we steal the green and blue components from each pixel's
 * neighbours. For example, for a red pixel, we compute the average of the
 * green pixels on its right and below it, and the average of the blue pixels
 * on its left and above it.
 */
static void
postprocess_line_rgb24( unsigned char** lines,
                        unsigned char*  write,
                        int             width,
                        int             offset )
{
  unsigned char*  above   = lines[0] + 3;
  unsigned char*  current = lines[1] + 3;
  unsigned char*  below   = lines[2] + 3;
  int             nn;

  width *= 3;
  for ( nn = 0; nn < width; nn += 3 )
  {
    if (offset == 0)  /* red */
    {
      write[nn]   = current[nn];
      write[nn+1] = (unsigned char)((current[nn+4] + below[nn+1]) >> 1);
      write[nn+2] = (unsigned char)((current[nn-1] + above[nn+2]) >> 1);
      offset      = 1;
    }
    else if (offset == 1)  /* green */
    {
      write[nn]   = (unsigned char)((current[nn-3] + above[nn]) >> 1);
      write[nn+1] = current[nn+1];
      write[nn+2] = (unsigned char)((current[nn+5] + below[nn+2]) >> 1);
      offset      = 2;
    }
    else  /* blue */
    {
      write[nn]   = (unsigned char)((current[nn+3] + below[nn])   >> 1);
      write[nn+1] = (unsigned char)((current[nn-2] + above[nn+1]) >> 1);
      write[nn+2] = current[nn+2];
      offset      = 0;
    }
  }
}


/************************************************************************/
/************************************************************************/
/*****                                                              *****/
/*****               R G B 5 6 5    S U P P O R T                   *****/
/*****                                                              *****/
/************************************************************************/
/************************************************************************/




/* this function performs AA+swizzling of a given line from/to RGB565 buffers
 */
static void
swizzle_line_rgb565( unsigned char** lines,
                     unsigned char*  _write,
                     int             width,
                     int             offset )
{
  unsigned short*  above   = (unsigned short*) lines[0] + 1;
  unsigned short*  current = (unsigned short*) lines[1] + 1;
  unsigned short*  below   = (unsigned short*) lines[2] + 1;
  unsigned short*  write   = (unsigned short*) _write;
  int              nn;

  static const unsigned int    masks[3] = { 0xf800, 0x07e0, 0x001f };

  for (nn = 0; nn < width; nn++)
  {
    unsigned int   mask = masks[offset];
#ifdef ANTIALIAS
    unsigned int   sum;

    sum  = ((unsigned int)current[nn] & mask) << 2;

    sum += ((unsigned int)current[nn-1] & mask) +
           ((unsigned int)current[nn+1] & mask) +
           ((unsigned int)above[nn] & mask)     +
           ((unsigned int)below[nn] & mask);

    write[nn] = (unsigned short)( (sum >> 3) & mask );
#else
    write[nn] = (unsigned short)( current[nn] & mask );
#endif

    if (++offset == 3)
      offset = 0;
  }
}


static void
postprocess_line_rgb565( unsigned char** lines,
                         unsigned char*  _write,
                         int             width,
                         int             offset )
{
  unsigned short*  above   = (unsigned short*) lines[0] + 1;
  unsigned short*  current = (unsigned short*) lines[1] + 1;
  unsigned short*  below   = (unsigned short*) lines[2] + 1;
  unsigned short*  write   = (unsigned short*) _write;
  int              nn;

  static const unsigned int  masks[5] = { 0xf800, 0x07e0, 0x001f, 0xf800, 0x07e0 };

  unsigned int     l_mask, r_mask, c_mask;

  l_mask = masks[offset];
  c_mask = masks[offset+1];
  r_mask = masks[offset+2];

  for ( nn = 0; nn < width; nn += 1 )
  {
    unsigned int  left, right, center, tmp;

    center =   current[nn];
    left   = ((current[nn-1] & l_mask) + (above[nn] & l_mask)) >> 1;
    right  = ((current[nn+1] & r_mask) + (below[nn] & r_mask)) >> 1;

    write[nn] = (unsigned short)( (left & l_mask)   |
                                  (right & r_mask)  |
                                  (center & c_mask) );

    tmp    = l_mask;
    l_mask = c_mask;
    c_mask = r_mask;
    r_mask = tmp;
  }
}



/************************************************************************/
/************************************************************************/
/*****                                                              *****/
/*****               X R G B 3 2    S U P P O R T                   *****/
/*****                                                              *****/
/************************************************************************/
/************************************************************************/


/* this function performs AA+swizzling of a given line from/to 32-bit ARGB or RGB
 * buffers
 */
static void
swizzle_line_xrgb32( unsigned char**  lines,
                     unsigned char*   _write,
                     int              width,
                     int              offset )
{
  unsigned int*  above   = (unsigned int*) lines[0] + 1;
  unsigned int*  current = (unsigned int*) lines[1] + 1;
  unsigned int*  below   = (unsigned int*) lines[2] + 1;
  unsigned int*  write   = (unsigned int*) _write;
  int            nn;
  unsigned int   mask    = (0xff0000) >> (offset*8);

  for (nn = 0; nn < width; nn++)
  {
#ifdef ANTIALIAS
    unsigned int   sum;

    sum  = (current[nn] & mask) << 2;

    sum += (current[nn-1] & mask) +
           (current[nn+1] & mask) +
           (above[nn]     & mask) +
           (below[nn]     & mask);

    write[nn] = (sum >> 3) & mask;  /* should we set ALPHA to 0xFF ? */
#else
    write[nn] = current[nn] & mask;
#endif

    mask >>= 8;
    if (mask == 0)
      mask = 0x00ff0000;
  }
}


static void
postprocess_line_xrgb32( unsigned char** lines,
                         unsigned char*  _write,
                         int             width,
                         int             offset )
{
  unsigned int*  above   = (unsigned int*) lines[0] + 1;
  unsigned int*  current = (unsigned int*) lines[1] + 1;
  unsigned int*  below   = (unsigned int*) lines[2] + 1;
  unsigned int*  write   = (unsigned int*) _write;
  int             nn;

  static const unsigned int  masks[5] =
                      { 0xff0000, 0x00ff00, 0x0000ff, 0xff0000, 0x00ff00 };

  unsigned int     l_mask, r_mask, c_mask;

  l_mask = masks[offset];
  c_mask = masks[offset+1];
  r_mask = masks[offset+2];

  for ( nn = 0; nn < width; nn += 1 )
  {
    unsigned int  left, right, center, tmp;

    center =   current[nn];
    left   = ((current[nn-1] & l_mask) + (above[nn] & l_mask)) >> 1;
    right  = ((current[nn+1] & r_mask) + (below[nn] & r_mask)) >> 1;

    write[nn] = (unsigned int)( (left   & l_mask)   |
                                (right  & r_mask)  |
                                (center & c_mask) );

    tmp    = l_mask;
    l_mask = c_mask;
    c_mask = r_mask;
    r_mask = tmp;
  }
}



static void
gr_swizzle_generic( unsigned char*    read_buff,
                   int                read_pitch,
                   unsigned char*     write_buff,
                   int                write_pitch,
                   int                buff_width,
                   int                buff_height,
                   int                x,
                   int                y,
                   int                width,
                   int                height,
                   int                pixbytes,
                   filter_func_t      swizzle_func,
                   filter_func_t      postprocess_func )
{
  unsigned char*  temp_lines;
  unsigned char   temp_local[ 2048 ];
  unsigned int    temp_size;

  if ( height <= 0 || width <= 0 )
    return;

  if ( read_pitch < 0 )
    read_buff -= (buff_height-1)*read_pitch;

  if ( write_pitch < 0 )
    write_buff -= (buff_height-1)*write_pitch;

 /* we allocate a work buffer that will be used to hold three
  * working 'lines', each of them having width+2 pixels. the first
  * and last pixels being always 0
  */
  temp_size = (unsigned int)( ( width + 2 ) * 3 * pixbytes );
  if ( temp_size <= sizeof ( temp_local ) )
  {
    /* try to use stack allocation, which is a lot faster than malloc */
    temp_lines = temp_local;
  }
  else
  {
    temp_lines = (unsigned char*)malloc( temp_size );
    if ( temp_lines == NULL )
      return;
  }

  filter_rect_generic( read_buff, read_pitch, write_buff, write_pitch,
                       buff_width, buff_height, x, y, width, height,
                       pixbytes, swizzle_func, temp_lines );


#ifdef POSTPROCESS
  /* perform darkness correction */
  if ( postprocess_func )
    filter_rect_generic( write_buff, write_pitch, write_buff, write_pitch,
                         buff_width, buff_height, x, y, width, height,
                         pixbytes, postprocess_func,
                         temp_lines );
#endif

  /* free work buffer if needed */
  if (temp_lines != temp_local)
    free( temp_lines );
}



extern void
gr_swizzle_rect_rgb24( unsigned char*    read_buff,
                       int               read_pitch,
                       unsigned char*    write_buff,
                       int               write_pitch,
                       int               buff_width,
                       int               buff_height,
                       int               x,
                       int               y,
                       int               width,
                       int               height )
{
  gr_swizzle_generic( read_buff, read_pitch,
                      write_buff, write_pitch,
                      buff_width,
                      buff_height,
                      x, y, width, height,
                      3,
                      swizzle_line_rgb24,
                      postprocess_line_rgb24 );
}



extern void
gr_swizzle_rect_rgb565( unsigned char*    read_buff,
                        int               read_pitch,
                        unsigned char*    write_buff,
                        int               write_pitch,
                        int               buff_width,
                        int               buff_height,
                        int               x,
                        int               y,
                        int               width,
                        int               height )
{
  gr_swizzle_generic( read_buff, read_pitch,
                      write_buff, write_pitch,
                      buff_width,
                      buff_height,
                      x, y, width, height,
                      2,
                      swizzle_line_rgb565,
                      postprocess_line_rgb565 );
}



extern void
gr_swizzle_rect_xrgb32( unsigned char*    read_buff,
                        int               read_pitch,
                        unsigned char*    write_buff,
                        int               write_pitch,
                        int               buff_width,
                        int               buff_height,
                        int               x,
                        int               y,
                        int               width,
                        int               height )
{
  gr_swizzle_generic( read_buff, read_pitch,
                      write_buff, write_pitch,
                      buff_width,
                      buff_height,
                      x, y, width, height,
                      4,
                      swizzle_line_xrgb32,
                      postprocess_line_xrgb32 );
}




