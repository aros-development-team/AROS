/*
    Copyright (C) 2022-2026, The AROS Development Team. All rights reserved.
*/

/**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dostags.h>
#include <graphics/gfxbase.h>
#include <graphics/rpattr.h>
#include <cybergraphx/cybergraphics.h>
#include <intuition/imageclass.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>
#include <intuition/cghooks.h>
#include <datatypes/datatypesclass.h>
#include <datatypes/pictureclass.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/iffparse.h>
#include <proto/datatypes.h>
#include <proto/tiff.h>

#include <tiffinline.h>

#include <aros/macros.h>

#include <aros/symbolsets.h>

# include <sys/types.h>
#include <stdarg.h>
#include <stdio.h>

#include "debug.h"
#include "methods.h"

#ifndef MIN
#if defined(__GNUC__) || defined(__clang__)
#define MIN(a, b) ({ \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a < _b ? _a : _b; \
})
#else
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#endif

#ifndef TIFFGetR
/* libtiff's TIFFReadRGBAImage*() returns 32-bit ABGR pixels in native endianness.
   These macros match libtiff's reference TIFFGetR/G/B/A definitions. */
#define TIFFGetR(abgr) ((UBYTE)((abgr) & 0xFF))
#define TIFFGetG(abgr) ((UBYTE)(((abgr) >> 8) & 0xFF))
#define TIFFGetB(abgr) ((UBYTE)(((abgr) >> 16) & 0xFF))
#define TIFFGetA(abgr) ((UBYTE)(((abgr) >> 24) & 0xFF))
#endif

ADD2LIBS("datatypes/picture.datatype", 0, struct Library *, PictureBase);

/**************************************************************************************************/

/* Dummy functions for the linker */
void abort(void)
{
    exit(1);
}

void exit(int bla)
{
    D(bug("[tiff.datatype] %s()\n", __func__));
    abort();
}

/**************************************************************************************************/

static void tiffConvert16to8(UWORD pi, UWORD sspp, ULONG pxfmt, ULONG width, ULONG height, const UBYTE *src, UBYTE *dst)
{
    const UWORD *wsrc = (const UWORD *)src; /* assume properly aligned */
    UBYTE *d = dst;
    ULONG npixels = width * height;
    ULONG i;

    D(bug("[tiff.datatype] %s(%04x, %04x, %08x, %u, %u, 0x%p, 0x%p)\n", __func__, pi, sspp, pxfmt, width, height, src,
          dst));

    /* 1-sample (grayscale / palette) case */
    if(sspp == 1 || sspp == 2) {
        D(bug("[tiff.datatype] %s: greyscale/pallete\n", __func__));
        for(i = 0; i < npixels; ++i) {
            ULONG idx = i * (ULONG)sspp;
            UWORD sample = wsrc[idx];             /* 16-bit sample */
            UBYTE v = (UBYTE)(sample >> 8);     /* take high byte */

            if(pi == PHOTOMETRIC_MINISWHITE) {
                D(bug("[tiff.datatype] %s: MINISWHITE\n", __func__));
                v = (UBYTE)(255 - v);
            }

            if(pxfmt == PBPAFMT_RGB) {
                d[3 * i + 0] = v;
                d[3 * i + 1] = v;
                d[3 * i + 2] = v;
            } else if(pxfmt == PBPAFMT_RGBA) {
                d[4 * i + 0] = v;
                d[4 * i + 1] = v;
                d[4 * i + 2] = v;
                if(sspp == 2)
                    d[4 * i + 3] = (UBYTE)(wsrc[idx + 1] >> 8);
                else
                    d[4 * i + 3] = 0xFF;
            }
        }
        return;
    }

    /* interleaved RGB (3) or RGBA (4) */
    if(sspp == 3 || sspp == 4) {
        D(bug("[tiff.datatype] %s: RGB%s\n", __func__, (sspp == 4) ? "A" : ""));
        ULONG idx;
        for(i = 0; i < npixels; ++i) {
            /* index into 16-bit words: pixel i starts at i * sspp */
            idx = i * (ULONG)sspp;

            UWORD s0 = wsrc[idx + 0];
            UWORD s1 = wsrc[idx + 1];
            UWORD s2 = wsrc[idx + 2];

            UBYTE r = (UBYTE)(s0 >> 8);
            UBYTE g = (UBYTE)(s1 >> 8);
            UBYTE b = (UBYTE)(s2 >> 8);

            if(pi == PHOTOMETRIC_MINISWHITE) {
                D(bug("[tiff.datatype] %s: MINISWHITE\n", __func__));
                r = (UBYTE)(255 - r);
                g = (UBYTE)(255 - g);
                b = (UBYTE)(255 - b);
            }

            if(pxfmt == PBPAFMT_RGB) {
                d[3 * i + 0] = r;
                d[3 * i + 1] = g;
                d[3 * i + 2] = b;
            } else if(pxfmt == PBPAFMT_RGBA) {
                d[4 * i + 0] = r;
                d[4 * i + 1] = g;
                d[4 * i + 2] = b;
                if(sspp == 4) {
                    UWORD sa = wsrc[idx + 3];
                    d[4 * i + 3] = (UBYTE)(sa >> 8);
                } else
                    d[4 * i + 3] = 0xFF;

            }
        }
        return;
    }

    /* unsupported sspp: do nothing (could log) */
}

static void tiffConvert32to8(UWORD pi, UWORD sspp, ULONG pxfmt, ULONG width, ULONG height, const UBYTE *src, UBYTE *dst)
{
    const ULONG *lsrc = (const ULONG *)src;
    ULONG npixels = width * height;

    D(bug("[tiff.datatype] %s(%04x, %04x, %08x, %u, %u, 0x%p, 0x%p)\n", __func__, pi, sspp, pxfmt, width, height, src,
          dst));

    for(ULONG i = 0; i < npixels; i++) {
        if(pi < PHOTOMETRIC_RGB && (sspp == 1 || sspp == 2)) {
            ULONG idx = i * (ULONG)sspp;
            UBYTE pixval = (UBYTE)((lsrc[idx] >> 24) & 0xFF);

            if(pi == PHOTOMETRIC_MINISWHITE) {
                pixval = 255 - pixval;
            }

            if(pxfmt == PBPAFMT_RGB) {
                dst[3 * i + 0] = pixval;
                dst[3 * i + 1] = pixval;
                dst[3 * i + 2] = pixval;
            } else if(pxfmt == PBPAFMT_RGBA) {
                dst[4 * i + 0] = pixval;
                dst[4 * i + 1] = pixval;
                dst[4 * i + 2] = pixval;
                if(sspp == 2)
                    dst[4 * i + 3] = (UBYTE)((lsrc[idx + 1] >> 24) & 0xFF);
                else
                    dst[4 * i + 3] = 0xFF;
            }
        } else if(sspp == 3) {
            const ULONG *long_ptr = &lsrc[i * 3];

            if(pxfmt == PBPAFMT_RGB) {
                dst[3 * i + 0] = (long_ptr[0] >> 24) & 0xFF;
                dst[3 * i + 1] = (long_ptr[1] >> 24) & 0xFF;
                dst[3 * i + 2] = (long_ptr[2] >> 24) & 0xFF;
            } else if(pxfmt == PBPAFMT_RGBA) {
                dst[4 * i + 0] = (long_ptr[0] >> 24) & 0xFF;
                dst[4 * i + 1] = (long_ptr[1] >> 24) & 0xFF;
                dst[4 * i + 2] = (long_ptr[2] >> 24) & 0xFF;
                dst[4 * i + 3] = 0xFF;
            }
        } else if(sspp == 4) {
            const ULONG *long_ptr = &lsrc[i * 4];

            if(pxfmt == PBPAFMT_RGB) {
                dst[3 * i + 0] = (long_ptr[0] >> 24) & 0xFF;
                dst[3 * i + 1] = (long_ptr[1] >> 24) & 0xFF;
                dst[3 * i + 2] = (long_ptr[2] >> 24) & 0xFF;
            } else if(pxfmt == PBPAFMT_RGBA) {
                dst[4 * i + 0] = (long_ptr[0] >> 24) & 0xFF;
                dst[4 * i + 1] = (long_ptr[1] >> 24) & 0xFF;
                dst[4 * i + 2] = (long_ptr[2] >> 24) & 0xFF;
                dst[4 * i + 3] = (long_ptr[3] >> 24) & 0xFF;
            }
        }
    }
}


static void tiffInterleavePlanarSeparate(UBYTE *dst, const UBYTE *src,
        ULONG planeBytes, UWORD spp,
        ULONG bytesPerSample, ULONG pixelCount)
{
    /* src layout: plane0[planeBytes], plane1[planeBytes], ...
       Each plane is a contiguous array of samples for that plane. */
    for(ULONG i = 0; i < pixelCount; ++i) {
        for(UWORD s = 0; s < spp; ++s) {
            const UBYTE *sp = src + (ULONG)s * planeBytes + i * bytesPerSample;
            UBYTE *dp = dst + (i * (ULONG)spp + s) * bytesPerSample;
            CopyMem((APTR)sp, (APTR)dp, bytesPerSample);
        }
    }
}

static void tiffConvert64uTo8(UWORD pi, UWORD sspp, ULONG pxfmt,
                              ULONG width, ULONG height,
                              const UBYTE *src, UBYTE *dst)
{
    const unsigned long long *qsrc = (const unsigned long long *)src;
    ULONG npixels = width * height;

    for(ULONG i = 0; i < npixels; ++i) {
        if(pi < PHOTOMETRIC_RGB && (sspp == 1 || sspp == 2)) {
            ULONG idx = i * (ULONG)sspp;
            UBYTE pixval = (UBYTE)((qsrc[idx] >> 56) & 0xFF);

            if(pi == PHOTOMETRIC_MINISWHITE)
                pixval = 255 - pixval;

            if(pxfmt == PBPAFMT_RGB) {
                dst[3 * i + 0] = pixval;
                dst[3 * i + 1] = pixval;
                dst[3 * i + 2] = pixval;
            } else if(pxfmt == PBPAFMT_RGBA) {
                dst[4 * i + 0] = pixval;
                dst[4 * i + 1] = pixval;
                dst[4 * i + 2] = pixval;
                if(sspp == 2)
                    dst[4 * i + 3] = (UBYTE)((qsrc[idx + 1] >> 56) & 0xFF);
                else
                    dst[4 * i + 3] = 0xFF;
            }
        } else if(sspp == 3) {
            const unsigned long long *p = &qsrc[i * 3];
            UBYTE r = (UBYTE)((p[0] >> 56) & 0xFF);
            UBYTE g = (UBYTE)((p[1] >> 56) & 0xFF);
            UBYTE b = (UBYTE)((p[2] >> 56) & 0xFF);

            if(pi == PHOTOMETRIC_MINISWHITE) {
                r = 255 - r;
                g = 255 - g;
                b = 255 - b;
            }

            if(pxfmt == PBPAFMT_RGB) {
                dst[3 * i + 0] = r;
                dst[3 * i + 1] = g;
                dst[3 * i + 2] = b;
            } else if(pxfmt == PBPAFMT_RGBA) {
                dst[4 * i + 0] = r;
                dst[4 * i + 1] = g;
                dst[4 * i + 2] = b;
                dst[4 * i + 3] = 0xFF;
            }
        } else if(sspp == 4) {
            const unsigned long long *p = &qsrc[i * 4];
            UBYTE r = (UBYTE)((p[0] >> 56) & 0xFF);
            UBYTE g = (UBYTE)((p[1] >> 56) & 0xFF);
            UBYTE b = (UBYTE)((p[2] >> 56) & 0xFF);
            UBYTE a = (UBYTE)((p[3] >> 56) & 0xFF);

            if(pi == PHOTOMETRIC_MINISWHITE) {
                r = 255 - r;
                g = 255 - g;
                b = 255 - b;
            }

            if(pxfmt == PBPAFMT_RGB) {
                dst[3 * i + 0] = r;
                dst[3 * i + 1] = g;
                dst[3 * i + 2] = b;
            } else if(pxfmt == PBPAFMT_RGBA) {
                dst[4 * i + 0] = r;
                dst[4 * i + 1] = g;
                dst[4 * i + 2] = b;
                dst[4 * i + 3] = a;
            }
        }
    }
}

static void tiffConvert64fTo8(UWORD pi, UWORD sspp, ULONG pxfmt,
                              ULONG width, ULONG height,
                              const UBYTE *src, UBYTE *dst,
                              BOOL haveRange, double smin, double smax)
{
    const double *dsrc = (const double *)src;
    ULONG npixels = width * height;

    double vmin = smin, vmax = smax;

    if(!haveRange) {
        BOOL haveAny = FALSE;
        vmin = 0.0;
        vmax = 0.0;
        for(ULONG i = 0; i < npixels * (ULONG)sspp; ++i) {
            double v = dsrc[i];
            if(!isfinite(v))
                continue;
            if(!haveAny) {
                vmin = vmax = v;
                haveAny = TRUE;
            } else {
                if(v < vmin) vmin = v;
                if(v > vmax) vmax = v;
            }
        }
        if(!haveAny) {
            vmin = 0.0;
            vmax = 1.0;
        }
    }

    double denom = (vmax - vmin);
    if(denom == 0.0)
        denom = 1.0;

    for(ULONG i = 0; i < npixels; ++i) {
        if(pi < PHOTOMETRIC_RGB && (sspp == 1 || sspp == 2)) {
            ULONG idx = i * (ULONG)sspp;
            double gv = dsrc[idx + 0];
            UBYTE g8 = 0;

            if(isfinite(gv)) {
                double n = (gv - vmin) / denom;
                if(n < 0.0) n = 0.0;
                if(n > 1.0) n = 1.0;
                g8 = (UBYTE)(n * 255.0 + 0.5);
            }

            if(pi == PHOTOMETRIC_MINISWHITE)
                g8 = 255 - g8;

            UBYTE a8 = 0xFF;
            if(sspp == 2) {
                double av = dsrc[idx + 1];
                if(isfinite(av)) {
                    if(!haveRange) {
                        /* when auto-ranging, treat alpha as [0..1] */
                        double n = av;
                        if(n < 0.0) n = 0.0;
                        if(n > 1.0) n = 1.0;
                        a8 = (UBYTE)(n * 255.0 + 0.5);
                    } else {
                        double n = (av - vmin) / denom;
                        if(n < 0.0) n = 0.0;
                        if(n > 1.0) n = 1.0;
                        a8 = (UBYTE)(n * 255.0 + 0.5);
                    }
                } else {
                    a8 = 0;
                }
            }

            if(pxfmt == PBPAFMT_RGB) {
                dst[3 * i + 0] = g8;
                dst[3 * i + 1] = g8;
                dst[3 * i + 2] = g8;
            } else if(pxfmt == PBPAFMT_RGBA) {
                dst[4 * i + 0] = g8;
                dst[4 * i + 1] = g8;
                dst[4 * i + 2] = g8;
                dst[4 * i + 3] = a8;
            }
        } else if(sspp == 3 || sspp == 4) {
            ULONG idx = i * (ULONG)sspp;
            double rv = dsrc[idx + 0];
            double gv = dsrc[idx + 1];
            double bv = dsrc[idx + 2];

            UBYTE r8 = 0, g8 = 0, b8 = 0, a8 = 0xFF;

            if(isfinite(rv)) {
                double n = (rv - vmin) / denom;
                if(n < 0.0) n = 0.0;
                if(n > 1.0) n = 1.0;
                r8 = (UBYTE)(n * 255.0 + 0.5);
            }
            if(isfinite(gv)) {
                double n = (gv - vmin) / denom;
                if(n < 0.0) n = 0.0;
                if(n > 1.0) n = 1.0;
                g8 = (UBYTE)(n * 255.0 + 0.5);
            }
            if(isfinite(bv)) {
                double n = (bv - vmin) / denom;
                if(n < 0.0) n = 0.0;
                if(n > 1.0) n = 1.0;
                b8 = (UBYTE)(n * 255.0 + 0.5);
            }

            if(pi == PHOTOMETRIC_MINISWHITE) {
                r8 = 255 - r8;
                g8 = 255 - g8;
                b8 = 255 - b8;
            }

            if(sspp == 4) {
                double av = dsrc[idx + 3];
                if(isfinite(av)) {
                    if(!haveRange) {
                        double n = av;
                        if(n < 0.0) n = 0.0;
                        if(n > 1.0) n = 1.0;
                        a8 = (UBYTE)(n * 255.0 + 0.5);
                    } else {
                        double n = (av - vmin) / denom;
                        if(n < 0.0) n = 0.0;
                        if(n > 1.0) n = 1.0;
                        a8 = (UBYTE)(n * 255.0 + 0.5);
                    }
                } else {
                    a8 = 0;
                }
            }

            if(pxfmt == PBPAFMT_RGB) {
                dst[3 * i + 0] = r8;
                dst[3 * i + 1] = g8;
                dst[3 * i + 2] = b8;
            } else if(pxfmt == PBPAFMT_RGBA) {
                dst[4 * i + 0] = r8;
                dst[4 * i + 1] = g8;
                dst[4 * i + 2] = b8;
                dst[4 * i + 3] = a8;
            }
        }
    }
}

static ULONG tiffRowBytes(ULONG width, ULONG pxfmt)
{
    if(pxfmt == PBPAFMT_RGBA) {
        return width * 4;
    }
    if(pxfmt == PBPAFMT_RGB) {
        return width * 3;
    }
    if(pxfmt == PBPAFMT_LUT8) {
        return width;
    }

    return width;
}

static BOOL tiffLoadRGBAFallback(struct IClass *cl, Object *o, TIFF *tif, ULONG width, ULONG height)
{
    ULONG pixelCount = width * height;
    ULONG *raster = AllocVec(pixelCount * sizeof(ULONG), MEMF_ANY);
    UBYTE *rgba = NULL;

    D(bug("[tiff.datatype] %s(%u,%u)\n", __func__, width, height));

    if(!raster) {
        return FALSE;
    }

    if(!TIFFReadRGBAImageOriented(tif, width, height, raster, ORIENTATION_TOPLEFT, 0)) {
        FreeVec(raster);
        return FALSE;
    }

    rgba = AllocVec(pixelCount * 4, MEMF_ANY);
    if(!rgba) {
        FreeVec(raster);
        return FALSE;
    }

    for(ULONG i = 0; i < pixelCount; ++i) {
        ULONG pixel = raster[i];
        rgba[4 * i + 0] = TIFFGetR(pixel);
        rgba[4 * i + 1] = TIFFGetG(pixel);
        rgba[4 * i + 2] = TIFFGetB(pixel);
        rgba[4 * i + 3] = TIFFGetA(pixel);
    }

    if(!DoSuperMethod(cl, o,
                      PDTM_WRITEPIXELARRAY,
                      (IPTR)rgba,
                      PBPAFMT_RGBA,
                      width * 4,
                      0,
                      0,
                      width,
                      height)) {
        D(bug("[tiff.datatype] %s: DT object failed to render\n", __func__));
        FreeVec(rgba);
        FreeVec(raster);
        return FALSE;
    }

    FreeVec(rgba);
    FreeVec(raster);
    return TRUE;
}


static void tiffYCbCr2RGB(UBYTE y, UBYTE cb, UBYTE cr, UBYTE *r, UBYTE *g, UBYTE *b)
{
    LONG c = (LONG)y - 16;
    LONG d = (LONG)cb - 128;
    LONG e = (LONG)cr - 128;

    LONG rr = (298 * c + 409 * e + 128) >> 8;
    LONG gg = (298 * c - 100 * d - 208 * e + 128) >> 8;
    LONG bb = (298 * c + 516 * d + 128) >> 8;

    if(rr < 0)
        rr = 0;
    else if(rr > 255)
        rr = 255;
    if(gg < 0)
        gg = 0;
    else if(gg > 255)
        gg = 255;
    if(bb < 0)
        bb = 0;
    else if(bb > 255)
        bb = 255;

    *r = (UBYTE)rr;
    *g = (UBYTE)gg;
    *b = (UBYTE)bb;
}

static void tiffConvertYCbCr(ULONG pxfmt, ULONG width, ULONG height,
                             UBYTE *src, UBYTE *dst)
{
    ULONG i;

    D(bug("[tiff.datatype] %s()\n", __func__));

    for(i = 0; i < width * height; i++) {
        UBYTE y  = src[i * 3 + 0];
        UBYTE cb = src[i * 3 + 1];
        UBYTE cr = src[i * 3 + 2];

        UBYTE r, g, b;
        tiffYCbCr2RGB(y, cb, cr, &r, &g, &b);

        if(pxfmt == PBPAFMT_RGB) {
            dst[i * 3 + 0] = r;
            dst[i * 3 + 1] = g;
            dst[i * 3 + 2] = b;
        } else if(pxfmt == PBPAFMT_RGBA) {
            dst[i * 4 + 0] = r;
            dst[i * 4 + 1] = g;
            dst[i * 4 + 2] = b;
            dst[i * 4 + 3] = 0xFF;
        }
    }
}

/**************************************************************************************************/

static BOOL LoadTIFF(struct IClass *cl, Object *o)
{
    union {
        struct IFFHandle   *iff;
        BPTR                bptr;
    } filehandle;
    IPTR                    sourcetype;
    struct BitMapHeader     *bmhd;
    TIFF          *tif;
    char tiffFName[1024];

    D(bug("[tiff.datatype] %s()\n", __func__));

    if(GetDTAttrs(o,   DTA_SourceType, (IPTR)&sourcetype,
                  DTA_Handle, (IPTR)&filehandle,
                  PDTA_BitMapHeader, (IPTR)&bmhd,
                  TAG_DONE) != 3) {
        return FALSE;
    }

    if(sourcetype == DTST_RAM && filehandle.iff == NULL && bmhd) {
        D(bug("[tiff.datatype] %s: Creating an empty object\n", __func__));
        return TRUE;
    }
    if(sourcetype != DTST_FILE || !filehandle.bptr || !bmhd) {
        D(bug("[tiff.datatype] %s: unsupported mode\n", __func__));
        return FALSE;
    }

    NameFromFH(filehandle.bptr, tiffFName, 1023);
    D(bug("[tiff.datatype] %s: opening '%s'\n", __func__, tiffFName));

    tif = TIFFOpen(tiffFName, "r");
    if(tif) {
        ULONG imageLength = 0, tileLength = 0;
        ULONG imageWidth = 0, tileWidth = 0;
        ULONG RowsPerStrip = 0;
        UWORD BitsPerSample = 0, samplesize;
        UWORD SamplesPerPixel = 0;
        UWORD PhotometricInterpretation = 0;
        UWORD compression = 0;
        UWORD planar = PLANARCONFIG_CONTIG;
        UWORD sampleFormat = SAMPLEFORMAT_UINT;
        UWORD ycbcrSubSamplingH = 1, ycbcrSubSamplingV = 1;
        double SMinSampleValue = 0.0, SMaxSampleValue = 1.0;
        BOOL haveSMinSampleValue = FALSE, haveSMaxSampleValue = FALSE;
        STRPTR name = NULL;
        BOOL isTiled = FALSE, useYCbCr = TRUE, useFallback = FALSE;
        ULONG buffersize = 0, bufsize = 0, x = 0, y = 0;
        UBYTE *buf = NULL, *tmp_buf = NULL;
        IPTR pformat = 0;

        D(bug("[tiff.datatype] %s: tif @  0x%p\n", __func__, tif));

        if(TIFFGetField(tif, TIFFTAG_COMPRESSION, &compression)) {
            if(compression == COMPRESSION_JPEG) {
                TIFFSetField(tif, TIFFTAG_JPEGCOLORMODE, JPEGCOLORMODE_RGB);
                D(bug("[tiff.datatype] %s: JPEG compression detected - using RGB mode\n", __func__));
                useYCbCr = FALSE;
            } else {
                D(bug("[tiff.datatype] %s: TIFF uses compression type: %u (not JPEG)\n", __func__, compression));
            }
        } else {
            D(bug("[tiff.datatype] %s: Could not read compression tag.\n", __func__));
        }

        TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &BitsPerSample);
        if((samplesize = BitsPerSample) < 8)
            samplesize = 8;
        TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP, &RowsPerStrip);
        TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLESPERPIXEL, &SamplesPerPixel);
        TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &imageWidth);
        TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &imageLength);
        TIFFGetFieldDefaulted(tif, TIFFTAG_PLANARCONFIG, &planar);
        TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLEFORMAT, &sampleFormat);
        if(sampleFormat == SAMPLEFORMAT_IEEEFP) {
            haveSMinSampleValue = (TIFFGetField(tif, TIFFTAG_SMINSAMPLEVALUE, &SMinSampleValue) != 0);
            haveSMaxSampleValue = (TIFFGetField(tif, TIFFTAG_SMAXSAMPLEVALUE, &SMaxSampleValue) != 0);
        }
        TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &PhotometricInterpretation);

        if(PhotometricInterpretation == PHOTOMETRIC_YCBCR) {
            TIFFGetFieldDefaulted(tif, TIFFTAG_YCBCRSUBSAMPLING, &ycbcrSubSamplingH, &ycbcrSubSamplingV);
            if(useYCbCr && (ycbcrSubSamplingH != 1 || ycbcrSubSamplingV != 1)) {
                D(bug("[tiff.datatype] %s: YCbCr subsampling %ux%u not supported - using RGBA fallback\n",
                      __func__, ycbcrSubSamplingH, ycbcrSubSamplingV));
                useFallback = TRUE;
            }
        }

        if(!(BitsPerSample == 1 || BitsPerSample == 2 || BitsPerSample == 4 || BitsPerSample == 8 || BitsPerSample == 16
                || BitsPerSample == 32 || BitsPerSample == 64))
            useFallback = TRUE;
        if(SamplesPerPixel > 4)
            useFallback = TRUE;
        if(SamplesPerPixel > 1 && BitsPerSample < 8)
            useFallback = TRUE;
        if(PhotometricInterpretation == PHOTOMETRIC_SEPARATED ||
                PhotometricInterpretation == PHOTOMETRIC_CIELAB ||
                PhotometricInterpretation == PHOTOMETRIC_ICCLAB ||
                PhotometricInterpretation == PHOTOMETRIC_ITULAB ||
                PhotometricInterpretation == PHOTOMETRIC_LOGL ||
                PhotometricInterpretation == PHOTOMETRIC_LOGLUV)
            useFallback = TRUE;
        if(PhotometricInterpretation == PHOTOMETRIC_YCBCR && BitsPerSample != 8)
            useFallback = TRUE;
        if(PhotometricInterpretation == PHOTOMETRIC_PALETTE && BitsPerSample > 8)
            useFallback = TRUE;
        if(sampleFormat != SAMPLEFORMAT_UINT) {
            if(!(sampleFormat == SAMPLEFORMAT_IEEEFP && BitsPerSample == 64))
                useFallback = TRUE;
        }
        if(planar == PLANARCONFIG_SEPARATE && (BitsPerSample < 8))
            useFallback = TRUE;

        bmhd->bmh_Width  = bmhd->bmh_PageWidth  = imageWidth;
        bmhd->bmh_Height = bmhd->bmh_PageHeight = imageLength;
        bmhd->bmh_Depth = samplesize * SamplesPerPixel;
        if(bmhd->bmh_Depth > 32)
            bmhd->bmh_Depth = 32;
        if(useFallback)
            bmhd->bmh_Depth = 32;

        D(
            bug("[tiff.datatype] %s: %ux%ux%u (%ux%x)\n", __func__, imageWidth, imageLength, bmhd->bmh_Depth, BitsPerSample,
                SamplesPerPixel);
            bug("[tiff.datatype] %s: PhotometricInterpretation %04x\n", __func__, PhotometricInterpretation);
        )

        /* Pass picture size to picture.datatype */
        GetDTAttrs(o, DTA_Name, (IPTR)&name, TAG_DONE);
        SetDTAttrs(o, NULL, NULL, DTA_NominalHoriz, imageWidth,
                   DTA_NominalVert, imageLength,
                   DTA_ObjName, (IPTR)name,
                   TAG_DONE);

        if(useFallback) {
            BOOL ok = tiffLoadRGBAFallback(cl, o, tif, imageWidth, imageLength);
            TIFFClose(tif);
            return ok;
        }

        if(!TIFFIsTiled(tif)) {
            tileWidth = imageWidth;
            tileLength = 1;
            buffersize = TIFFScanlineSize(tif);
        } else {
            TIFFGetField(tif, TIFFTAG_TILEWIDTH, &tileWidth);
            TIFFGetField(tif, TIFFTAG_TILELENGTH, &tileLength);
            buffersize = TIFFTileSize(tif);
            isTiled = TRUE;
        }

        D(bug("[tiff.datatype] %s: Allocating %u bytes\n", __func__, buffersize * ((SamplesPerPixel * samplesize) / 8)));
        buf = AllocVec(buffersize * ((SamplesPerPixel * samplesize) / 8), MEMF_ANY);
        if(!buf) {
            TIFFClose(tif);
            return FALSE;
        }
        D(bug("[tiff.datatype] %s: buf @ 0x%p\n", __func__, buf));

        if(BitsPerSample <= 8) {
            UBYTE *plnrbuf = NULL, *ycbcrbuf = NULL;
            BOOL done = FALSE;

            if(SamplesPerPixel < 3) {
                if(PhotometricInterpretation < PHOTOMETRIC_RGB) {
                    if(SamplesPerPixel == 2) {
                        tmp_buf = AllocVec(tileWidth * tileLength * 4, 0);
                        pformat = PBPAFMT_RGBA;
                        D(bug("[tiff.datatype] %s[8BPS]: Greyscale + Alpha image\n", __func__));
                    } else {
                        tmp_buf = AllocVec(tileWidth * tileLength * 3, 0);
                        pformat = PBPAFMT_RGB;
                        D(
                            if(BitsPerSample == 1)
                            bug("[tiff.datatype] %s[8BPS]: Black & White image\n", __func__);
                            else
                                bug("[tiff.datatype] %s[8BPS]: %ubit Greyscale image\n", __func__, BitsPerSample);
                            )
                            }
                } else if(PhotometricInterpretation == PHOTOMETRIC_PALETTE) {
                    UWORD                   *red_colormap;
                    UWORD                   *green_colormap;
                    UWORD                   *blue_colormap;

                    D(bug("[tiff.datatype] %s[8BPS]: %ubit Palette Mapped\n", __func__, BitsPerSample));

                    if((BitsPerSample < 8) || (SamplesPerPixel > 1))
                        tmp_buf = AllocVec(tileWidth * tileLength, 0);

                    if(TIFFGetField(tif, TIFFTAG_COLORMAP, &red_colormap, &green_colormap, &blue_colormap)) {
                        struct ColorRegister    *colorregs = 0;
                        ULONG                   *cregs = 0;
                        SetDTAttrs(o, NULL, NULL, PDTA_NumColors, 1 << BitsPerSample, TAG_DONE);
                        if(GetDTAttrs(o, PDTA_ColorRegisters, (IPTR) &colorregs,
                                      PDTA_CRegs, (IPTR) &cregs,
                                      TAG_DONE) == 2) {
                            for(int i = 0; i < (1 << BitsPerSample); i++) {
                                colorregs->red   = red_colormap[i] >> 8;
                                colorregs->green = green_colormap[i] >> 8;
                                colorregs->blue  = blue_colormap[i] >> 8;

                                *cregs++ = ((ULONG)colorregs->red)   * 0x01010101;
                                *cregs++ = ((ULONG)colorregs->green) * 0x01010101;
                                *cregs++ = ((ULONG)colorregs->blue)  * 0x01010101;

                                colorregs++;
                            }
                            D(bug("[tiff.datatype] %s[8BPS]: read %u palette entries\n", __func__, 1 << BitsPerSample));
                        } /* if (GetDTAttrs(o, ... */
                    }
                    pformat = PBPAFMT_LUT8;
                }
            } else if(SamplesPerPixel == 3)
                pformat = PBPAFMT_RGB;
            else if(SamplesPerPixel == 4)
                pformat = PBPAFMT_RGBA;
            else {
                D(bug("[tiff.datatype] %s[8BPS]: unhandled SamplesPerPixel (%u)\n", __func__, SamplesPerPixel));
            }

            if(planar == PLANARCONFIG_SEPARATE) {
                D(bug("[tiff.datatype] %s[8BPS]: data stored in planes\n", __func__));
                plnrbuf = AllocVec(buffersize * SamplesPerPixel, MEMF_ANY);
                D(bug("[tiff.datatype] %s[8BPS]: allocated conversion buffer (%u x %u x %u = %ubytes) @ 0x%p\n", __func__, tileWidth,
                      tileLength, SamplesPerPixel, buffersize * SamplesPerPixel, plnrbuf));
            } else if((PhotometricInterpretation == PHOTOMETRIC_YCBCR) && (useYCbCr)) {
                D(bug("[tiff.datatype] %s[8BPS]: YCBCR data\n", __func__));
                ycbcrbuf = AllocVec(buffersize, MEMF_ANY);
                D(bug("[tiff.datatype] %s[8BPS]: allocated conversion buffer (%ubytes) @ 0x%p\n", __func__,
                      buffersize, ycbcrbuf));
            }

            for(y = 0; !done && y < bmhd->bmh_Height; y += tileLength) {
                for(x = 0; !done && x < bmhd->bmh_Width; x += tileWidth) {
                    ULONG copyWidth  = MIN(tileWidth,  bmhd->bmh_Width  - x);
                    ULONG copyHeight = MIN(tileLength, bmhd->bmh_Height - y);

                    if(isTiled) {
                        D(bug("[tiff.datatype] %s[8BPS]: Tiled read %ux%u @ %u,%u...\n", __func__,
                              tileWidth, tileLength, x, y));
                        if(!(plnrbuf)) {
                            if(TIFFReadTile(tif, (ycbcrbuf) ? ycbcrbuf : buf, x, y, 0, 0) < 0) {
                                done = TRUE;
                                break;
                            }
                            if(ycbcrbuf) {
                                tiffConvertYCbCr(pformat, tileWidth, tileLength, ycbcrbuf, buf);
                            }
                        } else {
                            ULONG planeSize = tileWidth * tileLength;
                            int plane, planemax = SamplesPerPixel;
                            for(plane = 0; plane < planemax; ++plane) {
                                if(TIFFReadTile(tif, plnrbuf + plane * planeSize, x, y, 0, plane) < 0) {
                                    done = TRUE;
                                    break;
                                }
                            }
                        }
                    } else {
                        D(bug("[tiff.datatype] %s[8BPS]: Scanline read line %u...\n", __func__, y));
                        copyWidth  = bmhd->bmh_Width;
                        copyHeight = 1;
                        if(!(plnrbuf)) {
                            if(TIFFReadScanline(tif, (ycbcrbuf) ? ycbcrbuf : buf, y, 0) != 1) {
                                done = TRUE;
                                break;
                            }
                            if(ycbcrbuf) {
                                tiffConvertYCbCr(pformat, copyWidth, copyHeight, ycbcrbuf, buf);
                            }
                        } else {
                            ULONG planeSize = copyWidth * copyHeight;
                            int plane, planemax = SamplesPerPixel;
                            for(plane = 0; plane < planemax; ++plane) {
                                if(TIFFReadScanline(tif, plnrbuf + plane * planeSize, y, plane) != 1) {
                                    done = TRUE;
                                    break;
                                }
                            }
                        }
                    }
                    if(plnrbuf) {
                        ULONG pixels = isTiled ? (tileWidth * tileLength) : (copyWidth * copyHeight);
                        tiffInterleavePlanarSeparate(buf, plnrbuf, buffersize, SamplesPerPixel, 1, pixels);
                    }

                    if(done) break;

                    UBYTE *pixelDataPtr = NULL;
                    ULONG bytesPerPixel;


                    if(tmp_buf) {
                        /* Expand/convert into tmp_buf.
                           tmp_buf uses a fixed stride of tileWidth pixels per row (tileLength rows). */
                        if(PhotometricInterpretation == PHOTOMETRIC_PALETTE) {
                            bytesPerPixel = 1;

                            if(BitsPerSample == 8) {
                                for(ULONG row = 0; row < copyHeight; ++row) {
                                    CopyMem(buf + row * tileWidth, tmp_buf + row * tileWidth, copyWidth);
                                }
                            } else {
                                UBYTE mask = (1 << BitsPerSample) - 1;
                                ULONG packedRowBytes = (tileWidth * BitsPerSample + 7) / 8;

                                for(ULONG row = 0; row < copyHeight; ++row) {
                                    const UBYTE *srcRow = buf + row * packedRowBytes;
                                    UBYTE *dstRow = tmp_buf + row * tileWidth;

                                    for(ULONG col = 0; col < copyWidth; ++col) {
                                        ULONG bit_offset = col * BitsPerSample;
                                        ULONG byte_index = bit_offset >> 3;
                                        ULONG bit_in_byte = bit_offset & 7;
                                        UBYTE shift = (UBYTE)(8 - BitsPerSample - bit_in_byte);
                                        dstRow[col] = (srcRow[byte_index] >> shift) & mask;
                                    }
                                }
                            }
                        } else {
                            BOOL hasAlpha = (SamplesPerPixel == 2);
                            bytesPerPixel = hasAlpha ? 4 : 3;

                            if(BitsPerSample == 8) {
                                for(ULONG row = 0; row < copyHeight; ++row) {
                                    const UBYTE *srcRow = buf + row * tileWidth * SamplesPerPixel;

                                    for(ULONG col = 0; col < copyWidth; ++col) {
                                        UBYTE pixval = srcRow[col * SamplesPerPixel];
                                        if(PhotometricInterpretation == PHOTOMETRIC_MINISWHITE)
                                            pixval = 255 - pixval;

                                        if(hasAlpha) {
                                            UBYTE a = srcRow[col * SamplesPerPixel + 1];
                                            tmp_buf[(row * tileWidth + col) * 4 + 0] = pixval;
                                            tmp_buf[(row * tileWidth + col) * 4 + 1] = pixval;
                                            tmp_buf[(row * tileWidth + col) * 4 + 2] = pixval;
                                            tmp_buf[(row * tileWidth + col) * 4 + 3] = a;
                                        } else {
                                            tmp_buf[(row * tileWidth + col) * 3 + 0] = pixval;
                                            tmp_buf[(row * tileWidth + col) * 3 + 1] = pixval;
                                            tmp_buf[(row * tileWidth + col) * 3 + 2] = pixval;
                                        }
                                    }
                                }
                            } else {
                                /* Packed 1/2/4-bpp grayscale. */
                                UBYTE mask = (1 << BitsPerSample) - 1;
                                ULONG packedRowBytes = (tileWidth * BitsPerSample + 7) / 8;

                                for(ULONG row = 0; row < copyHeight; ++row) {
                                    const UBYTE *srcRow = buf + row * packedRowBytes;

                                    for(ULONG col = 0; col < copyWidth; ++col) {
                                        ULONG bit_offset = col * BitsPerSample;
                                        ULONG byte_index = bit_offset >> 3;
                                        ULONG bit_in_byte = bit_offset & 7;
                                        UBYTE shift = (UBYTE)(8 - BitsPerSample - bit_in_byte);
                                        UBYTE v = (srcRow[byte_index] >> shift) & mask;
                                        v = (UBYTE)((v * 255) / mask);

                                        if(PhotometricInterpretation == PHOTOMETRIC_MINISWHITE)
                                            v = 255 - v;

                                        tmp_buf[(row * tileWidth + col) * 3 + 0] = v;
                                        tmp_buf[(row * tileWidth + col) * 3 + 1] = v;
                                        tmp_buf[(row * tileWidth + col) * 3 + 2] = v;
                                    }
                                }
                            }
                        }

                        pixelDataPtr = tmp_buf;
                    } else {
                        bytesPerPixel = SamplesPerPixel;
                        pixelDataPtr = buf;
                    }

                    ULONG srcRowBytes;
                    if(isTiled)
                        srcRowBytes = tileWidth * bytesPerPixel;
                    else
                        srcRowBytes = copyWidth * bytesPerPixel;

                    D(bug("[tiff.datatype] %s[8BPS]: rendering %ux%u @ %u,%u, srcRowBytes=%u, bpp=%u\n", __func__,
                          copyWidth, copyHeight, x, y, srcRowBytes, bytesPerPixel));

                    if(!DoSuperMethod(cl, o,
                                      PDTM_WRITEPIXELARRAY,
                                      (IPTR) pixelDataPtr,
                                      pformat,
                                      srcRowBytes,
                                      x,
                                      y,
                                      copyWidth,
                                      copyHeight)) {
                        D(bug("[tiff.datatype] %s[8BPS]: DT object failed to render\n", __func__));
                        done = TRUE;
                        break;
                    }
                }
            }
            if(ycbcrbuf)
                FreeVec(ycbcrbuf);
            if(plnrbuf)
                FreeVec(plnrbuf);
        } else if(BitsPerSample == 16) {
            BOOL done = FALSE;
            UBYTE *plnrbuf = NULL;

            if(planar == PLANARCONFIG_SEPARATE) {
                plnrbuf = AllocVec(buffersize * SamplesPerPixel, MEMF_ANY);
                if(!plnrbuf) {
                    D(bug("[tiff.datatype] %s[16BPS]: failed to allocate planar buffer\n", __func__));
                    FreeVec(tmp_buf);
                    FreeVec(buf);
                    TIFFClose(tif);
                    return FALSE;
                }
            }

            if(SamplesPerPixel == 4) {
                tmp_buf = AllocVec(tileWidth * tileLength * 4, MEMF_ANY);
                pformat = PBPAFMT_RGBA;
            } else if(SamplesPerPixel == 2 && PhotometricInterpretation < PHOTOMETRIC_RGB) {
                tmp_buf = AllocVec(tileWidth * tileLength * 4, MEMF_ANY);
                pformat = PBPAFMT_RGBA;
                D(bug("[tiff.datatype] %s[16BPS]: Greyscale + Alpha image\n", __func__));
            } else if(SamplesPerPixel < 4) {
                tmp_buf = AllocVec(tileWidth * tileLength * 3, MEMF_ANY);
                pformat = PBPAFMT_RGB;
                if(PhotometricInterpretation < PHOTOMETRIC_RGB) {
                    D(bug("[tiff.datatype] %s[16BPS]: %ubit Greyscale image\n", __func__, BitsPerSample);)
                }
            } else {
                D(bug("[tiff.datatype] %s[16BPS]: unhandled SamplesPerPixel\n", __func__));
            }

            for(y = 0; !done && y < bmhd->bmh_Height; y += tileLength) {
                for(x = 0; !done && x < bmhd->bmh_Width; x += tileWidth) {
                    if(isTiled) {
                        D(bug("[tiff.datatype] %s[16BPS]: Tiled read %ux%u @ %u,%u...\n", __func__, tileWidth, tileLength, x, y));
                        if(planar == PLANARCONFIG_SEPARATE) {
                            for(int plane = 0; plane < SamplesPerPixel; ++plane) {
                                if(TIFFReadTile(tif, plnrbuf + (ULONG)plane * buffersize, x, y, 0, plane) < 0) {
                                    done = TRUE;
                                    break;
                                }
                            }
                            if(!done) {
                                tiffInterleavePlanarSeparate(buf, plnrbuf, buffersize, SamplesPerPixel, 2, tileWidth * tileLength);
                            }
                        } else {
                            if(TIFFReadTile(tif, buf, x, y, 0, 0) < 0) {
                                done = TRUE;
                                break;
                            }
                        }
                    } else {
                        D(bug("[tiff.datatype] %s[16BPS]: Scanline read...\n", __func__));
                        if(planar == PLANARCONFIG_SEPARATE) {
                            for(int plane = 0; plane < SamplesPerPixel; ++plane) {
                                if(TIFFReadScanline(tif, plnrbuf + (ULONG)plane * buffersize, y, plane) != 1) {
                                    done = TRUE;
                                    break;
                                }
                            }
                            if(!done) {
                                tiffInterleavePlanarSeparate(buf, plnrbuf, buffersize, SamplesPerPixel, 2, bmhd->bmh_Width);
                            }
                        } else {
                            if(TIFFReadScanline(tif, buf, y, 0) != 1) {
                                done = TRUE;
                                break;
                            }
                        }
                    }

                    if(!done) {
                        if(tmp_buf) {
                            D(bug("[tiff.datatype] %s[16BPS]: calling tiffConvert16to8\n", __func__));
                            tiffConvert16to8(PhotometricInterpretation, SamplesPerPixel, pformat, tileWidth, tileLength, buf, tmp_buf);
                        }
                        D(bug("[tiff.datatype] %s[16BPS]: rendering to datatype obj...\n", __func__));
                        ULONG rowBytes = tmp_buf ? tiffRowBytes(tileWidth, pformat)
                                         : (tileWidth * SamplesPerPixel * (BitsPerSample / 8));
                        if(!DoSuperMethod(cl, o,
                                          PDTM_WRITEPIXELARRAY,
                                          (IPTR)(tmp_buf ? tmp_buf : buf),
                                          pformat,
                                          rowBytes,
                                          x,
                                          y,
                                          MIN(tileWidth, bmhd->bmh_Width - x),
                                          MIN(tileLength, bmhd->bmh_Height - y))) {
                            D(bug("[tiff.datatype] %s[16BPS]: DT object failed to render\n", __func__));
                            done = TRUE;
                            break;
                        }
                    }
                }
            }
            if(plnrbuf)
                FreeVec(plnrbuf);
        } else if(BitsPerSample == 32) {
            BOOL done = FALSE;
            UBYTE *plnrbuf = NULL;

            if(planar == PLANARCONFIG_SEPARATE) {
                plnrbuf = AllocVec(buffersize * SamplesPerPixel, MEMF_ANY);
                if(!plnrbuf) {
                    D(bug("[tiff.datatype] %s[32BPS]: failed to allocate planar buffer\n", __func__));
                    FreeVec(tmp_buf);
                    FreeVec(buf);
                    TIFFClose(tif);
                    return FALSE;
                }
            }

            if(SamplesPerPixel == 4) {
                tmp_buf = AllocVec(tileWidth * tileLength * 4, 0);
                pformat = PBPAFMT_RGBA;
            } else if(SamplesPerPixel == 2 && PhotometricInterpretation < PHOTOMETRIC_RGB) {
                tmp_buf = AllocVec(tileWidth * tileLength * 4, 0);
                pformat = PBPAFMT_RGBA;
                D(bug("[tiff.datatype] %s[32BPS]: Greyscale + Alpha image\n", __func__));
            } else if(SamplesPerPixel < 4) {
                tmp_buf = AllocVec(tileWidth * tileLength * 3, 0);
                pformat = PBPAFMT_RGB;
                if(PhotometricInterpretation < PHOTOMETRIC_RGB) {
                    D(bug("[tiff.datatype] %s[32BPS]: %ubit Greyscale image\n", __func__, BitsPerSample);)
                }
            } else {
                D(bug("[tiff.datatype] %s[32BPS]: unhandled SamplesPerPixel\n", __func__));
            }

            for(y = 0; !done && y < bmhd->bmh_Height;) {
                APTR PixelData;
                ULONG PixelArrayMod;

                if(isTiled) {
                    for(x = 0; !done && x < bmhd->bmh_Width; x += tileWidth) {
                        D(bug("[tiff.datatype] %s[32BPS]: Tiled read %ux%u @ %u,%u...\n", __func__,
                              tileWidth, tileLength, x, y));

                        if(planar == PLANARCONFIG_SEPARATE) {
                            for(int plane = 0; plane < SamplesPerPixel; ++plane) {
                                if(TIFFReadTile(tif, plnrbuf + (ULONG)plane * buffersize, x, y, 0, plane) < 0) {
                                    done = TRUE;
                                    break;
                                }
                            }
                            if(!done) {
                                tiffInterleavePlanarSeparate(buf, plnrbuf, buffersize, SamplesPerPixel, 4, tileWidth * tileLength);
                            }
                        } else {
                            if(TIFFReadTile(tif, buf, x, y, 0, 0) < 0) {
                                done = TRUE;
                                break;
                            }
                        }

                        if(!done) {
                            UWORD copyWidth  = MIN(tileWidth,  bmhd->bmh_Width  - x);
                            UWORD copyHeight = MIN(tileLength, bmhd->bmh_Height - y);

                            if(tmp_buf) {
                                tiffConvert32to8(PhotometricInterpretation, SamplesPerPixel,
                                                 pformat, tileWidth, tileLength, buf, tmp_buf);

                                PixelArrayMod = tiffRowBytes(tileWidth, pformat);
                                PixelData     = tmp_buf;
                            } else {
                                PixelArrayMod = tileWidth * SamplesPerPixel * (BitsPerSample / 8);
                                PixelData     = buf;
                            }

                            if(!DoSuperMethod(cl, o,
                                              PDTM_WRITEPIXELARRAY,
                                              (IPTR)PixelData,
                                              pformat,
                                              PixelArrayMod,
                                              x, y,
                                              copyWidth, copyHeight)) {
                                D(bug("[tiff.datatype] %s[32BPS]: DT object failed to render\n", __func__));
                                done = TRUE;
                                break;
                            }
                        }
                    }
                    y += tileLength;  // next tile row
                } else {
                    D(bug("[tiff.datatype] %s[32BPS]: Scanline read line %u...\n", __func__, y));

                    if(planar == PLANARCONFIG_SEPARATE) {
                        for(int plane = 0; plane < SamplesPerPixel; ++plane) {
                            if(TIFFReadScanline(tif, plnrbuf + (ULONG)plane * buffersize, y, plane) != 1) {
                                done = TRUE;
                                break;
                            }
                        }
                        if(!done) {
                            tiffInterleavePlanarSeparate(buf, plnrbuf, buffersize, SamplesPerPixel, 4, bmhd->bmh_Width);
                        }
                    } else {
                        if(TIFFReadScanline(tif, buf, y, 0) != 1) {
                            done = TRUE;
                            break;
                        }
                    }

                    if(tmp_buf) {
                        tiffConvert32to8(PhotometricInterpretation, SamplesPerPixel,
                                         pformat, bmhd->bmh_Width, 1, buf, tmp_buf);

                        PixelArrayMod = tiffRowBytes(bmhd->bmh_Width, pformat);
                        PixelData     = tmp_buf;
                    } else {
                        PixelArrayMod = bmhd->bmh_Width * SamplesPerPixel * (BitsPerSample / 8);
                        PixelData     = buf;
                    }

                    if(!DoSuperMethod(cl, o,
                                      PDTM_WRITEPIXELARRAY,
                                      (IPTR)PixelData,
                                      pformat,
                                      PixelArrayMod,
                                      0, y,
                                      bmhd->bmh_Width, 1)) {
                        D(bug("[tiff.datatype] %s[32BPS]: DT object failed to render\n", __func__));
                        done = TRUE;
                        break;
                    }

                    y += 1; // next scanline
                }
            }
            if(plnrbuf)
                FreeVec(plnrbuf);

        } else if(BitsPerSample == 64) {
            BOOL done = FALSE;
            UBYTE *plnrbuf = NULL;

            /* 64-bit input always gets downsampled to 8-bit output */
            if(SamplesPerPixel == 4) {
                tmp_buf = AllocVec(tileWidth * tileLength * 4, MEMF_ANY);
                pformat = PBPAFMT_RGBA;
            } else if(SamplesPerPixel == 2 && PhotometricInterpretation < PHOTOMETRIC_RGB) {
                tmp_buf = AllocVec(tileWidth * tileLength * 4, MEMF_ANY);
                pformat = PBPAFMT_RGBA;
                D(bug("[tiff.datatype] %s[64BPS]: Greyscale + Alpha image\n", __func__));
            } else if(SamplesPerPixel < 4) {
                tmp_buf = AllocVec(tileWidth * tileLength * 3, MEMF_ANY);
                pformat = PBPAFMT_RGB;
                if(PhotometricInterpretation < PHOTOMETRIC_RGB) {
                    D(bug("[tiff.datatype] %s[64BPS]: 64-bit Greyscale image\n", __func__);)
                }
            } else {
                D(bug("[tiff.datatype] %s[64BPS]: unhandled SamplesPerPixel\n", __func__));
            }

            if(!tmp_buf) {
                D(bug("[tiff.datatype] %s[64BPS]: failed to allocate conversion buffer\n", __func__));
                TIFFClose(tif);
                FreeVec(buf);
                return FALSE;
            }

            if(planar == PLANARCONFIG_SEPARATE) {
                plnrbuf = AllocVec(buffersize * SamplesPerPixel, MEMF_ANY);
                if(!plnrbuf) {
                    D(bug("[tiff.datatype] %s[64BPS]: failed to allocate planar buffer\n", __func__));
                    FreeVec(tmp_buf);
                    FreeVec(buf);
                    TIFFClose(tif);
                    return FALSE;
                }
            }

            for(y = 0; !done && y < bmhd->bmh_Height; y += tileLength) {
                for(x = 0; !done && x < bmhd->bmh_Width; x += tileWidth) {
                    if(isTiled) {
                        D(bug("[tiff.datatype] %s[64BPS]: Tiled read %ux%u @ %u,%u...\n", __func__, tileWidth, tileLength, x, y));
                        if(planar == PLANARCONFIG_SEPARATE) {
                            for(int plane = 0; plane < SamplesPerPixel; ++plane) {
                                if(TIFFReadTile(tif, plnrbuf + (ULONG)plane * buffersize, x, y, 0, plane) < 0) {
                                    done = TRUE;
                                    break;
                                }
                            }
                            if(!done) {
                                tiffInterleavePlanarSeparate(buf, plnrbuf, buffersize, SamplesPerPixel, 8, tileWidth * tileLength);
                            }
                        } else {
                            if(TIFFReadTile(tif, buf, x, y, 0, 0) < 0) {
                                done = TRUE;
                                break;
                            }
                        }
                    } else {
                        D(bug("[tiff.datatype] %s[64BPS]: Scanline read...\n", __func__));
                        if(planar == PLANARCONFIG_SEPARATE) {
                            for(int plane = 0; plane < SamplesPerPixel; ++plane) {
                                if(TIFFReadScanline(tif, plnrbuf + (ULONG)plane * buffersize, y, plane) != 1) {
                                    done = TRUE;
                                    break;
                                }
                            }
                            if(!done) {
                                tiffInterleavePlanarSeparate(buf, plnrbuf, buffersize, SamplesPerPixel, 8, bmhd->bmh_Width);
                            }
                        } else {
                            if(TIFFReadScanline(tif, buf, y, 0) != 1) {
                                done = TRUE;
                                break;
                            }
                        }
                    }

                    if(!done) {
                        if(sampleFormat == SAMPLEFORMAT_UINT) {
                            tiffConvert64uTo8(PhotometricInterpretation, SamplesPerPixel, pformat,
                                              tileWidth, tileLength, buf, tmp_buf);
                        } else if(sampleFormat == SAMPLEFORMAT_IEEEFP) {
                            BOOL haveRange = (haveSMinSampleValue && haveSMaxSampleValue);
                            tiffConvert64fTo8(PhotometricInterpretation, SamplesPerPixel, pformat,
                                              tileWidth, tileLength, buf, tmp_buf,
                                              haveRange, SMinSampleValue, SMaxSampleValue);
                        } else {
                            D(bug("[tiff.datatype] %s[64BPS]: unsupported SampleFormat %u\n", __func__, sampleFormat));
                            done = TRUE;
                            break;
                        }

                        ULONG rowBytes = tiffRowBytes(tileWidth, pformat);

                        if(!DoSuperMethod(cl, o,
                                          PDTM_WRITEPIXELARRAY,
                                          (IPTR)tmp_buf,
                                          pformat,
                                          rowBytes,
                                          x,
                                          y,
                                          MIN(tileWidth, bmhd->bmh_Width - x),
                                          MIN(tileLength, bmhd->bmh_Height - y))) {
                            D(bug("[tiff.datatype] %s[64BPS]: DT object failed to render\n", __func__));
                            done = TRUE;
                            break;
                        }
                    }
                }
            }

            if(plnrbuf)
                FreeVec(plnrbuf);
        }

        D(bug("[tiff.datatype] %s: done, cleaning up...\n", __func__));

        FreeVec(tmp_buf);
        FreeVec(buf);

        TIFFClose(tif);

        return TRUE;
    }
    D(bug("[tiff.datatype] %s: failed to open tif\n", __func__));

    return FALSE;
}

/**************************************************************************************************/

/**************************************************************************************************/

static BOOL SaveTIFF(struct IClass *cl, Object *o, struct dtWrite *dtw)
{
    D(bug("[tiff.datatype] %s()\n", __func__));

    return TRUE;
}

/**************************************************************************************************/

IPTR TIFF__OM_NEW(Class *cl, Object *o, Msg msg)
{
    Object *newobj;

    D(bug("[tiff.datatype] %s()\n", __func__));

    newobj = (Object *)DoSuperMethodA(cl, o, (Msg)msg);
    if(newobj) {
        if(!LoadTIFF(cl, newobj)) {
            CoerceMethod(cl, newobj, OM_DISPOSE);
            newobj = NULL;
        }
    }

    return (IPTR)newobj;
}

/**************************************************************************************************/

IPTR TIFF__DTM_WRITE(Class *cl, Object *o, struct dtWrite *dtw)
{
    D(bug("[tiff.datatype] %s()\n", __func__));
    if((dtw -> dtw_Mode) == DTWM_RAW) {
        /* Local data format requested */
        return SaveTIFF(cl, o, dtw);
    } else {
        /* Pass msg to superclass (which writes an IFF ILBM picture)... */
        return DoSuperMethodA(cl, o, (Msg)dtw);
    }
}

/**************************************************************************************************/
