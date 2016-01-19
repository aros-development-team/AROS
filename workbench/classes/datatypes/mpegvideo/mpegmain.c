
/*
**
**  $VER: mpegmain.c 1.11 (7.11.97)
**  mpegvideo.datatype 1.11
**
**  mpeg video main body
**
**  Written 1996/1997 by Roland 'Gizzy' Mainz
**
*/

#ifndef DEBUG
#   define DEBUG 0
#endif
#include <aros/debug.h>

/* amiga includes */
#include <exec/types.h>
#include <exec/lists.h>

#if !defined(__AROS__) && !defined(__MORPHOS__)
/* amiga prototypes */
#include <clib/dos_protos.h>
#include <clib/exec_protos.h>

/* amiga pragmas */
#include <pragmas/dos_pragmas.h>
#include <pragmas/exec_pragmas.h>
#else
#include <proto/exec.h>
#include <proto/dos.h>
#endif

/* ansi includes */
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

/* project includes */
#include "mpegmyassert.h"
#include "mpegvideo.h"
#include "mpegproto.h"
#include "mpegutil.h"

/* local prototypes */
static void ChunkyScale( UBYTE *, ULONG, ULONG, UBYTE *, ULONG, ULONG );


/*
 *--------------------------------------------------------------
 *
 * get_more_data --
 *
 *    Called by correct_underflow in bit parsing utilities to
 *      read in more data.
 *
 * Results:
 *    Input buffer updated, buffer length updated.
 *      Returns 1 if data read, 0 if EOF, -1 if error.
 *
 * Side effects:
 *      None.
 *
 *--------------------------------------------------------------
 */


int get_more_data( struct MPEGVideoInstData *mvid, unsigned int *buf_start, int max_length, int *length_ptr, unsigned int **buf_ptr )
{
    long   length,
           num_read,
           request;
    UBYTE *buffer,
          *mark;

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    if( EOF_flag )
    {
        D(bug("[mpegvideo.datatype] %s: reached end of file\n", __func__));
      return( 0 );
    }

    length = *length_ptr;
    buffer = (UBYTE *)*buf_ptr;

    if( length < 0 )
    {
#ifndef NDEBUG
      error_printf( mvid, "get_more_data: bad length %ld\n", length );
#endif /* !NDEBUG */

      myexit( mvid, RETURN_ERROR, ERROR_SEEK_ERROR );
    }

    if( length > 0 )
    {
      memcpy( buf_start, buffer, (size_t)(length * 4) );
      mark = ((UBYTE *)(buf_start + length));
    }
    else
    {
      mark   = (UBYTE *)buf_start;
      length = 0;
    }

    request = (max_length - length) * 4;

    /* store read mark here */
    mvid -> mvid_ReadMarkPos = Seek( (mvid -> input), 0L, OFFSET_CURRENT );
    mvid -> mvid_ReadMark    = mark;

    D(bug("[mpegvideo.datatype] %s: ReadMarkPos = %d\n", __func__, mvid->mvid_ReadMarkPos));

    num_read = Read( (mvid -> input), mark, request );

    if( num_read != request )
    {
      if( IoErr() )
      {
        error_printf( mvid, "read error %ld\n", IoErr() );

        myexit( mvid, RETURN_ERROR, IoErr() );
      }
      else
      {
        verbose_printf( mvid, "EOF found\n" );
      }
    }

    D(bug("[mpegvideo.datatype] %s: updating requestor...\n", __func__));

    /* Update progress gauge... */
    mvid -> mvid_PR . pr_Curr += num_read;
    UpdateProgressRequester( mvid );

    /* Paulo Villegas - 26/1/1993: Correction for 4-byte alignment */
    {
      long   num_read_rounded;
      UBYTE *index;

      num_read_rounded = 4 * (num_read / 4);

      /* this can happen only if num_read < request; i.e. end of file reached */
      if( num_read_rounded < num_read )
      {
        num_read_rounded = 4 * (num_read / 4 + 1);

        /* fill in with zeros */
        for( index = mark + num_read ; index < (mark + num_read_rounded) ; *(index++) = 0 );

        /* advance to the next 4-byte boundary */
        num_read = num_read_rounded;
      }
    }

    if( num_read < 0 )
    {
      return( -1 );
    }
    else
    {
      if( num_read == 0 )
      {
        *buf_ptr    = buf_start;
        *length_ptr = length + 2UL;  /* test test: New for V1.10: Add the two bytes downstairs to the length */

        /* Make 32 bits after end equal to 0 and 32 bits after that equal to seq end code
         * in order to prevent messy data from infinite recursion.
         */
        *(buf_start + length)     = 0x0UL;
        *(buf_start + length + 1) = SEQ_END_CODE;

        EOF_flag = TRUE;
        
        return( 0 );
      }
    }

    num_read /= 4UL;

    *buf_ptr    = buf_start;
    *length_ptr = length + num_read;

    return( 1 );
}


void loadvideo( struct MPEGVideoInstData *mvid )
{
    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    /* Get file size, update progress gauge */
    (void)Seek( (mvid -> input), 0L, OFFSET_END );
    mvid -> mvid_PR . pr_Max = Seek( (mvid -> input), 0L, OFFSET_BEGINNING );
    UpdateProgressRequester( mvid );

    /* Create new video stream context */
    mvid -> mvid_VidStream = NewVidStream( mvid, (int)(mvid -> mvid_BufLength) );
    D(bug("[mpegvideo.datatype] %s: VidStream context @ 0x%p\n", __func__, mvid -> mvid_VidStream));

    /* Get initial information about the stream */
    mpegVidRsrc( mvid, 0, (mvid -> mvid_VidStream) );

    video_width  = (ULONG)(curVidStream -> h_size);
    video_height = (ULONG)(curVidStream -> v_size);

    if( anim_width  == 0UL ) anim_width  = video_width;
    if( anim_height == 0UL ) anim_height = video_height;

    xtpf = GetFrameRate( mvid, (mvid -> mvid_VidStream) );

    while(TRUE)
    {
      if( (mvid -> mvid_LoadAll) || ((mvid -> mvid_KeyBitMap) == NULL) )
      {
        mpegVidRsrc( mvid, 0, (mvid -> mvid_VidStream) );
      }
      else
      {
        mpegVidRsrcScan( mvid, 0, (mvid -> mvid_VidStream) );
      }

      /* Maximum number of frames reached ? (If requested) */
      if( mvid -> mvid_MaxFrame )
      {
        if( totNumFrames > (mvid -> mvid_MaxFrame) )
        {
          break;
        }
      }

      /* Check for abort... */
      if( CheckSignal( SIGBREAKF_CTRL_D ) )
      {
        myexit( mvid, RETURN_WARN, ERROR_BREAK );
      }

      /* Check if we have enough memory... */
      if( mvid -> mvid_MinTotalMem )
      {
        if( (mvid -> mvid_MinTotalMem) > AvailMem( 0UL ) )
        {
          myexit( mvid, RETURN_WARN, ERROR_BREAK );
        }
      }
    }
}


/*
 *--------------------------------------------------------------
 *
 * DoDitherImage --
 *
 *    Called when image needs to be dithered. Selects correct
 *      dither routine based on info in ditherType.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    None.
 *
 *--------------------------------------------------------------
 */


void DoDitherImage( struct MPEGVideoInstData *mvid, UBYTE *l, UBYTE *Cr, UBYTE *Cb, UBYTE *disp, UWORD h, UWORD w )
{
    UBYTE *sl  = NULL,
          *sCr = NULL,
          *sCb = NULL;

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    /* Scale ? */
    if( (video_width != anim_width) || (video_height != anim_height) )
    {
      verbose_printf( mvid, "scaling %lu/%lu -> %lu/%lu\n", video_width, video_height, anim_width, anim_height );

      sl  = (UBYTE *)mymalloc( mvid, (anim_width  * anim_height)      );
      sCr = (UBYTE *)mymalloc( mvid, ((anim_width * anim_height) / 4) );
      sCb = (UBYTE *)mymalloc( mvid, ((anim_width * anim_height) / 4) );

      ChunkyScale( sl,  (ULONG)anim_width,       (ULONG)anim_height,       l,  (ULONG)w,       (ULONG)h       );
      ChunkyScale( sCr, (ULONG)(anim_width / 2), (ULONG)(anim_height / 2), Cr, (ULONG)(w / 2), (ULONG)(h / 2) );
      ChunkyScale( sCb, (ULONG)(anim_width / 2), (ULONG)(anim_height / 2), Cb, (ULONG)(w / 2), (ULONG)(h / 2) );

      l  = sl,
      Cr = sCr,
      Cb = sCb,
      h  = anim_height;
      w  = anim_width;
    }

#if 0
    if( mvid -> mvid_PalettePerFrame )
    {
      DiceColorDitherImage( mvid, l, Cr, Cb, disp, h, w );
    }
#endif

    switch( ditherType )
    {
      case FULL_COLOR_DITHER16: Color16DitherImage(   mvid, l, Cr, Cb, disp, h, w ); break;
      case FULL_COLOR_DITHER:   Color32DitherImage(   mvid, l, Cr, Cb, disp, h, w ); break;
#if 0
      case FAST_COLOR_DITHER:   FastColorDitherImage( mvid, l, Cr, Cb, disp, h, w ); break;
#endif
      case ORDERED_DITHER:      OrderedDitherImage(   mvid, l, Cr, Cb, disp, h, w ); break;
      case GRAY_DITHER:         GrayDitherImage(      mvid, l, Cr, Cb, disp, h, w ); break;
      case HAM_DITHER:          Color32DitherImage(   mvid, l, Cr, Cb, disp, h, w ); break;
    }

    /* free tmp scaling buffers */
    if( sl && sCr && sCb )
    {
      myfree( mvid, sl  );
      myfree( mvid, sCr );
      myfree( mvid, sCb );
    }
}


void myexit( struct MPEGVideoInstData *mvid, long retval, long retval2 )
{
    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    mvid -> mvid_retval  = retval;
    mvid -> mvid_retval2 = retval2;

    longjmp( exit_buf, 1 );
}


static
void ChunkyScale( UBYTE *dest, ULONG destwidth, ULONG destheight, UBYTE *source, ULONG srcwidth, ULONG srcheight )
{
    ULONG x,
          y;

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

    for( y = 0UL ; y < destheight ; y++ )
    {
      UBYTE *srcrow = source + (((y * srcheight) / destheight) * srcwidth);

      for( x = 0UL ; x < destwidth ; x++ )
      {
        *dest++ = *(srcrow + ((x * srcwidth) / destwidth));
      }
    }
}



