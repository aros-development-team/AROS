/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: X11 gfx HIDD for AROS.
    Lang: English.
*/

#define __OOP_NOATTRBASES__

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

#include "bitmap_class.h"
#include "x11gfx_intern.h"
#include "x11.h"

#include LC_LIBDEFS_FILE

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#define XFLUSH(x) XCALL(XFlush, x)
//#define XFLUSH(x)

/****************************************************************************************/

#define IS_X11GFX_ATTR(attr, idx) ( ( (idx) = (attr) - HiddX11GfxAB) < num_Hidd_X11BitMap_Attrs)

int xshm_major;

/* Some attrbases needed as global vars.
 These are write-once read-many */

OOP_AttrBase HiddBitMapAttrBase;
OOP_AttrBase HiddX11BitMapAB;
OOP_AttrBase HiddSyncAttrBase;
OOP_AttrBase HiddPixFmtAttrBase;
OOP_AttrBase HiddGfxAttrBase;
OOP_AttrBase HiddAttrBase;

static const struct OOP_ABDescr attrbases[] =
{
    { IID_Hidd_BitMap       , &HiddBitMapAttrBase   },
    { IID_Hidd_X11BitMap    , &HiddX11BitMapAB      },
    { IID_Hidd_Sync         , &HiddSyncAttrBase     },
    { IID_Hidd_PixFmt       , &HiddPixFmtAttrBase   },
    { IID_Hidd_Gfx          , &HiddGfxAttrBase      },
    { IID_Hidd              , &HiddAttrBase         },
    { NULL                  , NULL                  }
};

static VOID cleanupx11stuff(struct x11_staticdata *xsd);
static BOOL initx11stuff(struct x11_staticdata *xsd);

/****************************************************************************************/

OOP_Object *X11Cl__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct TagItem pftags[] =
    {
        { aHidd_PixFmt_RedShift     , 0        }, /* 0 */
        { aHidd_PixFmt_GreenShift   , 0        }, /* 1 */
        { aHidd_PixFmt_BlueShift    , 0        }, /* 2 */
        { aHidd_PixFmt_AlphaShift   , 0        }, /* 3 */
        { aHidd_PixFmt_RedMask      , 0        }, /* 4 */
        { aHidd_PixFmt_GreenMask    , 0        }, /* 5 */
        { aHidd_PixFmt_BlueMask     , 0        }, /* 6 */
        { aHidd_PixFmt_AlphaMask    , 0        }, /* 7 */
        { aHidd_PixFmt_ColorModel   , 0        }, /* 8 */
        { aHidd_PixFmt_Depth        , 0        }, /* 9 */
        { aHidd_PixFmt_BytesPerPixel, 0        }, /* 10 */
        { aHidd_PixFmt_BitsPerPixel , 0        }, /* 11 */
        { aHidd_PixFmt_StdPixFmt    , 0        }, /* 12 */
        { aHidd_PixFmt_CLUTShift    , 0        }, /* 13 */
        { aHidd_PixFmt_CLUTMask     , 0        }, /* 14 */ 
        { aHidd_PixFmt_BitMapType   , 0        }, /* 15 */   
        { TAG_DONE                  , 0UL   }
    };

    struct TagItem tags_160_160[] =
    {
        { aHidd_Sync_HDisp      , 160                     },
        { aHidd_Sync_VDisp      , 160                     },
        { aHidd_Sync_Description, (IPTR)"X11:160x160"     },
        { TAG_DONE              , 0UL                     }
    };
    
    struct TagItem tags_240_320[] =
    {
        { aHidd_Sync_HDisp      , 240                     },
        { aHidd_Sync_VDisp      , 320                     },
        { aHidd_Sync_Description, (IPTR)"X11:240x320"     },
        { TAG_DONE              , 0UL                     }
    };

    struct TagItem tags_320_240[] = 
    {
        { aHidd_Sync_HDisp      , 320                     },
        { aHidd_Sync_VDisp      , 240                     },
        { aHidd_Sync_Description, (IPTR)"X11:320x240"     },
        { TAG_DONE              , 0UL                     }
    };

    struct TagItem tags_512_384[] = 
    {
        { aHidd_Sync_HDisp      , 512                     },
        { aHidd_Sync_VDisp      , 384                     },
        { aHidd_Sync_Description, (IPTR)"X11:512x384"     },
        { TAG_DONE              , 0UL                     }
    };

    struct TagItem tags_640_480[] = 
    {
        { aHidd_Sync_HDisp      , 640                     },
        { aHidd_Sync_VDisp      , 480                     },
        { aHidd_Sync_Description, (IPTR)"X11:640x480"     },
        { TAG_DONE              , 0UL                     }
    };

    struct TagItem tags_800_600[] = 
    {
        { aHidd_Sync_HDisp      , 800                     },
        { aHidd_Sync_VDisp      , 600                     },
        { aHidd_Sync_Description, (IPTR)"X11:800x600"     },
        { TAG_DONE              , 0UL                     }
    };

    struct TagItem tags_1024_768[] = 
    {
        { aHidd_Sync_HDisp      , 1024                  },
        { aHidd_Sync_VDisp      , 768                   },
        { aHidd_Sync_Description, (IPTR)"X11:1024x768"  },
        { TAG_DONE              , 0UL                   }
    };
    
    struct TagItem tags_1152_864[] = 
    {
        { aHidd_Sync_HDisp      , 1152                  },
        { aHidd_Sync_VDisp      , 864                   },
        { aHidd_Sync_Description, (IPTR)"X11:1152x864"  },
        { TAG_DONE              , 0UL                   }
    };
    
    struct TagItem tags_1280_800[] =
    {
        { aHidd_Sync_HDisp      , 1280                  },
        { aHidd_Sync_VDisp      , 800                   },
        { aHidd_Sync_Description, (IPTR)"X11:1280x800"  },
        { TAG_DONE              , 0UL                   }
    };

    struct TagItem tags_1280_960[] = 
    {
        { aHidd_Sync_HDisp      , 1280                  },
        { aHidd_Sync_VDisp      , 960                   },
        { aHidd_Sync_Description, (IPTR)"X11:1280x960"  },
        { TAG_DONE              , 0UL                   }
    };
    
    struct TagItem tags_1280_1024[] =
    {
        { aHidd_Sync_HDisp      , 1280                  },
        { aHidd_Sync_VDisp      , 1024                  },
        { aHidd_Sync_Description, (IPTR)"X11:1280x1024" },
        { TAG_DONE              , 0UL                   }
    };

    struct TagItem tags_1400_1050[] = 
    {
        { aHidd_Sync_HDisp      , 1400                  },
        { aHidd_Sync_VDisp      , 1050                   },
        { aHidd_Sync_Description, (IPTR)"X11:1400x1050"  },
        { TAG_DONE              , 0UL                   }
    };
    
    struct TagItem tags_1600_1200[] = 
    {
        { aHidd_Sync_HDisp      , 1600                  },
        { aHidd_Sync_VDisp      , 1200                  },
        { aHidd_Sync_Description, (IPTR)"X11:1600x1200" },
        { TAG_DONE              , 0UL                   }
    };
    
    struct TagItem tags_1680_1050[] =
    {
        { aHidd_Sync_HDisp      , 1680                  },
        { aHidd_Sync_VDisp      , 1050                  },
        { aHidd_Sync_Description, (IPTR)"X11:1680x1050" },
        { TAG_DONE              , 0UL                   }
    };

    struct TagItem tags_1920_1080[] =
    {
        { aHidd_Sync_HDisp      , 1920                  },
        { aHidd_Sync_VDisp      , 1080                  },
        { aHidd_Sync_Description, (IPTR)"X11:1920x1080" },
        { TAG_DONE              , 0UL                   }
    };

    struct TagItem tags_1920_1200[] =
    {
        { aHidd_Sync_HDisp      , 1920                  },
        { aHidd_Sync_VDisp      , 1200                  },
        { aHidd_Sync_Description, (IPTR)"X11:1920x1200" },
        { TAG_DONE              , 0UL                   }
    };

    /* Default display modes. Used on displays which do not support Free86-VidModeExtension */
    struct TagItem default_mode_tags[] =
    {
        { aHidd_Gfx_PixFmtTags  , (IPTR)pftags          },
        { aHidd_Gfx_SyncTags    , (IPTR)tags_160_160    },
        { aHidd_Gfx_SyncTags    , (IPTR)tags_240_320    },
        { aHidd_Gfx_SyncTags    , (IPTR)tags_320_240    },
        { aHidd_Gfx_SyncTags    , (IPTR)tags_512_384    },
        { aHidd_Gfx_SyncTags    , (IPTR)tags_640_480    },
        { aHidd_Gfx_SyncTags    , (IPTR)tags_800_600    },
        { aHidd_Gfx_SyncTags    , (IPTR)tags_1024_768   },
        { aHidd_Gfx_SyncTags    , (IPTR)tags_1152_864   },
        { aHidd_Gfx_SyncTags    , (IPTR)tags_1280_800   },
        { aHidd_Gfx_SyncTags    , (IPTR)tags_1280_960   },
        { aHidd_Gfx_SyncTags    , (IPTR)tags_1280_1024  },
        { aHidd_Gfx_SyncTags    , (IPTR)tags_1400_1050  },
        { aHidd_Gfx_SyncTags    , (IPTR)tags_1600_1200  },
        { aHidd_Gfx_SyncTags    , (IPTR)tags_1680_1050  },
        { aHidd_Gfx_SyncTags    , (IPTR)tags_1920_1080  },
        { aHidd_Gfx_SyncTags    , (IPTR)tags_1920_1200  },
        { TAG_DONE              , 0UL                   }
    };

    struct TagItem *mode_tags = NULL;

    struct TagItem mytags[] =
    {
        { aHidd_Gfx_ModeTags    , (IPTR)default_mode_tags   },
        { aHidd_Name            , (IPTR)"x11_1.monitor"     },
        { aHidd_HardwareName    , (IPTR)"X Window system"   },
        { aHidd_ProducerName    , (IPTR)"X.Org Foundation"  },
        { TAG_MORE              , (IPTR)msg->attrList       }
    };
 
    struct pRoot_New mymsg = { msg->mID, mytags };

    struct TagItem *resolution = NULL;
    XF86VidModeModeInfo** modes = NULL;
    static int modeNum = 0;
    ULONG realmode = 0;

    ULONG i, screen;
    Display *disp;
    
    EnterFunc(bug("X11Gfx::New()\n"));

    /* Do GfxHidd initalization here */
    if (!initx11stuff(XSD(cl)))
    {
        kprintf("!!! initx11stuff() FAILED IN X11Gfx::New() !!!\n");
        ReturnPtr("X11Gfx::New()", OOP_Object *, NULL);
    }
    
    /* Get supported X11 resolution from RandR resources */
    
    disp = XCALL(XOpenDisplay, NULL);
    screen = XCALL(XDefaultScreen, disp);    
//  rootwin = XCALL(XRootWindow, disp, screen);

    XVMCALL(XF86VidModeGetAllModeLines, disp, screen, &modeNum, &modes);
    D(bug("[X11Gfx] Found %u modes, table at 0x%P\n", modeNum, modes));

    if (modeNum)
    {
        /* Got XF86VidMode data, use it */ 
        if ((resolution = AllocMem(modeNum * sizeof(struct TagItem) * 4, MEMF_PUBLIC)) == NULL)
        {
            XCALL(XCloseDisplay, disp);
            kprintf("!!! Couldn't allocate resolution memory in X11Gfx:New(): %d !!!\n", XSD(cl)->vi.class);
            cleanupx11stuff(XSD(cl));
            ReturnPtr("X11Gfx::New", OOP_Object *, NULL);
        }
    
        for(i = 0; i < modeNum; i++)
        {
            ULONG j;
            BOOL insert;
            insert = TRUE;

            /* avoid duplicated resolution */
            for(j = 0; j < realmode; j++)
            {
                if(resolution[j * 4].ti_Data == modes[i]->hdisplay && resolution[j * 4 + 1].ti_Data == modes[i]->vdisplay)
                { /* Found a matching resolution. Don't insert ! */
                    insert = FALSE;
                }
            }

            if(insert)
            {
                resolution[realmode * 4 + 0].ti_Tag = aHidd_Sync_HDisp;
                resolution[realmode * 4 + 0].ti_Data = modes[i]->hdisplay;

                resolution[realmode * 4 + 1].ti_Tag = aHidd_Sync_VDisp;
                resolution[realmode * 4 + 1].ti_Data = modes[i]->vdisplay;

                resolution[realmode * 4 + 2].ti_Tag = aHidd_Sync_Description;
                resolution[realmode * 4 + 2].ti_Data = (IPTR)"X11: %hx%v";
        
                resolution[realmode * 4 + 3].ti_Tag = TAG_DONE;
                resolution[realmode * 4 + 3].ti_Data = 0UL;

                realmode++;
            }
        }

        if((mode_tags = AllocMem(sizeof(struct TagItem) * (realmode + 2), MEMF_PUBLIC)) == NULL)
        {
            FreeMem(resolution, modeNum * sizeof(struct TagItem) * 4);
            XCALL(XCloseDisplay, disp);
            kprintf("!!! Couldn't allocate mode_tags memory in X11Gfx:New(): %d !!!\n", XSD(cl)->vi.class);
            cleanupx11stuff(XSD(cl));
            ReturnPtr("X11Gfx::New", OOP_Object *, NULL);
        }

        mode_tags[0].ti_Tag = aHidd_Gfx_PixFmtTags;
        mode_tags[0].ti_Data = (IPTR)pftags;
        
        /* The different screenmode from XF86VMODE */
        for(i=0; i < realmode; i++)
        {
            mode_tags[1 + i].ti_Tag = aHidd_Gfx_SyncTags;
            mode_tags[1 + i].ti_Data = (IPTR)(resolution + i*4);
        }

        mode_tags[1 + i].ti_Tag = TAG_DONE;
        mode_tags[1 + i].ti_Data = 0UL;
    
        /* Use our new mode tags instead of default ones */
        mytags[0].ti_Data = (IPTR)mode_tags;
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
        if (resolution)
        {
            FreeMem(resolution, modeNum * sizeof(struct TagItem) * 4);
            FreeMem(mode_tags, sizeof(struct TagItem) * (realmode + 2));
        }
        XCALL(XCloseDisplay, disp);
        kprintf("!!! UNHANDLED COLOR MODEL IN X11Gfx:New(): %d !!!\n", XSD(cl)->vi.class);
        cleanupx11stuff(XSD(cl));
        ReturnPtr("X11Gfx::New", OOP_Object *, NULL);
    }
        
    pftags[9].ti_Data   = XSD(cl)->depth;
    pftags[10].ti_Data  = XSD(cl)->bytes_per_pixel;
    pftags[11].ti_Data  = XSD(cl)->depth;
    pftags[12].ti_Data  = vHidd_StdPixFmt_Native;
    
    /* FIXME: Do better than this */

    /* We assume chunky */
    pftags[15].ti_Data = vHidd_BitMapType_Chunky;

    D(bug("Calling super method\n"));
    
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&mymsg);
    
    D(bug("Super method called\n"));

    FreeMem(resolution, modeNum * sizeof(struct TagItem) * 4);
    FreeMem(mode_tags, sizeof(struct TagItem) * (realmode + 2));
    XCALL(XCloseDisplay, disp);

    if (NULL != o)
    {
        XColor           bg, fg;
        struct gfx_data *data = OOP_INST_DATA(cl, o);
    
        LOCK_X11
        data->display   = XSD(cl)->display;
        data->screen    = DefaultScreen( data->display );
        data->depth     = DisplayPlanes( data->display, data->screen );
        data->colmap    = DefaultColormap( data->display, data->screen );
        D(bug("x11_func.XCreateFontCursor(%x), display(%x)\n", x11_func.XCreateFontCursor, data->display));
        /* Create cursor */
        data->cursor = XCALL(XCreateFontCursor,  data->display, XC_top_left_arrow);

        fg.pixel = BlackPixel(data->display, data->screen);
        fg.red = 0x0000; fg.green = 0x0000; fg.blue = 0x0000;
        fg.flags = (DoRed | DoGreen | DoBlue);
        bg.pixel = WhitePixel(data->display, data->screen);
        bg.red = 0xFFFF; bg.green = 0xFFFF; bg.blue = 0xFFFF;
        bg.flags = (DoRed | DoGreen | DoBlue);

        XCALL(XRecolorCursor, data->display, data->cursor, &fg, &bg);
    
        switch(DoesBackingStore(ScreenOfDisplay(data->display, data->screen)))
        {
            case WhenMapped:
            case Always:
                break;
        
            case NotUseful:
                bug("\n"
            "+----------------------------------------------------------+\n"
            "| Your X Server seems to have backing store disabled!      |\n"
            "| ===================================================      |\n"
            "|                                                          |\n"
            "| If possible you should try to switch it on, otherwise    |\n"
            "| AROS will have problems with its display. When the AROS  |\n"
            "| X window is hidden by other X windows, or is dragged     |\n"
            "| off screen, then the graphics in those parts will get    |\n"
            "| lost, unless backing store support is enabled.           |\n"
            "|                                                          |\n"
            "| In case your X11 Server is XFree 4.x then switching on   |\n"
            "| backingstore support can be done by starting the X11     |\n"
            "| server with something like \"startx -- +bs\". Depending    |\n"
            "| on what X driver you use it might also be possible       |\n"
            "| to turn it on by adding                                  |\n"
            "|                                                          |\n"
            "|         Option \"Backingstore\"                            |\n"
            "|                                                          |\n"
            "| to the Device Section of your X Window config file,      |\n"
            "| which usually is \"/etc/X11/xorg.conf\"                    |\n"
            "| or \"/etc/X11/XFree86Config\"                              |\n"
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
    EnterFunc(bug("X11Gfx::Dispose(o=%p)\n", o));
    
    cleanupx11stuff(XSD(cl));

    D(bug("X11Gfx::Dispose: calling super\n"));    
    OOP_DoSuperMethod(cl, o, msg);
    
    ReturnVoid("X11Gfx::Dispose");
}

/****************************************************************************************/

OOP_Object *X11Cl__Hidd_Gfx__NewBitMap(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_NewBitMap *msg)
{
    BOOL                        framebuffer = FALSE;
    struct pHidd_Gfx_NewBitMap  p;
    OOP_Object                  *newbm;
    HIDDT_ModeID                modeid;
    struct gfx_data             *data;
    struct TagItem              tags[] =
    {
        { aHidd_X11BitMap_SysDisplay   , 0 }, /* 0 */
        { aHidd_X11BitMap_SysScreen    , 0 }, /* 1 */
        { aHidd_X11BitMap_SysCursor    , 0 }, /* 2 */
        { aHidd_X11BitMap_ColorMap     , 0 }, /* 3 */
        { aHidd_X11BitMap_VisualClass  , 0 }, /* 4 */
        { TAG_IGNORE                   , 0 }, /* 5 */
        { TAG_MORE                     , 0 }  /* 6 */
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
#if USE_FRAMEBUFFER
    framebuffer = GetTagData(aHidd_BitMap_FrameBuffer, FALSE, msg->attrList);
#else
    framebuffer = GetTagData(aHidd_BitMap_Displayable, FALSE, msg->attrList);
#endif
    modeid = GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);

    /* Apparently, framebuffer is never used. */
    (void)framebuffer;

    if (modeid != vHidd_ModeID_Invalid)
    {
        /* ModeID supplied, it's for sure X11 bitmap */
        tags[5].ti_Tag    = aHidd_BitMap_ClassPtr;
        tags[5].ti_Data    = (IPTR)XSD(cl)->bmclass;
    }

    /* !!! IMPORTANT !!! */
    p.mID = msg->mID;
    p.attrList = tags;

    newbm = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&p);

    ReturnPtr("X11Gfx::NewBitMap", OOP_Object *, newbm);
}

/****************************************************************************************/

VOID X11Cl__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx;

    if (IS_GFX_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
#if !USE_FRAMEBUFFER
        case aoHidd_Gfx_NoFrameBuffer:
#endif
        case aoHidd_Gfx_IsWindowed:
            *msg->storage = TRUE;
            return;

        case aoHidd_Gfx_HWSpriteTypes:
#if X11SOFTMOUSE
            *msg->storage = 0;
#else
            *msg->storage = vHidd_SpriteType_DirectColor;
#endif
            return;

        case aoHidd_Gfx_DriverName:
            *msg->storage = (IPTR) "X11";
            return;
        }
    }
    OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
}

/****************************************************************************************/

VOID X11Cl__Root__Set(OOP_Class *cl, OOP_Object *obj, struct pRoot_Set *msg)
{
    struct TagItem *tag, *tstate;
    ULONG idx;
    struct x11_staticdata *data = XSD(cl);

    tstate = msg->attrList;
    while ((tag = NextTagItem(&tstate)))
    {
        if (IS_GFX_ATTR(tag->ti_Tag, idx))
        {
            switch (idx)
            {
            case aoHidd_Gfx_ActiveCallBack:
                data->activecallback = (void *) tag->ti_Data;
                break;

            case aoHidd_Gfx_ActiveCallBackData:
                data->callbackdata = (void *) tag->ti_Data;
                break;
            }
        }
    }
    OOP_DoSuperMethod(cl, obj, &msg->mID);
}

/****************************************************************************************/

VOID X11Cl__Hidd_Gfx__CopyBox(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CopyBox *msg)
{
    ULONG mode;
    Drawable src = 0, dest = 0;
    GC gc = 0;

    struct gfx_data *data = OOP_INST_DATA(cl, o);

    mode = GC_DRMD(msg->gc);

    OOP_GetAttr(msg->src, aHidd_X11BitMap_Drawable, (IPTR *) &src);
    OOP_GetAttr(msg->dest, aHidd_X11BitMap_Drawable, (IPTR *) &dest);

    if (0 == dest || 0 == src)
    {
        /*
         * One of objects is not an X11 bitmap.
         * Let the superclass do the copying in a more general way
         */
        OOP_DoSuperMethod(cl, o, &msg->mID);
        return;
    }

    OOP_GetAttr(msg->src, aHidd_X11BitMap_GC, (IPTR *) &gc);

    HostLib_Lock();

    XCALL(XSetFunction, data->display, gc, mode);
    XCALL(XCopyArea,
            data->display, src, dest, gc, msg->srcX, msg->srcY, msg->width, msg->height, msg->destX, msg->destY);

    HostLib_Unlock();
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

    for (i = 32; mask; i--)
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
    BOOL ok = TRUE;
    XVisualInfo template;
    XVisualInfo *visinfo;
    int template_mask;
    int numvisuals;
    XImage *testimage;

    EnterFunc(bug("initx11stuff()\n"));

    if (!X11_Init(xsd))
        return FALSE;

    LOCK_X11

    /* Get some info on the display */
    template.visualid = XCALL(XVisualIDFromVisual, DefaultVisual(xsd->display, DefaultScreen(xsd->display)));
    template_mask = VisualIDMask;

    visinfo = XCALL(XGetVisualInfo, xsd->display, template_mask, &template, &numvisuals);

    if (numvisuals > 1)
    {
        kprintf("!!! GOT MORE THAN ONE VISUAL FROM X !!!\n");
//            CCALL(raise, SIGSTOP);
    }

    if (NULL == visinfo)
    {
        kprintf("!!! COULD NOT GET X VISUAL INFO !!!\n");
        CCALL(raise, SIGSTOP);

        ok = FALSE;
    }
    else
    {
        /* Store the visual info structure */

        memcpy(&xsd->vi, visinfo, sizeof(XVisualInfo));

        XCALL(XFree, visinfo);

        visinfo = &xsd->vi;

        /* We only support TrueColor for now */

        switch (visinfo->class)
        {
        case TrueColor:
            /* Get the pixel masks */
            xsd->red_shift = mask_to_shift(xsd->vi.red_mask);
            xsd->green_shift = mask_to_shift(xsd->vi.green_mask);
            xsd->blue_shift = mask_to_shift(xsd->vi.blue_mask);
            break;

        case PseudoColor:
            /* stegerg */
            xsd->vi.red_mask = ((1 << xsd->vi.bits_per_rgb) - 1) << (xsd->vi.bits_per_rgb * 2);
            xsd->vi.green_mask = ((1 << xsd->vi.bits_per_rgb) - 1) << (xsd->vi.bits_per_rgb * 1);
            xsd->vi.blue_mask = ((1 << xsd->vi.bits_per_rgb) - 1);
            xsd->red_shift = mask_to_shift(xsd->vi.red_mask);
            xsd->green_shift = mask_to_shift(xsd->vi.green_mask);
            xsd->blue_shift = mask_to_shift(xsd->vi.blue_mask);
            /* end stegerg */
            break;

        default:
            kprintf("!!! GFX HIDD only supports truecolor and pseudocolor diplays for now !!!\n");
            CCALL(raise, SIGSTOP);
        }

        xsd->depth = 0;

        /* stegerg: based on xwininfo source */

        {
            XWindowAttributes win_attributes;

            if (!XCALL(XGetWindowAttributes, xsd->display,
                    RootWindow(xsd->display, DefaultScreen(xsd->display)),
                    &win_attributes))
            {
                kprintf("!!! X11gfx could not get bits per pixel\n");
                CCALL(raise, SIGSTOP);
            }
            xsd->depth = win_attributes.depth;
            kprintf("\n");
            kprintf("DisplayPlanes        = %d\n", DisplayPlanes(xsd->display, DefaultScreen(xsd->display)));
            kprintf("DefaultDepth         = %d\n", DefaultDepth(xsd->display, DefaultScreen(xsd->display)));

            kprintf("\n\n BITS PER PIXEL = %d \n\n\n", xsd->depth);
        }

        /* Create a dummy X image to get bits per pixel */
        testimage = XCALL(XGetImage, xsd->display, RootWindow(xsd->display,
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
            CCALL(raise, SIGSTOP);
        }

        if (PseudoColor == xsd->vi.class)
        {
            xsd->clut_mask = (1L << xsd->depth) - 1;
            xsd->clut_shift = 0;
        }
    }

    /* Create a dummy window for pixmaps */

    xsd->dummy_window_for_creating_pixmaps = XCALL(XCreateSimpleWindow, xsd->display,
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
    {
        char *displayname = XCALL(XDisplayName, NULL);

        if ((strncmp(displayname, ":", 1) == 0) ||
                (strncmp(displayname, "unix:", 5) == 0))
        {
            /* Display is local, not remote. XSHM is possible */

            /* Do we have Xshm support ? */
            xsd->xshm_info = init_shared_mem(xsd->display);

            if (NULL == xsd->xshm_info)
            {
                /* ok = FALSE; */
                kprintf("INITIALIZATION OF XSHM FAILED !!\n");
            }
            else
            {
                int a, b;

                InitSemaphore(&xsd->shm_sema);
                xsd->use_xshm = TRUE;

                XCALL(XQueryExtension, xsd->display, "MIT-SHM", &xshm_major, &a, &b);
            }
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
        XCALL(XDestroyWindow, xsd->display, xsd->dummy_window_for_creating_pixmaps);
    }

#if USE_XSHM
    cleanup_shared_mem(xsd->display, xsd->xshm_info);
#endif 

    UNLOCK_X11
}

/****************************************************************************************/

//#define xsd (&LIBBASE->xsd)
/****************************************************************************************/

static int x11gfx_init(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[X11] Initializing semaphore 0x%p\n", &LIBBASE->xsd.sema));

    InitSemaphore(&LIBBASE->xsd.sema);

    return OOP_ObtainAttrBases(attrbases);
}

/****************************************************************************************/

static int x11gfx_expunge(LIBBASETYPEPTR LIBBASE)
{
    OOP_ReleaseAttrBases(attrbases);
    return TRUE;
}

/****************************************************************************************/

ADD2INITLIB(x11gfx_init, 0);
ADD2EXPUNGELIB(x11gfx_expunge, 0);

/****************************************************************************************/
