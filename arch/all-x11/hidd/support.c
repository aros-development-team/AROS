/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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

#define SET_PF_TAG(idx, code, val)	\
	pftags[idx].ti_Tag  = aHidd_PixFmt_ ## code;	\
	pftags[idx].ti_Data = (IPTR)val;

BOOL set_pixelformat(struct TagItem *pftags
	, struct x11_staticdata *xsd
	, Drawable d)
{
#if 0    
    struct TagItem pf_tags[] = {
    	{ aHidd_PixFmt_RedShift,	xsd->red_shift		}, /* 0 */
	{ aHidd_PixFmt_GreenShift,	xsd->green_shift	}, /* 1 */
	{ aHidd_PixFmt_BlueShift,  	xsd->blue_shift		}, /* 2 */
	{ aHidd_PixFmt_AlphaShift,	0			}, /* 3 */
	{ aHidd_PixFmt_RedMask,		xsd->vi.red_mask	}, /* 4 */
	{ aHidd_PixFmt_GreenMask,	xsd->vi.green_mask	}, /* 5 */
	{ aHidd_PixFmt_BlueMask,	xsd->vi.blue_mask	}, /* 6 */
	{ aHidd_PixFmt_AlphaMask,	0x00000000		}, /* 7 */
	{ aHidd_PixFmt_ColorModel,	0			}, /* 8 */
	{ aHidd_PixFmt_Depth,		0			}, /* 9 */
	{ aHidd_PixFmt_BytesPerPixel,	0			}, /* 10 */
	{ aHidd_PixFmt_BitsPerPixel,	0			}, /* 11 */
	{ aHidd_PixFmt_StdPixFmt,	0			}, /* 12 */
	{ aHidd_PixFmt_CLUTShift,	xsd->clut_shift		}, /* 13 */
	{ aHidd_PixFmt_CLUTMask,	xsd->clut_mask		}, /* 14 */
	{ aHidd_PixFmt_BitMapType,	0			}, /* 15 */
	{ TAG_DONE, 0UL }
    };
#endif    
    ULONG bytes_per_pixel, size;
    HIDDT_ColorModel cmodel;
    
    switch (xsd->vi.class) {
    	case TrueColor:	  cmodel = vHidd_ColorModel_TrueColor;		break;
	case PseudoColor: cmodel = vHidd_ColorModel_Palette;		break;
	case DirectColor: cmodel = vHidd_ColorModel_DirectColor;	break;
	case StaticColor: cmodel = vHidd_ColorModel_StaticPalette;	break;
    }

    /* Get some info on the bitmap */
    get_bitmap_info(xsd, d, &size, &bytes_per_pixel);
    
    SET_PF_TAG(0, RedShift,	xsd->red_shift		);
    SET_PF_TAG(1, GreenShift,	xsd->green_shift	);
    SET_PF_TAG(2, BlueShift,	xsd->blue_shift		);
    SET_PF_TAG(3, AlphaShift,	0			);
    
    SET_PF_TAG(4, RedMask,	xsd->vi.red_mask	);
    SET_PF_TAG(5, GreenMask,	xsd->vi.green_mask	);
    SET_PF_TAG(6, BlueMask,	xsd->vi.blue_mask	);
    SET_PF_TAG(7, AlphaMask,	0x00000000		);
    
    SET_PF_TAG(8, ColorModel,	cmodel	);
    SET_PF_TAG(9, Depth,	size	);
    
    SET_PF_TAG(10, BytesPerPixel,	bytes_per_pixel	);
    SET_PF_TAG(11, BitsPerPixel,	size		);
    SET_PF_TAG(12, CLUTShift,		xsd->clut_shift	);
    SET_PF_TAG(13, CLUTMask,		xsd->clut_mask	);
    SET_PF_TAG(14, BitMapType,		vHidd_BitMapType_Chunky	);
    
    pftags[15].ti_Tag = TAG_DONE;
    
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
//	kill(getpid(), 19);
    }
	
    *bppix = (*sz + 7) >> 3;
}

#undef OOPBase
#define OOPBase (OOP_OOPBASE(o))

VOID Hidd_X11Mouse_HandleEvent(OOP_Object *o, XEvent *event)
{
    struct pHidd_X11Mouse_HandleEvent msg;
    static OOP_MethodID mid = 0;
    
    if (!mid)
    	mid = OOP_GetMethodID(IID_Hidd_X11Mouse, moHidd_X11Mouse_HandleEvent);
	
    msg.mID = mid;
    msg.event = event;
    
    OOP_DoMethod(o, (OOP_Msg) &msg);
}


VOID Hidd_X11Kbd_HandleEvent(OOP_Object *o, XEvent *event)
{
    struct pHidd_X11Kbd_HandleEvent msg;
    static OOP_MethodID mid = 0;
    
    
    if (!mid)
    	mid = OOP_GetMethodID(IID_Hidd_X11Kbd, moHidd_X11Kbd_HandleEvent);

    msg.mID = mid;
    msg.event = event;
    
    OOP_DoMethod(o, (OOP_Msg) &msg);
}


