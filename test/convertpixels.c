#include <hidd/graphics.h>
#include <proto/graphics.h>

#include <stdio.h>

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

int main(void)
{
    ConvertPixelsA(argb, 0, SRC_PIXFMT, rgb15, 0, DST_PIXFMT, 8, 1, 0);
    ConvertPixelsA(rgb15, 0, DST_PIXFMT, argb_inv, 0, SRC_PIXFMT, 8, 1, 0);
    
    {
    	int i;
	
	for(i = 0; i < 8; i++)
	{
	    printf("ARGB32 %08lx = RGB15 %04x (%02x %02x %02x) (%3d%% %3d%% %3d%%) [%08x]\n",
	    	    argb[i], rgb15[i],
		    (rgb15[i] & 0x7C00) >> 10,
		    (rgb15[i] & 0x03E0) >> 5,
		    (rgb15[i] & 0x001F),
		    ((rgb15[i] & 0x7C00) >> 10) * 100 / 31,
		    ((rgb15[i] & 0x03E0) >> 5) * 100 / 31,
		    (rgb15[i] & 0x001F) * 100 / 31,
		    argb_inv[i]
		    );
	}
    }
    
    return 0;
}

