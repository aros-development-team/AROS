
/*
**
**  $VER: dispatch.c 2.4 (24.5.98)
**  gifanim.datatype 2.4
**
**  Dispatch routine for a DataTypes class
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
#include "classdata.h"

#include "dispatch.h"

/* ansi includes */
#include <limits.h>
#include <stdio.h>

/*****************************************************************************/

/* decoder related local prototypes */
static BOOL                 ReadColorMap( struct ClassBase *, struct GIFAnimInstData *, UWORD, struct ColorRegister * );
static int                  DoExtension( struct ClassBase *, Object *, struct GIFAnimInstData *, TEXT );
static int                  GetDataBlock( struct ClassBase *, struct GIFAnimInstData *, UBYTE * );
static int                  GetCode( struct ClassBase *, struct GIFAnimInstData *, int, BOOL );
static int                  LWZReadByte( struct ClassBase *, struct GIFAnimInstData *, BOOL, int );
static int                  getbase2( int );

/*****************************************************************************/

/* local prototypes */
static struct FrameNode    *AllocFrameNode( struct ClassBase *, APTR );
static struct BitMap       *AllocBitMapPooled( struct ClassBase *, ULONG, ULONG, ULONG, APTR );
static void                 AttachSample( struct ClassBase *, struct GIFAnimInstData * );

/*****************************************************************************/

#if !defined (__AROS__)
/* Create "gifanim.datatype" BOOPSI class */
struct IClass *initClass( struct ClassBase *cb )
{
    struct IClass *cl;

    /* Create our class... */
    if( cl = MakeClass( GIFANIMDTCLASS, ANIMATIONDTCLASS, NULL, (ULONG)sizeof( struct GIFAnimInstData ), 0UL ) )
    {
      cl -> cl_Dispatcher . h_Entry = (HOOKFUNC)Dispatch;
#define DTSTACKSIZE (16384UL)
      cl -> cl_Dispatcher . h_Entry    = (HOOKFUNC)StackSwapDispatch; /* see stackswap.c */
      cl -> cl_Dispatcher . h_SubEntry = (HOOKFUNC)Dispatch;          /* see stackswap.c */
      cl -> cl_Dispatcher . h_Data     = (APTR)DTSTACKSIZE;           /* see stackswap.c */
      cl -> cl_UserData                = cb;
      AddClass( cl );
    }

    return( cl );
}

#include "methods.h"

/* class dispatcher */
DISPATCHERFLAGS
ULONG Dispatch( REGA0 struct IClass *cl, REGA2 Object *o, REGA1 Msg msg )
{
    struct ClassBase        *cb = (struct ClassBase *)(cl -> cl_UserData);
    struct GIFAnimInstData  *gaid;
    ULONG                    retval = 0UL;

    switch( msg -> MethodID )
    {
      case OM_NEW:
          retval = DT_NewMethod( cl, o, msg );
          break;

      case OM_DISPOSE:
          retval = DT_DisposeMethod( cl, o, msg );
          break;

/* TEST TEST / Support for format change "on-the-fly" disabled here / TEST TEST
 * DO NOT make any assumptions on this EXPERIMENTAL code !
 */
#ifdef COMMENTED_OUT
      case DTM_FRAMEBOX:
          retval = DT_FrameBoxMethod( cl, o, msg );
          break;
#endif /* COMMENTED_OUT */

      case OM_UPDATE:
      case OM_SET:
          retval = DT_SetMethod( cl, o, msg );
          break;

      case DTM_WRITE:
          retval = DT_WriteMethod( cl, o, msg );
          break;

      case ADTM_LOADFRAME:
          retval = DT_LoadFrameMethod( cl, o, msg );
          break;

      case ADTM_UNLOADFRAME:
          retval = DT_UnLoadFrameMethod( cl, o, msg );
          break;

      /* Let the superclass handle everything else */
      default:
      {
          retval = DoSuperMethodA( cl, o, msg );
      }
          break;
    }

    return( retval );
}
#endif

BOOL ScanFrames( struct ClassBase *cb, Object *o )
{
    struct GIFAnimInstData *gaid    = (struct GIFAnimInstData *)INST_DATA( (cb -> cb_Lib . cl_Class), o );
    LONG                    error   = 0L;
    BOOL                    success = FALSE;

    D(bug("[gifanim.datatype]: %s()\n", __func__));

    InitSemaphore( (&(gaid -> gaid_SigSem)) );
    NewList( (struct List *)(&(gaid -> gaid_FrameList)) );

    /* Create a memory pool for frame nodes */
    if ((gaid -> gaid_Pool = CreatePool( MEMF_PUBLIC, 8192UL, 8192UL ) ) != NULL)
    {
      BPTR                 fh = BNULL;                             /* handle (file handle)      */
      IPTR                 sourcetype = 0;                     /* type of stream (either DTST_FILE or DTST_RAM) */
      ULONG                modeid /*= (ULONG)INVALID_ID*/; /* anim view mode                  */
      ULONG                animwidth,                      /* anim width                      */
                           animheight,                     /* anim height                     */
                           animdepth;                      /* anim depth                      */
      ULONG                timestamp  = 0UL;               /* timestamp                       */

        D(bug("[gifanim.datatype] %s: pool @ 0x%p\n", __func__, gaid -> gaid_Pool));

      /* Prefs defaults */
      gaid -> gaid_LoadAll = TRUE;              /* The decoder is too slow to allow realtime decoding of a
                                                 * 576 * 124 * 8 GIF Image, even on a mc68040 :-((
                                                 */
      gaid -> gaid_ModeID  = (ULONG)INVALID_ID; /* no screen mode selected yet */
      gaid -> gaid_Volume  = 64UL;

      /* Read prefs */
      ReadENVPrefs( cb, gaid, NULL );

      /* Get file handle, handle type and BitMapHeader */
      if( GetDTAttrs( o, DTA_SourceType,    (&sourcetype),
                         DTA_Handle,        (&fh),
                         DTA_Name,          (&(gaid -> gaid_ProjectName)),
                         TAG_DONE ) == 3UL )
      {
        switch( sourcetype )
        {
          case DTST_FILE:
          {
              D(bug("[gifanim.datatype] %s: DTST_FILE (0x%p)\n", __func__, fh));

              if( fh )
              {
                BPTR lock;

                if ((lock = DupLockFromFH( fh ) ) != BNULL)
                {
                  /* Set up a filehandle for disk-based loading (random loading) */
                  if ((gaid -> gaid_FH = (BPTR)OpenFromLock( lock ) ) != BNULL)
                  {
                    success = TRUE;
                  }
                  else
                  {
                    /* failure */
                    UnLock( lock );
                  }
                }
              }

              /* OpenFromLock failed ? - Then open by name :-( */
              if( (gaid -> gaid_FH) == BNULL )
              {
                /* Set up a filehandle for disk-based loading (random loading) */
                if ((gaid -> gaid_FH = (BPTR)Open( (gaid -> gaid_ProjectName), MODE_OLDFILE ) ) != BNULL)
                {
                  success = TRUE;
                }
                else
                {
                  /* Can't open file */
                  error = IoErr();
                }
              }
          }
              break;

          case DTST_RAM: /* empty object */
          {
              D(bug("[gifanim.datatype] %s: DTST_RAM\n", __func__));

              success = TRUE;
          }
              break;

          default:
          {
              /* unsupported source type */
              error = ERROR_NOT_IMPLEMENTED;
          }
              break;
        }

        /* Any error ? */
        if( success )
        {
          /* Now we enter the next stage of testing... */
          success = FALSE;

          if( fh )
          {
            struct GIFDecoder    *gifdec                           = (&(gaid -> gaid_GIFDec));
            struct FrameNode     *fn;
            ULONG                 numcmaps                         = 0UL; /* number of created cmaps */
            UBYTE                 buf[ 16 ];
            struct ColorRegister  localColorMap[ MAXCOLORMAPSIZE ] = { { 0 } };
            struct ColorRegister  savedTransparentColor            = { 0 };
            UBYTE                 c;
            BOOL                  useGlobalColormap;
            UWORD                 bitPixel;

            gifdec -> file                = fh;
            gifdec -> Gif89 . transparent = (UWORD)~0U; /* means: no transparent color */

            D(bug("[gifanim.datatype] %s: checking sig..\n", __func__));

            gaid->gaid_VerStr = AllocVec(7, MEMF_ANY);

            /* Read "GIF" indentifer and version */
            if( ReadOK( cb, gifdec, gaid->gaid_VerStr, 6 ) )
            {
                gaid->gaid_VerStr[6] = 0;

              /* Is there the GIF signature ? */
              if( !strncmp( gaid->gaid_VerStr, "GIF", 3 ) )
              {
                STRPTR version = (STRPTR)(gaid->gaid_VerStr + 3);

                  D(bug("[gifanim.datatype] %s: checking gif version..\n", __func__));

                /* Check if we support this GIF version */
                if( (!strncmp( version, "87a", 3 )) ||
                    (!strncmp( version, "89a", 3 )) )
                {
                        D(bug("[gifanim.datatype]: %s: read gif screen..\n", __func__));
                

                  /* Read GIF Screen */
                  if( ReadOK( cb, gifdec, buf, 7 ) )
                  {
                    struct FrameNode *prevnode = NULL;
                    UBYTE            *deltamap = NULL;

                    gifdec -> GifScreen . Width           = LOHI2UINT16( buf[ 0 ], buf[ 1 ] );
                    gifdec -> GifScreen . Height          = LOHI2UINT16( buf[ 2 ], buf[ 3 ] );
                    gifdec -> GifScreen . BitPixel        = 2 << (buf[ 4 ] & 0x07);
                    gifdec -> GifScreen . ColorResolution = (((buf[ 4 ] & 0x70) >> 3) + 1);
                    gifdec -> GifScreen . Background      = buf[ 5 ];
                    gifdec -> GifScreen . AspectRatio     = buf[ 6 ];

                                 gaid -> gaid_Width        = gifdec -> GifScreen . Width;
                    animwidth  = gaid -> gaid_PaddedWidth  = (gaid -> gaid_UseChunkyMap)?(gaid -> gaid_Width):(((gaid -> gaid_Width) + 15UL) & ~15UL); /* align for c2p-wpa (c2c does not need padding) */
                    animheight = gaid -> gaid_Height       = gifdec -> GifScreen . Height;
                    animdepth  = gaid -> gaid_Depth        = (gaid -> gaid_UseChunkyMap)?(DIRECTRGB_DEPTH):((ULONG)getbase2( (gifdec -> GifScreen . BitPixel) ));

                    D(bug("[gifanim.datatype] %s: %dx%dx%d\n", __func__, animwidth, animheight, animdepth));

                    /* Global Colormap ? */
                    if( BitSet( buf[ 4 ], LOCALCOLORMAP ) )
                    {
                        D(bug("[gifanim.datatype]: %s: reading global colormap..\n", __func__));

                      numcmaps++;

                      if( ReadColorMap( cb, gaid, (gifdec -> GifScreen . BitPixel), (gifdec -> GifScreen . ColorMap) ) )
                      {
                          D(bug("[gifanim.datatype]: %s:   failed!\n", __func__));
                        error = IoErr();
                        error_printf( cb, gaid, "error %d reading global colormap\n", error);
                      }
                    }
                    else
                    {
                      /* No global colormap ? - Then the background color in the GifScreen is a NOP */
                      gifdec -> GifScreen . Background = 0U;
                    }

                    while (TRUE)
                    {
                        D(bug("[gifanim.datatype]: %s: reading chunk..\n", __func__));
                      /* Read chunk ID char */
                      if( !ReadOK( cb, gifdec, (&c), 1 ) )
                      {
                        error = IoErr();
                        error_printf( cb, gaid, "EOF / read error on image data (%x)\n", error);
                        break;
                      }

                      switch( c )
                      {
                        case ';': /* GIF terminator ? */
                        {
                        D(bug("[gifanim.datatype]: %s: ## terminator\n", __func__));

                            goto scandone;
                        }

                        case '!': /* Extension ? */
                        {
                        D(bug("[gifanim.datatype]: %s: ## extension\n", __func__));

                            if( ReadOK( cb, gifdec, (&c), 1 ) )
                            {
                              if( DoExtension( cb, o, gaid, c ) == -1 )
                              {
                                error = IoErr();
                                error_printf( cb, gaid, "error %x in extension\n", error);
                                goto scandone;
                              }
                            }
                            else
                            {
                              error = IoErr();
                              error_printf( cb, gaid, "OF / read error on extension function code (%x)\n", error);
                              goto scandone;
                            }
                        }
                            break;

                        case ',': /* Raster data start ? */
                        {
                        D(bug("[gifanim.datatype]: %s: ## rast data\n", __func__));

                            /* Create an prepare a new frame node */
                            if ((fn = AllocFrameNode( cb, (gaid -> gaid_Pool) )) != NULL)
                            {
                                D(bug("[gifanim.datatype]: %s:    frame node @ 0x%p\n", __func__, fn));
                              if( (gaid -> gaid_LoadAll) || (timestamp == 0UL) )
                              {
                                if( !(fn -> fn_BitMap = AllocFrameBitMap( cb, gaid ) ) )
                                {
                                    D(bug("[gifanim.datatype]: %s: failed to allocate bitmap!\n", __func__));
                                  error = ERROR_NO_FREE_STORE;
                                }

                                /* Allocate array for chunkypixel data */
                                if ((fn -> fn_ChunkyMap = (UBYTE *)AllocPooledVec( cb, (gaid -> gaid_Pool), ((animwidth * animheight) + 256) )) != NULL)
                                {
                                  /* Get a clean background to avoid that rubbish shows througth transparent parts */
                                  memset( (fn -> fn_ChunkyMap), 0, (size_t)(animwidth * animheight) );
                                }
                                else
                                {
                                    D(bug("[gifanim.datatype]: %s: failed to allocate chunky data!\n", __func__));
                                  error = ERROR_NO_FREE_STORE;
                                }
                              }

                              if( error == 0L )
                              {
                                ULONG duration;

                                /* Get position of bitmap */
                                fn -> fn_BMOffset = Seek( fh, 0L, OFFSET_CURRENT ); /* BUG: does not check for failure */

                                D(bug("[gifanim.datatype]: %s:    bitmap offset %lu\n", __func__, fn->fn_BMOffset));

                                if( !ReadOK( cb, gifdec, buf, 9 ) )
                                {
                                  error = IoErr();
                                  error_printf( cb, gaid, "couldn't read left/top/width/height (%x)\n", error);
                                  goto scandone;
                                }

                                /* Local color map ? */
                                useGlobalColormap = !BitSet( buf[ 8 ], LOCALCOLORMAP );

                                /* Size of local color map */
                                bitPixel = 1 << ((buf[ 8 ] & 0x07) + 1);

                                /* Store GIF89a related attributes */
                                fn -> fn_GIF89aDisposal    = gifdec -> Gif89 . disposal;    /* Store disposal mode for current frame  */
                                fn -> fn_GIF89aTransparent = gifdec -> Gif89 . transparent; /* Store currents frame transparent color */

                                if( fn -> fn_ChunkyMap )
                                {
                                D(bug("[gifanim.datatype]: %s:    chunky\n", __func__));

                                  /* disposal method */
                                  switch( fn -> fn_GIF89aDisposal )
                                  {
                                    case GIF89A_DISPOSE_NOP:
                                    {
                                        D(bug("[gifanim.datatype]: %s:    GIF89A_DISPOSE_NOP\n", __func__));
                                        /* Background not transparent ? */
                                        if( ((fn -> fn_GIF89aTransparent) == ~0U) ||
                                            ((fn -> fn_GIF89aTransparent) != 0U) )
                                        {
                                          /* restore to color 0 */
                                          memset( (fn -> fn_ChunkyMap), 0, (size_t)(animwidth * animheight) );
                                        }
                                    }
                                        break;

                                    case GIF89A_DISPOSE_NODISPOSE:
                                    {
                                        D(bug("[gifanim.datatype]: %s:    GIF89A_DISPOSE_NODISPOSE\n", __func__));
                                        /* do not dispose prev image */

                                        /* If we have a previous frame, copy it  */
                                        if( prevnode )
                                        {
                                          CopyMem( (prevnode -> fn_ChunkyMap), (fn -> fn_ChunkyMap), (animwidth * animheight) );
#ifdef DELTAWPA8
                                          CopyBitMap( cb, (fn -> fn_BitMap), (prevnode -> fn_BitMap), animwidth, animheight );
                                          deltamap = prevnode -> fn_ChunkyMap;
#endif /* DELTAWPA8 */
                                        }
                                        else
                                        {
                                          /* Background not transparent ? */
                                          if( ((fn -> fn_GIF89aTransparent) == ~0U) ||
                                              ((fn -> fn_GIF89aTransparent) != 0U) )
                                          {
                                            /* restore to color 0 */
                                            memset( (fn -> fn_ChunkyMap), 0, (size_t)(animwidth * animheight) );
                                          }
#ifdef DELTAWPA8
                                          deltamap = NULL;
#endif /* DELTAWPA8 */
                                        }
                                    }
                                        break;

                                    case GIF89A_DISPOSE_RESTOREBACKGROUND:
                                    {
                                        D(bug("[gifanim.datatype]: %s:    GIF89A_DISPOSE_RESTOREBACKGROUND\n", __func__));
                                        /* Background not transparent ? */
                                        if( ((fn -> fn_GIF89aTransparent) == ~0U) ||
                                            ((fn -> fn_GIF89aTransparent) != (gaid -> gaid_GIFDec . GifScreen . Background)) )
                                        {
                                          /* Restore to background color */
                                          memset( (fn -> fn_ChunkyMap), (gifdec -> GifScreen . Background), (size_t)(animwidth * animheight) );
                                        }
                                    }
                                        break;

                                    case GIF89A_DISPOSE_RESTOREPREVIOUS:
                                    {
                                        D(bug("[gifanim.datatype]: %s:    GIF89A_DISPOSE_RESTOREPREVIOUS\n", __func__));
                                        /* restore previous image  */

                                        /* If we have a previous frame, copy it  */
                                        if( prevnode )
                                        {
                                          CopyMem( (prevnode -> fn_ChunkyMap), (fn -> fn_ChunkyMap), (animwidth * animheight) );
#ifdef DELTAWPA8
                                          CopyBitMap( cb, (fn -> fn_BitMap), (prevnode -> fn_BitMap), animwidth, animheight );
                                          deltamap = prevnode -> fn_ChunkyMap;
#endif /* DELTAWPA8 */
                                        }
                                        else
                                        {
                                          /* restore to color 0 */
                                          memset( (fn -> fn_ChunkyMap), 0, (size_t)(animwidth * animheight) );
#ifdef DELTAWPA8
                                          deltamap = NULL;
#endif /* DELTAWPA8 */
                                        }
                                    }
                                        break;

                                    default: /* GIF89A_DISPOSE_RESERVED4 - GIF89A_DISPOSE_RESERVED7 */
                                    {
                                        error_printf( cb, gaid, "unsupported disposal method %lu\n", (ULONG)(gifdec -> Gif89 . disposal) );
                                    }
                                        break;
                                  }
                                }

                                /* Save transparent color (if we have one) */
                                if( ((fn -> fn_GIF89aTransparent) != ~0U) && (timestamp != 0UL) )
                                {
                                    D(bug("[gifanim.datatype]: %s:    save transp\n", __func__));
                                  savedTransparentColor = localColorMap[ (fn -> fn_GIF89aTransparent) ];
                                }

                                /* Get colormap */
                                if( useGlobalColormap )
                                {
                                  /* use global colormap and depth */
                                  bitPixel = gifdec -> GifScreen . BitPixel;
                                  memcpy( localColorMap, (gifdec -> GifScreen . ColorMap), (sizeof( struct ColorRegister ) * bitPixel) );
                                }
                                else
                                {
                                    D(bug("[gifanim.datatype]: %s:    reading colormap...\n", __func__));
                                  numcmaps++;

                                  if( ReadColorMap( cb, gaid, bitPixel, localColorMap ) )
                                  {
                                    error = IoErr();
                                    error_printf( cb, gaid, "error %x reading local colormap\n", error);
                                    goto scandone;
                                  }
                                }

                                /* Restore transparent color (if we have one) */
                                if( (fn -> fn_GIF89aTransparent) != ~0U )
                                {
                                    D(bug("[gifanim.datatype]: %s:    restore transp\n", __func__));
                                  localColorMap[ (fn -> fn_GIF89aTransparent) ] = savedTransparentColor;
                                }

                                if( !(gaid -> gaid_UseChunkyMap) )
                                {
                                  /* The first palette must be moved to the object's palette */
                                  if( timestamp == 0UL )
                                  {
                                    if( !CMAP2Object( cb, o, (UBYTE *)localColorMap, (ULONG)(bitPixel * 3UL) ) )
                                    {
                                        D(bug("[gifanim.datatype]: %s: failed to allocate object cmap!\n", __func__));
                                      /* can't alloc object's color table */
                                      error = ERROR_NO_FREE_STORE;
                                    }
                                  }

                                  /* Create a palette-per-frame colormap here */
                                  if( !(fn -> fn_CMap = CMAP2ColorMap( cb, (1UL << (ULONG)(gaid -> gaid_Depth)), (UBYTE *)localColorMap, (ULONG)(bitPixel * 3UL) )) )
                                  {
                                      D(bug("[gifanim.datatype]: %s: failed to allocate frame cmap!\n", __func__));
                                    /* can't alloc colormap */
                                    error = ERROR_NO_FREE_STORE;
                                  }
                                }

                                D(bug("[gifanim.datatype]: %s:    copying colormap\n", __func__));
                                /* Copy colormap for 24 bit output */
                                memcpy( (void *)(fn -> fn_ColorMap), (void *)localColorMap, (size_t)(sizeof( struct ColorRegister ) * bitPixel) );

                                D(bug("[gifanim.datatype]: %s:    reading image ...\n", __func__));
                                (void)ReadImage( cb, gaid,
                                                 (fn -> fn_ChunkyMap),
                                                 (UWORD)animwidth,
                                                 LOHI2UINT16( buf[ 0 ], buf[ 1 ] ),
                                                 LOHI2UINT16( buf[ 2 ], buf[ 3 ] ),
                                                 LOHI2UINT16( buf[ 4 ], buf[ 5 ] ),
                                                 LOHI2UINT16( buf[ 6 ], buf[ 7 ] ),
                                                 BitSet( buf[ 8 ], INTERLACE ),
                                                 ((fn -> fn_BitMap) == NULL),
                                                 (fn -> fn_GIF89aTransparent) );

                                /* Get size of bitmap (curr_pos - start_of_bm) */
                                fn -> fn_BMSize = Seek( fh, 0L, OFFSET_CURRENT ) - (fn -> fn_BMOffset); /* BUG: does not check for failure */

                                D(bug("[gifanim.datatype]: %s:    bitmap = %d bytes\n", __func__, fn->fn_BMSize));

                                if( fn -> fn_BitMap )
                                {
                                    D(bug("[gifanim.datatype]: %s:    rendering to bitmap @ 0x%p\n", __func__, fn->fn_BitMap));
                                  if( gaid -> gaid_UseChunkyMap )
                                  {
                                    WriteRGBPixelArray8( cb, (fn -> fn_BitMap), animwidth, animheight, (fn -> fn_ColorMap), (fn -> fn_ChunkyMap) );
                                  }
                                  else
                                  {
                                    WriteDeltaPixelArray8Fast( (fn -> fn_BitMap), (fn -> fn_ChunkyMap), deltamap );
                                  }
                                }

                                /* Bump timestamp... */
                                if( ((gifdec -> Gif89 . delayTime) != ~0U) &&
                                    ((gifdec -> Gif89 . delayTime) > 1U)   &&
                                    ((gifdec -> Gif89 . delayTime) < 2000U) )
                                {
                                  duration = (gifdec -> Gif89 . delayTime);
                                }
                                else
                                {
                                  duration = 0;
                                }

                                fn -> fn_TimeStamp = timestamp;
                                fn -> fn_Frame     = timestamp;
                                fn -> fn_Duration  = duration;

                                D(bug("[gifanim.datatype]: %s:    %08p:%08p\n", __func__, fn->fn_TimeStamp, fn->fn_Duration));

                                AddTail( (struct List *)(&(gaid -> gaid_FrameList)), (struct Node *)(&(fn -> fn_Node)) );

                                prevnode = fn;

                                /* Next frame starts at timestamp... */
                                timestamp += (fn -> fn_Duration) + 1UL;
                              }
                            }
                        }
                            break;

                        case 0x00: /* padding byte ? */
                        {
                            /* Padding bytes are not part of the Compuserve documents, but... */
                            if( !(gaid -> gaid_StrictSyntax) )
                            {
                              break;
                            }
                        }
                        /* fall througth */
                        default: /* Not a valid raster data start character ? */
                        {
                            error_printf( cb, gaid, "invalid character 0x%02x, ignoring\n", (int)c );
                        }
                            break;
                      }

                      /* on error break */
                      if( error )
                      {
                          D(bug("[gifanim.datatype]: %s: ERROR!\n", __func__));
                        break;
                      }
                    }

scandone:
                    D(bug("[gifanim.datatype]: %s: scan done\n", __func__));

                    /* Any frames ? */
                    if( timestamp && (error == 0L) && numcmaps )
                    {
                      if( numcmaps == 1UL )
                      {
                        /* We only have a global colormap and no colormap changes (or a direct RGB bitmap),
                         * delete first colormap (a colormap in the first frames indicates following colormap
                         * changes)
                         */
                        struct FrameNode *firstnode = (struct FrameNode *)(gaid -> gaid_FrameList . mlh_Head);

                        if( firstnode -> fn_CMap )
                        {
                          FreeColorMap( (firstnode -> fn_CMap) );
                          firstnode -> fn_CMap = NULL;
                        }
                      }
                      else
                      {
                        /* All frames must have a colormap, therefore we replicate the colormap
                         * from the previous colormap if one is missing
                         */
                        struct FrameNode *worknode,
                                         *nextnode;
                        struct ColorMap  *currcm = NULL;

                        verbose_printf( cb, gaid, "Animation has palette changes per frame\n" );

                        worknode = (struct FrameNode *)(gaid -> gaid_FrameList . mlh_Head);

                        while ((nextnode = (struct FrameNode *)(worknode -> fn_Node . mln_Succ) ) != NULL)
                        {
                          if( worknode -> fn_CMap )
                          {
                            /* Current node contains colormap, this are the colors for the following frames... */
                            currcm = worknode -> fn_CMap;
                          }
                          else
                          {
                            if( currcm )
                            {
                              /* Copy colormap from previous one... */
                              if( !(worknode -> fn_CMap = CopyColorMap( cb, currcm )) )
                              {
                                /* Can't copy/alloc colormap */
                                error = ERROR_NO_FREE_STORE;
                              }
                            }
                            else
                            {
                              verbose_printf( cb, gaid, "scan/load: no colormap, can't copy it\n" );
                            }
                          }

                          worknode = nextnode;
                        }
                      }
                    }

                    /* Check for required information */
                    if( error == 0L )
                    {
                      /* Any frames loaded ? */
                      if( timestamp == 0UL )
                      {
                        /* not enougth frames (at least one required) */
                        error = DTERROR_NOT_ENOUGH_DATA;
                      }
                    }

                    /* Any error ? */
                    if( error == 0L )
                    {
                      struct FrameNode *firstfn = (struct FrameNode *)(gaid -> gaid_FrameList . mlh_Head); /* short cut to the first FrameNode */

                      /* Alloc bitmap as key bitmap */
                      if( gaid -> gaid_UseChunkyMap )
                      {
                        gaid -> gaid_KeyBitMap = AllocBitMap( animwidth, animheight, animdepth, (BMF_SPECIALFMT | SHIFT_PIXFMT( DIRECTRGB_PIXFMT )), NULL );
                      }
                      else
                      {
                        gaid -> gaid_KeyBitMap = AllocBitMap( animwidth, animheight, animdepth, BMF_CLEAR, NULL );
                      }

                      if( gaid -> gaid_KeyBitMap )
                      {
                        if( (firstfn -> fn_BitMap) == NULL )
                        {
                          /* can't alloc first bitmap */
                          error = ERROR_NO_FREE_STORE;
                        }

                        if( error == 0L )
                        {
                          /* Copy first frame into key bitmap */
                          CopyBitMap( cb, (gaid -> gaid_KeyBitMap), (firstfn -> fn_BitMap), animwidth, animheight );

                          /* No screen mode id set by prefs ? */
                          if( (gaid -> gaid_ModeID) != (ULONG)INVALID_ID )
                          {
                            modeid = gaid -> gaid_ModeID;
                          }
                          else
                          {
                            if( gaid -> gaid_UseChunkyMap )
                            {
                              /* We don't have fixed values for cybergfx mode id's, therefore we have to ask for them */
                              if( (modeid = BestCModeIDTags( CYBRBIDTG_NominalWidth,  animwidth,
                                                             CYBRBIDTG_NominalHeight, animheight,
                                                             CYBRBIDTG_Depth,         animdepth,
                                                             TAG_DONE )) == INVALID_ID )
                              {
#if 0
                                error = 1L; /* inducate an error here :-( */
#else
                                /* Workaround for CyberGFX bug :-( */
                                if( (modeid = BestCModeIDTags( CYBRBIDTG_NominalWidth,  640UL,
                                                               CYBRBIDTG_NominalHeight, 480UL,
                                                               CYBRBIDTG_Depth,         animdepth,
                                                               TAG_DONE )) == INVALID_ID )
                                {
                                  modeid = 0UL;

                                  error_printf( cb, gaid, "'CyberGFX bug' workaround failed, too ! Using modeid=%x.\n", modeid);
                                }
#endif

                                error_printf( cb, gaid, "No screenmode available for %lu/%lu/%lu\n", animwidth, animheight, animdepth );
                              }
                            }
                            else
                            {
                              /* BUG: Does currently not support SUPERHIRES modes */
                              if( animwidth >= 640UL )
                              {
                                if( animheight >= 400 )
                                {
                                  modeid = HIRESLACE_KEY;
                                }
                                else
                                {
                                  modeid = HIRES_KEY;
                                }
                              }
                              else
                              {
                                if( animheight >= 400 )
                                {
                                  modeid = LORESLACE_KEY;
                                }
                                else
                                {
                                  modeid = LORES_KEY;
                                }
                              }
                            }
                          }

                          /* No fps set by prefs ? */
                          if( (gaid -> gaid_FPS) == 0UL )
                          {
                            gaid -> gaid_FPS = 100; /* defaults to 100 fps. GIF 89a delay values counts in
                                                     * 1/100 sec steps. We set the alf_Duration field
                                                     * to this value (got from the GIF 89a extension).
                                                     */
                          }

                          AttachSample( cb, gaid );

                          verbose_printf( cb, gaid, "width %lu height %lu depth %lu frames %lu fps %lu\n",
                                          animwidth,
                                          animheight,
                                          animdepth,
                                          timestamp,
                                          (gaid -> gaid_FPS) );

                          /* Set misc. attributes */
                          SetDTAttrs( o, NULL, NULL,
                                      DTA_ObjName,                                       (gaid -> gaid_ProjectName),
                                      DTA_TotalHoriz,                                    animwidth,
                                      DTA_TotalVert,                                     animheight,
                                      ADTA_Width,                                        (gaid -> gaid_Width),
                                      ADTA_Height,                                       animheight,
                                      ADTA_Depth,                                        animdepth,
                                      ADTA_Frames,                                       timestamp,
                                      ADTA_FramesPerSecond,                              (gaid -> gaid_FPS),
                                      ADTA_ModeID,                                       modeid,
                                      ADTA_KeyFrame,                                     (gaid -> gaid_KeyBitMap),
                                      XTAG( (firstfn -> fn_Sample), ADTA_Sample       ), (firstfn -> fn_Sample),
                                      XTAG( (firstfn -> fn_Sample), ADTA_SampleLength ), ((firstfn -> fn_SampleLength) / ((firstfn -> fn_Duration) + 1UL)),
                                      XTAG( (firstfn -> fn_Sample), ADTA_Period       ), (firstfn -> fn_Period),
                                      XTAG( (firstfn -> fn_Sample), ADTA_Volume       ), (gaid -> gaid_Volume),
                                      TAG_DONE );

                          /* All done for now... */
                          success = TRUE;
                        }
                      }
                      else
                      {
                        /* can't alloc key bitmap */
                        error = ERROR_NO_FREE_STORE;
                      }
                    }
                  }
                  else
                  {
                    error = IoErr();
                    error_printf( cb, gaid, "failed to read gif screen descriptor (%x)\n", error);
                  }
                }
                else
                {
                  /* unsupported GIF version number */
                  error = DTERROR_UNKNOWN_COMPRESSION;
                  error_printf( cb, gaid, "error %x bad version number, not '87a' or '89a'\n", error);
                }
              }
              else
              {
                /* no GIF signature */
                error = ERROR_OBJECT_WRONG_TYPE;
                error_printf( cb, gaid, "error %x not a GIF file\n", error);
              }
            }
            else
            {
              error = IoErr();
              error_printf( cb, gaid, "error %x reading magic number\n", error);
            }

            /* Prepare decoder for dynamic frame access */
            gifdec -> file = gaid -> gaid_FH;
          }
          else
          {
            /* No file handle ? - Be sure we got a DTST_RAM sourcetype */
            if( sourcetype == DTST_RAM )
            {
              /* The object is used without any input file.
               * This "empty" object is used to run the encoder only...
               */
              success = TRUE;
            }
            else
            {
              /* No handle ! */
              error = ERROR_REQUIRED_ARG_MISSING;
            }
          }
        }
      }
      else
      {
        /* can't get required attributes from superclass */
        error = ERROR_OBJECT_WRONG_TYPE;
      }
    }
    else
    {
      /* no memory pool */
      error = ERROR_NO_FREE_STORE;
    }

    SetIoErr( error );

    return( success );
}


static
struct FrameNode *AllocFrameNode( struct ClassBase *cb, APTR pool )
{
    struct FrameNode *fn;

    D(bug("[gifanim.datatype]: %s()\n", __func__));

    if ((fn = (struct FrameNode *)AllocPooled( pool, (ULONG)sizeof( struct FrameNode ) ) ) != NULL)
    {
      memset( fn, 0, sizeof( struct FrameNode ) );
    }

    return( fn );
}


struct FrameNode *FindFrameNode( struct MinList *fnl, ULONG timestamp )
{

    D(bug("[gifanim.datatype]: %s()\n", __func__));

    if( fnl )
    {
      struct FrameNode *worknode,
                       *nextnode,
                       *prevnode;

      prevnode = worknode = (struct FrameNode *)(fnl -> mlh_Head);

      while ((nextnode = (struct FrameNode *)(worknode -> fn_Node . mln_Succ) ) != NULL)
      {
        if( (worknode -> fn_TimeStamp) > timestamp )
        {
          return( prevnode );
        }

        prevnode = worknode;
        worknode = nextnode;
      }

      if( !IsListEmpty( ((struct List *)fnl) ) )
      {
        return( prevnode );
      }
    }

    return( NULL );
}


void FreeFrameNodeResources( struct ClassBase *cb, struct GIFAnimInstData *gaid )
{
    struct FrameNode *worknode;

    D(bug("[gifanim.datatype]: %s()\n", __func__));

/* The follwoing was used for debugging */
/* #define FREE_LIST_IN_REVERSE_ORDER 1 */

#ifdef FREE_LIST_IN_REVERSE_ORDER
    struct FrameNode *nextnode;

    worknode = (struct FrameNode *)(gaid -> gaid_FrameList . mlh_Head);

    while ((nextnode = (struct FrameNode *)(worknode -> fn_Node . mln_Succ) ) != NULL)
#else
    while ((worknode = (struct FrameNode *)RemTail( (struct List *)(&(gaid -> gaid_FrameList)) ) ) != NULL)
#endif /* FREE_LIST_IN_REVERSE_ORDER */
    {
      if( worknode -> fn_CMap )
      {
        FreeColorMap( (worknode -> fn_CMap) );
        worknode -> fn_CMap = NULL;
      }

      if( worknode -> fn_BitMap )
      {
        FreeFrameBitMap( cb, gaid, (worknode -> fn_BitMap) );
        worknode -> fn_BitMap = NULL;
      }

#ifdef FREE_LIST_IN_REVERSE_ORDER
      worknode = nextnode;
#endif /* FREE_LIST_IN_REVERSE_ORDER */
    }
}


struct BitMap *AllocFrameBitMap( struct ClassBase *cb, struct GIFAnimInstData *gaid )
{
    D(bug("[gifanim.datatype]: %s()\n", __func__));

    if( gaid -> gaid_UseChunkyMap )
    {
      return( AllocBitMap( (ULONG)(gaid -> gaid_PaddedWidth), (ULONG)(gaid -> gaid_Height), (ULONG)(gaid -> gaid_Depth),
                           (BMF_SPECIALFMT | SHIFT_PIXFMT( DIRECTRGB_PIXFMT )), NULL ) );
    }
    else
    {
      return( AllocBitMapPooled( cb, (ULONG)(gaid -> gaid_PaddedWidth), (ULONG)(gaid -> gaid_Height), (ULONG)(gaid -> gaid_Depth), (gaid -> gaid_Pool) ) );
    }
}


void FreeFrameBitMap( struct ClassBase *cb, struct GIFAnimInstData *gaid, struct BitMap *bm )
{
    D(bug("[gifanim.datatype]: %s()\n", __func__));

    if( bm )
    {
      if( gaid -> gaid_UseChunkyMap )
      {
        FreeBitMap( bm );
      }
      else
      {
        FreePooledVec( cb, (gaid -> gaid_Pool), bm );
      }
    }
}


/* This function assumes (0UL < depth) && (depth <= 8UL) */
static
struct BitMap *AllocBitMapPooled( struct ClassBase *cb, ULONG width, ULONG height, ULONG depth, APTR pool )
{
    struct BitMap *bm;

    ULONG          planesize,
                   size;

    D(bug("[gifanim.datatype]: %s()\n", __func__));

    planesize = (ULONG)RASSIZE( width, height ) + 16UL;
    size      = ((ULONG)sizeof( struct BitMap )) + (planesize * depth) + width;

    if ((bm = (struct BitMap *)AllocPooledVec( cb, pool, size ) ) != NULL)
    {
      UWORD    pl;
      PLANEPTR plane;

      InitBitMap( bm, depth, width, height );

      plane = (PLANEPTR)(bm + 1); /* First plane follows struct BitMap */

      /* Set up plane data */
      pl = 0U;

      /* Set up plane ptrs */
      while( pl < depth )
      {
        bm -> Planes[ pl ] = plane;

        plane = (PLANEPTR)(((UBYTE *)plane) + planesize + 8);
        pl++;
      }

      /* Clear the remaining plane ptrs */
      while( pl < 8U )
      {
        bm -> Planes[ pl ] = NULL;

        pl++;
      }
    }

    return( bm );
}


void OpenLogfile( struct ClassBase *cb, struct GIFAnimInstData *gaid )
{
    D(bug("[gifanim.datatype]: %s()\n", __func__));

    if( ((gaid -> gaid_VerboseOutput) == BNULL) || ((gaid -> gaid_VerboseOutput) == (BPTR)-1L) )
    {
      STRPTR confile;

      if ((confile = (STRPTR)AllocVec( (((gaid -> gaid_ProjectName)?(strlen( (gaid -> gaid_ProjectName) )):(0UL)) + 100UL), MEMF_PUBLIC ) ) != NULL)
      {
        mysprintf( cb, confile, "CON:////GIF Anim DataType %s/auto/wait/close/inactive",
                   ((gaid -> gaid_ProjectName)?(FilePart( (gaid -> gaid_ProjectName) )):(NULL)) );

        gaid -> gaid_VerboseOutput = Open( confile, MODE_READWRITE );

        FreeVec( confile );
      }
    }
}

#if !defined(__AROS__)
void error_printf( struct ClassBase *cb, struct GIFAnimInstData *gaid, STRPTR format, ... )
{
    va_list args;

    if( (gaid -> gaid_VerboseOutput) != (BPTR)-1L )
    {
        OpenLogfile( cb, gaid );

        if( gaid -> gaid_VerboseOutput )
        {
            va_start (args, format);
            VFPrintf( (gaid -> gaid_VerboseOutput), format, (const IPTR *)args);
            va_end (args);
        }
    }
}


void verbose_printf( struct ClassBase *cb, struct GIFAnimInstData *gaid, STRPTR format, ... )
{
    va_list args;

    if( (gaid -> gaid_VerboseOutput) && ((gaid -> gaid_VerboseOutput) != (BPTR)-1L) )
    {
        va_start (args, format);
        VFPrintf( (gaid -> gaid_VerboseOutput), format, (const IPTR *)args);
        va_end (args);
    }
}
#endif

static
void AttachSample( struct ClassBase *cb, struct GIFAnimInstData *gaid )
{
    D(bug("[gifanim.datatype]: %s()\n", __func__));

    if( gaid -> gaid_Sample )
    {
      struct FrameNode *worknode,
                       *nextnode;

      ULONG             period          = gaid -> gaid_Period;
      ULONG             samplesperframe;
      BYTE             *sample          = gaid -> gaid_Sample;

      samplesperframe = (((SysBase -> ex_EClockFrequency) * 10UL) / (period * (gaid -> gaid_FPS) * 2UL));

      if( gaid -> gaid_SamplesPerFrame )
      {
        period = (period * samplesperframe) / (gaid -> gaid_SamplesPerFrame);

        samplesperframe = gaid -> gaid_SamplesPerFrame;

        verbose_printf( cb, gaid, "period corrected from %lu to %lu to match spf=%lu with fps=%lu\n",
                        (gaid -> gaid_Period), period, samplesperframe, (gaid -> gaid_FPS) );
      }

      verbose_printf( cb, gaid, "Attching samples (sysclock %lu period %lu fps %lu length %lu samplesperframe %lu)...\n",
                      (SysBase -> ex_EClockFrequency), period, (gaid -> gaid_FPS), (gaid -> gaid_SampleLength), samplesperframe );

      worknode = (struct FrameNode *)(gaid -> gaid_FrameList . mlh_Head);

      while ((nextnode = (struct FrameNode *)(worknode -> fn_Node . mln_Succ) ) != NULL)
      {
        worknode -> fn_Sample       = sample;
        worknode -> fn_SampleLength = samplesperframe * ((worknode -> fn_Duration) + 1UL);
        worknode -> fn_Period       = period;

        sample += worknode -> fn_SampleLength;

        /* End of sample reached ? */
        if( (IPTR)(sample - (gaid -> gaid_Sample)) > (gaid -> gaid_SampleLength) )
        {
          /* Cut last size of sample to fit */
          worknode -> fn_SampleLength -= (IPTR)(sample - (gaid -> gaid_Sample));

          break;
        }

        worknode = nextnode;
      }
    }
}


/* Read a GIF colormap info a struct ColorRegister */
static
BOOL ReadColorMap( struct ClassBase *cb, struct GIFAnimInstData *gaid, UWORD numcolors, struct ColorRegister *color )
{
    D(bug("[gifanim.datatype]: %s()\n", __func__));

    return( (BOOL)(!ReadOK( cb, (&(gaid -> gaid_GIFDec)), color, (ULONG)(GIFCMAPENTRYSIZE * numcolors) )) );
}


static
int DoExtension( struct ClassBase *cb, Object *o, struct GIFAnimInstData *gaid, TEXT label )
{
    struct GIFDecoder *gifdec     = (&(gaid -> gaid_GIFDec));
    UBYTE              buf[ 257 ] = { 0 };
    STRPTR             str;
    int                count;

    D(bug("[gifanim.datatype]: %s()\n", __func__));

    switch( (int)label )
    {
      case 0x01:              /* Plain Text Extension */
      {
          UWORD lpos,
                tpos,
                width,
                height,
                cellw,
                cellh,
                foreground,
                background;

          error_printf( cb, gaid, "'Plain text extension' (%d) not supported yet. Please send this animation to the author that"
                                  "this can be implemented\n", (int)label );

          if( GetDataBlock( cb, gaid, buf ) == -1 )
          {
            return( -1 );
          }

          lpos       = LOHI2UINT16( buf[ 0 ], buf[ 1 ] );
          tpos       = LOHI2UINT16( buf[ 2 ], buf[ 3 ] );
          width      = LOHI2UINT16( buf[ 4 ], buf[ 5 ] );
          height     = LOHI2UINT16( buf[ 6 ], buf[ 7 ] );
          cellw      = buf[ 8 ];
          cellh      = buf[ 9 ];
          foreground = buf[ 10 ];
          background = buf[ 11 ];

          verbose_printf( cb, gaid, "Plain text: "
                                    "left %lu top %lu width %lu height %lu "
                                    "cell width %lu cell height %lu"
                                    "foreground %lu background %lu", lpos, tpos, width, height, cellw, cellh, foreground, background );

          while( (count = GetDataBlock( cb, gaid, buf )), ((count != 0) && (count != -1)) )
          {
#if 0
            PPM_ASSIGN( image[ ypos ][ xpos ], cmap[ CM_RED ][ v ], cmap[ CM_GREEN ][ v ], cmap[ CM_BLUE ][ v ] );

            index++;
#endif

            /* Clear buffer for next cycle */
            memset( (void *)buf, 0, sizeof( buf ) );
          }

          return( 0 );
      }

      case 0xf9:              /* Graphic Control Extension */
      {
          STRPTR fmt; /* Format string for verbose output (fmt changes if transparent color is set) */

          if( GetDataBlock( cb, gaid, buf ) == -1 )
          {
            return( -1 );
          }

          /* Get "delta" mode (disposal of previous frame), input flag and the delay time in 1/100 sec) */
          gifdec -> Gif89 . disposal    = (buf[ 0 ] >> 2) & 0x7;
          gifdec -> Gif89 . inputFlag   = (buf[ 0 ] >> 1) & 0x1;
          gifdec -> Gif89 . delayTime   = LOHI2UINT16( buf[ 1 ], buf[ 2 ] );

          /* Any transparent color ? */
          if( buf[ 0 ] & 0x01 )
          {
            gifdec -> Gif89 . transparent = buf[ 3 ];

            fmt = "Graphic Control Extension: disposal %s (%lu)%s transparent %lu\n";
          }
          else
          {
            fmt = "Graphic Control Extension: disposal %s (%lu)%s\n";
          }

          /* Verbose output ? */
          if( (gaid -> gaid_VerboseOutput) && ((gaid -> gaid_VerboseOutput) != (BPTR)-1L) )
          {
            STRPTR user_input = ((gifdec -> Gif89 . inputFlag)?(" user input requested"):(""));
            STRPTR disposal;

            switch( gifdec -> Gif89 . disposal )
            {
              case GIF89A_DISPOSE_NOP:                  disposal = "nop";                break;
              case GIF89A_DISPOSE_NODISPOSE:            disposal = "no dispose";         break;
              case GIF89A_DISPOSE_RESTOREBACKGROUND:    disposal = "restore background"; break;
              case GIF89A_DISPOSE_RESTOREPREVIOUS:      disposal = "restore previous";   break;
              default:                                  disposal = "reserved";           break;
            }

            verbose_printf( cb, gaid, fmt,
                                      disposal,
                                      (ULONG)(gifdec -> Gif89 . disposal),
                                      user_input,
                                      (gifdec -> Gif89 . transparent) );
          }

          /* Ignore remaining data... */
          while( (count = GetDataBlock( cb, gaid, (UBYTE *)buf )), ((count != 0) && (count != -1)) )
                  ;

          /* Return 0 (success) or -1 (error) */
          return( count );
      }

      case 0xfe:              /* Comment Extension */
      {
          STRPTR annotation = NULL;

          /* Get all comment extension chunks, and append them on the DTA_ObjAnnotation string we've created before */
          while( (count = GetDataBlock( cb, gaid, buf )), ((count != 0) && (count != -1)) )
          {
            ULONG  size;
            STRPTR oldannotation = NULL;

            buf[ 255 ] = '\0'; /* terminate explicitly */

            size = (ULONG)strlen( buf ) + 2UL;

            (void)GetDTAttrs( o, DTA_ObjAnnotation, (&oldannotation), TAG_DONE );

            if( oldannotation )
            {
              size += (ULONG)strlen( oldannotation ) + 2UL;
            }

            /* Allocate a temp. buffer */
            if ((annotation = (STRPTR)AllocMem( size, MEMF_ANY ) ) != NULL)
            {
              if( oldannotation )
              {
                strcpy( annotation, oldannotation );
              }
              else
              {
                annotation[ 0 ] = '\0'; /* terminate */
              }

              /* Append the new buffer */
              IBMPC2ISOLatin1( buf, (annotation + strlen( annotation )) );

              /* Store the comment */
              SetDTAttrs( o, NULL, NULL, DTA_ObjAnnotation, annotation, TAG_DONE );

              /* Free temp string */
              FreeMem( annotation, size );
            }

            /* Clear buffer for next cycle */
            memset( (void *)buf, 0, sizeof( buf ) );
          }

          /* After all, prompt the annotation to the user */
          (void)GetDTAttrs( o, DTA_ObjAnnotation, (&annotation), TAG_DONE );

          verbose_printf( cb, gaid, "Comment Extension: '%s'\n", annotation );

          return( 0 );
      }

      case 0xff:              /* Application Extension */
      {
          str = "Application Extension";
      }
          break;

      default:
      {
          mysprintf( cb, buf, "UNKNOWN (0x%02lx)", (long)label );
          str = buf;
      }
          break;
    }

    verbose_printf( cb, gaid, "got a '%s' extension\n", ((str)?(str):(STRPTR)"") );

    /* skip extension data */
    while( (count = GetDataBlock( cb, gaid, buf )), ((count != 0) && (count != -1)) )
      ;

    /* Returns 0 (success) or -1 (error) */
    return( count );
}


static
int GetDataBlock( struct ClassBase *cb, struct GIFAnimInstData *gaid, UBYTE *buf )
{
    struct GIFDecoder *gifdec = (&(gaid -> gaid_GIFDec));
    UBYTE              count;

    D(bug("[gifanim.datatype]: %s()\n", __func__));

    if( !ReadOK( cb, gifdec, &count, 1 ) )
    {
      error_printf( cb, gaid, "error %d in getting DataBlock size\n", -1);

      return( -1 );
    }

    gifdec -> ZeroDataBlock = (count == 0);

    if( (count != 0) && (!ReadOK( cb, gifdec, buf, (ULONG)count ) ) )
    {
      error_printf( cb, gaid, "error %d in reading DataBlock\n", -1);

      return( -1 );
    }

    return( count );
}


/* returns -1 for error */
static
int GetCode( struct ClassBase *cb, struct GIFAnimInstData *gaid, int code_size, BOOL flag )
{
    struct GIFDecoder *gifdec = (&(gaid -> gaid_GIFDec));
    int                i,
                       j,
                       ret;
    UBYTE              count;

    if( flag )
    {
      gifdec -> GetCode . curbit  = 0;
      gifdec -> GetCode . lastbit = 0;
      gifdec -> GetCode . done    = FALSE;

      return( 0 );
    }

    if( (gifdec -> GetCode . curbit + code_size) >= (gifdec -> GetCode . lastbit) )
    {
      if( gifdec -> GetCode . done )
      {
        if( (gifdec -> GetCode . curbit) >= (gifdec -> GetCode . lastbit) )
          error_printf( cb, gaid, "GetCode: ran past the end of code bits (%d)\n", gifdec->GetCode.lastbit);

        return( -1 );
      }

      gifdec -> GetCode . buf[ 0 ] = gifdec -> GetCode . buf[ gifdec -> GetCode . last_byte - 2 ];
      gifdec -> GetCode . buf[ 1 ] = gifdec -> GetCode . buf[ gifdec -> GetCode . last_byte - 1 ];

      if( (count = GetDataBlock( cb, gaid, (&(gifdec -> GetCode . buf[ 2 ])) )) == 0 )
        gifdec -> GetCode . done = TRUE;

      gifdec -> GetCode . last_byte = 2 + count;
      gifdec -> GetCode . curbit    = (gifdec -> GetCode . curbit - gifdec -> GetCode . lastbit) + 16;
      gifdec -> GetCode . lastbit   = (2 + count) * 8 ;
    }

    ret = 0;

    for( i = gifdec -> GetCode . curbit, j = 0; j < code_size ; i++, j++ )
      ret |= ((gifdec -> GetCode . buf[ i / 8 ] & (1 << (i % 8))) != 0) << j;

    gifdec -> GetCode . curbit += code_size;

    return( ret );
}


static
int LWZReadByte( struct ClassBase *cb, struct GIFAnimInstData *gaid, BOOL flag, int input_code_size )
{
             struct GIFDecoder *gifdec = (&(gaid -> gaid_GIFDec)); /* shortcut */
             int                code,
                                incode;
    register int                i;

    if( flag )
    {
      gifdec -> LWZReadByte . set_code_size = input_code_size;
      gifdec -> LWZReadByte . code_size     = gifdec -> LWZReadByte . set_code_size + 1;
      gifdec -> LWZReadByte . clear_code    = 1 << gifdec -> LWZReadByte . set_code_size ;
      gifdec -> LWZReadByte . end_code      = gifdec -> LWZReadByte . clear_code + 1;
      gifdec -> LWZReadByte . max_code_size = 2 * gifdec -> LWZReadByte . clear_code;
      gifdec -> LWZReadByte . max_code      = gifdec -> LWZReadByte . clear_code + 2;

      (void)GetCode( cb, gaid, 0, TRUE );

      gifdec -> LWZReadByte . fresh = TRUE;

      /* Fill table with the codes... */
      for( i = 0 ; i < (gifdec -> LWZReadByte . clear_code) ; i++ )
      {
        gifdec -> LWZReadByte . table[ 0 ][ i ] = 0;
        gifdec -> LWZReadByte . table[ 1 ][ i ] = i;
      }

      /* ... and clear the remaining part  */
      for( ; i < (1 << MAX_LWZ_BITS) ; i++ )
      {
        gifdec -> LWZReadByte . table[ 0 ][ i ] = 0;
      }

      gifdec -> LWZReadByte . table[ 1 ][ 0 ] = 0;

      /* Reset stack ptr */
      gifdec -> LWZReadByte . sp = gifdec -> LWZReadByte . stack;

      return( 0 );
    }
    else
    {
      if( gifdec -> LWZReadByte . fresh )
      {
        gifdec -> LWZReadByte . fresh = FALSE;

        do
        {
          gifdec -> LWZReadByte . firstcode = gifdec -> LWZReadByte . oldcode = GetCode( cb, gaid, gifdec -> LWZReadByte . code_size, FALSE );
        } while( (gifdec -> LWZReadByte . firstcode) == (gifdec -> LWZReadByte . clear_code) );

        return( gifdec -> LWZReadByte . firstcode );
      }
    }

    if( (gifdec -> LWZReadByte . sp) > (gifdec -> LWZReadByte . stack) )
    {
      return( *--gifdec -> LWZReadByte . sp );
    }

    while( (code = GetCode( cb, gaid, gifdec -> LWZReadByte . code_size, FALSE )) >= 0 )
    {
      if( code == gifdec -> LWZReadByte . clear_code )
      {
        for( i = 0 ; i < gifdec -> LWZReadByte . clear_code ; i++ )
        {
          gifdec -> LWZReadByte . table[ 0 ][ i ] = 0;
          gifdec -> LWZReadByte . table[ 1 ][ i ] = i;
        }

        for( ; i < (1 << MAX_LWZ_BITS) ; i++ )
        {
          gifdec -> LWZReadByte . table[ 0 ][ i ] =
            gifdec -> LWZReadByte . table[ 1 ][ i ] = 0;
        }

        gifdec -> LWZReadByte . code_size       = gifdec -> LWZReadByte . set_code_size + 1;
        gifdec -> LWZReadByte . max_code_size   = 2 * gifdec -> LWZReadByte . clear_code;
        gifdec -> LWZReadByte . max_code        = gifdec -> LWZReadByte . clear_code + 2;
        gifdec -> LWZReadByte . sp              = gifdec -> LWZReadByte . stack;
        gifdec -> LWZReadByte . firstcode       =
          gifdec -> LWZReadByte . oldcode       = GetCode( cb, gaid, gifdec -> LWZReadByte . code_size, FALSE );

        return( gifdec -> LWZReadByte . firstcode );
      }
      else
      {
        if( code == gifdec -> LWZReadByte . end_code )
        {
          int   count;
          UBYTE buf[ 260 ];

          if( gifdec -> ZeroDataBlock )
            return( -2 );

          while( (count = GetDataBlock( cb, gaid, buf )) > 0 )
            ;

          if( count != 0 )
            error_printf( cb, gaid, "missing EOD in data stream (common occurence) (count=%d)\n", count);

          return( -2 );
        }
      }

      incode = code;

      if( code >= (gifdec -> LWZReadByte . max_code) )
      {
        *gifdec -> LWZReadByte . sp++ = gifdec -> LWZReadByte . firstcode;
        code = gifdec -> LWZReadByte . oldcode;
      }

      while( code >= gifdec -> LWZReadByte . clear_code )
      {
        *gifdec -> LWZReadByte . sp++ = gifdec -> LWZReadByte . table[ 1 ][ code ];

        if( code == gifdec -> LWZReadByte . table[ 0 ][ code ] )
          error_printf( cb, gaid, "circular table entry (code=%d) BIG ERROR\n", code);

        code = gifdec -> LWZReadByte . table[ 0 ][ code ];
      }

      *gifdec -> LWZReadByte . sp++ = gifdec -> LWZReadByte . firstcode = gifdec -> LWZReadByte . table[ 1 ][ code ];

      if( (code = gifdec -> LWZReadByte . max_code) < (1 << MAX_LWZ_BITS ) )
      {
        gifdec -> LWZReadByte . table[ 0 ][ code ] = gifdec -> LWZReadByte . oldcode;
        gifdec -> LWZReadByte . table[ 1 ][ code ] = gifdec -> LWZReadByte . firstcode;
        gifdec -> LWZReadByte . max_code++;

        if( (gifdec -> LWZReadByte . max_code >= gifdec -> LWZReadByte . max_code_size) && (gifdec -> LWZReadByte . max_code_size < (1 << MAX_LWZ_BITS)) )
        {
          gifdec -> LWZReadByte . max_code_size *= 2;
          gifdec -> LWZReadByte . code_size++;
        }
      }

      gifdec -> LWZReadByte . oldcode = incode;

      if( gifdec -> LWZReadByte . sp > gifdec -> LWZReadByte . stack )
      {
        return( *--gifdec -> LWZReadByte . sp );
      }
    }

    return( code );
}


int ReadImage( struct ClassBase *cb, struct GIFAnimInstData *gaid, UBYTE *image,
               UWORD imagewidth, UWORD left, UWORD top, UWORD len, UWORD height,
               BOOL interlace, BOOL ignore, UWORD transparent )
{
    struct GIFDecoder *gifdec = (&(gaid -> gaid_GIFDec)); /* shortcut */
    UBYTE              c;

    D(bug("[gifanim.datatype]: %s()\n", __func__));

    /* Initialize the Compression routines */
    if( !ReadOK( cb, gifdec, &c, 1 ) )
    {
      return( -1 );
    }

    /* If this is an "uninteresting picture" ignore it. */
    if( ignore )
    {
      D(bug("[gifanim.datatype]: %s: skipping gif image...\n", __func__ ) );

      /* Loop until end of raster data */
      while(TRUE)
      {
        if( !ReadOK( cb, gifdec, &c, 1 ) )
        {
          error_printf( cb, gaid, "EOF (%d) / reading block byte count\n", -1);
          return( -1 );
        }

        if( c == 0 )
        {
          D(bug("[gifanim.datatype]: %s: gif image done\n", __func__ ) );
          break;
        }

        /* Skip... */
        if( Seek( (gifdec -> file), (long)c, OFFSET_CURRENT ) == -1L )
        {
          return( -1 );
        }
      }
    }
    else
    {
       WORD v;
      ULONG xpos    = 0UL,
            ypos    = 0UL,
            offset  = (top * imagewidth) + left, /* "ypos" position in image (byte offset) */
            pass    = 0UL;                       /* interlace pass */

      if( LWZReadByte( cb, gaid, TRUE, c ) < 0 )
        error_printf( cb, gaid, "error (<%d) reading image\n", 0);

      D(bug("[gifanim.datatype]: %s: reading %lx %ld.%ld / %ld by %ld%s GIF image\n", __func__, image, left, top, len, height, interlace ? " interlaced" : "" ) );

      while( (v = LWZReadByte( cb, gaid, FALSE, c )) >= 0 )
      {
        /* Pixel transparent ? */
        if( (transparent == ~0U) || (transparent != v) )
        {
          /* Store pixel */
          image[ offset + xpos ] = v;
        }

        xpos++;

        if( xpos == len )
        {
          xpos = 0UL;

          if( interlace )
          {
            switch( pass )
            {
              case 0UL:
              case 1UL: ypos += 8UL; break;
              case 2UL: ypos += 4UL; break;
              case 3UL: ypos += 2UL; break;
            }

            if( ypos >= height )
            {
              pass++;

              switch( pass )
              {
                case 1UL: ypos = 4UL;  break;
                case 2UL: ypos = 2UL;  break;
                case 3UL: ypos = 1UL;  break;
                default: goto fini;
              }
            }
          }
          else
          {
            ypos++;
          }

          offset = ((ypos + top) * imagewidth) + left;
        }

        if( ypos >= height )
          break;
      }

fini:
      if( (v = LWZReadByte( cb, gaid, FALSE, c )) >= 0 )
      {
        /* 0x00-bytes are treated here as padding bytes unless the STRICTSYNTAX option is set... */
        if( (v != 0) || (gaid -> gaid_StrictSyntax) )
        {
          verbose_printf( cb, gaid, "too much input data %ld, ignoring extra...\n", (long)v );
        }
      }
    }

    return( 0 );
}


/* got from my anim.datatype (IFF ANIM) */
struct FrameNode *GetPrevFrameNode( struct FrameNode *currfn, ULONG interleave )
{
    struct FrameNode *worknode,
                     *prevnode;

    D(bug("[gifanim.datatype]: %s()\n", __func__));

    /* Get previous frame */
    worknode = currfn;

    while ((prevnode = (struct FrameNode *)(worknode -> fn_Node . mln_Pred) ) != NULL)
    {
      if( (interleave-- == 0U) || ((prevnode -> fn_Node . mln_Pred) == NULL) )
      {
        break;
      }

      worknode = prevnode;
    }

    return( worknode );
}


/* WritePixelArray8 replacement by Peter McGavin (p.mcgavin@irl.cri.nz),
 * slightly adapted to fit here...
 */

void WriteDeltaPixelArray8Fast( struct BitMap *dest, UBYTE *source, UBYTE *prev )
{
             ULONG *plane[ 8 ] = { 0 };
    register ULONG *chunky     = (ULONG *)source, /* fetch 32 bits per cycle */
                   *prevchunky = (ULONG *)prev;
             ULONG  numcycles  = ((dest -> Rows) * (dest -> BytesPerRow)) / sizeof( ULONG ),
                    i;

    D(bug("[gifanim.datatype]: %s()\n", __func__));

    /* Copy plane ptrs */
    for( i = 0UL ; i < (dest -> Depth) ; i++ )
    {
        D(bug("[gifanim.datatype] %s: plane %d @ 0x%p\n", __func__, i, dest -> Planes[ i ]));
      plane[ i ] = (ULONG *)(dest -> Planes[ i ]);
    }

    /* Fill unused planes with plane 0, which will be written last, all previous accesses
     * will be droped (assumes that a cache hides this "dummy" writes)
     */
    for( ; i < 8UL ; i++ )
    {
      plane[ i ] = (ULONG *)(dest -> Planes[ 0 ]);
    }

#define merge( a, b, mask, shift ) \
      tmp = mask & (a ^ (b >> shift));   \
      a ^= tmp;                          \
      b ^= (tmp << shift)

    /* Check if we have to do the "delta" test */
    if( prevchunky )
    {
        /* Process bitmaps */
      for( i = 0UL ; i < numcycles ; i++ )
      {
        ULONG curr0, curr1, curr2, curr3, curr4, curr5, curr6, curr7;
        ULONG prev0, prev1, prev2, prev3, prev4, prev5, prev6, prev7;
        ULONG tmp;

        /* process 32 pixels */

        curr0 =  AROS_BE2LONG(*chunky++);  curr4 =  AROS_BE2LONG(*chunky++);
        curr1 =  AROS_BE2LONG(*chunky++);  curr5 =  AROS_BE2LONG(*chunky++);
        curr2 =  AROS_BE2LONG(*chunky++);  curr6 =  AROS_BE2LONG(*chunky++);
        curr3 =  AROS_BE2LONG(*chunky++);  curr7 =  AROS_BE2LONG(*chunky++);

        prev0 =  AROS_BE2LONG(*prevchunky++);  prev4 =  AROS_BE2LONG(*prevchunky++);
        prev1 =  AROS_BE2LONG(*prevchunky++);  prev5 =  AROS_BE2LONG(*prevchunky++);
        prev2 =  AROS_BE2LONG(*prevchunky++);  prev6 =  AROS_BE2LONG(*prevchunky++);
        prev3 =  AROS_BE2LONG(*prevchunky++);  prev7 =  AROS_BE2LONG(*prevchunky++);

        /* I use the '+' here to avoid that the compiler skips an expression.
         * WARNING: The code assumes that the code is executed in the sequence as it occurs here
         */
        if ( (curr0 != prev0) || (curr4 != prev4) ||
            (curr1 != prev1) || (curr5 != prev5) ||
            (curr2 != prev2) || (curr6 != prev6) ||
            (curr3 != prev3) || (curr7 != prev7))
        {
          merge( curr0, curr2, 0x0000ffff, 16 );
          merge( curr1, curr3, 0x0000ffff, 16 );
          merge( curr4, curr6, 0x0000ffff, 16 );
          merge( curr5, curr7, 0x0000ffff, 16 );

          merge( curr0, curr1, 0x00ff00ff,  8 );
          merge( curr2, curr3, 0x00ff00ff,  8 );
          merge( curr4, curr5, 0x00ff00ff,  8 );
          merge( curr6, curr7, 0x00ff00ff,  8 );

          merge( curr0, curr4, 0x0f0f0f0f,  4 );
          merge( curr1, curr5, 0x0f0f0f0f,  4 );
          merge( curr2, curr6, 0x0f0f0f0f,  4 );
          merge( curr3, curr7, 0x0f0f0f0f,  4 );

          merge( curr0, curr2, 0x33333333,  2 );
          merge( curr1, curr3, 0x33333333,  2 );
          merge( curr4, curr6, 0x33333333,  2 );
          merge( curr5, curr7, 0x33333333,  2 );

          merge( curr0, curr1, 0x55555555,  1 );
          merge( curr2, curr3, 0x55555555,  1 );
          merge( curr4, curr5, 0x55555555,  1 );
          merge( curr6, curr7, 0x55555555,  1 );

          *plane[ 7 ]++ = AROS_LONG2BE(curr0);
          *plane[ 6 ]++ = AROS_LONG2BE(curr1);
          *plane[ 5 ]++ = AROS_LONG2BE(curr2);
          *plane[ 4 ]++ = AROS_LONG2BE(curr3);
          *plane[ 3 ]++ = AROS_LONG2BE(curr4);
          *plane[ 2 ]++ = AROS_LONG2BE(curr5);
          *plane[ 1 ]++ = AROS_LONG2BE(curr6);
          *plane[ 0 ]++ = AROS_LONG2BE(curr7);
        }
        else
        {
          plane[ 7 ]++;
          plane[ 6 ]++;
          plane[ 5 ]++;
          plane[ 4 ]++;
          plane[ 3 ]++;
          plane[ 2 ]++;
          plane[ 1 ]++;
          plane[ 0 ]++;
        }
      }
    }
    else
    {
      /* Process bitmaps */
      for( i = 0UL ; i < numcycles ; i++ )
      {
        register ULONG b0, b1, b2, b3, b4, b5, b6, b7,
                       tmp;

        /* process 32 pixels */
        b0 = AROS_BE2LONG(*chunky++);  b4 = AROS_BE2LONG(*chunky++);
        b1 = AROS_BE2LONG(*chunky++);  b5 = AROS_BE2LONG(*chunky++);
        b2 = AROS_BE2LONG(*chunky++);  b6 = AROS_BE2LONG(*chunky++);
        b3 = AROS_BE2LONG(*chunky++);  b7 = AROS_BE2LONG(*chunky++);

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

        *plane[ 7 ]++ = AROS_LONG2BE(b0);
        *plane[ 6 ]++ = AROS_LONG2BE(b1);
        *plane[ 5 ]++ = AROS_LONG2BE(b2);
        *plane[ 4 ]++ = AROS_LONG2BE(b3);
        *plane[ 3 ]++ = AROS_LONG2BE(b4);
        *plane[ 2 ]++ = AROS_LONG2BE(b5);
        *plane[ 1 ]++ = AROS_LONG2BE(b6);
        *plane[ 0 ]++ = AROS_LONG2BE(b7);
      }
    }
}


static
int getbase2( int x )
{
    int i = 0,
        j = 1;

    while( x > j )
    {
      j *= 2;
      i++;
    }

    return( i );
}



/* Read and test */
BOOL ReadOK( struct ClassBase *cb, struct GIFDecoder *gifdec, void *buffer, ULONG len )
{
    if( (gifdec -> which_fh) == WHICHFH_FILE )
    {
      return( (BOOL)(Read( (gifdec -> file), buffer, len ) == len) );
    }
    else
    {
      /* Check if the request fit in out buffer... */
      if( (((gifdec -> buffer) - (gifdec -> file_buffer)) + len) <= (gifdec -> buffersize) )
      {
        CopyMem( (gifdec -> buffer), buffer, len );
        gifdec -> buffer += len;

        return( TRUE );
      }
    }

    return( FALSE );
}




