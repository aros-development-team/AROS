/*
 * Copyright (c) 1988-1997 Sam Leffler
 * Copyright (c) 1991-1997 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 *
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 *
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#ifndef _TIFFTYPES_
#define	_TIFFTYPES_

#include <tiffconf.h>
#include <tiff.h>

#include <stdarg.h>
#include <stdio.h>

/*
 * TIFF is defined as an incomplete type to hide the
 * library's internal data structures from clients.
 */
typedef struct tiff TIFF;

/*
 * The following typedefs define the intrinsic size of
 * data types used in the *exported* interfaces.  These
 * definitions depend on the proper definition of types
 * in tiff.h.  Note also that the varargs interface used
 * to pass tag types and values uses the types defined in
 * tiff.h directly.
 *
 * NB: ttag_t is unsigned int and not unsigned short because
 *     ANSI C requires that the type before the ellipsis be a
 *     promoted type (i.e. one of int, unsigned int, pointer,
 *     or double) and because we defined pseudo-tags that are
 *     outside the range of legal Aldus-assigned tags.
 * NB: tsize_t is signed and not unsigned because some functions
 *     return -1.
 * NB: toff_t is not off_t for many reasons; TIFFs max out at
 *     32-bit file offsets, and BigTIFF maxes out at 64-bit
 *     offsets being the most important, and to ensure use of
 *     a consistently unsigned type across architectures.
 *     Prior to libtiff 4.0, this was an unsigned 32 bit type.
 */
/*
 * this is the machine addressing size type, only it's signed, so make it
 * int32_t on 32bit machines, int64_t on 64bit machines
 */
typedef signed long tmsize_t;

typedef uint64_t toff_t;          /* file offset */
/* the following are deprecated and should be replaced by their defining
   counterparts */
typedef uint32_t ttag_t;          /* directory tag */
typedef uint16_t tdir_t;          /* directory index */
typedef uint16_t tsample_t;       /* sample number */
typedef uint32_t tstrile_t;       /* strip or tile number */
typedef tstrile_t tstrip_t;     /* strip number */
typedef tstrile_t ttile_t;      /* tile number */
typedef tmsize_t tsize_t;       /* i/o size in bytes */
typedef void* tdata_t;          /* image data ref */

typedef void* thandle_t;       /* client data handle */

/* Structure for holding information about a display device. */

typedef unsigned char TIFFRGBValue;               /* 8-bit samples */

typedef struct {
	float d_mat[3][3];                        /* XYZ -> luminance matrix */
	float d_YCR;                              /* Light o/p for reference white */
	float d_YCG;
	float d_YCB;
	uint32_t d_Vrwr;                            /* Pixel values for ref. white */
	uint32_t d_Vrwg;
	uint32_t d_Vrwb;
	float d_Y0R;                              /* Residual light for black pixel */
	float d_Y0G;
	float d_Y0B;
	float d_gammaR;                           /* Gamma values for the three guns */
	float d_gammaG;
	float d_gammaB;
} TIFFDisplay;

typedef struct {                                  /* YCbCr->RGB support */
	TIFFRGBValue* clamptab;                   /* range clamping table */
	int* Cr_r_tab;
	int* Cb_b_tab;
	int32_t* Cr_g_tab;
	int32_t* Cb_g_tab;
	int32_t* Y_tab;
} TIFFYCbCrToRGB;

typedef struct {                                  /* CIE Lab 1976->RGB support */
	int range;                                /* Size of conversion table */
#define CIELABTORGB_TABLE_RANGE 1500
	float rstep, gstep, bstep;
	float X0, Y0, Z0;                         /* Reference white point */
	TIFFDisplay display;
	float Yr2r[CIELABTORGB_TABLE_RANGE + 1];  /* Conversion of Yr to r */
	float Yg2g[CIELABTORGB_TABLE_RANGE + 1];  /* Conversion of Yg to g */
	float Yb2b[CIELABTORGB_TABLE_RANGE + 1];  /* Conversion of Yb to b */
} TIFFCIELabToRGB;

/*
 * RGBA-style image support.
 */
typedef struct _TIFFRGBAImage TIFFRGBAImage;
/*
 * The image reading and conversion routines invoke
 * ``put routines'' to copy/image/whatever tiles of
 * raw image data.  A default set of routines are 
 * provided to convert/copy raw image data to 8-bit
 * packed ABGR format rasters.  Applications can supply
 * alternate routines that unpack the data into a
 * different format or, for example, unpack the data
 * and draw the unpacked raster on the display.
 */
typedef void (*tileContigRoutine)
    (TIFFRGBAImage*, uint32_t*, uint32_t, uint32_t, uint32_t, uint32_t, int32_t, int32_t,
     unsigned char*);
typedef void (*tileSeparateRoutine)
    (TIFFRGBAImage*, uint32_t*, uint32_t, uint32_t, uint32_t, uint32_t, int32_t, int32_t,
     unsigned char*, unsigned char*, unsigned char*, unsigned char*);
/*
 * RGBA-reader state.
 */
struct _TIFFRGBAImage {
	TIFF* tif;                              /* image handle */
	int stoponerr;                          /* stop on read error */
	int isContig;                           /* data is packed/separate */
	int alpha;                              /* type of alpha data present */
	uint32_t width;                           /* image width */
	uint32_t height;                          /* image height */
	uint16_t bitspersample;                   /* image bits/sample */
	uint16_t samplesperpixel;                 /* image samples/pixel */
	uint16_t orientation;                     /* image orientation */
	uint16_t req_orientation;                 /* requested orientation */
	uint16_t photometric;                     /* image photometric interp */
	uint16_t* redcmap;                        /* colormap palette */
	uint16_t* greencmap;
	uint16_t* bluecmap;
	/* get image data routine */
	int (*get)(TIFFRGBAImage*, uint32_t*, uint32_t, uint32_t);
	/* put decoded strip/tile */
	union {
	    void (*any)(TIFFRGBAImage*);
	    tileContigRoutine contig;
	    tileSeparateRoutine separate;
	} put;
	TIFFRGBValue* Map;                      /* sample mapping array */
	uint32_t** BWmap;                         /* black&white map */
	uint32_t** PALmap;                        /* palette image map */
	TIFFYCbCrToRGB* ycbcr;                  /* YCbCr conversion state */
	TIFFCIELabToRGB* cielab;                /* CIE L*a*b conversion state */

	uint8_t* UaToAa;                          /* Unassociated alpha to associated alpha conversion LUT */
	uint8_t* Bitdepth16To8;                   /* LUT for conversion from 16bit to 8bit values */

	int row_offset;
	int col_offset;
};

/*
 * A CODEC is a software package that implements decoding,
 * encoding, or decoding+encoding of a compression algorithm.
 * The library provides a collection of builtin codecs.
 * More codecs may be registered through calls to the library
 * and/or the builtin implementations may be overridden.
 */
typedef int (*TIFFInitMethod)(TIFF*, int);
typedef struct {
	char* name;
	uint16_t scheme;
	TIFFInitMethod init;
} TIFFCodec;

typedef void (*TIFFErrorHandler)(const char*, const char*, va_list);
typedef void (*TIFFErrorHandlerExt)(thandle_t, const char*, const char*, va_list);
typedef tmsize_t (*TIFFReadWriteProc)(thandle_t, void*, tmsize_t);
typedef toff_t (*TIFFSeekProc)(thandle_t, toff_t, int);
typedef int (*TIFFCloseProc)(thandle_t);
typedef toff_t (*TIFFSizeProc)(thandle_t);
typedef int (*TIFFMapFileProc)(thandle_t, void** base, toff_t* size);
typedef void (*TIFFUnmapFileProc)(thandle_t, void* base, toff_t size);
typedef void (*TIFFExtendProc)(TIFF*);


typedef struct _TIFFField TIFFField;
typedef struct _TIFFFieldArray TIFFFieldArray;

typedef int (*TIFFVSetMethod)(TIFF*, uint32_t, va_list);
typedef int (*TIFFVGetMethod)(TIFF*, uint32_t, va_list);
typedef void (*TIFFPrintMethod)(TIFF*, FILE*, long);

typedef struct {
    TIFFVSetMethod vsetfield; /* tag set routine */
    TIFFVGetMethod vgetfield; /* tag get routine */
    TIFFPrintMethod printdir; /* directory print routine */
} TIFFTagMethods;

/****************************************************************************
 *               O B S O L E T E D    I N T E R F A C E S
 *
 * Don't use this stuff in your applications, it may be removed in the future
 * libtiff versions.
 ****************************************************************************/
typedef	struct {
	ttag_t	field_tag;		/* field's tag */
	short	field_readcount;	/* read count/TIFF_VARIABLE/TIFF_SPP */
	short	field_writecount;	/* write count/TIFF_VARIABLE */
	TIFFDataType field_type;	/* type of associated data */
        unsigned short field_bit;	/* bit in fieldsset bit vector */
	unsigned char field_oktochange;	/* if true, can change while writing */
	unsigned char field_passcount;	/* if true, pass dir count on set */
	char	*field_name;		/* ASCII name */
} TIFFFieldInfo;

#endif /* _TIFFTYPES_ */
