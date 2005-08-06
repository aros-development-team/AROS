/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <exec/memory.h>
#include <graphics/layers.h>
#include <graphics/gfx.h>
#include <intuition/intuition.h>
#include <intuition/imageclass.h>
#include <intuition/windecorclass.h>
#include <intuition/gadgetclass.h>
#include <intuition/extensions.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/layers.h>
#include <exec/ports.h>
#include "intuition_intern.h"
#include "inputhandler.h"
#include "inputhandler_actions.h"
#include "boopsigadgets.h"

#ifdef SKINS
    #include "transplayers.h"
    #include "intuition_extend.h"
#endif

#ifndef DEBUG_OpenWindow
#   define DEBUG_OpenWindow 0
#endif
#undef DEBUG
#define DEBUG 0
#if DEBUG_OpenWindow
#   define DEBUG 1
#endif
#   include <aros/debug.h>

struct OpenWindowActionMsg
{
    struct IntuiActionMsg    msg;
    struct Window   	    *window;
    struct BitMap   	    *bitmap;
    struct Hook     	    *backfillhook;
    struct Region   	    *shape;
    struct Hook     	    *shapehook;
    struct Layer    	    *parentlayer;
    BOOL    	    	     visible;
    BOOL    	    	     success;
};

static VOID int_openwindow(struct OpenWindowActionMsg *msg,
                           struct IntuitionBase *IntuitionBase);

/*****************************************************************************

    NAME */

AROS_LH1(struct Window *, OpenWindow,

         /*  SYNOPSIS */
         AROS_LHA(struct NewWindow *, newWindow, A0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 34, Intuition)

/*  FUNCTION
    Opens a new window with the characteristics specified in
    newWindow.

    INPUTS
    newWindow - How you would like your new window.

    RESULT
    A pointer to the new window or NULL if it couldn't be
    opened. Reasons for this might be lack of memory or illegal
    attributes.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
    CloseWindow(), ModifyIDCMP()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct OpenWindowActionMsg   msg;
    struct NewWindow        	 nw;
    struct Window   	    	*w = NULL, *helpgroupwindow = NULL, *parentwin = NULL;
    struct TagItem  	    	*tag, *tagList;
    struct RastPort 	    	*rp;
    struct Hook     	    	*backfillhook = LAYERS_BACKFILL, *shapehook = NULL;
    struct Region   	    	*shape = NULL;
    struct IBox     	    	*zoombox = NULL;
    struct Image    	    	*AmigaKey = NULL;
    struct Image    	    	*Checkmark = NULL;
    struct Layer    	    	*parentl = NULL;
#ifdef SKINS
    struct SkinInfo 	    	*skininfo = NULL;
    BOOL    	    	    	 hasskininfo = FALSE;
    struct Region   	    	*usertranspregion = NULL;
    struct Hook     	    	*usertransphook = NULL;
    struct MsgPort  	    	*userport = NULL;
#endif
    STRPTR          	    	 pubScreenName = NULL;
    UBYTE           	    	*screenTitle = NULL;
    BOOL            	    	 autoAdjust = FALSE, pubScreenFallBack = FALSE;
    ULONG           	    	 innerWidth = ~0L;
    ULONG           	    	 innerHeight = ~0L;
    WORD            	    	 mousequeue = DEFAULTMOUSEQUEUE;
    WORD            	    	 repeatqueue = 3; /* stegerg: test on my Amiga suggests this */
    ULONG           	    	 moreFlags = 0;
    ULONG           	    	 helpgroup = 0;
    ULONG           	    	 extrabuttons = 0, extrabuttonsid = ETI_Dummy;
  //ULONG     	    	    	 lock;
    ULONG                   	 windowvisible = TRUE;
    BOOL            	    	 driver_init_done = FALSE, have_helpgroup = FALSE;
    BOOL                    	 do_setwindowpointer = FALSE;

    ASSERT_VALID_PTR_ROMOK(newWindow);

    D(bug("OpenWindow (%p = { Left=%d Top=%d Width=%d Height=%d })\n"
          , newWindow
          , newWindow->LeftEdge
          , newWindow->TopEdge
          , newWindow->Width
          , newWindow->Height
         ));

    nw = *newWindow;

#define WFLG_PRIVATEFLAGS (WFLG_WINDOWREFRESH |\
WFLG_WINDOWTICKED  | WFLG_VISITOR       | \
WFLG_ZOOMED        |\
WFLG_WINDOWACTIVE )
    /* jDc*/
    /* WFLG_WBENCHWINDOW  | \*/
    /* this is used by WORKBENCH! */
    /* WFLG_HASZOOM       | \*/
    /* do NOT filter this! how do you think apps could manage to get a zoom image with struct NewWindow? */

    nw.Flags &= ~WFLG_PRIVATEFLAGS;

    if (newWindow->Flags & WFLG_NW_EXTENDED)
    {
        tagList = ((struct ExtNewWindow *)newWindow)->Extension;
    #if 1
        /* Sanitycheck the taglist pointer. Some Am*gaOS 1.3/2.x era
         * apps have WFLG_NW_EXTENDED set with bogus Extension taglist
         * pointer... (older CygnusED for example) - Piru
         */
        if (((ULONG) tagList & 1) || !TypeOfMem(tagList))
        {
            tagList = NULL;
        }
    #endif
    }
    else
    {
        tagList = NULL;
    }

    DEBUG_OPENWINDOW(dprintf("OpenWindow: NewWindow 0x%lx TagList 0x%lx\n",
                 newWindow, tagList));

    if (tagList)
    {
        ASSERT_VALID_PTR_ROMOK(tagList);

        /* Look at WA_Flags first, since boolean tags override part of it
         * even if they appear before it.
         */
        nw.Flags |= (GetTagData(WA_Flags, nw.Flags, tagList) & ~WFLG_PRIVATEFLAGS);

        while ((tag = NextTagItem (&tagList)))
        {
            /* ASSERT_VALID_PTR_ROMOK(tag); */

            DEBUG_OPENWINDOW(dprintf("OpenWindow: Tag 0x%08lx 0x%08lx\n",
                         tag->ti_Tag, tag->ti_Data));

            switch (tag->ti_Tag)
            {
            case WA_Left:
                nw.LeftEdge     = tag->ti_Data;
                break;

            case WA_Top:
                nw.TopEdge  	= tag->ti_Data;
                break;

            case WA_Width:
                nw.Width    	= tag->ti_Data;
                break;

            case WA_Height:
                nw.Height   	= tag->ti_Data;
                break;

            case WA_IDCMP:
                nw.IDCMPFlags   = tag->ti_Data;
                break;

            case WA_MinWidth:
                nw.MinWidth     = tag->ti_Data;
                break;

            case WA_MinHeight:
                nw.MinHeight    = tag->ti_Data;
                break;

            case WA_MaxWidth:
                nw.MaxWidth     = tag->ti_Data;
                break;

            case WA_MaxHeight:
                nw.MaxHeight    = tag->ti_Data;
                break;

            case WA_Gadgets:
                nw.FirstGadget  = (struct Gadget *)(tag->ti_Data);
                break;

            case WA_Title:
                nw.Title    = (UBYTE *)(tag->ti_Data);
                break;

            case WA_ScreenTitle:
                screenTitle = (UBYTE *)tag->ti_Data;
                break;

            case WA_AutoAdjust:
                autoAdjust  = (tag->ti_Data != 0);
                break;

            case WA_InnerWidth:
                innerWidth  = tag->ti_Data;
                break;

            case WA_InnerHeight:
                innerHeight = tag->ti_Data;
                break;

#define MODIFY_FLAG(name)   if (tag->ti_Data) \
nw.Flags |= (name); else nw.Flags &= ~(name)
#define MODIFY_MFLAG(name)  if (tag->ti_Data) \
moreFlags |= (name); else moreFlags &= ~(name)

            case WA_SizeGadget:
                MODIFY_FLAG(WFLG_SIZEGADGET);
                break;

            case WA_DragBar:
                MODIFY_FLAG(WFLG_DRAGBAR);
                break;

            case WA_DepthGadget:
                MODIFY_FLAG(WFLG_DEPTHGADGET);
                break;

            case WA_CloseGadget:
                MODIFY_FLAG(WFLG_CLOSEGADGET);
                break;

            case WA_Backdrop:
                MODIFY_FLAG(WFLG_BACKDROP);
                break;

            case WA_ReportMouse:
                MODIFY_FLAG(WFLG_REPORTMOUSE);
                break;

            case WA_NoCareRefresh:
                MODIFY_FLAG(WFLG_NOCAREREFRESH);
                break;

            case WA_Borderless:
                MODIFY_FLAG(WFLG_BORDERLESS);
                break;

            case WA_Activate:
                MODIFY_FLAG(WFLG_ACTIVATE);
                break;

            case WA_RMBTrap:
                MODIFY_FLAG(WFLG_RMBTRAP);
                break;

            case WA_WBenchWindow:
                MODIFY_FLAG(WFLG_WBENCHWINDOW);
                break;

            case WA_SizeBRight:
                MODIFY_FLAG(WFLG_SIZEBRIGHT);
                break;

            case WA_SizeBBottom:
                MODIFY_FLAG(WFLG_SIZEBBOTTOM);
                break;

            case WA_GimmeZeroZero:
                MODIFY_FLAG(WFLG_GIMMEZEROZERO);
                break;

            case WA_NewLookMenus:
                MODIFY_FLAG(WFLG_NEWLOOKMENUS);
                break;

            case WA_Zoom:
                zoombox = (struct IBox *)tag->ti_Data;
                DEBUG_OPENWINDOW(dprintf("OpenWindow: zoom %d %d %d %d\n",
                             zoombox->Left, zoombox->Top, zoombox->Width, zoombox->Height));
                MODIFY_FLAG(WFLG_HASZOOM);
                break;

            case WA_DetailPen:
                if (nw.DetailPen == 0xFF)
                    nw.DetailPen = tag->ti_Data;
                break;

            case WA_BlockPen:
                if (nw.BlockPen == 0xFF)
                    nw.BlockPen = tag->ti_Data;
                break;

            case WA_CustomScreen:
                nw.Screen = (struct Screen *)(tag->ti_Data);
                nw.Type = CUSTOMSCREEN;
                break;

            case WA_SuperBitMap:
                nw.Flags |= WFLG_SUPER_BITMAP;
                nw.BitMap = (struct BitMap *)(tag->ti_Data);
                break;

            case WA_SimpleRefresh:
                if (tag->ti_Data)
                    nw.Flags |= WFLG_SIMPLE_REFRESH;
                break;

            case WA_SmartRefresh:
                if (tag->ti_Data)
                    nw.Flags |= WFLG_SMART_REFRESH;
                break;

            case WA_PubScreenFallBack:
                pubScreenFallBack = (tag->ti_Data ? TRUE : FALSE);
                break;

            case WA_PubScreenName:
                //nw.Type = PUBLICSCREEN; //handled below!!
                pubScreenName = (STRPTR)tag->ti_Data;
                break;

            case WA_PubScreen:
                nw.Type = PUBLICSCREEN;
                nw.Screen = (struct Screen *)tag->ti_Data;
                break;

            case WA_BackFill:
                backfillhook = (struct Hook *)tag->ti_Data;
                break;

            case WA_MouseQueue:
                mousequeue = tag->ti_Data;
                break;

                /* These two are not implemented in AmigaOS */
            case WA_WindowName:
            case WA_Colors:
                break;

            case WA_NotifyDepth:
                MODIFY_MFLAG(WMFLG_NOTIFYDEPTH);
                break;

            case WA_RptQueue:
                repeatqueue = tag->ti_Data;
                break;

            case WA_Checkmark:
                Checkmark = (struct Image *)tag->ti_Data;
                break;

            case WA_AmigaKey:
                AmigaKey = (struct Image *)tag->ti_Data;
                break;

            case WA_HelpGroup:
                helpgroup = (ULONG)tag->ti_Data;
                have_helpgroup = TRUE;
                break;

            case WA_HelpGroupWindow:
                helpgroupwindow = (struct Window *)tag->ti_Data;
                break;

            case WA_MenuHelp:
                MODIFY_MFLAG(WMFLG_MENUHELP);
                break;

            case WA_PointerDelay:
                MODIFY_MFLAG(WMFLG_POINTERDELAY);
                break;

            case WA_TabletMessages:
                MODIFY_MFLAG(WMFLG_TABLETMESSAGES);
                break;

            case WA_ExtraTitlebarGadgets:
                extrabuttons = (ULONG)tag->ti_Data;
                break;

            case WA_ExtraGadgetsStartID:
                extrabuttonsid = (ULONG)tag->ti_Data;
                break;

            case WA_ExtraGadget_Iconify:
                if (tag->ti_Data)
                {
                    extrabuttons |= ETG_ICONIFY;
                }
                else
                {
                    extrabuttons &= ~ETG_ICONIFY;
                };
                break;

            case WA_ExtraGadget_Lock:
                if (tag->ti_Data)
                {
                    extrabuttons |= ETG_LOCK;
                }
                else
                {
                    extrabuttons &= ~ETG_LOCK;
                };
                break;

            case WA_ExtraGadget_MUI:
                if (tag->ti_Data)
                {
                    extrabuttons |= ETG_MUI;
                }
                else
                {
                    extrabuttons &= ~ETG_MUI;
                };
                break;

            case WA_ExtraGadget_PopUp:
                if (tag->ti_Data)
                {
                    extrabuttons |= ETG_POPUP;
                }
                else
                {
                    extrabuttons &= ~ETG_POPUP;
                };
                break;

            case WA_ExtraGadget_Snapshot:
                if (tag->ti_Data)
                {
                    extrabuttons |= ETG_SNAPSHOT;
                }
                else
                {
                    extrabuttons &= ~ETG_SNAPSHOT;
                };
                break;

            case WA_ExtraGadget_Jump:
                if (tag->ti_Data)
                {
                    extrabuttons |= ETG_JUMP;
                }
                else
                {
                    extrabuttons &= ~ETG_JUMP;
                };
                break;

    	#ifdef SKINS
            case WA_SkinInfo:
                skininfo = (struct SkinInfo *)tag->ti_Data;
                hasskininfo = TRUE;
                break;

            case WA_TransparentRegion:
                usertranspregion = (struct Region *)tag->ti_Data;
                usertransphook = NULL; //doesn't make sense
                break;

            case WA_TransparentRegionHook:
                usertransphook = (struct Hook *)tag->ti_Data;
                usertranspregion = NULL;
                break;

            case WA_UserPort:
                userport = (struct MsgPort *)tag->ti_Data;
                break;

            case WA_IAmMUI:
                MODIFY_MFLAG(WMFLG_IAMMUI);
                break;
    	#endif

    	#ifndef __MORPHOS__
            case WA_Shape:
                shape = (struct Region *)tag->ti_Data;
                break;

    	    case WA_ShapeHook:
	    	shapehook = (struct Hook *)tag->ti_Data;
		break;
		

            case WA_Parent:
                parentwin = ((struct Window *)tag->ti_Data);
                parentl   = parentwin->WLayer;
                break;

            case WA_Visible:
                windowvisible = (ULONG)tag->ti_Data;
                break;
    	#endif

            case WA_Pointer:
            case WA_BusyPointer:
                do_setwindowpointer = TRUE;
                break;

            } /* switch Tag */

        } /* while ((tag = NextTagItem (&tagList))) */

    } /* if (tagList) */

    if (nw.Flags & WFLG_SIZEGADGET)
    {
        if (!(nw.Flags & (WFLG_SIZEBRIGHT | WFLG_SIZEBBOTTOM)))
        {
            nw.Flags |= WFLG_SIZEBRIGHT;
        }

        //jDc: tested behavior of intuition68k
        nw.Flags |= WFLG_HASZOOM;
    }
    else
    {
        nw.Flags &= ~(WFLG_SIZEBRIGHT|WFLG_SIZEBBOTTOM);
    }

    if (nw.Flags & WFLG_BORDERLESS)
    {
        nw.Flags &= ~(WFLG_SIZEBRIGHT|WFLG_SIZEBBOTTOM|WFLG_SIZEGADGET);
    }

    /* Find out on which Screen the window must open */

        /* (cyfm 03/03/03 check for nw.Type == PUBLICSCREEN as well, some programs
         *  like TurboPrint GraphicPublisher specify {WA_PubScreen, NULL} and want
         *  to open on the default public screen that way 
         */

    if (!nw.Screen && (pubScreenName || (nw.Type == PUBLICSCREEN)))
    {
        struct Screen *pubs = 0;

        moreFlags |= WMFLG_DO_UNLOCKPUBSCREEN;
        pubs = LockPubScreen(pubScreenName);
        if (!pubs && pubScreenFallBack)
        {
            nw.Screen = LockPubScreen(NULL);
        }
        if (pubs)
        {
            nw.Screen = pubs;
        }
        nw.Type = PUBLICSCREEN;
        if (nw.Screen) nw.Flags |= WFLG_VISITOR;
    }

    if (!nw.Screen && nw.Type == WBENCHSCREEN)
    {
        nw.Screen = LockPubScreen("Workbench");
        if (nw.Screen)
        {
            moreFlags |= WMFLG_DO_UNLOCKPUBSCREEN;
            if (nw.Screen) nw.Flags |= WFLG_VISITOR;
        }
    }

    if (nw.Screen == NULL)
        goto failexit;

    w  = AllocMem (sizeof(struct IntWindow), MEMF_CLEAR);

    DEBUG_OPENWINDOW(dprintf("OpenWindow: Window 0x%lx\n", w));

    /* nlorentz: For now, creating a rastport becomes the responsibility of
       intui_OpenWindow(). This is because intui_OpenWindow() in
       config/hidd/intuition_driver.c must call CreateUpfrontLayer(),
       and that will create a rastport for the layer/window, and we don't
       want two rastports pr. window.
       Alternatively we may create a layers_driver.c driver for layers,
       and then call CreateUpfrontLayer() here from openwindow.
       For the Amiga window<-->X11 window stuff, the layers driver
       would just allocate a layer struct, a rastport and
       put the rasport into layer->RastPort, so we
       could get it inside this routine and put it into
       window->RPort;.

    */

    if (NULL == w)
        goto failexit;

    DEBUG_OPENWINDOW(dprintf("OpenWindow: Flags 0x%lx MoreFlags 0x%lx IDCMP 0x%lx\n",
                nw.Flags, moreFlags, nw.IDCMPFlags));

    w->WScreen = nw.Screen;

    #ifdef SKINS
    if (userport)
    {
        w->UserPort = userport;
        ((struct IntWindow *)w)->specialflags |= SPFLAG_USERPORT;
    }
    #endif

    if (!ModifyIDCMP (w, nw.IDCMPFlags))
        goto failexit;

    ((struct IntWindow *)w)->extrabuttons = extrabuttons;
    ((struct IntWindow *)w)->extrabuttonsid = extrabuttonsid;

    /*    w->RPort     = rp; */

    //w->FirstGadget = nw.FirstGadget;

    w->DetailPen = (nw.DetailPen != 0xFF) ? nw.DetailPen : w->WScreen->DetailPen;
    w->BlockPen  = (nw.BlockPen != 0xFF)  ? nw.BlockPen  : w->WScreen->BlockPen;

    /* Copy flags */
    w->Flags = nw.Flags;
    w->MoreFlags = moreFlags;

    if (!(w->Flags & WFLG_BORDERLESS))
    {
        w->BorderLeft   = w->WScreen->WBorLeft;
        w->BorderRight  = w->WScreen->WBorRight;
    #ifdef TITLEHACK
        w->BorderTop    = w->WScreen->WBorBottom;
    #else
        w->BorderTop    = w->WScreen->WBorTop;
    #endif
        w->BorderBottom = w->WScreen->WBorBottom;
    }

    if (nw.Title || (w->Flags & (WFLG_DRAGBAR | WFLG_CLOSEGADGET | WFLG_DEPTHGADGET)))
    {
        /* this is a hack. the correct way to "correct" (increase if necessary)
           the w->Border??? items would be to check all GACT_???BORDER gadgets
           (inclusive sysgadgets which are GACT_????BORDER gadgets as well) in
           nw.FirstGadget (or WA_Gadgets tag) and all sysgadgets and then
           make sure that each window border is big enough so that none of these
           gadgets extends outside the window border area */

    #ifdef TITLEHACK
        w->BorderTop    = w->WScreen->WBorTop;
    #endif
        /* Georg Steger: ??? font ??? */
        if (w->WScreen->Font)
            w->BorderTop += ((struct IntScreen *)(w->WScreen))->DInfo.dri.dri_Font->tf_YSize + 1;
        else
            w->BorderTop += GfxBase->DefaultFont->tf_YSize + 1;
    #ifndef TITLEHACK
        #ifdef SKINS
        if (hasskininfo)
        {
            struct windowclassprefs *wcprefs;
            struct IntDrawInfo      *dri;
            if ((dri = (struct IntDrawInfo *)GetScreenDrawInfo(w->WScreen)))
            {
                wcprefs = (struct windowclassprefs *)int_GetCustomPrefs(TYPE_WINDOWCLASS,dri,IntuitionBase);
                w->BorderTop += wcprefs->titlebarincrement;
                int_FreeCustomPrefs(TYPE_WINDOWCLASS,dri,IntuitionBase);
            }
        }
        #endif
    #endif
    }

    /* look for GACT_???BORDER gadgets which increase the BorderSizes */

    if (nw.FirstGadget)
    {
        struct Gadget *gad;

        for(gad = nw.FirstGadget; gad; gad = gad->NextGadget)
        {
            WORD gadx1, gady1, gadx2, gady2;

            if (gad->Activation & GACT_LEFTBORDER)
            {
                /* may never be GFLG_RELRIGHT / GFLG_RELWIDTH */

                gadx2 = gad->LeftEdge + gad->Width - 1;
                if (gadx2 >= w->BorderLeft) w->BorderLeft = gadx2/* + 1*/;
            }

            if (gad->Activation & GACT_TOPBORDER)
            {
                /* may never be GFLG_RELBOTTOM / GFLG_RELHEIGHT */

                gady2 = gad->TopEdge + gad->Height - 1;
                if (gady2 >= w->BorderTop) w->BorderTop = gady2/* + 1*/;
            }

            if (gad->Activation & GACT_RIGHTBORDER)
            {
                /* must be GFLG_RELRIGHT but never GFLG_RELWIDTH */

                gadx1 = -gad->LeftEdge;
                if (gadx1 >= w->BorderRight) w->BorderRight = gadx1/* + 1*/;
            }

            if (gad->Activation & GACT_BOTTOMBORDER)
            {
                /* must be GFLG_RELBOTTOM but never GFLG_RELHEIGHT */

                gady1 = -gad->TopEdge;
                if (gady1 >= w->BorderBottom) w->BorderBottom = gady1/* + 1*/;
            }

        } /* for(gad = nw.FirstGadget; gad; gad = gad->NextGadget) */

    } /* if (nw.FirstGadget) */

    if (!(w->Flags & WFLG_SIZEBRIGHT)) if (w->BorderRight > w->WScreen->WBorRight)
    {
        w->Flags |= WFLG_SIZEBRIGHT;
    }

    if (!(w->Flags & WFLG_SIZEBBOTTOM)) if (w->BorderBottom > w->WScreen->WBorBottom)
    {
        w->Flags |= WFLG_SIZEBBOTTOM;
    }

    
    //    if ((w->Flags & WFLG_SIZEGADGET) &&
    //       (w->Flags & (WFLG_SIZEBRIGHT | WFLG_SIZEBBOTTOM)))
    {
        IPTR 	    	 sizewidth = 16, sizeheight = 16;
        struct Image 	*im;
        struct DrawInfo *dri;

        if ((dri = GetScreenDrawInfo(w->WScreen)))
        {
            struct TagItem imtags[] =
            {
                {SYSIA_DrawInfo , (STACKIPTR)dri    	    	    	    	    	    	    	},
                {SYSIA_Which	, SIZEIMAGE 	    	    	    	    	    	    	    	},
                {SYSIA_Size 	, w->WScreen->Flags & SCREENHIRES ? SYSISIZE_MEDRES : SYSISIZE_LOWRES	},
                {TAG_DONE   	    	    	    	    	    	    	    	    	    	}
            };

            if ((im = NewObjectA(NULL, SYSICLASS, imtags)))
            {
                GetAttr(IA_Width, (Object *)im, &sizewidth);
                GetAttr(IA_Height, (Object *)im, &sizeheight);

                DisposeObject(im);
            }
            FreeScreenDrawInfo(w->WScreen, dri);
        }

        if (w->Flags & WFLG_SIZEBRIGHT)
        {
            if (w->BorderRight < sizewidth) w->BorderRight = sizewidth;
        }

        if (w->Flags & WFLG_SIZEBBOTTOM)
        {
            if (w->BorderBottom < sizeheight) w->BorderBottom = sizeheight;
        }

        IW(w)->sizeimage_width = sizewidth;
        IW(w)->sizeimage_height = sizeheight;

        /* now increase window size if it's necessary */
    }

#ifdef SKINS
    {
        IW(w)->custombackfill.h_Entry = (HOOKFUNC)HookEntry;
        IW(w)->custombackfill.h_SubEntry = (HOOKFUNC)GradientizeBackfillFunc;
        IW(w)->custombackfill.h_Data = &IW(w)->hd;
        IW(w)->hd.intuitionBase = IntuitionBase;
        
        IW(w)->usertranspregion = usertranspregion;
        IW(w)->usertransphook = usertransphook;
    }
#endif

    if (innerWidth != ~0L) nw.Width = innerWidth + w->BorderLeft + w->BorderRight;
    if (innerHeight != ~0L) nw.Height = innerHeight + w->BorderTop + w->BorderBottom;

    {
        LONG parentwidth;
        LONG parentheight;

        parentwidth  = parentwin ? parentwin->Width : w->WScreen->Width;
        parentheight = parentwin ? parentwin->Height : w->WScreen->Height;

        w->Width   = (nw.Width  != ~0) ? nw.Width  : parentwidth - nw.LeftEdge;
        w->Height  = (nw.Height != ~0) ? nw.Height : parentheight - nw.TopEdge;

        if (autoAdjust)
        {

            if (w->Width  > parentwidth)  w->Width  = parentwidth;
            if (w->Height > parentheight) w->Height = parentheight;

            if (nw.LeftEdge < 0) nw.LeftEdge = 0;
            if (nw.TopEdge < 0) nw.TopEdge = 0;

            if ((nw.LeftEdge + w->Width) > parentwidth)
                nw.LeftEdge = parentwidth - w->Width;

            if ((nw.TopEdge + w->Height) > parentheight)
                nw.TopEdge = parentheight - w->Height;
        }

        w->GZZWidth  = w->Width  - w->BorderLeft - w->BorderRight;
        w->GZZHeight = w->Height - w->BorderTop  - w->BorderBottom;
    }

    if (nw.LeftEdge < 0 || nw.TopEdge < 0 ||
        nw.LeftEdge + w->Width > w->WScreen->Width ||
        nw.TopEdge + w->Height > w->WScreen->Height)
        goto failexit;

    if (NULL == parentwin)
    {
        w->LeftEdge    = nw.LeftEdge;
        w->TopEdge     = nw.TopEdge;
    }
    else
    {
        w->LeftEdge    = nw.LeftEdge + parentwin->LeftEdge;
        w->TopEdge     = nw.TopEdge  + parentwin->TopEdge;
    }

#ifndef __MORPHOS__
    w->RelLeftEdge = nw.LeftEdge;
    w->RelTopEdge  = nw.TopEdge;
#endif

    w->MinWidth  = (nw.MinWidth  != 0) ? nw.MinWidth  : w->Width;
    w->MinHeight = (nw.MinHeight != 0) ? nw.MinHeight : w->Height;
    w->MaxWidth  = (nw.MaxWidth  != 0) ? nw.MaxWidth  : w->Width;
    w->MaxHeight = (nw.MaxHeight != 0) ? nw.MaxHeight : w->Height;

    //jDc: tested behavior of intuition68k
    if ((UWORD)w->MaxWidth < w->Width) w->MaxWidth = w->Width;
    if ((UWORD)w->MaxHeight < w->Height) w->MaxHeight = w->Height;

    /* check if maxwidth/height is not bigger than screen */
    if (w->MaxWidth > w->WScreen->Width) w->MaxWidth = w->WScreen->Width;
    if (w->MaxHeight > w->WScreen->Height) w->MaxHeight = w->WScreen->Height;

    if (zoombox)
    {
        ((struct IntWindow *)w)->ZipLeftEdge = zoombox->Left;
        ((struct IntWindow *)w)->ZipTopEdge  = zoombox->Top;
        ((struct IntWindow *)w)->ZipWidth    = zoombox->Width;
        ((struct IntWindow *)w)->ZipHeight   = zoombox->Height;
    }
    else
    {
        ((struct IntWindow *)w)->ZipLeftEdge = nw.LeftEdge;
        ((struct IntWindow *)w)->ZipTopEdge  = nw.TopEdge;	
        ((struct IntWindow *)w)->ZipWidth    = (w->Width == w->MinWidth) ? w->MaxWidth : w->MinWidth;
        ((struct IntWindow *)w)->ZipHeight   = (w->Height == w->MinHeight) ? w->MaxHeight : w->MinHeight;
    }

    DEBUG_OPENWINDOW(dprintf("OpenWindow: zip %d %d %d %d\n",
                	     ((struct IntWindow *)w)->ZipLeftEdge,
                	     ((struct IntWindow *)w)->ZipTopEdge,
                	     ((struct IntWindow *)w)->ZipWidth,
                	     ((struct IntWindow *)w)->ZipHeight));


    IW(w)->mousequeue = mousequeue;
    IW(w)->repeatqueue = repeatqueue;

    /* Amiga and checkmark images for menus */

    IW(w)->Checkmark = Checkmark ? Checkmark :
                    	    	   ((struct IntScreen *)(w->WScreen))->DInfo.dri.dri_CheckMark;

    IW(w)->AmigaKey  = AmigaKey  ? AmigaKey  :
                    	    	   ((struct IntScreen *)(w->WScreen))->DInfo.dri.dri_AmigaKey;

#ifndef __MORPHOS__
    /* child support */
    if (NULL != parentwin)
    {
        if (parentwin->firstchild)
            parentwin->firstchild->prevchild = w;

        w->nextchild = parentwin->firstchild;
        parentwin->firstchild = w;
        w->parent = parentwin;
    }
#endif

#ifdef SKINS
    if (hasskininfo) ((struct IntWindow *)(w))->specialflags = SPFLAG_SKININFO;
#endif

#ifdef DAMAGECACHE
    if ((w->Flags & WFLG_SIMPLE_REFRESH) && (IS_DOCAREREFRESH(w)) && (!(w->Flags & WFLG_BORDERLESS))) IW(w)->trashregion = NewRegion();
#endif

    /* Help stuff */

    if (!have_helpgroup && helpgroupwindow)
    {
        if (IW(helpgroupwindow)->helpflags & HELPF_ISHELPGROUP)
        {
            helpgroup = IW(helpgroupwindow)->helpgroup;
            have_helpgroup = TRUE;
        }
    }

    if (have_helpgroup)
    {
        IW(w)->helpflags |= HELPF_ISHELPGROUP;
        IW(w)->helpgroup = helpgroup;
    }

    w->Title = nw.Title;


#if 1

    /* Use safe OpenFont.. - Piru
     */
     
    if (GetPrivScreen(w->WScreen)->SpecialFlags & SF_SysFont)
    {
    	w->IFont = SafeReopenFont(IntuitionBase, &GfxBase->DefaultFont);
    }
    else
    {
    	w->IFont = SafeReopenFont(IntuitionBase, &(GetPrivScreen(w->WScreen)->DInfo.dri.dri_Font));
    }
    
    if (w->IFont == NULL)
        goto failexit;

#else

#warning: Really hacky way of re-opening GfxBase->DefaultFont

    Forbid();
    w->IFont = GfxBase->DefaultFont;
    w->IFont->tf_Accessors++;
    Permit();

#endif

    /* jDc: intui68k waits before opening the window until
    ** Move/SizeWindow actions are over (does it mean it's executed on
    ** caller's context?).
    */

#ifdef USEWINDOWLOCK
    if (!(FindTask(0) == ((struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data)->InputDeviceTask))
    {
        ObtainSemaphore(&GetPrivIBase(IntuitionBase)->WindowLock);
    }
#endif

    msg.window = w;
    msg.bitmap = nw.BitMap;
    //msg.backfillhook = backfillhook == LAYERS_BACKFILL ? &IW(w)->custombackfill : backfillhook;
    msg.backfillhook = backfillhook;
    msg.shape = shape;
    msg.shapehook = shapehook;
    msg.parentlayer = parentl;
    msg.visible = windowvisible;

    DoSyncAction((APTR)int_openwindow, &msg.msg, IntuitionBase);

#ifdef USEWINDOWLOCK
    if (!(FindTask(0) == ((struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data)->InputDeviceTask))
    {
        ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->WindowLock);
    }
#endif

    if (!msg.success)
        goto failexit;

    /* nlorentz: The driver has in some way or another allocated a rastport for us,
       which now is ready for us to use. */

    driver_init_done = TRUE;
    rp = w->RPort;

    D(bug("called driver, rp=%p\n", rp));

    /* The window RastPort always gets the font from GfxBase->DefaultFont, which
       is the system's default font. Usually topaz 8, but it can be changed with
       the Fonts prefs program to another fixed-sized font. */

    SetFont (rp, w->IFont);

    D(bug("set fonts\n"));

#warning Remove workaround!
    /* lbischoff: The following 4 Setxxx lines are a workaround for the InitRastPort
       problem (Bug #75 in docs/BUGS). They ensure that at least a window's rastport
       is initialized correctly. Remove them if they are not needed any longer!
    */
    SetABPenDrMd (rp, rp->FgPen, rp->BgPen, rp->DrawMode);
    SetWriteMask (rp, rp->Mask);
    D(bug("set pens\n"));


    /* Send all GA_RelSpecial BOOPSI gadgets in the list the GM_LAYOUT msg */
    /*DoGMLayout(w->FirstGadget, w, NULL, -1, TRUE, IntuitionBase);

    if (NULL != w->FirstGadget)
        RefreshGadgets (w->FirstGadget, w, NULL);
    */

    if (nw.FirstGadget)
    {
    	struct IntDrawInfo     	      *dri = &((struct IntScreen *)(w->WScreen))->DInfo;
    	struct wdpLayoutBorderGadgets  msg;

	msg.MethodID 	    	= WDM_LAYOUT_BORDERGADGETS;
	msg.wdp_Window 	    	= w;
	msg.wdp_Gadgets     	= nw.FirstGadget;
	msg.wdp_Flags   	= WDF_LBG_INITIAL | WDF_LBG_MULTIPLE;

	LOCKSHARED_WINDECOR(dri);
	DoMethodA(dri->dri_WinDecorObj, (Msg)&msg);	
	UNLOCK_WINDECOR(dri);

        AddGList(w, nw.FirstGadget, -1, -1, NULL);
    }

#if 0
    /* !!! This does double refreshing as the system gadgets also are refreshed
       in the above RfreshGadgets() call */
    if (nw.Flags & WFLG_ACTIVATE)
    {
        /* RefreshWindowFrame() will be called from within ActivateWindow().
        No point in doing double refreshing. */

        ActivateWindow(w);
    }
    else
    {
        RefreshWindowFrame(w);
    }
#endif

    if (screenTitle != NULL)
        SetWindowTitles (w, (CONST_STRPTR)~0L, screenTitle);

    UpdateMouseCoords(w);

    if (do_setwindowpointer)
    {
        //jDc: main for () loop destroys original taglist pointer, we need to get
        //it once again here!
        tagList = (struct TagItem *)((struct ExtNewWindow *)newWindow)->Extension;
        SetWindowPointerA(w, tagList);
    }

    goto exit;

failexit:
    D(bug("fail\n"));

    if (w)
    {
        ModifyIDCMP (w, 0L);

        /* nlorentz: Freeing the rasport is now intui_CloseWindow()'s task.

           if (rp)
           {
               FreeRastPort (rp);
           }
        */

        if (driver_init_done)
            intui_CloseWindow(w, IntuitionBase);

        if (w->IFont) CloseFont(w->IFont);

        FreeMem (w, sizeof(struct IntWindow));

        w = NULL;
    }

    if (nw.Screen && (moreFlags & WMFLG_DO_UNLOCKPUBSCREEN))
    {
        UnlockPubScreen(NULL, nw.Screen);
    }

exit:

    DEBUG_OPENWINDOW(dprintf("OpenWindow: Return 0x%lx\n", w));

    ReturnPtr ("OpenWindow", struct Window *, w);
    AROS_LIBFUNC_EXIT
} /* OpenWindow */



/**********************************************************************************/

static VOID int_openwindow(struct OpenWindowActionMsg *msg,
                           struct IntuitionBase *IntuitionBase)
{
    struct Window * w = msg->window;
    struct BitMap * SuperBitMap = msg->bitmap;
    struct Hook   * backfillhook = msg->backfillhook;
#ifdef CreateLayerTagList
    struct Region * shape = msg->shape;
    struct Hook   * shapehook = msg->shapehook;
    struct Layer  * parent = msg->parentlayer;
    BOOL            visible = msg->visible;
#endif

#ifdef SKINS
    BOOL            installtransphook = FALSE;
    BOOL            notransphook = TRUE;
#endif

    /* Create a layer for the window */
    LONG layerflags = 0;

    EnterFunc(bug("int_OpenWindow(w=%p)\n", w));

    D(bug("screen: %p\n", w->WScreen));
    D(bug("bitmap: %p\n", w->WScreen->RastPort.BitMap));

    /* Just insert some default values, should be taken from
       w->WScreen->WBorxxxx */

    /* Set the layer's flags according to the flags of the
    ** window
    */

    /* refresh type */
    if (w->Flags & WFLG_SIMPLE_REFRESH)
    {
        layerflags |= LAYERSIMPLE;
    }
    else
    {
        if (w->Flags & WFLG_SUPER_BITMAP && SuperBitMap)
	{
            layerflags |= LAYERSUPER;
	}
        else
	{
            layerflags |= LAYERSMART;
	}
    }
    
    if (w->Flags & WFLG_BACKDROP)
    {
        layerflags |= LAYERBACKDROP;
    }

    D(bug("Window dims: (%d, %d, %d, %d)\n",
          w->LeftEdge, w->TopEdge, w->Width, w->Height));

#ifdef SKINS
    //install transp layer hook!
    {
        struct windowclassprefs *wcprefs = NULL;
	
        wcprefs = (struct windowclassprefs*)int_GetCustomPrefs(TYPE_WINDOWCLASS,&((struct IntScreen*)(w->WScreen))->DInfo,IntuitionBase);
        if (wcprefs->flags & WINDOWCLASS_PREFS_ROUNDEDEDGES && (w->Title || (w->Flags & (WFLG_DRAGBAR | WFLG_CLOSEGADGET | WFLG_DEPTHGADGET))))
        {
            IW(w)->transpregion = NewRegion();
            if (IW(w)->transpregion)
            {
                installtransphook = TRUE; notransphook = FALSE;
                IW(w)->specialflags |= SPFLAG_TRANSPHOOK;
            }
        }
	else
	{
            if (IW(w)->usertransphook)
            {
                IW(w)->transpregion = NewRegion();
                if (IW(w)->transpregion)
                {
                    installtransphook = TRUE;
                    IW(w)->specialflags |= SPFLAG_TRANSPHOOK;
                }
            }
	    else if (IW(w)->usertranspregion)
            {
                IW(w)->transpregion = NewRegion();
                if (IW(w)->transpregion)
                {
                    installtransphook = TRUE;
                    IW(w)->specialflags |= SPFLAG_TRANSPHOOK;
                }
            }
        }
        int_FreeCustomPrefs(TYPE_SYSICLASS,&((struct IntScreen*)(w->WScreen))->DInfo,IntuitionBase);
    }
#endif

//  LockLayers(&w->WScreen->LayerInfo);

    /* A GimmeZeroZero window??? */
    if (w->Flags & WFLG_GIMMEZEROZERO)
    {
        /*
          A GimmeZeroZero window is to be created:
            - the outer window will be a simple refresh layer
            - the inner window will be a layer according to the flags
          What is the size of the inner/outer window supposed to be???
          I just make it that the outer window has the size of what is requested
        */

    #ifdef __MORPHOS__

        struct TagItem layertags[] =
        {
            {LA_BackfillHook, (IPTR)LAYERS_NOBACKFILL},
            {SuperBitMap ? LA_SuperBitMap : TAG_IGNORE, (IPTR)SuperBitMap},
            {installtransphook ? LA_TransHook : TAG_IGNORE, notransphook ? (IPTR)&((struct IntIntuitionBase *)(IntuitionBase))->notransphook : (IPTR)&((struct IntIntuitionBase *)(IntuitionBase))->transphook},
            {installtransphook ? LA_TransRegion : TAG_IGNORE, (IPTR)IW(w)->transpregion},
            {LA_WindowPtr, (IPTR)w},
            {TAG_DONE}
        };



        /* First create outer window */
        struct Layer * L = CreateUpfrontLayerTagList(
                                   &w->WScreen->LayerInfo
                                   , w->WScreen->RastPort.BitMap
                                   , w->LeftEdge
                                   , w->TopEdge
                                   , w->LeftEdge + w->Width - 1
                                   , w->TopEdge  + w->Height - 1
                                   , LAYERSIMPLE | (layerflags & LAYERBACKDROP)
                                   , (struct TagItem *)&layertags);

    #else
        /* First create outer window */

        struct TagItem lay_tags[] =
        {
            {LA_Hook        , (IPTR)LAYERS_NOBACKFILL    	    	    	    	    	},
            {LA_Priority    , (layerflags & LAYERBACKDROP) ? BACKDROPPRIORITY : UPFRONTPRIORITY },
            {LA_SuperBitMap , (IPTR)NULL                          	    	    	    	},
            {LA_ChildOf     , (IPTR)parent                              	    	    	},
            {LA_Visible     , (ULONG)visible                                                    },
            {TAG_DONE                                           	    	    	    	}
        };

        struct Layer * L = CreateLayerTagList(&w->WScreen->LayerInfo,
                        		       w->WScreen->RastPort.BitMap,
                        		       w->RelLeftEdge,
                        		       w->RelTopEdge,
                        		       w->RelLeftEdge + w->Width - 1,
                        		       w->RelTopEdge + w->Height - 1,
                        		       LAYERSIMPLE | (layerflags & LAYERBACKDROP),
                        		       lay_tags);

    #endif
        /* Could the layer be created. Nothing bad happened so far, so simply leave */
        if (NULL == L)
        {
            msg->success = FALSE;
//            UnlockLayers(&w->WScreen->LayerInfo);
            ReturnVoid("intui_OpenWindow(No GimmeZeroZero layer)");
        }

        D(bug("created outer GimmeZeroZero layer.\n"));

        /* install it as the BorderRPort */
        w->BorderRPort = L->rp;
        BLAYER(w) = L;

        /* This layer belongs to a window */
    #ifndef __MORPHOS__
        L->Window = (APTR)w;
    #endif

    #ifdef __MORPHOS__
        /* Now comes the inner window */
        layertags[0].ti_Data = (IPTR)backfillhook;
        w->WLayer = CreateUpfrontLayerTagList(
                &w->WScreen->LayerInfo
                , w->WScreen->RastPort.BitMap
                , w->LeftEdge + w->BorderLeft
                , w->TopEdge  + w->BorderTop
                , w->LeftEdge + w->BorderLeft + w->GZZWidth - 1
                , w->TopEdge  + w->BorderTop + w->GZZHeight - 1
                , layerflags
                , (struct TagItem *)&layertags);

    #else
        /* Now comes the inner window */

    	{
            struct TagItem lay_tags[] =
            {
                {LA_Hook     	, (IPTR)backfillhook	    	    	    	    	    	    },
                {LA_Priority    , (layerflags & LAYERBACKDROP) ? BACKDROPPRIORITY : UPFRONTPRIORITY },
                {LA_Shape       , (IPTR)shape                               	    	    	    },
		{LA_ShapeHook	, (IPTR)shapehook   	    	    	    	    	    	    },
                {LA_SuperBitMap , (IPTR)SuperBitMap                           	    	    	    },
                {LA_ChildOf     , (IPTR)parent                              	    	    	    },
                {LA_Visible     , (ULONG)visible                                                    },
                {TAG_DONE                                           	    	    	    	    }
            };

            w->WLayer = CreateLayerTagList(&w->WScreen->LayerInfo,
                        		    w->WScreen->RastPort.BitMap,
                        		    w->RelLeftEdge + w->BorderLeft,
                        		    w->RelTopEdge + w->BorderTop,
                        		    w->RelLeftEdge + w->BorderLeft + w->GZZWidth - 1,
                        		    w->RelTopEdge + w->BorderTop + w->GZZHeight - 1,
                        		    layerflags,
                        		    lay_tags);
					    
	}

    #endif

        /* could this layer be created? If not then delete the outer window and exit */
        if (NULL == w->WLayer)
        {
            DeleteLayer(0, L);
            msg->success = FALSE;
//            UnlockLayers(&w->WScreen->LayerInfo);
            ReturnVoid("intui_OpenWindow(No window layer)");
        }

        /* That should do it, I guess... */
    }
    else
    {
    #ifdef __MORPHOS__

        struct TagItem layertags[] =
        {
            {LA_BackfillHook, (IPTR)backfillhook},
            {SuperBitMap ? LA_SuperBitMap : TAG_IGNORE, (IPTR)SuperBitMap},
            {installtransphook ? LA_TransHook : TAG_IGNORE, notransphook ? (IPTR)&((struct IntIntuitionBase *)(IntuitionBase))->notransphook : (IPTR)&((struct IntIntuitionBase *)(IntuitionBase))->transphook},
            {installtransphook ? LA_TransRegion : TAG_IGNORE, (IPTR)IW(w)->transpregion},
            {LA_WindowPtr, (IPTR)w},
            {TAG_DONE}
        };

        D(dprintf("CreateUpfontLayerTagList(taglist 0x%lx)\n",&layertags));

        w->WLayer = CreateUpfrontLayerTagList(      &w->WScreen->LayerInfo,
                                w->WScreen->RastPort.BitMap,
                                w->LeftEdge,
                                w->TopEdge,
                                w->LeftEdge + w->Width - 1,
                                w->TopEdge  + w->Height - 1,
                                layerflags,
                                (struct TagItem *)&layertags);
    #else

        struct TagItem lay_tags[] =
        {
            {LA_Hook        , (IPTR)backfillhook	    	    	    	    	    	},
            {LA_Priority    , (layerflags & LAYERBACKDROP) ? BACKDROPPRIORITY : UPFRONTPRIORITY },
            {LA_Shape       , (IPTR)shape                               	    	    	},
	    {LA_ShapeHook   , (IPTR)shapehook   	    	    	    	    	    	},
            {LA_SuperBitMap , (IPTR)SuperBitMap                           	    	    	},
            {LA_ChildOf     , (IPTR)parent                              	    	    	},
            {LA_Visible     , (ULONG)visible                                                    },
            {TAG_DONE                                           	    	    	    	}
        };

        w->WLayer = CreateLayerTagList(&w->WScreen->LayerInfo,
                        	       w->WScreen->RastPort.BitMap,
                        	       w->RelLeftEdge,
                        	       w->RelTopEdge,
                        	       w->RelLeftEdge + w->Width - 1,
                        	       w->RelTopEdge + w->Height - 1,
                        	       layerflags,
                        	       lay_tags);

    #endif

        /* Install the BorderRPort here! see GZZ window above  */
        if (NULL != w->WLayer)
        {
            /*
               I am installing a totally new RastPort here so window and frame can
               have different fonts etc.
            */
            w->BorderRPort = AllocMem(sizeof(struct RastPort), MEMF_ANY);

            if (w->BorderRPort)
            {
                InitRastPort(w->BorderRPort);
                w->BorderRPort->Layer  = w->WLayer;
                w->BorderRPort->BitMap = w->WLayer->rp->BitMap;
            }
            else
            {
                /* no memory for RastPort! Simply close the window */
                intui_CloseWindow(w, IntuitionBase);
                msg->success = FALSE;
//                UnlockLayers(&w->WScreen->LayerInfo);
                ReturnVoid("intui_OpenWindow(No BorderRPort)");
            }
        }
    }

    D(bug("Layer created: %p\n", w->WLayer));
    D(bug("Window created: %p\n", w));

    /* common code for GZZ and regular windows */

    if (w->WLayer)
    {

        if ((layerflags & LAYERBACKDROP) && (w->WScreen->Flags & SHOWTITLE))
        {
#ifdef __MORPHOS__
            struct Layer *blayer;

            /* make sure the screen titlebar is in front of all user created */
            /* backdrop windows */

            blayer = w->WScreen->BarLayer;

#ifdef SKINS
            if (GetPrivScreen(w->WScreen)->SpecialFlags & (SF_InvisibleBar|SF_AppearingBar)) blayer = 0;
#endif

            if (blayer) MoveLayerInFrontOf(blayer,w->WLayer);

            D(bug("move screen bar layer in front of window backdrop layer\n"));
#else
            /* backdrop window was created over screen barlayer, but it must be
            under the screen barlayer if screen has flag SHOWTITLE set */

            AROS_ATOMIC_AND(w->WScreen->Flags, ~SHOWTITLE);

            ShowTitle(w->WScreen, TRUE);
#endif
        }

        /* Layer gets pointer to the window */
        WLAYER(w) = w->WLayer;

        CheckLayers(w->WScreen,IntuitionBase);

//        UnlockLayers(&w->WScreen->LayerInfo);

#ifndef __MORPHOS__
        w->WLayer->Window = (APTR)w;
#endif
        /* Window needs a rastport */
        w->RPort = w->WLayer->rp;

        /* installation of the correct BorderRPort already happened above !! */

        if (CreateWinSysGadgets(w, IntuitionBase))
        {
            LONG lock;
	    
	    lock = LockIBase (0);

            /* insert new window into parent/descendant list
            **
            ** before: parent win xyz
            **            |
            **            |
            **            |
            **        descendant win abc
            **
            ** after:  parent win xyz
            **              \
            **       \
            **         newwindow w
            **      /
            **         /
            **        /
            **   descendant win abc
            */

#if 1
	    {
            struct Window *parent, *descendant_of_parent;

            parent = IntuitionBase->ActiveWindow;
            if (!parent) parent = w->WScreen->FirstWindow;
            if (parent)
            {
                descendant_of_parent = parent->Descendant;
                parent->Descendant = w;
                if (descendant_of_parent) descendant_of_parent->Parent = w;
            }
            else
            {
                descendant_of_parent = NULL;
            }

            w->Descendant = descendant_of_parent;
            w->Parent = parent;
	    }
#endif

            w->NextWindow = w->WScreen->FirstWindow;
            w->WScreen->FirstWindow = w;

            w->WindowPort = GetPrivIBase(IntuitionBase)->IntuiReplyPort;

            UnlockIBase (lock);

            AddResourceToList(w, RESOURCE_WINDOW, IntuitionBase);

            if (w->Flags & WFLG_ACTIVATE)
            {
                ActivateWindow(w);
            }
            else
            {
                RefreshWindowFrame(w);
            }

            msg->success = TRUE;
            ReturnVoid("int_openwindow");
        }

        CloseWindow(w);
        //int_closewindow(w, IntuitionBase);

    } /* if (layer created) */

//    UnlockLayers(&w->WScreen->LayerInfo);


    D(bug("int_openwindow(General failure)"));

    msg->success = FALSE;
    ReturnVoid("int_openwindow");
}
