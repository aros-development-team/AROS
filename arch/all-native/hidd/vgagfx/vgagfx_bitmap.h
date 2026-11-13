/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.
*/

#ifndef VGAGFX_BITMAP_H
#define VGAGFX_BITMAP_H

#include "vgagfx_support.h"
#include <hidd/gfx.h>

/*
   This attribute interface is common for both vga onscreen and offscreen bitmap
   classes, although they don't have a common superclass
*/

#define CLID_Hidd_BitMap_VGA "hidd.bitmap.vga"
#define IID_Hidd_BitMap_VGA "hidd.bitmap.vga"

#define HiddVGABitMapAB __abHidd_VGABitMap

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddVGABitMapAB;
#endif

enum {
    aoHidd_VGABitMap_Drawable,
    
    num_Hidd_VGABitMap_Attrs
};

#define aHidd_VGABitMap_Drawable	(HiddVGABitMapAB + aoHidd_VGABitMap_Drawable)



/* This structure is used for both onscreen and offscreen VGA bitmaps !! */

#define IS_BM_ATTR(attr, idx) ( ( (idx) = (attr) - HiddBitMapAttrBase) < num_Hidd_BitMap_Attrs)
#define IS_VGABM_ATTR(attr, idx) ( ( (idx) = (attr) - HiddVGABitMapAB) < num_Hidd_VGABitMap_Attrs)


/*
   This structure is used as instance data for both the
   onbitmap and offbitmap classes.
*/

struct VGAGfxBitMapData
{
    struct vgaHWRec	*Regs;		/* Registers */
    unsigned char	*VideoData;	/* Pointing to video data */
    unsigned int	width;		/* Width of bitmap */
    unsigned int 	height;		/* Height of bitmap */
    unsigned int	bpr;		/* Bytes per row */
    unsigned int	disp_width;	/* Display width */
    unsigned int	disp_height;	/* Display height */
    int			xoffset;	/* Logical screen offset */
    int			yoffset;
    char		bpp;    	/* bits per pixel */
    BOOL		disp;		/* !=0 - displayed */
};

struct Box
{
    int x1, y1;
    int x2, y2;
};

VOID bitmap_clear(OOP_Class *, OOP_Object *, struct pHidd_BitMap_Clear *);
BOOL bitmap_setcolors(OOP_Class *, OOP_Object *, struct pHidd_BitMap_SetColors *);
VOID bitmap_putpixel(OOP_Class *, OOP_Object *, struct pHidd_BitMap_PutPixel *);
HIDDT_Pixel bitmap_getpixel(OOP_Class *, OOP_Object *, struct pHidd_BitMap_GetPixel *);
ULONG bitmap_drawpixel(OOP_Class *, OOP_Object *, struct pHidd_BitMap_DrawPixel *);

void vgaRefreshPixel(struct VGAGfxBitMapData *data, unsigned int x, unsigned int y);
void vgaRefreshArea(struct VGAGfxBitMapData *, struct Box *);
void vgaEraseArea(struct VGAGfxBitMapData *, struct Box *);

#endif /* VGAGFX_BITMAP_H */
