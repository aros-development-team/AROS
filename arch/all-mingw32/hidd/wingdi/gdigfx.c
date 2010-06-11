/*
    Copyright  1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: GDI gfx HIDD for AROS.
    Lang: English.
*/

#define __OOP_NOATTRBASES__

#include <exec/libraries.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <graphics/displayinfo.h>
#include <aros/libcall.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <oop/oop.h>

#include <hidd/hidd.h>
#include <hidd/graphics.h>

#include <aros/symbolsets.h>

#include "gdi.h"
#include "bitmap.h"

#include LC_LIBDEFS_FILE

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#ifdef DEBUG_POINTER

#define PRINT_POINTER(image, xsize, xmax, ymax)		\
bug("[GDIGfx] Pointer data:\n");			\
{							\
    ULONG *pix = (ULONG *)image;			\
    ULONG x, y;						\
							\
    for (y = 0; y < ymax; y++) {			\
        for (x = 0; x < xmax; x++)			\
	    bug("0x%08X ", pix[x]);			\
	bug("\n");					\
	pix += xsize;					\
    }							\
}

#else
#define PRINT_POINTER(image, xsize, xmax, ymax)
#endif

/****************************************************************************************/

/* Some attrbases needed as global vars.
  These are write-once read-many */

static OOP_AttrBase HiddBitMapAttrBase;  
static OOP_AttrBase HiddGDIBitMapAB;
static OOP_AttrBase HiddSyncAttrBase;
static OOP_AttrBase HiddPixFmtAttrBase;
static OOP_AttrBase HiddGfxAttrBase;

static struct OOP_ABDescr attrbases[] =
{
    { IID_Hidd_BitMap	, &HiddBitMapAttrBase	},
    { IID_Hidd_GDIBitMap, &HiddGDIBitMapAB	},
    { IID_Hidd_Sync 	, &HiddSyncAttrBase	},
    { IID_Hidd_PixFmt	, &HiddPixFmtAttrBase	},
    { IID_Hidd_Gfx  	, &HiddGfxAttrBase	},
    { NULL  	    	, NULL      	    	}
};

void GfxIntHandler(struct gfx_data *data, struct Task *task)
{
    Signal(task, SIGF_BLIT);
}

/****************************************************************************************/

OOP_Object *GDICl__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct TagItem pftags[] =
    {
        /* Shifts are non-obviously calculated from the MSB, not from the LSB.
           I. e. color value is placed in the most significant byte of the ULONG
           before shifting (cc000000, not 000000cc) */
    	{ aHidd_PixFmt_RedShift     , 24			   },
	{ aHidd_PixFmt_GreenShift   , 16			   },
	{ aHidd_PixFmt_BlueShift    , 8				   },
	{ aHidd_PixFmt_AlphaShift   , 0				   },
	{ aHidd_PixFmt_RedMask	    , 0x000000FF		   },
	{ aHidd_PixFmt_GreenMask    , 0x0000FF00		   },
	{ aHidd_PixFmt_BlueMask     , 0x00FF0000		   },
	{ aHidd_PixFmt_AlphaMask    , 0x00000000		   },
	/* Windows effectively hides from us all details of
	   color management and everything looks like if the
	   display mode is always truecolor */
	{ aHidd_PixFmt_ColorModel   , vHidd_ColorModel_TrueColor   },
	{ aHidd_PixFmt_Depth	    , 24			   },
	{ aHidd_PixFmt_BytesPerPixel, 4				   },
	{ aHidd_PixFmt_BitsPerPixel , 24			   },
	{ aHidd_PixFmt_StdPixFmt    , vHidd_StdPixFmt_Native	   },
	{ aHidd_PixFmt_BitMapType   , vHidd_BitMapType_Chunky      },
	{ TAG_DONE  	    	    , 0UL			   } 
    };
    
    struct TagItem tags_160_160[] =
    {
    	{ aHidd_Sync_HDisp  	, 160 	    	    	 },
	{ aHidd_Sync_VDisp  	, 160 	    	    	 },
	{ aHidd_Sync_Description, (IPTR)"Windows:160x160"},
	{ TAG_DONE  	    	, 0UL 	    	    	 }
    };
    
    struct TagItem tags_240_320[] =
    {
    	{ aHidd_Sync_HDisp  	, 240 	    	    	 },
	{ aHidd_Sync_VDisp  	, 320 	    	    	 },
	{ aHidd_Sync_Description, (IPTR)"Windows:240x320"},
	{ TAG_DONE  	    	, 0UL 	    	    	 } 
    };

    struct TagItem tags_320_240[] = 
    {
    	{ aHidd_Sync_HDisp  	, 320 	    	    	 },
	{ aHidd_Sync_VDisp  	, 240 	    	    	 },
	{ aHidd_Sync_Description, (IPTR)"Windows:320x240"},
	{ TAG_DONE  	    	, 0UL 	    	    	 }
    };

    struct TagItem tags_512_384[] = 
    {
    	{ aHidd_Sync_HDisp  	, 512 	    	    	 },
	{ aHidd_Sync_VDisp  	, 384 	    	    	 },
	{ aHidd_Sync_Description, (IPTR)"Windows:512x384"},
	{ TAG_DONE  	    	, 0UL 	    	    	 }
    };

    struct TagItem tags_640_480[] = 
    {
    	{ aHidd_Sync_HDisp  	, 640 	    	    	 },
	{ aHidd_Sync_VDisp  	, 480 	    	    	 },
	{ aHidd_Sync_Description, (IPTR)"Windows:640x480"},
	{ TAG_DONE  	    	, 0UL 	    	    	 }
    };

    struct TagItem tags_800_600[] = 
    {
    	{ aHidd_Sync_HDisp  	, 800 	    	    	 },
	{ aHidd_Sync_VDisp  	, 600 	    	    	 },
	{ aHidd_Sync_Description, (IPTR)"Windows:800x600"},
	{ TAG_DONE  	    	, 0UL 	    	    	 }
    };

    struct TagItem tags_1024_768[] = 
    {
    	{ aHidd_Sync_HDisp  	, 1024      	    	  },
	{ aHidd_Sync_VDisp  	, 768       	    	  },
	{ aHidd_Sync_Description, (IPTR)"Windows:1024x768"},
	{ TAG_DONE  	    	, 0UL       	    	  }
    };
    
    struct TagItem tags_1152_864[] = 
    {
    	{ aHidd_Sync_HDisp  	, 1152      	    	  },
	{ aHidd_Sync_VDisp  	, 864       	    	  },
	{ aHidd_Sync_Description, (IPTR)"Windows:1152x864"},
	{ TAG_DONE  	    	, 0UL       	    	  }
    };

    struct TagItem tags_1280_800[] = 
    {
    	{ aHidd_Sync_HDisp  	, 1280      	    	  },
	{ aHidd_Sync_VDisp  	, 800       	    	  },
	{ aHidd_Sync_Description, (IPTR)"Windows:1280x800"},
	{ TAG_DONE  	    	, 0UL       	    	  }
    };

    struct TagItem tags_1280_960[] = 
    {
    	{ aHidd_Sync_HDisp  	, 1280      	    	  },
	{ aHidd_Sync_VDisp  	, 960       	    	  },
	{ aHidd_Sync_Description, (IPTR)"Windows:1280x960"},
	{ TAG_DONE  	    	, 0UL       	    	  }
    };
    
    struct TagItem tags_1280_1024[] =
    {
    	{ aHidd_Sync_HDisp  	, 1280      	    	   },
	{ aHidd_Sync_VDisp  	, 1024      	    	   },
	{ aHidd_Sync_Description, (IPTR)"Windows:1280x1024"},
	{ TAG_DONE  	    	, 0UL       	    	   }
    };
    
    struct TagItem tags_1600_1200[] = 
    {
    	{ aHidd_Sync_HDisp  	, 1600      	    	   },
	{ aHidd_Sync_VDisp  	, 1200      	    	   },
	{ aHidd_Sync_Description, (IPTR)"Windows:1600x1200"},
	{ TAG_DONE  	    	, 0UL       	    	   }
    };
    
    struct TagItem mode_tags[] =
    {
	{ aHidd_Gfx_PixFmtTags	, (IPTR)pftags		},

	/* Default values for the sync attributes */
	{ aHidd_Sync_PixelClock , 0			},
	{ aHidd_Sync_HTotal	, 0			},
	{ aHidd_Sync_VTotal	, 0			},
	{ aHidd_Sync_HMin	, 112			}, /* In fact these can be even smaller, and */
	{ aHidd_Sync_VMin	, 112			}, /* maximum can be even bigger...	     */
	{ aHidd_Sync_HMax	, 16384			},
	{ aHidd_Sync_VMax	, 16384			},

	/* The different syncmodes. The default attribute values above 
	    will be applied to each of these. Note that
	    you can alter the defaults between the tags below 
	*/
	{ aHidd_Gfx_SyncTags	, (IPTR)tags_160_160	},
	{ aHidd_Gfx_SyncTags	, (IPTR)tags_240_320	},
	{ aHidd_Gfx_SyncTags	, (IPTR)tags_320_240	},
	{ aHidd_Gfx_SyncTags	, (IPTR)tags_512_384	},
	{ aHidd_Gfx_SyncTags	, (IPTR)tags_640_480	},
	{ aHidd_Gfx_SyncTags	, (IPTR)tags_800_600	},
	{ aHidd_Gfx_SyncTags	, (IPTR)tags_1024_768	},
	{ aHidd_Gfx_SyncTags	, (IPTR)tags_1152_864	},
        { aHidd_Gfx_SyncTags	, (IPTR)tags_1280_800	},
	{ aHidd_Gfx_SyncTags	, (IPTR)tags_1280_960	},
	{ aHidd_Gfx_SyncTags	, (IPTR)tags_1280_1024	},
	{ aHidd_Gfx_SyncTags	, (IPTR)tags_1600_1200	},
	{ TAG_DONE  	    	, 0UL 	    	    	}
    };
    
    struct TagItem mytags[] =
    {
	{ aHidd_Gfx_ModeTags	, (IPTR)mode_tags	},
	{ TAG_MORE  	    	, (IPTR)msg->attrList 	}
    };
    struct pRoot_New mymsg = { msg->mID, mytags };
    APTR display;
    ULONG vfreq, htotal, vtotal;

    EnterFunc(bug("GDIGfx::New()\n"));

    Forbid();

    /* TODO: If we replace "DISPLAY" with something else here,
       we get a possibility to select on which monitor to work.
       Perhaps we should have some kind of preferences for this. */
    display = GDICALL(CreateDC, "DISPLAY", NULL, NULL, NULL);
    if (!display) {
        Permit();
	ReturnPtr("GDIGfx::New()", OOP_Object *, NULL);
    }
    
    /* Fill in sync data */
    vfreq  = GDICALL(GetDeviceCaps, display, VREFRESH);
    htotal = GDICALL(GetDeviceCaps, display, HORZRES);
    vtotal = GDICALL(GetDeviceCaps, display, VERTRES);

    Permit();

    D(bug("[GDI] Display size is %u x %u, Vertical refresh rate is %d Hz\n", htotal, vtotal, vfreq));
    /* Sometimes Windows may refuse to provide us with the valid refresh rate.
       In this case it returns either 0 or 1 (which means "hardware default").
       Perhaps we should assume two standard VGA clocks for 0 and 1 ? */
    if (vfreq > 1)
        mode_tags[1].ti_Data = vfreq * htotal * vtotal;
    mode_tags[2].ti_Data = htotal;
    mode_tags[3].ti_Data = vtotal;

    /* Register gfxmodes */
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&mymsg);
    if (NULL != o)
    {
	struct gfx_data *data = OOP_INST_DATA(cl, o);

	D(bug("GDIGfx::New(): Got object from super\n"));
	data->display = display;
	NewList((struct List *)&data->bitmaps);
    }
    ReturnPtr("GDIGfx::New", OOP_Object *, o);
}

/********** GfxHidd::Dispose()  ******************************/
VOID GDICl__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct gfx_data *data;
    
    EnterFunc(bug("GDIGfx::Dispose(o=%p)\n", o));
    
    data = OOP_INST_DATA(cl, o);
    
    Forbid();
    if (data->fbwin)
        NATIVECALL(GDI_PutMsg, data->fbwin, WM_CLOSE, 0, 0);
    if (data->cursor)
        USERCALL(DestroyIcon, data->cursor);

    GDICALL(DeleteDC, data->display);

    D(bug("GDIGfx::Dispose: calling super\n"));    
    OOP_DoSuperMethod(cl, o, msg);
    
    ReturnVoid("GDIGfx::Dispose");
}

/****************************************************************************************/

OOP_Object *GDICl__Hidd_Gfx__NewBitMap(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_NewBitMap *msg)
{  
    HIDDT_ModeID		 modeid;
    struct pHidd_Gfx_NewBitMap   p;
    OOP_Object      	    	*newbm;
    HIDDT_StdPixFmt		 stdpf;
    
    struct gfx_data 	    	*data;
    struct TagItem  	    	 tags[] =
    {
    	{ aHidd_GDIBitMap_SysDisplay, 0UL }, /* 0 */
	{ TAG_IGNORE	    	    , 0UL }, /* 1 */
	{ TAG_IGNORE		    , 32  }, /* 2 */
	{ TAG_MORE  	    	    , 0UL }  /* 3 */
    };
    
    EnterFunc(bug("GDIGfx::NewBitMap()\n"));
    data = OOP_INST_DATA(cl, o);
    
    tags[0].ti_Data = (IPTR)data->display;
    tags[1].ti_Data = (IPTR)XSD(cl)->bmclass;
    tags[3].ti_Data = (IPTR)msg->attrList;

    /* Create a GDI bitmap if we have a valid ModeID.

       Also GDI bitmap can be created if there's no explicit
       pixelformat specification and a friend bitmap is supplied,
       which is a GDI bitmap. This is handled in the
       superclass.

       Some day when AROS learns to deal with several display drivers at once, this check may go
       away completely. This should really be handled by graphics.library. We do it here only because
       display bitmap classes are currently private and only drivers themselves know about them.
    */
    modeid = (HIDDT_ModeID)GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);
    stdpf = (HIDDT_StdPixFmt)GetTagData(aHidd_BitMap_StdPixFmt, vHidd_StdPixFmt_Unknown, msg->attrList);

    if (modeid != vHidd_ModeID_Invalid) {
        tags[1].ti_Tag  = aHidd_BitMap_ClassPtr;
	D(bug("[GDI] ModeID: 0x%08lX, ClassPtr: 0x%p\n", modeid, tags[1].ti_Data));
    }
    /* longword-align planar bitmaps. This is needed for BlitColorExpansion() to work properly. */
    if (stdpf == vHidd_StdPixFmt_Plane)
	tags[2].ti_Tag = aHidd_BitMap_Align;
	    
    p.mID = msg->mID;
    p.attrList = tags;
    newbm = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&p);
    ReturnPtr("GDIGfx::NewBitMap", OOP_Object *, newbm);
}

/****************************************************************************************/

VOID GDICl__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct gfx_data *data = OOP_INST_DATA(cl, o);
    ULONG   	     idx;
    
    if (IS_GFX_ATTR(msg->attrID, idx))
    {
    	switch (idx)
	{
	    case aoHidd_Gfx_IsWindowed:
	    case aoHidd_Gfx_SupportsHWCursor:
	    case aoHidd_Gfx_NoFrameBuffer:
	    	*msg->storage = (IPTR)TRUE;
		break;
	
	    default:
	    	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
		break;
	}
    }
    else
    {
    	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }

    return;
}

/****************************************************************************************/

static void AddToList(struct MinList *list, OOP_Object *bitmap)
{
    OOP_Class *bmclass = OOP_OCLASS(bitmap);
    struct bitmap_data *bmdata = OOP_INST_DATA(bmclass, bitmap);

    D(bug("[GDI] Will display bitmap data 0x%p, window 0x%p\n", bmdata, bmdata->window));
    if (bmdata->node.mln_Pred) {
	D(bug("[GDI] This bitmap is already on display\n"));
	Remove((struct Node *)bmdata);
    }
    AddTail((struct List *)list, (struct Node *)bmdata);
}

static void ShowList(struct gfx_data *data, struct MinList *list)
{
    struct bitmap_data *bmdata;
    BOOL empty = TRUE;

    /* If something left in displayed bitmaps list, close it */
    for (bmdata = (struct bitmap_data *)data->bitmaps.mlh_Head;
	bmdata->node.mln_Succ; bmdata = (struct bitmap_data *)bmdata->node.mln_Succ) {
	D(bug("[GDI] Hiding bitmap data 0x%p, window 0x%p\n", bmdata, bmdata->window));
	if (bmdata->window) {
	    NATIVECALL(GDI_PutMsg, bmdata->window, WM_CLOSE, 0, 0);
	    bmdata->window = NULL;
	}
	bmdata->node.mln_Pred = NULL;
    }

    /* Transfer the contents of our temporary list into display list */
    data->bitmaps.mlh_Head               = list->mlh_Head;
    data->bitmaps.mlh_Head->mln_Pred     = (struct MinNode *)&data->bitmaps.mlh_Head;
    data->bitmaps.mlh_TailPred           = list->mlh_TailPred;
    data->bitmaps.mlh_TailPred->mln_Succ = (struct MinNode *)&data->bitmaps.mlh_Tail;

    /* Now traverse through the new list and (re)open every bitmap's window. This will
	cause rearranging them in the correct Z-order */
    for (bmdata = (struct bitmap_data *)data->bitmaps.mlh_Head;
	bmdata->node.mln_Succ; bmdata = (struct bitmap_data *)bmdata->node.mln_Succ) {

	D(bug("[GDI] Showing bitmap data 0x%p, window 0x%p\n", bmdata, bmdata->window));
	SetSignal(0, SIGF_BLIT);
	NATIVECALL(GDI_PutMsg, data->fbwin, NOTY_SHOW, (IPTR)data, (IPTR)bmdata);
	Wait(SIGF_BLIT);

	empty = FALSE;
    }
    
    /* Close empty display window */
    if (empty && data->fbwin) {
        NATIVECALL(GDI_PutMsg, data->fbwin, WM_CLOSE, 0, 0);
	data->fbwin = NULL;
    }
}

ULONG GDICl__Hidd_Gfx__ShowViewPorts(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_ShowViewPorts *msg)
{
    struct gfx_data *data = OOP_INST_DATA(cl, o);
    struct Task *me = FindTask(NULL);;
    struct HIDD_ViewPortData *vpdata;
    void *gfx_int;
    struct MinList new_bitmaps;

    gfx_int = KrnAddIRQHandler(XSD(cl)->ctl->GfxIrq, GfxIntHandler, data, me);
    if (gfx_int) {

	NewList((struct List *)&new_bitmaps);
	Forbid();

	/* Traverse through bitmaps chain and move them from old displayed list to the new temporary one */
	for (vpdata = msg->Data; vpdata; vpdata = vpdata->Next)
	    AddToList(&new_bitmaps, vpdata->Bitmap);
	ShowList(data, &new_bitmaps);
	
	Permit();
	KrnRemIRQHandler(gfx_int);
    }
    /* We always return TRUE in order to indicate that we support this method.
       LoadView() has no return code, so we have no rights for error too. */
    return TRUE;
}

/****************************************************************************************/

OOP_Object *GDICl__Hidd_Gfx__Show(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_Show *msg)
{
    struct gfx_data *data = OOP_INST_DATA(cl, o);
    struct Task *me = FindTask(NULL);;
    void *gfx_int;
    struct MinList new_bitmaps;

    gfx_int = KrnAddIRQHandler(XSD(cl)->ctl->GfxIrq, GfxIntHandler, data, me);
    if (gfx_int) {

	NewList((struct List *)&new_bitmaps);
	Forbid();

	if (msg->bitMap)
	    AddToList(&new_bitmaps, msg->bitMap);
	ShowList(data, &new_bitmaps);

	Permit();
	KrnRemIRQHandler(gfx_int);
    }

    return msg->bitMap;
}

/****************************************************************************************/

extern ULONG Copy_DrawModeTable[];

VOID GDICl__Hidd_Gfx__CopyBox(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CopyBox *msg)
{
    APTR src = NULL, dest = NULL;
    ULONG drmd;
    struct gfx_data *data;
    IPTR xoffset, yoffset;
    
    EnterFunc(bug("[GDI] hidd.gfx.wingdi::CopyBox(0x%p(%lu, %lu, %lu, %lu) -> 0x%p(%lu, %lu)\n", msg->src, msg->srcX, msg->srcY, msg->width, msg->height,
    		  msg->dest, msg->destX, msg->destY));
    data = OOP_INST_DATA(cl, o);
    
    OOP_GetAttr(msg->src,  aHidd_GDIBitMap_DeviceContext, (IPTR *)&src);
    OOP_GetAttr(msg->dest, aHidd_GDIBitMap_DeviceContext, (IPTR *)&dest);
    OOP_GetAttr(msg->dest, aHidd_BitMap_LeftEdge, &xoffset);
    OOP_GetAttr(msg->dest, aHidd_BitMap_TopEdge, &yoffset);
	
    if (NULL == dest || NULL == src)
    {
        D(bug("[GDI] Process by superclass\n"));
	/* The destination object is not a GDI bitmap.
	    Let the superclass do the copying in a more general way
	*/
	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	
	return;
    }

    drmd = GC_DRMD(msg->gc);
    Forbid();
    GDICALL(BitBlt, dest, msg->destX, msg->destY, msg->width, msg->height, src, msg->srcX, msg->srcY, Copy_DrawModeTable[drmd]);
    Permit();
    
}

/****************************************************************************************/

BOOL GDICl__Hidd_Gfx__SetCursorShape(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorShape *msg)
{
    struct gfx_data *data = OOP_INST_DATA(cl, o);
    OOP_Object *pfmt;
    OOP_Object *colormap;
    IPTR depth;
    HIDDT_Color color;
    IPTR width, height, x, y;
    ULONG *buf, *mask;
    ULONG bufsize;
    APTR buf_bm, mask_bm;
    APTR cursor = NULL;
    
    OOP_GetAttr(msg->shape, aHidd_BitMap_Width, &width);
    OOP_GetAttr(msg->shape, aHidd_BitMap_Height, &height);
    OOP_GetAttr(msg->shape, aHidd_BitMap_PixFmt, (APTR)&pfmt);
    OOP_GetAttr(pfmt, aHidd_PixFmt_Depth, &depth);
    OOP_GetAttr(msg->shape, aHidd_BitMap_ColorMap, (APTR)&colormap);
    
    bufsize = width * height * 4;
    buf = AllocMem(bufsize, MEMF_ANY);
    if (buf) {
        mask = AllocMem(bufsize, MEMF_ANY);
	if (mask) {
	    BITMAP bm;
	    ULONG i;

	    HIDD_BM_GetImage(msg->shape, (UBYTE *)buf, width * 4, 0, 0, width, height, vHidd_StdPixFmt_ARGB32_Native);
	    PRINT_POINTER(buf, width, 8, 8);
	    /* Construct the mask from alpha channel data. The mask will be used on pre-XP systems or
	       on LUT screens. Of course there'll be no alpha blending there. */
	    for (i = 0; i < width * height; i++)
	        mask[i] = (buf[i] & 0xFF000000) ? 0 : 0xFFFFFFFF;

	    bm.bmType = 0;
	    bm.bmWidth = width;
	    bm.bmHeight = height;
	    bm.bmWidthBytes = width * 4;
            bm.bmPlanes = 1;
	    bm.bmBitsPixel = 32;
	    bm.bmBits = buf;
	    Forbid();
	    buf_bm = GDICALL(CreateBitmapIndirect, &bm);
	    if (buf_bm) {
	        bm.bmBits = mask;
	        mask_bm = GDICALL(CreateBitmapIndirect, &bm);
		if (mask_bm) {
		    ICONINFO curs = {
		        FALSE,
			-msg->xoffset, -msg->yoffset,
			mask_bm, buf_bm
		    };
		    cursor = USERCALL(CreateIconIndirect, &curs);
		    if (cursor) {
		        APTR old_cursor = data->cursor;
			
			D(bug("[GDI] Created cursor 0x%p, old cursor 0x%p\n", cursor, old_cursor));
			data->cursor = cursor;
			USERCALL(SetCursor, cursor);
		        if (old_cursor)
			    USERCALL(DestroyIcon, old_cursor);
		    }
		    GDICALL(DeleteObject, mask_bm);
		}
		GDICALL(DeleteObject, buf_bm);
	    }
	    Permit();
	    FreeMem(mask, bufsize);
	}
	FreeMem(buf, bufsize);
    }
    return cursor ? TRUE : FALSE;
}

/****************************************************************************************/

/* This is simple - all modes have the same properties */
static struct HIDD_ModeProperties mode_props = {
    DIPF_IS_SPRITES,
    1,
    COMPF_ABOVE|COMPF_BELOW|COMPF_LEFT|COMPF_RIGHT
};

ULONG GDICl__Hidd_Gfx__ModeProperties(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_ModeProperties *msg)
{
    ULONG len = msg->propsLen;

    if (len > sizeof(mode_props))
        len = sizeof(mode_props);
    CopyMem(&mode_props, msg->props, len);

    return len;
}

/****************************************************************************************/

static int gdigfx_init(LIBBASETYPEPTR LIBBASE) 
{
    InitSemaphore(&LIBBASE->xsd.sema);

    if (OOP_ObtainAttrBases(attrbases)) {
        D(bug("[GDI] Starting up GDI controller\n"));

	Forbid();
        LIBBASE->xsd.ctl = NATIVECALL(GDI_Init);
	Permit();
	
	if (LIBBASE->xsd.ctl)
	    return TRUE;
    }
    return FALSE;
}

/****************************************************************************************/

static int gdigfx_expunge(LIBBASETYPEPTR LIBBASE)
{
    if (LIBBASE->xsd.ctl) {
        Forbid();
	NATIVECALL(GDI_Shutdown, LIBBASE->xsd.ctl);
	Permit();
    }
    OOP_ReleaseAttrBases(attrbases);

    return TRUE;
}

/****************************************************************************************/

ADD2INITLIB(gdigfx_init, 0);
ADD2EXPUNGELIB(gdigfx_expunge, 0);
