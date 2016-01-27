
/*
**
**  $VER: encoder.c 2.2 (13.4.98)
**  gifanim.datatype 2.2
**
**  GIF Encoder of gifanim.datatype
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

/* encoder header */
#include "encoder.h"

#include <libraries/realtime.h>

/*****************************************************************************/

#define fputc( c, fh ) FPutC( (fh), (long)(c) )
#define EOF            (-1)

static const int      maxbits    = MAX_LWZ_BITS;                  /* user settable max # bits/code */
static const code_int maxmaxcode = (code_int)1UL << MAX_LWZ_BITS; /* should NEVER generate this code */

#define MAXCODE( nbits ) (((code_int) 1 << (nbits)) - 1)

#define HashTabOf( i )    (genc -> htab[ i ])
#define CodeTabOf( i )    (genc -> codetab[ i ])

static const code_int hsize = HSIZE;                 /* for dynamic table sizing */

/* To save much memory, we overlay the table used by compress() with those
 * used by decompress().  The tab_prefix table is the same size and type
 * as the codetab.  The tab_suffix table needs 2**MAX_LWZ_BITS characters.  We
 * get this from the beginning of htab.  The output stack uses the rest
 * of htab, and contains characters.  There is plenty of room for any
 * possible stack (stack used to be 8000 characters).
 */

#define tab_prefixof(i) CodeTabOf(i)
#define tab_suffixof(i) ((UBYTE *)(genc -> htab))[i]
#define de_stack        ((UBYTE *)&tab_suffixof((code_int)1<<MAX_LWZ_BITS))

/*****************************************************************************/

/* local prototypes */
static struct GIFEncoder *CreateGIFEncoder( struct ClassBase *, Object *, struct GIFAnimInstData *, struct BitMap *, ULONG, ULONG, ULONG, ULONG, ULONG, BPTR );
static void               DeleteGIFEncoder( struct ClassBase *, struct GIFEncoder *  );

static int                GetPixel( struct GIFEncoder *, int x, int y );
static void               BumpPixel( struct GIFEncoder * );
static int                GIFNextPixel( struct GIFEncoder * );
static BOOL               GIFEncode( struct GIFEncoder *, struct GIFAnimInstData *, BOOL, WORD, WORD, struct ColorRegister *, ULONG, ULONG, ULONG );
static void               Putword( struct GIFEncoder *, int );
static void               compress( struct GIFEncoder *, int init_bits );
static void               output( struct GIFEncoder *, code_int );
static void               cl_block( struct GIFEncoder * );
static void               cl_hash( struct GIFEncoder *, count_int hsize );
static void               char_init( struct GIFEncoder * );

static void               char_out( struct GIFEncoder *, int );
static void               flush_char( struct GIFEncoder * );

/*****************************************************************************/


ULONG SaveGIFAnim( struct ClassBase *cb, struct IClass *cl, Object *o, struct dtWrite *dtw )
{
    ULONG retval = 0UL;
    LONG  error  = 0L;

    /* A NULL file handle is a nop (GMultiView uses this to test if a datatype supports RAW writing) */
    if( dtw -> dtw_FileHandle )
    {
      struct GIFAnimInstData *gaid = (struct GIFAnimInstData *)INST_DATA( cl, o );

      IPTR                 modeid = 0;
      struct ColorRegister *cm = NULL;
      IPTR                 numcolors = 0;
      IPTR                 startframe = 0UL,
                            numframes  = 0UL,
                            framestep  = 1UL;
      IPTR                 tpf        = 0UL;
      IPTR                 animwidth = 0,
                            animheight = 0,
                            animdepth = 0;
      struct BitMap        *keyframe = NULL;

      if( GetDTAttrs( o, ADTA_ModeID,           (&modeid),
                         ADTA_ColorRegisters,   (&cm),
                         ADTA_NumColors,        (&numcolors),
                         ADTA_Width,            (&animwidth),
                         ADTA_Height,           (&animheight),
                         ADTA_Depth,            (&animdepth),
                         ADTA_KeyFrame,         (&keyframe),
                         ADTA_Frame,            (&startframe),
                         ADTA_Frames,           (&numframes),
                         ADTA_TicksPerFrame,    (&tpf),
                         TAG_DONE ) == 10UL )
      {
        verbose_printf( cb, gaid, "anim %lu*%lu*%lu, %lu colors, %lu frames, tpf %lu\n",
                                  animwidth, animheight, animdepth, numcolors, numframes, tpf );

        /* GIF only supports up to 256 colors (8 bit) */
        if( animdepth <= 8UL )
        {
          /* HAM cannot be implemented (same problem as truecolor support); EHB would be possible (on request) */
          if( !(modeid & (HAM | EXTRA_HALFBRITE)) )
          {
            struct GIFEncoder *genc;

            if ((genc = CreateGIFEncoder( cb, o, gaid, keyframe, animwidth, animheight, animdepth, numcolors, tpf, (dtw -> dtw_FileHandle) ) ) != NULL)
            {
              struct TagItem *tstate,
                             *ti;

              numframes -= startframe;

              tstate = dtw -> dtw_AttrList;

              while ((ti = NextTagItem( (&tstate) ) ) != NULL)
              {
                switch( ti -> ti_Tag )
                {
                  case ADTA_Frame:            startframe = ti -> ti_Data; break;
                  case ADTA_Frames:           numframes  = ti -> ti_Data; break;
                  case ADTA_FrameIncrement:   framestep  = ti -> ti_Data; break;
                }
              }

              if( framestep == 0UL ) framestep = 1UL;

              error_printf( cb, gaid, "saving gif anim %lu %lu %lu\n", startframe, numframes, framestep );

              if( numframes )
              {
                retval = (LONG)GIFEncode( genc, gaid, (genc -> interlace), (genc -> backgroundpen), (genc -> transparentpen), cm, startframe, numframes, framestep );
                error = IoErr();
              }

              DeleteGIFEncoder( cb, genc );
            }
            else
            {
              /* Can't alloc gif encoder context data */
              error = ERROR_NO_FREE_STORE;
              error_printf( cb, gaid, "no memory for encoder context data (%d)\n", error);
            }
          }
          else
          {
            error_printf( cb, gaid, "HAM/EHB not supported by the GIF format (%x)\n", -1);
            error = (modeid & EXTRA_HALFBRITE)?(ERROR_NOT_IMPLEMENTED):(DTERROR_INVALID_DATA);
          }
        }
        else
        {
          error_printf( cb, gaid, "Object is too deep (%lu planes), gif encoder works only with depth <= 8\n", animdepth );
          error = DTERROR_INVALID_DATA;
        }
      }
      else
      {
        /* can't get required attributes from object */
        error_printf( cb, gaid, "failed to obatin %d attributes\n", 10);
        error = ERROR_OBJECT_WRONG_TYPE;
      }

      /* Some info... */
      if( retval )
      {
        verbose_printf( cb, gaid, "GIF Animation successfully created\n" );
      }
      else
      {
        error_printf( cb, gaid, "GIF Animation creation failed, error %ld\n", error );
      }
    }

    SetIoErr( error );

    return( retval );
}


/*****************************************************************************/

static
struct GIFEncoder *CreateGIFEncoder( struct ClassBase *cb, Object *o, struct GIFAnimInstData *gaid, struct BitMap *keybm, ULONG animwidth, ULONG animheight, ULONG animdepth, ULONG numcolors, ULONG tpf, BPTR out )
{
    /* Idiot testing... */
    if( o && animwidth && animheight && animdepth && (numcolors > 0UL) && (numcolors <= MAXCOLORMAPSIZE) && tpf && keybm )
    {
      ULONG              chunkysize = (animwidth + 2UL) * (animheight + 2UL) + 256; /* size of one chunkymap with safety space */
      struct GIFEncoder *genc;
      ULONG              size1,
                         size2,
                         msize;

      /* Compute amount of memory needed for the gif encoder... */
      size1 =         sizeof( struct GIFEncoder ) + 8UL;    /* Encoder context data */
      size2 = size1 + chunkysize                  + 8UL;    /* Chunky map 1 */
      msize = size2 + chunkysize                  + 8UL;    /* Chunky map 2 */

      if ((genc = (struct GIFEncoder *)AllocVec( msize, (MEMF_PUBLIC | MEMF_CLEAR) ) ) != NULL)
      {
        /* get the chunkymap ptrs... */
        genc -> srcchunkymap[ 0 ] = MEMORY_NAL_FOLLOWING( genc, size1 );
        genc -> srcchunkymap[ 1 ] = MEMORY_NAL_FOLLOWING( genc, size2 );

        /* Init the temp. rastport required by ReadPixelArray8 */
        InitRastPort( (&(genc -> rpa8tmprp)) );
        genc -> rpa8tmprp . BitMap = AllocBitMap( ((animwidth + 15UL) & ~15UL), 1UL, animdepth, BMF_MINPLANES, keybm );

        /* Get a system bitmap (where RPA8 can read from...) */
        genc -> srcbm = AllocBitMap( animwidth, animheight, animdepth, BMF_MINPLANES, keybm );

        if( (genc -> srcbm) && (genc -> rpa8tmprp . BitMap) )
        {
          /* Fill-in the remaining ptrs... */
          genc -> classbase  = cb;
          genc -> object     = o;
          genc -> outfile    = out;
          genc -> animwidth  = animwidth;
          genc -> animheight = animheight;
          genc -> animdepth  = animdepth;
          genc -> numcolors  = numcolors;
          genc -> tpf        = tpf;

          /* Prefs defaults... */
          genc -> interlace      = FALSE;
          genc -> backgroundpen  =  0; /* default bg pen */
          genc -> transparentpen = -1; /* means: no transparent pen */

          /* Read encoder preferences */
          ReadENVPrefs( cb, gaid, genc );

          /* Init source rastport */
          InitRastPort( (&(genc -> rp)) );
          genc -> rp . BitMap = genc -> srcbm;

          return( genc );
        }
        else
        {
          error_printf( cb, gaid, "no memory for temporary %d bit bitmap(s)\n", animdepth);
        }

        FreeBitMap( (genc -> rpa8tmprp . BitMap) );
        FreeBitMap( (genc -> srcbm) );
      }
      else
      {
        error_printf( cb, gaid, "no memory for %d byte encoder context data\n", msize);
      }

      FreeVec( genc );
    }
    else
    {
      error_printf( cb, gaid, "invalid arguments for object @ 0x%p\n", o);
    }


    return( NULL );
}


static
void DeleteGIFEncoder( struct ClassBase *cb, struct GIFEncoder *genc )
{
    if( genc )
    {
      /* Unload any loaded frame... */
      if( genc -> loadmsg . alf_UserData )
      {
        genc -> loadmsg . MethodID = ADTM_UNLOADFRAME;
        DoMethodA( (genc -> object), (Msg)(&(genc -> loadmsg)) );
      }

      FreeBitMap( (genc -> rpa8tmprp . BitMap) );
      FreeBitMap( (genc -> srcbm) );

      FreeVec( genc );
    }
}

/*****************************************************************************/

/* here begins the real encoder code... */
#define cb (genc -> classbase)

static
int GetPixel( struct GIFEncoder *genc, int x, int y )
{
#if 1
    int pixel = (int)ReadPixel( (&(genc -> rp)), (long)x, (long)y );
#else
    int pixel = (int)genc -> currchunkymap[ ((genc -> animwidth) * y) + x ];
#endif

    /* Should not occur, but... */
    if( pixel >= (1UL << (genc -> animdepth)) )
    {
      pixel = 0;
    }

    return( pixel );
}


/* Bump the 'curx' and 'cury' to point to the next pixel */
static
void BumpPixel( struct GIFEncoder *genc )
{
    /* Bump the current X position */
    genc -> curx++;

    /* If we are at the end of a scan line, set curx back to the beginning
     * If we are interlaced, bump the cury to the appropriate spot,
     * otherwise, just increment it.
     */
    if( (genc -> curx) == (genc -> Width) )
    {
      genc -> curx = 0;

      if( genc -> Interlace )
      {
        switch( genc -> Pass )
        {
          case 0:
          {
              genc -> cury += 8;

              if( (genc -> cury) >= (genc -> Height) )
              {
                genc -> Pass++;
                genc -> cury = 4;
              }
          }
              break;

          case 1:
          {
              genc -> cury += 8;

              if( (genc -> cury) >= (genc -> Height) )
              {
                genc -> Pass++;
                genc -> cury = 2;
              }
          }
              break;

          case 2:
          {
              genc -> cury += 4;

              if( (genc -> cury) >= (genc -> Height) )
              {
                genc -> Pass++;
                genc -> cury = 1;
              }
          }
              break;

          case 3:
          {
              genc -> cury += 2;
          }
              break;
        }
      }
      else
      {
        genc -> cury++;
      }
    }
}


/* Return the next pixel from the image */
static
int GIFNextPixel( struct GIFEncoder *genc )
{
    int r;

    if( (genc -> CountDown) == 0 )
      return( EOF );

    genc -> CountDown--;

    r = GetPixel( genc, (genc -> curx), (genc -> cury) );

    BumpPixel( genc );

    return( r );
}


/* public */
static
BOOL GIFEncode( struct GIFEncoder *genc, struct GIFAnimInstData *gaid,
                BOOL GInterlace, WORD Background, WORD Transparent,
                struct ColorRegister *cm, ULONG startframe, ULONG numframes, ULONG framestep )
{
    ULONG                timestamp;
    int                  B;
    int                  RWidth,
                         RHeight;
    int                  Resolution;
    int                  ColorMapSize;
    int                  InitCodeSize;
    int                  i;
    BPTR                 fp               = genc -> outfile;
    LONG                 gif_delay        = 0L;
    BOOL                 save_frame;
    struct ColorRegister localcolormap[ MAXCOLORMAPSIZE ];
    BOOL                 save_localcolormap;
    BOOL                 firstframe       = TRUE; /* the first frame has not been written yet */

    genc -> Interlace = GInterlace;

    ColorMapSize = 1UL << (genc -> animdepth);

    RWidth  = genc -> Width  = genc -> animwidth;
    RHeight = genc -> Height = genc -> animheight;

    Resolution = genc -> animdepth;

    /* The initial code size */
    if( (genc -> animdepth) <= 1 )
      InitCodeSize = 2;
    else
      InitCodeSize = (genc -> animdepth);

    /* Write the GIF signature */
    FWrite( fp, "GIF", 1, 3 );

    /* Write the GIF version number (only "89a" because of the "Graphics Control Extension" used for timing) */
    FWrite( fp, "89a", 1, 3 );

    /* Write out the screen width and height */
    Putword( genc, RWidth  );
    Putword( genc, RHeight );

    /* Flag field */
    {
      /* Indicate that there is a global colour map */
      B = LOCALCOLORMAP; /* Yes, there is a global color map */

      /* OR in the resolution */
      B |= (Resolution - 1) << 5;

      /* OR in the Bits per Pixel (Size of global color map) */
      B |= ((genc -> animdepth) - 1);

      /* Write it out */
      fputc( B, fp );
    }

    /* Write out the Background colour */
    fputc( Background, fp );

    /* Write pixel aspect ratio (not implemented yet, therefore 0 here) */
    fputc( 0, fp );

    /* Write out the Global Colour Map */
    for( i = 0 ; i < ColorMapSize ; i++ )
    {
      fputc( (cm[ i ] . red),   fp );
      fputc( (cm[ i ] . green), fp );
      fputc( (cm[ i ] . blue),  fp );
    }

    /* Write comment extensions for each DTA_Obj#? attribute we found... */
    {
      /* ti_Tag is the attribute to check; ti_Data is the label written in front of the data */
      struct TagItem commentstags[] =
      {
        { DTA_ObjName,       (IPTR)"name: "       },
        { DTA_ObjAuthor,     (IPTR)"author: "     },
        { DTA_ObjAnnotation, (IPTR)"annotation: " },
        { DTA_ObjCopyright,  (IPTR)"copyright: "  },
        { DTA_ObjVersion,    (IPTR)"version: "    },
        { TAG_DONE,          0UL                   }
      };

      struct TagItem *tstate = commentstags,
                     *ti;

      while ((ti = NextTagItem( (&tstate) ) ) != NULL)
      {
        STRPTR string = NULL,
               label = (STRPTR)(ti -> ti_Data);

        (void)GetDTAttrs( (genc -> object), (ti -> ti_Tag), (&string), TAG_DONE );

        if( string )
        {
          UBYTE *c;

          fputc( '!',   fp ); /* Extension */
          fputc( 0xfe,  fp ); /* GIF89a Comment extension */

          c = label;  while( *c ) char_out( genc, *c++ ); /* write comment label            */
          c = string; while( *c ) char_out( genc, *c++ ); /* write DTA_Obj#? attribute data */

          flush_char( genc );

          fputc( 0, fp ); /* Block terminator */
        }
      }
    }

    for( timestamp = startframe ; numframes > 0UL ; timestamp += framestep, numframes-- )
    {
      /* reset some values */
      save_frame         = FALSE;
      save_localcolormap = FALSE;

      /* Check for abort... */
      if( CheckSignal( SIGBREAKF_CTRL_D ) )
      {
        SetIoErr( ERROR_BREAK );
        return( FALSE );
      }

      genc -> loadmsg . MethodID      = ADTM_LOADFRAME;
      genc -> loadmsg . alf_TimeStamp = timestamp;
      genc -> loadmsg . alf_Frame     = timestamp; /* CBM anim.datatype compatibility ONLY */

      if( DoMethodA( (genc -> object), (Msg)(&(genc -> loadmsg)) ) )
      {
        /* print frame contents */
        verbose_printf( cb, gaid, "frame: timestamp %lu frame %lu duration %lu bitmap %lx cmap %lx sample %lx len %lu period %lu\n",
                        timestamp,
                        (genc -> loadmsg . alf_Frame),
                        (genc -> loadmsg . alf_Duration),
                        (genc -> loadmsg . alf_BitMap),
                        (genc -> loadmsg . alf_CMap),
                        (genc -> loadmsg . alf_Sample),
                        (genc -> loadmsg . alf_SampleLength),
                        (genc -> loadmsg . alf_Period) );

        /* If we got a bitmap (should always be TRUE), compare it with the previous one
         * If there was a change of the contents, write a matching gif image out...
         */
        if( genc -> loadmsg . alf_BitMap )
        {
          /* swap bitmaps */
          genc -> whichbm = !(genc -> whichbm); /* 0 to 1 or 1 to 0 */

          genc -> currchunkymap = genc -> srcchunkymap[ (genc -> whichbm) ]; /* shortcut */

          /* Copy given bitmap (which may be a planar bitmap in fast-mem) to a system-allocated bitmap
           * that ReadPixel(%|Array8) can reach it...
           */
          CopyBitMap( cb, (genc -> srcbm), (genc -> loadmsg . alf_BitMap), (genc -> animwidth), (genc -> animheight) );

          /* Convert data into a chunky pixel map */
          (void)ReadPixelArray8( (&(genc -> rp)), 0UL, 0UL, ((genc -> animwidth) - 1UL), ((genc -> animheight) - 1UL), (genc -> currchunkymap), (&(genc -> rpa8tmprp)) );

          /* The first frame must be written because ther's no previous frame to compare... */
          if( firstframe )
          {
            firstframe = FALSE;
            save_frame = TRUE;
          }
          else
          {
            /* Compare old and new chunky map and check if they're equal
             * If they're equal, there's no need to write them out...
             */
            save_frame = memcmp( (genc -> srcchunkymap[ (genc -> whichbm) ]),
                                 (genc -> srcchunkymap[ !(genc -> whichbm) ]),
                                 (size_t)((genc -> animwidth) * (genc -> animheight)) );
          }
        }
        else
        {
          /* Should not occur */
          error_printf( cb, gaid, "WARNING: no bitmap @ timestamp %d\n", timestamp);
          SetIoErr( ERROR_OBJECT_WRONG_TYPE ); /* not very meaningfull */
          return( FALSE );
        }

        /* If we got a local colormap, check if it is different from the global one.
         * If there is a difference, set "save_localcolormap" to TRUE that a local colormap is saved below
         */
        if( genc -> loadmsg . alf_CMap )
        {
          ULONG color;
          ULONG rgb[ 3 ];

          for( color = 0UL ; color < ColorMapSize ; color++ )
          {
            GetRGB32( (genc -> loadmsg . alf_CMap), color, 1UL, rgb );

            localcolormap[ color ] . red   = (UBYTE)(rgb[ 0 ] >> 24UL);
            localcolormap[ color ] . green = (UBYTE)(rgb[ 1 ] >> 24UL);
            localcolormap[ color ] . blue  = (UBYTE)(rgb[ 2 ] >> 24UL);

            /* Any different between alf_CMap and global color map (e.g. ADTA_ColorRegisters) ? */
            if( ((localcolormap[ color ] . red)   != (cm[ color ] . red))   ||
                ((localcolormap[ color ] . green) != (cm[ color ] . green)) ||
                ((localcolormap[ color ] . blue)  != (cm[ color ] . blue)) )
            {
              /* Save local colormap */
              save_localcolormap = TRUE;
            }
          }
        }

        /* Any change in the data ? */
        if( save_frame || save_localcolormap )
        {
          verbose_printf( cb, gaid, "saving gif image%s, %ld/100 sec delay\n",
                          ((save_localcolormap)?(" with local colormap"):("")),
                          gif_delay );

          /* Calculate number of bits we are expecting */
          genc -> CountDown = (long)(genc -> Width) * (long)(genc -> Height);

          /* Set up the current x and y position */
          genc -> curx = genc -> cury = 0;

          /* Indicate which pass we are on (if interlace) */
          genc -> Pass = 0;

          /* Reset */
          genc -> cur_accum = 0;
          genc -> cur_bits  = 0;

          genc -> a_count   = 0;

          /* Write out Graphic Control Extension for timing (and transparent colour index, if necessary.) */
          {
            UBYTE packed = 0U;

            fputc( '!',                                  fp );  /* Extension */
            fputc( 0xf9,                                 fp );  /* GIF89a Graphic Control Extension */
            fputc( 4,                                    fp );  /* Block size */

            if( Transparent >= 0 )
            {
              packed |= 0x01;                                   /* has transparent color ! */
            }

            fputc( packed,                               fp );

            Putword( genc, (int)gif_delay );                    /* Delay in 1/100 sec */

            fputc( (Transparent >= 0)?(Transparent):(0), fp );
            fputc( 0,                                    fp );  /* Block terminator */

            gif_delay = 0L; /* delay time written... */
          }

          /* Write GIF Image Descriptor */
          {
            UBYTE packed = 0U;

            /* Write: "Start of raster data" */
            fputc( ',', fp );

            /* Write the Image header (does currently not support deltas) */
            Putword( genc, 0                 );
            Putword( genc, 0                 );
            Putword( genc, (genc -> Width)   );
            Putword( genc, (genc -> Height)  );

            /* Write out whether or not the image is interlaced */
            if( genc -> Interlace )
            {
              packed |= INTERLACE;
            }

            /* Write out whether or not the image has an own colormap */
            if( save_localcolormap )
            {
              packed |= LOCALCOLORMAP;

              /* OR in the Bits per Pixel (Size of local color map) */
              packed |= ((genc -> animdepth) - 1);
            }

            fputc( packed, fp );
          }

          /* Write local color map ? */
          if( save_localcolormap )
          {
            ULONG color;

            /* Write out the Local Colour Map */
            for( color = 0UL ; color < ColorMapSize ; color++ )
            {
              fputc( (localcolormap[ color ] . red),    fp );
              fputc( (localcolormap[ color ] . green),  fp );
              fputc( (localcolormap[ color ] . blue),   fp );
            }
          }

          /* Write out the initial code size */
          fputc( InitCodeSize, fp );

          /* Go and actually compress the data */
          compress( genc, (InitCodeSize + 1) );

          /* Write out a Zero-length packet (to end the series) */
          fputc( 0, fp );
        }

        /* Sum here the time to wait for this frame...
         * (and Scale from 1/1200 units (realtime.library && ADTA_TicksPerFrame) to GIF delay (1/100) units)
         */
        gif_delay += INTDIVR( (100UL * (genc -> tpf)),  TICK_FREQ );
      }
      else
      {
        /* ADTM_LOADFRAME failed: Result2 contains the error cause */
        return( FALSE );
      }
    }

    /* Write the GIF file terminator */
    fputc( ';', fp );

    return( TRUE );
}


/* Write out a LITTLE_ENDIAN word to the GIF file */
static
void Putword( struct GIFEncoder *genc, int w )
{
    fputc( (w & 0xff),         (genc -> outfile) );
    fputc( ((w / 256) & 0xff), (genc -> outfile) );
}


/***************************************************************************
 *
 *  GIFCOMPR.C       - GIF Image compression routines
 *
 *  Lempel-Ziv compression based on 'compress'.  GIF modifications by
 *  David Rowley (mgardi@watdcsu.waterloo.edu)
 *
 ***************************************************************************/


/*
 *
 * GIF Image compression - modified 'compress'
 *
 * Based on: compress.c - File compression ala IEEE Computer, June 1984.
 *
 * By Authors:  Spencer W. Thomas       (decvax!harpo!utah-cs!utah-gr!thomas)
 *              Jim McKie               (decvax!mcvax!jim)
 *              Steve Davies            (decvax!vax135!petsd!peora!srd)
 *              Ken Turkowski           (decvax!decwrl!turtlevax!ken)
 *              James A. Woods          (decvax!ihnp4!ames!jaw)
 *              Joe Orost               (decvax!vax135!petsd!joe)
 *
 */


/*
 * compress stdin to stdout
 *
 * Algorithm:  use open addressing double hashing (no chaining) on the
 * prefix code / next character combination.  We do a variant of Knuth's
 * algorithm D (vol. 3, sec. 6.4) along with G. Knott's relatively-prime
 * secondary probe.  Here, the modular division first probe is gives way
 * to a faster exclusive-or manipulation.  Also do block compression with
 * an adaptive reset, whereby the code table is cleared when the compression
 * ratio decreases, but after the table fills.  The variable-length output
 * codes are re-sized at this point, and a special CLEAR code is generated
 * for the decompressor.  Late addition:  construct the table according to
 * file size for noticeable speed improvement on small files.  Please direct
 * questions about this implementation to ames!jaw.
 */

static
void compress( struct GIFEncoder *genc, int init_bits )
{
    register long     fcode;
    register code_int i /* = 0 */;
    register int      c;
    register code_int ent;
    register code_int disp;
    register code_int hsize_reg;
    register int      hshift;

    /* Set up the globals:  g_init_bits - initial number of bits */
    genc -> g_init_bits = init_bits;

    /* Set up the necessary values */
    genc -> clear_flg = FALSE;
    genc -> maxcode   = MAXCODE((genc -> n_bits) = (genc -> g_init_bits));

    genc -> ClearCode  = (1 << (init_bits - 1));
    genc -> EOFCode    = genc -> ClearCode + 1;
    genc -> free_ent   = genc -> ClearCode + 2;

    char_init( genc );

    ent = GIFNextPixel( genc );

    for( fcode = (long)hsize, hshift = 0 ;  fcode < 65536L ; fcode *= 2L )
    {
      hshift++;
    }

    hshift = 8 - hshift;                /* set hash code range bound */

    hsize_reg = hsize;
    cl_hash( genc, (count_int)hsize_reg );            /* clear hash table */

    output( genc, (code_int)(genc -> ClearCode) );

    while( (c = GIFNextPixel( genc )) != EOF )
    {
        fcode = (long) (((long) c << maxbits) + ent);
        i = (((code_int)c << hshift) ^ ent);    /* xor hashing */

        if( HashTabOf (i) == fcode )
        {
            ent = CodeTabOf (i);
            continue;
        }
        else
          if ( (long)HashTabOf (i) < 0 )      /* empty slot */
            goto nomatch;

        disp = hsize_reg - i;           /* secondary hash (after G. Knott) */
        if ( i == 0 )
          disp = 1;
probe:
        if ( (i -= disp) < 0 )
          i += hsize_reg;

        if ( HashTabOf (i) == fcode )
        {
          ent = CodeTabOf (i);
          continue;
        }

        if( (long)HashTabOf (i) > 0 )
          goto probe;

nomatch:
        output( genc, (code_int) ent );
        ent = c;
        if( (genc -> free_ent) < maxmaxcode )
        {
          CodeTabOf( i ) = genc -> free_ent++; /* code -> hashtable */
          HashTabOf( i ) = fcode;
        }
        else
        {
          cl_block( genc );
        }
    }

    /* Put out the final code. */
    output( genc, (code_int)ent );
    output( genc, (code_int)(genc -> EOFCode) );
}

/*****************************************************************
 * TAG( output )
 *
 * Output the given code.
 * Inputs:
 *      code:   A n_bits-bit integer.  If == -1, then EOF.  This assumes
 *              that n_bits =< (long)wordsize - 1.
 * Outputs:
 *      Outputs code to the file.
 * Assumptions:
 *      Chars are 8 bits long.
 * Algorithm:
 *      Maintain a BITS character long buffer (so that 8 codes will
 * fit in it exactly).  Use the VAX insv instruction to insert each
 * code in turn.  When the buffer fills up empty it and start over.
 */

static const
unsigned long masks[] =
{
  0x0000, 0x0001, 0x0003, 0x0007,
  0x000F, 0x001F, 0x003F, 0x007F,
  0x00FF, 0x01FF, 0x03FF, 0x07FF,
  0x0FFF, 0x1FFF, 0x3FFF, 0x7FFF,
  0xFFFF
};


static
void output( struct GIFEncoder *genc, code_int code )
{
    genc -> cur_accum &= masks[ genc -> cur_bits ];

    if( genc -> cur_bits > 0 )
      genc -> cur_accum |= ((long)code << genc -> cur_bits);
    else
      genc -> cur_accum = code;

    genc -> cur_bits += genc -> n_bits;

    while( genc -> cur_bits >= 8 )
    {
      char_out( genc, (unsigned int)(genc -> cur_accum & 0xff) );
      genc -> cur_accum >>= 8;
      genc -> cur_bits -= 8;
    }

    /* If the next entry is going to be too big for the code size,
     * then increase it, if possible.
     */
    if( ((genc -> free_ent) > (genc -> maxcode)) || (genc -> clear_flg) )
    {
      if( genc -> clear_flg )
      {
        genc -> maxcode = MAXCODE ((genc -> n_bits) = (genc -> g_init_bits));
        genc -> clear_flg = FALSE;
      }
      else
      {
        ++genc -> n_bits;

        if( (genc -> n_bits) == maxbits )
          genc -> maxcode = maxmaxcode;
        else
          genc -> maxcode = MAXCODE(genc -> n_bits);
      }
    }

    if( code == (genc -> EOFCode) )
    {
      /* At EOF, write the rest of the buffer. */
      while( genc -> cur_bits > 0 )
      {
        char_out( genc, (unsigned int)(genc -> cur_accum & 0xff) );
        genc -> cur_accum >>= 8;
        genc -> cur_bits -= 8;
      }

      flush_char( genc );
    }
}


/* Clear out the hash table */
static
void cl_block( struct GIFEncoder *genc )             /* table clear for block compress */
{
    cl_hash ( genc, (count_int) hsize );
    genc -> free_ent  = (genc -> ClearCode) + 2;
    genc -> clear_flg = TRUE;

    output( genc, (code_int)(genc -> ClearCode) );
}


static
void cl_hash( struct GIFEncoder *genc, register count_int hsize )          /* reset code table */
{
    register count_int *htab_p = (genc -> htab) + hsize;

    register long i;
    register long m1 = -1;

    i = hsize - 16;
    do /* might use Sys V memset(3) here */
    {
      *(htab_p-16) = m1;
      *(htab_p-15) = m1;
      *(htab_p-14) = m1;
      *(htab_p-13) = m1;
      *(htab_p-12) = m1;
      *(htab_p-11) = m1;
      *(htab_p-10) = m1;
      *(htab_p- 9) = m1;
      *(htab_p- 8) = m1;
      *(htab_p- 7) = m1;
      *(htab_p- 6) = m1;
      *(htab_p- 5) = m1;
      *(htab_p- 4) = m1;
      *(htab_p- 3) = m1;
      *(htab_p- 2) = m1;
      *(htab_p- 1) = m1;
      htab_p -= 16;
    } while ((i -= 16) >= 0);

    for ( i += 16 ; i > 0 ; i-- )
      *--htab_p = m1;
}


/******************************************************************************
 *
 * GIF Specific routines
 *
 ******************************************************************************/


/* Set up the 'byte output' routine */
static
void char_init( struct GIFEncoder *genc )
{
    genc -> a_count = 0;
}


/* Add a character to the end of the current packet, and if it is 254
 * characters, flush the packet to disk.
 */
static
void char_out( struct GIFEncoder *genc, int c )
{
    genc -> accum[ (genc -> a_count)++ ] = c;

    if( (genc -> a_count) >= 254 )
      flush_char( genc );
}


/* Flush the packet to disk, and reset the accumulator */
static
void flush_char( struct GIFEncoder *genc )
{
    if( (genc -> a_count) > 0 )
    {
      fputc( (genc -> a_count), (genc -> outfile) );
      FWrite( (genc -> outfile), (genc -> accum), 1UL, (ULONG)(genc -> a_count) );
      genc -> a_count = 0;
    }
}


