/*
    Copyright (C) 2016-2026, The AROS Development Team. All rights reserved.

    Desc: X11 gfx display class for AROS.
*/

#include "x11_debug.h"

#include <proto/utility.h>
#include <graphics/monitor.h>

#include <X11/cursorfont.h>
#include <signal.h>
#include <string.h>

#include "x11_types.h"
#include LC_LIBDEFS_FILE
#include "x11_hostlib.h"
#include "x11_xshm.h"

#define XVIDMODETAGS            11

#define XFLUSH(x) XCALL(XFlush, x)

/****************************************************************************************/

static inline ULONG fakeCLOCK(ULONG width, ULONG height)
{
    ULONG retval;
    retval = (1000000000 / STANDARD_COLORCLOCKS);
    return retval;
}
static inline ULONG fakeHTOTAL(ULONG width, ULONG height)
{
    ULONG retval;
    retval = (fakeCLOCK(width, height) / (100000000 / STANDARD_COLORCLOCKS / 22));
    return retval;
}

static inline BOOL matchModes(struct TagItem *resolution, XF86VidModeModeInfo *xfmode)
{
    if ((resolution[3].ti_Data == xfmode->hdisplay) && (resolution[4].ti_Data == xfmode->vdisplay))
        return TRUE;
    return FALSE;
}

OOP_Object *X11Display__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
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
        { aHidd_Sync_PixelClock , fakeCLOCK(160,160)    },
        { aHidd_Sync_HTotal     , fakeHTOTAL(160,160)   },
        { aHidd_Sync_HDisp      , 160                   },
        { aHidd_Sync_VDisp      , 160                   },
        { aHidd_Sync_Description, (IPTR)"X11:160x160"   },
        { TAG_DONE              , 0UL                   }
    };
    
    struct TagItem tags_240_320[] =
    {
        { aHidd_Sync_PixelClock , fakeCLOCK(240,320)    },
        { aHidd_Sync_HTotal     , fakeHTOTAL(240,320)   },
        { aHidd_Sync_HDisp      , 240                   },
        { aHidd_Sync_VDisp      , 320                   },
        { aHidd_Sync_Description, (IPTR)"X11:240x320"   },
        { TAG_DONE              , 0UL                   }
    };

    struct TagItem tags_320_240[] =
    {
        { aHidd_Sync_PixelClock , fakeCLOCK(320,240)    },
        { aHidd_Sync_HTotal     , fakeHTOTAL(320,240)   },
        { aHidd_Sync_HDisp      , 320                   },
        { aHidd_Sync_VDisp      , 240                   },
        { aHidd_Sync_Description, (IPTR)"X11:320x240"   },
        { TAG_DONE              , 0UL                   }
    };

    struct TagItem tags_512_384[] =
    {
        { aHidd_Sync_PixelClock , fakeCLOCK(512,384)    },
        { aHidd_Sync_HTotal     , fakeHTOTAL(512,384)   },
        { aHidd_Sync_HDisp      , 512                   },
        { aHidd_Sync_VDisp      , 384                   },
        { aHidd_Sync_Description, (IPTR)"X11:512x384"   },
        { TAG_DONE              , 0UL                   }
    };

    struct TagItem tags_640_480[] =
    {
        { aHidd_Sync_PixelClock , fakeCLOCK(640,480)    },
        { aHidd_Sync_HTotal     , fakeHTOTAL(640,480)   },
        { aHidd_Sync_HDisp      , 640                   },
        { aHidd_Sync_VDisp      , 480                   },
        { aHidd_Sync_Description, (IPTR)"X11:640x480"   },
        { TAG_DONE              , 0UL                   }
    };

    struct TagItem tags_800_600[] =
    {
        { aHidd_Sync_PixelClock , fakeCLOCK(800,600)    },
        { aHidd_Sync_HTotal     , fakeHTOTAL(800,600)   },
        { aHidd_Sync_HDisp      , 800                   },
        { aHidd_Sync_VDisp      , 600                   },
        { aHidd_Sync_Description, (IPTR)"X11:800x600"   },
        { TAG_DONE              , 0UL                   }
    };

    struct TagItem tags_1024_768[] =
    {
        { aHidd_Sync_PixelClock , fakeCLOCK(1024,768)   },
        { aHidd_Sync_HTotal     , fakeHTOTAL(1024,768)  },
        { aHidd_Sync_HDisp      , 1024                  },
        { aHidd_Sync_VDisp      , 768                   },
        { aHidd_Sync_Description, (IPTR)"X11:1024x768"  },
        { TAG_DONE              , 0UL                   }
    };
    
    struct TagItem tags_1152_864[] =
    {
        { aHidd_Sync_PixelClock , fakeCLOCK(1152,864)   },
        { aHidd_Sync_HTotal     , fakeHTOTAL(1152,864)  },
        { aHidd_Sync_HDisp      , 1152                  },
        { aHidd_Sync_VDisp      , 864                   },
        { aHidd_Sync_Description, (IPTR)"X11:1152x864"  },
        { TAG_DONE              , 0UL                   }
    };
    
    struct TagItem tags_1280_800[] =
    {
        { aHidd_Sync_PixelClock , fakeCLOCK(1280,800)   },
        { aHidd_Sync_HTotal     , fakeHTOTAL(1280,800)  },
        { aHidd_Sync_HDisp      , 1280                  },
        { aHidd_Sync_VDisp      , 800                   },
        { aHidd_Sync_Description, (IPTR)"X11:1280x800"  },
        { TAG_DONE              , 0UL                   }
    };

    struct TagItem tags_1280_960[] =
    {
        { aHidd_Sync_PixelClock , fakeCLOCK(1280,960)   },
        { aHidd_Sync_HTotal     , fakeHTOTAL(1280,960)  },
        { aHidd_Sync_HDisp      , 1280                  },
        { aHidd_Sync_VDisp      , 960                   },
        { aHidd_Sync_Description, (IPTR)"X11:1280x960"  },
        { TAG_DONE              , 0UL                   }
    };
    
    struct TagItem tags_1280_1024[] =
    {
        { aHidd_Sync_PixelClock , fakeCLOCK(1280,1024)  },
        { aHidd_Sync_HTotal     , fakeHTOTAL(1280,1024) },
        { aHidd_Sync_HDisp      , 1280                  },
        { aHidd_Sync_VDisp      , 1024                  },
        { aHidd_Sync_Description, (IPTR)"X11:1280x1024" },
        { TAG_DONE              , 0UL                   }
    };

    struct TagItem tags_1400_1050[] =
    {
        { aHidd_Sync_PixelClock , fakeCLOCK(1400,1050)  },
        { aHidd_Sync_HTotal     , fakeHTOTAL(1400,1050) },
        { aHidd_Sync_HDisp      , 1400                  },
        { aHidd_Sync_VDisp      , 1050                  },
        { aHidd_Sync_Description, (IPTR)"X11:1400x1050" },
        { TAG_DONE              , 0UL                   }
    };
    
    struct TagItem tags_1600_1200[] =
    {
        { aHidd_Sync_PixelClock , fakeCLOCK(1600,1200)  },
        { aHidd_Sync_HTotal     , fakeHTOTAL(1600,1200) },
        { aHidd_Sync_HDisp      , 1600                  },
        { aHidd_Sync_VDisp      , 1200                  },
        { aHidd_Sync_Description, (IPTR)"X11:1600x1200" },
        { TAG_DONE              , 0UL                   }
    };
    
    struct TagItem tags_1680_1050[] =
    {
        { aHidd_Sync_PixelClock , fakeCLOCK(1680,1050)  },
        { aHidd_Sync_HTotal     , fakeHTOTAL(1680,1050) },
        { aHidd_Sync_HDisp      , 1680                  },
        { aHidd_Sync_VDisp      , 1050                  },
        { aHidd_Sync_Description, (IPTR)"X11:1680x1050" },
        { TAG_DONE              , 0UL                   }
    };

    struct TagItem tags_1920_1080[] =
    {
        { aHidd_Sync_PixelClock , fakeCLOCK(1920,1080)  },
        { aHidd_Sync_HTotal     , fakeHTOTAL(1920,1080) },
        { aHidd_Sync_HDisp      , 1920                  },
        { aHidd_Sync_VDisp      , 1080                  },
        { aHidd_Sync_Description, (IPTR)"X11:1920x1080" },
        { TAG_DONE              , 0UL                   }
    };

    struct TagItem tags_1920_1200[] =
    {
        { aHidd_Sync_PixelClock , fakeCLOCK(1920,1200)  },
        { aHidd_Sync_HTotal     , fakeHTOTAL(1920,1200) },
        { aHidd_Sync_HDisp      , 1920                  },
        { aHidd_Sync_VDisp      , 1200                  },
        { aHidd_Sync_Description, (IPTR)"X11:1920x1200" },
        { TAG_DONE              , 0UL                   }
    };

    /* Default display modes. Used on displays which do not support Free86-VidModeExtension */
    struct TagItem default_mode_tags[] =
    {
        { aHidd_DMEnum_PixFmtTags  , (IPTR)pftags          },
        { aHidd_DMEnum_SyncTags    , (IPTR)tags_160_160    },
        { aHidd_DMEnum_SyncTags    , (IPTR)tags_240_320    },
        { aHidd_DMEnum_SyncTags    , (IPTR)tags_320_240    },
        { aHidd_DMEnum_SyncTags    , (IPTR)tags_512_384    },
        { aHidd_DMEnum_SyncTags    , (IPTR)tags_640_480    },
        { aHidd_DMEnum_SyncTags    , (IPTR)tags_800_600    },
        { aHidd_DMEnum_SyncTags    , (IPTR)tags_1024_768   },
        { aHidd_DMEnum_SyncTags    , (IPTR)tags_1152_864   },
        { aHidd_DMEnum_SyncTags    , (IPTR)tags_1280_800   },
        { aHidd_DMEnum_SyncTags    , (IPTR)tags_1280_960   },
        { aHidd_DMEnum_SyncTags    , (IPTR)tags_1280_1024  },
        { aHidd_DMEnum_SyncTags    , (IPTR)tags_1400_1050  },
        { aHidd_DMEnum_SyncTags    , (IPTR)tags_1600_1200  },
        { aHidd_DMEnum_SyncTags    , (IPTR)tags_1680_1050  },
        { aHidd_DMEnum_SyncTags    , (IPTR)tags_1920_1080  },
        { aHidd_DMEnum_SyncTags    , (IPTR)tags_1920_1200  },
        { TAG_DONE              , 0UL                   }
    };

    struct TagItem *mode_tags = NULL;

    struct TagItem mytags[] =
    {
        { aHidd_Display_ModeTags    , (IPTR)default_mode_tags       },
        { TAG_MORE              , (IPTR)msg->attrList           }
    };
 
    struct pRoot_New mymsg = { msg->mID, mytags };

    struct TagItem *resolution = NULL;
    XF86VidModeModeInfo** modes = NULL;
    static int modeNum = 0;
    ULONG realmode = 0;

    ULONG i, screen;
    Display *disp;
    
    D(bug("[X11:Gfx] %s()\n", __func__));

    /* Do GfxHidd initalization here */
    if (!initx11stuff(XSD(cl)))
    {
        D(bug("[X11:Gfx] %s: initialisation failed!\n", __func__));
        return NULL;
    }
    
    /* Get supported X11 resolution from RandR resources */
    
    disp = XCALL(XOpenDisplay, NULL);
    screen = XCALL(XDefaultScreen, disp);
//  rootwin = XCALL(XRootWindow, disp, screen);

    if (!(XSD(cl)->options & OPTION_FORCESTDMODES))
    {
        XVMCALL(XF86VidModeGetAllModeLines, disp, screen, &modeNum, &modes);
        D(bug("[X11:Gfx] Found %u modes, table at 0x%P\n", modeNum, modes));
    
        if (modeNum)
        {
            /* Got XF86VidMode data, use it */
            if ((resolution = AllocMem(modeNum * sizeof(struct TagItem) * XVIDMODETAGS, MEMF_PUBLIC)) == NULL)
            {
                D(bug("[X11:Gfx] failed to allocate memory for %d modes: %d !!!\n", modeNum, XSD(cl)->vi->class));

                XCALL(XCloseDisplay, disp);
                cleanupx11stuff(XSD(cl));

                return NULL;
            }

            for(i = 0; i < modeNum; i++)
            {
                ULONG j;
                BOOL insert;
                insert = TRUE;

                /* avoid duplicated resolution */
                for(j = 0; j < realmode; j++)
                {
                    if(matchModes(&resolution[j * XVIDMODETAGS], modes[i]))
                    { /* Found a matching resolution. Don't insert ! */
                        insert = FALSE;
                    }
                }

                if(insert)
                {
                    resolution[realmode * XVIDMODETAGS + 0].ti_Tag = aHidd_Sync_PixelClock;
                    resolution[realmode * XVIDMODETAGS + 0].ti_Data = (modes[i]->dotclock * 1000);

                    resolution[realmode * XVIDMODETAGS + 1].ti_Tag = aHidd_Sync_HTotal;
                    resolution[realmode * XVIDMODETAGS + 1].ti_Data = modes[i]->htotal;

                    resolution[realmode * XVIDMODETAGS + 2].ti_Tag = aHidd_Sync_VTotal;
                    resolution[realmode * XVIDMODETAGS + 2].ti_Data = modes[i]->vtotal;

                    resolution[realmode * XVIDMODETAGS + 3].ti_Tag = aHidd_Sync_HDisp;
                    resolution[realmode * XVIDMODETAGS + 3].ti_Data = modes[i]->hdisplay;

                    resolution[realmode * XVIDMODETAGS + 4].ti_Tag = aHidd_Sync_VDisp;
                    resolution[realmode * XVIDMODETAGS + 4].ti_Data = modes[i]->vdisplay;

                    resolution[realmode * XVIDMODETAGS + 5].ti_Tag = aHidd_Sync_HSyncStart;
                    resolution[realmode * XVIDMODETAGS + 5].ti_Data = modes[i]->hsyncstart;

                    resolution[realmode * XVIDMODETAGS + 6].ti_Tag = aHidd_Sync_HSyncEnd;
                    resolution[realmode * XVIDMODETAGS + 6].ti_Data = modes[i]->hsyncend;

                    resolution[realmode * XVIDMODETAGS + 7].ti_Tag = aHidd_Sync_VSyncStart;
                    resolution[realmode * XVIDMODETAGS + 7].ti_Data = modes[i]->vsyncstart;

                    resolution[realmode * XVIDMODETAGS + 8].ti_Tag = aHidd_Sync_VSyncEnd;
                    resolution[realmode * XVIDMODETAGS + 8].ti_Data = modes[i]->vsyncend;

                    resolution[realmode * XVIDMODETAGS + 9].ti_Tag = aHidd_Sync_Description;
                    resolution[realmode * XVIDMODETAGS + 9].ti_Data = (IPTR)"X11: %hx%v";

                    resolution[realmode * XVIDMODETAGS + 10].ti_Tag = TAG_DONE;
                    resolution[realmode * XVIDMODETAGS + 10].ti_Data = 0UL;

                    realmode++;
                }
            }

            if((mode_tags = AllocMem(sizeof(struct TagItem) * (realmode + 2), MEMF_PUBLIC)) == NULL)
            {
                D(bug("[X11:Gfx] failed to allocate memory for mode tag's: %d !!!\n", XSD(cl)->vi->class));

                FreeMem(resolution, modeNum * sizeof(struct TagItem) * XVIDMODETAGS);
                XCALL(XCloseDisplay, disp);
                cleanupx11stuff(XSD(cl));

                return NULL;
            }

            mode_tags[0].ti_Tag = aHidd_DMEnum_PixFmtTags;
            mode_tags[0].ti_Data = (IPTR)pftags;

            /* The different screenmode from XF86VMODE */
            for(i=0; i < realmode; i++)
            {
                mode_tags[1 + i].ti_Tag = aHidd_DMEnum_SyncTags;
                mode_tags[1 + i].ti_Data = (IPTR)(resolution + i * XVIDMODETAGS);
            }

            mode_tags[1 + i].ti_Tag = TAG_DONE;
            mode_tags[1 + i].ti_Data = 0UL;
        
            /* Use our new mode tags instead of default ones */
            mytags[0].ti_Data = (IPTR)mode_tags;
        }
    }

    /* Register gfxmodes */
    pftags[0].ti_Data = XSD(cl)->red_shift;
    pftags[1].ti_Data = XSD(cl)->green_shift;
    pftags[2].ti_Data = XSD(cl)->blue_shift;
    pftags[3].ti_Data = 0;
        
    pftags[4].ti_Data = XSD(cl)->vi->red_mask;
    pftags[5].ti_Data = XSD(cl)->vi->green_mask;
    pftags[6].ti_Data = XSD(cl)->vi->blue_mask;
    pftags[7].ti_Data = 0x00000000;

    /* Support 32-bit modes (ie. odroid XU4 with 3D driver) */
    if (XSD(cl)->depth > 24)
    {
        pftags[7].ti_Data = 0xFFFFFFFF ^ (XSD(cl)->vi->red_mask
                | XSD(cl)->vi->green_mask | XSD(cl)->vi->blue_mask);
        pftags[3].ti_Data = mask_to_shift(pftags[7].ti_Data);
    }

    if (XSD(cl)->vi->class == TrueColor)
    {
        pftags[8].ti_Data = vHidd_ColorModel_TrueColor;
    }
    else if (XSD(cl)->vi->class == PseudoColor)
    {
        pftags[8].ti_Data = vHidd_ColorModel_Palette;
        pftags[13].ti_Data = XSD(cl)->clut_shift;
        pftags[14].ti_Data = XSD(cl)->clut_mask;
    }
    else
    {
        D(bug("[X11:Gfx] unsupported color model: %d\n", XSD(cl)->vi->class));
        if (resolution)
        {
            FreeMem(resolution, modeNum * sizeof(struct TagItem) * XVIDMODETAGS);
            FreeMem(mode_tags, sizeof(struct TagItem) * (realmode + 2));
        }
        XCALL(XCloseDisplay, disp);
        cleanupx11stuff(XSD(cl));

        return NULL;
    }
        
    pftags[9].ti_Data   = XSD(cl)->depth;
    pftags[10].ti_Data  = XSD(cl)->bytes_per_pixel;
    pftags[11].ti_Data  = XSD(cl)->depth;
    pftags[12].ti_Data  = vHidd_StdPixFmt_Native;
    
    /* FIXME: Do better than this */

    /* We assume chunky */
    pftags[15].ti_Data = vHidd_BitMapType_Chunky;

    D(bug("[X11:Gfx] Calling super method\n"));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&mymsg);

    D(bug("[X11:Gfx] Super method returned\n"));

    FreeMem(resolution, modeNum * sizeof(struct TagItem) * XVIDMODETAGS);
    FreeMem(mode_tags, sizeof(struct TagItem) * (realmode + 2));
    XCALL(XCloseDisplay, disp);

    if (NULL != o)
    {
        XColor           bg, fg;
        struct X11DisplayData *data = OOP_INST_DATA(cl, o);

        data->basebm = OOP_FindClass(CLID_Hidd_BitMap);
        D(bug("[X11:Gfx] BitMap class @ 0x%p\n", data->basebm);)

        LOCK_X11
        data->display   = XSD(cl)->display;
        data->screen    = DefaultScreen( data->display );
        data->depth     = DisplayPlanes( data->display, data->screen );
        data->colmap    = DefaultColormap( data->display, data->screen );
        D(bug("[X11:Gfx] calling x11_func.XCreateFontCursor(%x), display(%x)\n", x11_func.XCreateFontCursor, data->display));
        /* Create cursor */
        data->cursor = XCALL(XCreateFontCursor,  data->display, XC_top_left_arrow);

        fg.pixel = BlackPixel(data->display, data->screen);
        fg.red = 0x0000; fg.green = 0x0000; fg.blue = 0x0000;
        fg.flags = (DoRed | DoGreen | DoBlue);
        bg.pixel = WhitePixel(data->display, data->screen);
        bg.red = 0xFFFF; bg.green = 0xFFFF; bg.blue = 0xFFFF;
        bg.flags = (DoRed | DoGreen | DoBlue);

        XCALL(XRecolorCursor, data->display, data->cursor, &fg, &bg);

        if (XSD(cl)->options & OPTION_BACKINGSTORE)
        {
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
        }
    
        UNLOCK_X11
    
        D(bug("[X11:Gfx] Got object from super\n"));

        data->display = XSD(cl)->display;
    }

    ReturnPtr("X11Display::New", OOP_Object *, o);
}


/****************************************************************************************/

VOID X11Display__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx;

    D(bug("[X11Gfx:Display] %s()\n", __func__));

    Hidd_Display_Switch (msg->attrID, idx)
    {
    case aoHidd_Display_SpriteTypes:
#if X11SOFTMOUSE
        *msg->storage = 0;
#else
        *msg->storage = vHidd_SpriteType_DirectColor;
#endif
        return;
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/****************************************************************************************/

OOP_Object *X11Display__Hidd_Display__CreateObject(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_CreateObject *msg)
{
    struct X11DisplayData *data = OOP_INST_DATA(cl, o);
    OOP_Object      *object = NULL;

    D(bug("[X11:Gfx] %s()\n", __func__));

    if (msg->cl == data->basebm)
    {
        struct pHidd_Display_CreateObject  p;
        HIDDT_ModeID                modeid;
        struct X11DisplayData             *data;
        struct TagItem              tags[] =
        {
            { aHidd_BitMap_X11_SysDisplay   , 0 }, /* 0 */
            { aHidd_BitMap_X11_SysScreen    , 0 }, /* 1 */
            { aHidd_BitMap_X11_SysCursor    , 0 }, /* 2 */
            { aHidd_BitMap_X11_ColorMap     , 0 }, /* 3 */
            { aHidd_BitMap_X11_VisualClass  , 0 }, /* 4 */
            { TAG_IGNORE                   , 0 }, /* 5 */
            { TAG_MORE                     , 0 }  /* 6 */
        };

        data = OOP_INST_DATA(cl, o);

        tags[0].ti_Data = (IPTR)data->display;
        tags[1].ti_Data = data->screen;
        tags[2].ti_Data = (IPTR)data->cursor;
        tags[3].ti_Data = data->colmap;
        tags[4].ti_Data = XSD(cl)->vi->class;
        tags[6].ti_Data = (IPTR)msg->attrList;

        /* Displayable bitmap ? */
        modeid = GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);

        if (modeid != vHidd_ModeID_Invalid)
        {
            /* ModeID supplied, it's for sure X11 bitmap */
            tags[5].ti_Tag    = aHidd_BitMap_ClassPtr;
            tags[5].ti_Data    = (IPTR)XSD(cl)->bmclass;
        }

        p.mID = msg->mID;
        p.cl = msg->cl;
        p.attrList = tags;

        object = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&p);
    }
    else
        object = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    ReturnPtr("X11Display::CreateObject", OOP_Object *, object);
}


/****************************************************************************************/

BOOL X11Display__Hidd_Display__SetCursorShape(OOP_Class *cl, OOP_Object *o,
    struct pHidd_Display_SetCursorShape *msg)
{
    D(bug("[X11:Gfx] %s()\n", __func__));

#if X11SOFTMOUSE
    /* Dummy implementation */
    return TRUE;
#else
    BOOL success = TRUE;
    IPTR width, height;
    XcursorImage *image;
    struct MsgPort *port;
    struct notify_msg nmsg;
    struct X11DisplayData *data = OOP_INST_DATA(cl, o);

    /* Create an X11 cursor from the passed bitmap */
    OOP_GetAttr(msg->shape, aHidd_BitMap_Width, &width);
    OOP_GetAttr(msg->shape, aHidd_BitMap_Height, &height);

    LOCK_X11

    image = XCCALL(XcursorImageCreate, width, height);

    HIDD_BM_GetImage(msg->shape, (UBYTE *)image->pixels, width * 4, 0, 0,
        width, height, vHidd_StdPixFmt_BGRA32);

    image->xhot = -msg->xoffset;
    image->yhot = -msg->yoffset;

    data->cursor = XCCALL(XcursorImageLoadCursor, data->display, image);
    XCCALL(XcursorImageDestroy, image);

    UNLOCK_X11

    /* Tell the X11 task to change the pointer on all X windows */
    port = CreateMsgPort();
    if (port == NULL)
        success = FALSE;

    if (success)
    {
        nmsg.notify_type = NOTY_NEWCURSOR;
        nmsg.xdisplay = data->display;
        nmsg.xwindow = (Window)data->cursor;
        nmsg.execmsg.mn_ReplyPort = port;

        X11DoNotify(XSD(cl), &nmsg);
        DeleteMsgPort(port);
    }

    return success;
#endif
}


/****************************************************************************************/

VOID X11Display__Hidd_Display__SetCursorVisible(OOP_Class *cl, OOP_Object *o,
    struct pHidd_Display_SetCursorVisible *msg)
{
    D(bug("[X11:Gfx] %s()\n", __func__));

#if X11SOFTMOUSE
    /* Dummy implementation */
#else
    BOOL success = TRUE;
    struct MsgPort *port;
    struct notify_msg nmsg;
    struct X11DisplayData *data = OOP_INST_DATA(cl, o);

    /* Tell the X11 task to change the pointer on all X windows */
    port = CreateMsgPort();
    if (port == NULL)
        success = FALSE;

    if (success)
    {
        nmsg.notify_type = NOTY_NEWCURSOR;
        nmsg.xdisplay = data->display;
        nmsg.xwindow = (Window) (msg->visible ? data->cursor : None);
        nmsg.execmsg.mn_ReplyPort = port;

        X11DoNotify(XSD(cl), &nmsg);
        DeleteMsgPort(port);
    }
#endif

    return;
}

