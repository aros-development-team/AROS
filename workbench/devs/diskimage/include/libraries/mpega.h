/*------------------------------------------------------------------------------

    File    :   MPEGA.h

    Author  :   Stéphane TAVENARD

    $VER:   MPEGA.h  2.0  (21/06/1998)

    (C) Copyright 1997-1998 Stéphane TAVENARD
        All Rights Reserved

    #Rev|   Date   |                      Comment
    ----|----------|--------------------------------------------------------
    0   |25/10/1997| Initial revision                                     ST
    1   |21/06/1998| Added MPEGA_scale                                    ST

    ------------------------------------------------------------------------

    MPEGA decoder library definitions

------------------------------------------------------------------------------*/

#ifndef LIBRARIES_MPEGA_H
#define LIBRARIES_MPEGA_H

#define MPEGA_VERSION 2 /* #1 */

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef UTILITY_HOOKS_H
#include <utility/hooks.h>
#endif

/* Controls for decoding */

/* Qualities */
#define MPEGA_QUALITY_LOW    0
#define MPEGA_QUALITY_MEDIUM 1
#define MPEGA_QUALITY_HIGH   2

/*
   Bitstream Hook function is called like (SAS/C syntax):


   ULONG __saveds __asm HookFunc( register __a0 struct Hook  *hook,
                                  register __a2 APTR          handle,
                                  register __a1 MPEGA_ACCESS *access );

   MPEGA_ACCESS struct specify bitstream access function & parameters

   access->func == MPEGA_BSFUNC_OPEN
      open the bitstream
      access->data.open.buffer_size is the i/o block size your read function can use
      access->data.open.stream_size is the total size of the current stream
                                    (in bytes, set it to 0 if unknown)
      return your file handle (or NULL if failed)
   access->func == MPEGA_BSFUNC_CLOSE
      close the bitstream
      return 0 if ok
   access->func == MPEGA_BSFUNC_READ
      read bytes from bitstream.
      access->data.read.buffer is the destination buffer.
      access->data.read.num_bytes is the number of bytes requested for read.
      return # of bytes read or 0 if EOF.
   access->func == MPEGA_BSFUNC_SEEK
      seek into the bitstream
      access->data.seek.abs_byte_seek_pos is the absolute byte position to reach.
      return 0 if ok
*/

#define MPEGA_BSFUNC_OPEN  0
#define MPEGA_BSFUNC_CLOSE 1
#define MPEGA_BSFUNC_READ  2
#define MPEGA_BSFUNC_SEEK  3

typedef struct {

   LONG  func;           /* MPEGA_BSFUNC_xxx */
   union {
      struct {
         char *stream_name; /* in */
         LONG buffer_size;  /* in */
         LONG stream_size;  /* out */
      } open;
      struct {
         void *buffer;      /* in/out */
         LONG num_bytes;    /* in */
      } read;
      struct {
         LONG abs_byte_seek_pos; /* out */
      } seek;
   } data;

} MPEGA_ACCESS;

/* Decoding output settings */

typedef struct {
   WORD freq_div;    /* 1, 2 or 4 */
   WORD quality;     /* 0 (low) .. 2 (high) */
   LONG freq_max;    /* for automatic freq_div (if mono_freq_div == 0) */
} MPEGA_OUTPUT;

/* Decoding layer settings */
typedef struct {
   WORD force_mono;        /* 1 to decode stereo stream in mono, 0 otherwise */
   MPEGA_OUTPUT mono;      /* mono settings */
   MPEGA_OUTPUT stereo;    /* stereo settings */
} MPEGA_LAYER;

/* Full control structure of MPEG Audio decoding */
typedef struct {
   struct Hook *bs_access;    /* NULL for default access (file I/O) or give your own bitstream access */
   MPEGA_LAYER layer_1_2;     /* Layer I & II settings */
   MPEGA_LAYER layer_3;       /* Layer III settings */
   WORD check_mpeg;           /* 1 to check for mpeg audio validity at start of stream, 0 otherwise */
   LONG stream_buffer_size;   /* size of bitstream buffer in bytes (0 -> default size) */
                              /* NOTE: stream_buffer_size must be multiple of 4 bytes */
} MPEGA_CTRL;

/* MPEG Audio modes */

#define MPEGA_MODE_STEREO   0
#define MPEGA_MODE_J_STEREO 1
#define MPEGA_MODE_DUAL     2
#define MPEGA_MODE_MONO     3

typedef struct {
   /* Public data (read only) */
   /* Stream info */
   WORD  norm;          /* 1 or 2 */
   WORD  layer;         /* 1..3 */
   WORD  mode;          /* 0..3  (MPEGA_MODE_xxx) */
   WORD  bitrate;       /* in kbps */
   LONG  frequency;     /* in Hz */
   WORD  channels;      /* 1 or 2 */
   ULONG ms_duration;   /* stream duration in ms */
   WORD  private_bit;   /* 0 or 1 */
   WORD  copyright;     /* 0 or 1 */
   WORD  original;      /* 0 or 1 */
   /* Decoding info according to MPEG control */
   WORD  dec_channels;  /* decoded channels 1 or 2 */
   WORD  dec_quality;   /* decoding quality 0..2 */
   LONG  dec_frequency; /* decoding frequency in Hz */

   /* Private data */
   void  *handle;
} MPEGA_STREAM;

#define MPEGA_MAX_CHANNELS 2    // Max channels
#define MPEGA_PCM_SIZE     1152 // Max samples per frame

/* Error codes */

#define MPEGA_ERR_NONE     0
#define MPEGA_ERR_BASE     0
#define MPEGA_ERR_EOF      (MPEGA_ERR_BASE-1)
#define MPEGA_ERR_BADFRAME (MPEGA_ERR_BASE-2)
#define MPEGA_ERR_MEM      (MPEGA_ERR_BASE-3)
#define MPEGA_ERR_NO_SYNC  (MPEGA_ERR_BASE-4)
#define MPEGA_ERR_BADVALUE (MPEGA_ERR_BASE-5) /* #1 */

#endif /* LIBRARIES_MPEGA_H */
