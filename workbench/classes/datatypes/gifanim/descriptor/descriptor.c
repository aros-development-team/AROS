
/*
**
**  $VER: descriptor.c 2.3 (24.5.98)
**  gifanim.datatype 2.3
**
**  DataTypes custom comparision routine for a GIF Animation
**
**  Written 1997/1998 by Roland 'Gizzy' Mainz
**  Original example source from David N. Junod
**
*/

/* main includes */
#include "/classbase.h"
#include "/classdata.h"

/* we don't have our class library base yet, therefore we use the resources from the given DTHookContext */
#undef SysBase
#undef DOSBase
#undef UtilityBase
#define SysBase     (dthc -> dthc_SysBase)
#define UtilityBase (dthc -> dthc_UtilityBase)
#define DOSBase     (dthc -> dthc_DOSBase)

/*****************************************************************************/
/* debugging */
void kprintf( STRPTR, ... );

#if 0
#define D( x ) x
#else
#define D( x )
#endif

/*****************************************************************************/
/* misc. */

#define ReadOK( file, buffer, len ) (Read( (file), (buffer), (LONG)(len) ) == (LONG)(len))

/*****************************************************************************/


/* This custom comparisation code check if a GIF file has more than one image
 * It does NOT check the syntax/compression/invalid format/invalid contents, this
 * is the job of the class library !!
 */
DISPATCHERFLAGS
BOOL DTHook( REGA0 struct DTHookContext *dthc )
{
    BPTR  file;

    ULONG imagecount = 0UL;
/* In our definition, a gif animation is a gif stream with at least two frames... */
#define EnougthFramesToBeAGIFAnimation ((BOOL)(imagecount > 1UL))

    /* Make sure we have a file handle ! */
    if( file = dthc -> dthc_FileHandle )
    {
      UBYTE buf[ 16 ];
      UBYTE c;

      if( !ReadOK( file, buf, 6 ) )
      {
        D( kprintf( "error reading magic number" ) );
        return( FALSE );
      }

      /* Magic number check */
      if( strncmp( (char *)buf, "GIF", 3 ) != 0 )
      {
        D( kprintf( "not a GIF file" ) );
        return( FALSE );
      }

/* The version check has been disabled because this is the job of the class library */
#ifdef COMMENTED_OUT
      /* Version check */
      {
        UBYTE version[ 4 ];

        strncpy( version, (char *)(buf + 3), 3 );
        version[ 3 ] = '\0';

        if( strcmp( version, "87a" ) && strcmp( version, "89a" ) )
        {
          return( FALSE );
        }
      }
#endif /* COMMENTED_OUT */

      if( !ReadOK( file, buf, 7 ) )
      {
        D( kprintf( "failed to read screen descriptor\n" ) );
        return( FALSE );
      }

      /* Global Colormap ? */
      if( BitSet( buf[ 4 ], LOCALCOLORMAP ) )
      {
        UWORD bitPixel = 2 << (buf[ 4 ] & 0x07);

        /* Skip colormap */
        if( Seek( file, (LONG)(GIFCMAPENTRYSIZE * bitPixel), OFFSET_CURRENT ) == -1L )
        {
          return( FALSE );
        }
      }

      for( ;; )
      {
        if( !ReadOK( file, (&c), 1 ) )
        {
          D( kprintf( "EOF / read error on image data\n" ) );
          return( EnougthFramesToBeAGIFAnimation );
        }

        switch( c )
        {
          case ';': /* GIF terminator ? */
              return( EnougthFramesToBeAGIFAnimation );

          case '!': /* Extension ? */
          {
              UBYTE count;

              if( !ReadOK( file, &c, 1 ) )
              {
                D( kprintf( "OF / read error on extention function code\n" ) );
                return( EnougthFramesToBeAGIFAnimation );
              }

              do
              {
                if( !ReadOK( file, &count, 1 ) )
                {
                  D( kprintf( "error in getting DataBlock size\n" ) );
                  return( EnougthFramesToBeAGIFAnimation );
                }

                if( (count != 0) && (Seek( file, (LONG)count, OFFSET_CURRENT ) == -1L) )
                {
                  D( kprintf( "error in reading DataBlock\n" ) );
                  return( EnougthFramesToBeAGIFAnimation );
                }
              } while( count != 0 );
          }
              break;

          case ',': /* Raster data start ? */
          {
              UBYTE c;
              BOOL  useGlobalColormap;

              imagecount++;

              /* Second image ? */
              if( EnougthFramesToBeAGIFAnimation )
              {
                return( TRUE );
              }

              if( !ReadOK( file, buf, 9 ) )
              {
                D( kprintf( "couldn't read left/top/width/height\n" ) );
                return( EnougthFramesToBeAGIFAnimation );
              }

              useGlobalColormap = !BitSet( buf[ 8 ], LOCALCOLORMAP );

              if( !useGlobalColormap )
              {
                UWORD bitPixel = 1 << ((buf[ 8 ] & 0x07) + 1);

                /* Skip local colormap */
                if( Seek( file, (LONG)(GIFCMAPENTRYSIZE * bitPixel), OFFSET_CURRENT ) == -1L )
                {
                  return( EnougthFramesToBeAGIFAnimation );
                }
              }

              /* Initialize the Compression routines */
              if( !ReadOK( file, &c, 1 ) )
              {
                D( kprintf( "EOF / read error on image data\n" ) );
                return( EnougthFramesToBeAGIFAnimation );
              }

              D( kprintf( "skipping image...\n" ) );

              /* Loop until end of raster data */
              for( ;; )
              {
                if( !ReadOK( file, &c, 1 ) )
                {
                  D( kprintf( "EOF / reading block byte count\n" ) );
                  return( EnougthFramesToBeAGIFAnimation );
                }

                if( c == 0 )
                {
                  D( kprintf( "image done\n" ) );
                  break;
                }

                /* Skip... */
                if( Seek( file, (LONG)c, OFFSET_CURRENT ) == -1L )
                {
                  return( EnougthFramesToBeAGIFAnimation );
                }
              }
          }
              break;

          default: /* Not a valid raster data start character ? */
          {
              D( kprintf( "invalid character 0x%02x, ignoring\n", (int)c ) );
          }
              break;
        }
      }
    }

    return( EnougthFramesToBeAGIFAnimation );
}




