/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/cybergraphics.h>
#include <clib/macros.h>
#include <dos/dos.h>
#include <dos/datetime.h>
#include <intuition/intuitionbase.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <intuition/preferences.h>
#include <intuition/extensions.h>
#include <graphics/layers.h>
#include <graphics/rpattr.h>
#include <graphics/gfxmacros.h>
#include <cybergraphx/cybergraphics.h>

#ifdef SKINS
#   include "intuition_customize.h"
#   include "intuition_extend.h"
#   include "mosmisc.h"
#endif

#include "intuition_preferences.h"
#include "intuition_intern.h"

#include "boopsigadgets.h"
#include "showhide.h"

#include <string.h>


#undef DEBUG
#define DEBUG 0
#   include <aros/debug.h>

ULONG addextragadget(struct Window *w,BOOL is_gzz,struct DrawInfo *dri,LONG relright,ULONG imagetype,ULONG gadgetid,ULONG gadgettype,struct IntuitionBase *IntuitionBase);
extern ULONG HookEntry();

/**********************************************************************************/

void LoadDefaultPreferences(struct IntuitionBase * IntuitionBase)
{
    BYTE read_preferences = FALSE;
#   ifdef SKINS
        static UWORD DriPens2[NUMDRIPENS] = { 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1, 1 , 1 , 0};
        static UWORD DriPens4[NUMDRIPENS] = { 1, 0, 1, 2, 1, 3, 1, 0, 2, 1, 2, 1 , 2 , 1};
#   else
        static UWORD DriPens2[NUMDRIPENS] = { 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1, 1};
        static UWORD DriPens4[NUMDRIPENS] = { 1, 0, 1, 2, 1, 3, 1, 0, 2, 1, 2, 1};
#   endif /* SKINS */
    
    /*
    ** Load the intuition preferences from a file on the disk
    ** Allocate storage for the preferences, even if it's just a copy
    ** of the default preferences.
    */
    GetPrivIBase(IntuitionBase)->DefaultPreferences =
        AllocMem(sizeof(struct Preferences), MEMF_CLEAR);

    GetPrivIBase(IntuitionBase)->ScreenModePrefs.smp_DisplayID  = INVALID_ID;
    GetPrivIBase(IntuitionBase)->ScreenModePrefs.smp_Width  	= AROS_DEFAULT_WBWIDTH;
    GetPrivIBase(IntuitionBase)->ScreenModePrefs.smp_Height 	= AROS_DEFAULT_WBHEIGHT;
    GetPrivIBase(IntuitionBase)->ScreenModePrefs.smp_Depth  	= AROS_DEFAULT_WBDEPTH;
    GetPrivIBase(IntuitionBase)->ScreenModePrefs.smp_Control 	= 0;

    GetPrivIBase(IntuitionBase)->IControlPrefs.ic_TimeOut  = 50;
    GetPrivIBase(IntuitionBase)->IControlPrefs.ic_MetaDrag = IEQUALIFIER_LCOMMAND;    
    GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags    = ICF_3DMENUS |
    	    	    	    	    	    	    	     ICF_OFFSCREENLAYERS |
							     ICF_AVOIDWINBORDERERASE |
    	    	    					     ICF_MODEPROMOTE | 
    	    	    					     ICF_MENUSNAP |
							     ICF_STRGAD_FILTER |
							     ICF_COERCE_LACE;
    GetPrivIBase(IntuitionBase)->IControlPrefs.ic_WBtoFront   = 'N';
    GetPrivIBase(IntuitionBase)->IControlPrefs.ic_FrontToBack = 'M';
    GetPrivIBase(IntuitionBase)->IControlPrefs.ic_ReqTrue     = 'V';
    GetPrivIBase(IntuitionBase)->IControlPrefs.ic_ReqFalse    = 'B';  


    /*
     * Mouse default.
     */
    GetPrivIBase(IntuitionBase)->DefaultPreferences->PointerTicks = 2;

#warning FIXME: Try to load preferences from a file!


    /*******************************************************************
        DOSBase = OpenLibrary("dos.library",0);
        if (NULL != DOSBase)
        {
          if (NULL != (pref_file = Open("envarc:",MODE_OLDFILE)))
          {
            *
            **  Read it and check whether the file was valid.
            *
     
            if (sizeof(struct Preferences) ==
                Read(pref_file,
                     GetPrivIBase(IntuitionBase)->DefaultPreferences,
                     sizeof(struct Preferences)))
              read_preferences = TRUE;
     
            Close(pref_file);
          }
          CloseLibrary(DOSBase)
        }
    ****************************************************************/

    if (FALSE == read_preferences)
    {
        /*
        ** no (valid) preferences file is available.
        */
        CopyMem(&IntuitionDefaultPreferences,
                GetPrivIBase(IntuitionBase)->DefaultPreferences,
                sizeof(struct Preferences));
    }


    /*
    ** Activate the preferences...
    */

    GetPrivIBase(IntuitionBase)->ActivePreferences =
        AllocMem(sizeof(struct Preferences),
                 MEMF_CLEAR);

#if 1
    CopyMem(GetPrivIBase(IntuitionBase)->DefaultPreferences,
            GetPrivIBase(IntuitionBase)->ActivePreferences,
            sizeof(struct Preferences));
#else
SetPrefs(GetPrivIBase(IntuitionBase)->DefaultPreferences,
    sizeof(struct Preferences),
    FALSE/*TRUE*/);
#endif

    CopyMem(DriPens2, GetPrivIBase(IntuitionBase)->DriPens2, sizeof(DriPens2));
    CopyMem(DriPens4, GetPrivIBase(IntuitionBase)->DriPens4, sizeof(DriPens4));
    CopyMem(DriPens4, GetPrivIBase(IntuitionBase)->DriPens8, sizeof(DriPens4));
}

/**********************************************************************************/

void CheckRectFill(struct RastPort *rp, WORD x1, WORD y1, WORD x2, WORD y2,
                   struct IntuitionBase * IntuitionBase)
{

    if ((x2 >= x1) && (y2 >= y1))
    {
        RectFill(rp, x1, y1, x2, y2);
    }
}

/**********************************************************************************/

#define TITLEBAR_HEIGHT (w->BorderTop)

BOOL CreateWinSysGadgets(struct Window *w, struct IntuitionBase *IntuitionBase)
{

    struct DrawInfo *dri;
    BOOL    	     is_gzz;

    EnterFunc(bug("CreateWinSysGadgets(w=%p)\n", w));

    is_gzz = (w->Flags & WFLG_GIMMEZEROZERO) ? TRUE : FALSE;

    dri = GetScreenDrawInfo(w->WScreen);
    if (dri)
    {
        LONG db_left, db_width, relright,ewidth; /* dragbar sizes */
        BOOL sysgads_ok = TRUE;


        db_left = 0;
        db_width = 0; /* Georg Steger: was w->Width; */

        /* Relright of rightmost button */
        //relright = - (TITLEBAR_HEIGHT - 1);

        relright = 1;

        /* Now try to create the various gadgets */

        if (w->Flags & WFLG_SIZEGADGET)
        {
            /* this code must not change the 'relright' variable */
            WORD width  = ((struct IntWindow *)w)->sizeimage_width;
            WORD height = ((struct IntWindow *)w)->sizeimage_height;

            struct TagItem size_tags[] =
            {
                {GA_Image   	, 0             },
                {GA_RelRight    , -width  + 1   },
                {GA_RelBottom   , -height + 1   },
                {GA_Width   	, width     	},
                {GA_Height  	, height    	},
                {GA_SysGadget   , TRUE      	},
                {GA_SysGType    , GTYP_SIZING   },
                {GA_BottomBorder, TRUE      	},
                {GA_RightBorder , TRUE      	},
                {GA_GZZGadget   , is_gzz    	},
                {TAG_DONE           	    	}
            };

            struct TagItem image_tags[] =
            {
                {IA_Width   	, width     	    	    	    	    	    	    	     },
                {IA_Height  	, height    	    	    	    	    	    	    	     },
                {SYSIA_Which    , SIZEIMAGE 	    	    	    	    	    	    	     },
                {SYSIA_DrawInfo , (IPTR)dri 	    	    	    	    	    	    	     },
                {SYSIA_Size 	, w->WScreen->Flags & SCREENHIRES ? SYSISIZE_MEDRES : SYSISIZE_LOWRES},
                {TAG_DONE           	    	    	    	    	    	    	    	     }
            };
            Object *im;

            im = NewObjectA(NULL, SYSICLASS, image_tags);
            if (!im)
            {
                sysgads_ok = FALSE;
            }
            else
            {
                size_tags[0].ti_Data = (IPTR)im;

                SYSGAD(w, SIZEGAD) = NewObjectA(NULL, BUTTONGCLASS, size_tags);

                if (!SYSGAD(w, SIZEGAD))
                {
                    DisposeObject(im);
                    sysgads_ok = FALSE;
                }
            }
        }

        if (w->Flags & WFLG_DEPTHGADGET)
        {
            struct TagItem depth_tags[] =
            {
                {GA_Image   	, 0                 },
              //{GA_RelRight  	, relright          },
                {GA_Top     	, 0                 },
	    #if SQUARE_WIN_GADGETS
                {GA_Width   	, TITLEBAR_HEIGHT   },
	    #endif
                {GA_Height  	, TITLEBAR_HEIGHT   },
                {GA_SysGadget   , TRUE              },
                {GA_SysGType    , GTYP_WDEPTH       },
                {GA_TopBorder   , TRUE              },
                {GA_GZZGadget   , is_gzz            },
                {GA_RelVerify   , TRUE              },
                {TAG_DONE                   	    }
            };

            struct TagItem image_tags[] =
            {
    	#ifdef SKINS
            	{TAG_IGNORE 	, 0         	    	    	    },
    	#else
            	{IA_Left    	, -1                	    	    },
    	#endif
    	    #if SQUARE_WIN_GADGETS
                {IA_Width   	, TITLEBAR_HEIGHT + 1       	    },
	    #endif
                {IA_Height  	, TITLEBAR_HEIGHT   	    	    },
                {SYSIA_Which    , DEPTHIMAGE        	    	    },
                {SYSIA_DrawInfo , (IPTR)dri         	    	    },
                {SYSIA_Size 	, w->WScreen->Flags & SCREENHIRES ?
                    	    	  SYSISIZE_MEDRES : SYSISIZE_LOWRES },
                {TAG_DONE                   	    	    	    }
            };
            Object *im;

            im = NewObjectA(NULL, SYSICLASS, image_tags);
            if (!im)
            {
                sysgads_ok = FALSE;
            }
            else
            {
                depth_tags[0].ti_Data = (IPTR)im;

                SYSGAD(w, DEPTHGAD) = NewObjectA(NULL, BUTTONGCLASS, depth_tags);

                if (!SYSGAD(w, DEPTHGAD))
                {
                    DisposeObject(im);
                    sysgads_ok = FALSE;
                }
                else
                {
                    IPTR width;
                    GetAttr(GA_Width, SYSGAD(w, DEPTHGAD), &width);

                    /*****/
#ifndef SKINS
                    --width;
#endif

                    relright -= width;
                    db_width -= width;

                    {
                        struct TagItem gadtags[] =
                        {
                            {GA_RelRight, relright  },
                         /* {GA_Width   , width     }, */
                            {TAG_DONE               }
                        };

                        SetAttrsA(SYSGAD(w, DEPTHGAD), gadtags);
                    }
                }
            }

        }

    	/* RKRMs: window gets zoom gadget if WA_Zoom tag was used,
	   or if window has both a sizegadget and a depthgadget */
	
        if ((w->Flags & WFLG_HASZOOM) ||
	    ((w->Flags & WFLG_SIZEGADGET) && (w->Flags & WFLG_DEPTHGADGET)))
        {
            struct TagItem zoom_tags[] =
            {
                {GA_Image   	, 0                 },
                //{GA_RelRight  , relright          },
                {GA_Top     	, 0                 },
	    #if SQUARE_WIN_GADGETS
                {GA_Width   	, TITLEBAR_HEIGHT   },
	    #endif
                {GA_Height  	, TITLEBAR_HEIGHT   },
                {GA_SysGadget   , TRUE              },
                {GA_SysGType    , GTYP_WZOOM        },
                {GA_TopBorder   , TRUE              },
                {GA_GZZGadget   , is_gzz            },
                {GA_RelVerify   , TRUE              },
                {TAG_DONE                   	    }
            };

            struct TagItem image_tags[] =
            {
    	    #ifdef SKINS
                {TAG_IGNORE 	, 0         	    	    	    },
    	    #else
                {IA_Left    	, -1                	    	    },
    	    #endif
    	    #if SQUARE_WIN_GADGETS
                {IA_Width   	, TITLEBAR_HEIGHT + 1       	    },
	    #endif
                {IA_Height  	, TITLEBAR_HEIGHT   	    	    },
                {SYSIA_Which    , ZOOMIMAGE         	    	    },
                {SYSIA_DrawInfo , (IPTR)dri         	    	    },
                {SYSIA_Size 	, w->WScreen->Flags & SCREENHIRES ?
                    	    	  SYSISIZE_MEDRES : SYSISIZE_LOWRES },
                {TAG_DONE                   	    	    	    }
            };
            Object *im;

            im = NewObjectA(NULL, SYSICLASS, image_tags);
            if (!im)
            {
                sysgads_ok = FALSE;
            }
            else
            {
                zoom_tags[0].ti_Data = (IPTR)im;

                SYSGAD(w, ZOOMGAD) = NewObjectA(NULL, BUTTONGCLASS, zoom_tags);

                if (!SYSGAD(w, ZOOMGAD))
                {
                    DisposeObject(im);
                    sysgads_ok = FALSE;
                }
                else
                {
                    IPTR width;
                    GetAttr(GA_Width, SYSGAD(w, ZOOMGAD), &width);

                    --width;

                    relright -= width;
                    db_width -= width;

                    {
                        struct TagItem gadtags[] =
                            {
                                {GA_RelRight, relright},
                                {TAG_DONE             }
                            };


                        SetAttrsA(SYSGAD(w, ZOOMGAD), gadtags);
                    }
                }
            }
        }

        if (((struct IntWindow *)(w))->extrabuttons & ETG_LOCK)
        {
            ewidth = addextragadget(w,is_gzz,dri,relright,LOCKIMAGE,((struct IntWindow *)w)->extrabuttonsid + ETD_Lock,LOCKGAD,IntuitionBase);
            relright -= ewidth;
            db_width -= ewidth;
        }

        if (((struct IntWindow *)(w))->extrabuttons & ETG_ICONIFY)
        {
            ewidth = addextragadget(w,is_gzz,dri,relright,ICONIFYIMAGE,((struct IntWindow *)w)->extrabuttonsid + ETD_Iconify,ICONIFYGAD,IntuitionBase);
            relright -= ewidth;
            db_width -= ewidth;
        }

        if (((struct IntWindow *)(w))->extrabuttons & ETG_JUMP)
        {
            ewidth = addextragadget(w,is_gzz,dri,relright,JUMPIMAGE,((struct IntWindow *)w)->extrabuttonsid + ETD_Jump,JUMPGAD,IntuitionBase);
            relright -= ewidth;
            db_width -= ewidth;
        }

        if (((struct IntWindow *)(w))->extrabuttons & ETG_SNAPSHOT)
        {
            ewidth = addextragadget(w,is_gzz,dri,relright,SNAPSHOTIMAGE,((struct IntWindow *)w)->extrabuttonsid + ETD_Snapshot,SNAPSHOTGAD,IntuitionBase);
            relright -= ewidth;
            db_width -= ewidth;
        }

        if (((struct IntWindow *)(w))->extrabuttons & ETG_MUI)
        {
            ewidth = addextragadget(w,is_gzz,dri,relright,MUIIMAGE,((struct IntWindow *)w)->extrabuttonsid + ETD_MUI,MUIGAD,IntuitionBase);
            relright -= ewidth;
            db_width -= ewidth;
        }

        if (((struct IntWindow *)(w))->extrabuttons & ETG_POPUP)
        {
            ewidth = addextragadget(w,is_gzz,dri,relright,POPUPIMAGE,((struct IntWindow *)w)->extrabuttonsid + ETD_PopUp,POPUPGAD,IntuitionBase);
            relright -= ewidth;
            db_width -= ewidth;
        }

        if (w->Flags & WFLG_CLOSEGADGET)
        {
            struct TagItem close_tags[] =
            {
                {GA_Image   	, 0         	    },
                {GA_Left    	, 0         	    },
                {GA_Top     	, 0         	    },
	    #if SQUARE_WIN_GADGETS
                {GA_Width   	, TITLEBAR_HEIGHT   },
	    #endif
                {GA_Height  	, TITLEBAR_HEIGHT   },
                {GA_SysGadget   , TRUE              },
                {GA_SysGType    , GTYP_CLOSE        },
                {GA_TopBorder   , TRUE              },
                {GA_GZZGadget   , is_gzz            },
                {GA_RelVerify   , TRUE              },
                {TAG_DONE                   	    }
            };
            struct TagItem image_tags[] =
            {
	    #if SQUARE_WIN_GADGETS
                {IA_Width 	, TITLEBAR_HEIGHT + 1 	    	    },
	    #endif
                {IA_Height      , TITLEBAR_HEIGHT	    	    },
                {SYSIA_Which    , CLOSEIMAGE            	    },
                {SYSIA_DrawInfo , (IPTR)dri     	    	    },
                {SYSIA_Size     , w->WScreen->Flags & SCREENHIRES ?
                     	    	  SYSISIZE_MEDRES : SYSISIZE_LOWRES },
                {TAG_DONE               	    	    	    }
            };
            Object *im;

            im = NewObjectA(NULL, SYSICLASS, image_tags);
            if (!im)
            {
                sysgads_ok = FALSE;
            }
            else
            {
                close_tags[0].ti_Data = (IPTR)im;

                SYSGAD(w, CLOSEGAD) = NewObjectA(NULL, BUTTONGCLASS, close_tags);

                if (!SYSGAD(w, CLOSEGAD))
                {
                    DisposeObject(im);
                    sysgads_ok = FALSE;
                }
                else
                {
                    IPTR width;
                    GetAttr(GA_Width, SYSGAD(w, CLOSEGAD), &width);

                    db_left  += width;
                    db_width -= width;
                }
            }
        }

        if (w->Flags & WFLG_DRAGBAR)
        {

            struct TagItem dragbar_tags[] =
            {
        	{GA_Left    	, 0/*db_left*/	    },
        	{GA_Top     	, 0         	    },
        	{GA_RelWidth    , 0/*db_width*/     },
        	{GA_Height  	, TITLEBAR_HEIGHT   },
        	{GA_SysGadget   , TRUE              },
        	{GA_SysGType    , GTYP_WDRAGGING    },
        	{GA_TopBorder   , TRUE              },
        	{GA_GZZGadget   , is_gzz            },
        	{TAG_DONE                   	     }
            };
            SYSGAD(w, DRAGBAR) = NewObjectA(NULL, BUTTONGCLASS, dragbar_tags);

            if (!SYSGAD(w, DRAGBAR))
                sysgads_ok = FALSE;

        }


        D(bug("Dragbar:  %p\n", SYSGAD(w, DRAGBAR ) ));
        D(bug("Depthgad: %p\n", SYSGAD(w, DEPTHGAD) ));
        D(bug("Zoomgad:  %p\n", SYSGAD(w, ZOOMGAD ) ));
        D(bug("Closegad: %p\n", SYSGAD(w, CLOSEGAD) ));
        D(bug("Sizegad:  %p\n", SYSGAD(w, SIZEGAD ) ));

        /* Don't need drawinfo anymore */
        FreeScreenDrawInfo(w->WScreen, dri);

        if (sysgads_ok)
        {
            int i;


            D(bug("Adding gadgets\n"));
            for (i = NUM_SYSGADS; --i >= 0; )
            {
                if (SYSGAD(w, i))
                    AddGadget(w, (struct Gadget *)SYSGAD(w, i), 0);
            }

            ReturnBool("CreateWinSysGadgets", TRUE);

        } /* if (sysgads created) */

        KillWinSysGadgets(w, IntuitionBase);

    } /* if (got DrawInfo) */
    ReturnBool("CreateWinSysGadgets", FALSE);

}

/**********************************************************************************/

VOID KillWinSysGadgets(struct Window *w, struct IntuitionBase *IntuitionBase)
{
    /* Free system gadgets */
    UWORD i;

    for (i = 0; i < NUM_SYSGADS; i ++)
    {
        if (SYSGAD(w, i))
        {
            RemoveGadget( w, (struct Gadget *)SYSGAD(w, i));
            DisposeObject((Object *)((struct Gadget *)SYSGAD(w, i))->GadgetRender);
            DisposeObject( SYSGAD(w, i) );
        }
    }
}

/**********************************************************************************/

void CreateScreenBar(struct Screen *scr, struct IntuitionBase *IntuitionBase)
{
    BOOL  front = TRUE;
    ULONG backdrop = LAYERBACKDROP;
    WORD  ypos = 0;

#ifdef SKINS
    if (scr->Flags & SCREENQUIET || (GetPrivScreen(scr)->SpecialFlags & SF_InvisibleBar)) front = FALSE;
    if (GetPrivScreen(scr)->SpecialFlags & SF_AppearingBar)
    {
        backdrop = 0;
        ypos = - (scr->BarHeight + 1);
    }
#else
    if (scr->Flags & SCREENQUIET) front = FALSE;
#endif

    if (!scr->BarLayer)
    {
        if (front)
        {
            scr->BarLayer = CreateUpfrontHookLayer(&scr->LayerInfo,
                                                   scr->RastPort.BitMap,
                                                   0,
                                                   ypos,
                                                   scr->Width - 1,
                                                   scr->BarHeight + ypos, /* 1 pixel heigher than scr->BarHeight */
                                                   LAYERSIMPLE | backdrop,
                                                   LAYERS_NOBACKFILL,
                                                   NULL);
        }
	else
	{
            scr->BarLayer = CreateBehindHookLayer(&scr->LayerInfo,
                                                   scr->RastPort.BitMap,
                                                   0,
                                                   ypos,
                                                   scr->Width - 1,
                                                   scr->BarHeight + ypos, /* 1 pixel heigher than scr->BarHeight */
                                                   LAYERSIMPLE | backdrop,
                                                   LAYERS_NOBACKFILL,
                                                   NULL);
        }

        if (scr->BarLayer)
        {
            SetFont(scr->BarLayer->rp, ((struct IntScreen *)scr)->DInfo.dri_Font);
            RenderScreenBar(scr, FALSE, IntuitionBase);
        }

    }
}

/**********************************************************************************/

void KillScreenBar(struct Screen *scr, struct IntuitionBase *IntuitionBase)
{
    if (scr->BarLayer)
    {
        DeleteLayer(0, scr->BarLayer);
        scr->BarLayer = FALSE;
    }

}

/**********************************************************************************/

#ifdef SKINS
//RenderScreenBar moved to morphos/mosmisc.c
#endif

#ifndef SKINS

void RenderScreenBar(struct Screen *scr, BOOL refresh, struct IntuitionBase *IntuitionBase)
{

    struct DrawInfo *dri = &((struct IntScreen *)scr)->DInfo;
    struct RastPort *rp;

    if (scr->BarLayer)
    {
#if USE_NEWDISPLAYBEEP
        BOOL beeping;
#else
#define beeping 0
#endif

        rp = scr->BarLayer->rp;

        /* must lock GadgetLock to avoid deadlocks with ObtainGIRPort
           when calling refreshgadget inside layer update state */
        LockLayerInfo(scr->BarLayer->LayerInfo);
        LOCKGADGET
        LockLayer(NULL,scr->BarLayer);

#if USE_NEWDISPLAYBEEP
        beeping = (scr->Flags & BEEPING) && GetBitMapAttr(rp->BitMap, BMA_DEPTH) > 8;
#endif

        if (refresh) BeginUpdate(scr->BarLayer);


        /* real BarHeight = scr->BarHeight + 1 !!! */


        SetDrMd(rp, JAM2);
        SetAPen(rp, dri->dri_Pens[beeping ? BARDETAILPEN : BARBLOCKPEN]);
        RectFill(rp, 0, 0, scr->Width - 1, scr->BarHeight - 1);

        SetAPen(rp, dri->dri_Pens[beeping ? BARDETAILPEN : BARTRIMPEN]);
        RectFill(rp, 0, scr->BarHeight, scr->Width - 1, scr->BarHeight);

        if (scr->Title)
        {
#ifdef __MORPHOS__
            CONST_STRPTR title = patchScreenTitle(scr->Title);
#else
            //FIXME: what's patchScreenTitle supposed to do? anything we want for AROS?
            CONST_STRPTR title = scr->Title;
#endif /* __MORPHOS__ */

            SetAPen(rp, dri->dri_Pens[beeping ? BARBLOCKPEN: BARDETAILPEN]);
            SetBPen(rp, dri->dri_Pens[beeping ? BARDETAILPEN : BARBLOCKPEN]);

            Move(rp, scr->BarHBorder, scr->BarVBorder + rp->TxBaseline);

            Text(rp, title, strlen(title));
        }

        if (scr->FirstGadget)
        {
            RefreshBoopsiGadget(scr->FirstGadget, (struct Window *)scr, NULL, IntuitionBase);
        }

        if (refresh)
        {
            scr->BarLayer->Flags &= ~LAYERREFRESH;
            EndUpdate(scr->BarLayer, TRUE);
        }



        UnlockLayer(scr->BarLayer);
        UNLOCKGADGET
        UnlockLayerInfo(scr->BarLayer->LayerInfo);

    } /* if (scr->BarLayer) */

}

#endif

/**********************************************************************************/

void UpdateMouseCoords(struct Window *win)
{
    WORD scrmousex = win->WScreen->MouseX;
    WORD scrmousey = win->WScreen->MouseY;

    win->MouseX    = scrmousex - win->LeftEdge;
    win->MouseY    = scrmousey - win->TopEdge;

    /* stegerg: AmigaOS sets this even if window is not GZZ
       so we do the same as they are handy also for non-GZZ
       windows */

    win->GZZMouseX = scrmousex - (win->LeftEdge + win->BorderLeft);
    win->GZZMouseY = scrmousey - (win->TopEdge + win->BorderTop);
}

/**********************************************************************************/

/* subtract rectangle b from rectangle b. resulting rectangles will be put into
   destrectarray which must have place for at least 4 rectangles. Returns number
   of resulting rectangles */

#if 0  /* use <clib/macros.h> MAX/MIN macros */
#undef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

#undef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

WORD SubtractRectFromRect(struct Rectangle *a, struct Rectangle *b, struct Rectangle *destrectarray)
{
    struct Rectangle    intersect;
    BOOL            	intersecting = FALSE;
    WORD            	numrects = 0;

    /* calc. intersection between a and b */

    if (a->MinX <= b->MaxX)
    {
        if (a->MinY <= b->MaxY)
        {
            if (a->MaxX >= b->MinX)
            {
                if (a->MaxY >= b->MinY)
                {
                    intersect.MinX = MAX(a->MinX, b->MinX);
                    intersect.MinY = MAX(a->MinY, b->MinY);
                    intersect.MaxX = MIN(a->MaxX, b->MaxX);
                    intersect.MaxY = MIN(a->MaxY, b->MaxY);

                    intersecting = TRUE;
                }
            }
        }
    }

    if (!intersecting)
    {
        destrectarray[numrects++] = *a;

    } /* not intersecting */
    else
    {
        if (intersect.MinY > a->MinY) /* upper */
        {
            destrectarray->MinX = a->MinX;
            destrectarray->MinY = a->MinY;
            destrectarray->MaxX = a->MaxX;
            destrectarray->MaxY = intersect.MinY - 1;

            numrects++;
            destrectarray++;
        }

        if (intersect.MaxY < a->MaxY) /* lower */
        {
            destrectarray->MinX = a->MinX;
            destrectarray->MinY = intersect.MaxY + 1;
            destrectarray->MaxX = a->MaxX;
            destrectarray->MaxY = a->MaxY;

            numrects++;
            destrectarray++;
        }

        if (intersect.MinX > a->MinX) /* left */
        {
            destrectarray->MinX = a->MinX;
            destrectarray->MinY = intersect.MinY;
            destrectarray->MaxX = intersect.MinX - 1;
            destrectarray->MaxY = intersect.MaxY;

            numrects++;
            destrectarray++;
        }

        if (intersect.MaxX < a->MaxX) /* right */
        {
            destrectarray->MinX = intersect.MaxX + 1;
            destrectarray->MinY = intersect.MinY;
            destrectarray->MaxX = a->MaxX;
            destrectarray->MaxY = intersect.MaxY;

            numrects++;
            destrectarray++;
        }

    } /* intersecting */

    return numrects;

}

ULONG addextragadget(struct Window *w,BOOL is_gzz,struct DrawInfo *dri,LONG relright,ULONG imagetype,ULONG gadgetid,ULONG gadgettype,struct IntuitionBase *IntuitionBase)
{
    struct TagItem gadget_tags[] =
    {
        {GA_Image   	, 0                 },
        {GA_ToggleSelect, FALSE             },
        {GA_Top     	, 0                 },
        {GA_Height  	, TITLEBAR_HEIGHT   },
        {GA_TopBorder   , TRUE              },
        {GA_GZZGadget   , is_gzz            },
        {GA_ID      	, gadgetid          },
        {GA_RelVerify	, TRUE              },
        {TAG_DONE                           }
    };
    struct TagItem image_tags[] =
    {
    #ifdef SKINS
        {TAG_IGNORE 	, 0             	    	    },
    #else
        {IA_Left    	, -1            	    	    },
    #endif
        {IA_Height  	, TITLEBAR_HEIGHT       	    },
        {SYSIA_Which    , imagetype         	    	    },
        {SYSIA_DrawInfo , (IPTR)dri         	    	    },
        {SYSIA_Size 	, w->WScreen->Flags & SCREENHIRES ?
            	      SYSISIZE_MEDRES : SYSISIZE_LOWRES     },
        {TAG_DONE                   	    	    	    }
    };
    Object *im;

    if (gadgettype == LOCKGAD)
    gadget_tags[1].ti_Data = TRUE;

    im = NewObjectA(NULL, SYSICLASS, image_tags);
    if (im)
    {
        gadget_tags[0].ti_Data = (IPTR)im;

        SYSGAD(w, gadgettype) = NewObjectA(NULL, BUTTONGCLASS, gadget_tags);

        if (!SYSGAD(w, gadgettype))
        {
            DisposeObject(im);
        }
        else
        {
            IPTR width;
            GetAttr(GA_Width, SYSGAD(w, gadgettype), &width);

            --width;

            {
                struct TagItem gadtags[] =
                {
                    {GA_RelRight, relright - width  },
                    {TAG_DONE               	    }
                };

                SetAttrsA(SYSGAD(w, gadgettype), gadtags);
            }

            return width;
        }
    }

    return 0;

}

/**********************************************************************************/

LONG CalcResourceHash(APTR resource)
{
    LONG l1, l2, l3, l4, hash;
    
    /* FIXME: Probably sucks. I have no clue about this hash stuff */
    
    l1 = ((LONG)resource) & 0xFF;
    l2 = (((LONG)resource) >> 8) & 0xFF;
    l3 = (((LONG)resource) >> 16) & 0xFF;
    l4 = (((LONG)resource) >> 24) & 0xFF;
   
    hash = (l1 + l2 + l3 + l4) % RESOURCELIST_HASHSIZE;

    return hash;
}
/**********************************************************************************/

void AddResourceToList(APTR resource, UWORD resourcetype, struct IntuitionBase *IntuitionBase)
{
    struct HashNode *hn = NULL;
    LONG    	     hash;
    ULONG   	     ilock;
        
    switch(resourcetype)
    {
    	case RESOURCE_WINDOW:
	    hn = &((struct IntWindow *)resource)->hashnode;
	    hn->type = RESOURCE_WINDOW;
	    break;
	    
	default:
	    D(bug("AddResourceToList: Unknown resource type!!!\n"));
	    return;
    }  

    hash = CalcResourceHash(resource);

    hn->resource = resource;
	    
    ilock = LockIBase(0);
    AddTail((struct List *)&GetPrivIBase(IntuitionBase)->ResourceList[hash], (struct Node *)hn);
    UnlockIBase(ilock);
}

/**********************************************************************************/

void RemoveResourceFromList(APTR resource, UWORD resourcetype, struct IntuitionBase *IntuitionBase)
{
    struct HashNode *hn = NULL;
    ULONG   	     ilock;
    
    switch(resourcetype)
    {	
    	case RESOURCE_WINDOW:
	    hn = &((struct IntWindow *)resource)->hashnode;
	    break;
	    
	default:
	    D(bug("RemoveResourceFromList: Unknown resource type!!!\n"));
	    return;
    }
    
    if (hn->type != resourcetype)
    {
    	D(bug("RemoveResourceFromList: Panic. Resource Type mismatch!!!\n"));
    }
    
    ilock = LockIBase(0);
    Remove((struct Node *)hn);
    UnlockIBase(ilock);
}

/**********************************************************************************/

BOOL ResourceExisting(APTR resource, UWORD resourcetype, struct IntuitionBase *IntuitionBase)
{
    struct HashNode *hn = NULL;
    LONG    	     hash;
    ULONG   	     ilock;
    BOOL    	     exists = FALSE;
    
    hash = CalcResourceHash(resource);
    
    ilock = LockIBase(0);
    ForeachNode((struct List *)&GetPrivIBase(IntuitionBase)->ResourceList[hash], hn)
    {
    	if ((hn->resource == resource) && (hn->type == resourcetype))
	{
	    exists = TRUE;
	    break;
	}
    }
    UnlockIBase(ilock);
    
    return exists;
}
/**********************************************************************************/
