#include "gblender.h"
#include <stdlib.h>

#if 0  /* using slow power functions */

#include <math.h>

static void
gblender_set_gamma_table( double           gamma_value,
                          unsigned short*  gamma_ramp,
                          unsigned char*   gamma_ramp_inv )
{
  const int  gmax = (256 << GBLENDER_GAMMA_SHIFT)-1;

  if ( gamma_value <= 0 )  /* special case for sRGB */
  {
    int  ii;

    for ( ii = 0; ii < 256; ii++ )
    {
      double  x = (double)ii / 255.0;

      if ( x <= 0.039285714 )
        x /= 12.92321;
      else
        x = pow( (x+0.055)/ 1.055, 2.4 );

      gamma_ramp[ii] = (unsigned short)(gmax*x + 0.5);
    }

    for ( ii = 0; ii <= gmax; ii++ )
    {
      double  x = (double)ii / gmax;

      if ( x <= 0.0030399346 )
        x *= 12.92321;
      else
        x = 1.055*pow(x,1/2.4) - 0.055;

      gamma_ramp_inv[ii] = (unsigned char)(255.*x + 0.5);
    }
  }
  else
  {
    int     ii;
    double  gamma_inv = 1. / gamma_value;

    /* voltage to linear */
    for ( ii = 0; ii < 256; ii++ )
      gamma_ramp[ii] =
        (unsigned short)( gmax*pow( (double)ii/255., gamma_value ) + 0.5 );

    /* linear to voltage */
    for ( ii = 0; ii <= gmax; ii++ )
      gamma_ramp_inv[ii] =
        (unsigned char)( 255.*pow( (double)ii/gmax, gamma_inv ) + 0.5 );
  }
}

#else  /* using fast finite differences */

static void
gblender_set_gamma_table( double           gamma_value,
                          unsigned short*  gamma_ramp,
                          unsigned char*   gamma_ramp_inv )
{
  const int  gmax = (256 << GBLENDER_GAMMA_SHIFT)-1;
  double     p;

  if ( gamma_value <= 0 )  /* special case for sRGB */
  {
    int     ii;

    /* voltage to linear; power function using finite differences */
    for ( p = 1.0, ii = 255; ii > (int)(255.*0.039285714); ii-- )
    {
      gamma_ramp[ii] = (unsigned short)( gmax*p + 0.5 );
      p -= 2.4 * p / ( ii + 255. * 0.055 );
    }
    for ( ; ii >= 0; ii-- )
      gamma_ramp[ii] = (unsigned short)( gmax*ii/(255.*12.92321) + 0.5 );


    /* linear to voltage; power function using finite differences */
    for ( p = 1.0, ii = gmax; ii > (int)(gmax*0.0030399346); ii-- )
    {
      gamma_ramp_inv[ii] = (unsigned char)( 255.*p + 0.5 );
      p -= ( p + 0.055 ) / ( 2.4 * ii );
    }
    for ( ; ii >= 0; ii-- )
      gamma_ramp_inv[ii] = (unsigned char)( 255.*12.92321*ii/gmax + 0.5 );
  }
  else
  {
    int     ii;
    double  gamma_inv = 1. / gamma_value;

    /* voltage to linear; power function using finite differences */
    for ( p = 1.0, ii = 255; ii > 0; ii-- )
    {
      gamma_ramp[ii] = (unsigned short)( gmax*p + 0.5 );
      p -= gamma_value * p / ii;
    }
    gamma_ramp[ii] = 0;

    /* linear to voltage; power function using finite differences */
    for ( p = 1.0, ii = gmax; ii > 0; ii-- )
    {
      gamma_ramp_inv[ii] = (unsigned char)( 255.*p + 0.5 );
      p -= gamma_inv * p / ii;
    }
    gamma_ramp[ii] = 0;
  }
}

#endif

/* clear the cache
 */
static void
gblender_clear( GBlender  blender )
{
  int          nn;
  GBlenderKey  keys = blender->keys;

  if ( blender->channels )
  {
    GBlenderChanKey  chan_keys = (GBlenderChanKey) blender->keys;

    for ( nn = 0; nn < GBLENDER_KEY_COUNT; nn++ )
      chan_keys[nn].index = -1;

    blender->cache_r_back  = ~0U;
    blender->cache_r_fore  = ~0U;
    blender->cache_r_cells = NULL;

    blender->cache_g_back  = ~0U;
    blender->cache_g_fore  = ~0U;
    blender->cache_g_cells = NULL;

    blender->cache_b_back  = ~0U;
    blender->cache_b_fore  = ~0U;
    blender->cache_b_cells = NULL;
  }
  else
  {
    for ( nn = 0; nn < GBLENDER_KEY_COUNT; nn++ )
      keys[nn].cells = NULL;

    blender->cache_back  = ~0U;
    blender->cache_fore  = ~0U;
    blender->cache_cells = NULL;
  }
}

GBLENDER_APIDEF( void )
gblender_reset( GBlender  blender )
{
  gblender_clear( blender );

  if ( blender->channels )
  {
    blender->cache_r_back  = 0;
    blender->cache_r_fore  = 0xFFFFFF;
    blender->cache_r_cells = gblender_lookup_channel( blender,
                                                      blender->cache_r_back,
                                                      blender->cache_r_fore );
    blender->cache_g_back  = 0;
    blender->cache_g_fore  = 0xFFFFFF;
    blender->cache_g_cells = gblender_lookup_channel( blender,
                                                      blender->cache_g_back,
                                                      blender->cache_g_fore );
    blender->cache_b_back  = 0;
    blender->cache_b_fore  = 0xFFFFFF;
    blender->cache_b_cells = gblender_lookup_channel( blender,
                                                      blender->cache_b_back,
                                                      blender->cache_b_fore );
  }
  else
  {
    blender->cache_back  = 0;
    blender->cache_fore  = 0xFFFFFF;
    blender->cache_cells = gblender_lookup( blender,
                                            blender->cache_back,
                                            blender->cache_fore );
  }

#ifdef GBLENDER_STATS
  blender->stat_hits    = 0;
  blender->stat_lookups = 0;
  blender->stat_keys    = 0;
  blender->stat_clears  = 0;
#endif
}

GBLENDER_APIDEF( void )
gblender_init( GBlender   blender,
               double     gamma_value )
{
  blender->channels = 0;

  gblender_set_gamma_table ( gamma_value,
                             blender->gamma_ramp,
                             blender->gamma_ramp_inv );

  gblender_reset( blender );
}


GBLENDER_API( void )
gblender_use_channels( GBlender  blender,
                       int       channels )
{
  channels = (channels != 0);

  if ( blender->channels != channels )
  {
    blender->channels = channels;
    gblender_reset( blender );
  }
}



/* recompute the grade levels of a given key
 */
static void
gblender_reset_key( GBlender     blender,
                    GBlenderKey  key )
{
  GBlenderPixel  back = key->background;
  GBlenderPixel  fore = key->foreground;
  GBlenderCell*  gr   = key->cells;
  unsigned int   nn;

  const unsigned char*   gamma_ramp_inv = blender->gamma_ramp_inv;
  const unsigned short*  gamma_ramp     = blender->gamma_ramp;

  unsigned int  r1,g1,b1,r2,g2,b2;

  r1 = ( back >> 16 ) & 255;
  g1 = ( back >> 8 )  & 255;
  b1 = ( back )       & 255;

  r2 = ( fore >> 16 ) & 255;
  g2 = ( fore >> 8 )  & 255;
  b2 = ( fore )       & 255;

#ifdef GBLENDER_STORE_BYTES
  gr[0] = (unsigned char)r1;
  gr[1] = (unsigned char)g1;
  gr[2] = (unsigned char)b1;
  gr   += 3;
#else
  gr[0] = back;
  gr   += 1;
#endif

  r1 = gamma_ramp[r1];
  g1 = gamma_ramp[g1];
  b1 = gamma_ramp[b1];

  r2 = gamma_ramp[r2];
  g2 = gamma_ramp[g2];
  b2 = gamma_ramp[b2];

  for ( nn = 1; nn < GBLENDER_SHADE_COUNT; nn++ )
  {
    unsigned int  a = 255 * nn / ( GBLENDER_SHADE_COUNT - 1 );
    unsigned int  r, g, b;


    r = ( r2 * a + r1 * ( 255 - a ) + 127 ) / 255;
    g = ( g2 * a + g1 * ( 255 - a ) + 127 ) / 255;
    b = ( b2 * a + b1 * ( 255 - a ) + 127 ) / 255;

    r = gamma_ramp_inv[r];
    g = gamma_ramp_inv[g];
    b = gamma_ramp_inv[b];

#ifdef GBLENDER_STORE_BYTES
    gr[0] = (unsigned char)r;
    gr[1] = (unsigned char)g;
    gr[2] = (unsigned char)b;
    gr   += 3;
#else
    gr[0] = (( r & 255 ) << 16) |
            (( g & 255 ) << 8 ) |
            (( b & 255 )      ) ;
    gr ++;
#endif
  }
}

 /* lookup the grades of a given (background,foreground) couple
  */
GBLENDER_APIDEF( GBlenderCell* )
gblender_lookup( GBlender       blender,
                 GBlenderPixel  background,
                 GBlenderPixel  foreground )
{
  int          idx, idx0;
  GBlenderKey  key;

#ifdef GBLENDER_STATS
  blender->stat_hits--;
  blender->stat_lookups++;
#endif

#if 0
  if ( blender->channels )
  {
    /* set to normal mode */
    blender->channels = 0;
    gblender_reset( blender );
  }
#endif

  idx0 = ( background + foreground*63 ) & (GBLENDER_KEY_COUNT-1);
  idx  = idx0;
  do
  {
    key = blender->keys + idx;

    if ( key->cells == NULL )
      goto NewNode;

    if ( key->background == background &&
         key->foreground == foreground )
      goto Exit;

    idx = (idx+1) & (GBLENDER_KEY_COUNT-1);
  }
  while ( idx != idx0 );

 /* the cache is full, clear it completely
  */
#ifdef GBLENDER_STATS
  blender->stat_clears++;
#endif
  gblender_clear( blender );

NewNode:
  key->background = background;
  key->foreground = foreground;
  key->cells      = blender->cells +
                    idx*(GBLENDER_SHADE_COUNT*GBLENDER_CELL_SIZE);

  gblender_reset_key( blender, key );

#ifdef GBLENDER_STATS
  blender->stat_keys++;
#endif

Exit:
  return  key->cells;
}


static void
gblender_reset_channel_key( GBlender         blender,
                            GBlenderChanKey  key )
{
  unsigned int    back = key->backfore & 255;
  unsigned int    fore = (key->backfore >> 8) & 255;
  unsigned char*  gr   = (unsigned char*)blender->cells + key->index;
  unsigned int    nn;

  const unsigned char*   gamma_ramp_inv = blender->gamma_ramp_inv;
  const unsigned short*  gamma_ramp     = blender->gamma_ramp;

  unsigned int  r1,r2;

  r1    = back;
  r2    = fore;

  gr[0] = (unsigned char)r1;
  gr++;

  r1 = gamma_ramp[r1];
  r2 = gamma_ramp[r2];

  for ( nn = 1; nn < GBLENDER_SHADE_COUNT; nn++ )
  {
    unsigned int  a = 255 * nn / ( GBLENDER_SHADE_COUNT - 1 );
    unsigned int  r;


    r = ( r2 * a + r1 * ( 255 - a ) + 127 ) / 255;

    r  = gamma_ramp_inv[r];

    gr[0] = (unsigned char)r;
    gr++;
  }
}


GBLENDER_APIDEF( unsigned char* )
gblender_lookup_channel( GBlender      blender,
                         unsigned int  background,
                         unsigned int  foreground )
{
  int              idx, idx0;
  unsigned short   backfore = (unsigned short)((foreground << 8) | background);
  GBlenderChanKey  key;

#ifdef GBLENDER_STATS
  blender->stat_hits--;
  blender->stat_lookups++;
#endif

#if 0
  if ( !blender->channels )
  {
    /* set to normal mode */
    blender->channels = 1;
    gblender_reset( blender );
  }
#endif

  idx0 = ( background + foreground*17 ) & (GBLENDER_KEY_COUNT-1);
  idx  = idx0;
  do
  {
    key = (GBlenderChanKey)blender->keys + idx;

    if ( key->index < 0 )
      goto NewNode;

    if ( key->backfore == backfore )
      goto Exit;

    idx = (idx+1) & (GBLENDER_KEY_COUNT-1);
  }
  while ( idx != idx0 );

 /* the cache is full, clear it completely
  */
#ifdef GBLENDER_STATS
  blender->stat_clears++;
#endif
  gblender_clear( blender );

NewNode:
  key->backfore   = backfore;
  key->index      = (signed short)( idx * GBLENDER_SHADE_COUNT );

  gblender_reset_channel_key( blender, key );

#ifdef GBLENDER_STATS
  blender->stat_keys++;
#endif

Exit:
  return  (unsigned char*)blender->cells + key->index;
}



#ifdef GBLENDER_STATS
#include <stdio.h>
GBLENDER_APIDEF( void )
gblender_dump_stats( GBlender  blender )
{
  printf( "hits = %ld, miss1 = %ld, miss2 = %ld, rate1=%.2f%%, rate2=%.2f%%\n",
           blender->stat_hits,
           blender->stat_lookups,
           blender->stat_keys,
           (100.0*blender->stat_hits) / (double)(blender->stat_hits + blender->stat_lookups),
           (100.0*blender->stat_lookups) / (double)( blender->stat_lookups + blender->stat_keys)
           );
}
#endif
