/*
    Copyright © 2021, The AROS Development Team. All rights reserved.
    $Id$
*/

#define __OOP_NOATTRBASES__

#ifdef NDEBUG
#warn "NDEBUG defined!!"
#endif

#include <hidd/gfx.h>
#include <proto/graphics.h>
#include <proto/oop.h>

#include <stdio.h>

#ifdef NDEBUG
#warn "NDEBUG defined!!"
#endif

#include <assert.h>

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>

#undef ConvertPixels

static OOP_AttrBase HiddBitMapAttrBase;

#if AROS_BIG_ENDIAN
#define SRC_PIXFMT vHidd_StdPixFmt_ARGB32
#define DST_PIXFMT vHidd_StdPixFmt_RGB15
#else
#define SRC_PIXFMT vHidd_StdPixFmt_BGRA32
#define DST_PIXFMT vHidd_StdPixFmt_RGB15_LE
#endif

static ULONG argb[8] =
{
    0x00112233,
    0x00FFFFFF,
    0xFF888888,
    0x00FF0000,
    0x0000FF00,
    0x000000FF,
    0x00FFFF00,
    0x8899AABB,
};
static UWORD rgb15[8];
static ULONG argb_inv[8];
static struct BitMap *bitmap;

/* Pointer to the file used by the tests. */
static FILE* temp_file = NULL;

/* The suite initialization function.
  * Returns zero on success, non-zero otherwise.
 */
int init_suite(void)
{
    HiddBitMapAttrBase = OOP_ObtainAttrBase(IID_Hidd_BitMap);
    if (!HiddBitMapAttrBase) {
	return -1;
    }
    
    bitmap = AllocBitMap(1, 1, 16, 0, NULL);
    if (!bitmap) {
        OOP_ReleaseAttrBase(IID_Hidd_BitMap);
	return -1;
    }
    return 0;
}

/* The suite cleanup function.
  * Returns zero on success, non-zero otherwise.
 */
int clean_suite(void)
{
    OOP_ReleaseAttrBase(IID_Hidd_BitMap);
    return 0;
}

static int ConvertPixels(APTR srcPixels, ULONG srcMod, HIDDT_StdPixFmt srcPixFmt,
		   APTR dstPixels, ULONG dstMod, HIDDT_StdPixFmt dstPixFmt,
		   ULONG width, ULONG height, OOP_Object *bm)
{
    OOP_Object *gfxhidd = NULL;
    OOP_Object *srcpf, *dstpf;
    APTR src = srcPixels;
    APTR dst = dstPixels;

    OOP_GetAttr(bm, aHidd_BitMap_GfxHidd, (IPTR *)&gfxhidd);

    if (!gfxhidd) {
        printf("ConvertPixels(): Failed to obtain graphics driver\n");
	return 1;
    }

    srcpf = HIDD_Gfx_GetPixFmt(gfxhidd, srcPixFmt);
    dstpf = HIDD_Gfx_GetPixFmt(gfxhidd, dstPixFmt);

    if (!srcpf || !dstpf)
    {
    	printf("ConvertPixels(): Bad source (%ld) or dest (%ld) pixfmt!\n", srcPixFmt, dstPixFmt);
	return 1;
    }

    HIDD_BM_ConvertPixels(bm, &src, (HIDDT_PixelFormat *)srcpf, srcMod, 
    	    	    	  &dst, (HIDDT_PixelFormat *)dstpf, dstMod,
			  width, height, NULL);
    return 0;
}

/* Simple test of ConvertPixels(argb -> rgb15).
 */
void testCONVPIX(void)
{
    CU_ASSERT(0 == ConvertPixels(argb, 0, SRC_PIXFMT, rgb15, 0, DST_PIXFMT, 8, 1, HIDD_BM_OBJ(bitmap)));
#if (0)
    //TODO: verify the conversion?
    {
    	int i;
	
	for(i = 0; i < 8; i++)
	{
	    printf("ARGB32 %08x = RGB15 %04x (%02x %02x %02x) (%3d%% %3d%% %3d%%) [%08x]\n",
	    	    (unsigned int)argb[i], rgb15[i],
		    (rgb15[i] & 0x7C00) >> 10,
		    (rgb15[i] & 0x03E0) >> 5,
		    (rgb15[i] & 0x001F),
		    ((rgb15[i] & 0x7C00) >> 10) * 100 / 31,
		    ((rgb15[i] & 0x03E0) >> 5) * 100 / 31,
		    (rgb15[i] & 0x001F) * 100 / 31,
		    (unsigned int)argb_inv[i]
		    );
	}
    }
#endif
}

/* Simple test of ConvertPixels(rgb15 -> argb).
 */
void testCONVPIX2(void)
{
    CU_ASSERT(0 == ConvertPixels(rgb15, 0, DST_PIXFMT, argb_inv, 0, SRC_PIXFMT, 8, 1, HIDD_BM_OBJ(bitmap)));
#if (0)
    {
    	int i;
	
	for(i = 0; i < 8; i++)
	{
	    printf("ARGB32 %08x = RGB15 %04x (%02x %02x %02x) (%3d%% %3d%% %3d%%) [%08x]\n",
	    	    (unsigned int)argb[i], rgb15[i],
		    (rgb15[i] & 0x7C00) >> 10,
		    (rgb15[i] & 0x03E0) >> 5,
		    (rgb15[i] & 0x001F),
		    ((rgb15[i] & 0x7C00) >> 10) * 100 / 31,
		    ((rgb15[i] & 0x03E0) >> 5) * 100 / 31,
		    (rgb15[i] & 0x001F) * 100 / 31,
		    (unsigned int)argb_inv[i]
		    );
	}
    }
#endif
}

int main(void)
{
    CU_pSuite pSuite = NULL;

    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    /* add a suite to the registry */
    pSuite = CU_add_suite("ConvertPixels_Suite", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* add the tests to the suite */
    /* NOTE - ORDER IS IMPORTANT - MUST TEST fread() AFTER fprintf() */
    if ((NULL == CU_add_test(pSuite, "test of ConvertPixels(argb->rgb15)", testCONVPIX)) ||
       (NULL == CU_add_test(pSuite, "test of ConvertPixels(rgb15->argb)", testCONVPIX2)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Run all tests using the CUnit Basic & Automated interfaces */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("GfxHiddUnitTests");
    CU_set_output_filename("GfxHidd-ConvertPixels");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}

