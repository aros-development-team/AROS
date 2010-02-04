/*
    Copyright  1995-2010, The AROS Development Team. All rights reserved.
    $Id: gdigfx.c 29127 2008-08-10 20:13:22Z sonic $

    Desc: GDI gfx HIDD for AROS.
    Lang: English.
*/

#define __OOP_NOATTRBASES__

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <signal.h>
#include <string.h>

#include <exec/libraries.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <aros/libcall.h>
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


static VOID cleanupgdistuff(struct gdi_staticdata *xsd);
static BOOL initgdistuff(struct gdi_staticdata *xsd);

void GfxIntHandler(struct gfx_data *data, struct Task *task)
{
    Signal(task, SIGF_BLIT);
}

/****************************************************************************************/

OOP_Object *GDICl__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct TagItem pftags[] =
    {
    	{ aHidd_PixFmt_RedShift     , 0	    }, /* 0 */
	{ aHidd_PixFmt_GreenShift   , 0	    }, /* 1 */
	{ aHidd_PixFmt_BlueShift    , 0	    }, /* 2 */
	{ aHidd_PixFmt_AlphaShift   , 0	    }, /* 3 */
	{ aHidd_PixFmt_RedMask	    , 0	    }, /* 4 */
	{ aHidd_PixFmt_GreenMask    , 0	    }, /* 5 */
	{ aHidd_PixFmt_BlueMask     , 0	    }, /* 6 */
	{ aHidd_PixFmt_AlphaMask    , 0	    }, /* 7 */
	{ aHidd_PixFmt_ColorModel   , 0	    }, /* 8 */
	{ aHidd_PixFmt_Depth	    , 0	    }, /* 9 */
	{ aHidd_PixFmt_BytesPerPixel, 0	    }, /* 10 */
	{ aHidd_PixFmt_BitsPerPixel , 0	    }, /* 11 */
	{ aHidd_PixFmt_StdPixFmt    , 0	    }, /* 12 */
	{ aHidd_PixFmt_CLUTShift    , 0	    }, /* 13 */
	{ aHidd_PixFmt_CLUTMask     , 0	    }, /* 14 */ 
	{ aHidd_PixFmt_BitMapType   , 0	    }, /* 15 */   
	{ TAG_DONE  	    	    , 0UL   } 
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
	{ aHidd_Sync_PixelClock , 100000000	    	}, /* Oh boy, this pixelclock is fast ;-) */
	{ aHidd_Sync_LeftMargin , 0		    	},
	{ aHidd_Sync_RightMargin, 0		    	},
	{ aHidd_Sync_HSyncLength, 0		    	},
	{ aHidd_Sync_UpperMargin, 0		    	},
	{ aHidd_Sync_LowerMargin, 0		    	},
	{ aHidd_Sync_VSyncLength, 0		    	},
	{ aHidd_Sync_HMin	, 112			}, /* In fact these can be even smaller, and */
	{ aHidd_Sync_VMin	, 112			}, /* maximum can be even bigger...	     */
	{ aHidd_Sync_HMax	, 16384			},
	{ aHidd_Sync_VMax	, 16384			},

	
	/* The different syncmodes. The default attribute values above 
	    will be applied to each of these. Note that
	    you can alter the defaults between the tags bewlow 
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

    EnterFunc(bug("GDIGfx::New()\n"));

    /* Do GfxHidd initalization here */
    if (!initgdistuff(XSD(cl)))
    {
	D(kprintf("!!! initgdistuff() FAILED IN GDIGfx::New() !!!\n"));
	ReturnPtr("GDIGfx::New()", OOP_Object *, NULL);
    }
	
    /* Register gfxmodes */
    pftags[0].ti_Data = XSD(cl)->red_shift;
    pftags[1].ti_Data = XSD(cl)->green_shift;
    pftags[2].ti_Data = XSD(cl)->blue_shift;
    pftags[3].ti_Data = 0;
	    
    pftags[4].ti_Data = XSD(cl)->red_mask;
    pftags[5].ti_Data = XSD(cl)->green_mask;
    pftags[6].ti_Data = XSD(cl)->blue_mask;
    pftags[7].ti_Data = 0x00000000;
	    
    if (XSD(cl)->depth == 32)
    {
        pftags[8].ti_Data = vHidd_ColorModel_TrueColor;
    }
/*  else
    {
	pftags[8].ti_Data = vHidd_ColorModel_Palette;
        pftags[13].ti_Data = XSD(cl)->clut_shift;
	pftags[14].ti_Data = XSD(cl)->clut_mask;		
    }*/
    pftags[9].ti_Data = XSD(cl)->depth;
    pftags[10].ti_Data = XSD(cl)->depth >> 3;
    pftags[11].ti_Data = XSD(cl)->depth;
    pftags[12].ti_Data = vHidd_StdPixFmt_Native;
    /* We assume chunky */
    pftags[15].ti_Data = vHidd_BitMapType_Chunky;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&mymsg);
    if (NULL != o)
    {
	struct gfx_data *data = OOP_INST_DATA(cl, o);

	D(bug("GDIGfx::New(): Got object from super\n"));
	data->display = XSD(cl)->display;
    }
    ReturnPtr("GDIGfx::New", OOP_Object *, o);
}

/********** GfxHidd::Dispose()  ******************************/
VOID GDICl__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct gfx_data *data;
    
    EnterFunc(bug("GDIGfx::Dispose(o=%p)\n", o));
    
    data = OOP_INST_DATA(cl, o);
    
    if (data->cursor) {
        Forbid();
        USERCALL(DestroyIcon, data->cursor);
	Permit();
    }
    
    cleanupgdistuff(XSD(cl));

    D(bug("GDIGfx::Dispose: calling super\n"));    
    OOP_DoSuperMethod(cl, o, msg);
    
    ReturnVoid("GDIGfx::Dispose");
}

/****************************************************************************************/

OOP_Object *GDICl__Hidd_Gfx__NewBitMap(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_NewBitMap *msg)
{
    BOOL    	    	    	 displayable, framebuffer;    
    HIDDT_ModeID		 modeid;
    struct pHidd_Gfx_NewBitMap   p;
    OOP_Object      	    	*newbm;
    OOP_Object		  	*friend;
    HIDDT_StdPixFmt		 stdpf;
    
    struct gfx_data 	    	*data;
    struct TagItem  	    	 tags[] =
    {
    	{ aHidd_GDIBitMap_SysDisplay   , 0UL },	/* 0 */
	{ TAG_IGNORE	    	       , 0UL },	/* 1 */
	{ TAG_MORE  	    	       , 0UL }  /* 2 */
    };
    
    EnterFunc(bug("GDIGfx::NewBitMap()\n"));
    data = OOP_INST_DATA(cl, o);
    
    /* Never create a framebuffer */
    framebuffer = GetTagData(aHidd_BitMap_FrameBuffer, FALSE, msg->attrList);	
    if (framebuffer)
    {
        D(bug("[GDI] Attempt to create a framebuffer, we don't want it\n"));
        return NULL;
    }

    tags[0].ti_Data = (IPTR)data->display;
    tags[1].ti_Data = (IPTR)XSD(cl)->bmclass;
    tags[2].ti_Data = (IPTR)msg->attrList;

    /* When do we create a GDI bitmap ?
	- If the bitmap is displayable
	- If the user supplied a modeid
	- For 1-plane bitmaps
	- Bitmaps that have a friend that is a GDI bitmap
	  and there is no standard pixfmt supplied */
    displayable = GetTagData(aHidd_BitMap_Displayable, FALSE, msg->attrList);
    modeid = (HIDDT_ModeID)GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);
    friend = (OOP_Object *)GetTagData(aHidd_BitMap_Friend, 0, msg->attrList);
    stdpf = (HIDDT_StdPixFmt)GetTagData(aHidd_BitMap_StdPixFmt, vHidd_StdPixFmt_Unknown, msg->attrList);

    if (displayable || (modeid != vHidd_ModeID_Invalid) || (stdpf == vHidd_StdPixFmt_Plane)) {
        tags[1].ti_Tag  = aHidd_BitMap_ClassPtr;
	D(bug("[GDI] Displayable: %d, ModeID: 0x%08lX\n", displayable, modeid));
    } else if (friend && (stdpf == vHidd_StdPixFmt_Unknown)) {
        OOP_Object *gfxhidd;

        OOP_GetAttr(friend, aHidd_BitMap_GfxHidd, (APTR)&gfxhidd);
	if (gfxhidd == o) {
	    tags[1].ti_Tag  = aHidd_BitMap_ClassPtr;
	}
    }
    D(if (tags[1].ti_Tag == aHidd_BitMap_ClassPtr) bug("[GDI] Creating GDI bitmap, ClassPtr is %p\n", tags[1].ti_Data);)
	    
    /* !!! IMPORTANT !!! */
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

OOP_Object *GDICl__Hidd_Gfx__Show(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_Show *msg)
{
    struct gfx_data *data;
    struct Task *me;
    void *gfx_int;
    IPTR bm_win_tags[] = {aHidd_GDIBitMap_Window, 0, TAG_DONE};
	
    data = OOP_INST_DATA(cl, o);

    D(bug("[GDI] hidd.gfx.wingdi::Show(0x%p)\n", msg->bitMap));

    me = FindTask(NULL);
    gfx_int = KrnAddIRQHandler(XSD(cl)->ctl->IrqNum, GfxIntHandler, data, me);
    if (gfx_int) {
	IPTR bmdata = 0;

	Forbid();

	if (data->bitmap) {
	    D(bug("[GDI] Show(): old displayed bitmap 0x%p\n", data->bitmap));
	    bm_win_tags[1] = 0;
	    OOP_SetAttrs(data->bitmap, (struct TagItem *)bm_win_tags);
	}

	/* It's quite not easy to call AROS API from within window service thread,
	   so we pass private data of our bitmap class to it directly.
	   Don't use such tricks in normal AROS code, this isn't really good. */
	if (msg->bitMap)
	    bmdata = OOP_INST_DATA(XSD(cl)->bmclass, msg->bitMap);
	data->bitmap = msg->bitMap;

    	/* Hosted system has no real blitter, however we have host-side window service thread that does some work asynchronously,
	   and this looks like a real blitter. So we use this signal. Before we do it we ensure that it's reset (because it's
	   the same as SIGF_SINGLE) */
	SetSignal(0, SIGF_BLIT);
	Forbid();
	NATIVECALL(GDI_PutMsg, data->fbwin, NOTY_SHOW, (IPTR)data, bmdata);
	Permit();
	Wait(SIGF_BLIT);

	if (msg->bitMap) {
	    bm_win_tags[1] = (IPTR)data->fbwin;
	    OOP_SetAttrs(msg->bitMap, (struct TagItem *)bm_win_tags);
	}

	Permit();
	KrnRemIRQHandler(gfx_int);
	return msg->bitMap;
    }
    return NULL;

}

/****************************************************************************************/

/* Raster operations for a background */
static ULONG BitmapCopy_DrawModeTable[] = {
    BLACKNESS,
    SRCAND,  	 /* DSa  - src AND dest       */
    SRCERASE,	 /* SDna - src AND NOT dest   */
    SRCCOPY,	 /* S    - src		      */
    0x00220326,  /* DSna - NOT src AND dest   */
    0x00AA0029,  /* D    - dest		      */
    SRCINVERT,   /* DSx  - src XOR dest	      */
    SRCPAINT,	 /* DSo  - src OR dest	      */
    NOTSRCERASE, /* DSon - NOT (src OR dest)  */
    0x00990066,  /* DSxn - NOT (src XOR dest) */
    DSTINVERT,	 /* Dn   - NOT dest	      */
    0x00DD0228,  /* SDno - src OR NOT dest    */
    NOTSRCCOPY,	 /* Sn   - NOT src	      */
    MERGEPAINT,  /* DSno - NOT src OR dest    */
    0x007700E6,  /* DSan - NOT (src AND dest) */
    WHITENESS
};

VOID GDICl__Hidd_Gfx__CopyBox(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CopyBox *msg)
{
    APTR src = NULL, dest = NULL;
    APTR wnd;
    ULONG drmd;
    struct gfx_data *data;
    
    EnterFunc(bug("[GDI] hidd.gfx.wingdi::CopyBox(0x%p(%lu, %lu, %lu, %lu) -> 0x%p(%lu, %lu)\n", msg->src, msg->srcX, msg->srcY, msg->width, msg->height,
    		  msg->dest, msg->destX, msg->destY));
    data = OOP_INST_DATA(cl, o);
    
    OOP_GetAttr(msg->src,  aHidd_GDIBitMap_DeviceContext, (IPTR *)&src);
    OOP_GetAttr(msg->dest, aHidd_GDIBitMap_DeviceContext, (IPTR *)&dest);
	
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
    OOP_GetAttr(msg->dest, aHidd_GDIBitMap_Window, (IPTR *)&wnd);
    GDICALL(BitBlt, dest, msg->destX, msg->destY, msg->width, msg->height, src, msg->srcX, msg->srcY, BitmapCopy_DrawModeTable[drmd]);
    if (wnd) {
        RECT r = {msg->destX, msg->destY, msg->destX + msg->width, msg->destY + msg->height};

        D(bug("[GDI] CopyBox(): Refresh\n"));
        USERCALL(RedrawWindow, wnd, &r, NULL, RDW_INVALIDATE|RDW_UPDATENOW);
    }
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

#if AROS_BIG_ENDIAN
	    HIDD_BM_GetImage(msg->shape, (UBYTE *)buf, width * 4, 0, 0, width, height, vHidd_StdPixFmt_ARGB32);
#else
	    HIDD_BM_GetImage(msg->shape, (UBYTE *)buf, width * 4, 0, 0, width, height, vHidd_StdPixFmt_BGRA32);
#endif
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
		        D(bug("[GDI] Created cursor 0x%p\n", cursor));
			XSD(cl)->ctl->cursor = cursor;
			USERCALL(SetCursor, cursor);
		        if (data->cursor) {
			    D(bug("[GDI] Deleting old cursor 0x%p\n", data->cursor));
			    USERCALL(DestroyIcon, data->cursor);
			}
			data->cursor = cursor;
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

/*
   Inits sysdisplay, sysscreen, colormap, etc.. */
static BOOL initgdistuff(struct gdi_staticdata *xsd)
{
    BOOL    	     ok = TRUE;
    int     	     template_mask;

    EnterFunc(bug("initgdistuff()\n"));

    Forbid();
    xsd->depth = GDICALL(GetDeviceCaps, xsd->display, BITSPIXEL);
    Permit();
    D(bug("Screen depth: %lu\n", xsd->depth));
/*  if (xsd->depth == 32) {*/
	/* Get the pixel masks */
	xsd->red_mask    = 0x000000FF;
	xsd->green_mask  = 0x0000FF00;
	xsd->blue_mask   = 0x00FF0000;
	/* Shifts are non-obviously calculated from the MSB, not from the LSB.
	   I. e. color value is placed in the most significant byte of the ULONG
	   before shifting (cc000000, not 000000cc) */
	xsd->red_shift	 = 24;
	xsd->green_shift = 16;
	xsd->blue_shift	 = 8;
/*  } else {
	kprintf("!!! GFX HIDD only supports truecolor diplays for now !!!\n");
	return FALSE;
    }*/
    ReturnBool("initgdistuff", TRUE);
}

/****************************************************************************************/

static VOID cleanupgdistuff(struct gdi_staticdata *xsd)
{
    /* Do nothing for now */
}

/****************************************************************************************/

static int gdigfx_init(LIBBASETYPEPTR LIBBASE) 
{
    return OOP_ObtainAttrBases(attrbases);
}

/****************************************************************************************/

static int gdigfx_expunge(LIBBASETYPEPTR LIBBASE)
{
    OOP_ReleaseAttrBases(attrbases);
    return TRUE;
}

/****************************************************************************************/

ADD2INITLIB(gdigfx_init, 0);
ADD2EXPUNGELIB(gdigfx_expunge, 0);

/****************************************************************************************/
