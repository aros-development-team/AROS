#define __OOP_NOATTRBASES__

#include <hidd/graphics.h>
#include <proto/graphics.h>
#include <proto/oop.h>

#include <stdio.h>

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

static void ConvertPixels(APTR srcPixels, ULONG srcMod, HIDDT_StdPixFmt srcPixFmt,
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
	return;
    }

    srcpf = HIDD_Gfx_GetPixFmt(gfxhidd, srcPixFmt);
    dstpf = HIDD_Gfx_GetPixFmt(gfxhidd, dstPixFmt);

    if (!srcpf || !dstpf)
    {
    	printf("ConvertPixels(): Bad source (%ld) or dest (%ld) pixfmt!\n", srcPixFmt, dstPixFmt);
	return;
    }

    HIDD_BM_ConvertPixels(bm, &src, (HIDDT_PixelFormat *)srcpf, srcMod, 
    	    	    	  &dst, (HIDDT_PixelFormat *)dstpf, dstMod,
			  width, height, NULL);
}

int main(void)
{
    struct BitMap *bitmap;
    
    HiddBitMapAttrBase = OOP_ObtainAttrBase(IID_Hidd_BitMap);
    if (!HiddBitMapAttrBase) {
        printf("Failed to obtain IID_Hidd_BitMap\n");
	return RETURN_FAIL;
    }
    
    bitmap = AllocBitMap(1, 1, 16, 0, NULL);
    if (!bitmap) {
        printf("Failed to allocate a placeholder bitmap!\n");
	OOP_ReleaseAttrBase(IID_Hidd_BitMap);
	return RETURN_FAIL;
    }

    ConvertPixels(argb, 0, SRC_PIXFMT, rgb15, 0, DST_PIXFMT, 8, 1, HIDD_BM_OBJ(bitmap));
    ConvertPixels(rgb15, 0, DST_PIXFMT, argb_inv, 0, SRC_PIXFMT, 8, 1, HIDD_BM_OBJ(bitmap));
    
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
    
    OOP_ReleaseAttrBase(IID_Hidd_BitMap);
    return 0;
}

