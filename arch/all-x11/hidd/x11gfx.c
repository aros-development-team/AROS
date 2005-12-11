/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$

    Desc: X11 gfx HIDD for AROS.
    Lang: English.
*/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/Xutil.h>

#include <signal.h>
#include <string.h>

#include <exec/libraries.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <aros/libcall.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <oop/oop.h>

#include <hidd/hidd.h>
#include <hidd/graphics.h>

#include <aros/symbolsets.h>

#include "x11gfx_intern.h"
#include "x11.h"
#include "bitmap.h"

#include LC_LIBDEFS_FILE

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>


/****************************************************************************************/

#define IS_X11GFX_ATTR(attr, idx) ( ( (idx) = (attr) - HiddX11GfxAB) < num_Hidd_X11Gfx_Attrs)


/* Some attrbases needed as global vars.
  These are write-once read-many */

static OOP_AttrBase HiddBitMapAttrBase;  
static OOP_AttrBase HiddX11GfxAB;
static OOP_AttrBase HiddX11BitMapAB;
static OOP_AttrBase HiddSyncAttrBase;
static OOP_AttrBase HiddPixFmtAttrBase;
static OOP_AttrBase HiddGfxAttrBase;

static struct OOP_ABDescr attrbases[] =
{
    { IID_Hidd_BitMap	, &HiddBitMapAttrBase	},
    { IID_Hidd_X11Gfx	, &HiddX11GfxAB		},
    { IID_Hidd_X11BitMap, &HiddX11BitMapAB	},
    { IID_Hidd_Sync 	, &HiddSyncAttrBase	},
    { IID_Hidd_PixFmt	, &HiddPixFmtAttrBase	},
    { IID_Hidd_Gfx  	, &HiddGfxAttrBase	},
    { NULL  	    	, NULL      	    	}
};


static VOID cleanupx11stuff(struct x11_staticdata *xsd);
static BOOL initx11stuff(struct x11_staticdata *xsd);

/****************************************************************************************/

OOP_Object *X11Cl__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
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
    	{ aHidd_Sync_HDisp  	, 160 	    	    	},
	{ aHidd_Sync_VDisp  	, 160 	    	    	},
	{ aHidd_Sync_Description, (IPTR)"X11:160x160" 	},
	{ TAG_DONE  	    	, 0UL 	    	    	}
    };
    
    struct TagItem tags_240_320[] =
    {
    	{ aHidd_Sync_HDisp  	, 240 	    	    	},
	{ aHidd_Sync_VDisp  	, 320 	    	    	},
	{ aHidd_Sync_Description, (IPTR)"X11:240x320" 	},
	{ TAG_DONE  	    	, 0UL 	    	    	} 
    };

    struct TagItem tags_320_240[] = 
    {
    	{ aHidd_Sync_HDisp  	, 320 	    	    	},
	{ aHidd_Sync_VDisp  	, 240 	    	    	},
	{ aHidd_Sync_Description, (IPTR)"X11:320x240" 	},
	{ TAG_DONE  	    	, 0UL 	    	    	}
    };

    struct TagItem tags_512_384[] = 
    {
    	{ aHidd_Sync_HDisp  	, 512 	    	    	},
	{ aHidd_Sync_VDisp  	, 384 	    	    	},
	{ aHidd_Sync_Description, (IPTR)"X11:512x384" 	},
	{ TAG_DONE  	    	, 0UL 	    	    	}
    };

    struct TagItem tags_640_480[] = 
    {
    	{ aHidd_Sync_HDisp  	, 640 	    	    	},
	{ aHidd_Sync_VDisp  	, 480 	    	    	},
	{ aHidd_Sync_Description, (IPTR)"X11:640x480" 	},
	{ TAG_DONE  	    	, 0UL 	    	    	}
    };

    struct TagItem tags_800_600[] = 
    {
    	{ aHidd_Sync_HDisp  	, 800 	    	    	},
	{ aHidd_Sync_VDisp  	, 600 	    	    	},
	{ aHidd_Sync_Description, (IPTR)"X11:800x600" 	},
	{ TAG_DONE  	    	, 0UL 	    	    	}
    };

    struct TagItem tags_1024_768[] = 
    {
    	{ aHidd_Sync_HDisp  	, 1024      	    	},
	{ aHidd_Sync_VDisp  	, 768       	    	},
	{ aHidd_Sync_Description, (IPTR)"X11:1024x768"  },
	{ TAG_DONE  	    	, 0UL       	    	}
    };
    
    struct TagItem tags_1152_864[] = 
    {
    	{ aHidd_Sync_HDisp  	, 1152      	    	},
	{ aHidd_Sync_VDisp  	, 864       	    	},
	{ aHidd_Sync_Description, (IPTR)"X11:1152x864"  },
	{ TAG_DONE  	    	, 0UL       	    	}
    };
    
    struct TagItem tags_1280_960[] = 
    {
    	{ aHidd_Sync_HDisp  	, 1280      	    	},
	{ aHidd_Sync_VDisp  	, 960       	    	},
	{ aHidd_Sync_Description, (IPTR)"X11:1280x960"  },
	{ TAG_DONE  	    	, 0UL       	    	}
    };
    
    struct TagItem tags_1280_1024[] =
    {
    	{ aHidd_Sync_HDisp  	, 1280      	    	},
	{ aHidd_Sync_VDisp  	, 1024      	    	},
	{ aHidd_Sync_Description, (IPTR)"X11:1280x1024" },
	{ TAG_DONE  	    	, 0UL       	    	}
    };
    
    struct TagItem tags_1600_1200[] = 
    {
    	{ aHidd_Sync_HDisp  	, 1600      	    	},
	{ aHidd_Sync_VDisp  	, 1200      	    	},
	{ aHidd_Sync_Description, (IPTR)"X11:1600x1200" },
	{ TAG_DONE  	    	, 0UL       	    	}
    };
    
    struct TagItem mode_tags[] =
    {
	{ aHidd_Gfx_PixFmtTags	, (IPTR)pftags		},
	
	/* Default values for the sync attributes */
	{ aHidd_Sync_PixelClock , 100000000	    	}, /* Oh boy,  this X11 pixelclock is fast ;-) */
	{ aHidd_Sync_LeftMargin , 0		    	},
	{ aHidd_Sync_RightMargin, 0		    	},
	{ aHidd_Sync_HSyncLength, 0		    	},
	{ aHidd_Sync_UpperMargin, 0		    	},
	{ aHidd_Sync_LowerMargin, 0		    	},
	{ aHidd_Sync_VSyncLength, 0		    	},
	
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
    struct x11_staticdata *xsd = NULL;

    EnterFunc(bug("X11Gfx::New()\n"));

    /* Do GfxHidd initalization here */
    if (!initx11stuff(XSD(cl)))
    {
	kprintf("!!! initx11stuff() FAILED IN X11Gfx::New() !!!\n");
	ReturnPtr("X11Gfx::New()", OOP_Object *, NULL);
    }
	
    /* Register gfxmodes */
    pftags[0].ti_Data = XSD(cl)->red_shift;
    pftags[1].ti_Data = XSD(cl)->green_shift;
    pftags[2].ti_Data = XSD(cl)->blue_shift;
    pftags[3].ti_Data = 0;
	    
    pftags[4].ti_Data = XSD(cl)->vi.red_mask;
    pftags[5].ti_Data = XSD(cl)->vi.green_mask;
    pftags[6].ti_Data = XSD(cl)->vi.blue_mask;
    pftags[7].ti_Data = 0x00000000;
	    
    if (XSD(cl)->vi.class == TrueColor)
    {
        pftags[8].ti_Data = vHidd_ColorModel_TrueColor;
    }
    else if (XSD(cl)->vi.class == PseudoColor)
    {
	pftags[8].ti_Data = vHidd_ColorModel_Palette;
        pftags[13].ti_Data = XSD(cl)->clut_shift;
	pftags[14].ti_Data = XSD(cl)->clut_mask;		
    }
    else
    {
	kprintf("!!! UNHANDLED COLOR MODEL IN X11Gfx:New(): %d !!!\n", XSD(cl)->vi.class);
	cleanupx11stuff(xsd);
	ReturnPtr("X11Gfx::New", OOP_Object *, NULL);
    }
	    
    pftags[9].ti_Data = XSD(cl)->depth;
    pftags[10].ti_Data = XSD(cl)->bytes_per_pixel;
    pftags[11].ti_Data = XSD(cl)->depth;
    pftags[12].ti_Data = vHidd_StdPixFmt_Native;
    
    #warning Do better than this

    /* We assume chunky */
    pftags[15].ti_Data = vHidd_BitMapType_Chunky;
    
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&mymsg);
    if (NULL != o)
    {
	XColor      	 bg, fg;
	struct gfx_data *data = OOP_INST_DATA(cl, o);
	
    	LOCK_X11
	data->display	= XSD(cl)->display;
	data->screen	= DefaultScreen( data->display );
	data->depth	= DisplayPlanes( data->display, data->screen );
	data->colmap	= DefaultColormap( data->display, data->screen );
	/* Create cursor */
        data->cursor = XCreateFontCursor( data->display, XC_top_left_arrow);

	fg.pixel = BlackPixel(data->display, data->screen);
	fg.red = 0x0000; fg.green = 0x0000; fg.blue = 0x0000;
	fg.flags = (DoRed | DoGreen | DoBlue);
	bg.pixel = WhitePixel(data->display, data->screen);
	bg.red = 0xFFFF; bg.green = 0xFFFF; bg.blue = 0xFFFF;
	bg.flags = (DoRed | DoGreen | DoBlue);

	XRecolorCursor(data->display, data->cursor, &fg, &bg);
	
	switch(DoesBackingStore(ScreenOfDisplay(data->display, data->screen)))
	{
	    case WhenMapped:
   	    case Always:
	    	break;
		
	    case NotUseful:
	    	bug("\n"
		    "+----------------------------------------------------------|\n"
		    "| Your X Server seems to have backing store disabled!      |\n"
		    "| ===================================================      |\n"
		    "|                                                          |\n"
		    "| If possible you should try to switch it on, otherwise    |\n"
		    "| AROS will have problems with it's display. When AROS     |\n"
		    "| X window is hidden by other X windows, or is dragged     |\n"
		    "| off screen, then the gfx in those parts will get lost,   |\n"
		    "| unless backing store support is enabled.                 |\n"
		    "|                                                          |\n"
		    "| In case your X11 Server is XFree 4.x then switching on   |\n"
		    "| backingstore support can be done by starting the X11     |\n"
		    "| server with something like \"startx -- +bs\". Depending    |\n"
		    "| on what gfxcard driver you use it might also be possible |\n"
		    "| to turn it on by adding                                  |\n"
		    "|                                                          |\n"
		    "|         Option \"Backingstore\"                            |\n"
		    "|                                                          |\n"
		    "| to the Device Section of your XFree86 config file, which |\n"
		    "| usually is \"/etc/X11/XF86Config\".                        |\n"
		    "+----------------------------------------------------------+\n"
		    "\n");
	    	break;
		
	}
	
    	UNLOCK_X11
	
	D(bug("X11Gfx::New(): Got object from super\n"));

	data->display = XSD(cl)->display;
	
    }
    ReturnPtr("X11Gfx::New", OOP_Object *, o);
}

/********** GfxHidd::Dispose()  ******************************/
VOID X11Cl__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct gfx_data *data;
    
    EnterFunc(bug("X11Gfx::Dispose(o=%p)\n", o));
    
    data = OOP_INST_DATA(cl, o);    
    cleanupx11stuff(XSD(cl));

    D(bug("X11Gfx::Dispose: calling super\n"));    
    OOP_DoSuperMethod(cl, o, msg);
    
    ReturnVoid("X11Gfx::Dispose");
}

/****************************************************************************************/

OOP_Object *X11Cl__Hidd_Gfx__NewBitMap(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_NewBitMap *msg)
{
    BOOL    	    	    	 displayable, framebuffer;    
    struct pHidd_Gfx_NewBitMap   p;
    OOP_Object      	    	*newbm;
    IPTR    	    	    	 drawable;
    
    struct gfx_data 	    	*data;
    struct TagItem  	    	 tags[] =
    {
    	{ aHidd_X11Gfx_SysDisplay   , (IPTR) NULL   },	/* 0 */
	{ aHidd_X11Gfx_SysScreen    , 0UL   	    },	/* 1 */	
	{ aHidd_X11Gfx_SysCursor    , 0UL   	    },	/* 2 */
	{ aHidd_X11Gfx_ColorMap     , 0UL   	    },	/* 3 */
	{ aHidd_X11Gfx_VisualClass  , 0UL   	    },	/* 4 */
	{ TAG_IGNORE	    	    , 0UL   	    },	/* 5 */
	{ TAG_MORE  	    	    , (IPTR) NULL   }   /* 6 */
    };
    
    EnterFunc(bug("X11Gfx::NewBitMap()\n"));
    
    data = OOP_INST_DATA(cl, o);
    
    tags[0].ti_Data = (IPTR)data->display;
    tags[1].ti_Data = data->screen;
    tags[2].ti_Data = (IPTR)data->cursor;
    tags[3].ti_Data = data->colmap;
    tags[4].ti_Data = XSD(cl)->vi.class;
    tags[6].ti_Data = (IPTR)msg->attrList;
    
    /* Displayable bitmap ? */
    displayable = GetTagData(aHidd_BitMap_Displayable, FALSE, msg->attrList);
    framebuffer = GetTagData(aHidd_BitMap_FrameBuffer, FALSE, msg->attrList);
    
    if (framebuffer)
    {
    	tags[5].ti_Tag	= aHidd_BitMap_ClassPtr;
	tags[5].ti_Data	= (IPTR)XSD(cl)->onbmclass;
    }
    else if (displayable)
    {
    	tags[5].ti_Tag	= aHidd_BitMap_ClassPtr;
	tags[5].ti_Data	= (IPTR)XSD(cl)->offbmclass;
    }
    else
    {
    	/* When do we create an x11 offscreen bitmap ?
	    - For 1-plane bitmaps.
	    - Bitmaps that have a friend that is an X11 bitmap
	      and there is no standard pixfmt supplied
	    - If the user supplied a modeid.
	*/
	OOP_Object  	*friend;
	BOOL 	    	 usex11 = FALSE;
    	HIDDT_StdPixFmt  stdpf;

	friend = (OOP_Object *)GetTagData(aHidd_BitMap_Friend, 0, msg->attrList);
	stdpf = (HIDDT_StdPixFmt)GetTagData(aHidd_BitMap_StdPixFmt, vHidd_StdPixFmt_Unknown, msg->attrList);

	if (NULL != friend)
	{
	    if (vHidd_StdPixFmt_Unknown == stdpf)
	    {
	    	Drawable d;
		
	    	/* Is the friend ann X11 bitmap ? */
	    	d = (Drawable)OOP_GetAttr(friend, aHidd_X11BitMap_Drawable, (IPTR *)&d);
	    	if (0 != d)
		{
	    	    usex11 = TRUE;
		}
	    }
	}
	
	if (!usex11)
	{
	    if (vHidd_StdPixFmt_Plane == stdpf)
	    {
	    	usex11 = TRUE;
	    }
	    else
	    {
	    	HIDDT_ModeID modeid;
		
	    	modeid = (HIDDT_ModeID)GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);
		
		if (vHidd_ModeID_Invalid != modeid)
		{
		    usex11 = TRUE;
		}
	    }
	}
	
	if (usex11)
	{
	    tags[5].ti_Tag  = aHidd_BitMap_ClassPtr;
	    tags[5].ti_Data = (IPTR)XSD(cl)->offbmclass;
	    
	}
	else
	{
	    /* Let the superclass allocate if it is a standard pixelformat thus do nothing */
	    
	    kprintf("x11 hidd: Could not create offscreen bitmap for supplied attrs! Superclass hopefully can.\n");
	    
//	    *((ULONG *)0) = 0;
	}
    }
    
    /* !!! IMPORTANT !!! */
    
    p.mID = msg->mID;
    p.attrList = tags;
    
    newbm = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&p);
    
    if (NULL != newbm && framebuffer)
    {
    	OOP_GetAttr(newbm, aHidd_X11BitMap_Drawable, &drawable);
	data->fbwin = (Window)drawable;
    #if ADJUST_XWIN_SIZE
    	OOP_GetAttr(newbm, aHidd_X11BitMap_MasterWindow, &drawable);
	data->masterwin = (Window)drawable;
    #endif
    }
    
    ReturnPtr("X11Gfx::NewBitMap", OOP_Object *, newbm);
}

/****************************************************************************************/

VOID X11Cl__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct gfx_data *data = OOP_INST_DATA(cl, o);
    ULONG   	     idx;
    
    if (IS_X11GFX_ATTR(msg->attrID, idx))
    {
	switch (idx)
	{
	    case aoHidd_X11Gfx_SysDisplay:
	    	*msg->storage = (IPTR)data->display;
		break;
		
	    case aoHidd_X11Gfx_SysScreen:
	    	*msg->storage = (IPTR)data->screen;
		break;
	    
	    default:
	    	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
		break;
	}
    }
    else if (IS_GFX_ATTR(msg->attrID, idx))
    {
    	switch (idx)
	{
	    case aoHidd_Gfx_IsWindowed:
	    	*msg->storage = (IPTR)TRUE;
		break;
		
	    case aoHidd_Gfx_SupportsHWCursor:
    	    #if X11SOFTMOUSE
	    	*msg->storage = (IPTR)FALSE;
    	    #else
	    	*msg->storage = (IPTR)TRUE;
    	    #endif
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

OOP_Object *X11Cl__Hidd_Gfx__Show(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_Show *msg)
{
    OOP_Object      *fb = 0;
    IPTR    	     width, height, modeid;
    OOP_Object      *pf, *sync;
    struct gfx_data *data;
	
    data = OOP_INST_DATA(cl, o);

    if (!msg->bitMap)
    {
    	return (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
    
    OOP_GetAttr(msg->bitMap, aHidd_BitMap_ModeID, &modeid);
    if ( HIDD_Gfx_GetMode(o, (HIDDT_ModeID)modeid, &sync, &pf))
    {
    	struct MsgPort *port;
	
    #if 1
    	OOP_GetAttr(msg->bitMap, aHidd_BitMap_Width, &width);
	OOP_GetAttr(msg->bitMap, aHidd_BitMap_Height, &height);
    #else
    	OOP_GetAttr(sync, aHidd_Sync_HDisp, &width);
	OOP_GetAttr(sync, aHidd_Sync_VDisp, &height);
    #endif

    #if ADJUST_XWIN_SIZE		
	/* Send resize message to the x11 task */
	port = CreateMsgPort();
	if (NULL != port)
	{
	    struct notify_msg *nmsg;
	    
	    nmsg = AllocMem(sizeof (*nmsg), MEMF_PUBLIC);
	    if (NULL != nmsg)
	    {
	    	nmsg->notify_type 	= NOTY_RESIZEWINDOW;
		nmsg->xdisplay	  	= data->display;
		nmsg->xwindow	 	= data->fbwin;
		nmsg->masterxwindow	= data->masterwin;
		nmsg->width	  	= width;
		nmsg->height	  	= height;
		nmsg->execmsg.mn_ReplyPort = port;
		
		PutMsg(XSD(cl)->x11task_notify_port, (struct Message *)nmsg);
		
		WaitPort(port);
		FreeMem(nmsg, sizeof (*nmsg));
    #endif		
		
		fb = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    #if ADJUST_XWIN_SIZE		
	    }
	    DeleteMsgPort(port);
	}
	
    #endif
    }
    
    return fb;
}

/****************************************************************************************/

VOID X11Cl__Hidd_Gfx__CopyBox(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CopyBox *msg)
{
    ULONG   	     	 mode;
    Drawable 	     	 src = 0, dest = 0;
    struct gfx_data 	*data;
    struct bitmap_data  *bmdata;
    
    data = OOP_INST_DATA(cl, o);
    
    mode = GC_DRMD(msg->gc);
    
    OOP_GetAttr(msg->src,  aHidd_X11BitMap_Drawable, (IPTR *)&src);
    OOP_GetAttr(msg->dest, aHidd_X11BitMap_Drawable, (IPTR *)&dest);
	
    if (0 == dest || 0 == src)
    {
	/* The destination object is no X11 bitmap, onscreen nor offscreen.
	    Let the superclass do the copying in a more general way
	*/
	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	
	return;
    }

    LOCK_X11

    /* This may seem ugly, but we know nobody has subclassed
       the x11 class, since it's private
    */
    bmdata = OOP_INST_DATA(XSD(cl)->onbmclass, msg->src);

    XSetFunction(data->display, bmdata->gc, mode);
    
    XCopyArea(data->display
    	, src			/* src	*/
	, dest			/* dest */
	, bmdata->gc
	, msg->srcX
	, msg->srcY
	, msg->width
	, msg->height
	, msg->destX
	, msg->destY
    );
	
    XFlush(data->display);
    
    UNLOCK_X11
    
}

/****************************************************************************************/

BOOL X11Cl__Hidd_Gfx__SetCursorShape(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    /* Dummy implementation */
    return TRUE;
}

/****************************************************************************************/

BOOL X11Cl__Hidd_Gfx__SetCursorPos(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    /* Dummy implementation */
    return TRUE;
}

/****************************************************************************************/

VOID X11Cl__Hidd_Gfx__SetCursorVisible(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    /* Dummy implementation */
    return;
}

/****************************************************************************************/

static ULONG mask_to_shift(ULONG mask)
{
    ULONG i;
    
    for (i = 32; mask; i --)
    {
	mask >>= 1;
    }
	
    if (mask == 32)
    {
   	i = 0;
    }
	
    return i;
}

/****************************************************************************************/

#undef XSD
#define XSD(cl) xsd

/*
   Inits sysdisplay, sysscreen, colormap, etc.. */
static BOOL initx11stuff(struct x11_staticdata *xsd)
{
/*    XColor fg, bg; */
    BOOL    	     ok = TRUE;
    XVisualInfo      template;
    XVisualInfo     *visinfo;
    int     	     template_mask;
    int     	     numvisuals;
    XImage  	    *testimage;


    EnterFunc(bug("initx11stuff()\n"));

    LOCK_X11	

    /* Get some info on the display */
    template.visualid = XVisualIDFromVisual(DefaultVisual(xsd->display, DefaultScreen(xsd->display)));
    template_mask = VisualIDMask;

    visinfo = XGetVisualInfo(xsd->display, template_mask, &template, &numvisuals);

    if (numvisuals > 1)
    {
    	    kprintf("!!! GOT MORE THAN ONE VISUAL FROM X !!!\n");
//    	    kill(getpid(), SIGSTOP);
    }

    if (NULL == visinfo)
    {
    	    kprintf("!!! COULD NOT GET X VISUAL INFO !!!\n");
    	    kill(getpid(), SIGSTOP);
    	    
    	    ok = FALSE;
    }
    else
    {
	/* Store the visual info structure */

	memcpy(&xsd->vi, visinfo, sizeof (XVisualInfo));
	
	XFree(visinfo);
	
	visinfo = &xsd->vi;
	
	/* We only support TrueColor for now */
	
	switch (visinfo->class)
	{
	    case TrueColor:
		/* Get the pixel masks */
		xsd->red_shift	 = mask_to_shift(xsd->vi.red_mask);
		xsd->green_shift = mask_to_shift(xsd->vi.green_mask);
		xsd->blue_shift	 = mask_to_shift(xsd->vi.blue_mask);
	        break;
		
	    case PseudoColor:
	        /* stegerg */
	    	xsd->vi.red_mask   = ((1 << xsd->vi.bits_per_rgb) - 1) << (xsd->vi.bits_per_rgb * 2);
		xsd->vi.green_mask = ((1 << xsd->vi.bits_per_rgb) - 1) << (xsd->vi.bits_per_rgb * 1);
		xsd->vi.blue_mask  = ((1 << xsd->vi.bits_per_rgb) - 1);
		xsd->red_shift	 = mask_to_shift(xsd->vi.red_mask);
		xsd->green_shift = mask_to_shift(xsd->vi.green_mask);
		xsd->blue_shift	 = mask_to_shift(xsd->vi.blue_mask);
		/* end stegerg */
	    	break;
		
	    default:
	    	kprintf("!!! GFX HIDD only supports truecolor and pseudocolor diplays for now !!!\n");
	    	kill(getpid(), SIGSTOP);
	}

	xsd->depth = 0;

    	/* stegerg: based on xwininfo source */
	
	{
	    XWindowAttributes win_attributes;

    	    if (!XGetWindowAttributes(xsd->display,
	    	    	    	      RootWindow(xsd->display, DefaultScreen(xsd->display)),
				      &win_attributes))
    	    {
	    	kprintf("!!! X11gfx could not get bits per pixel\n");
	    	kill(getpid(), SIGSTOP);
	    }
	    xsd->depth = win_attributes.depth;
	    kprintf("\n");
	    kprintf("DisplayPlanes        = %d\n", DisplayPlanes(xsd->display, DefaultScreen(xsd->display)));
	    kprintf("DefaultDepth         = %d\n", DefaultDepth(xsd->display, DefaultScreen(xsd->display)));
	    
	    kprintf("\n\n BITS PER PIXEL = %d \n\n\n", xsd->depth);
 	}

	/* Create a dummy X image to get bits per pixel */
	testimage = XGetImage(xsd->display, RootWindow(xsd->display,
	    	    	      DefaultScreen(xsd->display)), 0, 0, 1, 1,
			      AllPlanes, ZPixmap);
	
	if (NULL != testimage)
	{
    	    xsd->bytes_per_pixel = (testimage->bits_per_pixel + 7) >> 3;
	    XDestroyImage(testimage);
	}
	else
	{
	    kprintf("!!! X11gfx could not get bits per pixel\n");
	    kill(getpid(), SIGSTOP);
	}
	
	if (PseudoColor == xsd->vi.class)
	{
	    xsd->clut_mask  = (1L << xsd->depth) - 1;
	    xsd->clut_shift = 0;
	}
    }

    /* Create a dummy window for pixmaps */

    xsd->dummy_window_for_creating_pixmaps = XCreateSimpleWindow(xsd->display,
    	    	    	    	    	    	    	    	 DefaultRootWindow(xsd->display),
								 0, 0, 100, 100,
								 0,
								 BlackPixel(xsd->display, DefaultScreen(xsd->display)),
   	    	    	    	    	    	    	    	 BlackPixel(xsd->display, DefaultScreen(xsd->display)));
    if (0 == xsd->dummy_window_for_creating_pixmaps)
    {
	ok = FALSE;
    }

#if USE_XSHM    	    
    if (xsd->local_display)
    {
	/* Do we have Xshm support ? */
	xsd->xshm_info = init_shared_mem(xsd->display);
	
	if (NULL == xsd->xshm_info)
	{
    	    /* ok = FALSE; */
    	    kprintf("INITIALIZATION OF XSHM FAILED !!\n");	    
	}
	else
	{

    	    InitSemaphore(&xsd->shm_sema);
    	    xsd->use_xshm = TRUE;
	}    	
    }
#endif


    UNLOCK_X11
    
    ReturnBool("initx11stuff", ok);

}

/****************************************************************************************/

static VOID cleanupx11stuff(struct x11_staticdata *xsd)
{
    LOCK_X11

    /* Do nothing for now */
    if (0 != xsd->dummy_window_for_creating_pixmaps)
    {
    	XDestroyWindow(xsd->display, xsd->dummy_window_for_creating_pixmaps);
    }
    
#if USE_XSHM
    cleanup_shared_mem(xsd->display, xsd->xshm_info);
#endif 
   
    UNLOCK_X11
}

/****************************************************************************************/

#define xsd (&LIBBASE->xsd)

/****************************************************************************************/

AROS_SET_LIBFUNC(x11gfx_init, LIBBASETYPE, LIBBASE) 
{
    AROS_SET_LIBFUNC_INIT

    return OOP_ObtainAttrBases(attrbases);
    
    AROS_SET_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_SET_LIBFUNC(x11gfx_expunge, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    OOP_ReleaseAttrBases(attrbases);

    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT
}

/****************************************************************************************/

ADD2INITLIB(x11gfx_init, 0);
ADD2EXPUNGELIB(x11gfx_expunge, 0);

/****************************************************************************************/
