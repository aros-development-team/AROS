/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Bitmap class for VGA hidd.
    Lang: English.
*/


#include <proto/oop.h>
#include <proto/utility.h>

#include <exec/memory.h>
#include <exec/lists.h>

#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <oop/oop.h>

#include <hidd/graphics.h>

#include <assert.h>

#include "vgahw.h"
#include "vga.h"
#include "vgaclass.h"

#include "bitmap.h"

/* Don't initialize static variables with "=0", otherwise they go into DATA segment */

OOP_AttrBase HiddBitMapAttrBase;
OOP_AttrBase HiddPixFmtAttrBase;
OOP_AttrBase HiddGfxAttrBase;
OOP_AttrBase HiddSyncAttrBase;
OOP_AttrBase HiddVGAGfxAB;
OOP_AttrBase HiddVGABitMapAB;

static struct OOP_ABDescr attrbases[] = 
{
    { IID_Hidd_BitMap,		&HiddBitMapAttrBase },
    { IID_Hidd_PixFmt,		&HiddPixFmtAttrBase },
    { IID_Hidd_Gfx,		&HiddGfxAttrBase },
    { IID_Hidd_Sync,		&HiddSyncAttrBase },
    /* Private bases */
    { IID_Hidd_VGAgfx,		&HiddVGAGfxAB	},
    { IID_Hidd_VGABitMap,	&HiddVGABitMapAB },
    { NULL, NULL }
};

void vgaRestore(struct vgaHWRec *, BOOL onlyDAC);
void * vgaSave(struct vgaHWRec *);
int vgaInitMode(struct vgaModeDesc *, struct vgaHWRec *);
void vgaLoadPalette(struct vgaHWRec *, unsigned char *);

void free_onbmclass(struct vga_staticdata *);

#define MNAME(x) onbitmap_ ## x

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#define OnBitmap 1
#include "bitmap_common.c"

#if 0

void clr();
void scr_RawPutChars(char *, int);

char tab[127];

#define rkprintf(x...)  scr_RawPutChars(tab, sprintf(tab, x))

#else

#define clr() /* eps */
#define rkprintf(x...) /* eps */

#endif

/*********** BitMap::New() *************************************/

static OOP_Object *onbitmap_new(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    EnterFunc(bug("VGAGfx.BitMap::New()\n"));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if (o)
    {
    	struct bitmap_data *data;
	
		OOP_Object *pf;

        IPTR width, height, depth;
	
        data = OOP_INST_DATA(cl, o);

		/* clear all data  */
        memset(data, 0, sizeof(struct bitmap_data));
	
		/* Get attr values */
		OOP_GetAttr(o, aHidd_BitMap_Width,		&width);
		OOP_GetAttr(o, aHidd_BitMap_Height, 	&height);
		OOP_GetAttr(o,  aHidd_BitMap_PixFmt,	(IPTR *)&pf);
		OOP_GetAttr(pf, aHidd_PixFmt_Depth,		&depth);
	
		ASSERT (width != 0 && height != 0 && depth != 0);
	
		/* 
		   We must only create depths that are supported by the friend drawable
	  	 Currently we only support the default depth
		*/

		data->width = width;
		data->height = height;
		data->bpp = depth;
		data->Regs = AllocVec(sizeof(struct vgaHWRec),MEMF_PUBLIC|MEMF_CLEAR);
		data->disp = -1;
		width=(width+15) & ~15;

		/*
			Here there is brand new method of getting pixelclock data.
			It was introduced here to make the code more portable. Besides
			it may now be used as a base for creating other low level
			video drivers
		*/

		if (data->Regs)
		{
		    data->VideoData = AllocVec(width*height,MEMF_PUBLIC|MEMF_CLEAR);
		    if (data->VideoData)
		    {
				struct vgaModeDesc mode;
				HIDDT_ModeID modeid;
				OOP_Object *sync;
				OOP_Object *pf;
				ULONG pixelc;
				
				/* We should be able to get modeID from the bitmap */
				OOP_GetAttr(o, aHidd_BitMap_ModeID, &modeid);
				
				if (modeid != vHidd_ModeID_Invalid)
				{
					struct Box box = {0, 0, width-1, height-1};
	
					/* Get Sync and PixelFormat properties */
					HIDD_Gfx_GetMode(XSD(cl)->vgahidd, modeid, &sync, &pf);

					mode.Width 	= width;
					mode.Height = height;
					mode.Depth 	= depth;
					OOP_GetAttr(sync, aHidd_Sync_PixelClock, &pixelc);

					mode.clock	= (pixelc > 26000000) ? 1 : 0;
					mode.Flags	= 0;
					mode.HSkew	= 0;
					OOP_GetAttr(sync, aHidd_Sync_HDisp, 		&mode.HDisplay);
					OOP_GetAttr(sync, aHidd_Sync_VDisp, 		&mode.VDisplay);
					OOP_GetAttr(sync, aHidd_Sync_HSyncStart, 	&mode.HSyncStart);
					OOP_GetAttr(sync, aHidd_Sync_VSyncStart, 	&mode.VSyncStart);
					OOP_GetAttr(sync, aHidd_Sync_HSyncEnd,		&mode.HSyncEnd);
					OOP_GetAttr(sync, aHidd_Sync_VSyncEnd,		&mode.VSyncEnd);
					OOP_GetAttr(sync, aHidd_Sync_HTotal,		&mode.HTotal);
					OOP_GetAttr(sync, aHidd_Sync_VTotal,		&mode.VTotal);
				    
				    ObtainSemaphore(&XSD(cl)->HW_acc);

				    /* Now, when the best display mode is chosen, we can build it */
				    vgaInitMode(&mode, data->Regs);
				    vgaLoadPalette(data->Regs,(unsigned char *)NULL);

				    /*
				       Because of not defined BitMap_Show method show 
				       bitmap immediately
				    */
		
				    vgaRestore(data->Regs, FALSE);
				    vgaRefreshArea(data, 1, &box);

				    ReleaseSemaphore(&XSD(cl)->HW_acc);

				    XSD(cl)->visible = data;	/* Set created object as visible */

				    ReturnPtr("VGAGfx.BitMap::New()", Object *, o);
				}
		
		    } /* if got data->VideoData */
		    FreeVec(data->Regs);
		} /* if got data->Regs */

		{
		    OOP_MethodID disp_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
	    	    OOP_CoerceMethod(cl, o, (OOP_Msg) &disp_mid);
		}
	
		o = NULL;
    } /* if created object */

    ReturnPtr("VGAGfx.BitMap::New()", OOP_Object *, o);
}

/**********  Bitmap::Dispose()  ***********************************/

static VOID onbitmap_dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    EnterFunc(bug("VGAGfx.BitMap::Dispose()\n"));
    
    if (data->VideoData)
	FreeVec(data->VideoData);
    if (data->Regs)
	FreeVec(data->Regs);
    
    OOP_DoSuperMethod(cl, o, msg);
    
    ReturnVoid("VGAGfx.BitMap::Dispose");
}

/*** init_onbmclass *********************************************************/

#undef XSD
#define XSD(cl) xsd

#define NUM_ROOT_METHODS   3
#define NUM_BITMAP_METHODS 10

OOP_Class *init_onbmclass(struct vga_staticdata *xsd)
{
    struct OOP_MethodDescr root_descr[NUM_ROOT_METHODS + 1] =
    {
        {(IPTR (*)())MNAME(new)    , moRoot_New    },
        {(IPTR (*)())MNAME(dispose), moRoot_Dispose},
#if 0
        {(IPTR (*)())MNAME(set)	   , moRoot_Set},
#endif
        {(IPTR (*)())MNAME(get)	   , moRoot_Get},
        {NULL, 0UL}
    };

    struct OOP_MethodDescr bitMap_descr[NUM_BITMAP_METHODS + 1] =
    {
        {(IPTR (*)())MNAME(setcolors),		moHidd_BitMap_SetColors},
    	{(IPTR (*)())MNAME(putpixel),		moHidd_BitMap_PutPixel},
    	{(IPTR (*)())MNAME(clear),		moHidd_BitMap_Clear},
    	{(IPTR (*)())MNAME(getpixel),		moHidd_BitMap_GetPixel},
/*    	{(IPTR (*)())MNAME(drawpixel),		moHidd_BitMap_DrawPixel},*/
    	{(IPTR (*)())MNAME(fillrect),		moHidd_BitMap_FillRect},
    	{(IPTR (*)())MNAME(getimage),		moHidd_BitMap_GetImage},
    	{(IPTR (*)())MNAME(putimage),		moHidd_BitMap_PutImage},
    	{(IPTR (*)())MNAME(blitcolorexpansion),	moHidd_BitMap_BlitColorExpansion},
    	{(IPTR (*)())MNAME(putimagelut),	moHidd_BitMap_PutImageLUT},
    	{(IPTR (*)())MNAME(getimagelut),	moHidd_BitMap_GetImageLUT},
        {NULL, 0UL}
    };
    
    struct OOP_InterfaceDescr ifdescr[] =
    {
        {root_descr,    IID_Root       , NUM_ROOT_METHODS},
        {bitMap_descr,  IID_Hidd_BitMap, NUM_BITMAP_METHODS},
        {NULL, NULL, 0}
    };

    OOP_AttrBase MetaAttrBase = OOP_ObtainAttrBase(IID_Meta);

    struct TagItem tags[] =
    {
        {aMeta_SuperID,        (IPTR) CLID_Hidd_BitMap},
        {aMeta_InterfaceDescr, (IPTR) ifdescr},
        {aMeta_InstSize,       (IPTR) sizeof(struct bitmap_data)},
        {TAG_DONE, 0UL}
    };
    
    OOP_Class *cl = NULL;

    EnterFunc(bug("init_bitmapclass(xsd=%p)\n", xsd));
    
    D(bug("Metattrbase: %x\n", MetaAttrBase));

    if(MetaAttrBase)
    {
	D(bug("Got attrbase\n"));
       
/*    for (;;) {cl = cl; } */
        cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
        if(cl)
        {
            D(bug("BitMap class ok\n"));
            xsd->onbmclass = cl;
            cl->UserData     = (APTR) xsd;
           
            /* Get attrbase for the BitMap interface */
	    if (OOP_ObtainAttrBases(attrbases))
            {
                OOP_AddClass(cl);
            }
            else
            {
                free_onbmclass( xsd );
                cl = NULL;
            }
        }
	
	/* We don't need this anymore */
	OOP_ReleaseAttrBase(IID_Meta);
    } /* if(MetaAttrBase) */

    ReturnPtr("init_onbmclass", OOP_Class *,  cl);
}

/*** free_bitmapclass *********************************************************/

void free_onbmclass(struct vga_staticdata *xsd)
{
    EnterFunc(bug("free_onbmclass(xsd=%p)\n", xsd));

    if(xsd)
    {
        OOP_RemoveClass(xsd->onbmclass);
        if(xsd->onbmclass) OOP_DisposeObject((OOP_Object *) xsd->onbmclass);
        xsd->onbmclass = NULL;
	
	OOP_ReleaseAttrBases(attrbases);
    }

    ReturnVoid("free_onbmclass");
}
