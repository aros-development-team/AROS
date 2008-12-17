/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality font engine         */
/*                                                                          */
/*  Copyright 2006 by                                                       */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/*                                                                          */
/*  gbench is a small program used to benchmark a new algorithm             */
/*  performing gamma-corrected alpha-blending.                              */
/*                                                                          */
/*  EXPERIMENTAL: The numbers given here do not correspond to               */
/*                typical usage patterns yet, and the algorithm             */
/*                can still be tuned.                                       */
/*                                                                          */
/****************************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

 /*
  *  gbench is a small program used to benchmark a new algorithm
  *  performing gamma-corrected alpha-blending.
  *
  *  EXPERIMENTAL: the numbers given here do not correspond to
  *                typical usage patterns yet, and the algorithm
  *                can still be tuned
  *
  */

#ifdef UNIX
#include <sys/time.h>
#endif
#include "gbench.h"

#define  xxCACHE

  static  int             use_gamma = 0;
  static  unsigned char   gamma_ramp[256];
  static  unsigned char   gamma_ramp_inv[256];


#define  ALGO_KEY_COUNT         256
#define  ALGO_GRADE_BITS        5
#define  ALGO_GRADE_COUNT      (1 << ALGO_GRADE_BITS)
#define  ALGO_GRADE_INDEX(x)   ((x) >> (8-ALGO_GRADE_BITS))

  typedef struct CKeyRec_
  {
    int             background;
    int             foreground;
    unsigned char*  grades;

  } CKeyRec, *CKey;

  static CKeyRec        ckeys  [ ALGO_KEY_COUNT ];
  static unsigned char  cgrades[ ALGO_KEY_COUNT * ALGO_GRADE_COUNT * 3 ];

  static  chits   = 0;
  static  cmiss1  = 0;
  static  cmiss2  = 0;


 /* clear the cache
  */
  static void
  cclear( void )
  {
    int  nn;

    for ( nn = 0; nn < ALGO_KEY_COUNT; nn++ )
      ckeys[nn].grades = NULL;
  }

 /* recompute the grade levels of a given key
  */
  static void
  ckey_reset( CKey   key )
  {
    int  back = key->background;
    int  fore = key->foreground;
    unsigned char*  gr = key->grades;
    int  nn;

    int  r1,g1,b1,r2,g2,b2;

    r1 = (unsigned char)( back >> 16 );
    g1 = (unsigned char)( back >> 8 );
    b1 = (unsigned char)( back );

    r2 = (unsigned char)( fore >> 16 );
    g2 = (unsigned char)( fore >> 8 );
    b2 = (unsigned char)( fore );

    gr[0] = r1;
    gr[1] = g1;
    gr[2] = b1;

    gr[3] = r2;
    gr[4] = g2;
    gr[5] = b2;

    gr += 6;

    if ( use_gamma )
    {
      r1 = gamma_ramp_inv[r1];
      g1 = gamma_ramp_inv[g1];
      b1 = gamma_ramp_inv[b1];

      r2 = gamma_ramp_inv[r2];
      g2 = gamma_ramp_inv[g2];
      b2 = gamma_ramp_inv[b2];
    }

    for ( nn = 1; nn < ALGO_GRADE_COUNT-1; nn++ )
    {
      int  r = r1 + ((r2-r1)*nn)/(ALGO_GRADE_COUNT-1);
      int  g = g1 + ((g2-g1)*nn)/(ALGO_GRADE_COUNT-1);
      int  b = b1 + ((b2-b1)*nn)/(ALGO_GRADE_COUNT-1);

      if ( use_gamma )
      {
        r = gamma_ramp[r];
        g = gamma_ramp[g];
        b = gamma_ramp[b];
      }

      gr[0] = (unsigned char)r;
      gr[1] = (unsigned char)g;
      gr[2] = (unsigned char)b;

      gr += 3;
    }
    cmiss2 ++;
  }

 /* lookup the grades of a given (background,foreground) couple
  */
  static const unsigned char*
  clookup( int  background,
           int  foreground )
  {
    int   index, index0;
    CKey  key;

    cmiss1++;

    index0 = ( background + foreground*7 ) % ALGO_KEY_COUNT;
    index  = index0;
    do
    {
      key = ckeys + index;

      if ( key->grades == NULL )
        goto NewNode;

      if ( key->background == background &&
           key->foreground == foreground )
        goto Exit;

      index = (index+1) % ALGO_KEY_COUNT;
    }
    while ( index != index0 );

   /* the cache is full, clear it completely
    */
    cclear();

  NewNode:
    key->background = background;
    key->foreground = foreground;
    key->grades     = cgrades + index0*(3*ALGO_GRADE_COUNT);

    ckey_reset( key );

  Exit:
    return  (const unsigned char*)key->grades;
  }



  void
  ggamma_set( double  gamma )
  {
    int    ii;
    double gamma_inv = 1.0f / gamma;

    cclear();

    for ( ii = 0; ii < 256; ii++ )
      gamma_ramp[ii] = (unsigned char)( pow( (double)ii/255.0f, gamma )*255 );

    for ( ii = 0; ii < 256; ii++ )
      gamma_ramp_inv[ii] = (unsigned char)( pow( (double)ii/255.0f, gamma_inv ) * 255.0f );

    use_gamma = (gamma != 1.0f);
  }



  static void
  gblitter_blitrgb24_gray_direct( GBlitter  blitter,
                                  int       color )
  {
    unsigned char   r = (unsigned char)(color >> 16);
    unsigned char   g = (unsigned char)(color >> 8);
    unsigned char   b = (unsigned char)(color);

    int             h = blitter->height;
    unsigned char*  src_line = blitter->src_line;
    unsigned char*  dst_line = blitter->dst_line;

    if ( use_gamma )
    {
      int  r1 = gamma_ramp_inv[r];
      int  g1 = gamma_ramp_inv[g];
      int  b1 = gamma_ramp_inv[b];

      do
      {
        unsigned char*  src = src_line + (blitter->src_x);
        unsigned char*  dst = dst_line + (blitter->dst_x*3);
        int             w   = blitter->width;

        do
        {
          int  a = src[0];

          if ( a < 2 )
          {
            /* nothing */
          }
          else if ( a >= 254 )
          {
            dst[0] = r;
            dst[1] = g;
            dst[2] = b;
          }
          else
          {
            int  r0 = dst[0];
            int  g0 = dst[1];
            int  b0 = dst[2];

            r0 = gamma_ramp_inv[r0];
            g0 = gamma_ramp_inv[g0];
            b0 = gamma_ramp_inv[b0];

            a = a + (a >> 7);

            r0 += (r1 - r0)*a/256;
            g0 += (g1 - g0)*a/256;
            b0 += (b1 - b0)*a/256;

            r0 = gamma_ramp[r0];
            g0 = gamma_ramp[g0];
            b0 = gamma_ramp[b0];

            dst[0] = (unsigned char)r0;
            dst[1] = (unsigned char)g0;
            dst[2] = (unsigned char)b0;
          }

          src += 1;
          dst += 3;
        }
        while (--w > 0);

        src_line += blitter->src_incr;
        dst_line += blitter->dst_incr;
      }
      while (--h > 0);

      return;
    }

    do
    {
      unsigned char*  src = src_line + (blitter->src_x);
      unsigned char*  dst = dst_line + (blitter->dst_x*3);
      int             w   = blitter->width;

      do
      {
        int  a = src[0];

        if ( a < 2 )
        {
          /* nothing */
        }
        else if ( a >= 254 )
        {
          dst[0] = r;
          dst[1] = g;
          dst[2] = b;
        }
        else
        {
          int  r0 = dst[0];
          int  g0 = dst[1];
          int  b0 = dst[2];

          a = a + (a >> 7);

          r0 += (r - r0)*a/256;
          g0 += (g - g0)*a/256;
          b0 += (b - b0)*a/256;

          dst[0] = (unsigned char)r0;
          dst[1] = (unsigned char)g0;
          dst[2] = (unsigned char)b0;
        }

        src += 1;
        dst += 3;
      }
      while (--w > 0);

      src_line += blitter->src_incr;
      dst_line += blitter->dst_incr;
    }
    while (--h > 0);
  }


  static void
  gblitter_blitrgb24_gray_cache( GBlitter  blitter,
                                 int       color )
  {
    unsigned char   r = (unsigned char)(color >> 16);
    unsigned char   g = (unsigned char)(color >> 8);
    unsigned char   b = (unsigned char)(color);

    int                   back   = -1;
    const unsigned char*  grades = NULL;

    int             h = blitter->height;
    unsigned char*  src_line = blitter->src_line;
    unsigned char*  dst_line = blitter->dst_line;

    do
    {
      unsigned char*  src = src_line + (blitter->src_x);
      unsigned char*  dst = dst_line + (blitter->dst_x*3);
      int             w   = blitter->width;

      do
      {
        int  a = src[0];

        if ( a < 2 )
        {
          /* nothing */
        }
        else if ( a >= 254 )
        {
          dst[0] = r;
          dst[1] = g;
          dst[2] = b;
        }
        else
        {
          int                   back0 = ((int)dst[0] << 16) | ((int)dst[1] << 8) | dst[2];
          const unsigned char*  g;

          if ( back0 != back )
          {
            grades = clookup( back0, color );
            back   = back0;
          }
          else
            chits++;

          g = grades + ALGO_GRADE_INDEX(a)*3;

          dst[0] = g[0];
          dst[1] = g[1];
          dst[2] = g[2];
        }

        src += 1;
        dst += 3;
      }
      while (--w > 0);

      src_line += blitter->src_incr;
      dst_line += blitter->dst_incr;
    }
    while (--h > 0);
  }



  int
  gblitter_init_rgb24( GBlitter   blitter,
                       GBitmap    src,
                       int        dst_x,
                       int        dst_y,
                       int        dst_width,
                       int        dst_height,
                       void*      dst_buffer,
                       int        dst_pitch )
  {
    int  width  = src->width;
    int  height = src->height;
    int  delta;
    int  src_x  = 0;
    int  src_y  = 0;

    if ( dst_x < 0 )
    {
      width += dst_x;
      src_x  = -dst_x;
      dst_x  = 0;
    }

    delta = dst_x + width - dst_width;
    if ( delta > 0 )
      width -= delta;

    if ( dst_y < 0 )
    {
      height += dst_y;
      src_y   = -dst_y;
      dst_y   = 0;
    }

    delta = dst_y + height - dst_height;
    if ( delta > 0 )
      height -= delta;

    if ( width <= 0 || height <= 0 )
    {
      blitter->width  = 0;
      blitter->height = 0;
      blitter->blit   = NULL;

      return 1;
    }

    blitter->width    = width;
    blitter->height   = height;

    blitter->src_x    = src_x;
    blitter->src_line = src->buffer + src_y*src->pitch;
    blitter->src_incr = src->pitch;

    blitter->dst_x    = dst_x;
    blitter->dst_line = (unsigned char*)dst_buffer + dst_y*dst_pitch;
    blitter->dst_incr = dst_pitch;

    return 0;
  }



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef UNIX
#include <sys/time.h>
#endif

typedef int  (*bench_t)( int  arg );

#define BENCH_TIME 3.0f

double
get_time(void)
{
#ifdef UNIX
  struct timeval tv;

  gettimeofday(&tv, NULL);
  return (double)tv.tv_sec + (double)tv.tv_usec / 1E6;
#else
  /* clock() has an awful precision (~10ms) under Linux 2.4 + glibc 2.2 */
  return (double)clock() / (double)CLOCKS_PER_SEC;
#endif
}


double  bench_time = BENCH_TIME;

static void
bench( bench_t      bench_func,
       int          bench_arg,
       const char*  title,
       int          max)
{
  int      i, n, done;
  double   t0, delta;

  printf("%-30s : ", title);
  fflush(stdout);

  n = 0;
  done = 0;
  t0 = get_time();
  do
  {
    if (!(*bench_func)( bench_arg ) )
      done++;
    n++;
    delta = get_time() - t0;
  }
  while ((!max || n < max) && delta < bench_time);

  printf("%5.3f us/op\n", delta * 1E6 / (double)done);
}



/* this is un-hinted "W" in Times New Roman
 * this glyph is very fuzzy
 */
static const unsigned char  glyph_data[18*14] =
{
   0x4a, 0x91, 0x94, 0x93, 0x5a, 0x00, 0x4c, 0x92, 0x94, 0x94, 0x67, 0x0b, 0x00,
 0x00, 0x61, 0x92, 0x93, 0x50,
   0x00, 0x65, 0xff, 0xbb, 0x00, 0x00, 0x00, 0x65, 0xff, 0xd5, 0x00, 0x00, 0x00,
 0x00, 0x00, 0xa4, 0x7f, 0x00,
   0x00, 0x08, 0xf0, 0xe6, 0x01, 0x00, 0x00, 0x02, 0xe4, 0xf1, 0x05, 0x00, 0x00,
 0x00, 0x00, 0xc0, 0x10, 0x00,
   0x00, 0x00, 0x9c, 0xff, 0x3f, 0x00, 0x00, 0x00, 0x8e, 0xff, 0x4b, 0x00, 0x00,
 0x00, 0x1d, 0xae, 0x00, 0x00,
   0x00, 0x00, 0x41, 0xff, 0x99, 0x00, 0x00, 0x00, 0x82, 0xff, 0xa6, 0x00, 0x00,
 0x00, 0x74, 0x57, 0x00, 0x00,
   0x00, 0x00, 0x01, 0xe3, 0xed, 0x05, 0x00, 0x00, 0xcd, 0xd9, 0xf4, 0x0b, 0x00,
 0x00, 0xc1, 0x0a, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x89, 0xff, 0x4d, 0x00, 0x33, 0xa1, 0x7a, 0xff, 0x5b, 0x00,
 0x25, 0xa6, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x2e, 0xff, 0xa7, 0x00, 0x8c, 0x46, 0x20, 0xfe, 0xb6, 0x00,
 0x7d, 0x4e, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0xd2, 0xf5, 0x0d, 0xca, 0x03, 0x00, 0xc4, 0xfb, 0x15,
 0xc4, 0x06, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x77, 0xff, 0x98, 0x90, 0x00, 0x00, 0x69, 0xff, 0x98,
 0x9d, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x1d, 0xfe, 0xff, 0x35, 0x00, 0x00, 0x13, 0xfa, 0xff,
 0x45, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0xbf, 0xd9, 0x00, 0x00, 0x00, 0x00, 0xb2, 0xe9,
 0x03, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x57, 0x94,
 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x0b, 0x13, 0x00, 0x00, 0x00, 0x00, 0x08, 0x19,
 0x00, 0x00, 0x00, 0x00, 0x00
};

static const GBitmapRec   glyph =
{
  18,
  14,
  18,
  (unsigned char*) glyph_data,
  GBITMAP_FORMAT_GRAY
};

#define  SIZE_X  640
#define  SIZE_Y  480


static unsigned char   buffer[ SIZE_X*3*SIZE_Y ];

unsigned long  seed = 0;

unsigned long  my_rand( void )
{
  seed = seed * 1103515245 + 12345;
  return ((seed >>16) & 32767);
}


#define  RAND(n)  ((unsigned int)my_rand() % (n))

static int
do_glyph( int  arg )
{
  GBlitterRec  blit;
  int          dst_x = RAND(SIZE_X);
  int          dst_y = RAND(SIZE_Y);
  int          color = 0xFFFFFF;  /* draw white exclusively */

  if ( gblitter_init_rgb24( &blit,
                             (GBitmap)&glyph,
                             dst_x,
                             dst_y,
                             SIZE_X,
                             SIZE_Y,
                             buffer,
                             SIZE_X*3 ) )
    return 1;

  if ( arg )
    gblitter_blitrgb24_gray_cache( &blit, color );
  else
    gblitter_blitrgb24_gray_direct( &blit, color );


  return 0;
}



static int
do_glyph_color( int  arg )
{
  GBlitterRec  blit;
  int          dst_x = RAND(SIZE_X);
  int          dst_y = RAND(SIZE_Y);
  int          r     = RAND(256);
  int          g     = RAND(256);
  int          b     = RAND(256);
  int          color = (r << 16) | (g << 8) | b;  /* draw in colors */

  if ( gblitter_init_rgb24( &blit,
                             (GBitmap)&glyph,
                             dst_x,
                             dst_y,
                             SIZE_X,
                             SIZE_Y,
                             buffer,
                             SIZE_X*3 ) )
    return 1;

  if ( arg )
    gblitter_blitrgb24_gray_cache( &blit, color );
  else
    gblitter_blitrgb24_gray_direct( &blit, color );

  return 0;
}


static void
dump_cache_stats( void )
{
  printf( "hits = %ld, miss1 = %ld, miss2 = %ld, hitrate=%.2f%%, miss2rate=%.2f%%\n",
          chits, cmiss1, cmiss2, (double)chits*100.0 / (chits+cmiss1), (double)cmiss2*100.0 / (double)cmiss1 );
}


void usage(void)
{
  fprintf( stderr,
    "gbench: graphics glyph blending benchmark\n"
    "-----------------------------------------\n\n"
    "Usage: gbench [options]\n\n"
    "options:\n" );
  fprintf( stderr,
  "   -t : max time per bench in seconds (default is %.0f)\n", BENCH_TIME );
  fprintf( stderr,
  "   -s seed  : specify random seed\n" );
  fprintf( stderr,
  "   -g gamma : specify gamma\n" );
  exit( 1 );
}

#define TEST(x) (!tests || strchr(tests, x))

int
main(int argc,
     char** argv)
{
  char* tests = NULL;
  int size;
  double gamma = 1.0;

  while (argc > 1 && argv[1][0] == '-')
  {
    switch (argv[1][1])
    {
    case 't':
      argc--;
      argv++;
      if (argc < 1 ||
          sscanf(argv[1], "%lf", &bench_time) != 1)
        usage();
      break;

    case 'g':
      argc--;
      argv++;
      if (argc < 1 ||
          sscanf(argv[1], "%lf", &gamma) != 1)
        usage();
      break;


    case 's':
      if ( argc < 1 )
        usage();

      seed = (unsigned long)atol( argv[1] );
      argc -= 2;
      argv += 2;
      break;

#if 0
    case 'b':
      argc--;
      argv++;
      if (argc < 2)
        usage();
      tests = argv[1];
      break;
#endif

    default:
      fprintf(stderr, "Unknown argument `%s'\n\n", argv[1]);
      usage();
      break;
    }

    argc--;
    argv++;
  }

  if ( argc != 1 )
    usage();

  ggamma_set( gamma );

  memset( buffer, 0, sizeof(buffer) );
  if (TEST('a')) bench( do_glyph, 0, "direct white glyph", 0 );

  chits = cmiss1 = cmiss2 = 0;
  memset( buffer, 0, sizeof(buffer) );
  if (TEST('b')) bench( do_glyph, 1, "cache white glyph", 0 );
  dump_cache_stats();

  memset( buffer, 0, sizeof(buffer) );
  if (TEST('c')) bench( do_glyph_color, 0, "direct color glyph", 0 );

  chits = cmiss1 = cmiss2 = 0;
  memset( buffer, 0, sizeof(buffer) );
  if (TEST('d')) bench( do_glyph_color, 1, "cache color glyph", 0 );
  dump_cache_stats();

  return 0;
}


/* End */
