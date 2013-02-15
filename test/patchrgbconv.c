#define __OOP_NOATTRBASES__

#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/oop.h>
#include <dos/rdargs.h>
#include <hidd/graphics.h>
#include <aros/macros.h>
#include <aros/debug.h>

#include <stdio.h>
#include <stdlib.h>

static BOOL noisy;

#include "patchrgbconv_macros.h"
#include "patchrgbconv_rgbswap.h"
#include "patchrgbconv_argb32.h"
#include "patchrgbconv_bgra32.h"
#include "patchrgbconv_rgba32.h"
#include "patchrgbconv_abgr32.h"
#include "patchrgbconv_rgb24.h"
#include "patchrgbconv_bgr24.h"
#include "patchrgbconv_rgb15.h"
#include "patchrgbconv_bgr15.h"
#include "patchrgbconv_rgb16.h"
#include "patchrgbconv_bgr16.h"
#include "patchrgbconv_rgb16oe.h"
#include "patchrgbconv_bgr16oe.h"
#include "patchrgbconv_rgb15oe.h"
#include "patchrgbconv_bgr15oe.h"
#include "patchrgbconv_xrgb32.h"
#include "patchrgbconv_bgrx32.h"
#include "patchrgbconv_rgbx32.h"
#include "patchrgbconv_xbgr32.h"

#include "patchrgbconv_verify.h"

#define VERIFY_SIZE 100

#define ARG_TEMPLATE "VERIFY=V/S,BENCH=B/N/K,NOISY=N/S"
#define ARG_VERIFY 0
#define ARG_BENCH 1
#define ARG_NOISY 2
#define ARG_NUM 3

#undef ConvertPixels

static OOP_AttrBase HiddBitMapAttrBase;

static IPTR args[ARG_NUM];

static ULONG dstbuf_orig[VERIFY_SIZE * 4 + 10];
static ULONG dstbuf_new[VERIFY_SIZE * 4 + 10];
static UBYTE *benchmem;

static void dumpmem(UBYTE *buf, ULONG num, ULONG size, STRPTR info)
{
    int i, b;
    
    bug("  %s", info);
    
    for(i = 0; i < num; i++)
    {
    	if ((i % (32 / size)) == 0)
	{
	    bug("\n   ");
	}
	
    	for(b = 0; b < size; b++)
	{
	#if AROS_BIG_ENDAIN
	    bug("%02x", buf[b]);
	#else
	    bug("%02x", buf[size - 1 -b]);
    	#endif
	}
	buf += size;
	bug(" ");
    }
    bug("\n\n");
}

void ConvertPixels(APTR srcPixels, ULONG srcMod, HIDDT_StdPixFmt srcPixFmt,
		   APTR dstPixels, ULONG dstMod, HIDDT_StdPixFmt dstPixFmt,
		   ULONG width, ULONG height, OOP_Object *bm)
{
    OOP_Object *gfxhidd = NULL;
    OOP_Object *srcpf, *dstpf;
    APTR src = srcPixels;
    APTR dst = dstPixels;

    OOP_GetAttr(bm, aHidd_BitMap_GfxHidd, (IPTR *)&gfxhidd);

    if (!gfxhidd) {
        bug("ConvertPixels(): Failed to obtain graphics driver\n");
	return;
    }

    srcpf = HIDD_Gfx_GetPixFmt(gfxhidd, srcPixFmt);
    dstpf = HIDD_Gfx_GetPixFmt(gfxhidd, dstPixFmt);

    if (!srcpf || !dstpf)
    {
    	bug("ConvertPixels(): Bad source (%d) or dest (%d) pixfmt!\n", srcPixFmt, dstPixFmt);
	return;
    }

    HIDD_BM_ConvertPixels(bm, &src, (HIDDT_PixelFormat *)srcpf, srcMod, 
    	    	    	  &dst, (HIDDT_PixelFormat *)dstpf, dstMod,
			  width, height, NULL);
}

typedef void (*DOFUNC)(ULONG srcfmt, ULONG dstfmt, ULONG srcbits, ULONG dstbits,
    	    	       HIDDT_RGBConversionFunction f, BOOL verify, ULONG bench,
		       APTR testpixels, ULONG numtestpixels, char *srcfmt_string,
		       char *dstfmt_string, OOP_Object *bm);

static void uninstallfunc(ULONG srcfmt, ULONG dstfmt, ULONG srcbits, ULONG dstbits,
    	    	    	  HIDDT_RGBConversionFunction f, BOOL verify, ULONG bench,
			  APTR testpixels, ULONG numtestpixels, char *srcfmt_string,
			  char *dstfmt_string, OOP_Object *bm)
{
    (void)srcbits;
    (void)dstbits;
    (void)(f);
    (void)verify;
    (void)bench;
    (void)testpixels;
    (void)numtestpixels;
    (void)srcfmt_string;
    (void)dstfmt_string;

    HIDD_BM_SetRGBConversionFunction(bm, srcfmt, dstfmt, NULL);
}

			
static void installfunc(ULONG srcfmt, ULONG dstfmt, ULONG srcbits, ULONG dstbits,
    	    	    	HIDDT_RGBConversionFunction f, BOOL verify, ULONG bench,
			APTR testpixels, ULONG numtestpixels, char *srcfmt_string,
			char *dstfmt_string, OOP_Object *bm)
{
    ULONG srcbytes, dstbytes;
    ULONG sec1, sec2, micro1, micro2, time1 = 0, time2;
    
    srcbytes = (srcbits + 7) / 8;
    dstbytes = (dstbits + 7) / 8;
    
    if (verify)
    {
    	ConvertPixels(testpixels, 0, srcfmt,
	    	       dstbuf_orig, numtestpixels * dstbytes, dstfmt,
		       numtestpixels,
		       VERIFY_SIZE / numtestpixels,
		       bm);
	 
    }
    
    if (bench && benchmem)
    {
    	CurrentTime(&sec1, &micro1);
	ConvertPixels(benchmem, 0, srcfmt,
	    	       benchmem, 0, dstfmt,
		       bench, 1,
		       bm);
		       
    	CurrentTime(&sec2, &micro2);
	
	time1 = (sec2 - sec1) * 1000000 + (((LONG)micro2) - ((LONG)micro1));
    }

    HIDD_BM_SetRGBConversionFunction(bm, srcfmt, dstfmt, f);

    if (verify)
    {
    	ConvertPixels(testpixels, 0, srcfmt,
	    	       dstbuf_new, numtestpixels * dstbytes, dstfmt,
		       numtestpixels,
		       VERIFY_SIZE / numtestpixels,
		       bm);
	
	if (memcmp(dstbuf_orig, dstbuf_new, (VERIFY_SIZE / numtestpixels) * (numtestpixels * dstbytes)) != 0)
	{
	    int numtest = (VERIFY_SIZE / numtestpixels) * numtestpixels;
	    
	    bug("  Verify %s to %s (%s to %s) failed!\n",
	    	srcfmt_string, dstfmt_string,
		pf_to_string[srcfmt], pf_to_string[dstfmt]);
		
	    dumpmem((UBYTE *)testpixels, numtestpixels, srcbytes, (STRPTR)"SRC:");
	    dumpmem((UBYTE *)dstbuf_orig, numtest, dstbytes, (STRPTR)"OLD:");
	    dumpmem((UBYTE *)dstbuf_new, numtest, dstbytes, (STRPTR)"NEW:");
	    
	}
	
    }

    if (bench && benchmem)
    {
    	static char sbuf[256];
	
    	CurrentTime(&sec1, &micro1);
	ConvertPixels(benchmem, 0, srcfmt,
	    	       benchmem, 0, dstfmt,
		       bench, 1,
		       bm);
		       
    	CurrentTime(&sec2, &micro2);
	
	time2 = (sec2 - sec1) * 1000000 + (((LONG)micro2) - ((LONG)micro1));
	
	sprintf(sbuf, " Benchmark %s to %s (%s to %s): before %d (%f) after %d (%f) (%d %%)\n", 
	    	      srcfmt_string, dstfmt_string,
		      pf_to_string[srcfmt], pf_to_string[dstfmt],
	    	      (int)time1, time1 / 1000000.0, 
	    	      (int)time2, time2 / 1000000.0,
		      (int)(time2 ? time1 * 100 / time2 : 0));
	bug("%s", sbuf);
    }

}

#define PATCHFUNC(a,b) \
    (*func)(FMT_ ## a, FMT_ ## b, a ## _ ## BITS, b ## _ ## BITS, convert_ ## a ## _ ## b, \
    	    	verify, bench, testpixels_ ## a, NUMTESTPIXELS_ ## a, # a, # b, HIDD_BM_OBJ(bitmap));

int main(int argc, char **argv)
{
    struct RDArgs *myargs;
    BOOL    	   verify = FALSE;
    ULONG   	   bench = 0;
    DOFUNC  	   func = installfunc;
    int     	   i;
    struct BitMap  *bitmap;

    HiddBitMapAttrBase = OOP_ObtainAttrBase(IID_Hidd_BitMap);
    if (!HiddBitMapAttrBase) {
        printf("Failed to obtain IID_Hidd_BitMap\n");
	
	return RETURN_FAIL;
    }

    if ((myargs = ReadArgs((STRPTR)ARG_TEMPLATE, args, NULL)))
    {
    	if (args[ARG_VERIFY]) verify = TRUE;
	if (args[ARG_BENCH]) bench = *(IPTR *)args[ARG_BENCH];
	if (args[ARG_NOISY]) noisy = TRUE;
	
    	FreeArgs(myargs);
    }
    else
    {
    	PrintFault(IoErr(), (STRPTR)argv[0]);
	OOP_ReleaseAttrBase(IID_Hidd_BitMap);
	return RETURN_FAIL;
    }
        
    if (bench)
    {
    	benchmem = malloc(bench * 4);
	if (!benchmem)
	{
	    PrintFault(ERROR_NO_FREE_STORE, (STRPTR)argv[0]);	
    	    OOP_ReleaseAttrBase(IID_Hidd_BitMap);
	    return RETURN_FAIL;
	}
    }

    /* We need a placeholder bitmap object in order to talk to bitmap class */
    bitmap = AllocBitMap(1, 1, 16, 0, NULL);
    if (!bitmap) {
	PrintFault(ERROR_NO_FREE_STORE, (STRPTR)argv[0]);
        OOP_ReleaseAttrBase(IID_Hidd_BitMap);
	return RETURN_FAIL;
    }

    #define P(a,b) PATCHFUNC(a,b)
        
    for(i = 0; i < 2; i++)
    {
	/* ARGB32 to #? */
	P(ARGB32,RGB16)   P(ARGB32,BGR16)   P(ARGB32,RGB15)   P(ARGB32,BGR15)
	P(ARGB32,BGRA32)  P(ARGB32,RGBA32)  P(ARGB32,ABGR32)  P(ARGB32,RGB24)
	P(ARGB32,BGR24)   P(ARGB32,RGB16OE) P(ARGB32,BGR16OE) P(ARGB32,RGB15OE)
	P(ARGB32,BGR15OE) P(ARGB32,XRGB32)  P(ARGB32,BGRX32)  P(ARGB32,RGBX32)
	P(ARGB32,XBGR32)

	/* BGRA32 to #? */
	P(BGRA32,RGB16)   P(BGRA32,BGR16)   P(BGRA32,RGB15)   P(BGRA32,BGR15)
	P(BGRA32,ARGB32)  P(BGRA32,RGBA32)  P(BGRA32,ABGR32)  P(BGRA32,RGB24)
	P(BGRA32,BGR24)   P(BGRA32,RGB16OE) P(BGRA32,BGR16OE) P(BGRA32,RGB15OE)
	P(BGRA32,BGR15OE) P(BGRA32,XRGB32)  P(BGRA32,BGRX32)  P(BGRA32,RGBX32)
	P(BGRA32,XBGR32)

	/* RGBA32 to #? */
	P(RGBA32,RGB16)   P(RGBA32,BGR16)   P(RGBA32,RGB15)   P(RGBA32,BGR15)
	P(RGBA32,BGRA32)  P(RGBA32,ARGB32)  P(RGBA32,ABGR32)  P(RGBA32,RGB24)
	P(RGBA32,BGR24)   P(RGBA32,RGB16OE) P(RGBA32,BGR16OE) P(RGBA32,RGB15OE)
	P(RGBA32,BGR15OE) P(RGBA32,XRGB32)  P(RGBA32,BGRX32)  P(RGBA32,RGBX32)
	P(RGBA32,XBGR32)

	/* ABGR32 to #? */
	P(ABGR32,RGB16)   P(ABGR32,BGR16)   P(ABGR32,RGB15)   P(ABGR32,BGR15)
	P(ABGR32,BGRA32)  P(ABGR32,ARGB32)  P(ABGR32,RGBA32)  P(ABGR32,RGB24)
	P(ABGR32,BGR24)   P(ABGR32,RGB16OE) P(ABGR32,BGR16OE) P(ABGR32,RGB15OE)
	P(ABGR32,BGR15OE) P(ABGR32,XRGB32)  P(ABGR32,BGRX32)  P(ABGR32,RGBX32)
	P(ABGR32,XBGR32)

	/* RGB24 to #? */
	P(RGB24,RGB16)    P(RGB24,BGR16)    P(RGB24,RGB15)    P(RGB24,BGR15)
	P(RGB24,ARGB32)   P(RGB24,BGRA32)   P(RGB24,RGBA32)   P(RGB24,ABGR32)
	P(RGB24,BGR24)    P(RGB24,RGB16OE)  P(RGB24,BGR16OE)  P(RGB24,RGB15OE)
	P(RGB24,BGR15OE)  P(RGB24,XRGB32)   P(RGB24,BGRX32)   P(RGB24,RGBX32)
	P(RGB24,XBGR32)

	/* BGR24 to #? */
	P(BGR24,RGB16)    P(BGR24,BGR16)    P(BGR24,RGB15)    P(BGR24,BGR15)
	P(BGR24,ARGB32)   P(BGR24,BGRA32)   P(BGR24,RGBA32)   P(BGR24,ABGR32)
	P(BGR24,RGB24)    P(BGR24,RGB16OE)  P(BGR24,BGR16OE)  P(BGR24,RGB15OE)
	P(BGR24,BGR15OE)  P(BGR24,XRGB32)   P(BGR24,BGRX32)   P(BGR24,RGBX32)
	P(BGR24,XBGR32)

	/* RGB15 to #? */
	P(RGB15,RGB16)    P(RGB15,BGR16)    P(RGB15,BGR15)    P(RGB15,ARGB32)
	P(RGB15,BGRA32)   P(RGB15,RGBA32)   P(RGB15,ABGR32)   P(RGB15,RGB24)
	P(RGB15,BGR24)    P(RGB15,RGB16OE)  P(RGB15,RGB15OE)  P(RGB15,BGR16OE)
	P(RGB15,BGR15OE)  P(RGB15,XRGB32)   P(RGB15,BGRX32)   P(RGB15,RGBX32)
	P(RGB15,XBGR32)

	/* BGR15 to #? */
	P(BGR15,RGB16)    P(BGR15,BGR16)    P(BGR15,RGB15)    P(BGR15,ARGB32)
	P(BGR15,BGRA32)   P(BGR15,RGBA32)   P(BGR15,ABGR32)   P(BGR15,RGB24)
	P(BGR15,BGR24)    P(BGR15,RGB16OE)  P(BGR15,RGB15OE)  P(BGR15,BGR16OE)
	P(BGR15,BGR15OE)  P(BGR15,XRGB32)   P(BGR15,BGRX32)   P(BGR15,RGBX32)
	P(BGR15,XBGR32)

	/* RGB16 to #? */
	P(RGB16,RGB15)    P(RGB16,BGR16)    P(RGB16,BGR15)    P(RGB16,ARGB32)
	P(RGB16,BGRA32)   P(RGB16,RGBA32)   P(RGB16,ABGR32)   P(RGB16,RGB24)
	P(RGB16,BGR24)    P(RGB16,RGB16OE)  P(RGB16,RGB15OE)  P(RGB16,BGR16OE)
	P(RGB16,BGR15OE)  P(RGB16,XRGB32)   P(RGB16,BGRX32)   P(RGB16,RGBX32)
	P(RGB16,XBGR32)

	/* BGR16 to #? */
	P(BGR16,RGB15)    P(BGR16,RGB16)    P(BGR16,BGR15)    P(BGR16,ARGB32)
	P(BGR16,BGRA32)   P(BGR16,RGBA32)   P(BGR16,ABGR32)   P(BGR16,RGB24)
	P(BGR16,BGR24)    P(BGR16,RGB16OE)  P(BGR16,RGB15OE)  P(BGR16,BGR16OE)
	P(BGR16,BGR15OE)  P(BGR16,XRGB32)   P(BGR16,BGRX32)   P(BGR16,RGBX32)
	P(BGR16,XBGR32)

	/* RGB16OE to #? */
	P(RGB16OE,RGB16)   P(RGB16OE,RGB15)  P(RGB16OE,BGR16)   P(RGB16OE,BGR15)
	P(RGB16OE,ARGB32)  P(RGB16OE,BGRA32) P(RGB16OE,RGBA32)  P(RGB16OE,ABGR32)
	P(RGB16OE,RGB24)   P(RGB16OE,BGR24)  P(RGB16OE,RGB15OE) P(RGB16OE,BGR16OE)
	P(RGB16OE,BGR15OE) P(RGB16OE,XRGB32) P(RGB16OE,BGRX32)  P(RGB16OE,RGBX32)
	P(RGB16OE,XBGR32)

	/* BGR16OE to #? */
	P(BGR16OE,RGB16)   P(BGR16OE,RGB15)  P(BGR16OE,BGR16)   P(BGR16OE,BGR15)
	P(BGR16OE,ARGB32)  P(BGR16OE,BGRA32) P(BGR16OE,RGBA32)  P(BGR16OE,ABGR32)
	P(BGR16OE,RGB24)   P(BGR16OE,BGR24)  P(BGR16OE,RGB15OE) P(BGR16OE,RGB16OE)
	P(BGR16OE,BGR15OE) P(BGR16OE,XRGB32) P(BGR16OE,BGRX32)  P(BGR16OE,RGBX32)
	P(BGR16OE,XBGR32)

	/* RGB15OE to #? */
	P(RGB15OE,RGB16)   P(RGB15OE,RGB15)  P(RGB15OE,BGR16)   P(RGB15OE,BGR15)
	P(RGB15OE,ARGB32)  P(RGB15OE,BGRA32) P(RGB15OE,RGBA32)  P(RGB15OE,ABGR32)
	P(RGB15OE,RGB24)   P(RGB15OE,BGR24)  P(RGB15OE,RGB16OE) P(RGB15OE,BGR16OE)
	P(RGB15OE,BGR15OE) P(RGB15OE,XRGB32) P(RGB15OE,BGRX32)  P(RGB15OE,RGBX32)
	P(RGB15OE,XBGR32)

	/* BGR15OE to #? */
	P(BGR15OE,RGB16)   P(BGR15OE,RGB15)  P(BGR15OE,BGR16)   P(BGR15OE,BGR15)
	P(BGR15OE,ARGB32)  P(BGR15OE,BGRA32) P(BGR15OE,RGBA32)  P(BGR15OE,ABGR32)
	P(BGR15OE,RGB24)   P(BGR15OE,BGR24)  P(BGR15OE,RGB16OE) P(BGR15OE,BGR16OE)
	P(BGR15OE,RGB15OE) P(BGR15OE,XRGB32) P(BGR15OE,BGRX32)  P(BGR15OE,RGBX32)
	P(BGR15OE,XBGR32)

	/* XRGB32 to #? */
	P(XRGB32,RGB16)   P(XRGB32,BGR16)   P(XRGB32,RGB15)   P(XRGB32,BGR15)
	P(XRGB32,ARGB32)  P(XRGB32,BGRA32)  P(XRGB32,RGBA32)  P(XRGB32,ABGR32)
	P(XRGB32,RGB24)   P(XRGB32,BGR24)   P(XRGB32,RGB16OE) P(XRGB32,BGR16OE)
	P(XRGB32,RGB15OE) P(XRGB32,BGR15OE) P(XRGB32,BGRX32)  P(XRGB32,RGBX32)
	P(XRGB32,XBGR32)

	/* BGRX32 to #? */
	P(BGRX32,RGB16)   P(BGRX32,BGR16)   P(BGRX32,RGB15)   P(BGRX32,BGR15)
	P(BGRX32,ARGB32)  P(BGRX32,BGRA32)  P(BGRX32,RGBA32)  P(BGRX32,ABGR32)
	P(BGRX32,RGB24)   P(BGRX32,BGR24)   P(BGRX32,RGB16OE) P(BGRX32,BGR16OE)
	P(BGRX32,RGB15OE) P(BGRX32,BGR15OE) P(BGRX32,XRGB32)  P(BGRX32,RGBX32)
	P(BGRX32,XBGR32)

	/* RGBX32 to #? */
	P(RGBX32,RGB16)   P(RGBX32,BGR16)   P(RGBX32,RGB15)   P(RGBX32,BGR15)
	P(RGBX32,BGRA32)  P(RGBX32,ARGB32)  P(RGBX32,ABGR32)  P(RGBX32,RGBA32)
	P(RGBX32,RGB24)   P(RGBX32,BGR24)   P(RGBX32,RGB16OE) P(RGBX32,BGR16OE)
	P(RGBX32,RGB15OE) P(RGBX32,BGR15OE) P(RGBX32,XRGB32)  P(RGBX32,BGRX32)
	P(RGBX32,XBGR32)

	/* XBGR32 to #? */
	P(XBGR32,RGB16)   P(XBGR32,BGR16)   P(XBGR32,RGB15)   P(XBGR32,BGR15)
	P(XBGR32,BGRA32)  P(XBGR32,ARGB32)  P(XBGR32,RGBA32)  P(XBGR32,ABGR32)
	P(XBGR32,RGB24)   P(XBGR32,BGR24)   P(XBGR32,RGB16OE) P(XBGR32,BGR16OE)
	P(XBGR32,RGB15OE) P(XBGR32,BGR15OE) P(XBGR32,XRGB32)  P(XBGR32,BGRX32)
	P(XBGR32,RGBX32)    
    
    	if (i == 0)
	{
	    Wait(SIGBREAKF_CTRL_C);
	    
	    func = uninstallfunc;
	}
	
    } /* for(int i= 0; i < 2; i++) */

    #undef P
    
    FreeBitMap(bitmap);
    OOP_ReleaseAttrBase(IID_Hidd_BitMap);
             
    return RETURN_OK;
}
