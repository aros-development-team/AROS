/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: X11 gfx HIDD for AROS.
    Lang: English.
*/

#include <X11/Xlib.h>

#include <proto/oop.h>
#include <proto/exec.h>
#include <exec/memory.h>

#include <hidd/graphics.h>
#include <aros/debug.h>

#include "x11gfx_intern.h"
#include "x11.h"

#include <X11/Xlib.h>

#undef OOPBase

#undef SysBase

#define SysBase xsd->sysbase
#define OOPBase xsd->oopbase

#undef LX11
#undef UX11
#define LX11 ObtainSemaphore(&xsd->x11sema);
#define UX11 ReleaseSemaphore(&xsd->x11sema);

BOOL set_pixelformat(Object *bm
	, struct x11_staticdata *xsd
	, Drawable d)
{
    
    struct TagItem pf_tags[] = {
    	{ aHidd_PixFmt_RedShift,	xsd->red_shift		}, /* 0 */
	{ aHidd_PixFmt_GreenShift,	xsd->green_shift	}, /* 1 */
	{ aHidd_PixFmt_BlueShift,  	xsd->blue_shift		}, /* 2 */
	{ aHidd_PixFmt_AlphaShift,	0			}, /* 3 */
	{ aHidd_PixFmt_RedMask,		xsd->vi.red_mask	}, /* 4 */
	{ aHidd_PixFmt_GreenMask,	xsd->vi.green_mask	}, /* 5 */
	{ aHidd_PixFmt_BlueMask,	xsd->vi.blue_mask	}, /* 6 */
	{ aHidd_PixFmt_AlphaMask,	0x00000000		}, /* 7 */
	{ aHidd_PixFmt_GraphType,	0			}, /* 8 */
	{ aHidd_PixFmt_Depth,		0			}, /* 9 */
	{ aHidd_PixFmt_BytesPerPixel,	0			}, /* 10 */
	{ aHidd_PixFmt_BitsPerPixel,	0			}, /* 11 */
	{ aHidd_PixFmt_StdPixFmt,	0			}, /* 12 */
	{ aHidd_PixFmt_CLUTShift,	xsd->clut_shift		}, /* 13 */
	{ aHidd_PixFmt_CLUTMask,	xsd->clut_mask		}, /* 13 */
	{ TAG_DONE, 0UL }
    };
    
    ULONG bytes_per_pixel, size;
    Object *pf;
    
    
    switch (xsd->vi.class) {
    	case TrueColor:
	    pf_tags[8].ti_Data = vHidd_GT_TrueColor;
	    break;
	
	case PseudoColor:
	    pf_tags[8].ti_Data = vHidd_GT_Palette;
	    break;
	
	case DirectColor:
//	    kprintf("!!! HUH ?? What is DirectColor ??\n");
	    pf_tags[8].ti_Data = vHidd_GT_TrueColor;
	    break;
	
	case StaticColor:
	    pf_tags[8].ti_Data = vHidd_GT_StaticPalette;
	    break;
    }

    pf_tags[12].ti_Data = vHidd_PixFmt_Native;
    
    /* Get some info on the bitmap */
    get_bitmap_info(xsd, d, &size, &bytes_per_pixel);
    
    pf_tags[10].ti_Data = bytes_per_pixel;
    pf_tags[11].ti_Data = size;
    
    
#warning Is the below correct in all cases ?
    pf_tags[9].ti_Data = size;
    
    
    pf = HIDD_BM_SetPixelFormat(bm, pf_tags);
    if (NULL == pf) {
	return FALSE;
    }
    
#define PF(x) ((HIDDT_PixelFormat *)x)    

/*
kprintf("PF: %p\n", pf);

kprintf("(%d, %d, %d, %d), (%x, %x, %x, %x), %d, %d, %d, %d\n"
	, PF(pf)->red_shift
	, PF(pf)->green_shift
	, PF(pf)->blue_shift
	, PF(pf)->alpha_shift
	, PF(pf)->red_mask
	, PF(pf)->green_mask
	, PF(pf)->blue_mask
	, PF(pf)->alpha_mask
	, PF(pf)->bytes_per_pixel
	, PF(pf)->size
	, PF(pf)->depth
	, PF(pf)->stdpixfmt);

*/    
    return TRUE;

}



VOID get_bitmap_info(struct x11_staticdata *xsd
	, Drawable d
	, ULONG *sz
	, ULONG *bppix)
{
    XImage *testimage;
    
LX11	

    /* Create a dummy X image to get bits per pixel */
    testimage = XGetImage(xsd->display
	, d // RootWindow(xsd->display, DefaultScreen(xsd->display))
	, 0, 0, 1, 1
	, AllPlanes, ZPixmap
    );
    
UX11
	
    if (NULL != testimage) {
	*sz = testimage->bits_per_pixel;
LX11
	    XDestroyImage(testimage);
UX11
    } else {
	kprintf("!!! set_pixelformat(): could not get bits per pixel\n");
	kill(getpid(), 19);
    }
	
    *bppix = (*sz + 7) >> 3;
}


#undef OOPBase

#undef OOPBase
#define OOPBase ((struct Library *)OCLASS(OCLASS(o))->UserData)


VOID Hidd_X11Mouse_HandleEvent(Object *o, XEvent *event)
{
    struct pHidd_X11Mouse_HandleEvent msg;
    static MethodID mid = 0;
    
    if (!mid)
    	mid = GetMethodID(IID_Hidd_X11Mouse, moHidd_X11Mouse_HandleEvent);
	
    msg.mID = mid;
    msg.event = event;
    
    DoMethod(o, (Msg) &msg);
}


VOID Hidd_X11Kbd_HandleEvent(Object *o, XEvent *event)
{
    struct pHidd_X11Kbd_HandleEvent msg;
    static MethodID mid = 0;
    
    
    if (!mid)
    	mid = GetMethodID(IID_Hidd_X11Kbd, moHidd_X11Kbd_HandleEvent);

    msg.mID = mid;
    msg.event = event;
    
    DoMethod(o, (Msg) &msg);
}


