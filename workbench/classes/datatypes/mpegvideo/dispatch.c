/*
**
**  $VER: dispatch.c 1.11 (7.11.97)
**  mpegvideo.datatype 1.11
**
**  Dispatch routine for a DataTypes class
**
**  Written 1996/1997 by Roland 'Gizzy' Mainz
**  Original example source from David N. Junod
**
*/

#ifndef DEBUG
#   define DEBUG 0
#endif
#include <aros/debug.h>

/* main includes */
#include "classbase.h"

/* ansi includes */
#include <limits.h>
#include <stdio.h>

/* project includes */
#include "mpegutil.h"

/*****************************************************************************/

/* is animation.datatype V41 ? */
#if !defined(__AROS__)
#define ISV41 ((classbase -> cb_SuperClassBase -> lib_Version) == 41U)
#else
#define ISV41 TRUE
#endif

/*****************************************************************************/

/* local prototypes */
static STRPTR            GetPrefsVar( struct ClassBase *, STRPTR );
static BOOL              matchstr( struct ClassBase *, STRPTR, STRPTR );
static void              ReadENVPrefs( struct ClassBase *, struct MPEGVideoInstData * );
#if !defined(__AROS__)
static BOOL              AttemptOpenVMM( struct MPEGVideoInstData * );
#endif
static void              AttachSample( struct MPEGVideoInstData * );

static void              CreateProgressRequester( struct MPEGVideoInstData * );
static void              DeleteProgressRequester( struct MPEGVideoInstData * );

static struct BitMap    *AllocFastBitMap( struct MPEGVideoInstData *, ULONG, ULONG, ULONG );
static void              CopyBitMap( struct ClassBase *, struct BitMap *, struct BitMap *, ULONG, ULONG );


/*****************************************************************************/

#if !defined (__AROS__)
struct IClass *initClass( struct ClassBase *classbase )
{
    struct IClass *cl;

    init_tables();

    /* Create our class... */
    if( cl = MakeClass( MPEGVIDEODTCLASS, ANIMATIONDTCLASS, NULL, (ULONG)sizeof( struct MPEGVideoInstData ), 0UL ) )
    {
#define DTSTACKSIZE (16384UL)
      cl -> cl_Dispatcher . h_Entry    = (HOOKFUNC)StackSwapDispatch; /* see stackswap.c */
      cl -> cl_Dispatcher . h_SubEntry = (HOOKFUNC)Dispatch;
      cl -> cl_Dispatcher . h_Data     = (APTR)DTSTACKSIZE;           /* see stackswap.c */
      cl -> cl_UserData                = (ULONG)classbase;

      AddClass( cl );
    }

    return( cl );
}

/*****************************************************************************/

#include "methods.h"

/* class dispatcher */
DISPATCHERFLAGS
ULONG Dispatch( REGA0 struct IClass *cl, REGA2 Object *o, REGA1 Msg msg )
{
    struct ClassBase          *classbase = (struct ClassBase *)(cl -> cl_UserData);
    struct MPEGVideoInstData  *mvid;
    ULONG                      retval    = 0UL;

    switch( msg -> MethodID )
    {
      case OM_NEW:
          retval = DT_NewMethod( cl, o, msg );
          break;

      case OM_DISPOSE:
          retval = DT_DisposeMethod( cl, o, msg );
          break;

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

LONG LoadFrames( struct ClassBase *classbase, Object *o )
{
    struct MPEGVideoInstData *mvid  = (struct MPEGVideoInstData *)INST_DATA( (classbase -> cb_Lib . cl_Class), o );
    LONG                      error = 0L;

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    mvid -> mvid_ClassBase = classbase;
    InitSemaphore( (&(mvid -> mvid_SigSem)) );
    NewList( (struct List *)(&(mvid -> mvid_FrameList)) );

    /* Create a memory pool for frame nodes */
    if ((mvid -> mvid_Pool = CreatePool( (MEMF_CLEAR | MEMF_PUBLIC), 1024UL, 1024UL ) ) != NULL)
    {
        D(bug("[mpegvideo.datatype] %s: pool @ 0x%p\n", __func__, mvid -> mvid_Pool));
      /* init state... */
      mvid -> mvid_mpegVidRsrc_first = TRUE;
      mvid -> mvid_IndexScan         = TRUE;

      /* defaults */
      mvid -> mvid_LoadAll       = TRUE;
      mvid -> mvid_BufLength     = 16384UL;
      mvid -> mvid_Volume        = 63UL;
      mvid -> mvid_gammaCorrect  = 1.0;
      mvid -> mvid_chromaCorrect = 1.0;
      mvid -> mvid_ModeID        = (ULONG)INVALID_ID; /* Means: Choose display mode id automatically */

      /* Get file handle, project name and BitMapHeader */
      if( GetDTAttrs( o, DTA_Handle,        (&(mvid -> input)),
                         DTA_Name,          (&(mvid -> mvid_ProjectName)),
                         TAG_DONE ) == 2UL )
      {
        if( mvid -> input )
        {
          /* Read preferences... */
          ReadENVPrefs( classbase, mvid );

          /* Set some defaults */
          if( ditherType == 0 )
          {
            ditherType = HAM_DITHER;
          }

          /* HAM/FULL_COLOR extra handling stuff... */
          switch( ditherType )
          {
            case HAM_DITHER:
            {
                mvid -> mvid_ModeID = HAM;

                /* We only support HAM with 6 or 8 planes... */
                if( (anim_depth != 6UL) && (anim_depth != 8UL) && (anim_depth != 0UL) )
                {
                  error_printf( mvid, "We only support HAM with 6 or 8 planes, depth %lu not supported\n", anim_depth );

                  anim_depth = 0UL; /* use default depth */
                }
            }
                break;

            case FULL_COLOR_DITHER:
            {
                if( (anim_depth != 24UL) )
                {
                  if( (anim_depth != 0UL) && (anim_depth != 32UL) )
                  {
                    error_printf( mvid, "24bit RGB output must have a depth of 24 bit %lu\n", anim_depth );
                  }

                  anim_depth = ((mvid -> mvid_UseChunkyMap)?(32UL):(24UL));
                }
            }
                break;

            case FULL_COLOR_DITHER16:
            {
                if( anim_depth != 16UL )
                {
                  if( anim_depth != 0UL )
                  {
                    error_printf( mvid, "%dbit RGB output must have a depth of 16 bit\n", 16);
                  }

                  anim_depth = 16UL;
                }
            }
                break;
          }

          /* Use default depth... */
          if( anim_depth == 0UL )
          {
            switch( ditherType )
            {
              case HAM_DITHER:
              {
                  if( (GfxBase -> ChipRevBits0) & GFXF_AA_LISA )
                  {
                    anim_depth = 8UL;
                  }
                  else
                  {
                    anim_depth = 6UL;
                  }
              }
                  break;

#if 0
              case FAST_COLOR_DITHER:
#endif
              case ORDERED_DITHER:
                  anim_depth = 8UL;
                  break;

              default:
                  anim_depth = 5UL;
                  break;
            }
          }

          /* Use default #?_RANGE values */
          if( (LUM_RANGE == 0UL) || (CR_RANGE == 0UL) || (CB_RANGE == 0UL) )
          {
            if( LUM_RANGE || CR_RANGE || CB_RANGE )
            {
              error_printf( mvid, "(LUM|CR|CB)_RANGE settings = %d. You must set all three at once !\n", 0);
            }

            switch( anim_depth )
            {
              case 8: LUM_RANGE =  7; CR_RANGE = 6; CB_RANGE = 6; break;
              case 7: LUM_RANGE =  5; CR_RANGE = 5; CB_RANGE = 5; break;
              case 6: LUM_RANGE =  4; CR_RANGE = 4; CB_RANGE = 4; break;
              case 5: LUM_RANGE =  3; CR_RANGE = 3; CB_RANGE = 3; break;
              case 4: LUM_RANGE = 16; CR_RANGE = 1; CB_RANGE = 1; break;
              case 3: LUM_RANGE =  8; CR_RANGE = 1; CB_RANGE = 1; break;
              case 2: LUM_RANGE =  4; CR_RANGE = 1; CB_RANGE = 1; break;
              case 1: LUM_RANGE =  2; CR_RANGE = 1; CB_RANGE = 1; break;
            }
          }

          /* round width */
          if( anim_width )
          {
            /* width must be divisible by 16... */
            anim_width = (anim_width + 15UL) & ~15UL;
          }

          /* round height */
          if( anim_height )
          {
            /* height must be divisible by 16... */
            anim_height = (anim_height + 15) & ~15;
          }

          /* Pop up progress bar, if requested */
          CreateProgressRequester( mvid );

          /* Set "point of return" on decoder exit/error */
          if( setjmp( exit_buf ) == 0 )
          {
            /* The following set up is not needed for direct RGB-coded bitmaps */
            if( anim_depth <= 8UL )
            {
              lum_values = (UBYTE *)mymalloc( mvid, (size_t)(LUM_RANGE * sizeof( UBYTE )) );
                D(bug("[mpegvideo.datatype] %s: lum_values @ 0x%p\n", __func__, lum_values));
              cr_values  = (UBYTE *)mymalloc( mvid, (size_t)(CR_RANGE  * sizeof( UBYTE )) );
                D(bug("[mpegvideo.datatype] %s: cr_values @ 0x%p\n", __func__, cr_values));
              cb_values  = (UBYTE *)mymalloc( mvid, (size_t)(CB_RANGE  * sizeof( UBYTE )) );
                D(bug("[mpegvideo.datatype] %s: cb_values @ 0x%p\n", __func__, cb_values));
              mappixel   =  (LONG *)mymalloc( mvid, (size_t)((MAX( (LUM_RANGE * CR_RANGE * CB_RANGE), 256UL ) + 1UL) * sizeof( LONG )) );
                D(bug("[mpegvideo.datatype] %s: mappixel @ 0x%p\n", __func__, mappixel));
            }

            switch( ditherType )
            {
              case FULL_COLOR_DITHER:
              case FULL_COLOR_DITHER16:
              {
                  /* Set up 24/16 bit color settings */
                  InitColorDither( mvid );
              }
                  break;

              case HAM_DITHER:
              {
                  /* Set up 24 bit color settings */
                  InitColorDither( mvid );

                  InitHAMDisplay( mvid );
              }
                  break;

              case ORDERED_DITHER:
              {
                  InitColor( mvid );
                  InitOrderedDither( mvid );
                  InitDisplay( mvid );
              }
                  break;

              case GRAY_DITHER:
              {
                  InitGrayDisplay( mvid );
              }
                  break;

#if 0
              case FAST_COLOR_DITHER:
              {
                  /* Set up dicecolor requirements (base palette) */
                  InitColor( mvid );
                  InitOrderedDither( mvid );
                  InitDisplay( mvid );

                  /* Set up 24 bit color settings */
                  InitColorDither( mvid );
              }
                  break;
#endif
            }

            loadvideo( mvid );
          }

          /* If we have nothing to load anymore, shut down the decoder here... */
          if( (mvid -> mvid_LoadAll) == TRUE )
          {
            mpeg_closedown( mvid );
          }

          /* We're done with the scan */
          DeleteProgressRequester( mvid );

          /* Check for errors during loading (if there was not IGNOREERRORS switch set) */
          if( ((mvid -> mvid_retval) <= RETURN_WARN) || (mvid -> mvid_IgnoreErrors) )
          {
            verbose_printf( mvid, "width %lu height %lu depth %lu frames %lu tpf %lu\n", anim_width, anim_height, anim_depth, totNumFrames, xtpf );

            /* Anything to work on ? */
            if( anim_width && anim_height && anim_depth && totNumFrames )
            {
              struct ColorRegister *acm;
              ULONG                *acregs;
              ULONG                 nc;

              /* Key bitmap ? */
              if( mvid -> mvid_KeyBitMap )
              {
                ULONG numcolors;

                if( (mvid -> mvid_ModeID) == INVALID_ID )
                {
                  /* BUG: Does currently not support SUPERHIRES modes */
                  if( anim_width >= 640UL )
                  {
                    if( anim_height >= 400 )
                    {
                      mvid -> mvid_ModeID = HIRESLACE_KEY;
                    }
                    else
                    {
                      mvid -> mvid_ModeID = HIRES_KEY;
                    }
                  }
                  else
                  {
                    if( anim_height >= 400 )
                    {
                      mvid -> mvid_ModeID = LORESLACE_KEY;
                    }
                    else
                    {
                      mvid -> mvid_ModeID = LORES_KEY;
                    }
                  }
                }

                numcolors = ((mvid -> mvid_PalettePerFrame)?(1UL << anim_depth):(used_cnt));

                /* Store misc attributes */
                SetDTAttrs( o, NULL, NULL, DTA_ObjName,      (mvid -> mvid_ProjectName),
                                           DTA_NominalHoriz, anim_width,
                                           DTA_NominalVert,  anim_height,
                                           ADTA_ModeID,      (mvid -> mvid_ModeID),
                                           ADTA_Width,       anim_width,
                                           ADTA_Height,      anim_height,
                                           ADTA_Depth,       anim_depth,
                                           ADTA_NumColors,   numcolors,
                                           TAG_DONE );


                /* Get color context */
                if( GetDTAttrs( o,
                                ADTA_ColorRegisters, (&acm),
                                ADTA_CRegs,          (&acregs),
                                ADTA_NumColors,      (&nc),
                                TAG_DONE ) == 3UL )
                {
                  /* All valid ? */
                  if( (acm && acregs && nc) || (numcolors == 0UL) )
                  {
                    ULONG i;

                    /* Copy colors from used_colors array into obj's acregs and acm */
                    for( i = 0UL ; i < nc ; i++ )
                    {
                      ULONG r,
                            b,
                            g;

                      acm[ i ] = used_colors[ i ];

                      /* Replicate the color information.
                       * This surrounds an OS bug which uses the low-oreder bytes of the 32-bit colors
                       * instead of the high order ones
                       */
                      r = used_colors[ i ] . red;
                      g = used_colors[ i ] . green;
                      b = used_colors[ i ] . blue;

                      acregs[ (i * 3) + 0 ] = r * 0x01010101UL;
                      acregs[ (i * 3) + 1 ] = g * 0x01010101UL;
                      acregs[ (i * 3) + 2 ] = b * 0x01010101UL;
                    }

                    /* Set up mvid_FirstFrameNode */
                    mvid -> mvid_FirstFrameNode = (struct FrameNode *)(mvid -> mvid_FrameList . mlh_Head);

                    /* Copy first frame into key bitmap */
                    CopyBitMap( classbase, (mvid -> mvid_KeyBitMap), (mvid -> mvid_FirstFrameNode -> fn_BitMap), (ULONG)anim_width, (ULONG)anim_height );

                    if( (mvid -> mvid_TicksPerFrame) == 0UL )
                    {
                      mvid -> mvid_TicksPerFrame = xtpf;
                    }

                    AttachSample( mvid );

                    /* Set up required attributes */
                    SetDTAttrs( o, NULL, NULL, ADTA_Frames,                            totNumFrames,
                                               XTAG( ISV41,    ADTA_TicksPerFrame   ), (mvid -> mvid_TicksPerFrame),
                                               XTAG( (!ISV41), ADTA_FramesPerSecond ), (TICK_FREQ / (mvid -> mvid_TicksPerFrame)),
                                               ADTA_KeyFrame,                          (mvid -> mvid_KeyBitMap),
                                               ADTA_Sample,                            (mvid -> mvid_FirstFrameNode -> fn_Sample),
                                               ADTA_SampleLength,                      (mvid -> mvid_FirstFrameNode -> fn_SampleLength),
                                               ADTA_Period,                            (mvid -> mvid_FirstFrameNode -> fn_Period),
                                               ADTA_Volume,                            (mvid -> mvid_Volume),
                                               ADTA_Cycles,                            1UL,
                                               TAG_DONE );
                  }
                  else
                  {
                    /* no color table etc. from superclass */
                    error = ERROR_NO_FREE_STORE;
                  }
                }
                else
                {
                  /* can't get required args from superclass */
                  error = ERROR_NO_FREE_STORE;
                }
              }
              else
              {
                /* can't alloc key bitmap */
                error = ERROR_NO_FREE_STORE;
              }
            }
            else
            {
              /* something during load went wrong, we got no frames */
              error = DTERROR_NOT_ENOUGH_DATA;
            }
          }
          else
          {
            /* error during loading */
            error = mvid -> mvid_retval2;
          }
        }
        else
        {
          /* no fh */
          error = ERROR_REQUIRED_ARG_MISSING;
        }
      }
      else
      {
        /* can't get required attributes from superclass */
        error = ERROR_REQUIRED_ARG_MISSING;
      }
    }
    else
    {
      /* no memory pool */
      error = ERROR_NO_FREE_STORE;
    }
    
    return( error );
}


struct FrameNode *AllocFrameNode( struct ClassBase *classbase, APTR pool )
{
    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    return( (struct FrameNode *)AllocPooled( pool, (ULONG)sizeof( struct FrameNode ) ) );
}


void FreeFrameNode( struct MPEGVideoInstData *mvid, struct FrameNode *fn )
{
    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    if( fn )
    {
      FreeFrameBitMap( mvid, (fn -> fn_BitMap) );

      FreeColorMap( (fn -> fn_CMap) );

      FreePooled( (mvid -> mvid_Pool), (APTR)fn, (ULONG)sizeof( struct FrameNode ) );
    }
}


struct FrameNode *FindFrameNode( struct MinList *fnl, ULONG timestamp )
{
    D(bug("[mpegvideo.datatype] %s()\n", __func__));

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


struct FrameNode *FindNextIFrame( struct FrameNode *fn )
{
    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    if( fn )
    {
      struct FrameNode *worknode,
                       *nextnode;

      if ((worknode = (struct FrameNode *)(fn -> fn_Node . mln_Succ) ) != NULL)
      {
        while ((nextnode = (struct FrameNode *)(worknode -> fn_Node . mln_Succ) ) != NULL)
        {
          if( (worknode -> fn_IFrame) == NULL )
          {
            return( worknode );
          }

          worknode = nextnode;
        }
      }
    }

    return( NULL );
}


#if !defined(__AROS__)
void mysprintf( struct ClassBase *classbase, STRPTR buffer, STRPTR fmt, ... )
{
    APTR args;

    args = (APTR)((&fmt) + 1);

    RawDoFmt( fmt, args, (void (*))"\x16\xc0\x4e\x75", buffer );
}


void verbose_printf( struct MPEGVideoInstData *mvid, STRPTR format, ... )
{
    va_list args;

    if( mvid -> mvid_VerboseOutput )
    {
        va_start (args, format);
        VFPrintf( (mvid -> mvid_VerboseOutput), format, (const IPTR *)args);
        va_end (args);
    }
}


void debug_printf( struct MPEGVideoInstData *mvid, STRPTR format, ... )
{
    va_list args;

    if( (mvid -> mvid_VerboseOutput) && (mvid -> mvid_DoDebug) )
    {
        va_start (args, format);
        VFPrintf( (mvid -> mvid_VerboseOutput), format, (const IPTR *)args);
        va_end (args);
    }
}


void syntax_printf( struct MPEGVideoInstData *mvid, STRPTR format, ... )
{
    va_list args;

    if( (mvid -> mvid_VerboseOutput) && (mvid -> mvid_DoSyntax) )
    {
        va_start (args, format);
        VFPrintf( (mvid -> mvid_VerboseOutput), format, (const IPTR *)args);
        va_end (args);
    }
}


void error_printf( struct MPEGVideoInstData *mvid, STRPTR format, ... )
{
    struct ClassBase *classbase = mvid -> mvid_ClassBase;
    va_list args;

    OpenLogfile( classbase, mvid );

    if( mvid -> mvid_VerboseOutput )
    {
        va_start (args, format);
        VFPrintf( (mvid -> mvid_VerboseOutput), format, (const IPTR *)args);
        va_end (args);
    }
}
#endif

static
void CopyBitMap( struct ClassBase *classbase, struct BitMap *dest, struct BitMap *src, ULONG width, ULONG height )
{
    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    if( dest && src )
    {
      if( CyberGfxBase )
      {
        /* Assumption: If a non-planar bitmap occurs BltBitMap should be able
         * to blit it into a planar one
         */
        BltBitMap( src, 0L, 0L, dest, 0L, 0L, width, height, 0xC0UL, 0xFFUL, NULL );

        WaitBlit();
      }
      else
      {
        ULONG planesize = (ULONG)(dest -> BytesPerRow) * (ULONG)(dest -> Rows);
        UWORD i;

        for( i = 0U ; i < (dest -> Depth) ; i++ )
        {
          CopyMem( (src -> Planes[ i ]), (dest -> Planes[ i ]), planesize );
        }
      }
    }
}


static
struct BitMap *AllocFastBitMap( struct MPEGVideoInstData *mvid, ULONG width, ULONG height, ULONG depth )
{
    struct BitMap    *bm;
    ULONG             planesize,
                      moredepthsize,
                      size;

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    planesize      = (ULONG)RASSIZE( width, height ) + 64UL;
    moredepthsize  = (depth > 8UL)?((depth - 8UL) * sizeof( PLANEPTR )):(0UL);
    size           = ((ULONG)sizeof( struct BitMap )) + moredepthsize + (planesize * depth);

#if !defined(__AROS__)
    if( mvid -> mvid_UseVMM )
        bm = (struct BitMap *)AllocVVec( size, 0UL );
    else
#endif
      bm = (struct BitMap *)AllocVec( size, MEMF_PUBLIC );

    if( bm )
    {
      PLANEPTR plane;
      UWORD    pl;

      InitBitMap( bm, depth, width, height );

      plane = (PLANEPTR)(((UBYTE *)(bm + 1)) + moredepthsize); /* First plane follows struct BitMap */

      /* Set up plane data */
      pl = 0U;

      /* Set up plane ptrs */
      while( pl < depth )
      {
        bm -> Planes[ pl ] = plane;

        plane = (PLANEPTR)(((UBYTE *)plane) + planesize);
        pl++;
      }

      /* Clear the remaining plane ptrs (only of standart 8 plane struct bitmap) */
      while( pl < 8U )
      {
        bm -> Planes[ pl ] = NULL;

        pl++;
      }
    }

    return( bm );
}


struct BitMap *AllocFrameBitMap( struct MPEGVideoInstData *mvid )
{
    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    if( mvid -> mvid_UseChunkyMap )
    {
      return( AllocBitMap( (ULONG)anim_width, (ULONG)anim_height, (ULONG)anim_depth, (BMF_SPECIALFMT | SHIFT_PIXFMT( pixfmt )), NULL ) );
    }
    else
    {
      return( AllocFastBitMap( mvid, (ULONG)anim_width, (ULONG)anim_height, (ULONG)anim_depth ) );
    }
}


void FreeFrameBitMap( struct MPEGVideoInstData *mvid, struct BitMap *bm )
{
    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    if( bm )
    {
      if( mvid -> mvid_UseChunkyMap )
      {
        FreeBitMap( bm );
      }
      else
      {
#if !defined(__AROS__)
        if( mvid -> mvid_UseVMM )
          FreeVVec( (APTR)bm );
        else
#endif
          FreeVec( (APTR)bm );
      }
    }
}


/****** mpegvideo.datatype/preferences ***************************************
*
*   NAME
*       preferences
*
*   DESCRIPTION
*       The "ENV:Classes/DataTypes/mpegvideo.prefs" file contains global
*       settings for the datatype.
*       The preferences file is an ASCII file containing one line where the
*       preferences can be set.
*       It can be superset by a local variable with the same name.
*
*       Each line can contain settings, special settings for some projects
*       can be set using the MATCHPROJECT option.
*       Lines beginning with a '#' or ';' chars are treated as comments.
*       Lines are limitted to 256 chars.
*
*   TEMPLATE
*       MATCHPROJECT/K,MODEID/K/N,WIDTH/K/N,HEIGHT/K/N,DEPTH/K/N,DITHER/K,
*       LUM_RANGE/K/N,CR_RANGE/K/N,CB_RANGE/K/N,COLORERROR/K/N,
*       PALETTEPERFRAME/S,NOPALETTEPERFRAME/S,GAMMACORRECT/K,CHROMACORRECT/K,
*       MAXFRAME/K/N,SKIPFRAMES/K/N,FPS/K/N,PFRAMES/S,NOPFRAMES/S,BFRAMES/S,
*       NOBFRAMES/S,SAMPLE/K,VOLUME/K/N,BUFFER/K/N,LOADALL/S,NOLOADALL/S,
*       USEVMM/S,MINTOTALMEM/K/N,IGNOREERRORS/S,VERBOSE/S,PROGRESSGAUGE/S,
*       NOPROGRESSGAUGE/S,QUALITY/S,NOQUALITY/S
*
*       MATCHPROJECT -- The settings in this line belongs only to this
*           project(s), e.g. if the case-insensitive pattern does not match,
*           this line is ignored.
*           The maximum length of the pattern is 128 chars.
*           Defaults to #?, which matches any project.
*
*       MODEID -- Select screen mode id of datatype (will be stored in
*           ADTA_ModeID). Note that the DOS ReadArgs function used for parsing
*           fetches a SIGNED long. The bit 31 will be represented by minus
*           '-'. (example: "MODEID=266240" sets the mode to the A2024 screen
*           mode id)
*           Defaults to -1, which means: Use the best screenmode available
*           for the given width, height and depth.
*
*       WIDTH -- Set the animation's width. The video will be scaled to this
*           width.
*           Defaults to 0, which means: Use video's width.
*
*       HEIGHT -- Set the animation's height. The video will be scaled to this
*           height.
*           Defaults to 0, which means: Use video's height.
*
*       DEPTH -- depth for the selected scaling mode
*           A value describing the "depth" of the animation.
*           1 upto 8 are valid, under- or overflows will be truncated
*           to the maximum supported.
*           The default depth will be selected by the DITHER mode.
*
*       DITHER -- dither type, one of
*           GRAY            -- grayscale output
*           HAM             -- ham ham6/ham8 etc. (default)
*           EHB             -- extra halfbright                     (n/a)
*           COLOR           -- color output
*           ORDERED         -- ordered dither
*           24BITCHUNKY     -- true color, 24 bit
*           16BITCHUNKY     -- true color, 16 bit
*           24BITPLANAR     -- true color, 24 bit
*
*           GRAY is the grayscale mode.
*               If no depth is given, the depth default to 5 (32 colors).
*
*           HAM (hold-and-modify) mode: Either HAM6 or HAM8, set by the depth
*               option. If no depth is given, this defaults to 6 for OCS/ECS
*               and 8 for AGA/AAA machines.
*               This is the default dither mode if no DITHER option is given.
*
*           EHB (extra halfbright mode):
*               If no depth is given, the depth default to 6 (32/64 colors).
*               not implemented yet
*
*           COLOR remaps the frames into a fixed color space.
*               If no depth is given, the depth default to 8 (256 colors).
*
*           ORDERED uses ordered dithering.
*               If no depth is given, the depth default to 8 (256 colors).
*
*           24BITCHUNKY uses chunkypixel CyberGFX bitmaps
*               Fixed to a depth of 32 (XRGB).
*               Requires at least animation.datatype V41.3.
*
*           16BITCHUNKY uses chunkypixel CyberGFX bitmaps
*               Fixed to a depth of 16 (XRGB).
*               Requires at least animation.datatype V41.3.
*
*           24BITPLANAR uses a planar 24 bit depth bitmap (non-interleaved)
*               Fixed to a depth of 24 (RGB).
*               Requires at least animation.datatype V41.3.
*
*           Defaults to HAM.
*
*       LUM_RANGE -- sets the number of colors assigned to the luminance
*           component when dithering the image.  The product of LUM_RANGE,
*           CR_RANGE and CB_RANGE should be less than the number of colors
*           selected by the DEPTH option.
*           This will only affect ORDERED and COLOR dithering and the base
*           palette of the DICECOLOR remapping.
*           Any value between 1 upto 255 is allowed, 0 is treated as 1.
*
*           Defaults: see DEFAULTS section below
*
*       CR_RANGE -- sets the number of colors assigned to the red component of
*           the chrominace range when dithering the image.  The product of
*           LUM_RANGE, CR_RANGE and CB_RANGE should be less than the number of
*           colors selected by the DEPTH option.
*           This will only affect ORDERED and COLOR dithering and the base
*           palette of the DICECOLOR remapping.
*           Any value between 1 upto 255 is allowed, 0 is treated as 1.
*
*           Defaults: see DEFAULTS section below
*
*       CB_RANGE -- sets the number of colors assigned to the blue component
*           of the chrominace range when dithering the image.  The product of
*           LUM_RANGE, CR_RANGE and CB_RANGE should be less than the number of
*           colors selected by the DEPTH option.
*           This will only affect ORDERED and COLOR dithering and the base
*           palette of the DICECOLOR remapping.
*           Any value between 1 upto 255 is allowed, 0 is treated as 1.
*
*           Defaults: see DEFAULTS section below
*
*       COLORERROR -- Set the error range when allocating colors.
*           The error range is used for color matching
*           (like this: if( ABS( (pixel . red) - (colormap . red) ) +
*                       ABS( <dito. green> ) + ABS( <dito. blue> )
*                       < colorerror ) then use this color index).
*           A low value (0) means high quality remapping and slow remapping,
*           high values (50) means low quality (using less color indexes).
*           The value set here will also affect DICECOLOR remapping, because
*           the search algorithm is the same.
*
*           Defaults to 0.
*
*       PALETTEPERFRAME -- Create a own palette for each frame.
*           __Currently__ a NOP for HAM dithering and always a NOP
*           for all direct-RGB output modes (24BITCHUNKY, 16BITCHUNKY, 
*           24BITPLANAR).
*
*           Note that this option requires animation.datatype V41 to work.
*
*       NOPALETTERPERFRAME -- Turns PALETTEPERFRAME switch off.
*
*       GAMMACORRECT -- Gamma correction value (defined as fixed point
*           number).
*           Defaults to "1.0".
*
*       CHROMACORRECT -- Chroma correction value (defined as fixed point
*           number).
*           Defaults to "1.0".
*
*       MAXFRAME -- Maximum number of frames to load.
*           Defaults to 0, which means: Load all frames.
*
*       SKIPFRAMES -- Load only the n-th frame of an animation.
*           The internal timing (e.g. time code) is not affected, so
*           the FPS value will be correct.
*           Defaults to 0 which means: Skip no frame.
*
*           Note that this option requires animation.datatype V41 to work
*           properly.
*
*       FPS -- frames per second
*           Defaults to 0, which means: overtake fps rate from video stream.
*           Setting the FPS value also affects an attched sound. The period
*           of the sample (e.g. the playback speed) will everytimes as long
*           as the frame is displayed.
*
*       PFRAMES -- Turns off the NOPFRAMES option.
*           Default is on.
*
*       NOPFRAMES -- ignore any type P frames (predicted frames) when loading.
*
*       BFRAMES -- Turns off the NOBFRAMES option.
*           Default is on.
*
*       NOBFRAMES -- ignore any type B frames (bidirectional frames) when
*           loading.
*           Default is off.
*
*       SAMPLE -- Attach the given sample to the animation. The sample will
*           be loaded using datatypes (GID_SOUND).
*           Only one sample can be attached to one video stream, any following
*           attempt to attach the sample will be ignored.
*
*           Default: no sample
*
*       VOLUME -- Volume of the sound when playing.
*           Defaults to 63, which is the maximum. A value greater than 64 will
*           be set to 63.
*
*       BUFFER -- read buffers size. Minimum is 2048, lower values are set to
*           2048.
*           Defaults to 16384.
*
*       LOADALL -- load all frames before displaying it.
*
*       NOLOADALL -- turns off LOADALL switch.
*
*       USEVMM -- Use Martin Apel's vmm.library for bitmaps.
*           The verbose output will tell you if VMM memory will be used or
*           not.
*           This option is useless if CyberGFX bitmaps are used !
*           Default is off.
*
*       NOUSEVMM -- Turn VMM usage for bitmaps off.
*
*       MINTOTALMEM -- Minimum total memory available. If less memory
*           available, abort loading.
*           Defaults to 0, which means: Don't use this option.
*
*       IGNOREERRORS -- Ignore errors while parsing/decoding etc.
*           Usefull if a syntax error or read error (which may occur with
*           some old, buggy CD filesystems) happens.
*           Default is off.
*
*       VERBOSE -- Verbose output. Prints out current frame etc., some
*           statistical information and maybe, debugging infos.
*           Verbose output will be printed in a console window
*           ("CON://///auto/wait/close/inactive/MPEG Video DataType").
*           Default is off.
*
*       PROGRESSGAUGE -- Display a load progress gauge.
*           Default is on.
*
*       NOPROGRESSGAUGE -- Disables the progress gauge which is displayed
*           during loading of the mpeg stream.
*           Default is off.
*
*       QUALITY -- If set, mpegvideo.datatype uses floating-point dct
*           decoding, which results in a better output quality.
*           Default is off.
*
*       NOQUALITY -- Turns QUALITY switch off.
*           Default is on.
*
*   DEFAULTS
*       Defaults for the options are noted above.
*
*       LUM_RANGE, CR_RANGE, CB_RANGE options have defaults based on the
*       depth:
*
*       depth | LUM_RANGE | CR_RANGE | CB_RANGE | comment
*       ------+-----------+----------+----------+----------------------
*          8  |         7 |        6 |       6  | color output
*          7  |         5 |        5 |       5  | color output
*          6  |         4 |        4 |       4  | color output
*          5  |         3 |        3 |       3  | color output
*          4  |        16 |        1 |       1  | grayscale output
*          3  |         8 |        1 |       1  | grayscale output
*          2  |         4 |        1 |       1  | grayscale output
*          1  |         2 |        1 |       1  | black & white output
*
*   NOTE
*       An invalid prefs file line will force the default settings for this
*       line and the VERBOSE option.
*
*   BUGS
*       - Low memory may cause that the prefs file won't be parsed.
*
*       - Lines are limitted to 256 chars
*
******************************************************************************
*
*/


static
STRPTR GetPrefsVar( struct ClassBase *classbase, STRPTR name )
{
          STRPTR buff;
    const ULONG  buffsize = 16UL;

    if ((buff = (STRPTR)AllocVec( (buffsize + 2UL), (MEMF_PUBLIC | MEMF_CLEAR) ) ) != NULL)
    {
      if( GetVar( name, buff, buffsize, GVF_BINARY_VAR ) != (-1L) )
      {
        ULONG varsize = IoErr();

        varsize += 2UL;

        if( varsize > buffsize )
        {
          FreeVec( buff );

          if ((buff = (STRPTR)AllocVec( (varsize + 2UL), (MEMF_PUBLIC | MEMF_CLEAR) ) ) != NULL)
          {
            if( GetVar( name, buff, varsize, GVF_BINARY_VAR ) != (-1L) )
            {
              return( buff );
            }
          }
        }
        else
        {
          return( buff );
        }
      }

      FreeVec( buff );
    }

    return( NULL );
}


static
BOOL matchstr( struct ClassBase *classbase, STRPTR pat, STRPTR s )
{
    TEXT buff[ 512 ];

    if( pat && s )
    {
      if( ParsePatternNoCase( pat, buff, (sizeof( buff ) - 1) ) != (-1L) )
      {
        if( MatchPatternNoCase( buff, s ) )
        {
          return( TRUE );
        }
      }
    }

    return( FALSE );
}


static
void ReadENVPrefs( struct ClassBase *classbase, struct MPEGVideoInstData *mvid )
{
    struct RDArgs envvarrda =
    {
        { NULL, 256L, 0L },
        0L,
        NULL,
        0L,
        NULL,
        RDAF_NOPROMPT
    };

    struct
    {
      STRPTR  matchproject;
      IPTR    *modeid;
      IPTR    *width;
      IPTR    *height;
      IPTR    *depth;
      STRPTR  dither;
      IPTR    *lum_range;
      IPTR    *cr_range;
      IPTR    *cb_range;
      IPTR    *colorerror;
      IPTR    paletteperframe;
      IPTR    nopaletteperframe;
      STRPTR  gammacorrect;
      STRPTR  chromacorrect;
      IPTR    *maxframe;
      IPTR    *skipframes;
      IPTR    *fps;
      IPTR    pframes;
      IPTR    nopframes;
      IPTR    bframes;
      IPTR    nobframes;
      STRPTR  sample;
      IPTR    *volume;
      IPTR    *buflength;
      IPTR    loadall;
      IPTR    noloadall;
      IPTR    usevmm;
      IPTR    nousevmm;
      IPTR    *mintotalmem;
      IPTR    ignoreerrors;
      IPTR    verbose;
      IPTR    progressgauge;
      IPTR    noprogressgauge;
      IPTR    quality;
      IPTR    noquality;
    } animargs;

    TEXT   varbuff[ 258 ];
    STRPTR var;

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    if ((var = GetPrefsVar( classbase, "Classes/DataTypes/mpegvideo.prefs" ) ) != NULL)
    {
      STRPTR prefsline      = var,
             nextprefsline;
      ULONG  linecount      = 1UL;

      /* Be sure that "var" contains at least one break-char */
      strcat( var, "\n" );

      while ((nextprefsline = strpbrk( prefsline, "\n" ) ) != NULL)
      {
        stccpy( varbuff, prefsline, (int)MIN( (sizeof( varbuff ) - 2UL), (((ULONG)(nextprefsline - prefsline)) + 1UL) ) );

        /* be sure that this line isn't a comment line or an empty line */
        if( (varbuff[ 0 ] != '#') && (varbuff[ 0 ] != ';') && (varbuff[ 0 ] != '\n') && (strlen( varbuff ) > 2UL) )
        {
          /* Prepare ReadArgs processing */
          strcat( varbuff, "\n" );                                       /* Add NEWLINE-char            */
          envvarrda . RDA_Source . CS_Buffer = varbuff;                  /* Buffer                      */
          envvarrda . RDA_Source . CS_Length = strlen( varbuff ) + 1UL;  /* Set up input buffer length  */
          envvarrda . RDA_Source . CS_CurChr = 0L;
          envvarrda . RDA_Buffer = NULL;
          envvarrda . RDA_BufSiz = 0L;
          memset( (void *)(&animargs), 0, sizeof( animargs ) );          /* Clear result array          */

          if( ReadArgs( "MATCHPROJECT/K,"
                        "MODEID/K/N,"
                        "WIDTH/K/N,"
                        "HEIGHT/K/N,"
                        "DEPTH/K/N,"
                        "DITHER/K,"
                        "LUM_RANGE/K/N,"
                        "CR_RANGE/K/N,"
                        "CB_RANGE/K/N,"
                        "COLORERROR/K/N,"
                        "PALETTEPERFRAME/S,"
                        "NOPALETTEPERFRAME/S,"
                        "GAMMACORRECT/K,"
                        "CHROMACORRECT/K,"
                        "MAXFRAME/K/N,"
                        "SKIPFRAMES/K/N,"
                        "FPS/K/N,"
                        "PFRAMES/S,"
                        "NOPFRAMES/S,"
                        "BFRAMES/S,"
                        "NOBFRAMES/S,"
                        "SAMPLE/K,"
                        "VOLUME/K/N,"
                        "BUFFER/K/N,"
                        "LOADALL/S,"
                        "NOLOADALL/S,"
                        "USEVMM/S,"
                        "NOUSEVMM/S,"
                        "MINTOTALMEM/K/N,"
                        "IGNOREERRORS/S,"
                        "VERBOSE/S,"
                        "PROGRESSGAUGE/S,"
                        "NOPROGRESSGAUGE/S,"
                        "QUALITY/S,"
                        "NOQUALITY/S", (IPTR *)(&animargs), (&envvarrda) ) )
          {
            BOOL noignore = TRUE;

            if( (animargs . matchproject) && (mvid -> mvid_ProjectName) )
            {
              noignore = matchstr( classbase, (animargs . matchproject), (mvid -> mvid_ProjectName) );
            }

            if( noignore )
            {
              if( animargs . verbose )
              {
                OpenLogfile( classbase, mvid );
              }

              if( animargs . modeid )
              {
                mvid -> mvid_ModeID = *(animargs . modeid);
              }

              if( animargs . width )
              {
                anim_width = *(animargs . width);
              }

              if( animargs . height )
              {
                anim_height = *(animargs . height);
              }

              if( animargs . depth )
              {
                anim_depth = *(animargs . depth);

                /* check bounds */
                if( anim_depth < 1UL )
                {
                  anim_depth = 1UL;
                }

                if( anim_depth > 8UL )
                {
                  anim_depth = 8UL;
                }
              }

              if( animargs . dither )
              {
                LONG d;

                d = FindArg( "GRAY/S,"
                             "HAM/S,"
                             "COLOR/S,"
                             "HYBRID/S,"
                             "FS2/S,"
                             "FS4/S,"
                             "ORDERED/S,"
                             "24BITCHUNKY/S,"
                             "16BITCHUNKY/S,"
                             "24BITPLANAR/S", (animargs . dither) );

                switch( d )
                {
                  case 1: /* HAM */
                  {
                      ditherType          = HAM_DITHER;
                  }
                      break;

#if 0
                  case 2: /* COLOR */
                  {
                      ditherType = FAST_COLOR_DITHER;
                  }
                      break;
#endif

                  case 6:
                      ditherType = ORDERED_DITHER;
                      break;

                  case 7:
                  {
                      /* Check if we have animation.datatype V41 (or higher) as superclass */
                      if( ISV41 )
                      {
                        /* Check here if we opened the cybergraphics.library. After this point, I'll assume
                         * that (mvid_UseChunkyMap == TRUE) implies a opened CyberGfxBase !!
                         */
                        if( CyberGfxBase )
                        {
                          ditherType = FULL_COLOR_DITHER;
                          pixfmt = PIXFMT_ARGB32;
                          mvid -> mvid_UseChunkyMap = TRUE;
                        }
                        else
                        {
                          error_printf( mvid, "no cybergraphics.library available, can't output a %d bit chunky map\n", 24);
                        }
                      }
                      else
                      {
                        error_printf( mvid, "Requires at least animation.datatype V%d for non-planar bitmap support\n", 41);
                      }
                  }
                      break;

                  case 8:
                  {
                      /* Check if we have animation.datatype V41 (or higher) as superclass */
                      if( ISV41 )
                      {
                        /* Check here if we opened the cybergraphics.library. After this point, I'll assume
                         * that (mvid_UseChunkyMap == TRUE) implies a opened CyberGfxBase !!
                         */
                        if( CyberGfxBase )
                        {
                          ditherType = FULL_COLOR_DITHER16;
                          pixfmt = PIXFMT_RGB16;
                          mvid -> mvid_UseChunkyMap = TRUE;
                        }
                        else
                        {
                          error_printf( mvid, "no cybergraphics.library available, can't output a %s bit chunky map\n", 16);
                        }
                      }
                      else
                      {
                        error_printf( mvid, "Requires at least animation.datatype V%d for non-planar bitmap support\n", 41);
                      }
                  }
                      break;

                  case 9:
                  {
                      /* Check if we have animation.datatype V41 (or higher) as superclass */
                      if( ISV41 )
                      {
                        ditherType = FULL_COLOR_DITHER;
                        mvid -> mvid_UseChunkyMap = FALSE;
                      }
                      else
                      {
                        error_printf( mvid, "Requires at least animation.datatype V%d for 24 bit planar bitmap support\n", 41);
                      }
                  }
                      break;

                  case 0:
                  default:
                      ditherType = GRAY_DITHER;
                      break;

                }
              }

              if( animargs . lum_range )
              {
                LUM_RANGE = *(animargs . lum_range);

                if( LUM_RANGE < 1UL ) LUM_RANGE = 1UL;
              }

              if( animargs . cr_range )
              {
                CR_RANGE = *(animargs . cr_range);

                if( CR_RANGE < 1UL ) CR_RANGE = 1UL;
              }

              if( animargs . cb_range )
              {
                CB_RANGE = *(animargs . cb_range);

                if( CB_RANGE < 1UL ) CB_RANGE = 1UL;
              }

              if( animargs . colorerror )
              {
                mvid -> mvid_ColorError = *(animargs . colorerror);
              }

              if( animargs . paletteperframe )
              {
                mvid -> mvid_PalettePerFrame = TRUE;
              }

              if( animargs . nopaletteperframe )
              {
                mvid -> mvid_PalettePerFrame = FALSE;
              }
              
              if( animargs . gammacorrect )
              {
                mvid -> mvid_gammaCorrect = strtod( (animargs . gammacorrect), NULL );

                if( (mvid -> mvid_gammaCorrect) == 0.0 )
                {
                  error_printf( mvid, "Illegal gamma correction value %s, restored to 1.0\n", (animargs . gammacorrect) );

                  mvid -> mvid_gammaCorrect = 1.0;
                }
              }

              if( animargs . chromacorrect )
              {
                mvid -> mvid_chromaCorrect = strtod( (animargs . chromacorrect), NULL );

                if( (mvid -> mvid_chromaCorrect) == 0.0 )
                {
                  error_printf( mvid, "Illegal chroma correction value %s, restored to 1.0\n", (animargs . chromacorrect) );

                  mvid -> mvid_chromaCorrect = 1.0;
                }
              }

              if( animargs . maxframe )
              {
                mvid -> mvid_MaxFrame = *(animargs . maxframe);
              }

              if( animargs . skipframes )
              {
                mvid -> mvid_SkipFrames = *(animargs . skipframes);
              }

              if( animargs . fps )
              {
                mvid -> mvid_TicksPerFrame = TICK_FREQ / MAX( (*(animargs . fps)), 1L );
              }

              if( animargs . pframes )
              {
                mvid -> No_P_Flag = FALSE;
              }

              if( animargs . nopframes )
              {
                mvid -> No_P_Flag = TRUE;
              }

              if( animargs . bframes )
              {
                mvid -> No_B_Flag = FALSE;
              }

              if( animargs . nobframes )
              {
                mvid -> No_B_Flag = TRUE;
              }

              if( (animargs . sample) && ((mvid -> mvid_Sample) == NULL) )
              {
                Object *so;
                LONG    ioerr = 0L;

                verbose_printf( mvid, "loading sample \"%s\"...\n", (animargs . sample) );

                if( ( so = NewDTObject( (animargs . sample), DTA_GroupID, GID_SOUND, TAG_DONE ) ) )
                {
                  BYTE *sample;
                  ULONG length;
                  ULONG period;

                  /* Get sample data from object */
                  if( GetDTAttrs( so, SDTA_Sample,       (&sample),
                                      SDTA_SampleLength, (&length),
                                      SDTA_Period,       (&period),
                                      TAG_DONE ) == 3UL )
                  {
                    if ((mvid -> mvid_Sample = (STRPTR)AllocPooled( (mvid -> mvid_Pool), (length + 1UL) ) ) != NULL)
                    {
                      /* Copy sample and context */
                      CopyMem( (APTR)sample, (APTR)(mvid -> mvid_Sample), length );
                      mvid -> mvid_SampleLength = length;
                      mvid -> mvid_Period       = period;
                    }
                    else
                    {
                      /* Can't alloc sample */
                      ioerr = ERROR_NO_FREE_STORE;
                    }
                  }
                  else
                  {
                    /* Object does not support the requested attributes */
                    ioerr = ERROR_OBJECT_WRONG_TYPE;
                  }

                  DisposeDTObject( so );
                }
                else
                {
                  ioerr = IoErr();
                }

                if( (mvid -> mvid_Sample) == NULL )
                {
                  TEXT errbuff[ 256 ];

                  if( ioerr >= DTERROR_UNKNOWN_DATATYPE )
                  {
                    mysprintf( classbase, errbuff, GetDTString( ioerr ), (animargs . sample) );
                  }
                  else
                  {
                    Fault( ioerr, (animargs . sample), errbuff, sizeof( errbuff ) );
                  }

                  error_printf( mvid, "can't load sample: \"%s\" line %lu\n", errbuff, linecount );
                }
              }

              if( animargs . volume )
              {
                mvid -> mvid_Volume = *(animargs . volume);

                if( (mvid -> mvid_Volume) > 64UL )
                {
                  mvid -> mvid_Volume = 64UL;
                }
              }
              else
              {
                mvid -> mvid_Volume = 64UL;
              }

              if( animargs . buflength )
              {
                mvid -> mvid_BufLength = *(animargs . buflength);

                if( (mvid -> mvid_BufLength) < 2048UL )
                {
                  mvid -> mvid_BufLength = 2048UL;
                }
              }

              if( animargs . loadall )
              {
                mvid -> mvid_LoadAll = TRUE;
              }

              if( animargs . noloadall )
              {
                mvid -> mvid_LoadAll = FALSE;
              }

#if !defined(__AROS__)
              if( animargs . usevmm )
              {
                /* vmm.library useable ? */
                mvid -> mvid_UseVMM = AttemptOpenVMM( mvid );
              }

              if( animargs . nousevmm )
              {
#endif
                mvid -> mvid_UseVMM = FALSE;
#if !defined(__AROS__)
              }
#endif

              if( animargs . mintotalmem )
              {
                mvid -> mvid_MinTotalMem = *(animargs . mintotalmem);
              }

              if( animargs . ignoreerrors )
              {
                mvid -> mvid_IgnoreErrors = TRUE;
              }

              if( animargs . progressgauge )
              {
                mvid -> mvid_PR . pr_Max = (LONG)0UL;
              }

              if( animargs . noprogressgauge )
              {
                mvid -> mvid_PR . pr_Max = (LONG)~0UL;
              }

              if( animargs . quality )
              {
                mvid -> mvid_Quality = TRUE;
              }

              if( animargs . noquality )
              {
                mvid -> mvid_Quality = FALSE;
              }
            }
            else
            {
              verbose_printf( mvid, "prefs line %lu ignored\n", linecount );
            }

            FreeArgs( (&envvarrda) );
          }
          else
          {
            LONG ioerr = IoErr();
            TEXT errbuff[ 256 ];

            OpenLogfile( classbase, mvid );

            Fault( ioerr, "Classes/DataTypes/mpegvideo.prefs", errbuff, sizeof( errbuff ) );

            error_printf( mvid, "preferences \"%s\" line %lu\n", errbuff, linecount );
          }
        }

        prefsline = ++nextprefsline;
        linecount++;
      }

      FreeVec( var );
    }
}


static
void AttachSample( struct MPEGVideoInstData *mvid )
{
    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    if( mvid -> mvid_Sample )
    {
      struct FrameNode *worknode,
                       *nextnode;

      ULONG             period          = ((mvid -> mvid_Period) * (TICK_FREQ / xtpf)) / (TICK_FREQ / (mvid -> mvid_TicksPerFrame));
      ULONG             samplesperframe = (((SysBase -> ex_EClockFrequency) * 10UL) / (period * (TICK_FREQ / (mvid -> mvid_TicksPerFrame)) * 2UL));
      BYTE             *sample          = mvid -> mvid_Sample;

      verbose_printf( mvid, "Attching samples (sysclock %lu period %lu fps %lu length %lu samplesperframe %lu)...\n",
                      (SysBase -> ex_EClockFrequency), period, (TICK_FREQ / (mvid -> mvid_TicksPerFrame)), (mvid -> mvid_SampleLength), samplesperframe );

      worknode = (struct FrameNode *)(mvid -> mvid_FrameList . mlh_Head);

      while ((nextnode = (struct FrameNode *)(worknode -> fn_Node . mln_Succ) ) != NULL)
      {
        worknode -> fn_Sample       = sample;
        worknode -> fn_SampleLength = samplesperframe * ((worknode -> fn_Duration) + 1UL);
        worknode -> fn_Period       = period;

        sample += worknode -> fn_SampleLength;

        /* End of sample reached ? */
        if( (ULONG)(sample - (mvid -> mvid_Sample)) > (mvid -> mvid_SampleLength) )
        {
          /* Cut last size of sample to fit */
          worknode -> fn_SampleLength -= (ULONG)(sample - (mvid -> mvid_Sample));

          break;
        }

        worknode = nextnode;
      }
    }
}

#if !defined(__AROS__)
static
BOOL AttemptOpenVMM( struct MPEGVideoInstData *mvid )
{
    struct ClassBase *classbase = mvid -> mvid_ClassBase;

    ObtainSemaphore( (&(classbase -> cb_Lock)) );

    if( (classbase -> cb_VMMBase) == NULL )
    {
      /* vmm.library will be closed in LibExpunge */
      classbase -> cb_VMMBase = OpenLibrary( "vmm.library", 3UL );
    }

    /* vmm.library loaded ? */
    if( classbase -> cb_VMMBase )
    {
      verbose_printf( mvid, "*** using \"vmm.library\" virtual memory\n" );
    }
    else
    {
      error_printf( mvid, "*** \"vmm.library\" version %d not found\n", 3);
    }

    ReleaseSemaphore( (&(classbase -> cb_Lock)) );

    return( MAKEBOOL( (classbase -> cb_VMMBase) ) );
}
#endif

void OpenLogfile( struct ClassBase *classbase, struct MPEGVideoInstData *mvid )
{
    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    if( (mvid -> mvid_VerboseOutput) == BNULL )
    {
      STRPTR confile;

      if ((confile = (STRPTR)AllocVec( (((mvid -> mvid_ProjectName)?(strlen( (mvid -> mvid_ProjectName) )):(0UL)) + 100UL), MEMF_PUBLIC ) ) != NULL)
      {
        mysprintf( classbase, confile, "CON:////MPEG Video DataType %s/auto/wait/close/inactive",
                   ((mvid -> mvid_ProjectName)?(FilePart( (mvid -> mvid_ProjectName) )):(NULL)) );

        mvid -> mvid_VerboseOutput = Open( confile, MODE_READWRITE );

        FreeVec( confile );
      }
    }
}


static
void CreateProgressRequester( struct MPEGVideoInstData *mvid )
{
    struct Process   *ThisProcess   = (struct Process *)FindTask( NULL );
    struct Screen    *scr           = NULL,
                     *pubscr        = NULL;

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    /* Check if progress gauge was disabled by prefs... */
    if( (mvid -> mvid_PR . pr_Max) != ~0UL )
    {
      /* Get progress requester's screen */
      if( ThisProcess -> pr_WindowPtr )
      {
        scr = ((struct Window *)(ThisProcess -> pr_WindowPtr)) -> WScreen;
      }
      else
      {
        pubscr = LockPubScreen( NULL );
      }

      /* Valid screen ? */
      if( scr || pubscr )
      {
        struct Screen   *s    = (scr)?(scr):(pubscr);
        struct TextFont *font = (s -> RastPort . Font)?(s -> RastPort . Font):(GfxBase -> DefaultFont);

        if( font )
        {
          ULONG width,
                height;

          width  =  MAX( (font -> tf_XSize), 8 ) * 42UL;
          height = (MAX( (font -> tf_YSize), 8 ) * 5UL) / 3UL;

          if( (mvid -> mvid_PR . pr_Window = OpenWindowTags( NULL, WA_InnerWidth,                    width,
                                                                  WA_InnerHeight,                   height,
                                                                  WA_MinWidth,                      (width / 2UL),
                                                                  WA_MinHeight,                     0UL, /* overtake value calculated by WA_InnerHeight */
                                                                  WA_MaxWidth,                      (~0UL),
                                                                  WA_MaxHeight,                     0UL, /* overtake value calculated by WA_InnerHeight */
                                                                  WA_SizeGadget,                    TRUE,
                                                                  WA_DragBar,                       TRUE,
                                                                  WA_DepthGadget,                   TRUE,
                                                                  WA_CloseGadget,                   TRUE,
                                                                  WA_RMBTrap,                       TRUE,
                                                                  WA_IDCMP,                         IDCMP_CLOSEWINDOW,
                                                                  WA_Title,                         (mvid -> mvid_ProjectName),
                                                                  WA_ScreenTitle,                   (mvid -> mvid_ProjectName),
                                                                  XTAG( scr,    WA_CustomScreen ),  scr,
                                                                  XTAG( pubscr, WA_PubScreen    ),  pubscr,
                                                                  TAG_DONE ) ) )
          {
            ULONG dummy;

            /* Snapshot start of decoding... */
            CurrentTime( (&(mvid -> mvid_PR . pr_StartSecond)), (&dummy) );
          }
        }
      }

      if( pubscr )
      {
        UnlockPubScreen( NULL, pubscr );
      }
    }
}


void UpdateProgressRequester( struct MPEGVideoInstData *mvid )
{
    struct Window    *win;

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    if( (mvid -> mvid_PR . pr_Max) == 0UL )
    {
      mvid -> mvid_PR . pr_Max = 1UL;
    }

    if ((win = mvid -> mvid_PR . pr_Window ) != NULL)
    {
      ULONG                 left      = (win -> BorderLeft) + 1UL,
                            top       = (win -> BorderTop)  + 1UL,
                            width     = (win -> Width)  - (win -> BorderLeft) - (win -> BorderRight)  - 3UL,
                            height    = (win -> Height) - (win -> BorderTop)  - (win -> BorderBottom) - 3UL,
                            currwidth = (ULONG)((double)width * ((double)(mvid -> mvid_PR . pr_Curr) / (double)(mvid -> mvid_PR . pr_Max)));
      struct IntuiMessage  *imsg;

      /* Check bounds... */
      if( currwidth > width )
      {
        currwidth = width;
      }

      /* Draw gauge part representing the loaded frames... */
      SetAPen( (win -> RPort), (ULONG)(win -> BlockPen) );
      RectFill( (win -> RPort), left, top, (left + currwidth), (top + height) );

      /* ...then draw gauge part representing remaining bytes in the stream */
      SetAPen( (win -> RPort), (ULONG)(win -> DetailPen) );
      RectFill( (win -> RPort), (left + currwidth), top, (left + width), (top + height) );

      /* Render the time (max and current) */
      {
        TEXT  buffer[ 256 ];
        ULONG currsec,
              currmic,
              fullsec;
        ULONG textlen,
              x;

        /* Current time */
        CurrentTime( (&currsec), (&currmic) );
        currsec = currsec - (mvid -> mvid_PR . pr_StartSecond);

        /* Full decoding time */
        fullsec = (ULONG)((double)currsec / (double)(mvid -> mvid_PR . pr_Curr) * (double)(mvid -> mvid_PR . pr_Max));

        /* Write buffer */
        mysprintf( classbase, buffer, "%lu:%2.2lu/%lu:%2.2lu",
                   (currsec / 60UL), (currsec % 60UL),
                   (fullsec / 60UL), (fullsec % 60UL) );

        textlen = TextLength( (win -> RPort), buffer, (ULONG)strlen( buffer ) );

        x = ((width - textlen) / 2UL) + 1UL;

        /* Print "remaining seconds text" */
        SetDrMd( (win -> RPort), (JAM1 | COMPLEMENT) );
        Move( (win -> RPort), x, (top + (win -> RPort -> Font -> tf_Baseline) + 2UL) );

        Text( (win -> RPort), buffer, (ULONG)strlen( buffer ) );

        SetDrMd( (win -> RPort), JAM1 );
      }

      /* Check for the close gadget */
      while ((imsg = (struct IntuiMessage *)GetMsg( (win -> UserPort) ) ) != NULL)
      {
        /* Handle each message */
        switch( imsg -> Class )
        {
          case IDCMP_CLOSEWINDOW:
          {
              /* Set abort signal... */
              Signal( FindTask( NULL ), SIGBREAKF_CTRL_D );
          }
              break;
        }

        ReplyMsg( (&(imsg -> ExecMessage)) );
      }
    }
}


static
void DeleteProgressRequester( struct MPEGVideoInstData *mvid )
{
    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    if( mvid -> mvid_PR . pr_Window )
    {
      CloseWindow( (mvid -> mvid_PR . pr_Window) );
      mvid -> mvid_PR . pr_Window = NULL;
    }
}



