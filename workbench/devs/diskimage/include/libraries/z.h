#ifndef LIBRARIES_Z_H
#define LIBRARIES_Z_H

#include <exec/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ZLIB_VERSION "1.2.4"
#define ZLIB_VERNUM 0x1240
#define ZLIB_VER_MAJOR 1
#define ZLIB_VER_MINOR 2
#define ZLIB_VER_REVISION 4
#define ZLIB_VER_SUBREVISION 0

/*
    The 'zlib' compression library provides in-memory compression and
  decompression functions, including integrity checks of the uncompressed data.
  This version of the library supports only one compression method (deflation)
  but other algorithms will be added later and will have the same stream
  interface.

    Compression can be done in a single step if the buffers are large enough,
  or can be done by repeated calls of the compression function.  In the latter
  case, the application must provide more input and/or consume the output
  (providing more output space) before each call.

    The compressed data format used by default by the in-memory functions is
  the zlib format, which is a zlib wrapper documented in RFC 1950, wrapped
  around a deflate stream, which is itself documented in RFC 1951.

    The library also supports reading and writing files in gzip (.gz) format
  with an interface similar to that of stdio using the functions that start
  with "gz".  The gzip format is different from the zlib format.  gzip is a
  gzip wrapper, documented in RFC 1952, wrapped around a deflate stream.

    This library can optionally read and write gzip streams in memory as well.

    The zlib format was designed to be compact and fast for use in memory
  and on communications channels.  The gzip format was designed for single-
  file compression on file systems, has a larger header than zlib to maintain
  directory information, and uses a different, slower check method than zlib.

    The library does not install any signal handler.  The decoder checks
  the consistency of the compressed data, so the library should never crash
  even in case of corrupted input.
*/

typedef APTR (*alloc_func) (APTR opaque, ULONG items, ULONG size);
typedef void (*free_func)  (APTR opaque, APTR address);

struct internal_state;

typedef struct z_stream_s {
    UBYTE     *next_in;  /* next input byte */
    ULONG      avail_in;  /* number of bytes available at next_in */
    ULONG      total_in;  /* total nb of input bytes read so far */

    UBYTE     *next_out; /* next output byte should be put there */
    ULONG      avail_out; /* remaining free space at next_out */
    ULONG      total_out; /* total nb of bytes output so far */

    char      *msg;      /* last error message, NULL if no error */
    struct internal_state *state; /* not visible by applications */

    alloc_func zalloc;  /* used to allocate the internal state */
    free_func  zfree;   /* used to free the internal state */
    APTR       opaque;  /* private data object passed to zalloc and zfree */

    LONG       data_type;  /* best guess about the data type: binary or text */
    ULONG      adler;      /* adler32 value of the uncompressed data */
    ULONG      reserved;   /* reserved for future use */
} z_stream;

typedef z_stream *z_streamp;

/*
     gzip header information passed to and from zlib routines.  See RFC 1952
  for more details on the meanings of these fields.
*/
typedef struct gz_header_s {
    LONG     text;       /* true if compressed data believed to be text */
    ULONG    time;       /* modification time */
    LONG     xflags;     /* extra flags (not used when writing a gzip file) */
    LONG     os;         /* operating system */
    UBYTE   *extra;     /* pointer to extra field or Z_NULL if none */
    ULONG    extra_len;  /* extra field length (valid if extra != Z_NULL) */
    ULONG    extra_max;  /* space at extra (only when reading header) */
    UBYTE   *name;      /* pointer to zero-terminated file name or Z_NULL */
    ULONG    name_max;   /* space at name (only when reading header) */
    UBYTE   *comment;   /* pointer to zero-terminated comment or Z_NULL */
    ULONG    comm_max;   /* space at comment (only when reading header) */
    LONG     hcrc;       /* true if there was or will be a header crc */
    LONG     done;       /* true when done reading gzip header (not used
                           when writing a gzip file) */
} gz_header;

typedef gz_header *gz_headerp;

/*
     The application must update next_in and avail_in when avail_in has dropped
   to zero.  It must update next_out and avail_out when avail_out has dropped
   to zero.  The application must initialize zalloc, zfree and opaque before
   calling the init function.  All other fields are set by the compression
   library and must not be updated by the application.

     The opaque value provided by the application will be passed as the first
   parameter for calls of zalloc and zfree.  This can be useful for custom
   memory management.  The compression library attaches no meaning to the
   opaque value.

     zalloc must return Z_NULL if there is not enough memory for the object.
   If zlib is used in a multi-threaded application, zalloc and zfree must be
   thread safe.

     On 16-bit systems, the functions zalloc and zfree must be able to allocate
   exactly 65536 bytes, but will not be required to allocate more than this if
   the symbol MAXSEG_64K is defined (see zconf.h).  WARNING: On MSDOS, pointers
   returned by zalloc for objects of exactly 65536 bytes *must* have their
   offset normalized to zero.  The default allocation function provided by this
   library ensures this (see zutil.c).  To reduce memory requirements and avoid
   any allocation of 64K objects, at the expense of compression ratio, compile
   the library with -DMAX_WBITS=14 (see zconf.h).

     The fields total_in and total_out can be used for statistics or progress
   reports.  After compression, total_in holds the total size of the
   uncompressed data and may be saved for use in the decompressor (particularly
   if the decompressor wants to decompress everything in a single step).
*/

                        /* constants */

#define Z_NO_FLUSH      0
#define Z_PARTIAL_FLUSH 1
#define Z_SYNC_FLUSH    2
#define Z_FULL_FLUSH    3
#define Z_FINISH        4
#define Z_BLOCK         5
#define Z_TREES         6
/* Allowed flush values; see deflate() and inflate() below for details */

#define Z_OK            0
#define Z_STREAM_END    1
#define Z_NEED_DICT     2
#define Z_ERRNO        (-1)
#define Z_STREAM_ERROR (-2)
#define Z_DATA_ERROR   (-3)
#define Z_MEM_ERROR    (-4)
#define Z_BUF_ERROR    (-5)
#define Z_VERSION_ERROR (-6)
/* Return codes for the compression/decompression functions. Negative values
 * are errors, positive values are used for special but normal events.
 */

#define Z_NO_COMPRESSION         0
#define Z_BEST_SPEED             1
#define Z_BEST_COMPRESSION       9
#define Z_DEFAULT_COMPRESSION  (-1)
/* compression levels */

#define Z_FILTERED            1
#define Z_HUFFMAN_ONLY        2
#define Z_RLE                 3
#define Z_FIXED               4
#define Z_DEFAULT_STRATEGY    0
/* compression strategy; see deflateInit2() below for details */

#define Z_BINARY   0
#define Z_TEXT     1
#define Z_ASCII    Z_TEXT   /* for compatibility with 1.2.2 and earlier */
#define Z_UNKNOWN  2
/* Possible values of the data_type field (though see inflate()) */

#define Z_DEFLATED   8
/* The deflate compression method (the only one supported in this version) */

#define Z_NULL  0  /* for initializing zalloc, zfree, opaque */

/* Typedefs for the interface */

typedef ULONG (*in_func) (APTR, UBYTE **);
typedef LONG (*out_func) (APTR, UBYTE *, ULONG);

#ifdef __cplusplus
}
#endif

#endif /* ZLIB_H */
