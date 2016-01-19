
/*
**
**  $VER: mpegvideo.c 1.11 (30.10.97)
**  mpegvideo.datatype 1.11
**
**  This file contains C code that implements the
**  video decoder model.
**
**  Written 1996/1997 by Roland 'Gizzy' Mainz
**
*/

#ifndef DEBUG
#   define DEBUG 0
#endif
#include <aros/debug.h>

#if 0
#define LOOSE_MPEG 1 /* accept malformed mpeg video streams */
#endif

/* project includes */
#include "mpegproto.h"
#include "mpegmyassert.h"
#include "mpegdecoders.h"
#include "mpegutil.h"


/* local prototypes */
static void         ParseSeqHead( struct MPEGVideoInstData *, VidStream * );
static void         ParseGOP( struct MPEGVideoInstData *, VidStream * );
static int          ParsePicture( struct MPEGVideoInstData *, VidStream * , TimeStamp );
static PictImage   *LockRingEntry( struct MPEGVideoInstData *, VidStream *vid_stream );
static void         ParseSlice( struct MPEGVideoInstData *, VidStream * );
static int          ParseMacroBlock( struct MPEGVideoInstData *, VidStream * );
static void         ReconIMBlock( struct MPEGVideoInstData *, VidStream * , int );
static void         ReconPMBlock( struct MPEGVideoInstData *, VidStream * , int , int , int , int );
static void         ReconBMBlock( struct MPEGVideoInstData *, VidStream * , int , int , int , int );
static void         ReconBiMBlock( struct MPEGVideoInstData *, VidStream * , int , int , int , int , int , int );
static void         ProcessSkippedPFrameMBlocks( struct MPEGVideoInstData *, VidStream * );
static void         ProcessSkippedBFrameMBlocks( struct MPEGVideoInstData *, VidStream * );
static void         ReconSkippedBlock( struct MPEGVideoInstData *, UBYTE * , UBYTE * , int , int , int , int , int , int , int , int );
static void         DoPictureDisplay( struct MPEGVideoInstData *, VidStream * );

/* Set up array for fast conversion from zig zag order to row/column coordinates. */
static const
UBYTE zigzag[ 64 ][ 2 ] =
{
  { 0, 0}, { 1, 0}, { 0, 1}, { 0, 2}, { 1, 1}, { 2, 0}, { 3, 0}, { 2, 1},
  { 1, 2}, { 0, 3}, { 0, 4}, { 1, 3}, { 2, 2}, { 3, 1}, { 4, 0}, { 5, 0},
  { 4, 1}, { 3, 2}, { 2, 3}, { 1, 4}, { 0, 5}, { 0, 6}, { 1, 5}, { 2, 4},
  { 3, 3}, { 4, 2}, { 5, 1}, { 6, 0}, { 7, 0}, { 6, 1}, { 5, 2}, { 4, 3},
  { 3, 4}, { 2, 5}, { 1, 6}, { 0, 7}, { 1, 7}, { 2, 6}, { 3, 5}, { 4, 4},
  { 5, 3}, { 6, 2}, { 7, 1}, { 7, 2}, { 6, 3}, { 5, 4}, { 4, 5}, { 3, 6},
  { 2, 7}, { 3, 7}, { 4, 6}, { 5, 5}, { 6, 4}, { 7, 3}, { 7, 4}, { 6, 5},
  { 5, 6}, { 4, 7}, { 5, 7}, { 6, 6}, { 7, 5}, { 7, 6}, { 6, 7}, { 7, 7}
};


/* Array mapping zigzag to array pointer offset. */
const
UBYTE zigzag_direct[ 64 ] =
{
   0,  1,  8, 16,  9,  2,  3, 10, 17, 24, 32, 25, 18, 11,  4,  5,
  12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13,  6,  7, 14, 21, 28,
  35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
  58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
};

/* Initialize P and B skip flags. */
#define No_P_Flag (mvid -> No_P_Flag)
#define No_B_Flag (mvid -> No_B_Flag)

/*
 * We use a lookup table to make sure values stay in the 0..255 range.
 * Since this is cropping (ie, x = (x < 0)?0:(x>255)?255:x; ), wee call this
 * table the "crop table".
 * MAX_NEG_CROP is the maximum neg/pos value we can handle.
 */
/*
#define MAX_NEG_CROP     (384)
*/
#define MAX_NEG_CROP     (384 * 4) /* shoud fix some assertCrop()-hits, see below */
#define NUM_CROP_ENTRIES (256 + (2 * MAX_NEG_CROP))

#ifdef NDEBUG
#define assertCrop( x )
#else
#define assertCrop( x )  assert( ((x) >= (-MAX_NEG_CROP)) && ((x) <= (256 + MAX_NEG_CROP)) )
#endif /* !NDEBUG */

static UBYTE cropTbl[ NUM_CROP_ENTRIES ];


/*
 *--------------------------------------------------------------
 *
 * NewVidStream --
 *
 *    Allocates and initializes a VidStream structure. Takes
 *      as parameter requested size for buffer length.
 *
 * Results:
 *    A pointer to the new VidStream structure.
 *
 * Side effects:
 *      None.
 *
 *--------------------------------------------------------------
 */


VidStream *NewVidStream( struct MPEGVideoInstData *mvid, int streambuflen )
{
    int           i,
                  j;
    VidStream    *new;
    const
    UBYTE         default_intra_matrix[ 64 ] =
    {
       8, 16, 19, 22, 26, 27, 29, 34,
      16, 16, 22, 24, 27, 29, 34, 37,
      19, 22, 26, 27, 29, 34, 34, 38,
      22, 22, 26, 27, 29, 34, 37, 40,
      22, 26, 27, 29, 32, 35, 40, 48,
      26, 27, 29, 32, 35, 40, 48, 58,
      26, 27, 29, 34, 38, 46, 56, 69,
      27, 29, 35, 38, 46, 56, 69, 83
    };

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    /* Check for legal buffer length. */
    if( streambuflen < 4 )
      return( NULL );

    /* Make buffer length multiple of 4. */
    streambuflen = (streambuflen + 3) >> 2;

    /* Allocate memory for new structure. */
    new = (VidStream *)mymalloc( mvid, sizeof( VidStream ) );

    /* Initialize pointers to extension and user data. */
    new -> group . ext_data       =
      new -> group . user_data    =
      new -> picture . extra_info =
      new -> picture . user_data  =
      new -> picture . ext_data   =
      new -> slice . extra_info   =
      new -> ext_data             =
      new -> user_data            = NULL;

    /* Copy default intra matrix. */
    for( i = 0 ; i < 8 ; i++ )
    {
      for( j = 0 ; j < 8 ; j++ )
      {
        new -> intra_quant_matrix[ j ][ i ] = default_intra_matrix[ i * 8 + j ];
      }
    }

    /* Initialize crop table. */
    for( i = (-MAX_NEG_CROP); i < NUM_CROP_ENTRIES - MAX_NEG_CROP ; i++ )
    {
      if( i <= 0 )
      {
        cropTbl[ i + MAX_NEG_CROP ] = 0;
      }
      else
      {
        if( i >= 255 )
        {
          cropTbl[ i + MAX_NEG_CROP ] = 255;
        }
        else
        {
          cropTbl[ i + MAX_NEG_CROP ] = i;
        }
      }
    }

    /* Initialize non intra quantization matrix. */
    for( i = 0 ; i < 8 ; i++ )
    {
      for( j = 0 ; j < 8 ; j++ )
      {
        new -> non_intra_quant_matrix[ j ][ i ] = 16;
      }
    }

    /* Initialize pointers to image spaces. */
    new -> current = new -> past = new -> future = NULL;

    for( i = 0 ; i < RING_BUF_SIZE ; i++ )
    {
      new -> ring[ i ] = NULL;
    }

    /* Create buffer. */
    new -> buf_start = (unsigned int *)mymalloc( mvid, ((streambuflen + 1) * 4) );

    /* Set max_buf_length to one less than actual length to deal with messy
     * data without proper seq. end codes.
     */
    new -> max_buf_length = streambuflen - 1;

    /* Initialize bitstream i/o fields. */
    new -> bit_offset = 0;
    new -> buf_length = 0;
    new -> buffer     = new -> buf_start;

    /* Return structure. */
    return( new );
}


void ResetVidStream( struct MPEGVideoInstData *mvid, VidStream *vid )
{
    int i;

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    /* Initialize pointers to image spaces. */
    vid -> current = vid -> past = vid -> future = NULL;

    /* Initialize rings */
    for( i = 0 ; i < RING_BUF_SIZE ; i++ )
    {
      if( vid -> ring[ i ] )
      {
        vid -> ring[ i ] -> locked = 0;  /* Unlock */
      }
    }

    /* Initialize bitstream i/o fields. */
    vid -> bit_offset = 0;
    vid -> buf_length = 0;
    vid -> buffer     = vid -> buf_start;
}


/*
 *--------------------------------------------------------------
 *
 * NewPictImage --
 *
 *    Allocates and initializes a PictImage structure.
 *      The width and height of the image space are passed in
 *      as parameters.
 *
 * Results:
 *    A pointer to the new PictImage structure.
 *
 * Side effects:
 *    None.
 *
 *--------------------------------------------------------------
 */


PictImage *NewPictImage( struct MPEGVideoInstData *mvid, unsigned int width, unsigned int height )
{
    PictImage *new;
    ULONG      dispwidth  = MAX( width,  anim_width  );
    ULONG      dispheight = MAX( height, anim_height );

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    /* Allocate memory space for new structure. */
    new = (PictImage *)mymalloc( mvid, sizeof( PictImage ) );

    /* Allocate memory for image spaces. */
    switch( ditherType )
    {
      case FULL_COLOR_DITHER:
      case HAM_DITHER:
          /* Allocate XRGB24 chunky pixel */
          new -> display = (UBYTE *)mymalloc( mvid, (size_t)(dispwidth * dispheight * 4UL) );
          break;

      case FULL_COLOR_DITHER16:
          /* Allocate XRGB16 chunky pixel */
          new -> display = (UBYTE *)mymalloc( mvid, (size_t)(dispwidth * dispheight * 2UL) );
          break;

      default:
          /* Allocate 8 bit chunky pixel */
          new -> display = (UBYTE *)mymalloc( mvid, (size_t)(dispwidth * dispheight) );
          break;
    }

    new -> luminance = (UBYTE *)mymalloc( mvid, (size_t)((width * height)      ) );
    new -> Cr        = (UBYTE *)mymalloc( mvid, (size_t)((width * height) / 4UL) );
    new -> Cb        = (UBYTE *)mymalloc( mvid, (size_t)((width * height) / 4UL) );

    /* Reset locked flag. */
    new -> locked = 0;

    /* Return pointer to new structure. */
    return( new );
}


/*
 *--------------------------------------------------------------
 *
 * DestroyPictImage --
 *
 *    Deallocates a PictImage structure.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *    None.
 *
 *--------------------------------------------------------------
 */


void DestroyPictImage( struct MPEGVideoInstData *mvid, PictImage *apictimage )
{
    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    if( apictimage -> luminance )
    {
      myfree( mvid, (apictimage -> luminance) );
      apictimage -> luminance = NULL;
    }

    if( apictimage -> Cr )
    {
      myfree( mvid, (apictimage -> Cr) );
      apictimage -> Cr = NULL;
    }

    if( apictimage -> Cb )
    {
      myfree( mvid, (apictimage -> Cb) );
      apictimage -> Cb = NULL;
    }

    if( apictimage -> display )
    {
      myfree( mvid, (apictimage -> display) );
      apictimage -> display = NULL;
    }

    myfree( mvid, apictimage );
}


/*
 *--------------------------------------------------------------
 *
 * mpegVidRsrc --
 *
 *      Parses bit stream until MB_QUANTUM number of
 *      macroblocks have been decoded or current slice or
 *      picture ends, whichever comes first. If the start
 *      of a frame is encountered, the frame is time stamped
 *      with the value passed in time_stamp. If the value
 *      passed in buffer is not null, the video stream buffer
 *      is set to buffer and the length of the buffer is
 *      expected in value passed in through length. The current
 *      video stream is set to vid_stream. If vid_stream
 *      is passed as NULL, a new VidStream structure is created
 *      and initialized and used as the current video stream.
 *
 * Results:
 *      A pointer to the video stream structure used.
 *
 * Side effects:
 *      Bit stream is irreversibly parsed. If a picture is completed,
 *      a function is called to display the frame at the correct time.
 *
 *--------------------------------------------------------------
 */


VidStream *mpegVidRsrc( struct MPEGVideoInstData *mvid, TimeStamp time_stamp, VidStream *vid_stream )
{
  unsigned int data;
  int          i,
               status;

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

  /* Set global curVidStream to vid_stream. Necessary because bit i/o use
   * curVidStream and are not passed vid_stream. Also set global bitstream
   * parameters.
   */
  curVidStream  =  vid_stream;
  bitOffset     =  curVidStream -> bit_offset;
  curBits       = *curVidStream -> buffer << bitOffset;
  bufLength     =  curVidStream -> buf_length;
  bitBuffer     =  (ULONG *)curVidStream -> buffer;

  /* If called for the first time, find start code, make sure it is a sequence start code. */
  if( mvid -> mvid_mpegVidRsrc_first )
  {
    mvid -> mvid_mpegVidRsrc_first = FALSE;

    next_start_code( mvid );
    show_bits32( data );

    if( data != SEQ_START_CODE )
    {
      error_printf( mvid, "This is not an MPEG stream. (%x)", data);

      myexit( mvid, RETURN_FAIL, ERROR_OBJECT_WRONG_TYPE );
    }
  }

  /* Get next 32 bits (size of start codes). */
  show_bits32( data );

  /* Process according to start code (or parse macroblock if not a start code at all. */
  switch( data )
  {
    case SEQ_END_CODE:
    {
        D(bug("[mpegvideo.datatype] %s: SEQ_END_CODE\n", __func__));
        /* Display last frame. */
        if( vid_stream -> future )
        {
          vid_stream -> current = vid_stream -> future;
          ExecuteDisplay( mvid, vid_stream );
        }

        /* Sequence done. Do the right thing. For right now, exit. */
#ifndef NDEBUG
        verbose_printf( mvid, "\nVideo successfully loaded. Done!\n" );
#endif /* !NDEBUG */

        myexit( mvid, RETURN_OK, 0L );
    }
        break;

    case SEQ_START_CODE:
    {
        D(bug("[mpegvideo.datatype] %s: SEQ_START_CODE\n", __func__));
        /* Sequence start code. Parse sequence header. */
        ParseSeqHead( mvid, vid_stream );

        /*
         * Return after sequence start code so that application above can use
         * info in header.
         */
        goto done;
    }

    case GOP_START_CODE:
    {
        D(bug("[mpegvideo.datatype] %s: GOP_START_CODE\n", __func__));
        if( mvid -> mvid_IndexScan )
        {
          mvid -> mvid_Last_PIC_SC_Pos = stream_pos( mvid );
        }

        /* Group of Pictures start code. Parse gop header. */
        ParseGOP( mvid, vid_stream );
    }
    case PICTURE_START_CODE:
    {
        D(bug("[mpegvideo.datatype] %s: PICTURE_START_CODE\n", __func__));
        /* Be sure that we don't overwrite the last GOP position ! */
        if( (data == PICTURE_START_CODE) && (mvid -> mvid_IndexScan) )
        {
          mvid -> mvid_Last_PIC_SC_Pos = stream_pos( mvid );
        }

        /* Picture start code. Parse picture header and first slice header.
         * returns currently only PARSE_OK or SKIP_PICTURE
         */
        status = ParsePicture( mvid, vid_stream, time_stamp );

        if( status == SKIP_PICTURE )
        {
          next_start_code( mvid );

#ifndef NDEBUG
          verbose_printf( mvid, "Skipping picture..." );
#endif /* !NDEBUG */

          while( !next_bits( mvid, 32, PICTURE_START_CODE ) )
          {
            if( next_bits( mvid, 32, GOP_START_CODE ) )
            {
              break;
            }
            else
            {
              if( next_bits( mvid, 32, SEQ_END_CODE ) )
                break;
            }

            flush_bits( 24 );
            next_start_code( mvid );
          }

#ifndef NDEBUG
          verbose_printf( mvid, "Done.\n" );
#endif /* !NDEBUG */

          /* Add empty frame, but only if we have at least ONE frame (no empty frames before the first frame) */
          if( totNumFrames )
          {
            AddFrame( mvid, NULL, NULL );
          }

          goto done;
        }

        ParseSlice( mvid, vid_stream );
    }
        break;

    default:
    {
        /* Check for slice start code. */
        if( (data >= SLICE_MIN_START_CODE) && (data <= SLICE_MAX_START_CODE) )
        {
          /* Slice start code. Parse slice header. */
          ParseSlice( mvid, vid_stream );
        }
        else
        {
          error_printf( mvid, "other mpeg data sequence code found %lx at %lx\n",
                        data, stream_pos( mvid ) );

          goto next_start_code; /* test test (1.10) - try to skip POSSIBLY invalid data */
        }
    }
        break;
  }

  /* Parse next MB_QUANTUM macroblocks. */
#if 0
  for( i = 0 ; i < MB_QUANTUM ; i++ )
#else
  i = 0UL;

  while( 1 )
#endif
  {
    /* Check to see if actually a startcode and not a macroblock. */
    if( !next_bits( mvid, 23, 0x00000000 ) )
    {
      /* Not start code. Parse Macroblock. */
      if( ParseMacroBlock( mvid, vid_stream ) == SKIP_TO_START_CODE )
      {
        goto next_start_code;
      }
    }
    else
    {
      /* Not macroblock, actually start code. Get start code. */
      next_start_code( mvid );
      show_bits32( data );

      /*
       * If start code is outside range of slice start codes, frame is
       * complete, display frame.
       */

      if( (data < SLICE_MIN_START_CODE) || (data > SLICE_MAX_START_CODE) )
      {
        DoPictureDisplay( mvid, vid_stream );
      }

      break;
    }

    if( i > MB_QUANTUM )
    {
      i = 0UL;
      error_printf( mvid, "macroblock quantum overflow %ld\n", (long)MB_QUANTUM );
    }
  }

  /* Return pointer to video stream structure. */
  goto done;

next_start_code:
  next_start_code( mvid );

done:
  /* Copy global bit i/o variables back into vid_stream. */
  vid_stream -> buffer      = (unsigned int *)bitBuffer;
  vid_stream -> buf_length  = bufLength;
  vid_stream -> bit_offset  = bitOffset;

  return( vid_stream );
}


VidStream *mpegVidRsrcScan( struct MPEGVideoInstData *mvid, TimeStamp time_stamp, VidStream *vid_stream )
{
  ULONG data;
  int   status = 0;

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

  /* Set global curVidStream to vid_stream. Necessary because bit i/o use
   * curVidStream and are not passed vid_stream. Also set global bitstream
   * parameters.
   */
  curVidStream  =  vid_stream;
  bitOffset     =  curVidStream -> bit_offset;
  curBits       = *curVidStream -> buffer << bitOffset;
  bufLength     =  curVidStream -> buf_length;
  bitBuffer     =  (ULONG *)curVidStream -> buffer;

  /* Get next 32 bits (size of start codes). */
  show_bits32( data );

  /* Process according to start code (or parse macroblock if not a start code at all). */
  switch( data )
  {
    case SEQ_END_CODE:
    {
        /* Sequence done. Do the right thing. For right now, exit. */
#ifndef NDEBUG
        verbose_printf( mvid, "\nVideo successfully loaded. Done!\n" );
#endif /* !NDEBUG */

        myexit( mvid, RETURN_OK, 0L );
    }
        break;

    case SEQ_START_CODE:
    {
        /* Sequence start code. Parse sequence header. */
        ParseSeqHead( mvid, vid_stream );

        /*
         * Return after sequence start code so that application above can use
         * info in header.
         */
    }
        break;

    case GOP_START_CODE:
    {
        if( mvid -> mvid_IndexScan )
        {
          mvid -> mvid_Last_PIC_SC_Pos = stream_pos( mvid );
        }

        /* Group of Pictures start code. Parse gop header. */
        ParseGOP( mvid, vid_stream );
    }
    case PICTURE_START_CODE:
    {
        /* Be sure that we don't overwrite the last GOP position ! */
        if( (data == PICTURE_START_CODE) && (mvid -> mvid_IndexScan) )
        {
          mvid -> mvid_Last_PIC_SC_Pos = stream_pos( mvid );
        }

        /* Picture start code. Parse picture header and first slice header.
         * returns currently only PARSE_OK or SKIP_PICTURE
         */
        if( ParsePicture( mvid, vid_stream, time_stamp )
#if 0
 != SKIP_PICTURE
#else
, TRUE
#endif
)
        {
          if( ((vid_stream -> picture . code_type) == I_TYPE) && (mvid -> mvid_IndexScan) )
          {
            mvid -> mvid_LastIFrameNode = NULL;
          }

          /* Add empty frame node */
          AddFrame( mvid, (UBYTE *)~0UL, NULL );

          if( ((vid_stream -> picture . code_type) == I_TYPE) && (mvid -> mvid_IndexScan) )
          {
            struct FrameNode *lastfn = ((struct FrameNode *)(mvid -> mvid_FrameList . mlh_TailPred));

            mvid -> mvid_LastIFrameNode = lastfn;
          }
        }
    }
    default:
    {
        status = SKIP_PICTURE;
    }
        break;
  }

  if( status == SKIP_PICTURE )
  {
    next_start_code( mvid );

    while( !next_bits( mvid, 32, PICTURE_START_CODE ) )
    {
      if( next_bits( mvid, 32, GOP_START_CODE ) )
      {
        break;
      }
      else
      {
        if( next_bits( mvid, 32, SEQ_END_CODE ) )
          break;
      }

      flush_bits( 24 );
      next_start_code( mvid );
    }
  }

  /* Copy global bit i/o variables back into vid_stream. */
  vid_stream -> buffer      = (unsigned int *)bitBuffer;
  vid_stream -> buf_length  = bufLength;
  vid_stream -> bit_offset  = bitOffset;

  return( vid_stream );
}


/*
 *--------------------------------------------------------------
 *
 * ParseSeqHead --
 *
 *      Assumes bit stream is at the begining of the sequence
 *      header start code. Parses off the sequence header.
 *
 * Results:
 *      Fills the vid_stream structure with values derived and
 *      decoded from the sequence header. Allocates the pict image
 *      structures based on the dimensions of the image space
 *      found in the sequence header.
 *
 * Side effects:
 *      Bit stream irreversibly parsed off.
 *
 *--------------------------------------------------------------
 */


static
void ParseSeqHead( struct MPEGVideoInstData *mvid, VidStream *vid_stream )
{
    ULONG data;
    int   i;

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    /* Flush off sequence start code. */
    flush_bits32;

    /* Get horizontal size of image space. */
    get_bits12( data );
    vid_stream -> h_size = data;

    /* Get vertical size of image space. */
    get_bits12( data );
    vid_stream -> v_size = data;

    /* Calculate macroblock width and height of image space. */
    vid_stream -> mb_width  = (vid_stream -> h_size + 15) / 16;
    vid_stream -> mb_height = (vid_stream -> v_size + 15) / 16;

    /* Initialize lmaxx, lmaxy, cmaxx, cmaxy. */
    lmaxx = vid_stream -> mb_width  * 16 - 1;
    lmaxy = vid_stream -> mb_height * 16 - 1;
    cmaxx = vid_stream -> mb_width  *  8 - 1;
    cmaxy = vid_stream -> mb_height *  8 - 1;

    /* Initialize ring buffer of pict images now that dimensions of image space are known. */
    if( vid_stream -> ring[ 0 ] == NULL )
    {
      for( i = 0 ; i < RING_BUF_SIZE ; i++ )
      {
        vid_stream -> ring[ i ] = NewPictImage( mvid, ((vid_stream -> mb_width) * 16), ((vid_stream -> mb_height) * 16) );
      }
    }

    /* Parse of aspect ratio code. */
    get_bits4( data );
    vid_stream -> aspect_ratio = (UBYTE)data;

    /* Parse off picture rate code. */
    get_bits4( data );
    vid_stream -> picture_rate = (UBYTE)data;

    /* Parse off bit rate. */
    get_bits18( data );
    vid_stream -> bit_rate = data;

    /* Flush marker bit. */
    check_marker;

    /* Parse off vbv buffer size. */
    get_bits10( data );
    vid_stream -> vbv_buffer_size = data;

    /* Parse off contrained parameter flag. */
    get_bits1( data );
    vid_stream -> const_param_flag = MAKEBOOL( data ); /* TRUE or FALSE */

    /* If intra_quant_matrix_flag set, parse off intra quant matrix values. */
    get_bits1( data );

    if( data )
    {
      for( i = 0 ; i < 64 ; i++ )
      {
        get_bits8( data );

        vid_stream -> intra_quant_matrix[ zigzag[ i ][ 1 ] ][ zigzag[ i ][ 0 ] ] = (UBYTE)data;
      }
    }

    /* If non intra quant matrix flag set, parse off non intra quant matrix values. */
    get_bits1( data );

    if( data )
    {
      for( i = 0 ; i < 64 ; i++ )
      {
        get_bits8( data );

        vid_stream -> non_intra_quant_matrix[ zigzag[ i ][ 1 ] ][ zigzag[ i ][ 0 ] ] = (UBYTE)data;
      }
    }

    /* Go to next start code. */
    next_start_code( mvid );

    /*
     * If next start code is extension start code, parse off extension data.
     */
    if( next_bits( mvid, 32, EXT_START_CODE ) )
    {
      flush_bits32;

      if( vid_stream -> ext_data )
      {
        myfree( mvid, vid_stream -> ext_data );
        vid_stream -> ext_data = NULL;
      }

      vid_stream -> ext_data = get_ext_data( mvid );
    }

    /* If next start code is user start code, parse off user data. */
    if( next_bits( mvid, 32, USER_START_CODE ) )
    {
      flush_bits32;

      if( vid_stream -> user_data )
      {
        myfree( mvid, vid_stream -> user_data );
        vid_stream -> user_data = NULL;
      }

      vid_stream -> user_data = get_ext_data( mvid );
    }
}



/*
 *--------------------------------------------------------------
 *
 * ParseGOP --
 *
 *      Parses of group of pictures header from bit stream
 *      associated with vid_stream.
 *
 * Results:
 *      Values in gop header placed into video stream structure.
 *
 * Side effects:
 *      Bit stream irreversibly parsed.
 *
 *--------------------------------------------------------------
 */


static
void ParseGOP( struct MPEGVideoInstData *mvid, VidStream *vid_stream )
{
    ULONG data;

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    /* Flush group of pictures start code. WWWWWWOOOOOOOSSSSSSHHHHH!!! */
    flush_bits32;

    /* Parse off drop frame flag. */
    get_bits1( data );
    vid_stream -> group . drop_flag = MAKEBOOL( data ); /* TRUE or FALSE */

    /* Parse off hour component of time code. */
    get_bits5( data );
    vid_stream -> group . tc_hours = (UBYTE)data;

    /* Parse off minute component of time code. */
    get_bits6( data );
    vid_stream -> group . tc_minutes = (UBYTE)data;

    /* Flush marker bit. */
    check_marker;

    /* Parse off second component of time code. */
    get_bits6( data );
    vid_stream -> group . tc_seconds = (UBYTE)data;

    /* Parse off picture count component of time code. */
    get_bits6( data );
    vid_stream -> group . tc_pictures = (UBYTE)data;

    /* Parse off closed gop and broken link flags. */
    get_bits2( data );

    if( data > 1 )
    {
      vid_stream -> group . closed_gop  = TRUE;
      vid_stream -> group . broken_link = (BOOL)(data > 2);
    }
    else
    {
      vid_stream -> group . closed_gop  = FALSE;
      vid_stream -> group . broken_link = MAKEBOOL( data );
    }

    /* Goto next start code. */
    next_start_code( mvid );

    /* If next start code is extension data, parse off extension data. */
    if( next_bits( mvid, 32, EXT_START_CODE ) )
    {
      flush_bits32;

      if( vid_stream -> group . ext_data )
      {
        myfree( mvid, vid_stream -> group . ext_data );
        vid_stream -> group . ext_data = NULL;
      }

      vid_stream -> group . ext_data = get_ext_data( mvid );
    }

    /* If next start code is user data, parse off user data. */
    if( next_bits( mvid, 32, USER_START_CODE ) )
    {
      flush_bits32;

      if( vid_stream -> group . user_data )
      {
        myfree( mvid, vid_stream -> group . user_data );
        vid_stream -> group . user_data = NULL;
      }

      vid_stream -> group . user_data = get_ext_data( mvid );
    }
}


/*
 *--------------------------------------------------------------
 *
 * ParsePicture --
 *
 *      Parses picture header. Marks picture to be presented
 *      at particular time given a time stamp.
 *
 * Results:
 *      Values from picture header put into video stream structure.
 *
 * Side effects:
 *      Bit stream irreversibly parsed.
 *
 *--------------------------------------------------------------
 */


static
int ParsePicture( struct MPEGVideoInstData *mvid, VidStream *vid_stream, TimeStamp time_stamp )
{
    ULONG data;

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    /* Flush header start code. */
    flush_bits32;

    /* Parse off temporal reference. */
    get_bits10( data );
    vid_stream -> picture . temp_ref = data;

    /* Parse of picture type. */
    get_bits3( data );
    vid_stream -> picture . code_type = data;

    switch( (vid_stream -> picture . code_type) )
    {
      case I_TYPE:
      {
          /* new in V1.10: track the start of the last I frame */

          mvid -> mvid_Last_I_TYPE_Pos = mvid -> mvid_Last_PIC_SC_Pos;

          debug_printf( mvid, "%s: I_TYPE frame start at %lx\n", __func__, (mvid -> mvid_Last_I_TYPE_Pos) );
      }
          break;

      case B_TYPE:
      {
          debug_printf( mvid, "%s: B_TYPE  frame found\n", __func__);

          if( No_B_Flag  )
          {
            return( SKIP_PICTURE );
          }

          if( (vid_stream -> past == NULL) || (vid_stream -> future == NULL) )
          {
            debug_printf( mvid, "%s: no past or future picture\n", __func__);

            return( SKIP_PICTURE );
          }
      }
          break;

      case P_TYPE:
      {
          debug_printf( mvid, "%s: P_TYPE  frame found\n", __func__);

          if( No_P_Flag )
          {
            return( SKIP_PICTURE );
          }

          if( vid_stream -> future == NULL )
          {
            debug_printf( mvid, "%s: no future picture\n", __func__);

            return( SKIP_PICTURE );
          }
      }
          break;

      case D_TYPE:
      {
          debug_printf( mvid, "%s: D_TYPE  frame found\n", __func__);
      }
          break;

      default:
      {
          debug_printf( mvid, "%s: unknown frame found\n", __func__);
      }
          break;
    }

    /* Parse off vbv buffer delay value. */
    get_bits16( data );
    vid_stream -> picture . vbv_delay = data;

    /* If P or B type frame... */
    if( (vid_stream -> picture . code_type == P_TYPE) || (vid_stream -> picture . code_type == B_TYPE) )
    {
      /* Parse off forward vector full pixel flag. */
      get_bits1( data );

      vid_stream -> picture . full_pel_forw_vector = MAKEBOOL( data ); /* TRUE or FALSE */

      /* Parse of forw_r_code. */
      get_bits3( data );

      /* Decode forw_r_code into forw_r_size and forw_f. */
      vid_stream -> picture . forw_r_size = data - 1;
      vid_stream -> picture . forw_f      = (1 << vid_stream -> picture . forw_r_size);
    }

    /* If B type frame... */
    if( vid_stream -> picture . code_type == B_TYPE )
    {
      /* Parse off back vector full pixel flag. */
      get_bits1( data );

      vid_stream -> picture . full_pel_back_vector = MAKEBOOL( data ); /* TRUE or FALSE */

      /* Parse off back_r_code. */
      get_bits3( data );

      /* Decode back_r_code into back_r_size and back_f. */
      vid_stream -> picture . back_r_size = data - 1;
      vid_stream -> picture . back_f      = (1 << vid_stream -> picture . back_r_size);
    }

    /* Get extra bit picture info. */
    if( vid_stream -> picture . extra_info )
    {
      myfree( mvid, vid_stream -> picture . extra_info );
      vid_stream -> picture . extra_info = NULL;
    }

    vid_stream -> picture . extra_info = get_extra_bit_info( mvid );

    /* Goto next start code. */
    next_start_code( mvid );

    /* If start code is extension start code, parse off extension data. */
    if( next_bits( mvid, 32, EXT_START_CODE ) )
    {
      flush_bits32;

      if( vid_stream -> picture . ext_data )
      {
        myfree( mvid, vid_stream -> picture . ext_data );
        vid_stream -> picture . ext_data = NULL;
      }

      vid_stream -> picture . ext_data = get_ext_data( mvid );
    }

    /* If start code is user start code, parse off user data. */
    if( next_bits( mvid, 32, USER_START_CODE ) )
    {
      flush_bits32;

      if( vid_stream -> picture . user_data )
      {
        myfree( mvid, vid_stream -> picture . user_data );
        vid_stream -> picture . user_data = NULL;
      }

      vid_stream -> picture . user_data = get_ext_data( mvid );
    }

    /* Set current pict image structure to the one just found in ring. */
    vid_stream -> current = LockRingEntry( mvid, vid_stream );

    /* Set time stamp. */
    vid_stream -> current -> show_time = time_stamp;

    /* Reset past macroblock address field. */
    vid_stream -> mblock . past_mb_addr = -1;

    return( PARSE_OK );
}


static
PictImage *LockRingEntry( struct MPEGVideoInstData *mvid, VidStream *vid_stream )
{
    /* Find a pict image structure in ring buffer not currently locked. */
    int i = 0;

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    while( vid_stream -> ring[ i ] -> locked )
    {
      if( ++i >= RING_BUF_SIZE )
      {
        error_printf( mvid, "Fatal error. Ring buffer full. (%d)\n", i);

        myexit( mvid, RETURN_FAIL, ERROR_BUFFER_OVERFLOW );
      }
    }

    return( vid_stream -> ring[ i ] );
}


/*
 *--------------------------------------------------------------
 *
 * ParseSlice --
 *
 *      Parses off slice header.
 *
 * Results:
 *      Values found in slice header put into video stream structure.
 *
 * Side effects:
 *      Bit stream irreversibly parsed.
 *
 *--------------------------------------------------------------
 */


static
void ParseSlice( struct MPEGVideoInstData *mvid, VidStream *vid_stream )
{
    ULONG data;

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    /* Flush slice start code. */
    flush_bits( 24 );

    /* Parse off slice vertical position. */
    get_bits8( data );
    vid_stream -> slice . vert_pos = data;

    /* Parse off quantization scale. */
    get_bits5( data );
    vid_stream -> slice . quant_scale = data;

    /* Parse off extra bit slice info. */
    if( vid_stream -> slice . extra_info )
    {
      myfree( mvid, vid_stream -> slice . extra_info );
      vid_stream -> slice . extra_info = NULL;
    }

    vid_stream -> slice . extra_info = get_extra_bit_info(mvid);

    /* Reset past intrablock address. */
    vid_stream -> mblock . past_intra_addr = -2;

    /* Reset previous recon motion vectors. */
    vid_stream -> mblock . recon_right_for_prev  = 0;
    vid_stream -> mblock . recon_down_for_prev   = 0;
    vid_stream -> mblock . recon_right_back_prev = 0;
    vid_stream -> mblock . recon_down_back_prev  = 0;

    /* Reset macroblock address. */
    vid_stream -> mblock . mb_address = ((vid_stream -> slice . vert_pos - 1) * vid_stream -> mb_width) - 1;

    /* Reset past dct dc y, cr, and cb values. */
    vid_stream -> block . dct_dc_y_past  = 1024;
    vid_stream -> block . dct_dc_cr_past = 1024;
    vid_stream -> block . dct_dc_cb_past = 1024;
}


/*
 *--------------------------------------------------------------
 *
 * ParseMacroBlock --
 *
 *      Parseoff macroblock. Reconstructs DCT values. Applies
 *      inverse DCT, reconstructs motion vectors, calculates and
 *      set pixel values for macroblock in current pict image
 *      structure.
 *
 * Results:
 *      Here's where everything really happens. Welcome to the
 *      heart of darkness.
 *
 * Side effects:
 *      Bit stream irreversibly parsed off.
 *
 *--------------------------------------------------------------
 */


static
int ParseMacroBlock( struct MPEGVideoInstData *mvid, VidStream *vid_stream )
{
  int          addr_incr;
  unsigned int data;
  int          mask,
               i,
               recon_right_for      = 0,
               recon_down_for       = 0,
               recon_right_back     = 0,
               recon_down_back      = 0;
  int          zero_block_flag      = 0;
  BOOL         mb_quant         = FALSE,
               mb_motion_forw   = FALSE,
               mb_motion_back   = FALSE,
               mb_pattern       = FALSE;

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

  if( vid_stream -> current == NULL )
  {
    error_printf( mvid, "ParseMacroBlock: no current framefor stream @ 0x%p\n", vid_stream);
    myexit( mvid, RETURN_FAIL, ERROR_BUFFER_OVERFLOW );
  }

  /* Parse off macroblock address increment and add to macroblock address. */
  do
  {
    DecodeMBAddrInc( addr_incr );

    if( addr_incr == MB_ESCAPE )
    {
      vid_stream -> mblock . mb_address += 33;
      addr_incr = MB_STUFFING;
    }
  } while( addr_incr == MB_STUFFING );

  vid_stream -> mblock . mb_address += addr_incr;

  if( (vid_stream -> mblock . mb_address) > ((vid_stream -> mb_height) * (vid_stream -> mb_width) - 1) )
  {
    return( SKIP_TO_START_CODE );
  }

  /* If macroblocks have been skipped, process skipped macroblocks. */
  if( ((vid_stream -> mblock . mb_address) - (vid_stream -> mblock . past_mb_addr)) > 1 )
  {
    switch( vid_stream -> picture . code_type )
    {
      case P_TYPE: ProcessSkippedPFrameMBlocks( mvid, vid_stream );                                                                     break;
      case B_TYPE: ProcessSkippedBFrameMBlocks( mvid, vid_stream );                                                                     break;
#ifndef NDEBUG
      case I_TYPE: break;
      case D_TYPE: assert( (vid_stream -> picture . code_type) != D_TYPE );                                                       break;
      default:     error_printf( mvid, "assert %s %ld: unknown macro block %ld\n", __FILE__, __LINE__, (vid_stream -> picture . code_type) ); break;
#endif /* !NDEBUG */
    }
  }

  /* Set past macroblock address to current macroblock address. */
  vid_stream -> mblock . past_mb_addr = vid_stream -> mblock . mb_address;

  /* Based on picture type decode macroblock type. */
  switch( vid_stream -> picture . code_type )
  {
    case I_TYPE: DecodeMBTypeI( mb_quant, mb_motion_forw, mb_motion_back, mb_pattern, (vid_stream -> mblock . mb_intra) );         break;
    case P_TYPE: DecodeMBTypeP( mb_quant, mb_motion_forw, mb_motion_back, mb_pattern, (vid_stream -> mblock . mb_intra) );         break;
    case B_TYPE: DecodeMBTypeB( mb_quant, mb_motion_forw, mb_motion_back, mb_pattern, (vid_stream -> mblock . mb_intra) );         break;
#ifndef NDEBUG
    case D_TYPE: assert( (vid_stream -> picture . code_type) != D_TYPE );                                                          break;
    default:     verbose_printf( mvid, "assert %s %ld: unknown macro block %ld\n", __FILE__, __LINE__, (vid_stream -> picture . code_type) );    break;
#endif /* !NDEBUG */
  }

  /* If quantization flag set, parse off new quantization scale. */
  if( mb_quant )
  {
    get_bits5( data );
    vid_stream -> slice . quant_scale = data;
  }

  /* If forward motion vectors exist... */
  if( mb_motion_forw )
  {
    /* Parse off and decode horizontal forward motion vector. */
    DecodeMotionVectors( vid_stream -> mblock . motion_h_forw_code );

    /* If horiz. forward r data exists, parse off. */
    if( ((vid_stream -> picture . forw_f) != 1) && ((vid_stream -> mblock . motion_h_forw_code) != 0) )
    {
      get_bitsn( (vid_stream -> picture . forw_r_size), data );
      vid_stream -> mblock . motion_h_forw_r = data;
    }

    /* Parse off and decode vertical forward motion vector. */
    DecodeMotionVectors( (vid_stream -> mblock . motion_v_forw_code) );

    /* If vert. forw. r data exists, parse off. */
    if( ((vid_stream -> picture . forw_f) != 1) && ((vid_stream -> mblock . motion_v_forw_code) != 0) )
    {
      get_bitsn( (vid_stream -> picture . forw_r_size), data );
      vid_stream -> mblock . motion_v_forw_r = data;
    }
  }

  /* If back motion vectors exist... */
  if( mb_motion_back )
  {
    /* Parse off and decode horiz. back motion vector. */
    DecodeMotionVectors( (vid_stream -> mblock . motion_h_back_code) );

    /* If horiz. back r data exists, parse off. */
    if( ((vid_stream -> picture . back_f) != 1) && ((vid_stream -> mblock . motion_h_back_code) != 0) )
    {
      get_bitsn( (vid_stream -> picture . back_r_size), data );
      vid_stream -> mblock . motion_h_back_r = data;
    }

    /* Parse off and decode vert. back motion vector. */
    DecodeMotionVectors( (vid_stream -> mblock . motion_v_back_code) );

    /* If vert. back r data exists, parse off. */
    if( ((vid_stream -> picture . back_f) != 1) && ((vid_stream -> mblock . motion_v_back_code) != 0) )
    {
      get_bitsn( (vid_stream -> picture . back_r_size), data );
      vid_stream -> mblock . motion_v_back_r = data;
    }
  }

  /* If mblock pattern flag set, parse and decode CBP (code block pattern). */
  if( mb_pattern )
  {
    DecodeCBP( vid_stream -> mblock . cbp );
  }
  else /* Otherwise, set CBP to zero. */
  {
    vid_stream -> mblock . cbp = 0;
  }

  /* Reconstruct motion vectors depending on picture type. */
  if( (vid_stream -> picture . code_type) == P_TYPE )
  {
    /* If no forw motion vectors, reset previous and current vectors to 0. */
    if( !mb_motion_forw )
    {
      recon_right_for = 0;
      recon_down_for  = 0;
      vid_stream -> mblock . recon_right_for_prev = 0;
      vid_stream -> mblock . recon_down_for_prev  = 0;
    }
    /*
     * Otherwise, compute new forw motion vectors. Reset previous vectors to
     * current vectors.
     */
    else
    {
      ComputeForwVector( mvid, (&recon_right_for), (&recon_down_for) );
    }
  }

  if( (vid_stream -> picture . code_type) == B_TYPE )
  {
    /* Reset prev. and current vectors to zero if mblock is intracoded. */
    if( vid_stream -> mblock . mb_intra )
    {
      vid_stream -> mblock . recon_right_for_prev   =
        vid_stream -> mblock . recon_down_for_prev    =
        vid_stream -> mblock . recon_right_back_prev  =
        vid_stream -> mblock . recon_down_back_prev   = 0;
    }
    else
    {
      /* If no forw vectors, current vectors equal prev. vectors. */
      if( !mb_motion_forw )
      {
        recon_right_for = vid_stream -> mblock . recon_right_for_prev;
        recon_down_for  = vid_stream -> mblock . recon_down_for_prev;
      }
      else /* Otherwise compute forw. vectors. Reset prev vectors to new values. */
      {
        ComputeForwVector( mvid, &recon_right_for, &recon_down_for );
      }

      /* If no back vectors, set back vectors to prev back vectors. */
      if( !mb_motion_back )
      {
        recon_right_back = vid_stream -> mblock . recon_right_back_prev;
        recon_down_back  = vid_stream -> mblock . recon_down_back_prev;
      }
      else /* Otherwise compute new vectors and reset prev. back vectors. */
      {
        ComputeBackVector( mvid, &recon_right_back, &recon_down_back );
      }

      /*
       * Store vector existance flags in structure for possible skipped
       * macroblocks to follow.
       */
      vid_stream -> mblock . bpict_past_forw = mb_motion_forw;
      vid_stream -> mblock . bpict_past_back = mb_motion_back;
    }
  }

  /* For each possible block in macroblock. */
  if( ditherType == GRAY_DITHER )
  {
    for( mask = 32, i = 0 ; i < 4 ; mask >>= 1, i++ )
    {
      /* If block exists... */
      if( (vid_stream -> mblock . mb_intra) || (vid_stream -> mblock . cbp & mask) )
      {
        zero_block_flag = 0;

        ParseReconBlock( mvid, i );
      }
      else
      {
        zero_block_flag = 1;
      }

      /* If macroblock is intra coded... */
      if( vid_stream -> mblock . mb_intra )
      {
        ReconIMBlock( mvid, vid_stream, i );
      }
      else
      {
        if( mb_motion_forw && mb_motion_back )
        {
          ReconBiMBlock( mvid, vid_stream, i, recon_right_for, recon_down_for, recon_right_back, recon_down_back, zero_block_flag );
        }
        else
        {
          if( mb_motion_forw || (vid_stream -> picture . code_type == P_TYPE) )
          {
            ReconPMBlock( mvid, vid_stream, i, recon_right_for, recon_down_for, zero_block_flag );
          }
          else
          {
            if( mb_motion_back )
            {
              ReconBMBlock( mvid, vid_stream, i, recon_right_back, recon_down_back, zero_block_flag );
            }
          }
        }
      }
    }

    /* Kill the Chrominace blocks... */
    if( (vid_stream -> mblock . mb_intra) || (vid_stream -> mblock . cbp & 0x2) )
    {
      ParseAwayBlock( mvid, 4 );
    }

    if( (vid_stream -> mblock . mb_intra) || (vid_stream -> mblock . cbp & 0x1) )
    {
      ParseAwayBlock( mvid, 5 );
    }
  }
  else
  {
    for( mask = 32, i = 0; i < 6; mask >>= 1, i++ )
    {
      /* If block exists... */
      if( (vid_stream -> mblock . mb_intra) || (vid_stream -> mblock . cbp & mask) )
      {
        zero_block_flag = 0;
        ParseReconBlock( mvid, i );
      }
      else
      {
        zero_block_flag = 1;
      }

      /* If macroblock is intra coded... */
      if( vid_stream -> mblock . mb_intra )
      {
        ReconIMBlock( mvid, vid_stream, i );
      }
      else
      {
        if( mb_motion_forw && mb_motion_back )
        {
          ReconBiMBlock( mvid, vid_stream, i, recon_right_for, recon_down_for, recon_right_back, recon_down_back, zero_block_flag);
        }
        else
        {
          if( mb_motion_forw || (vid_stream -> picture . code_type == P_TYPE) )
          {
            ReconPMBlock( mvid, vid_stream, i, recon_right_for, recon_down_for, zero_block_flag );
          }
          else
          {
            if( mb_motion_back )
            {
              ReconBMBlock( mvid, vid_stream, i, recon_right_back, recon_down_back, zero_block_flag );
            }
          }
        }
      }
    }
  }

  /* If D Type picture, flush marker bit. */
  if( vid_stream -> picture . code_type == D_TYPE )
  {
    check_marker;
  }

  /* If macroblock was intracoded, set macroblock past intra address. */
  if( vid_stream -> mblock . mb_intra )
  {
    vid_stream -> mblock . past_intra_addr = vid_stream -> mblock . mb_address;
  }

  return( PARSE_OK );
}


/*
 *--------------------------------------------------------------
 *
 * ReconIMBlock --
 *
 *    Reconstructs intra coded macroblock.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    None.
 *
 *--------------------------------------------------------------
 */


static
void ReconIMBlock( struct MPEGVideoInstData *mvid, VidStream *vid_stream, int bnum )
{
  int    mb_row, 
         mb_col, 
         row, 
         col, 
         row_size, 
         rr;
  UBYTE *dest;

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

  /* Calculate macroblock row and column from address. */
  mb_row = (vid_stream -> mblock . mb_address) / (vid_stream -> mb_width);
  mb_col = (vid_stream -> mblock . mb_address) % (vid_stream -> mb_width);

  /* If block is luminance block... */
  if( bnum < 4 )
  {
    /* Calculate row and col values for upper left pixel of block. */
    row = mb_row * 16;
    col = mb_col * 16;

    if( bnum > 1 )
      row += 8;

    if( bnum % 2 )
      col += 8;

    /* Set dest to luminance plane of current pict image. */
    dest = vid_stream -> current -> luminance;

    /* Establish row size. */
    row_size = vid_stream -> mb_width * 16;
  }
  else /* Otherwise if block is Cr block... */
  {
    if( bnum == 4 )
    {
      /* Set dest to Cr plane of current pict image. */
      dest = vid_stream -> current -> Cr;

      /* Establish row size. */
      row_size = vid_stream -> mb_width * 8;

      /* Calculate row,col for upper left pixel of block. */
      row = mb_row * 8;
      col = mb_col * 8;
    }
    else /* Otherwise block is Cb block, and ... */
    {
      /* Set dest to Cb plane of current pict image. */
      dest = vid_stream -> current -> Cb;

      /* Establish row size. */
      row_size = vid_stream -> mb_width * 8;

      /* Calculate row,col for upper left pixel value of block. */
      row = mb_row * 8;
      col = mb_col * 8;
    }
  }

  /* For each pixel in block, set to cropped reconstructed value from inverse dct. */
  {
    WORD  *sp = (&(vid_stream -> block . dct_recon[ 0 ][ 0 ]));
    UBYTE *cm = cropTbl + MAX_NEG_CROP;

    dest += row * row_size + col;
    for( rr = 0 ; rr < 4 ; rr++, sp += 16, dest += row_size )
    {
      dest[ 0 ] = cm[ sp[ 0 ] ]; assertCrop( sp[ 0 ] );
      dest[ 1 ] = cm[ sp[ 1 ] ]; assertCrop( sp[ 1 ] );
      dest[ 2 ] = cm[ sp[ 2 ] ]; assertCrop( sp[ 2 ] );
      dest[ 3 ] = cm[ sp[ 3 ] ]; assertCrop( sp[ 3 ] );
      dest[ 4 ] = cm[ sp[ 4 ] ]; assertCrop( sp[ 4 ] );
      dest[ 5 ] = cm[ sp[ 5 ] ]; assertCrop( sp[ 5 ] );
      dest[ 6 ] = cm[ sp[ 6 ] ]; assertCrop( sp[ 6 ] );
      dest[ 7 ] = cm[ sp[ 7 ] ]; assertCrop( sp[ 7 ] );

      dest += row_size;
      dest[ 0 ] = cm[ sp[  8 ] ]; assertCrop( sp[  8 ] );
      dest[ 1 ] = cm[ sp[  9 ] ]; assertCrop( sp[  9 ] );
      dest[ 2 ] = cm[ sp[ 10 ] ]; assertCrop( sp[ 10 ] );
      dest[ 3 ] = cm[ sp[ 11 ] ]; assertCrop( sp[ 11 ] );
      dest[ 4 ] = cm[ sp[ 12 ] ]; assertCrop( sp[ 12 ] );
      dest[ 5 ] = cm[ sp[ 13 ] ]; assertCrop( sp[ 13 ] );
      dest[ 6 ] = cm[ sp[ 14 ] ]; assertCrop( sp[ 14 ] );
      dest[ 7 ] = cm[ sp[ 15 ] ]; assertCrop( sp[ 15 ] );
    }
  }
}


/*
 *--------------------------------------------------------------
 *
 * ReconPMBlock --
 *
 *    Reconstructs forward predicted macroblocks.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *--------------------------------------------------------------
 */


static
void ReconPMBlock( struct MPEGVideoInstData *mvid, VidStream *vid_stream, int bnum, int recon_right_for, int recon_down_for, int zflag )
{
  int    mb_row     = 0,
         mb_col     = 0,
         row        = 0,
         col        = 0,
         row_size   = 0,
         rr         = 0;
  UBYTE *dest       = NULL,
        *past       = NULL;
  UBYTE *rindex1    = NULL,
        *rindex2    = NULL;
  UBYTE *index      = NULL;
  WORD  *blockvals  = NULL;

#ifdef LOOSE_MPEG
  int maxx          = 0,
      maxy          = 0;
  int illegalBlock  = 0;
  int row_start     = 0,
      row_end       = 0,
      rfirst        = 0,
      rlast         = 0,
      cc            = 0,
      col_start     = 0,
      col_end       = 0,
      cfirst        = 0,
      clast         = 0;
#endif /* LOOSE_MPEG */

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

  /* Calculate macroblock row and column from address. */
  mb_row = vid_stream -> mblock.mb_address / vid_stream -> mb_width;
  mb_col = vid_stream -> mblock.mb_address % vid_stream -> mb_width;

  if( bnum < 4 )
  {
    /* Calculate right_for, down_for motion vectors. */
    (mvid -> ReconPMBlock_right_for)       = recon_right_for >> 1;
    (mvid -> ReconPMBlock_down_for)        = recon_down_for >> 1;
    (mvid -> ReconPMBlock_right_half_for)  = recon_right_for & 0x1;
    (mvid -> ReconPMBlock_down_half_for)   = recon_down_for & 0x1;

    /* Set dest to luminance plane of current pict image. */
    dest = vid_stream -> current -> luminance;

    if( vid_stream -> picture . code_type == B_TYPE )
    {
      if( vid_stream -> past )
      {
        past = vid_stream -> past -> luminance;
      }
    }
    else
    {
      /* Set predicitive frame to current future frame. */
      if( vid_stream -> future )
      {
        past = vid_stream -> future -> luminance;
      }
    }

    /* Establish row size. */
    row_size = vid_stream->mb_width << 4;

    /* Calculate row,col of upper left pixel in block. */
    row = mb_row << 4;
    col = mb_col << 4;
    if (bnum > 1)
      row += 8;
    if (bnum % 2)
      col += 8;

#ifdef LOOSE_MPEG
    /* Check for block illegality. */
    maxx = lmaxx; maxy = lmaxy;

    if (row + (mvid -> ReconPMBlock_down_for) + 7 > maxy)
      illegalBlock |= 0x4;
    else
      if( row + (mvid -> ReconPMBlock_down_for) < 0 )
        illegalBlock |= 0x1;

    if (col + (mvid -> ReconPMBlock_right_for) + 7 > maxx)
      illegalBlock |= 0x2;
    else
      if (col + (mvid -> ReconPMBlock_right_for) < 0)
        illegalBlock |= 0x8;

#endif /* LOOSE_MPEG */
  }
  else /* Otherwise, block is NOT luminance block, ... */
  {
    /* Construct motion vectors. */
    recon_right_for             /= 2;
    recon_down_for              /= 2;
    (mvid -> ReconPMBlock_right_for)       = recon_right_for >> 1;
    (mvid -> ReconPMBlock_down_for)        = recon_down_for >> 1;
    (mvid -> ReconPMBlock_right_half_for)  = recon_right_for & 0x1;
    (mvid -> ReconPMBlock_down_half_for)   = recon_down_for & 0x1;

    /* Establish row size. */
    row_size = vid_stream -> mb_width << 3;

    /* Calculate row,col of upper left pixel in block. */
    row = mb_row << 3;
    col = mb_col << 3;

#ifdef LOOSE_MPEG
    /* Check for block illegality. */
    maxx = cmaxx; maxy = cmaxy;

    if (row + (mvid -> ReconPMBlock_down_for)  + 7 > maxy)
      illegalBlock |= 0x4;
    else
      if (row + (mvid -> ReconPMBlock_down_for) < 0)
        illegalBlock |= 0x1;

    if (col + (mvid -> ReconPMBlock_right_for)  + 7 > maxx)
      illegalBlock  |= 0x2;
    else
      if (col + (mvid -> ReconPMBlock_right_for) < 0)
        illegalBlock |= 0x8;
#endif /* LOOSE_MPEG */

    /* If block is Cr block... */
    if( bnum == 4 )
    {
      /* Set dest to Cr plane of current pict image. */
      dest = vid_stream -> current -> Cr;

      if( vid_stream -> picture . code_type == B_TYPE )
      {
        if( vid_stream -> past )
        {
          past = vid_stream -> past -> Cr;
        }
      }
      else
      {
        if( vid_stream -> future )
        {
          past = vid_stream -> future -> Cr;
        }
      }
    }
    else /* Otherwise, block is Cb block... */
    {
      /* Set dest to Cb plane of current pict image. */
      dest = vid_stream -> current -> Cb;

      if( vid_stream -> picture . code_type == B_TYPE )
      {
        if( vid_stream -> past )
        {
          past = vid_stream -> past -> Cb;
        }
      }
      else
      {
        if( vid_stream -> future )
        {
          past = vid_stream -> future -> Cb;
        }
      }
    }
  }

  /* For each pixel in block... */
#ifdef LOOSE_MPEG
  if( illegalBlock )
  {
    if( illegalBlock & 0x1 )
    {
      row_start = 0;
      row_end   = row + (mvid -> ReconPMBlock_down_for) + 8;
      rfirst    = rlast = 8 - row_end;
    }
    else
    {
      if (illegalBlock & 0x4)
      {
        row_start = row + (mvid -> ReconPMBlock_down_for);
        row_end = maxy+1;
        rlast = row_end - row_start - 1;
        rfirst = 0;
      }
      else
      {
        row_start = row + (mvid -> ReconPMBlock_down_for);
        row_end = row_start+8;
        rfirst = 0;
      }
    }

    if( illegalBlock & 0x8 )
    {
      col_start = 0;
      col_end = col + (mvid -> ReconPMBlock_right_for) + 8;
      cfirst = clast = 8 - col_end;
    }
    else
    {
      if (illegalBlock & 0x2)
      {
        col_start = col + (mvid -> ReconPMBlock_right_for);
        col_end = maxx + 1;
        clast = col_end - col_start - 1;
        cfirst = 0;
      }
      else
      {
        col_start = col + (mvid -> ReconPMBlock_right_for);
        col_end = col_start + 8;
        cfirst = 0;
      }
    }

    for( rr = row_start ; rr < row_end ; rr++ )
    {
      rindex1 = past + (rr * row_size) + col_start;
      index = dest + ((row + rfirst) * row_size) + col + cfirst;

      for( cc = col_start ; cc < col_end ; cc++ )
      {
        *index++ = *rindex1++;
      }
    }

    if (illegalBlock & 0x1)
    {
      for (rr = rlast -1; rr >=0; rr--) 
      {
        index = dest + ((row + rr) * row_size) + col;
        rindex1 = dest + ((row + rlast) * row_size) + col;
        for (cc = 0; cc < 8; cc ++) 
        {
          *index++ = *rindex1++;
        }
      }
    }
    else
    {
      if (illegalBlock & 0x4)
      {
        for (rr = rlast+1; rr < 8; rr++)
        {
          index = dest + ((row + rr) * row_size) + col;
          rindex1 = dest + ((row + rlast) * row_size) + col;
          for (cc = 0; cc < 8; cc ++)
          {
            *index++ = *rindex1++;
          }
        }
      }
    }

    if (illegalBlock & 0x2)
    {
      for (cc = clast+1; cc < 8; cc++)
      {
        index = dest + (row * row_size) + (col + cc);
        rindex1 = dest + (row * row_size) + (col + clast);
        for (rr = 0; rr < 8; rr++)
        {
          *index = *rindex1;
          index += row_size;
          rindex1 += row_size;
        }
      }
    }
    else
    {
      if (illegalBlock & 0x8)
      {
        for (cc = clast-1; cc >= 0; cc--)
        {
          index = dest + (row * row_size) + (col + cc);
          rindex1 = dest + (row * row_size) + (col + clast);

          for (rr = 0; rr < 8; rr++)
          {
            *index = *rindex1;
            index += row_size;
            rindex1 += row_size;
          }
        }
      }
    }

    if( !zflag )
    {
      for( rr = 0 ; rr < 8 ; rr++ )
      {
        index = dest + (row * row_size) + col;
        blockvals = &(vid_stream -> block . dct_recon[ rr ][ 0 ]);

        index[ 0 ] += blockvals[ 0 ];
        index[ 1 ] += blockvals[ 1 ];
        index[ 2 ] += blockvals[ 2 ];
        index[ 3 ] += blockvals[ 3 ];
        index[ 4 ] += blockvals[ 4 ];
        index[ 5 ] += blockvals[ 5 ];
        index[ 6 ] += blockvals[ 6 ];
        index[ 7 ] += blockvals[ 7 ];
      }
    }
  }
  else
  {
#endif /* LOOSE_MPEG */

    index = dest + (row * row_size) + col;
    rindex1 = past + (row + (mvid -> ReconPMBlock_down_for)) * row_size + col + (mvid -> ReconPMBlock_right_for);

    blockvals = (&(vid_stream -> block . dct_recon[ 0 ][ 0 ]));

    /*
     * Calculate predictive pixel value based on motion vectors and copy to
     * dest plane.
     */
    if( (!(mvid -> ReconPMBlock_down_half_for)) && (!(mvid -> ReconPMBlock_right_half_for)) )
    {
      UBYTE *cm = cropTbl + MAX_NEG_CROP;

      if (!zflag)
    for (rr = 0; rr < 4; rr++) 
    {
      index[ 0 ] = cm[ (int)rindex1[ 0 ] + (int)blockvals[ 0 ] ];
      index[ 1 ] = cm[ (int)rindex1[ 1 ] + (int)blockvals[ 1 ] ];
      index[ 2 ] = cm[ (int)rindex1[ 2 ] + (int)blockvals[ 2 ] ];
      index[ 3 ] = cm[ (int)rindex1[ 3 ] + (int)blockvals[ 3 ] ];
      index[ 4 ] = cm[ (int)rindex1[ 4 ] + (int)blockvals[ 4 ] ];
      index[ 5 ] = cm[ (int)rindex1[ 5 ] + (int)blockvals[ 5 ] ];
      index[ 6 ] = cm[ (int)rindex1[ 6 ] + (int)blockvals[ 6 ] ];
      index[ 7 ] = cm[ (int)rindex1[ 7 ] + (int)blockvals[ 7 ] ];
      index += row_size;
      rindex1 += row_size;

      index[ 0 ] = cm[ (int)rindex1[ 0 ] + (int)blockvals[  8 ] ];
      index[ 1 ] = cm[ (int)rindex1[ 1 ] + (int)blockvals[  9 ] ];
      index[ 2 ] = cm[ (int)rindex1[ 2 ] + (int)blockvals[ 10 ] ];
      index[ 3 ] = cm[ (int)rindex1[ 3 ] + (int)blockvals[ 11 ] ];
      index[ 4 ] = cm[ (int)rindex1[ 4 ] + (int)blockvals[ 12 ] ];
      index[ 5 ] = cm[ (int)rindex1[ 5 ] + (int)blockvals[ 13 ] ];
      index[ 6 ] = cm[ (int)rindex1[ 6 ] + (int)blockvals[ 14 ] ];
      index[ 7 ] = cm[ (int)rindex1[ 7 ] + (int)blockvals[ 15 ] ];
      blockvals += 16;
      index += row_size;
      rindex1 += row_size;
    }
      else
      {
    if ((mvid -> ReconPMBlock_right_for) & 0x1)
    {
      /* No alignment, use bye copy */
      for (rr = 0; rr < 4; rr++)
      {
        index[ 0 ] = rindex1[ 0 ];
        index[ 1 ] = rindex1[ 1 ];
        index[ 2 ] = rindex1[ 2 ];
        index[ 3 ] = rindex1[ 3 ];
        index[ 4 ] = rindex1[ 4 ];
        index[ 5 ] = rindex1[ 5 ];
        index[ 6 ] = rindex1[ 6 ];
        index[ 7 ] = rindex1[ 7 ];
        index += row_size;
        rindex1 += row_size;

        index[ 0 ] = rindex1[ 0 ];
        index[ 1 ] = rindex1[ 1 ];
        index[ 2 ] = rindex1[ 2 ];
        index[ 3 ] = rindex1[ 3 ];
        index[ 4 ] = rindex1[ 4 ];
        index[ 5 ] = rindex1[ 5 ];
        index[ 6 ] = rindex1[ 6 ];
        index[ 7 ] = rindex1[ 7 ];
        index += row_size;
        rindex1 += row_size;
      }
    }
    else
      if ((mvid -> ReconPMBlock_right_for) & 0x2)
      {
      /* Half-word bit aligned, use 16 bit copy */
      WORD *src = (WORD *)rindex1;
      WORD *dest = (WORD *)index;
      row_size >>= 1;
      for (rr = 0; rr < 4; rr++)
     {
        dest[0] = src[0];
        dest[1] = src[1];
        dest[2] = src[2];
        dest[3] = src[3];
        dest += row_size;
        src += row_size;

        dest[0] = src[0];
        dest[1] = src[1];
        dest[2] = src[2];
        dest[3] = src[3];
        dest += row_size;
        src += row_size;
      }
    }
    else
    {
      /* Word aligned, use 32 bit copy */
      int *src = (int *)rindex1;
      int *dest = (int *)index;
      row_size >>= 2;
      for (rr = 0; rr < 4; rr++)
      {
        dest[0] = src[0];
        dest[1] = src[1];
        dest += row_size;
        src += row_size;

        dest[0] = src[0];
        dest[1] = src[1];
        dest += row_size;
        src += row_size;
      }
    }
      }
    }
    else
    {
      UBYTE *cm = cropTbl + MAX_NEG_CROP;

      rindex2 = rindex1 + (mvid -> ReconPMBlock_right_half_for) + ((mvid -> ReconPMBlock_down_half_for) * row_size);
      if (!zflag)
    for (rr = 0; rr < 4; rr++)
    {
      index[ 0 ] = cm[ ((int)(rindex1[ 0 ] + rindex2[ 0 ]) >> 1) + blockvals[ 0 ] ];
      index[ 1 ] = cm[ ((int)(rindex1[ 1 ] + rindex2[ 1 ]) >> 1) + blockvals[ 1 ] ];
      index[ 2 ] = cm[ ((int)(rindex1[ 2 ] + rindex2[ 2 ]) >> 1) + blockvals[ 2 ] ];
      index[ 3 ] = cm[ ((int)(rindex1[ 3 ] + rindex2[ 3 ]) >> 1) + blockvals[ 3 ] ];
      index[ 4 ] = cm[ ((int)(rindex1[ 4 ] + rindex2[ 4 ]) >> 1) + blockvals[ 4 ] ];
      index[ 5 ] = cm[ ((int)(rindex1[ 5 ] + rindex2[ 5 ]) >> 1) + blockvals[ 5 ] ];
      index[ 6 ] = cm[ ((int)(rindex1[ 6 ] + rindex2[ 6 ]) >> 1) + blockvals[ 6 ] ];
      index[ 7 ] = cm[ ((int)(rindex1[ 7 ] + rindex2[ 7 ]) >> 1) + blockvals[ 7 ] ];
      index += row_size;
      rindex1 += row_size;
      rindex2 += row_size;

      index[ 0 ] = cm[ ((int)(rindex1[ 0 ] + rindex2[ 0 ]) >> 1) + blockvals[  8 ] ];
      index[ 1 ] = cm[ ((int)(rindex1[ 1 ] + rindex2[ 1 ]) >> 1) + blockvals[  9 ] ];
      index[ 2 ] = cm[ ((int)(rindex1[ 2 ] + rindex2[ 2 ]) >> 1) + blockvals[ 10 ] ];
      index[ 3 ] = cm[ ((int)(rindex1[ 3 ] + rindex2[ 3 ]) >> 1) + blockvals[ 11 ] ];
      index[ 4 ] = cm[ ((int)(rindex1[ 4 ] + rindex2[ 4 ]) >> 1) + blockvals[ 12 ] ];
      index[ 5 ] = cm[ ((int)(rindex1[ 5 ] + rindex2[ 5 ]) >> 1) + blockvals[ 13 ] ];
      index[ 6 ] = cm[ ((int)(rindex1[ 6 ] + rindex2[ 6 ]) >> 1) + blockvals[ 14 ] ];
      index[ 7 ] = cm[ ((int)(rindex1[ 7 ] + rindex2[ 7 ]) >> 1) + blockvals[ 15 ] ];
      blockvals += 16;
      index += row_size;
      rindex1 += row_size;
      rindex2 += row_size;
    }
    else
    for( rr = 0 ; rr < 4 ; rr++ )
    {
      index[ 0 ] = (int)(rindex1[ 0 ] + rindex2[ 0 ]) >> 1;
      index[ 1 ] = (int)(rindex1[ 1 ] + rindex2[ 1 ]) >> 1;
      index[ 2 ] = (int)(rindex1[ 2 ] + rindex2[ 2 ]) >> 1;
      index[ 3 ] = (int)(rindex1[ 3 ] + rindex2[ 3 ]) >> 1;
      index[ 4 ] = (int)(rindex1[ 4 ] + rindex2[ 4 ]) >> 1;
      index[ 5 ] = (int)(rindex1[ 5 ] + rindex2[ 5 ]) >> 1;
      index[ 6 ] = (int)(rindex1[ 6 ] + rindex2[ 6 ]) >> 1;
      index[ 7 ] = (int)(rindex1[ 7 ] + rindex2[ 7 ]) >> 1;
      index += row_size;
      rindex1 += row_size;
      rindex2 += row_size;

      index[ 0 ] = (int)(rindex1[ 0 ] + rindex2[ 0 ]) >> 1;
      index[ 1 ] = (int)(rindex1[ 1 ] + rindex2[ 1 ]) >> 1;
      index[ 2 ] = (int)(rindex1[ 2 ] + rindex2[ 2 ]) >> 1;
      index[ 3 ] = (int)(rindex1[ 3 ] + rindex2[ 3 ]) >> 1;
      index[ 4 ] = (int)(rindex1[ 4 ] + rindex2[ 4 ]) >> 1;
      index[ 5 ] = (int)(rindex1[ 5 ] + rindex2[ 5 ]) >> 1;
      index[ 6 ] = (int)(rindex1[ 6 ] + rindex2[ 6 ]) >> 1;
      index[ 7 ] = (int)(rindex1[ 7 ] + rindex2[ 7 ]) >> 1;
      index += row_size;
      rindex1 += row_size;
      rindex2 += row_size;
    }
    }

#ifdef LOOSE_MPEG
  }
#endif /* LOOSE_MPEG */
}


/*
 *--------------------------------------------------------------
 *
 * ReconBMBlock --
 *
 *    Reconstructs back predicted macroblocks.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *--------------------------------------------------------------
 */


static
void ReconBMBlock( struct MPEGVideoInstData *mvid, VidStream *vid_stream, int bnum, int recon_right_back, int recon_down_back, int zflag )
{
  int    mb_row          = 0,
         mb_col          = 0,
         row, col        = 0,
         row_size        = 0,
         rr              = 0;
  UBYTE *dest            = NULL,
        *future          = NULL;
  int    right_back      = 0,
         down_back       = 0,
         right_half_back = 0,
         down_half_back  = 0;
  UBYTE *rindex1         = NULL,
        *rindex2         = NULL;
  UBYTE *index           = NULL;
  WORD  *blockvals       = NULL;

#ifdef LOOSE_MPEG
  int illegalBlock  = 0;
  int maxx          = 0,
      maxy          = 0;
  int row_start     = 0,
      row_end       = 0,
      rlast         = 0,
      rfirst        = 0,
      cc            = 0,
      col_start     = 0,
      col_end       = 0,
      clast         = 0,
      cfirst        = 0;
#endif /* LOOSE_MPEG */

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

  /* Calculate macroblock row and column from address. */
  mb_row = vid_stream -> mblock . mb_address / vid_stream -> mb_width;
  mb_col = vid_stream -> mblock . mb_address % vid_stream -> mb_width;

  /* If block is luminance block... */
  if( bnum < 4 )
  {
    /* Calculate right_back, down_bakc motion vectors. */
    right_back       = recon_right_back >> 1;
    down_back        = recon_down_back  >> 1;
    right_half_back  = recon_right_back & 0x1;
    down_half_back   = recon_down_back  & 0x1;

    /* Set dest to luminance plane of current pict image. */
    dest = vid_stream->current->luminance;

    /*
     * If future frame exists, set future to luminance plane of future frame.
     */
    if (vid_stream->future )
      future = vid_stream->future->luminance;

    /* Establish row size. */
    row_size = vid_stream->mb_width << 4;

    /* Calculate row,col of upper left pixel in block. */
    row = mb_row << 4;
    col = mb_col << 4;
    if (bnum > 1)
      row += 8;
    if (bnum % 2)
      col += 8;

#ifdef LOOSE_MPEG
    /* Check for block illegality. */
    maxx = lmaxx; maxy = lmaxy;

    if (row + down_back + 7 > maxy) illegalBlock |= 0x4;
    else if (row + down_back < 0)  illegalBlock |= 0x1;

    if (col + right_back + 7 > maxx) illegalBlock |= 0x2;
    else if (col + right_back < 0) illegalBlock |= 0x8;
#endif /* LOOSE_MPEG */
  }
  else /* Otherwise, block is NOT luminance block, ... */
  {
    /* Construct motion vectors. */
    recon_right_back   /= 2;
    recon_down_back    /= 2;
    right_back          = recon_right_back >> 1;
    down_back           = recon_down_back  >> 1;
    right_half_back     = recon_right_back & 0x1;
    down_half_back      = recon_down_back  & 0x1;

    /* Establish row size. */
    row_size = vid_stream -> mb_width << 3;

    /* Calculate row,col of upper left pixel in block. */
    row = mb_row << 3;
    col = mb_col << 3;

#ifdef LOOSE_MPEG
    /* Check for block illegality. */
    maxx = cmaxx; maxy = cmaxy;

    if (row + down_back + 7 > maxy) illegalBlock |= 0x4;
    else if (row + down_back < 0) illegalBlock |= 0x1;

    if (col + right_back + 7 > maxx) illegalBlock  |= 0x2;
    else if (col + right_back < 0) illegalBlock |= 0x8;
#endif /* LOOSE_MPEG */

    /* If block is Cr block... */
    if( bnum == 4 )
    {
      /* Set dest to Cr plane of current pict image. */
      dest = vid_stream -> current -> Cr;

      /*
       * If future frame exists, set future to Cr plane of future image.
       */

      if( vid_stream -> future )
        future = vid_stream -> future -> Cr;
    }
    else /* Otherwise, block is Cb block... */
    {
      /* Set dest to Cb plane of current pict image. */
      dest = vid_stream -> current -> Cb;

      /*
       * If future frame exists, set future to Cb plane of future frame.
       */

      if( vid_stream -> future )
        future = vid_stream -> future -> Cb;
    }
  }

  /* For each pixel in block do... */
#ifdef LOOSE_MPEG
  if( illegalBlock )
  {
    if( illegalBlock & 0x1 )
    {
      row_start = 0;
      row_end = row+down_back+8;
      rfirst = rlast = 8 - row_end;
    }
    else if (illegalBlock & 0x4)
    {
      row_start = row + down_back;
      row_end = maxy+1;
      rlast = row_end - row_start - 1;
      rfirst = 0;
    }
    else
    {
      row_start = row+down_back;
      row_end = row_start+8;
      rfirst = 0;
    }

    if (illegalBlock & 0x8)
    {
      col_start = 0;
      col_end = col + right_back + 8;
      cfirst = clast = 8 - col_end;
    }
    else if (illegalBlock & 0x2)
    {
      col_start = col + right_back;
      col_end = maxx + 1;
      clast = col_end - col_start - 1;
      cfirst = 0;
    }
    else
    {
      col_start = col + right_back;
      col_end = col_start + 8;
      cfirst = 0;
    }

    for (rr = row_start; rr < row_end; rr++)
    {
      rindex1 = future + (rr * row_size) + col_start;
      index = dest + ((row + rfirst) * row_size) + col + cfirst;

      for (cc = col_start; cc < col_end; cc++)
      {
        *index++ = *rindex1++;
      }
    }

    if (illegalBlock & 0x1)
    {
      for (rr = rlast -1; rr >=0; rr--)
      {
        index = dest + ((row + rr) * row_size) + col;
        rindex1 = dest + ((row + rlast) * row_size) + col;
        for (cc = 0; cc < 8; cc ++)
        {
          *index++ = *rindex1++;
        }
      }
    }
    else if (illegalBlock & 0x4)
    {
      for (rr = rlast+1; rr < 8; rr++)
      {
        index = dest + ((row + rr) * row_size) + col;
        rindex1 = dest + ((row + rlast) * row_size) + col;
        for (cc = 0; cc < 8; cc ++)
        {
          *index++ = *rindex1++;
        }
      }
    }

    if (illegalBlock & 0x2) {
      for (cc = clast+1; cc < 8; cc++) {
    index = dest + (row * row_size) + (col + cc);
    rindex1 = dest + (row * row_size) + (col + clast);
    for (rr = 0; rr < 8; rr++) {
      *index = *rindex1;
      index += row_size;
      rindex1 += row_size;
    }
      }
    }
    else if (illegalBlock & 0x8) {
      for (cc = clast-1; cc >= 0; cc--) {
    index = dest + (row * row_size) + (col + cc);
    rindex1 = dest + (row * row_size) + (col + clast);
    for (rr = 0; rr < 8; rr++) {
      *index = *rindex1;
      index += row_size;
      rindex1 += row_size;
    }
      }
    }

    if (!zflag) {
      for (rr = 0; rr < 8; rr++) {
    index = dest + (row*row_size) + col;
    blockvals = &(vid_stream->block.dct_recon[rr][0]);
    index[0] += blockvals[0];
    index[1] += blockvals[1];
    index[2] += blockvals[2];
    index[3] += blockvals[3];
    index[4] += blockvals[4];
    index[5] += blockvals[5];
    index[6] += blockvals[6];
    index[7] += blockvals[7];
      }
    }
  }
  else {
#endif /* LOOSE_MPEG */

    index = dest + (row * row_size) + col;
    rindex1 = future + (row + down_back) * row_size + col + right_back;

    blockvals = &(vid_stream->block.dct_recon[0][0]);

    if ((!right_half_back) && (!down_half_back)) 
    {
      UBYTE *cm = cropTbl + MAX_NEG_CROP;
      if (!zflag)
    for (rr = 0; rr < 4; rr++) 
    {
      index[0] = cm[(int) rindex1[0] + (int) blockvals[0]];
      index[1] = cm[(int) rindex1[1] + (int) blockvals[1]];
      index[2] = cm[(int) rindex1[2] + (int) blockvals[2]];
      index[3] = cm[(int) rindex1[3] + (int) blockvals[3]];
      index[4] = cm[(int) rindex1[4] + (int) blockvals[4]];
      index[5] = cm[(int) rindex1[5] + (int) blockvals[5]];
      index[6] = cm[(int) rindex1[6] + (int) blockvals[6]];
      index[7] = cm[(int) rindex1[7] + (int) blockvals[7]];
      index += row_size;
      rindex1 += row_size;

      index[0] = cm[(int) rindex1[0] + (int) blockvals[8]];
      index[1] = cm[(int) rindex1[1] + (int) blockvals[9]];
      index[2] = cm[(int) rindex1[2] + (int) blockvals[10]];
      index[3] = cm[(int) rindex1[3] + (int) blockvals[11]];
      index[4] = cm[(int) rindex1[4] + (int) blockvals[12]];
      index[5] = cm[(int) rindex1[5] + (int) blockvals[13]];
      index[6] = cm[(int) rindex1[6] + (int) blockvals[14]];
      index[7] = cm[(int) rindex1[7] + (int) blockvals[15]];
      blockvals += 16;
      index += row_size;
      rindex1 += row_size;
    }
      else {
    if (right_back & 0x1) {
      /* No alignment, use bye copy */
      for (rr = 0; rr < 4; rr++) {
        index[0] = rindex1[0];
        index[1] = rindex1[1];
        index[2] = rindex1[2];
        index[3] = rindex1[3];
        index[4] = rindex1[4];
        index[5] = rindex1[5];
        index[6] = rindex1[6];
        index[7] = rindex1[7];
        index += row_size;
        rindex1 += row_size;

        index[0] = rindex1[0];
        index[1] = rindex1[1];
        index[2] = rindex1[2];
        index[3] = rindex1[3];
        index[4] = rindex1[4];
        index[5] = rindex1[5];
        index[6] = rindex1[6];
        index[7] = rindex1[7];
        index += row_size;
        rindex1 += row_size;
      }
    } else if (right_back & 0x2) {
      /* Half-word bit aligned, use 16 bit copy */
      WORD *src = (WORD *)rindex1;
      WORD *dest = (WORD *)index;
      row_size >>= 1;
      for (rr = 0; rr < 4; rr++) {
        dest[0] = src[0];
        dest[1] = src[1];
        dest[2] = src[2];
        dest[3] = src[3];
        dest += row_size;
        src += row_size;

        dest[0] = src[0];
        dest[1] = src[1];
        dest[2] = src[2];
        dest[3] = src[3];
        dest += row_size;
        src += row_size;
      }
    } else {
      /* Word aligned, use 32 bit copy */
      int *src = (int *)rindex1;
      int *dest = (int *)index;
      row_size >>= 2;
      for (rr = 0; rr < 4; rr++) {
        dest[0] = src[0];
        dest[1] = src[1];
        dest += row_size;
        src += row_size;

        dest[0] = src[0];
        dest[1] = src[1];
        dest += row_size;
        src += row_size;
      }
    }
      }
    } else {
      UBYTE *cm = cropTbl + MAX_NEG_CROP;
      rindex2 = rindex1 + right_half_back + (down_half_back * row_size);
      if (!zflag)
    for (rr = 0; rr < 4; rr++) {
      index[0] = cm[((int) (rindex1[0] + rindex2[0]) >> 1) + blockvals[0]];
      index[1] = cm[((int) (rindex1[1] + rindex2[1]) >> 1) + blockvals[1]];
      index[2] = cm[((int) (rindex1[2] + rindex2[2]) >> 1) + blockvals[2]];
      index[3] = cm[((int) (rindex1[3] + rindex2[3]) >> 1) + blockvals[3]];
      index[4] = cm[((int) (rindex1[4] + rindex2[4]) >> 1) + blockvals[4]];
      index[5] = cm[((int) (rindex1[5] + rindex2[5]) >> 1) + blockvals[5]];
      index[6] = cm[((int) (rindex1[6] + rindex2[6]) >> 1) + blockvals[6]];
      index[7] = cm[((int) (rindex1[7] + rindex2[7]) >> 1) + blockvals[7]];
      index += row_size;
      rindex1 += row_size;
      rindex2 += row_size;

      index[0] = cm[((int) (rindex1[0] + rindex2[0]) >> 1) + blockvals[8]];
      index[1] = cm[((int) (rindex1[1] + rindex2[1]) >> 1) + blockvals[9]];
      index[2] = cm[((int) (rindex1[2] + rindex2[2]) >> 1) + blockvals[10]];
      index[3] = cm[((int) (rindex1[3] + rindex2[3]) >> 1) + blockvals[11]];
      index[4] = cm[((int) (rindex1[4] + rindex2[4]) >> 1) + blockvals[12]];
      index[5] = cm[((int) (rindex1[5] + rindex2[5]) >> 1) + blockvals[13]];
      index[6] = cm[((int) (rindex1[6] + rindex2[6]) >> 1) + blockvals[14]];
      index[7] = cm[((int) (rindex1[7] + rindex2[7]) >> 1) + blockvals[15]];
      blockvals += 16;
      index += row_size;
      rindex1 += row_size;
      rindex2 += row_size;
    }
      else
    for (rr = 0; rr < 4; rr++) {
      index[0] = (int) (rindex1[0] + rindex2[0]) >> 1;
      index[1] = (int) (rindex1[1] + rindex2[1]) >> 1;
      index[2] = (int) (rindex1[2] + rindex2[2]) >> 1;
      index[3] = (int) (rindex1[3] + rindex2[3]) >> 1;
      index[4] = (int) (rindex1[4] + rindex2[4]) >> 1;
      index[5] = (int) (rindex1[5] + rindex2[5]) >> 1;
      index[6] = (int) (rindex1[6] + rindex2[6]) >> 1;
      index[7] = (int) (rindex1[7] + rindex2[7]) >> 1;
      index += row_size;
      rindex1 += row_size;
      rindex2 += row_size;

      index[0] = (int) (rindex1[0] + rindex2[0]) >> 1;
      index[1] = (int) (rindex1[1] + rindex2[1]) >> 1;
      index[2] = (int) (rindex1[2] + rindex2[2]) >> 1;
      index[3] = (int) (rindex1[3] + rindex2[3]) >> 1;
      index[4] = (int) (rindex1[4] + rindex2[4]) >> 1;
      index[5] = (int) (rindex1[5] + rindex2[5]) >> 1;
      index[6] = (int) (rindex1[6] + rindex2[6]) >> 1;
      index[7] = (int) (rindex1[7] + rindex2[7]) >> 1;
      index += row_size;
      rindex1 += row_size;
      rindex2 += row_size;
    }
    }

#ifdef LOOSE_MPEG
  }
#endif /* LOOSE_MPEG  */
}


/*
 *--------------------------------------------------------------
 *
 * ReconBiMBlock --
 *
 *    Reconstructs bidirectionally predicted macroblocks.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *--------------------------------------------------------------
 */


static
void ReconBiMBlock( struct MPEGVideoInstData *mvid, VidStream *vid_stream, int bnum, int recon_right_for, int recon_down_for, int recon_right_back, int recon_down_back, int zflag )
{
  int mb_row, mb_col, row, col, row_size, rr;
  UBYTE  *dest       = NULL,
         *past       = NULL,
         *future     = NULL;
  int     right_for,
          down_for
          /*,right_half_for,*/
          /*down_half_for*/;
  int     right_back,
          down_back
          /* right_half_back,
          down_half_back*/;
  UBYTE  *index      = NULL,
         *rindex1    = NULL,
         *bindex1    = NULL;
  WORD   *blockvals  = NULL;
  int     forw_row_start,
          back_row_start,
          forw_col_start,
          back_col_start;

#ifdef LOOSE_MPEG
  int illegal_forw = 0;
  int illegal_back = 0;
#endif /* LOOSE_MPEG */

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    /* Calculate macroblock row and column from address. */
  mb_row = vid_stream -> mblock . mb_address / vid_stream -> mb_width;
  mb_col = vid_stream -> mblock . mb_address % vid_stream -> mb_width;

  /* If block is luminance block... */

  if( bnum < 4 )
  {
    /*
     * Calculate right_for, down_for, right_half_for, down_half_for,
     * right_back, down_bakc, right_half_back, and down_half_back, motion
     * vectors.
     */
    right_for = recon_right_for >> 1;
    down_for = recon_down_for >> 1;
/*    right_half_for = recon_right_for & 0x1; */
/*    down_half_for = recon_down_for & 0x1; */

    right_back = recon_right_back >> 1;
    down_back = recon_down_back >> 1;
/*    right_half_back = recon_right_back & 0x1; */
/*    down_half_back = recon_down_back & 0x1; */

    /* Set dest to luminance plane of current pict image. */

    dest = vid_stream -> current -> luminance;

    /* If past frame exists, set past to luminance plane of past frame. */
    if( vid_stream -> past )
      past = vid_stream -> past -> luminance;

    /* If future frame exists, set future to luminance plane of future frame. */
    if( vid_stream -> future )
      future = vid_stream->future->luminance;

    /* Establish row size. */
    row_size = (vid_stream->mb_width << 4);

    /* Calculate row,col of upper left pixel in block. */
    row = (mb_row << 4);
    col = (mb_col << 4);
    if (bnum > 1)
      row += 8;
    if (bnum & 0x01)
      col += 8;

    forw_col_start = col + right_for;
    forw_row_start = row + down_for;

    back_col_start = col + right_back;
    back_row_start = row + down_back;

#ifdef LOOSE_MPEG
    /* Check for illegal pred. blocks. */
    if (forw_col_start+8 > lmaxx) illegal_forw = 1;
    else if (forw_col_start < 0) illegal_forw = 1;

    if (forw_row_start+8 > lmaxy) illegal_forw = 1;
    else if (forw_row_start < 0) illegal_forw = 1;

    if (back_col_start+8 > lmaxx) illegal_back = 1;
    else if (back_col_start < 0) illegal_back = 1;

    if (back_row_start+8 > lmaxy) illegal_back = 1;
    else if (back_row_start < 0) illegal_back = 1;
#endif /* LOOSE_MPEG */
  }
  else /* Otherwise, block is NOT luminance block, ... */
  {
    /* Construct motion vectors. */
    recon_right_for /= 2;
    recon_down_for /= 2;
    right_for = recon_right_for >> 1;
    down_for = recon_down_for >> 1;
/*    right_half_for = recon_right_for & 0x1; */
/*    down_half_for = recon_down_for & 0x1; */

    recon_right_back /= 2;
    recon_down_back /= 2;
    right_back = recon_right_back >> 1;
    down_back = recon_down_back >> 1;
/*    right_half_back = recon_right_back & 0x1; */
/*    down_half_back = recon_down_back & 0x1; */

    /* Establish row size. */
    row_size = (vid_stream->mb_width << 3);

    /* Calculate row,col of upper left pixel in block. */
    row = (mb_row << 3);
    col = (mb_col << 3);

    forw_col_start = col + right_for;
    forw_row_start = row + down_for;

    back_col_start = col + right_back;
    back_row_start = row + down_back;

#ifdef LOOSE_MPEG
    /* Check for illegal pred. blocks. */
    if (forw_col_start+8 > cmaxx) illegal_forw = 1;
    else if (forw_col_start < 0) illegal_forw = 1;

    if (forw_row_start+8 > cmaxy) illegal_forw = 1;
    else if (forw_row_start < 0) illegal_forw = 1;

    if (back_col_start+8 > cmaxx) illegal_back = 1;
    else if (back_col_start < 0) illegal_back = 1;

    if (back_row_start+8 > cmaxy) illegal_back = 1;
    else if (back_row_start < 0) illegal_back = 1;
#endif /* LOOSE_MPEG */

    /* If block is Cr block... */
    if( bnum == 4 )
    {
      /* Set dest to Cr plane of current pict image. */
      dest = vid_stream -> current -> Cr;

      /* If past frame exists, set past to Cr plane of past image. */
      if( vid_stream -> past )
        past = vid_stream -> past -> Cr;

      /* If future frame exists, set future to Cr plane of future image. */
      if( vid_stream -> future )
        future = vid_stream -> future -> Cr;
    }
    else /* Otherwise, block is Cb block... */
    {
      /* Set dest to Cb plane of current pict image. */
      dest = vid_stream -> current -> Cb;

      /* If past frame exists, set past to Cb plane of past frame. */
      if( vid_stream -> past )
        past = vid_stream -> past -> Cb;

      /* If future frame exists, set future to Cb plane of future frame. */
      if( vid_stream -> future )
        future = vid_stream -> future -> Cb;
    }
  }

  /* For each pixel in block... */
  index = dest + (row * row_size) + col;

#ifdef LOOSE_MPEG
  if (illegal_forw)
    rindex1 = future + back_row_start * row_size + back_col_start;
  else
#endif /* LOOSE_MPEG */
    rindex1 = past + forw_row_start  * row_size + forw_col_start;

#ifdef LOOSE_MPEG
  if (illegal_back)
    bindex1 = past + forw_row_start * row_size + forw_col_start;
  else
#endif /* LOOSE_MPEG */
    bindex1 = future + back_row_start * row_size + back_col_start;

  blockvals = (WORD *)(&(vid_stream -> block . dct_recon[ 0 ][ 0 ] ));

  {
    UBYTE *cm = cropTbl + MAX_NEG_CROP;

    if( !zflag )
    {
      for( rr = 0 ; rr < 4 ; rr++ )
      {
        index[ 0 ] = cm[ ((int)(rindex1[ 0 ] + bindex1[ 0 ]) >> 1) + blockvals[ 0 ] ];
        index[ 1 ] = cm[ ((int)(rindex1[ 1 ] + bindex1[ 1 ]) >> 1) + blockvals[ 1 ] ];
        index[ 2 ] = cm[ ((int)(rindex1[ 2 ] + bindex1[ 2 ]) >> 1) + blockvals[ 2 ] ];
        index[ 3 ] = cm[ ((int)(rindex1[ 3 ] + bindex1[ 3 ]) >> 1) + blockvals[ 3 ] ];
        index[ 4 ] = cm[ ((int)(rindex1[ 4 ] + bindex1[ 4 ]) >> 1) + blockvals[ 4 ] ];
        index[ 5 ] = cm[ ((int)(rindex1[ 5 ] + bindex1[ 5 ]) >> 1) + blockvals[ 5 ] ];
        index[ 6 ] = cm[ ((int)(rindex1[ 6 ] + bindex1[ 6 ]) >> 1) + blockvals[ 6 ] ];
        index[ 7 ] = cm[ ((int)(rindex1[ 7 ] + bindex1[ 7 ]) >> 1) + blockvals[ 7 ] ];
        index   += row_size;
        rindex1 += row_size;
        bindex1 += row_size;

        index[ 0 ] = cm[ ((int)(rindex1[ 0 ] + bindex1[ 0 ]) >> 1) + blockvals[  8 ] ];
        index[ 1 ] = cm[ ((int)(rindex1[ 1 ] + bindex1[ 1 ]) >> 1) + blockvals[  9 ] ];
        index[ 2 ] = cm[ ((int)(rindex1[ 2 ] + bindex1[ 2 ]) >> 1) + blockvals[ 10 ] ];
        index[ 3 ] = cm[ ((int)(rindex1[ 3 ] + bindex1[ 3 ]) >> 1) + blockvals[ 11 ] ];
        index[ 4 ] = cm[ ((int)(rindex1[ 4 ] + bindex1[ 4 ]) >> 1) + blockvals[ 12 ] ];
        index[ 5 ] = cm[ ((int)(rindex1[ 5 ] + bindex1[ 5 ]) >> 1) + blockvals[ 13 ] ];
        index[ 6 ] = cm[ ((int)(rindex1[ 6 ] + bindex1[ 6 ]) >> 1) + blockvals[ 14 ] ];
        index[ 7 ] = cm[ ((int)(rindex1[ 7 ] + bindex1[ 7 ]) >> 1) + blockvals[ 15 ] ];
        blockvals += 16;
        index     += row_size;
        rindex1   += row_size;
        bindex1   += row_size;
      }
    }
    else
    {
      for( rr = 0 ; rr < 4 ; rr++ )
      {
        index[ 0 ] = (int)(rindex1[ 0 ] + bindex1[ 0 ]) >> 1;
        index[ 1 ] = (int)(rindex1[ 1 ] + bindex1[ 1 ]) >> 1;
        index[ 2 ] = (int)(rindex1[ 2 ] + bindex1[ 2 ]) >> 1;
        index[ 3 ] = (int)(rindex1[ 3 ] + bindex1[ 3 ]) >> 1;
        index[ 4 ] = (int)(rindex1[ 4 ] + bindex1[ 4 ]) >> 1;
        index[ 5 ] = (int)(rindex1[ 5 ] + bindex1[ 5 ]) >> 1;
        index[ 6 ] = (int)(rindex1[ 6 ] + bindex1[ 6 ]) >> 1;
        index[ 7 ] = (int)(rindex1[ 7 ] + bindex1[ 7 ]) >> 1;
        index   += row_size;
        rindex1 += row_size;
        bindex1 += row_size;

        index[ 0 ] = (int)(rindex1[ 0 ] + bindex1[ 0 ]) >> 1;
        index[ 1 ] = (int)(rindex1[ 1 ] + bindex1[ 1 ]) >> 1;
        index[ 2 ] = (int)(rindex1[ 2 ] + bindex1[ 2 ]) >> 1;
        index[ 3 ] = (int)(rindex1[ 3 ] + bindex1[ 3 ]) >> 1;
        index[ 4 ] = (int)(rindex1[ 4 ] + bindex1[ 4 ]) >> 1;
        index[ 5 ] = (int)(rindex1[ 5 ] + bindex1[ 5 ]) >> 1;
        index[ 6 ] = (int)(rindex1[ 6 ] + bindex1[ 6 ]) >> 1;
        index[ 7 ] = (int)(rindex1[ 7 ] + bindex1[ 7 ]) >> 1;
        index   += row_size;
        rindex1 += row_size;
        bindex1 += row_size;
      }
    }
  }
}


/*
 *--------------------------------------------------------------
 *
 * ProcessSkippedPFrameMBlocks --
 *
 *    Processes skipped macroblocks in P frames.
 *
 * Results:
 *    Calculates pixel values for luminance, Cr, and Cb planes
 *      in current pict image for skipped macroblocks.
 *
 * Side effects:
 *    Pixel values in pict image changed.
 *
 *--------------------------------------------------------------
 */


static
void ProcessSkippedPFrameMBlocks( struct MPEGVideoInstData *mvid, VidStream *vid_stream )
{
  int row_size, half_row, mb_row, mb_col, row, col, rr;
  int addr, row_incr, half_row_incr, crow, ccol;
  int *dest, *src, *dest1, *src1;

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

  /* Calculate row sizes for luminance and Cr/Cb macroblock areas. */
  row_size = vid_stream->mb_width << 4;
  half_row = (row_size >> 1);
  row_incr = row_size >> 2;
  half_row_incr = half_row >> 2;

  /* For each skipped macroblock, do... */
  for( addr = vid_stream -> mblock . past_mb_addr + 1 ; addr < vid_stream -> mblock . mb_address ; addr++ )
  {
    /* Calculate macroblock row and col. */
    mb_row = addr / vid_stream -> mb_width;
    mb_col = addr % vid_stream -> mb_width;

    /* Calculate upper left pixel row,col for luminance plane. */
    row = mb_row << 4;
    col = mb_col << 4;

    /* For each row in macroblock luminance plane... */
    dest = (int *)(vid_stream -> current -> luminance + (row * row_size) + col);
    src  = (int *)(vid_stream -> future  -> luminance + (row * row_size) + col);

    for( rr = 0 ; rr < 8 ; rr++ )
    {
      /* Copy pixel values from last I or P picture. */
      dest[ 0 ] = src[ 0 ];
      dest[ 1 ] = src[ 1 ];
      dest[ 2 ] = src[ 2 ];
      dest[ 3 ] = src[ 3 ];
      dest += row_incr;
      src  += row_incr;

      dest[ 0 ] = src[ 0 ];
      dest[ 1 ] = src[ 1 ];
      dest[ 2 ] = src[ 2 ];
      dest[ 3 ] = src[ 3 ];
      dest += row_incr;
      src  += row_incr;
    }

    /* Divide row, col to get upper left pixel of macroblock in Cr and Cb planes. */
    crow = row >> 1;
    ccol = col >> 1;

    /* For each row in Cr, and Cb planes... */
    dest  = (int *)(vid_stream -> current -> Cr + (crow * half_row) + ccol);
    src   = (int *)(vid_stream -> future  -> Cr + (crow * half_row) + ccol);
    dest1 = (int *)(vid_stream -> current -> Cb + (crow * half_row) + ccol);
    src1  = (int *)(vid_stream -> future  -> Cb + (crow * half_row) + ccol);

    for( rr = 0 ; rr < 4 ; rr++ )
    {
      /* Copy pixel values from last I or P picture. */
      dest[ 0 ] = src[ 0 ];
      dest[ 1 ] = src[ 1 ];

      dest1[ 0] = src1[ 0 ];
      dest1[ 1] = src1[ 1 ];

      dest  += half_row_incr;
      src   += half_row_incr;
      dest1 += half_row_incr;
      src1  += half_row_incr;

      dest[ 0 ] = src[ 0 ];
      dest[ 1 ] = src[ 1 ];

      dest1[ 0 ] = src1[ 0 ];
      dest1[ 1 ] = src1[ 1 ];

      dest  += half_row_incr;
      src   += half_row_incr;
      dest1 += half_row_incr;
      src1  += half_row_incr;
    }
  }

  vid_stream -> mblock . recon_right_for_prev = 0;
  vid_stream -> mblock . recon_down_for_prev  = 0;
}


/*
 *--------------------------------------------------------------
 *
 * ProcessSkippedBFrameMBlocks --
 *
 *    Processes skipped macroblocks in B frames.
 *
 * Results:
 *    Calculates pixel values for luminance, Cr, and Cb planes
 *      in current pict image for skipped macroblocks.
 *
 * Side effects:
 *    Pixel values in pict image changed.
 *
 *--------------------------------------------------------------
 */


static
void ProcessSkippedBFrameMBlocks( struct MPEGVideoInstData *mvid, VidStream *vid_stream )
{
  int   row_size            = 0,
        half_row            = 0,
        mb_row              = 0,
        mb_col              = 0,
        row                 = 0,
        col                 = 0,
        rr                  = 0;
  int   right_half_for      = 0,
        down_half_for       = 0,
        c_right_half_for    = 0,
        c_down_half_for     = 0;
  int   right_half_back     = 0,
        down_half_back      = 0,
        c_right_half_back   = 0,
        c_down_half_back    = 0;
  int   addr                = 0,
        right_for           = 0,
        down_for            = 0;
  int   recon_right_for     = 0,
        recon_down_for      = 0;
  int   recon_right_back    = 0,
        recon_down_back     = 0;
  int   right_back          = 0,
        down_back           = 0;
  int   c_right_for         = 0,
        c_down_for          = 0;
  int   c_right_back        = 0,
        c_down_back         = 0;
  UBYTE forw_lum[ 256 ];
  UBYTE forw_cr[ 64 ],
        forw_cb[ 64 ];
  UBYTE back_lum[ 256 ],
        back_cr[ 64 ],
        back_cb[ 64 ];
  int   row_incr            = 0,
        half_row_incr       = 0;
  int   ccol                = 0,
        crow                = 0;

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

  /* Calculate row sizes for luminance and Cr/Cb macroblock areas. */
  row_size      = vid_stream -> mb_width << 4;
  half_row      = (row_size >> 1);
  row_incr      = row_size >> 2;
  half_row_incr = half_row >> 2;

  /* Establish motion vector codes based on full pixel flag. */
  if( vid_stream -> picture . full_pel_forw_vector )
  {
    recon_right_for = vid_stream -> mblock . recon_right_for_prev << 1;
    recon_down_for  = vid_stream -> mblock . recon_down_for_prev  << 1;
  }
  else
  {
    recon_right_for = vid_stream -> mblock . recon_right_for_prev;
    recon_down_for  = vid_stream -> mblock . recon_down_for_prev;
  }

  if( vid_stream -> picture . full_pel_back_vector )
  {
    recon_right_back = vid_stream -> mblock . recon_right_back_prev << 1;
    recon_down_back  = vid_stream -> mblock . recon_down_back_prev  << 1;
  }
  else
  {
    recon_right_back = vid_stream -> mblock . recon_right_back_prev;
    recon_down_back  = vid_stream -> mblock . recon_down_back_prev;
  }

  /* Calculate motion vectors. */
  if( vid_stream -> mblock . bpict_past_forw )
  {
    right_for       = recon_right_for >> 1;
    down_for        = recon_down_for >> 1;
    right_half_for  = recon_right_for & 0x1;
    down_half_for   = recon_down_for & 0x1;

    recon_right_for /= 2;
    recon_down_for  /= 2;
    c_right_for      = recon_right_for >> 1;
    c_down_for       = recon_down_for >> 1;
    c_right_half_for = recon_right_for & 0x1;
    c_down_half_for  = recon_down_for & 0x1;
  }

  if( vid_stream -> mblock . bpict_past_back )
  {
    right_back        = recon_right_back >> 1;
    down_back         = recon_down_back >> 1;
    right_half_back   = recon_right_back & 0x1;
    down_half_back    = recon_down_back & 0x1;

    recon_right_back /= 2;
    recon_down_back  /= 2;
    c_right_back      = recon_right_back >> 1;
    c_down_back       = recon_down_back >> 1;
    c_right_half_back = recon_right_back & 0x1;
    c_down_half_back  = recon_down_back & 0x1;
  }

  /* For each skipped macroblock, do... */
  for( addr = vid_stream -> mblock . past_mb_addr + 1; addr < vid_stream -> mblock . mb_address ; addr++ )
  {
    /* Calculate macroblock row and col. */
    mb_row = addr / vid_stream -> mb_width;
    mb_col = addr % vid_stream -> mb_width;

    /* Calculate upper left pixel row,col for luminance plane. */
    row  = mb_row << 4;
    col  = mb_col << 4;
    crow = row / 2;
    ccol = col / 2;

    /* If forward predicted, calculate prediction values. */
    if( vid_stream -> mblock . bpict_past_forw )
    {
      ReconSkippedBlock( mvid, vid_stream -> past -> luminance, forw_lum, row, col,   row_size, right_for,   down_for,   right_half_for,   down_half_for,   16 );
      ReconSkippedBlock( mvid, vid_stream -> past -> Cr,        forw_cr,  crow, ccol, half_row, c_right_for, c_down_for, c_right_half_for, c_down_half_for,  8 );
      ReconSkippedBlock( mvid, vid_stream -> past -> Cb,        forw_cb,  crow, ccol, half_row, c_right_for, c_down_for, c_right_half_for, c_down_half_for,  8 );
    }

    /* If back predicted, calculate prediction values. */
    if( vid_stream -> mblock . bpict_past_back )
    {
      ReconSkippedBlock( mvid, vid_stream -> future -> luminance,  back_lum, row,  col,  row_size, right_back,   down_back,   right_half_back,   down_half_back,   16 );
      ReconSkippedBlock( mvid, vid_stream -> future -> Cr,         back_cr,  crow, ccol, half_row, c_right_back, c_down_back, c_right_half_back, c_down_half_back,  8 );
      ReconSkippedBlock( mvid, vid_stream -> future -> Cb,         back_cb,  crow, ccol, half_row, c_right_back, c_down_back, c_right_half_back, c_down_half_back,  8 );
    }

    if( vid_stream -> mblock . bpict_past_forw && !vid_stream -> mblock . bpict_past_back )
    {
      int *dest, *dest1;
      int *src,  *src1;

      dest = (int *)(vid_stream -> current -> luminance + (row * row_size) + col);
      src = (int *)forw_lum;

      for( rr = 0 ; rr < 16 ; rr++ )
      {
        /* memcpy( dest, forw_lum + (rr << 4), 16 );  */
        dest[ 0 ] = src[ 0 ];
        dest[ 1 ] = src[ 1 ];
        dest[ 2 ] = src[ 2 ];
        dest[ 3 ] = src[ 3 ];
        dest += row_incr;
        src  += 4;
      }

      dest  = (int *)(vid_stream -> current -> Cr + (crow * half_row) + ccol);
      dest1 = (int *)(vid_stream -> current -> Cb + (crow * half_row) + ccol);
      src   = (int *)forw_cr;
      src1  = (int *)forw_cb;

      for( rr = 0 ; rr < 8 ; rr++ )
      {
        /* memcpy( dest, forw_cr + (rr << 3), 8 ); memcpy( dest1, forw_cb + (rr << 3), 8 ); */
        dest[ 0 ] = src[ 0 ];
        dest[ 1 ] = src[ 1 ];

        dest1[ 0 ] = src1[ 0 ];
        dest1[ 1 ] = src1[ 1 ];

        dest  += half_row_incr;
        dest1 += half_row_incr;
        src   += 2;
        src1  += 2;
      }
    }
    else
      if( vid_stream -> mblock . bpict_past_back && !vid_stream -> mblock . bpict_past_forw )
      {
      int *src,  *src1;
      int *dest, *dest1;

      dest = (int *)(vid_stream -> current -> luminance + (row * row_size) + col);
      src = (int *)back_lum;

      for( rr = 0 ; rr < 16 ; rr++ )
      {
        dest[ 0 ] = src[ 0 ];
        dest[ 1 ] = src[ 1 ];
        dest[ 2 ] = src[ 2 ];
        dest[ 3 ] = src[ 3 ];

        dest += row_incr;
        src += 4;
      }

      dest  = (int *)(vid_stream -> current -> Cr + (crow * half_row) + ccol);
      dest1 = (int *)(vid_stream -> current -> Cb + (crow * half_row) + ccol);
      src  = (int *)back_cr;
      src1 = (int *)back_cb;

      for( rr = 0 ; rr < 8 ; rr++ )
      {
        /* memcpy( dest, back_cr + (rr << 3), 8 ); memcpy( dest1, back_cb + (rr << 3), 8); */
        dest[ 0 ]  = src[ 0 ];
        dest[ 1 ]  = src[ 1 ];
        dest1[ 0 ] = src1[ 0 ];
        dest1[ 1 ] = src1[ 1 ];

        dest  += half_row_incr;
        dest1 += half_row_incr;
        src   += 2;
        src1  += 2;
      }
    }
    else
    {
      UBYTE *src1, *src2, *src1a, *src2a;
      UBYTE *dest, *dest1;

      dest = vid_stream -> current -> luminance + (row * row_size) + col;
      src1 = forw_lum;
      src2 = back_lum;

      for( rr = 0 ; rr < 16 ; rr++ )
      {
        dest[  0 ] = (int)(src1[  0 ] + src2[ 0  ]) >> 1;
        dest[  1 ] = (int)(src1[  1 ] + src2[ 1  ]) >> 1;
        dest[  2 ] = (int)(src1[  2 ] + src2[ 2  ]) >> 1;
        dest[  3 ] = (int)(src1[  3 ] + src2[ 3  ]) >> 1;
        dest[  4 ] = (int)(src1[  4 ] + src2[ 4  ]) >> 1;
        dest[  5 ] = (int)(src1[  5 ] + src2[ 5  ]) >> 1;
        dest[  6 ] = (int)(src1[  6 ] + src2[ 6  ]) >> 1;
        dest[  7 ] = (int)(src1[  7 ] + src2[ 7  ]) >> 1;
        dest[  8 ] = (int)(src1[  8 ] + src2[ 8  ]) >> 1;
        dest[  9 ] = (int)(src1[  9 ] + src2[ 9  ]) >> 1;
        dest[ 10 ] = (int)(src1[ 10 ] + src2[ 10 ]) >> 1;
        dest[ 11 ] = (int)(src1[ 11 ] + src2[ 11 ]) >> 1;
        dest[ 12 ] = (int)(src1[ 12 ] + src2[ 12 ]) >> 1;
        dest[ 13 ] = (int)(src1[ 13 ] + src2[ 13 ]) >> 1;
        dest[ 14 ] = (int)(src1[ 14 ] + src2[ 14 ]) >> 1;
        dest[ 15 ] = (int)(src1[ 15 ] + src2[ 15 ]) >> 1;

        dest += row_size;
        src1 += 16;
        src2 += 16;
      }

      dest  = vid_stream -> current -> Cr + (crow * half_row) + ccol;
      dest1 = vid_stream -> current -> Cb + (crow * half_row) + ccol;
      src1  = forw_cr;
      src2  = back_cr;
      src1a = forw_cb;
      src2a = back_cb;

      for( rr = 0 ; rr < 8 ; rr++ )
      {
        dest[ 0 ] = (int)(src1[ 0 ] + src2[ 0 ]) >> 1;
        dest[ 1 ] = (int)(src1[ 1 ] + src2[ 1 ]) >> 1;
        dest[ 2 ] = (int)(src1[ 2 ] + src2[ 2 ]) >> 1;
        dest[ 3 ] = (int)(src1[ 3 ] + src2[ 3 ]) >> 1;
        dest[ 4 ] = (int)(src1[ 4 ] + src2[ 4 ]) >> 1;
        dest[ 5 ] = (int)(src1[ 5 ] + src2[ 5 ]) >> 1;
        dest[ 6 ] = (int)(src1[ 6 ] + src2[ 6 ]) >> 1;
        dest[ 7 ] = (int)(src1[ 7 ] + src2[ 7 ]) >> 1;

        dest += half_row;
        src1 += 8;
        src2 += 8;

        dest1[ 0 ] = (int)(src1a[ 0 ] + src2a[ 0 ]) >> 1;
        dest1[ 1 ] = (int)(src1a[ 1 ] + src2a[ 1 ]) >> 1;
        dest1[ 2 ] = (int)(src1a[ 2 ] + src2a[ 2 ]) >> 1;
        dest1[ 3 ] = (int)(src1a[ 3 ] + src2a[ 3 ]) >> 1;
        dest1[ 4 ] = (int)(src1a[ 4 ] + src2a[ 4 ]) >> 1;
        dest1[ 5 ] = (int)(src1a[ 5 ] + src2a[ 5 ]) >> 1;
        dest1[ 6 ] = (int)(src1a[ 6 ] + src2a[ 6 ]) >> 1;
        dest1[ 7 ] = (int)(src1a[ 7 ] + src2a[ 7 ]) >> 1;

        dest1 += half_row;
        src1a += 8;
        src2a += 8;
      }
    }
  }
}


/*
 *--------------------------------------------------------------
 *
 * ReconSkippedBlock --
 *
 *    Reconstructs predictive block for skipped macroblocks
 *      in B Frames.
 *
 * Results:
 *    No return values.
 *
 * Side effects:
 *    None.
 *
 *--------------------------------------------------------------
 */


static
void ReconSkippedBlock( struct MPEGVideoInstData *mvid, UBYTE *source, UBYTE *dest, int row, int col, int row_size, int right, int down, int right_half, int down_half, int width )
{
  int    rr;
  UBYTE *source2;

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

  source += ((row + down) * row_size) + col + right;

  if( width == 16 )
  {
    if( (!right_half) && (!down_half) )
    {
      if( right & 0x1 )
      {
        /* No alignment, use bye copy */
        for( rr = 0 ; rr < 16 ; rr++ )
        {
          dest[  0 ] = source[  0 ];
          dest[  1 ] = source[  1 ];
          dest[  2 ] = source[  2 ];
          dest[  3 ] = source[  3 ];
          dest[  4 ] = source[  4 ];
          dest[  5 ] = source[  5 ];
          dest[  6 ] = source[  6 ];
          dest[  7 ] = source[  7 ];
          dest[  8 ] = source[  8 ];
          dest[  9 ] = source[  9 ];
          dest[ 10 ] = source[ 10 ];
          dest[ 11 ] = source[ 11 ];
          dest[ 12 ] = source[ 12 ];
          dest[ 13 ] = source[ 13 ];
          dest[ 14 ] = source[ 14 ];
          dest[ 15 ] = source[ 15 ];

          dest   += 16;
          source += row_size;
        }
      }
      else
      {
        if( right & 0x2 )
        {
          /* Half-word bit aligned, use 16 bit copy */
          WORD *src = (WORD *)source;
          WORD *d   = (WORD *)dest;
          row_size >>= 1;

          for( rr = 0 ; rr < 16 ; rr++ )
          {
            d[ 0 ] = src[ 0 ];
            d[ 1 ] = src[ 1 ];
            d[ 2 ] = src[ 2 ];
            d[ 3 ] = src[ 3 ];
            d[ 4 ] = src[ 4 ];
            d[ 5 ] = src[ 5 ];
            d[ 6 ] = src[ 6 ];
            d[ 7 ] = src[ 7 ];

            d   += 8;
            src += row_size;
          }
        }
        else
        {
          /* Word aligned, use 32 bit copy */
          int *src = (int *)source;
          int *d   = (int *)dest;
          row_size >>= 2;

          for( rr = 0 ; rr < 16 ; rr++ )
          {
            d[ 0 ] = src[ 0 ];
            d[ 1 ] = src[ 1 ];
            d[ 2 ] = src[ 2 ];
            d[ 3 ] = src[ 3 ];

            d   += 4;
            src += row_size;
          }
        }
      }
    }
    else
    {
      source2 = source + right_half + (row_size * down_half);

      for( rr = 0 ; rr < width ; rr++ )
      {
        dest[  0 ] = (int)(source[  0 ] + source2[  0 ]) >> 1;
        dest[  1 ] = (int)(source[  1 ] + source2[  1 ]) >> 1;
        dest[  2 ] = (int)(source[  2 ] + source2[  2 ]) >> 1;
        dest[  3 ] = (int)(source[  3 ] + source2[  3 ]) >> 1;
        dest[  4 ] = (int)(source[  4 ] + source2[  4 ]) >> 1;
        dest[  5 ] = (int)(source[  5 ] + source2[  5 ]) >> 1;
        dest[  6 ] = (int)(source[  6 ] + source2[  6 ]) >> 1;
        dest[  7 ] = (int)(source[  7 ] + source2[  7 ]) >> 1;
        dest[  8 ] = (int)(source[  8 ] + source2[  8 ]) >> 1;
        dest[  9 ] = (int)(source[  9 ] + source2[  9 ]) >> 1;
        dest[ 10 ] = (int)(source[ 10 ] + source2[ 10 ]) >> 1;
        dest[ 11 ] = (int)(source[ 11 ] + source2[ 11 ]) >> 1;
        dest[ 12 ] = (int)(source[ 12 ] + source2[ 12 ]) >> 1;
        dest[ 13 ] = (int)(source[ 13 ] + source2[ 13 ]) >> 1;
        dest[ 14 ] = (int)(source[ 14 ] + source2[ 14 ]) >> 1;
        dest[ 15 ] = (int)(source[ 15 ] + source2[ 15 ]) >> 1;

        dest    += width;
        source  += row_size;
        source2 += row_size;
      }
    }
  }
  else
  {
    /* must be (width == 8) */
    assert( width == 8 );

    if( (!right_half) && (!down_half) )
    {
      if( right & 0x1 )
      {
        for( rr = 0 ; rr < width ; rr++ )
        {
          dest[ 0 ] = source[ 0 ];
          dest[ 1 ] = source[ 1 ];
          dest[ 2 ] = source[ 2 ];
          dest[ 3 ] = source[ 3 ];
          dest[ 4 ] = source[ 4 ];
          dest[ 5 ] = source[ 5 ];
          dest[ 6 ] = source[ 6 ];
          dest[ 7 ] = source[ 7 ];

          dest   += 8;
          source += row_size;
        }
      }
      else
      {
        if( right & 0x02 )
        {
          WORD *d   = (WORD *)dest;
          WORD *src = (WORD *)source;
          row_size >>= 1;

          for( rr = 0 ; rr < width ; rr++ )
          {
            d[ 0 ] = src[ 0 ];
            d[ 1 ] = src[ 1 ];
            d[ 2 ] = src[ 2 ];
            d[ 3 ] = src[ 3 ];

            d   += 4;
            src += row_size;
          }
        }
        else
        {
          int *d   = (int *)dest;
          int *src = (int *)source;

          row_size >>= 2;
          for( rr = 0 ; rr < width ; rr++ )
          {
            d[ 0 ] = src[ 0 ];
            d[ 1 ] = src[ 1 ];

            d   += 2;
            src += row_size;
          }
        }
      }
    }
    else
    {
      source2 = source + right_half + (row_size * down_half);

      for( rr = 0 ; rr < width ; rr++ )
      {
        dest[ 0 ] = (int)(source[ 0 ] + source2[ 0 ]) >> 1;
        dest[ 1 ] = (int)(source[ 1 ] + source2[ 1 ]) >> 1;
        dest[ 2 ] = (int)(source[ 2 ] + source2[ 2 ]) >> 1;
        dest[ 3 ] = (int)(source[ 3 ] + source2[ 3 ]) >> 1;
        dest[ 4 ] = (int)(source[ 4 ] + source2[ 4 ]) >> 1;
        dest[ 5 ] = (int)(source[ 5 ] + source2[ 5 ]) >> 1;
        dest[ 6 ] = (int)(source[ 6 ] + source2[ 6 ]) >> 1;
        dest[ 7 ] = (int)(source[ 7 ] + source2[ 7 ]) >> 1;

        dest    += width;
        source  += row_size;
        source2 += row_size;
      }
    }
  }
}


/*
 *--------------------------------------------------------------
 *
 * DoPictureDisplay --
 *
 *    Converts image from Lum, Cr, Cb to colormap space. Puts
 *      image in lum plane. Updates past and future frame
 *      pointers. Dithers image. Sends to display mechanism.
 *
 * Results:
 *    Pict image structure locked if displaying or if frame
 *      is needed as past or future reference.
 *
 * Side effects:
 *    Lum plane pummelled.
 *
 *--------------------------------------------------------------
 */


static
void DoPictureDisplay( struct MPEGVideoInstData *mvid, VidStream *vid_stream )
{
    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    /* Convert to colormap space and dither. */
    DoDitherImage( mvid,
                   (vid_stream -> current -> luminance),
                   (vid_stream -> current -> Cr),
                   (vid_stream -> current -> Cb),
                   (vid_stream -> current -> display),
                   ((vid_stream -> mb_height) * 16),
                   ((vid_stream -> mb_width)  * 16) );

    /* Update past and future references if needed. */
    if( (vid_stream -> picture . code_type == I_TYPE) || (vid_stream -> picture . code_type == P_TYPE) )
    {
      if( vid_stream -> future == NULL )
      {
        vid_stream -> future            = vid_stream -> current;
        vid_stream -> future -> locked |= FUTURE_LOCK;
      }
      else
      {
        if( vid_stream -> past )
        {
          vid_stream -> past -> locked &= ~PAST_LOCK;
        }

        vid_stream -> past              = vid_stream -> future;
        vid_stream -> past -> locked   &= ~FUTURE_LOCK;
        vid_stream -> past -> locked   |= PAST_LOCK;
        vid_stream -> future            = vid_stream -> current;
        vid_stream -> future -> locked |= FUTURE_LOCK;
        vid_stream -> current           = vid_stream -> past;

        ExecuteDisplay( mvid, vid_stream );
      }
    }
    else
    {
      ExecuteDisplay( mvid, vid_stream );
    }
}


ULONG GetFrameRate( struct MPEGVideoInstData *mvid, VidStream *stream )
{
    /* Video rates table (taken from mpeg2enc and mpeg_play 2.3) */
    /* Cheat on Vid rates, round to 30, and use 30 if illegal value
     * Except for 9, where Xing means 15, and given their popularity, we'll
     * be nice and do it
     */
    const
    double ratetab[ 16 ] =
    {
      30U,  /* reserved */
      24000.0/1001.0,
      24.0,
      25.0,
      30000.0/1001.0,
      30.0,
      50.0,
      60000.0/1001.0,
      60.0,
      15.0, /* reserved */
      30.0, /* reserved */
      30.0, /* reserved */
      30.0, /* reserved */
      30.0, /* reserved */
      30.0, /* reserved */
      30.0  /* reserved */
    };

    UBYTE  ratecode = stream -> picture_rate;
    double fps;
    ULONG  ticksperframe;

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    if( ratecode < 16U )
    {
      fps = ratetab[ ratecode ];
    }
    else
    {
      fps = 30.0;
    }

    ticksperframe = (ULONG)((double)TICK_FREQ / fps);

    return( ticksperframe );
}


