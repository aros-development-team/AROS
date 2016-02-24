

/*
**
**  $VER: mpegamiga.c 1.11 (2.11.97)
**  mpegvideo.datatype 1.11
**
**  amiga support functions
**
**  Written 1996/1997 by Roland 'Gizzy' Mainz
**
*/

#ifndef DEBUG
#   define DEBUG 0
#endif
#include <aros/debug.h>

/* project includes */
#include "mpegmyassert.h"
#include "mpegproto.h"

/* ansi includes */
#include <math.h>
#include <limits.h>

/* local prototypes */
static void InitStoreFrame( struct MPEGVideoInstData *mvid );
static void WritePixelArray8Fast( struct BitMap *, UBYTE * );


void mpeg_closedown( struct MPEGVideoInstData *mvid )
{
    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    if( mvid -> mvid_MPPool )
    {
      DeletePool( (mvid -> mvid_MPPool) );
      mvid -> mvid_MPPool = NULL;
    }
}


static
void InitStoreFrame( struct MPEGVideoInstData *mvid )
{
    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    /* Alloc bitmap as key bitmap */
    if( mvid -> mvid_UseChunkyMap )
    {
      mvid -> mvid_KeyBitMap = AllocBitMap( (ULONG)anim_width, (ULONG)anim_height, (ULONG)anim_depth, (BMF_CLEAR | BMF_SPECIALFMT | SHIFT_PIXFMT( pixfmt )), NULL );
    }
    else
    {
      mvid -> mvid_KeyBitMap = AllocBitMap( (ULONG)anim_width, (ULONG)anim_height, (ULONG)anim_depth, (BMF_CLEAR | BMF_MINPLANES), NULL );
    }

    if( (mvid -> mvid_KeyBitMap) == NULL )
    {
      myexit( mvid, RETURN_FAIL, ERROR_NO_FREE_STORE );
    }
}


void StoreFrame( struct MPEGVideoInstData *mvid, UBYTE *data )
{
    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    /* All ready to store the frames ? */
    if( (mvid -> mvid_KeyBitMap) == NULL )
    {
      InitStoreFrame( mvid );
    }

    switch( ditherType )
    {
#if !defined(__AROS__)
      case HAM_DITHER:
      {
          ULONG *src = (ULONG *)data;
          UBYTE *dest,
                *tmp;

          ULONG  height = anim_height;

          dest = tmp = (UBYTE *)mymalloc( mvid, (size_t)((anim_width + 15UL) * anim_height) );

          while( height-- )
          {
            UBYTE *xrgb = (UBYTE *)src;

            switch( anim_depth )
            {
              case 8:
                  CreateHAM8Line( (xrgb + 1), (xrgb + 2), (xrgb + 3), dest, sizeof( ULONG ), anim_width );
                  break;

              case 6:
                  CreateHAM6Line( (xrgb + 1), (xrgb + 2), (xrgb + 3), dest, sizeof( ULONG ), anim_width );
                  break;
            }

            dest += anim_width;
            src  += anim_width;
          }

          AddFrame( mvid, tmp, NULL );

          myfree( mvid, tmp );
      }
          break;
#endif
      case FULL_COLOR_DITHER:
      case FULL_COLOR_DITHER16:
      {
          AddFrame( mvid, data, NULL );
      }
          break;

      default:
      {
          struct ColorMap *cm = NULL;

          if( mvid -> mvid_PalettePerFrame )
          {
            if ((cm = GetColorMap( (1UL << anim_depth) ) ) != NULL)
            {
              ULONG i,
                    r, g, b;

              /* Copy colors into colormap... */
              for( i = 0UL ; i < used_cnt ; i++ )
              {
                /* Bump colors from 8 to 32 bits-per-gun */
                r = ((ULONG)used_colors[ i ] . red)   * 0x01010101UL;
                g = ((ULONG)used_colors[ i ] . green) * 0x01010101UL;
                b = ((ULONG)used_colors[ i ] . blue)  * 0x01010101UL;

                SetRGB32CM( cm, i, r, g, b );
              }

              /* Fill remaining colors with "black" */
              for( ; i < (1UL << anim_depth) ; i++ )
              {
                SetRGB32CM( cm, i, 0UL, 0UL, 0UL );
              }
            }
          }

          AddFrame( mvid, data, cm );
      }
          break;
    }
}


void AddFrame( struct MPEGVideoInstData *mvid, UBYTE *data, struct ColorMap *cm )
{
    ULONG             timestamp = totNumFrames++; /* timestamp of this frame */
    struct FrameNode *fn;

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    /* Image ? */
    if( data )
    {
      BOOL created = FALSE;

      /* Does we have already a frame with this timestamp ? ... */
      if ((fn = FindFrameNode( (&(mvid -> mvid_FrameList)), timestamp ) ) != NULL)
      {
        /* ... really ? */
        if( (fn -> fn_TimeStamp) != timestamp )
        {
          fn = NULL;
        }
      }

      if( fn == NULL )
      {
        if( mvid -> mvid_IndexScan )
        {
          created = TRUE;

          fn = AllocFrameNode( classbase, (mvid -> mvid_Pool) );

          if( fn && (data != (UBYTE *)~0UL) )
          {
            /* Create a locked frame here if new HAD to create the fn and if we have data.
             * This is TRUE for the keyframe and/or of the mvid_LoadAll flag is TRUE
             */
            fn -> fn_IsKeyFrame = TRUE; /* lock this bitmap that ADTM_UNLOADFRAME won't free it */
          }
        }
        else
        {
          myexit( mvid, RETURN_ERROR, ERROR_INVALID_LOCK );
        }
      }

      if( fn )
      {
        if( created )
        {
          /* Store frame count */
          fn -> fn_TimeStamp =
            fn -> fn_Frame   = timestamp;
          fn -> fn_BMOffset  = mvid -> mvid_Last_PIC_SC_Pos;
          fn -> fn_IFrame    = mvid -> mvid_LastIFrameNode;

          AddTail( (struct List *)(&(mvid -> mvid_FrameList)), (struct Node *)(&(fn -> fn_Node)) );
        }

        /* Does someone wants only to create an empty framenode here (e.g. data == (UBYTE *)~0UL) ? */
        if( data != (UBYTE *)~0UL )
        {
          if( (fn -> fn_BitMap) == NULL )
          {
            if ((fn -> fn_BitMap = AllocFrameBitMap( mvid ) ) != NULL)
            {
              switch( ditherType )
              {
                case HAM_DITHER:
                case GRAY_DITHER:
#if 0
                case FAST_COLOR_DITHER:
#endif
                case ORDERED_DITHER:
                {
                    WritePixelArray8Fast( (fn -> fn_BitMap), data );

                    /* Store colormap */
                    fn -> fn_CMap = cm;
                }
                    break;

                case FULL_COLOR_DITHER:
                case FULL_COLOR_DITHER16:
                {
                    if( mvid -> mvid_UseChunkyMap )
                    {
                      APTR   handle;
                      APTR   plane  = NULL; /* be safe ! (CyberGFX has some problems with tag parsing...) */
                      ULONG  bpr    = 0UL,  /* be safe ! (CyberGFX has some problems with tag parsing...) */
                             height = 0UL;  /* be safe ! (CyberGFX has some problems with tag parsing...) */

                      if( ( handle = LockBitMapTags( (fn -> fn_BitMap),
                                                   LBMI_BASEADDRESS, (&plane),
                                                   LBMI_BYTESPERROW, (&bpr),
                                                   LBMI_HEIGHT,      (&height),
                                                   TAG_DONE ) ) )
                      {
                        CopyMem( data, plane, (bpr * height) );

                        UnLockBitMap( handle );
                      }
                      else
                      {
                        error_printf( mvid, "LockBitMapTags failed for bitmap @ 0x%p\n", fn->fn_BitMap);
                      }
                    }
                    else
                    {
                      if( anim_depth == 24UL )
                      {
                        struct BitMap red,
                                      green,
                                      blue;
                        ULONG         i;
                        UBYTE        *tmp,
                                     *tmp_run;
                        ULONG         size = anim_width * anim_height;

                        InitBitMap( (&red),   8, (ULONG)anim_width, (ULONG)anim_height );
                        InitBitMap( (&green), 8, (ULONG)anim_width, (ULONG)anim_height );
                        InitBitMap( (&blue),  8, (ULONG)anim_width, (ULONG)anim_height );

                        for( i = 0UL ; i < 24UL ; i++ )
                        {
                          if( i < 8  ) red   . Planes[ i      ] = fn -> fn_BitMap -> Planes[ i ]; else
                          if( i < 16 ) green . Planes[ i -  8 ] = fn -> fn_BitMap -> Planes[ i ]; else
                                       blue  . Planes[ i - 16 ] = fn -> fn_BitMap -> Planes[ i ];
                        }

                        /* Alloc temp memory for XRGB to RRR..., GGG..., BBB... array conversion */
                        tmp = mymalloc( mvid, (size_t)(size + 15UL) );

                        for( i = 0 ; i < 3 ; i++ )
                        {
                          ULONG j,
                                j_size = (size * 4UL) + i;

                          for( j = i, tmp_run = tmp ; j < j_size ; j += 4UL )
                          {
                            *tmp_run++ = data[ j ];
                          }

                          switch( i )
                          {
                            case 0: WritePixelArray8Fast( (&red),   tmp ); break;
                            case 1: WritePixelArray8Fast( (&green), tmp ); break;
                            case 2: WritePixelArray8Fast( (&blue),  tmp ); break;
                          }
                        }

                        myfree( mvid, tmp );
                      }
                      else
                      {
                        error_printf( mvid, "%dbit planar direct-RGB unsupported\n", anim_depth);
                      }
                    }
                }
                    break;
              }
            }
            else
            {
              /* no bitmap */
              myexit( mvid, RETURN_ERROR, ERROR_NO_FREE_STORE );
            }
          }
        }
      }
      else
      {
        /* can't alloc struct FrameNode */
        myexit( mvid, RETURN_ERROR, ERROR_NO_FREE_STORE );
      }
    }
    else
    {
      if( mvid -> mvid_IndexScan )
      {
        /* Find the nearest frame */
        if ((fn = FindFrameNode( (&(mvid -> mvid_FrameList)), timestamp ) ) != NULL)
        {
          /* Bump the duration time of the predecessor (frame) */
          fn -> fn_Duration = timestamp - (fn -> fn_TimeStamp);
        }
      }
    }
}



/***************************************************************/

#if !defined(__AROS__)
static APTR  AllocPooledVec( struct MPEGVideoInstData *mvid, APTR, ULONG );
static void  FreePooledVec( struct MPEGVideoInstData *mvid, APTR, APTR );
#else
#define AllocPooledVec(mvid, pool, size) AllocVecPooled(pool, size)
#define FreePooledVec(mvid, pool, mem) FreeVecPooled(pool, mem)
#endif
static ULONG VecSize( APTR );

void *mymalloc( struct MPEGVideoInstData *mvid, size_t s )
{
    void *mem;

    if( (mvid -> mvid_MPPool) == NULL )
    {
      if( (mvid -> mvid_MPPool = CreatePool( (MEMF_PUBLIC | MEMF_CLEAR), 16384UL, 16384UL )) == NULL )
      {
        myexit( mvid, RETURN_FAIL, ERROR_NO_FREE_STORE );
      }
    }

    if( (mem = (void *)AllocPooledVec( mvid, (mvid -> mvid_MPPool), (ULONG)s )) == NULL )
    {
      myexit( mvid, RETURN_ERROR, ERROR_NO_FREE_STORE );
    }

    return( mem );
}


void myfree( struct MPEGVideoInstData *mvid, void *mem )
{
    FreePooledVec( mvid, (mvid -> mvid_MPPool), mem );
}


void *myrealloc( struct MPEGVideoInstData *mvid, void *oldmem, size_t newsize )
{
    ULONG oldsize = 0UL;
    APTR  newmem  = NULL;

    if( oldmem )
    {
      oldsize = VecSize( oldmem );
    }

    if( newsize == oldsize )
    {
      return( oldmem );
    }

    if( newsize )
    {
      if ((newmem = AllocPooledVec( mvid, (mvid -> mvid_MPPool), (ULONG)newsize ) ) != NULL)
      {
        if( oldsize )
        {
          memcpy( newmem, oldmem, (size_t)MIN( oldsize, newsize ) );
        }

        FreePooledVec( mvid, (mvid -> mvid_MPPool), oldmem );
      }
      else
      {
        myexit( mvid, RETURN_FAIL, ERROR_NO_FREE_STORE );
      }
    }

    return( newmem );
}

#if !defined(__AROS__)
static
APTR AllocPooledVec( struct MPEGVideoInstData *mvid, APTR pool, ULONG memsize )
{
    ULONG *memory = NULL;

    if( pool && memsize )
    {
      memsize += (ULONG)sizeof( ULONG );

      if( memory = (ULONG *)AllocPooled( pool, memsize ) )
      {
        (*memory) = memsize;
        memory++;
      }
    }
    else
    {
      error_printf( mvid, "AllocPooledVec: illegal args %lx %lx\n", pool, memsize );
    }

    return( (APTR)memory );
}


static
void FreePooledVec( struct MPEGVideoInstData *mvid, APTR pool, APTR mem )
{
    if( pool && mem )
    {
      ULONG *memory;

      memory = (ULONG *)mem;

      memory--;

      FreePooled( pool, memory, (*memory) );
    }
    else
    {
      error_printf( mvid, "FreePooledVec: illegal args %lx %lx\n", pool, mem );
    }
}
#endif

/* get size of an AllocPooledVec memory */
static
ULONG VecSize( APTR mem )
{
    return( *(((ULONG *)mem) - 1) );
}


ULONG SearchColor( struct MPEGVideoInstData *mvid, struct ColorRegister *colortable, ULONG *numcolors, ULONG maxcount, struct ColorRegister *color )
{
    ULONG i;
    LONG error, minerror      =
#if !defined(__AROS__)
        LONG_MAX;
#else
        INT_MAX;
#endif
    ULONG minerrorindex = 0UL;
    const ULONG nc            = *numcolors; /* short cut to (*numcolors) (read only) */
    WORD er, eg, eb;

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    /* Check for a color in the specified range */
    for( i = 0UL ; i < nc ; i++, colortable++ )
    {
      er = ABS( ((WORD)(colortable -> red)   - (WORD)(color -> red)) );
      eg = ABS( ((WORD)(colortable -> green) - (WORD)(color -> green)) );
      eb = ABS( ((WORD)(colortable -> blue)  - (WORD)(color -> blue)) );

      error = er + eg + eb;

      if( error < minerror )
      {
        if( error < (mvid -> mvid_ColorError) )
        {
          return( i );
        }

        minerror       = error;
        minerrorindex  = i;
      }
    }

    /* Any color entry free for this frame ? */
    if( nc < maxcount )
    {
      *colortable = *color;

      minerrorindex = nc;

      (*numcolors)++;
    }

    return( minerrorindex );
}


static
void WritePixelArray8Fast( struct BitMap *dest, UBYTE *source )
{
    ULONG *plane[ 8 ] = { 0 },
          *chunky = (ULONG *)source; /* fetch 32 bits per cycle */
    ULONG  i;
    ULONG  numcycles = ((dest -> Rows) * (dest -> BytesPerRow)) / sizeof( ULONG );

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    /* Copy plane ptrs */
    for( i = 0UL ; i < (dest -> Depth) ; i++ )
    {
      plane[ i ] = (ULONG *)(dest -> Planes[ i ]);
    }

    /* Fill unused planes with plane 0, which will be written last, all prevoius accesses
     * will be droped (assumes that a cache hides this "dummy" writes)
     */
    for( ; i < 8UL ; i++ )
    {
      plane[ i ] = (ULONG *)(dest -> Planes[ 0 ]);
    }

    /* Process bitmaps */
    for( i = 0UL ; i < numcycles ; i++ )
    {
      ULONG tmp,
            b0, b1, b2, b3, b4, b5, b6, b7;

      /* process 32 pixels */
      b0 = *chunky++;  b4 = *chunky++;
      b1 = *chunky++;  b5 = *chunky++;
      b2 = *chunky++;  b6 = *chunky++;
      b3 = *chunky++;  b7 = *chunky++;

#define merge( a, b, mask, shift ) \
      tmp = mask & (a ^ (b >> shift));   \
      a ^= tmp;                          \
      b ^= (tmp << shift)

      merge( b0, b2, 0x0000ffff, 16 );
      merge( b1, b3, 0x0000ffff, 16 );
      merge( b4, b6, 0x0000ffff, 16 );
      merge( b5, b7, 0x0000ffff, 16 );

      merge( b0, b1, 0x00ff00ff,  8 );
      merge( b2, b3, 0x00ff00ff,  8 );
      merge( b4, b5, 0x00ff00ff,  8 );
      merge( b6, b7, 0x00ff00ff,  8 );

      merge( b0, b4, 0x0f0f0f0f,  4 );
      merge( b1, b5, 0x0f0f0f0f,  4 );
      merge( b2, b6, 0x0f0f0f0f,  4 );
      merge( b3, b7, 0x0f0f0f0f,  4 );

      merge( b0, b2, 0x33333333,  2 );
      merge( b1, b3, 0x33333333,  2 );
      merge( b4, b6, 0x33333333,  2 );
      merge( b5, b7, 0x33333333,  2 );

      merge( b0, b1, 0x55555555,  1 );
      merge( b2, b3, 0x55555555,  1 );
      merge( b4, b5, 0x55555555,  1 );
      merge( b6, b7, 0x55555555,  1 );

      *plane[ 7 ]++ = b0;
      *plane[ 6 ]++ = b1;
      *plane[ 5 ]++ = b2;
      *plane[ 4 ]++ = b3;
      *plane[ 3 ]++ = b4;
      *plane[ 2 ]++ = b5;
      *plane[ 1 ]++ = b6;
      *plane[ 0 ]++ = b7;
    }
}


