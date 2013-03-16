/* Pixel formats in AROS have their ID based on the pixel
   component layout in memory. So vHidd_StdPixFmt_ARGB32
   means 0xAA 0xRR 0xGG 0xBB in memory. When doing pixel
   accesses (ULONG in this case) to memory on big endian
   machines the pixel (ULONG) must in this case be ARGB32,
   while on little endian machines it must be BGRA32.

   For pixel conversion routines it might make more sense
   to have pixel format IDs which are based on the pixel
   access, so that for an ARGB32 format the pixel (ULONG)
   can be in ARGB32 format both on little and big endian
   machines.

   Therefore the FMT_ stuff below. "OE" means other
   endianess, ie. byte swapped. */

#if AROS_BIG_ENDIAN
#define FMT_RGB24   vHidd_StdPixFmt_RGB24
#define FMT_BGR24   vHidd_StdPixFmt_BGR24
#define FMT_RGB16   vHidd_StdPixFmt_RGB16
#define FMT_RGB16OE vHidd_StdPixFmt_RGB16_LE
#define FMT_BGR16   vHidd_StdPixFmt_BGR16
#define FMT_BGR16OE vHidd_StdPixFmt_BGR16_LE
#define FMT_RGB15   vHidd_StdPixFmt_RGB15
#define FMT_RGB15OE vHidd_StdPixFmt_RGB15_LE
#define FMT_BGR15   vHidd_StdPixFmt_BGR15
#define FMT_BGR15OE vHidd_StdPixFmt_BGR15_LE
#define FMT_ARGB32  vHidd_StdPixFmt_ARGB32
#define FMT_BGRA32  vHidd_StdPixFmt_BGRA32
#define FMT_RGBA32  vHidd_StdPixFmt_RGBA32
#define FMT_ABGR32  vHidd_StdPixFmt_ABGR32
#define FMT_XRGB32  vHidd_StdPixFmt_0RGB32
#define FMT_BGRX32  vHidd_StdPixFmt_BGR032
#define FMT_RGBX32  vHidd_StdPixFmt_RGB032
#define FMT_XBGR32  vHidd_StdPixFmt_0BGR32
#else
#define FMT_RGB24   vHidd_StdPixFmt_BGR24
#define FMT_BGR24   vHidd_StdPixFmt_RGB24
#define FMT_RGB16   vHidd_StdPixFmt_RGB16_LE
#define FMT_RGB16OE vHidd_StdPixFmt_RGB16
#define FMT_BGR16   vHidd_StdPixFmt_BGR16_LE
#define FMT_BGR16OE vHidd_StdPixFmt_BGR16
#define FMT_RGB15   vHidd_StdPixFmt_RGB15_LE
#define FMT_RGB15OE vHidd_StdPixFmt_RGB15
#define FMT_BGR15   vHidd_StdPixFmt_BGR15_LE
#define FMT_BGR15OE vHidd_StdPixFmt_BGR15
#define FMT_ARGB32  vHidd_StdPixFmt_BGRA32
#define FMT_BGRA32  vHidd_StdPixFmt_ARGB32
#define FMT_RGBA32  vHidd_StdPixFmt_ABGR32
#define FMT_ABGR32  vHidd_StdPixFmt_RGBA32
#define FMT_XRGB32  vHidd_StdPixFmt_BGR032
#define FMT_BGRX32  vHidd_StdPixFmt_0RGB32
#define FMT_RGBX32  vHidd_StdPixFmt_0BGR32
#define FMT_XBGR32  vHidd_StdPixFmt_RGB032
#endif

/* Component masks and shifts. Shifts are to left (!) not right, so
   that most significant component bit becomes bit #31 */

#define RGB24_RMASK 0x00FF0000
#define RGB24_GMASK 0x0000FF00
#define RGB24_BMASK 0x000000FF
#define RGB24_RSHIFT 8
#define RGB24_GSHIFT 16
#define RGB24_BSHIFT 24
#define RGB24_BITS 24

#define BGR24_RMASK 0x000000FF
#define BGR24_GMASK 0x0000FF00
#define BGR24_BMASK 0x00FF0000
#define BGR24_RSHIFT 24
#define BGR24_GSHIFT 16
#define BGR24_BSHIFT 8
#define BGR24_BITS 24

#define RGB16_RMASK 0xF800
#define RGB16_GMASK 0x07E0
#define RGB16_BMASK 0x001F
#define RGB16_RSHIFT 16
#define RGB16_GSHIFT 21
#define RGB16_BSHIFT 27
#define RGB16_BITS 16

#define BGR16_RMASK 0x001F
#define BGR16_GMASK 0x07E0
#define BGR16_BMASK 0xF800
#define BGR16_RSHIFT 27
#define BGR16_GSHIFT 21
#define BGR16_BSHIFT 16
#define BGR16_BITS 16

#define RGB15_RMASK 0x7C00
#define RGB15_GMASK 0x03E0
#define RGB15_BMASK 0x001F
#define RGB15_RSHIFT 17
#define RGB15_GSHIFT 22
#define RGB15_BSHIFT 27
#define RGB15_BITS 15

#define BGR15_RMASK 0x001F
#define BGR15_GMASK 0x03E0
#define BGR15_BMASK 0x7C00
#define BGR15_RSHIFT 27
#define BGR15_GSHIFT 22
#define BGR15_BSHIFT 17
#define BGR15_BITS 15

#define ARGB32_RMASK 0x00FF0000
#define ARGB32_GMASK 0x0000FF00
#define ARGB32_BMASK 0x000000FF
#define ARGB32_AMASK 0xFF000000
#define ARGB32_RSHIFT 8
#define ARGB32_GSHIFT 16
#define ARGB32_BSHIFT 24
#define ARGB32_ASHIFT 0
#define ARGB32_BITS 32

#define BGRA32_RMASK 0x0000FF00
#define BGRA32_GMASK 0x00FF0000
#define BGRA32_BMASK 0xFF000000
#define BGRA32_AMASK 0x000000FF
#define BGRA32_RSHIFT 16
#define BGRA32_GSHIFT 8
#define BGRA32_BSHIFT 0
#define BGRA32_ASHIFT 24
#define BGRA32_BITS 32

#define RGBA32_RMASK 0xFF000000
#define RGBA32_GMASK 0x00FF0000
#define RGBA32_BMASK 0x0000FF00
#define RGBA32_AMASK 0x000000FF
#define RGBA32_RSHIFT 0
#define RGBA32_GSHIFT 8
#define RGBA32_BSHIFT 16
#define RGBA32_ASHIFT 24
#define RGBA32_BITS 32

#define ABGR32_RMASK 0x000000FF
#define ABGR32_GMASK 0x0000FF00
#define ABGR32_BMASK 0x00FF0000
#define ABGR32_AMASK 0xFF000000
#define ABGR32_RSHIFT 24
#define ABGR32_GSHIFT 16
#define ABGR32_BSHIFT 8
#define ABGR32_ASHIFT 0
#define ABGR32_BITS 32

#define DOSHIFT(val,shift) (((shift) < 0) ? ((val) << (-(shift))) : ((val) >> (shift)))

/* Shift from a pixel format with many bits per component to one with fewer bits per component */

#define DOWNSHIFT16(val,src,dst) ((DOSHIFT(val, (dst ## _ ## RSHIFT - src ## _ ## RSHIFT)) & dst ## _ ## RMASK) | \
                                   (DOSHIFT(val, (dst ## _ ## GSHIFT - src ## _ ## GSHIFT)) & dst ## _ ## GMASK) | \
                                   (DOSHIFT(val, (dst ## _ ## BSHIFT - src ## _ ## BSHIFT)) & dst ## _ ## BMASK) )

/* Shift from a pixel format with few bits per component to one with more bits per component */

#define UPSHIFT16(val,src,dst)   (DOSHIFT((val) & src ## _ ## RMASK, (dst ## _ ## RSHIFT - src ## _ ## RSHIFT)) | \
                                   DOSHIFT((val) & src ## _ ## GMASK, (dst ## _ ## GSHIFT - src ## _ ## GSHIFT)) | \
                                   DOSHIFT((val) & src ## _ ## BMASK, (dst ## _ ## BSHIFT - src ## _ ## BSHIFT)) )

/* Shuffle around components (if src and dst pixfmt use same number of bits per component) */

#define SHUFFLE(val,src,dst)     DOWNSHIFT16(val,src,dst)

#define SHUFFLE32(val,src,dst)    ((DOSHIFT(val, (dst ## _ ## RSHIFT - src ## _ ## RSHIFT)) & dst ## _ ## RMASK) | \
                                   (DOSHIFT(val, (dst ## _ ## GSHIFT - src ## _ ## GSHIFT)) & dst ## _ ## GMASK) | \
                                   (DOSHIFT(val, (dst ## _ ## BSHIFT - src ## _ ## BSHIFT)) & dst ## _ ## BMASK) | \
                                   (DOSHIFT(val, (dst ## _ ## ASHIFT - src ## _ ## ASHIFT)) & dst ## _ ## AMASK) )

#define SHUFFLE24(val,src,dst)    ((DOSHIFT(val, (dst ## _ ## RSHIFT - src ## _ ## RSHIFT)) & dst ## _ ## RMASK) | \
                                   (DOSHIFT(val, (dst ## _ ## GSHIFT - src ## _ ## GSHIFT)) & dst ## _ ## GMASK) | \
                                   (DOSHIFT(val, (dst ## _ ## BSHIFT - src ## _ ## BSHIFT)) & dst ## _ ## BMASK) )

#define COMP8(val,which)     ( ((val) & (0xFF << ((3 - which) * 8)) ) >> ((3 - which) * 8) )

#if AROS_BIG_ENDIAN
#define PUT24(dst,a,b,c) dst[x*3] = (a); dst[x*3+1] = (b); dst[x*3+2] = (c);
#define GET24               (src[x * 3] << 16) + (src[x * 3 + 1] << 8) + (src[x * 3 + 2]);
#define GET24_INV     (src[x * 3]) + (src[x * 3 + 1] << 8) + (src[x * 3 + 2] << 16);
#else
#define PUT24(dst,a,b,c) dst[x*3] = (c); dst[x*3+1] = (b); dst[x*3+2] = (a);
#define GET24               (src[x * 3]) + (src[x * 3 + 1] << 8) + (src[x * 3 + 2] << 16);
#define GET24_INV     (src[x * 3] << 16) + (src[x * 3 + 1] << 8) + (src[x * 3 + 2]);
#endif

#define INV16(x)     AROS_SWAP_BYTES_WORD(x)

#define CONVERTFUNC(a,b) static ULONG convert_ ## a ## _ ## b \
    (APTR srcPixels, ULONG srcMod, HIDDT_StdPixFmt srcPixFmt, \
    APTR dstPixels, ULONG dstMod, HIDDT_StdPixFmt dstPixFmt, \
    ULONG width, ULONG height) \
{
        
#define CONVERTFUNC_INIT

#define CONVERTFUNC_EXIT }


