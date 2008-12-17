#include "gblender.h"
#include <stdlib.h>
#include <math.h>

static void
gblender_set_gamma_table( double           gamma_value,
                          unsigned short*  gamma_ramp,
                          unsigned char*   gamma_ramp_inv )
{
  int    gmax      = (256 << GBLENDER_GAMMA_SHIFT)-1;

  if ( gamma_value <= 0 )  /* special case for sRGB */
  {
    int  ii;

    for ( ii = 0; ii < 256; ii++ )
    {
      double  x = (double)ii / 255.0;

      if ( x <= 0.03926 )
        x = x/12.92;
      else
        x = pow( (x+0.055)/ 1.055, 2.4 );

      gamma_ramp[ii] = (unsigned short)(gmax*x);
    }

    for ( ii = 0; ii < gmax; ii++ )
    {
      double  x = (double)ii / gmax;

      if ( x <= 0.00304 )
        x = 12.92*x;
      else
        x = 1.055*pow(x,1/2.4) - 0.055;

      gamma_ramp_inv[ii] = (unsigned char)(255*x);
    }
    gamma_ramp_inv[gmax] = 255;
  }
  else
  {
    int    ii;
    double gamma_inv = 1.0f / gamma_value;

    /* voltage to linear */
    for ( ii = 0; ii < 256; ii++ )
      gamma_ramp[ii] = (unsigned short)( pow( (double)ii/255.0f, gamma_value )*gmax );

    /* linear to voltage */
    for ( ii = 0; ii < gmax; ii++ )
      gamma_ramp_inv[ii] = (unsigned char)( pow( (double)ii/gmax, gamma_inv ) * 255.0f );

    gamma_ramp_inv[gmax] = 255;
  }
}


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

    blender->cache_r_back  = -1;
    blender->cache_r_fore  = -1;
    blender->cache_r_cells = NULL;

    blender->cache_g_back  = -1;
    blender->cache_g_fore  = -1;
    blender->cache_g_cells = NULL;

    blender->cache_b_back  = -1;
    blender->cache_b_fore  = -1;
    blender->cache_g_cells = NULL;
  }
  else
  {
    for ( nn = 0; nn < GBLENDER_KEY_COUNT; nn++ )
      keys[nn].cells = NULL;

    blender->cache_back  = -1;
    blender->cache_fore  = -1;
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
  int            nn;
  int            gmax = (256 << GBLENDER_GAMMA_SHIFT)-1;

  const unsigned char*   gamma_ramp_inv = blender->gamma_ramp_inv;
  const unsigned short*  gamma_ramp     = blender->gamma_ramp;

  int  r1,g1,b1,r2,g2,b2;

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
    int  bits = 8;
    int  a    = 0;
    int  r, g, b;

    while ( bits >= GBLENDER_SHADE_BITS )
    {
      bits -= GBLENDER_SHADE_BITS;
      a    += (nn << bits);
    }
    if ( bits > 0 )
    {
      bits = GBLENDER_SHADE_BITS - bits;
      a   += (nn >> bits);
    }

    r = ((r2-r1)*a + 128);
    g = ((g2-g1)*a + 128);
    b = ((g2-g1)*a + 128);

    r = (r + (r >> 8)) >> 8;
    g = (g + (g >> 8)) >> 8;
    b = (b + (b >> 8)) >> 8;

    r += r1;
    g += g1;
    b += b1;

#if 0
    r = ( r | -(r >> 8) ) & 255;
    g = ( g | -(g >> 8) ) & 255;
    b = ( b | -(b >> 8) ) & 255;
#else
   if ( r < 0 ) r = 0; else if ( r > gmax ) r = gmax;
   if ( g < 0 ) g = 0; else if ( g > gmax ) g = gmax;
   if ( b < 0 ) b = 0; else if ( b > gmax ) b = gmax;
#endif

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

  idx0 = ( background + foreground*63 ) % GBLENDER_KEY_COUNT;
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
  gblender_clear( blender );
#endif

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
  int            back = key->backfore & 255;
  int            fore = (key->backfore >> 8) & 255;
  unsigned char* gr   = (unsigned char*)blender->cells + key->index;
  int            nn;

  const unsigned char*   gamma_ramp_inv = blender->gamma_ramp_inv;
  const unsigned short*  gamma_ramp     = blender->gamma_ramp;

  int  r1,r2;
  int  gmax = (256 << GBLENDER_GAMMA_SHIFT)-1;

  r1    = back;
  r2    = fore;

  gr[0] = r1;
  gr++;


  r1 = gamma_ramp[r1];
  r2 = gamma_ramp[r2];

  for ( nn = 1; nn < GBLENDER_SHADE_COUNT; nn++ )
  {
    int  bits = 8;
    int  a    = 0;
    int  r;

    while ( bits >= GBLENDER_SHADE_BITS )
    {
      a    += (nn << (bits - GBLENDER_SHADE_BITS));
      bits -= GBLENDER_SHADE_BITS;
    }
    if ( bits > 0 )
      a += (nn >> (GBLENDER_SHADE_BITS-bits));

    r = ((r2-r1)*a + 128);
    r = (r + (r >> 8)) >> 8;
    r += r1;
    if ( r < 0 ) r = 0; else if ( r > gmax ) r = gmax;
    r  = gamma_ramp_inv[r];

    gr[0] = (unsigned char)r;
    gr++;
  }
}


GBLENDER_APIDEF( unsigned char* )
gblender_lookup_channel( GBlender       blender,
                         int            background,
                         int            foreground )
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

  idx0 = ( background + foreground*17 ) % (GBLENDER_KEY_COUNT);
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
  gblender_clear( blender );
#endif

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
