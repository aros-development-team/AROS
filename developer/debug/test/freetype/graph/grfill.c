#include "graph.h"
#include <stdlib.h>
#include <memory.h>

static void
gr_fill_hline_mono( unsigned char*   line,
                    int              x,
                    int              width,
                    grColor          color )
{
  int  c1    = (x >> 3);
  int  lmask = 0xFF >> (x & 7);
  int  c2    = ((x+width-1) >> 3);
  int  rmask = 0x7F8 >> ((x+width-1) & 7);

  if ( color.value != 0 )
  {
    if ( c1 == c2 )
      line[c1] = (unsigned char)( line[c1] | (lmask & rmask));
    else
    {
      line[c1] = (unsigned char)(line[c1] | lmask);
      for ( ++c1; c1 < c2; c1++ )
        line[c1] = 0xFF;
      line[c2] = (unsigned char)(line[c2] | rmask);
    }
  }
  else
  {
    if ( c1 == c2 )
      line[c1] = (unsigned char)( line[c1] & ~(lmask & rmask) );
    else
    {
      line[c1] = (unsigned char)(line[c1] & ~lmask);
      for (++c1; c1 < c2; c1++)
        line[c1] = 0;
      line[c2] = (unsigned char)(line[c2] & ~rmask);
    }
  }
}

static void
gr_fill_hline_4( unsigned char*  line,
                 int             x,
                 int             width,
                 grColor         color )
{
  int  col = color.value | (color.value << 4);

  line += (x >> 1);
  if ( x & 1 )
  {
    line[0] = (unsigned char)((line[0] & 0xF0) | (col & 0x0F));
    line++;
    width--;
  }

  for ( ; width >= 2; width -= 2 )
  {
    line[0] = (unsigned char)col;
    line++;
  }

  if ( width > 0 )
    line[0] = (unsigned char)((line[0] & 0x0F) | (col & 0xF0));
}

static void
gr_fill_hline_8( unsigned char*   line,
                 int              x,
                 int              width,
                 grColor          color )
{
  memset( line+x, color.value, (unsigned int)width );
}

static void
gr_fill_hline_16( unsigned char*  _line,
                  int             x,
                  int             width,
                  grColor         color )
{
  unsigned short*  line = (unsigned short*)_line + x;

  for ( ; width > 0; width-- )
    *line++ = (unsigned short)color.value;
}

static void
gr_fill_hline_24( unsigned char*  line,
                  int             x,
                  int             width,
                  grColor         color )
{
  int  r = color.chroma[0];
  int  g = color.chroma[1];
  int  b = color.chroma[2];

  line += 3*x;

  if (r == g && g == b)
    memset( line, r, (unsigned int)(width*3) );
  else
  {
    for ( ; width > 0; width--, line += 3 )
    {
      line[0] = (unsigned char)r;
      line[1] = (unsigned char)g;
      line[2] = (unsigned char)b;
    }
  }
}

static void
gr_fill_hline_32( unsigned char*  line,
                  int             x,
                  int             width,
                  grColor         color )
{
  line += 4*x;

  /* clearly slow */
  for (; width > 0; width--, line += 4)
  {
    line[0] = color.chroma[0];
    line[1] = color.chroma[1];
    line[2] = color.chroma[2];
    line[3] = color.chroma[3];
  }
}


typedef void  (*grFillHLineFunc)( unsigned char*  line,
                                  int             x,
                                  int             width,
                                  grColor         color );

static const grFillHLineFunc  gr_fill_hline_funcs[gr_pixel_mode_max] =
{
  NULL,
  gr_fill_hline_mono,
  gr_fill_hline_4,
  gr_fill_hline_8,
  gr_fill_hline_8,
  gr_fill_hline_16,
  gr_fill_hline_16,
  gr_fill_hline_24,
  gr_fill_hline_32,
  NULL,
  NULL,
  NULL,
  NULL
};

extern void
grFillHLine( grBitmap*  target,
             int        x,
             int        y,
             int        width,
             grColor    color )
{
  int              delta;
  unsigned char*   line;
  grFillHLineFunc  hline_func = gr_fill_hline_funcs[target->mode];

  if ( x < 0 )
  {
    width += x;
    x      = 0;
  }
  delta = x + width - target->width;
  if ( delta > 0 )
    width -= delta;

  if ( y < 0 || y >= target->rows || width <= 0 || hline_func == NULL )
    return;

  line = target->buffer + y*target->pitch;
  if ( target->pitch < 0 )
    line -= target->pitch*(target->rows-1);

  hline_func( line, x, width, color );
}

extern void
grFillVLine( grBitmap*  target,
             int        x,
             int        y,
             int        height,
             grColor    color )
{
  int              delta;
  unsigned char*   line;
  grFillHLineFunc  hline_func = gr_fill_hline_funcs[ target->mode ];

  if ( y < 0 )
  {
    height += y;
    y       = 0;
  }
  delta = y + height - target->rows;
  if ( delta > 0 )
    height -= delta;

  if ( x < 0 || x >= target->width || height <= 0 || hline_func == NULL )
    return;

  line = target->buffer + y*target->pitch;
  if ( target->pitch < 0 )
    line -= target->pitch*(target->rows-1);

  for ( ; height > 0; height--, line += target->pitch )
    hline_func( line, x, 1, color );
}

extern void
grFillRect( grBitmap*   target,
            int         x,
            int         y,
            int         width,
            int         height,
            grColor     color )
{
  int              delta;
  unsigned char*   line;
  grFillHLineFunc  hline_func;
  int              size = 0;

  if ( x < 0 )
  {
    width -= x;
    x      = 0;
  }
  delta = x + width - target->width;
  if ( delta > 0 )
    width -= delta;

  if ( y < 0 )
  {
    height += y;
    y       = 0;
  }
  delta = y + height - target->rows;
  if ( delta > 0 )
    height -= delta;

  if ( width <= 0 || height <= 0 )
    return;

  line = target->buffer + y*target->pitch;
  if ( target->pitch < 0 )
    line -= target->pitch*(target->rows-1);

  hline_func = gr_fill_hline_funcs[ target->mode ];

  switch ( target->mode )
  {
  case gr_pixel_mode_rgb32:
    size++;
  case gr_pixel_mode_rgb24:
    size++;
  case gr_pixel_mode_rgb565:
  case gr_pixel_mode_rgb555:
    size += 2;
    hline_func( line, x, width, color );
    for ( line += size * x; --height > 0; line += target->pitch )
      memcpy( line + target->pitch, line, (size_t)size * (size_t)width );
    break;

  case gr_pixel_mode_gray:
  case gr_pixel_mode_pal8:
  case gr_pixel_mode_pal4:
  case gr_pixel_mode_mono:
    for ( ; height-- > 0; line += target->pitch )
      hline_func( line, x, width, color );
    break;

  default:
    break;
  }
}
