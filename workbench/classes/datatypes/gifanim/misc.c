
/*
**
**  $VER: misc.c 2.4 (13.4.98)
**  gifanim.datatype 2.4
**
**  Misc functions
**
**  Written 1997/1998 by Roland 'Gizzy' Mainz
**  Original example source from David N. Junod
**
*/

#ifndef DEBUG
#   define DEBUG 0
#endif
#include <aros/debug.h>

struct MyStackSwapStruct;
struct GIFAnimInstData;
struct GIFEncoder;

/* main includes */
#include "classbase.h"

/*****************************************************************************/

#if !defined(__AROS__)
/* clib sprintf replacement */
void mysprintf( struct ClassBase *cb, STRPTR buffer, STRPTR fmt, ... )
{
    APTR args;

    args = (APTR)((&fmt) + 1);

    RawDoFmt( fmt, args, (void (*))"\x16\xc0\x4e\x75", buffer );
}
#endif

/*****************************************************************************/

/* translate IBM PC character set to ISO Latin1. Limitted to 7 bit chars, except some german specific */
void IBMPC2ISOLatin1( STRPTR ibmpc, STRPTR isolatin1 )
{
    /* the following must be unsigned */
    register UBYTE *from = (UBYTE *)ibmpc,
                   *to   = (UBYTE *)isolatin1;

    if( from && to )
    {
      do
      {
        register UBYTE ch = *from;

        switch( ch )
        {
          case 132: *(to++) = 'ä'; break;
          case 148: *(to++) = 'ö'; break;
          case 129: *(to++) = 'ü'; break;
          case 142: *(to++) = 'Ä'; break;
          case 153: *(to++) = 'Ö'; break;
          case 154: *(to++) = 'Ü'; break;
          case 225: *(to++) = 'ß'; break;
          case   9:                       /* tab */
          case  10: break;                /* ignore LF to filter CTRL/LF-sequences */
          default:
          {
              if( ch < 128U )
              {
                *to++ = ch;
              }
              else
              {
                *to++ = '_'; /* can't convert */
              }
          }
              break;
        }
      } while( *from++ );
    }
}


/*****************************************************************************/

/* copy a given IFF CMAP chunk into a picture.datatype or animation.datatype object */
BOOL CMAP2Object( struct ClassBase *cb, Object *o, UBYTE *rgb, ULONG rgbsize )
{
    struct ColorRegister *acm = NULL;
    ULONG                *acregs = NULL;
    IPTR                 nc;

    D(bug("[gifanim.datatype]: %s()\n", __func__));

    /* file has this many colors (e.g. each color has one byte per R,B,G-gun) */
    nc = rgbsize / 3UL;

    SetDTAttrs( o, NULL, NULL, ADTA_NumColors, nc, TAG_DONE );

    /* Get color context */
    if( GetDTAttrs( o,
                    ADTA_ColorRegisters, (&acm),
                    ADTA_CRegs,          (&acregs),
                    ADTA_NumColors,      (&nc),
                    TAG_DONE ) == 3 )
    {
        D(bug("[gifanim.datatype] %s: got cmap attribs\n", __func__));
        D(bug("[gifanim.datatype] %s: colorregisters @ 0x%p, cregs @ 0x%p for %d colors\n", __func__, acm, acregs, nc));

      /* All valid ? */
      if( acm && acregs && nc )
      {
        ULONG i;

        for( i = 0UL ; i < nc ; i++, acm++ )
        {
          acm -> red   =  *rgb++;
          acm -> green =  *rgb++;
          acm -> blue  =  *rgb++;

          /* Replicate the color information.
           * This surrounds an OS bug which uses the low-order bytes of the 32-bit colors
           * instead of the high order ones
           */
          acregs[ ((i * 3) + 0) ] = AROS_BE2LONG(((ULONG)(acm -> red))   * 0x01010101UL);
          acregs[ ((i * 3) + 1) ] = AROS_BE2LONG(((ULONG)(acm -> green)) * 0x01010101UL);
          acregs[ ((i * 3) + 2) ] = AROS_BE2LONG(((ULONG)(acm -> blue))  * 0x01010101UL);
        }

        return( TRUE );
      }
    }

    return( FALSE );
}


/* Create a ColorMap from a given IFF CMAP chunk */
struct ColorMap *CMAP2ColorMap( struct ClassBase *cb, ULONG anumcolors, UBYTE *rgb, ULONG rgbsize )
{
    struct ColorMap *cm;
    ULONG            a_nc   = anumcolors;     /* Number of colors in animation */
    ULONG            rgb_nc = rgbsize / 3UL;  /* Number of colors in CMAP      */

    D(bug("[gifanim.datatype]: %s()\n", __func__));

    /* Get a colormap which hold all colors */
    if ((cm = GetColorMap( (long)MAX( a_nc, rgb_nc ) )) != NULL)
    {
      ULONG i,
            r, g, b;

      for( i = 0UL ; i < rgb_nc ; i++ )
      {
        r = *rgb++;
        g = *rgb++;
        b = *rgb++;

        /* Replicate color information (see CMAP2Object for details) and store them into colormap */
        SetRGB32CM( cm, i, r * 0x01010101UL, g * 0x01010101UL, b * 0x01010101UL);
      }

      /* BUG: the remaining entries should be filled with colors from the last colormap */
      for( ; i < a_nc ; i++ )
      {
        SetRGB32CM( cm, i, 0UL, 0UL, 0UL ); /* fill remaining entries with black */
      }
    }

    return( cm );
}


/* Clone a colormap */
struct ColorMap *CopyColorMap( struct ClassBase *cb, struct ColorMap *src )
{
    struct ColorMap *dest = NULL;

    D(bug("[gifanim.datatype]: %s()\n", __func__));

    if( src )
    {
      ULONG *ctable;

      if ((ctable = (ULONG *)AllocVec( ((ULONG)(src -> Count) * sizeof( ULONG ) * 3UL), MEMF_PUBLIC )) != NULL)
      {
        if ((dest = GetColorMap( (long)(src -> Count) )) != NULL)
        {
          ULONG i;

          GetRGB32( src, 0UL, (ULONG)(src -> Count), ctable );

          for( i = 0UL ; i < (src -> Count) ; i++ )
          {
            SetRGB32CM( dest, i, ctable[ ((i * 3) + 0) ], ctable[ ((i * 3) + 1) ], ctable[ ((i * 3) + 2) ]);
          }
        }

        FreeVec( ctable );
      }
    }

    return( dest );
}


/*****************************************************************************/

/* write a chunkypixel array into a truecolor bitmap using a given palette (requires CyberGFX) */
void WriteRGBPixelArray8( struct ClassBase *cb, struct BitMap *bm, ULONG animwidth, ULONG animheight, struct ColorRegister *cm, UBYTE *chunky )
{
             struct RastPort rp = { 0 };
    register ULONG           x,
                             y;

    D(bug("[gifanim.datatype]: %s()\n", __func__));

    /* Set up the temp. rastport */
    InitRastPort( (&rp) );
    rp . BitMap = bm;

    for( y = 0U ; y < animheight ; y++ )
    {
      for( x = 0U ; x < animwidth ; x++ )
      {
        register struct ColorRegister *cr = (&cm[ *chunky++ ]);

        /* The hack way: WriteRGBPPixel ignores the top byte (alpha channel) if the destination
         * has no alpha channel. Therefore we use here the struct ColorRegister entry directly by
         * moving the cr pointer one byte back and de-reference it to get the requested ULONG.
         * Saves some SHIFT and OR operations...
         */
        WriteRGBPixel( (&rp), (UWORD)x, (UWORD)y, *((ULONG *)(((UBYTE *)cr) - 1)) );
      }
    }
}


/*****************************************************************************/

/* from animation.datatype V41.5 */
static
void XCopyMem( struct ClassBase *cb, APTR src, APTR dest, ULONG size )
{
    /* Check if we can use the optimized CopyMemQuick */
    if( (ALIGN_LONG( src ) == src) && (ALIGN_LONG( dest ) == dest) )
    {
      register ULONG lsize = size & ~3UL,
                     cut   = size - lsize; /* remaining bytes (0-3) */

      CopyMemQuick( src, dest, lsize );

      if( cut )
      {
        src  = ((UBYTE *)src)  + lsize;
        dest = ((UBYTE *)dest) + lsize;

        CopyMem( src, dest, cut );
      }
    }
    else
    {
      CopyMem( src, dest, size );
    }
}


/* from animation.datatype V41.5 */
/* Copy bm1 to bm2, planar version (interleaved and non-interleaved) */
static
void CopyBitMapPlanar( struct ClassBase *cb, struct BitMap *bm1, struct BitMap *bm2, ULONG widthbpr )
{
    ULONG  bpr1 = bm1 -> BytesPerRow;
    ULONG  bpr2 = bm2 -> BytesPerRow;

    D(bug("[gifanim.datatype]: %s()\n", __func__));

    /* Same bitmap layout ? */
    if( bpr1 == bpr2 )
    {
      /* Interleaved BitMap ? */
      if( ((bm1 -> Planes[ 1 ]) - (bm1 -> Planes[ 0 ])) == (bpr1 / (ULONG)(bm1 -> Depth)) )
      {
        ULONG planesize = bpr2 * (ULONG)(bm2 -> Rows);

        XCopyMem( cb, (bm1 -> Planes[ 0 ]), (bm2 -> Planes[ 0 ]), planesize );
      }
      else
      {
        ULONG planesize = bpr2 * (ULONG)(bm2 -> Rows);
        UWORD i;

        for( i = 0U ; i < (bm2 -> Depth) ; i++ )
        {
          XCopyMem( cb, (bm1 -> Planes[ i ]), (bm2 -> Planes[ i ]), planesize );
        }
      }
    }
    else
    {
      register UBYTE *src;
      register UBYTE *dst;
      register LONG   r;
      register LONG   p;

      for( p = bm1 -> Depth - 1 ; p >= 0 ; p-- )
      {
        src = (BYTE *)bm1 -> Planes[ p ];
        dst = (BYTE *)bm2 -> Planes[ p ];

        for( r = bm1 -> Rows - 1 ; r >= 0 ; r-- )
        {
          CopyMem( src, dst, widthbpr );
          src += bpr1;
          dst += bpr2;
        }
      }
    }
}


/* from animation.datatype V41.5 */
/* Copy bm1 to bm2, system/CyberGFX function*/
static
void CopyBitMapSystem( struct ClassBase *cb, struct BitMap *bm1, struct BitMap *bm2, ULONG width, ULONG height )
{
    D(bug("[gifanim.datatype]: %s()\n", __func__));

    /* Assumption: If a non-planar bitmap occurs BltBitMap should be able
     * to blit it into a planar one
     */
    BltBitMap( bm1, 0L, 0L, bm2, 0L, 0L, width, height, 0xC0UL, 0xFFUL, NULL );

    WaitBlit();
}


void CopyBitMap( struct ClassBase *cb, struct BitMap *dest, struct BitMap *src, ULONG width, ULONG height )
{
    D(bug("[gifanim.datatype]: %s()\n", __func__));

    if( dest && src )
    {
      if( CyberGfxBase )
      {
        CopyBitMapSystem( cb, src, dest, width, height );
      }
      else
      {
        CopyBitMapPlanar( cb, src, dest, (width / 8UL) );
      }
    }
}

/*****************************************************************************/
#if !defined(__AROS__)
/* allocate a piece of memory from an exec memory pool and track the allocation size */
APTR AllocPooledVec( struct ClassBase *cb, APTR pool, ULONG memsize )
{
    IPTR *memory = NULL;

    if( pool && memsize )
    {
      memsize += (ULONG)sizeof( IPTR );

      if ((memory = (IPTR *)AllocPooled( pool, memsize ) ) != NULL)
      {
        (*memory) = memsize;

        memory++;
      }
    }

    return( (APTR)memory );
}

void FreePooledVec( struct ClassBase *cb, APTR pool, APTR mem )
{
    if( pool && mem )
    {
      IPTR *memory;

      memory = (IPTR *)mem;

      memory--;

      FreePooled( pool, memory, (*memory) );
    }
}
#endif

/*****************************************************************************/


