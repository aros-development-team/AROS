

#ifndef MPEGVIDEO_H
#define MPEGVIDEO_H 1

/*
**
**  $VER: mpegvideo.h 1.10 (30.10.97)
**  mpegvideo.datatype 1.10
**
**  header for the video decoder model
**
**  Written 1996/1997 by Roland 'Gizzy' Mainz
**
*/

/* amiga includes */
#include <exec/types.h>

/* MPEG Stream Constants */

/* Start codes. */
#define PICTURE_START_CODE       (0x00000100)
#define SLICE_MIN_START_CODE     (0x00000101)
#define SLICE_MAX_START_CODE     (0x000001af)
#define RESERVED_START_CODE_1    (0x000001b0)
#define RESERVED_START_CODE_2    (0x000001b1)
#define USER_START_CODE          (0x000001b2)
#define SEQ_START_CODE           (0x000001b3)
#define EXT_START_CODE           (0x000001b5)
#define RESERVED_START_CODE_3    (0x000001b6)
#define SEQ_END_CODE             (0x000001b7)
#define GOP_START_CODE           (0x000001b8)
#define ISO_11172_END_CODE       (0x000001b9)
#define PACK_START_CODE          (0x000001ba)
#define SYSTEM_HEADER_START_CODE (0x000001bb)
#define RESERVED_STREAM          (0x000001bc)
#define PRIVATE_STREAM_1         (0x000001bd)
#define PADDING_STREAM           (0x000001be)
#define PRIVATE_STREAM_2         (0x000001bf)
#define AUDIO_STREAM_0           (0x000001c0)
#define AUDIO_STREAM_31          (0x000001df)
#define VIDIO_STREAM_0           (0x000001e0)
#define VIDIO_STREAM_15          (0x000001ef)
#define RESERVED_DATA_STREAM_0   (0x000001f0)
#define RESERVED_DATA_STREAM_15  (0x000001ff)

/* Picture_coding_type */
#define I_TYPE  (1)
#define P_TYPE  (2)
#define B_TYPE  (3)
#define D_TYPE  (4)

/* Macros used with macroblock address decoding. */
#define MB_STUFFING  (34)
#define MB_ESCAPE    (35)

/* Special values for DCT Coefficients */
#define END_OF_BLOCK  (62)
#define ESCAPE        (61)

/* Define Parsing error codes. */
#define SKIP_PICTURE        (-10)
#define SKIP_TO_START_CODE   (-1)
#define PARSE_OK              (1)

/* Set ring buffer size. */
#define RING_BUF_SIZE (5)

/* Number of macroblocks to process in one call to mpegVidRsrc. */
#define MB_QUANTUM (120)

/* Lock flags for pict images. */
#define DISPLAY_LOCK (0x01)  /* n/a */
#define PAST_LOCK    (0x02)
#define FUTURE_LOCK  (0x04)

/* dither modes for ditherType var */
#define HAM_DITHER                (5)
#define GRAY_DITHER               (6)
#define FULL_COLOR_DITHER         (7)
#define FULL_COLOR_DITHER16       (8)
#if 0
#define FAST_COLOR_DITHER        (10)
#endif
#define ORDERED_DITHER            (9)

/* Temporary definition of time stamp structure. */
typedef int TimeStamp;

/* Structure with reconstructed pixel values. */
typedef struct pict_image
{
  UBYTE     *luminance;              /* Luminance plane.   */
  UBYTE     *Cr;                     /* Cr plane.          */
  UBYTE     *Cb;                     /* Cb plane.          */
  UBYTE     *display;                /* Display plane.     */
  UWORD      locked;                 /* Lock flag. (DISPLAY_LOCK, PAST_LOCK, FUTURE_LOCK) */
  TimeStamp  show_time;              /* Presentation time. */
} PictImage;


/* Group of pictures structure. */
typedef struct GoP
{
  BOOL          drop_flag;               /* Flag indicating dropped frame.                            */
  UBYTE         tc_hours;                /* Hour component of time code.   (5 bits)                   */
  UBYTE         tc_minutes;              /* Minute component of time code. (6 bits)                   */
  UBYTE         tc_seconds;              /* Second component of time code. (6 bits)                   */
  UBYTE         tc_pictures;             /* Picture counter of time code.  (6 bits)                   */
  BOOL          closed_gop;              /* Indicates no pred. vectors to previous group of pictures. */
  BOOL          broken_link;             /* B frame unable to be decoded.                             */
  UWORD         Pad0; /* LW-align */
  char         *ext_data;                /* Extension data.                                           */
  char         *user_data;               /* User data.                                                */
} GoP;


/* Picture structure. */
typedef struct pict
{
  unsigned int  temp_ref;                 /* Temporal reference.                                (10 bits) */
  UBYTE         code_type;                /* Frame type: P, B, I, D                              (3 bits) */
  UBYTE         Pad0;
  UWORD         vbv_delay;                /* Buffer delay.                                      (16 bits) */
  BOOL          full_pel_forw_vector;     /* Forw. vectors specified in full pixel values flag. */
  UWORD         Pad1;
  ULONG         forw_r_size;              /* Used for vector decoding.                          */
  LONG          forw_f;                   /* Used for vector decoding.                          */
  BOOL          full_pel_back_vector;     /* Back vectors specified in full pixel values flag.  */
  UWORD         Pad2;
  ULONG         back_r_size;              /* Used in decoding.                                  */
  LONG          back_f;                   /* Used in decoding.                                  */
  char         *extra_info;               /* Extra bit picture info.                            */
  char         *ext_data;                 /* Extension data.                                    */
  char         *user_data;                /* User data.                                         */
} Pict;


/* Slice structure. */
typedef struct slice
{
  UBYTE          vert_pos;    /* Vertical position of slice.          */
  UBYTE          quant_scale; /* Quantization scale.         (5 bits) */
  UWORD          Pad0;
  char          *extra_info;  /* Extra bit slice info.                */
} Slice;


/* Macroblock structure. */
typedef struct macroblock
{
  int           mb_address;               /* Macroblock address.              */
  int           past_mb_addr;             /* Previous mblock address.         */
  int           motion_h_forw_code;       /* Forw. horiz. motion vector code. */
  unsigned int  motion_h_forw_r;          /* Used in decoding vectors.        */
  int           motion_v_forw_code;       /* Forw. vert. motion vector code.  */
  unsigned int  motion_v_forw_r;          /* Used in decdoinge vectors.       */
  int           motion_h_back_code;       /* Back horiz. motion vector code.  */
  unsigned int  motion_h_back_r;          /* Used in decoding vectors.        */
  int           motion_v_back_code;       /* Back vert. motion vector code.   */
  unsigned int  motion_v_back_r;          /* Used in decoding vectors.        */
  unsigned int  cbp;                      /* Coded block pattern.             */
  BOOL          mb_intra;                 /* Intracoded mblock flag.          */
  BOOL          bpict_past_forw;          /* Past B frame forw. vector flag.  */
  BOOL          bpict_past_back;          /* Past B frame back vector flag.   */
  UWORD         Pad0;
  int           past_intra_addr;          /* Addr of last intracoded mblock.  */
  int           recon_right_for_prev;     /* Past right forw. vector.         */
  int           recon_down_for_prev;      /* Past down forw. vector.          */
  int           recon_right_back_prev;    /* Past right back vector.          */
  int           recon_down_back_prev;     /* Past down back vector.           */
} Macroblock;


/* Block structure. */
typedef struct block
{
  WORD dct_recon[ 8 ][ 8 ];         /* Reconstructed dct coeff matrix. */
  WORD dct_dc_y_past;               /* Past lum. dc dct coefficient.   */
  WORD dct_dc_cr_past;              /* Past cr dc dct coefficient.     */
  WORD dct_dc_cb_past;              /* Past cb dc dct coefficient.     */
} Block;


/* Video stream structure. */
typedef struct vid_stream
{
  UWORD         h_size;                            /* Horiz. size in pixels.                (12 bits) */
  UWORD         v_size;                            /* Vert. size in pixels.                 (12 bits) */
  UWORD         mb_height;                         /* Vert. size in mblocks.                          */
  UWORD         mb_width;                          /* Horiz. size in mblocks.                         */
  UBYTE         aspect_ratio;                      /* Code for aspect ratio.                 (4 bits) */
  UBYTE         picture_rate;                      /* Code for picture rate.                 (4 bits) */
  UWORD         Pad0;
  ULONG         bit_rate;                          /* Bit rate.                             (18 bits) */
  UWORD         vbv_buffer_size;                   /* Minimum buffer size.                  (10 bits) */
  BOOL          const_param_flag;                  /* Contrained parameter flag.                      */
  UBYTE         intra_quant_matrix[ 8 ][ 8 ];      /* Quantization matrix for intracoded frames.      */
  UBYTE         non_intra_quant_matrix[ 8 ][ 8 ];  /* Quanitization matrix for non intracoded frames. */
  char         *ext_data;                          /* Extension data.                                 */
  char         *user_data;                         /* User data.                                      */
  GoP           group;                             /* Current group of pict.                          */
  Pict          picture;                           /* Current picture.                                */
  Slice         slice;                             /* Current slice.                                  */
  Macroblock    mblock;                            /* Current macroblock.                             */
  Block         block;                             /* Current block.                                  */
  int           state;                             /* State of decoding.                              */
  int           bit_offset;                        /* Bit offset in stream.                           */
  unsigned int *buffer;                            /* Pointer to next byte in buffer.                 */
  int           buf_length;                        /* Length of remaining buffer.                     */
  unsigned int *buf_start;                         /* Pointer to buffer start.                        */
  int           max_buf_length;                    /* Max lenght of buffer.                           */
  PictImage    *past;                              /* Past predictive frame.                          */
  PictImage    *future;                            /* Future predictive frame.                        */
  PictImage    *current;                           /* Current frame.                                  */
  PictImage    *ring[ RING_BUF_SIZE ];             /* Ring buffer of frames.                          */
} VidStream;

/* Definition of Contant integer scale factor. */
#define CONST_BITS (13)

/* Misc DCT definitions */
#define DCTSIZE      (8)    /* The basic DCT block is 8x8 samples */
#define DCTSIZE2    (64)    /* DCTSIZE squared; # of elements in a block */

typedef WORD   DCTELEM;
typedef DCTELEM DCTBLOCK[ DCTSIZE2 ];

#endif /* !MPEGVIDEO_H */

