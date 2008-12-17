/****************************************************************************
 *
 *  Gamma-correct alpha blending of text
 *
 *  (C) 2004 David Turner
 *
 */

#ifndef __GBLENDER_H__
#define __GBLENDER_H__

#ifndef GBLENDER_API
#define GBLENDER_API(x)  extern x
#endif

#ifndef GBLENDER_APIDEF
#define GBLENDER_APIDEF(x)   x
#endif

#define  GBLENDER_SHADE_BITS      4   /* must be <= 7 !! */
#define  GBLENDER_SHADE_COUNT     ( 1 << GBLENDER_SHADE_BITS )
#define  GBLENDER_SHADE_INDEX(n)  ((n) >> (8-GBLENDER_SHADE_BITS))
#define  GBLENDER_KEY_COUNT       256  /* must be a power of 2 */
#define  GBLENDER_GAMMA_SHIFT     2

#define  xGBLENDER_STORE_BYTES  /* define this to store (R,G,B) values on 3
                                * bytes, instead of a single 32-bit integer.
                                * surprisingly, this can speed up
                                * the blender on certain machines.
                                * Go figure what's really happening though :-)
                                */

#define  xGBLENDER_STATS        /* define this to collect statistics in the
                                * blender
                                */

  typedef unsigned int    GBlenderPixel;  /* needs 32-bits here !! */

#ifdef GBLENDER_STORE_BYTES
  typedef unsigned char   GBlenderCell;
# define  GBLENDER_CELL_SIZE    3
#else
  typedef GBlenderPixel   GBlenderCell;
# define GBLENDER_CELL_SIZE     1
#endif


  typedef struct
  {
    GBlenderPixel  background;
    GBlenderPixel  foreground;
    GBlenderCell*  cells;

  } GBlenderKeyRec, *GBlenderKey;


  typedef struct
  {
    unsigned short  backfore;  /* (fore << 8) | back               */
    signed short    index;     /* offset in (unsigned char*)cells  */

  } GBlenderChanKeyRec, *GBlenderChanKey;


  typedef struct GBlenderRec_
  {
    GBlenderKeyRec        keys [ GBLENDER_KEY_COUNT ];
    GBlenderCell          cells[ GBLENDER_KEY_COUNT*GBLENDER_SHADE_COUNT*GBLENDER_CELL_SIZE ];

   /* a small cache for normal modes
    */
    GBlenderPixel         cache_back;
    GBlenderPixel         cache_fore;
    GBlenderCell*         cache_cells;

   /* a small cache for RGB channels modes
    */
    int                   cache_r_back;
    int                   cache_r_fore;
    unsigned char*        cache_r_cells;

    int                   cache_g_back;
    int                   cache_g_fore;
    unsigned char*        cache_g_cells;

    int                   cache_b_back;
    int                   cache_b_fore;
    unsigned char*        cache_b_cells;

   /* are we in color or channel mode ?
    */
    int                   channels;

   /* the gamma table
    */
    unsigned short        gamma_ramp[256];                              /* voltage to linear */
    unsigned char         gamma_ramp_inv[256 << GBLENDER_GAMMA_SHIFT];  /* linear to voltage */

#ifdef GBLENDER_STATS
    long                  stat_hits;    /* number of direct hits             */
    long                  stat_lookups; /* number of table lookups           */
    long                  stat_keys;    /* number of table key recomputation */
    long                  stat_clears;  /* number of table clears            */
#endif

  } GBlenderRec, *GBlender;


 /* initialize with a given gamma */
  GBLENDER_API( void )
  gblender_init( GBlender  blender,
                 double    gamma );


 /* clear blender, and reset stats */
  GBLENDER_API( void )
  gblender_reset( GBlender  blender );


  GBLENDER_API( void )
  gblender_use_channels( GBlender  blender,
                         int       channels );

 /* lookup a cell range for a given (background,foreground) pair
  */
  GBLENDER_API( GBlenderCell* )
  gblender_lookup( GBlender       blender,
                   GBlenderPixel  background,
                   GBlenderPixel  foreground );

  GBLENDER_API( unsigned char* )
  gblender_lookup_channel( GBlender   blender,
                           int        background,
                           int        foreground );

#ifdef GBLENDER_STATS
  GBLENDER_API( void )
  gblender_dump_stats( GBlender  blender );
#else
# define gblender_dump_stats(b)  do { } while (0);
#endif

#ifdef GBLENDER_STATS
#define GBLENDER_STAT_HIT(gb)   (gb)->stat_hits++
#else
#define GBLENDER_STAT_HIT(gb)   /* nothing */
#endif


  /* no final `;'! */
#define  GBLENDER_VARS(_gb,_fore)                                                                                               \
   GBlenderPixel    _gback  = (_gb)->cache_back;                                                                                \
   GBlenderCell*    _gcells = ( (_fore) == (_gb)->cache_fore ? (_gb)->cache_cells : gblender_lookup( (_gb), _gback, _fore ) );  \
   GBlenderPixel    _gfore  = (_fore)

#define  GBLENDER_LOOKUP(gb,back)                        \
   GBLENDER_STAT_HIT(gb);                                \
   if ( _gback != (GBlenderPixel)(back) )                \
   {                                                     \
     _gback  = (GBlenderPixel)(back);                    \
     _gcells = gblender_lookup( (gb), _gback, _gfore );  \
   }

#define  GBLENDER_CLOSE(_gb)     \
  (_gb)->cache_back  = _gback;   \
  (_gb)->cache_fore  = _gfore;   \
  (_gb)->cache_cells = _gcells;



  /* no final `;'! */
#define  GBLENDER_CHANNEL_VARS(_gb,_rfore,_gfore,_bfore)                                                                                         \
   int              _grback  = (_gb)->cache_r_back;                                                                                              \
   unsigned char*   _grcells = ( (_rfore) == (_gb)->cache_r_fore ? (_gb)->cache_r_cells : gblender_lookup_channel( (_gb), _grback, _rfore ));    \
   int              _grfore  = (_rfore);                                                                                                         \
   int              _ggback  = (_gb)->cache_g_back;                                                                                              \
   unsigned char*   _ggcells = ( (_gfore) == (_gb)->cache_g_fore ? (_gb)->cache_g_cells : gblender_lookup_channel( (_gb), _ggback, _gfore ));    \
   int              _ggfore  = (_rfore);                                                                                                         \
   int              _gbback  = (_gb)->cache_b_back;                                                                                              \
   unsigned char*   _gbcells = ( (_bfore) == (_gb)->cache_b_fore ? (_gb)->cache_b_cells : gblender_lookup_channel( (_gb), _gbback, _bfore ));    \
   int              _gbfore  = (_bfore)

#define  GBLENDER_CHANNEL_CLOSE(_gb)   \
  (_gb)->cache_r_back  = _grback;      \
  (_gb)->cache_r_fore  = _grfore;      \
  (_gb)->cache_r_cells = _grcells;     \
  (_gb)->cache_g_back  = _ggback;      \
  (_gb)->cache_g_fore  = _ggfore;      \
  (_gb)->cache_g_cells = _ggcells;     \
  (_gb)->cache_b_back  = _gbback;      \
  (_gb)->cache_b_fore  = _gbfore;      \
  (_gb)->cache_b_cells = _gbcells;


#define  GBLENDER_LOOKUP_R(gb,back)                                 \
   GBLENDER_STAT_HIT(gb);                                           \
   if ( _grback != (int)(back) )                                    \
   {                                                                \
     _grback  = (GBlenderPixel)(back);                              \
     _grcells = gblender_lookup_channel( (gb), _grback, _grfore );  \
   }

#define  GBLENDER_LOOKUP_G(gb,back)                                 \
   GBLENDER_STAT_HIT(gb);                                           \
   if ( _ggback != (int)(back) )                                    \
   {                                                                \
     _ggback  = (GBlenderPixel)(back);                              \
     _ggcells = gblender_lookup_channel( (gb), _ggback, _ggfore );  \
   }

#define  GBLENDER_LOOKUP_B(gb,back)                                 \
   GBLENDER_STAT_HIT(gb);                                           \
   if ( _gbback != (int)(back) )                                    \
   {                                                                \
     _gbback  = (GBlenderPixel)(back);                              \
     _gbcells = gblender_lookup_channel( (gb), _gbback, _gbfore );  \
   }


#endif /* __GBENCH_CACHE_H__ */
