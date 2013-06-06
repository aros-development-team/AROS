/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$

    Open a new screen.
*/

#include <aros/config.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <intuition/screens.h>
#include <intuition/intuition.h>
#include <intuition/imageclass.h>
#include <intuition/windecorclass.h>
#include <intuition/scrdecorclass.h>
#include <intuition/gadgetclass.h>
#include <intuition/extensions.h>
#include <graphics/modeid.h>
#include <graphics/videocontrol.h>
#include <graphics/displayinfo.h>
#include <prefs/screenmode.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <string.h>
#include <proto/cybergraphics.h>
#include <cybergraphx/cybergraphics.h>
#include <hidd/graphics.h>
#include "intuition_intern.h"
#include "intuition_customize.h"
#include "intuition_extend.h"
#include "inputhandler.h"
#include "inputhandler_support.h"
#include "inputhandler_actions.h"
#include "menus.h"
#include "monitorclass_private.h"

#ifndef DEBUG_OpenScreen
#define DEBUG_OpenScreen 0
#endif
#undef DEBUG
#if DEBUG_OpenScreen
#define DEBUG 1
#endif
#include <aros/debug.h>

struct OpenScreenActionMsg
{
    struct IntuiActionMsg    msg;
    struct IntScreen 	    *Screen;
    struct NewScreen 	    *NewScreen;
    struct List     	    *List;
};

static VOID int_openscreen(struct OpenScreenActionMsg *msg,
                           struct IntuitionBase *IntuitionBase);

#ifdef SKINS
extern const ULONG defaultdricolors[DRIPEN_NUMDRIPENS];
#endif

/*****************************************************************************

    NAME */

    AROS_LH1(struct Screen *, OpenScreen,

/*  SYNOPSIS */
         AROS_LHA(struct NewScreen *, newScreen, A0),

/*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 33, Intuition)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
	The function relies on private data being passed in DimensionInfo.reserved[0]
	by graphics.library/GetDisplayInfoData(). Keep this in sync when modifying
	the code.

    HISTORY
    29-10-95    digulla automatically created from
                intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct GfxBase          *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct LayersBase       *LayersBase = GetPrivIBase(IntuitionBase)->LayersBase;
    struct Library          *UtilityBase = GetPrivIBase(IntuitionBase)->UtilityBase;
    struct NewScreen 	     ns;
    struct TagItem   	    *tag, *tagList;
    IPTR		     vctl = 0;
    struct IntScreen 	    *screen;
    int              	     success;
    struct Hook      	    *layer_info_hook = NULL;
    struct ColorSpec 	    *colors = NULL;
    ULONG            	    *errorPtr;   /* Store error at user specified location */
    UWORD           	    *customdripens = NULL;
    ULONG           	    *colors32 = NULL;
    WORD            	     sysfont = -1;
    BOOL            	     ok = TRUE, rp_inited = FALSE, li_inited = FALSE, sharepens = FALSE;
#ifdef USEWINDOWLOCK
    BOOL            	     windowlock = FALSE;
#endif
    struct Rectangle        *dclip = NULL;
    LONG                     overscan = OSCAN_TEXT;
    DisplayInfoHandle        displayinfo;
    struct DimensionInfo     dimensions;
#ifdef __MORPHOS__
    struct MonitorInfo       monitor;
#endif
    ULONG                    allocbitmapflags = BMF_DISPLAYABLE;
    //ULONG                  lock;
    WORD                     numcolors = 0;
    UWORD		     spritebase;
#ifdef SKINS
    BOOL                     workbench = FALSE;
#endif
    BOOL		     draggable = TRUE;
    struct TagItem   	     modetags[] =
    {
        { BIDTAG_Depth        	, 0UL   },
        { BIDTAG_DesiredWidth   , 0UL   },
        { BIDTAG_DesiredHeight  , 0UL   },
        { BIDTAG_DIPFMustHave   , 0UL   },
        { TAG_DONE          	    	}
    };
    ULONG   	    	     modeid = INVALID_ID;
    struct DisplayInfo       dispinfo;

    /* Intuition not up yet? */
    if (!GetPrivIBase(IntuitionBase)->DefaultPointer)
    	return FALSE;

    ASSERT_VALID_PTR_ROMOK(newScreen);

#define COPY(x)     screen->Screen.x = ns.x
#define SetError(x) if (errorPtr != NULL) *errorPtr = x;

    D(bug("OpenScreen (%p = { Left=%d Top=%d Width=%d Height=%d Depth=%d Mode=%04x })\n"
          , newScreen
          , newScreen->LeftEdge
          , newScreen->TopEdge
          , newScreen->Width
          , newScreen->Height
          , newScreen->Depth
          , newScreen->ViewModes
         ));

    FireScreenNotifyMessage((IPTR) newScreen, SNOTIFY_BEFORE_OPENSCREEN, IntuitionBase);

    ns = *newScreen;

    if (newScreen->Type & NS_EXTENDED)
    {
        tagList = ((struct ExtNewScreen *)newScreen)->Extension;
    }
    else
    {
        tagList = NULL;
    }

    DEBUG_OPENSCREEN(dprintf("OpenScreen: Left %d Top %d Width %d Height %d Depth %d Tags 0x%lx\n",
                             ns.LeftEdge, ns.TopEdge, ns.Width, ns.Height, ns.Depth, tagList));

#ifdef __MORPHOS__
    if (!LocaleBase) LocaleBase = OpenLibrary("locale.library",0);
    if (!LocaleBase) return NULL;
#endif

#ifdef USEGETIPREFS //needs INTUITION_THEME_ENCHANCEMENT!
    if (!GetPrivIBase(IntuitionBase)->IPrefsLoaded && FindPort(SKINMANAGERPORTNAME))
    {
        /* let's init prefs before 1st OpenScreen that needs them*/
        int_SkinAction(SKA_GetIPrefs,0,0,IntuitionBase);
    }
#endif

    screen = AllocMem(sizeof (struct IntScreen), MEMF_ANY | MEMF_CLEAR);

    DEBUG_OPENSCREEN(dprintf("OpenScreen: screen 0x%lx\n", screen));

    /* Do this really early to be able to report errors */
    errorPtr = (ULONG *)GetTagData(SA_ErrorCode, 0, tagList);

    DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_ErrorCode 0x%lx\n",errorPtr));

    if (screen == NULL)
    {
        FireScreenNotifyMessage((IPTR) NULL, SNOTIFY_AFTER_OPENSCREEN, IntuitionBase);
        SetError(OSERR_NOMEM);
        return NULL;
    }

    if (tagList)
    {
        if (GetTagData(SA_LikeWorkbench, FALSE, tagList))
        {

            DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_LikeWorkbench\n"));

            ns.Width = GetPrivIBase(IntuitionBase)->ScreenModePrefs.smp_Width;
            ns.Height = GetPrivIBase(IntuitionBase)->ScreenModePrefs.smp_Height;
            ns.Depth = GetPrivIBase(IntuitionBase)->ScreenModePrefs.smp_Depth;
            modeid = GetPrivIBase(IntuitionBase)->ScreenModePrefs.smp_DisplayID;

            if (GetPrivIBase(IntuitionBase)->ScreenModePrefs.smp_Control & SMF_AUTOSCROLL)
            {
                /* need to mark autoscroll */
                ns.Type |= AUTOSCROLL;
            }
            sysfont = 1;
            sharepens = TRUE; /* not sure */
        }

        while((tag = NextTagItem(&tagList)))
        {
            DEBUG_OPENSCREEN(dprintf("OpenScreen: Tag 0x%08lx Data 0x%08lx\n",
                                     tag->ti_Tag, tag->ti_Data));
            switch(tag->ti_Tag)
            {
            case SA_Left:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_Left %ld\n",tag->ti_Data));
                ns.LeftEdge  = tag->ti_Data;
                break;

            case SA_Top:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_Top %ld\n",tag->ti_Data));
                ns.TopEdge   = tag->ti_Data;
                break;

            case SA_Width:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_Width %ld\n",tag->ti_Data));
                ns.Width     = tag->ti_Data;
                break;

            case SA_Height:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_Height %ld\n",tag->ti_Data));
                ns.Height    = tag->ti_Data;
                break;

            case SA_Depth:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_Depth %ld\n",tag->ti_Data));
                ns.Depth     = tag->ti_Data;
                break;

            case SA_DetailPen:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_DetailPen %ld\n",tag->ti_Data));
                ns.DetailPen = tag->ti_Data;
                break;

            case SA_BlockPen:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_BlockPen %ld\n",tag->ti_Data));
                ns.BlockPen  = tag->ti_Data;
                break;

            case SA_Type:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_Type 0x%lx\n",tag->ti_Data));
                ns.Type &= ~SCREENTYPE;
                ns.Type |= tag->ti_Data;
                break;

            case SA_Title:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_Title <%s>\n",tag->ti_Data));
                ns.DefaultTitle = (UBYTE *)tag->ti_Data;
                break;

            case SA_Font:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_Font 0x%lx\n",tag->ti_Data));
                ns.Font = (struct TextAttr *)tag->ti_Data;
                break;

            case SA_Colors32:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_Colors32 0x%lx\n",tag->ti_Data));
                colors32 = (ULONG *)tag->ti_Data;
                break;

            case SA_Colors:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_Colors 0x%lx\n",tag->ti_Data));
                colors = (struct ColorSpec *)tag->ti_Data;
                break;

            case SA_SysFont:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_SysFont 0x%lx\n",tag->ti_Data));
                sysfont = (WORD)tag->ti_Data;
                break;

            case SA_BitMap:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_BitMap 0x%lx\n",tag->ti_Data));
                if(tag->ti_Data)
                {
                    ns.Type |= CUSTOMBITMAP;
                    ns.CustomBitMap = (struct BitMap *)tag->ti_Data;
                }
                else
                {
                    DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_BitMap==NULL specified, custom bitmap use disabled\n"));
                }
                break;

                /* Name of this public screen. */
            case SA_PubName:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_PubName <%s>\n",tag->ti_Data));
                if (!tag->ti_Data)
                {
                    screen->pubScrNode = AllocMem(sizeof(struct PubScreenNode), MEMF_CLEAR);
                    break;
                }
                else
                {
                    struct Screen *old;

                    LockPubScreenList();

                    if (strcasecmp((char *)tag->ti_Data, "Workbench") == 0)
                    {
			/* FIXME: This would still not be safe, if a normal app tried to open its own screen with SA_PubName=Workbench */
                        if (GetPrivIBase(IntuitionBase)->WorkBench)
                        {
                            UnlockPubScreenList();

                            FireScreenNotifyMessage((IPTR) NULL, SNOTIFY_AFTER_OPENSCREEN, IntuitionBase);

                            return NULL;
                        }
#ifdef SKINS
                        workbench = TRUE;
#endif
                    }
                    else
                    {
                        old = LockPubScreen((STRPTR)tag->ti_Data);

                        if (old != NULL)
                        {
                            UnlockPubScreen(NULL, old);
                            SetError(OSERR_PUBNOTUNIQUE);
                            UnlockPubScreenList();
                            FireScreenNotifyMessage((IPTR) NULL, SNOTIFY_AFTER_OPENSCREEN, IntuitionBase);

                            return NULL;
                        }
                    }

                    UnlockPubScreenList();
                }

                screen->pubScrNode = AllocMem(sizeof(struct PubScreenNode), MEMF_CLEAR);

                DEBUG_OPENSCREEN(dprintf("OpenScreen: pubScrNode 0x%lx\n",screen->pubScrNode));

                if (screen->pubScrNode == NULL)
                {
                    SetError(OSERR_NOMEM);
                    FireScreenNotifyMessage((IPTR) NULL, SNOTIFY_AFTER_OPENSCREEN, IntuitionBase);

                    return NULL;
                }

                if ((ns.Type & SCREENTYPE) == CUSTOMSCREEN)
                {
                    ns.Type &= ~SCREENTYPE;
                    ns.Type |= PUBLICSCREEN;
                }

                screen->pubScrNode->psn_Node.ln_Name = AllocVec(MAXPUBSCREENNAME + 1,
                                                       MEMF_ANY);

                if (screen->pubScrNode->psn_Node.ln_Name == NULL)
                {
                    SetError(OSERR_NOMEM);
                    FreeMem(screen->pubScrNode, sizeof(struct PubScreenNode));
                    FireScreenNotifyMessage((IPTR) NULL, SNOTIFY_AFTER_OPENSCREEN, IntuitionBase);

                    return NULL;
                }

                /* Always open public screens in private mode. */
                screen->pubScrNode->psn_Flags |= PSNF_PRIVATE;
                strcpy(screen->pubScrNode->psn_Node.ln_Name, (STRPTR)tag->ti_Data);
                break;

                /* Signal bit number to use when signalling public screen
                   signal task. */
            case SA_PubSig:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_PubSig 0x%lx\n",tag->ti_Data));
                if (screen->pubScrNode == NULL)
                {
                    FireScreenNotifyMessage((IPTR) NULL, SNOTIFY_AFTER_OPENSCREEN, IntuitionBase);

                    return NULL;
                }
                /* If no PubTask is set, we set the calling task as default */
                if (screen->pubScrNode->psn_SigTask == NULL)
                    screen->pubScrNode->psn_SigTask = FindTask(NULL);

                screen->pubScrNode->psn_SigBit = (UBYTE)tag->ti_Data;
                break;

                /* Task that should be signalled when the public screen loses
                   its last visitor window. */
            case SA_PubTask:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_PubTask 0x%lx\n",tag->ti_Data));
                if (screen->pubScrNode == NULL)
                {
                    FireScreenNotifyMessage((IPTR) NULL, SNOTIFY_AFTER_OPENSCREEN, IntuitionBase);
                    return NULL;
                }
                screen->pubScrNode->psn_SigTask = (struct Task *)tag->ti_Data;
                break;

            case SA_BackFill:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_BackFill Hook 0x%lx\n",tag->ti_Data));
                layer_info_hook = (struct Hook *)tag->ti_Data;
                break;

            case SA_Quiet:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_Quiet 0x%lx\n",tag->ti_Data));
                if (tag->ti_Data)
                {
                    ns.Type |= SCREENQUIET;
                }
                else
                {
                    ns.Type &= ~SCREENQUIET;
                }
                break;

            case SA_ShowTitle:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_ShowTitle 0x%lx\n",tag->ti_Data));
                if (tag->ti_Data)
                {
                    ns.Type |= SHOWTITLE;
                }
                else
                {
                    ns.Type &= ~SHOWTITLE;
                }
                break;

            case SA_Pens:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_Pens 0x%lx\n",tag->ti_Data));
                customdripens = (UWORD *)tag->ti_Data;
                break;

            case SA_DisplayID:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_DisplayID 0x%lx\n",tag->ti_Data));
                //if (modeid == INVALID_ID)
                modeid = tag->ti_Data;
                break;

            case SA_SharePens:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_SharePens 0x%lx\n",tag->ti_Data));
                sharepens = tag->ti_Data ? TRUE : FALSE;
                if (tag->ti_Data)
                {
                    ns.Type |= PENSHARED;
                }
		else
		{
                    ns.Type &= ~PENSHARED;
                }
                break;

            case SA_Interleaved:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_Interleaved 0x%lx\n",tag->ti_Data));
                if (tag->ti_Data)
                {
                    allocbitmapflags |= BMF_INTERLEAVED;
                }
                else
                {
                    allocbitmapflags &= ~BMF_INTERLEAVED;
                }
                break;

            case SA_Behind:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_Behind 0x%lx\n",tag->ti_Data));
                if (tag->ti_Data)
                {
                    ns.Type |= SCREENBEHIND;
                }
                else
                {
                    ns.Type &= ~SCREENBEHIND;
                }
                break;

            case SA_DClip:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_DClip 0x%lx\n",tag->ti_Data));
                dclip = (struct Rectangle *)tag->ti_Data;
                break;

            case SA_Overscan:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_OverScan 0x%lx\n",tag->ti_Data));
                overscan = tag->ti_Data;
                break;

            case SA_LikeWorkbench:
            case SA_ErrorCode:
                /*
                 * handled elsewhere
                 */
                break;

            case SA_AutoScroll:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_AutoScroll 0x%lx\n",tag->ti_Data));
                ns.Type |= AUTOSCROLL;
                break;

            case SA_FullPalette:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_FullPalette 0x%lx\n",tag->ti_Data));
                break;

            case SA_ColorMapEntries:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_ColorMapEntries 0x%lx\n",tag->ti_Data));
		numcolors = tag->ti_Data;
                break;

            case SA_Parent:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_Parent 0x%lx\n",tag->ti_Data));
                break;

            case SA_Draggable:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_Draggable 0x%lx\n",tag->ti_Data));
		draggable = tag->ti_Data;
                break;

            case SA_Exclusive:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_Exclusive 0x%lx\n",tag->ti_Data));
                break;

            case SA_VideoControl:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_VideoControl 0x%p\n",tag->ti_Data));
		vctl = tag->ti_Data;
                break;

            case SA_FrontChild:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_FrontChild 0x%lx\n",tag->ti_Data));
                break;

            case SA_BackChild:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_BackChild 0x%lx\n",tag->ti_Data));
                break;

            case SA_MinimizeISG:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_MinimizeISG 0x%lx\n",tag->ti_Data));
                break;

	    /* TODO: Missing SA_ Tags */
            default:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: unknown tag 0x%lx data 0x%lx\n",
                                         tag->ti_Tag,
                                         tag->ti_Data));
                break;

            } /* switch (tag->ti_Tag) */

        } /* while ((tag = NextTagItem (&tagList))) */

    } /* if (tagList) */

    DEBUG_OPENSCREEN(dprintf("OpenScreen: Requested: Left %d Top %d Width %d Height %d Depth %d Tags 0x%lx\n",
                             ns.LeftEdge, ns.TopEdge, ns.Width, ns.Height, ns.Depth, tagList));

    /* First Init the RastPort then get the BitPlanes!! */

    modetags[0].ti_Data = ns.Depth;
    if (ns.Width != STDSCREENWIDTH)
    {
    	modetags[1].ti_Data = ns.Width;
    }
    else
    {
    	modetags[1].ti_Tag = TAG_IGNORE;
    }

    if (ns.Height != STDSCREENHEIGHT)
    {
    	modetags[2].ti_Data = ns.Height;
    }
    else
    {
    	modetags[2].ti_Tag = TAG_IGNORE;
    }

    /* if default HIRES_KEY or HIRESLACE_KEY is passed, make sure we find a replacement mode. */
    if (INVALID_ID != modeid)
    {
        if (FindDisplayInfo(modeid) == NULL)
        {
            switch(modeid)
            {
            case HIRES_KEY:
            case HIRESLACE_KEY:
                {
                    modetags[1].ti_Data = ns.Width;
                    modetags[2].ti_Data = ns.Height;

                    DEBUG_OPENSCREEN(dprintf("resetting native mode id !!\n");)
                    DEBUG_OPENSCREEN(dprintf("ns.Width %ld ns.Height %ld ns.Depth %ld !!\n",
                            (LONG) ns.Width, (LONG) ns.Height, (LONG) ns.Depth);)

		    modeid = INVALID_ID;
                }
                break;

            default:
                break;
            }
#ifdef __mc68000
            /* Got OpenScreenTags modeid without monitor, select system default.
             * We really need proper mode promotion support.
             */
            if ((modeid & MONITOR_ID_MASK) == 0) {
                modeid |= (GfxBase->DisplayFlags & PAL) ? PAL_MONITOR_ID : NTSC_MONITOR_ID;
            }
#endif
        }
    }

    if (INVALID_ID == modeid)
    {
        /* Old-style HAM or EHB request? */
        if (newScreen->ViewModes & (HAM | EXTRA_HALFBRITE)) {
            if (newScreen->ViewModes & HAM)
                modetags[3].ti_Data |= DIPF_IS_HAM;
            if (newScreen->ViewModes & EXTRA_HALFBRITE)
                modetags[3].ti_Data |= DIPF_IS_EXTRAHALFBRITE;
        } else {
            modetags[3].ti_Tag = TAG_IGNORE;
        }
        modeid = BestModeIDA(modetags);
        if (INVALID_ID == modeid)
        {
            DEBUG_OPENSCREEN(dprintf("!!! OpenScreen: Could not find valid modeid !!!\n");)

	    SetError(OSERR_UNKNOWNMODE);
            FireScreenNotifyMessage((IPTR) NULL, SNOTIFY_AFTER_OPENSCREEN, IntuitionBase);

            return NULL;
        }
    }

    DEBUG_OPENSCREEN(dprintf("OpenScreen: Corrected ModeID: 0x%08lx\n", modeid));

    InitRastPort(&screen->Screen.RastPort);
    rp_inited = TRUE;
    success = FALSE;

    if ((displayinfo = FindDisplayInfo(modeid)) != NULL &&
        GetDisplayInfoData(displayinfo, (APTR)&dispinfo, sizeof(dispinfo), DTAG_DISP, modeid)) {
        screen->DInfo.dri_Resolution.X = dispinfo.Resolution.x;
        screen->DInfo.dri_Resolution.Y = dispinfo.Resolution.y;
    } else {
        /* Fake a resolution */
        screen->DInfo.dri_Resolution.X = 22;
        screen->DInfo.dri_Resolution.Y = 22;
    }
 
    if ((displayinfo = FindDisplayInfo(modeid)) != NULL &&
        GetDisplayInfoData(displayinfo, (APTR)&dimensions, sizeof(dimensions), DTAG_DIMS, modeid) &&
#ifdef __MORPHOS__
        GetDisplayInfoData(displayinfo, &monitor, sizeof(monitor), DTAG_MNTR, modeid)
#else
	((screen->IMonitorNode = FindMonitorNode(modeid, IntuitionBase)) != NULL)
#endif
    )
    {
#ifdef __MORPHOS__
        screen->Monitor = monitor.Mspc;
#else
	/* graphics.library supplies HIDD composition flags here, specially for us */
	screen->SpecialFlags = dimensions.reserved[0] << 8;
#endif
        success = TRUE;

        if (dclip == NULL)
        {
            switch (overscan)
            {
            case OSCAN_STANDARD:
                dclip = &dimensions.StdOScan;
                break;

            case OSCAN_MAX:
                dclip = &dimensions.MaxOScan;
                break;

            case OSCAN_VIDEO:
                dclip = &dimensions.VideoOScan;
                break;

            default:
                dclip = &dimensions.TxtOScan;
                break;
            }
        }

        if (ns.Width == STDSCREENWIDTH)
            ns.Width = dclip->MaxX - dclip->MinX + 1;
	else if (ns.Width < dimensions.MinRasterWidth)
	    ns.Width = dimensions.MinRasterWidth;
	else if (ns.Width > dimensions.MaxRasterWidth)
	    ns.Width = dimensions.MaxRasterWidth;

        if (ns.Height == STDSCREENHEIGHT)
            ns.Height = dclip->MaxY - dclip->MinY + 1;
	else if (ns.Height < dimensions.MinRasterHeight)
	    ns.Height = dimensions.MinRasterHeight;
	else if (ns.Height > dimensions.MaxRasterHeight)
	    ns.Height = dimensions.MaxRasterHeight;

        DEBUG_OPENSCREEN(dprintf("OpenScreen: Corrected: Width %ld Height %ld\n", ns.Width, ns.Height));

        screen->Screen.RastPort.BitMap = NULL;
        if (ns.Type & CUSTOMBITMAP)
        {
            struct BitMap *custombm;

            custombm = ns.CustomBitMap;
#ifdef __MORPHOS__
            if (IsCyberModeID(modeid) && custombm)
            {
                int pixfmt = GetCyberIDAttr(CYBRIDATTR_PIXFMT,modeid);

                if(GetCyberMapAttr(custombm,CYBRMATTR_PIXFMT) != pixfmt)
                {
                        // incompatible formats !
                        custombm = NULL;
                }
            }
#else
            if (!IS_HIDD_BM(custombm) || (modeid != HIDD_BM_HIDDMODE(custombm)))
                custombm = NULL;
#endif

            if(custombm != NULL)
            {
                screen->Screen.RastPort.BitMap = custombm;
                ns.Depth    =   GetBitMapAttr(ns.CustomBitMap,BMA_DEPTH);
                DEBUG_OPENSCREEN(dprintf("OpenScreen: CustomBitmap Depth %ld\n",
                                         ns.Depth));
            }
            else
            {
                ns.CustomBitMap = NULL;
              	ns.Type &= ~CUSTOMBITMAP;
            }
        }

        if(screen->Screen.RastPort.BitMap == NULL)
        {
	    ULONG Depth = (dimensions.MaxDepth > 8) ? dimensions.MaxDepth : ns.Depth;
#ifdef __MORPHOS__
            ULONG pixfmt;

            switch(Depth)
            {
            case 15:
                pixfmt = PIXFMT_RGB15;
                break;
            case 16:
                pixfmt = PIXFMT_RGB16;
                break;
            case 24:
                pixfmt = PIXFMT_BGR24;
                break;
            case 32:
                pixfmt = PIXFMT_ARGB32;
                break;
            default:
                pixfmt = PIXFMT_LUT8;
                break;
            }

            if (IsCyberModeID(modeid))
            {
                pixfmt = GetCyberIDAttr(CYBRIDATTR_PIXFMT,modeid);
            }


            allocbitmapflags |= (BMF_SPECIALFMT|BMF_CLEAR);

            screen->Screen.RastPort.BitMap = AllocBitMap(ns.Width,
                                             ns.Height,
                                             Depth,
                                             allocbitmapflags | (pixfmt << 24),
                                             NULL);
#else
	    struct TagItem bmtags[] = {
		{BMATags_DisplayID, modeid},
		{TAG_DONE         , 0     }
	    };

	    screen->Screen.RastPort.BitMap = AllocBitMap(ns.Width,
                                             ns.Height,
                                             Depth,
                                             allocbitmapflags | BMF_CHECKVALUE,
                                             (struct BitMap *)bmtags);
#endif
            screen->AllocatedBitmap = screen->Screen.RastPort.BitMap;
        }

        DEBUG_OPENSCREEN(dprintf("OpenScreen: BitMap 0x%lx\n",
                                 screen->Screen.RastPort.BitMap));
    }
    else
    {
        DEBUG_OPENSCREEN(dprintf("OpenScreen: no displayinfo\n"));
	SetError(OSERR_UNKNOWNMODE);
    }

    /* Init screen's viewport */
    InitVPort(&screen->Screen.ViewPort);

    /* Allocate a RasInfo struct in which we have  a pointer
       to the struct BitMap, into which the driver can
       store its stuff. (Eg. pointer to a BitMap HIDD object)
    */
    screen->Screen.ViewPort.RasInfo = AllocMem(sizeof(struct RasInfo), MEMF_ANY | MEMF_CLEAR);

    DEBUG_OPENSCREEN(dprintf("OpenScreen: RasInfo 0x%lx\n",
                             screen->Screen.ViewPort.RasInfo));

    if (!success ||
        (screen->Screen.RastPort.BitMap == NULL) ||
        (screen->Screen.ViewPort.RasInfo == NULL))
    {
        ok = FALSE;
    }
    else
    {
        D(bug("got allocated stuff\n"));
	
	/*
	 * Store pointer to bitmap, so we can get hold of it
         * from within LoadRGBxx() functions
         */
        screen->Screen.ViewPort.RasInfo->BitMap = screen->Screen.RastPort.BitMap;
	/* Initialize legacy embedded BitMap structure */
	CopyMem(screen->Screen.RastPort.BitMap, &screen->Screen.BitMap, sizeof(struct BitMap));
    }

    if (ok)
    {
        /* Read depth from the bitmap to avoid AttachPalExtra/ObtainPen getting
         * confused if cgx decided to allocate a higher depth bitmap than what
         * was asked.
         */
        ns.Depth = GetBitMapAttr(screen->Screen.RastPort.BitMap,BMA_DEPTH);
	DEBUG_OPENSCREEN(bug("OpenScreen: Real bitmap depth is %u\n", ns.Depth));

	if (!numcolors) {
	    if (ns.Depth < 5)
	        numcolors = 32;
	    else if (ns.Depth > 8)
	        numcolors = 256;
	    else
                numcolors = 1L << ns.Depth;
	}

        /* Get a color map structure. Sufficient colors?? */

        DEBUG_OPENSCREEN(dprintf("OpenScreen: Colormap Entries %ld\n",
                                 numcolors));

        if ((screen->Screen.ViewPort.ColorMap = GetColorMap(numcolors)) != NULL)
        {
            if (0 == AttachPalExtra(screen->Screen.ViewPort.ColorMap,
                                    &screen->Screen.ViewPort))
            {
#if 0
                int 	i = 0;
                char    *alloclist;
                UWORD   *refcnt;

                refcnt  =(UWORD*) screen->Screen.ViewPort.ColorMap->PalExtra->pe_RefCnt;
                alloclist = (UBYTE *)(refcnt + screen->Screen.ViewPort.ColorMap->Count);

                DEBUG_OPENSCREEN(dprintf("OpenScreen: PalExtra alloclist 0x%lx Count %ld\n",alloclist,screen->Screen.ViewPort.ColorMap->Count));

                while(i < screen->Screen.ViewPort.ColorMap->Count)
                {
                    // initialize alloc list to -1,0,1,2,3,4

                    DEBUG_OPENSCREEN(dprintf("OpenScreen: alloclist[%ld]=%ld\n",
                                             i,alloclist[i]));
                    i++;
                }
#endif
            }
            else
            {
                ok = FALSE;
            }
        }
        else
        {
            ok = FALSE;
        }

        DEBUG_OPENSCREEN(dprintf("OpenScreen: ColorMap 0x%lx\n",
                                 screen->Screen.ViewPort.ColorMap));
    }

#ifdef __MORPHOS__
    screen->ModeID = modeid;
#endif

    if (ok)
    {
        struct ViewPortExtra *vpe = (struct ViewPortExtra *)GfxNew(VIEWPORT_EXTRA_TYPE);

        DEBUG_OPENSCREEN(dprintf("OpenScreen: ViewPortExtra 0x%lx\n", vpe));

        ok = FALSE;

        if (vpe)
        {
            struct TagItem tags[6];

            memcpy(&vpe->DisplayClip, dclip,sizeof(struct Rectangle));

	    screen->Screen.ViewPort.DxOffset = ns.LeftEdge;
	    screen->Screen.ViewPort.DyOffset = ns.TopEdge;
            screen->Screen.ViewPort.DWidth = dclip->MaxX - dclip->MinX + 1;//ns.Width; /* or from dclip ? */
            screen->Screen.ViewPort.DHeight = dclip->MaxY - dclip->MinY + 1;//ns.Height;

            tags[0].ti_Tag  = VTAG_ATTACH_CM_SET;
            tags[0].ti_Data = (IPTR)&screen->Screen.ViewPort;
            tags[1].ti_Tag  = VTAG_VIEWPORTEXTRA_SET;
            tags[1].ti_Data = (IPTR)vpe;
            tags[2].ti_Tag  = VTAG_NORMAL_DISP_SET;
            tags[2].ti_Data = (IPTR)displayinfo;
            tags[3].ti_Tag  = VTAG_VPMODEID_SET;
            tags[3].ti_Data = modeid;
	    tags[4].ti_Tag  = VTAG_SPEVEN_BASE_SET;
	    tags[5].ti_Tag  = VTAG_NEXTBUF_CM; /* if vctl is 0, this will terminate the list */
	    tags[5].ti_Data = vctl;

	    /* Originally we could always use palette entries 16-19 for
	       sprites, even if the screen has less than 32 colors. AROS may
	       run on hardware that does not allow this (e.g. VGA cards).
	       In this case we have to shift down sprite colors. Currently
	       we use 4 colors before last 4 colors. For example on VGA cards
	       with only 16 colors we use colors 9 - 12. Remember that last 4 colors
	       of the screen are always used by Intuition.
	       Remember that the first color of the sprite is always transparent. So actually
	       we use 3, not 4 colors.
	       Yes, sprites may look not as expected on screens with low color depth, but at
	       least they will be seen. It's better than nothing.
	       
	       Note that because our base color number doesn't always divide by 16, we use MSB to store
	       the remainder (offset in the color bank). Yes, it's a bit hacky, but i have no better idea
	       at the moment.

	       FIXME: this mapping scheme assumes that we always have at least 16 colors.
               For current display modes supported by AROS it's always true, but in future
	       we may support more display modes (for example monochrome ones), and we
	       should take into account that on screens with less than 11 colors this simply
	       won't work

	       TODO: I think we should have SpriteBase attribute for the bitmap which
	       defaults to acceptable value. We should just get its default value here.
	       The same attribute would be set by VideoControl() and MakeVPort() in order
	       to actually apply the value. */
#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
	    spritebase = 16;
#else
	    spritebase = (ns.Depth < 5) ? (1 << ns.Depth) - 8 : 16;
#endif
	    DEBUG_OPENSCREEN(bug("OpenScreen: spritebase is %u\n", spritebase));
	    tags[4].ti_Data = ((spritebase & 0x0F) << 8 ) | (spritebase >> 4);

            if (VideoControl(screen->Screen.ViewPort.ColorMap, tags) == 0)
            {
                DEBUG_OPENSCREEN(dprintf("OpenScreen: VideoControl ok\n"));
                ok = TRUE;
            }
            else
            {
                DEBUG_OPENSCREEN(dprintf("OpenScreen: VideoControl failed\n"));
            }
        }
    }

    if (ok)
    {
        struct Color32 *p;
        int 	    	k;

        DEBUG_OPENSCREEN(dprintf("OpenScreen: Set first 4 colors\n"));
        p = GetPrivIBase(IntuitionBase)->Colors;
        for (k = 0; k < 4; ++k)
        {
            DEBUG_OPENSCREEN(dprintf("OpenScreen: Viewport 0x%p Index %ld R 0x%lx G 0x%lx B 0x%lx\n",
                                     &screen->Screen.ViewPort,
                                     k, p[k].red, p[k].green, p[k].blue));
            SetRGB32(&screen->Screen.ViewPort, k, p[k].red, p[k].green, p[k].blue);
        }

        if (ns.Depth >= 3)
        {
	    ULONG lastcol = ((ns.Depth > 8) ? 256 : (1 << ns.Depth)) - 4;

            DEBUG_OPENSCREEN(dprintf("OpenScreen: Set last 4 colors (starting from %u)\n", lastcol));
            for (k = 0; k < 4; ++k)
            {
                DEBUG_OPENSCREEN(dprintf("OpenScreen: Viewport 0x%p Index %u R 0x%08X G 0x%08X B 0x%08X\n",
                                         &screen->Screen.ViewPort,
                                         k + lastcol, p[k+4].red, p[k+4].green, p[k+4].blue));

		if (k + lastcol < numcolors)
                    ObtainPen(screen->Screen.ViewPort.ColorMap, k + lastcol,
			      p[k+4].red, p[k+4].green, p[k+4].blue, 0);
		else
		    /* Can't be allocated, but can still be set. */
		    SetRGB32(&screen->Screen.ViewPort, k + lastcol,
			     p[k+4].red, p[k+4].green, p[k+4].blue);
            }
        }
	/* Allocate pens for mouse pointer sprite only on LUT screens. On hi- and
	   truecolor screens sprite colors come from colormap attached to the sprite
	   bitmap itself.
	   See pointerclass::New() for details. */
#ifndef ALWAYS_ALLOCATE_SPRITE_COLORS
	if (ns.Depth < 9)
#endif
	{
	    DEBUG_OPENSCREEN(dprintf("OpenScreen: Obtain Mousepointer colors\n"));
            /* Allocate pens for the mouse pointer */
            for (k = 1; k < 4; ++k)
            {
            	LONG opret = -1;
    	        DEBUG_OPENSCREEN(dprintf("OpenScreen: ViewPort 0x%p Index %d R 0x%08X G 0x%08X B 0x%08X\n",
                                         &screen->Screen.ViewPort,
                                         k + spritebase,
					 p[k+7].red,
					 p[k+7].green,
					 p[k+7].blue));
		if (k + spritebase < numcolors)
                    opret = ObtainPen(screen->Screen.ViewPort.ColorMap, k + spritebase,
			      p[k+7].red, p[k+7].green, p[k+7].blue, 0);
		if (opret < 0) /* ObtainPen() fails if spritebase >= number of screen colors */
		    SetRGB32(&screen->Screen.ViewPort, k + spritebase,
			     p[k+7].red, p[k+7].green, p[k+7].blue);
	    }
        }

        if (colors)  /* if SA_Colors tag exists */
        {
            DEBUG_OPENSCREEN(dprintf("OpenScreen: set SA_Colors 0x%lx\n",colors));
            for(; colors->ColorIndex != (WORD)~0; colors++)
            {
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SetRGB4 Viewport 0x%p Index %d R 0x%x G 0x%x B 0x%x\n",
                                         &screen->Screen.ViewPort,
                                         colors->ColorIndex,
                                         colors->Red,
                                         colors->Green,
                                         colors->Blue));
                SetRGB4(&screen->Screen.ViewPort,
                        colors->ColorIndex,
                        colors->Red,
                        colors->Green,
                        colors->Blue);
            }
        }

        if (colors32)  /* if SA_Colors32 tag exists */
        {
            DEBUG_OPENSCREEN(dprintf("OpenScreen: LoadRGB32 colors32 0x%lx\n",colors32));
            LoadRGB32(&screen->Screen.ViewPort, (const ULONG *)colors32);
        }

        D(bug("Loaded colors\n"));

	MakeVPort(&IntuitionBase->ViewLord, &screen->Screen.ViewPort);
	/* Put validated values into screen structure, this is important */
	screen->Screen.LeftEdge = screen->Screen.ViewPort.DxOffset;
        screen->Screen.TopEdge = screen->Screen.ViewPort.DyOffset;
        COPY(Width);
        COPY(Height);
        COPY(DetailPen);
        COPY(BlockPen);
        COPY(Font);
        COPY(DefaultTitle);

        //intui68k filters this
        screen->Screen.Flags = (ns.Type & ~NS_EXTENDED);

        /* Temporary hack, only used to set size of system gadgets */
        if (ns.Width >= 500 || ns.Height >= 300)
            screen->Screen.Flags |= SCREENHIRES;

        /*
           Copy the data from the rastport's bitmap
           to the screen's bitmap structure
        */
        screen->Screen.BitMap = *screen->Screen.RastPort.BitMap;

#ifdef __MORPHOS__
        screen->Screen.WBorTop    = 2;
        screen->Screen.WBorLeft   = 4;
        screen->Screen.WBorRight  = 4;
        screen->Screen.WBorBottom = 2;
#else
        screen->Screen.WBorTop    = 3;  /* Amiga default is 2 */
        screen->Screen.WBorLeft   = 4;
        screen->Screen.WBorRight  = 4;
        screen->Screen.WBorBottom = 2;  /* Amiga default is 2 */
#endif

        screen->Screen.Title = ns.DefaultTitle;

        DEBUG_OPENSCREEN(dprintf("OpenScreen: init layers\n"));
        InitLayers(&screen->Screen.LayerInfo);
        li_inited = TRUE;

        if (NULL != layer_info_hook)
        {
            DEBUG_OPENSCREEN(dprintf("OpenScreen: instal layerinfohook\n"));
            InstallLayerInfoHook(&screen->Screen.LayerInfo, layer_info_hook);
        }
        D(bug("layers intited screen\n"));

	/* Use higher than DRI_VERSION to detect manually created
	 * DrawInfo structures (for example WB3.1:Prefs/Palette)
	 */
        screen->DInfo.dri_Version = DRI_VERSION_AROS;
        screen->DInfo.dri_NumPens = NUMDRIPENS;
        screen->DInfo.dri_Pens = screen->Pens;
        /* dri_Depth is 8 on hi/true color screens like in AmigaOS with picasso96/cybergraphx */
        screen->DInfo.dri_Depth = (ns.Depth <= 8) ? ns.Depth : 8;

        screen->DInfo.dri_Screen = &screen->Screen;

        /* SA_SysFont overrides SA_Font! */

        DEBUG_OPENSCREEN(dprintf("OpenScreen: SysFont = %d, ns.Font = %p\n", sysfont, ns.Font));

        if (sysfont == 0)
        {
            /* Is handled below */
            DEBUG_OPENSCREEN(dprintf("OpenScreen: skip SysFont for now\n"));
        }
        else if (sysfont == 1)
        {
#if 1
            /* Use safe OpenFont here - Piru
             */
            screen->DInfo.dri_Font = SafeReopenFont(IntuitionBase, &GetPrivIBase(IntuitionBase)->ScreenFont);
#else
#warning: Really hacky way of re-opening ScreenFont

            Forbid();
            screen->DInfo.dri_Font = GetPrivIBase(IntuitionBase)->ScreenFont;
            screen->DInfo.dri_Font->tf_Accessors++;
            Permit();
#endif

	    screen->SpecialFlags |= SF_SysFont;

            DEBUG_OPENSCREEN(dprintf("OpenScreen: Set ScreenFont\n"));

        }
        else if (ns.Font)
        {
            screen->DInfo.dri_Font = OpenFont(ns.Font);
            DEBUG_OPENSCREEN(dprintf("OpenScreen: custom font 0x%lx\n",screen->DInfo.dri_Font));
        }

        if (!screen->DInfo.dri_Font)
        {
            /* GfxBase->DefaultFont is *not* always topaz 8. It
               can be set with the Font prefs program!! */

    	#if 1
            /* Use safe OpenFont.. - Piru
             */
            screen->DInfo.dri_Font = SafeReopenFont(IntuitionBase, &GfxBase->DefaultFont);
    	#else

    	    #warning: Really hacky way of re-opening system default font

            Forbid();
            screen->DInfo.dri_Font = GfxBase->DefaultFont;
            screen->DInfo.dri_Font->tf_Accessors++;
            Permit();
    	#endif
        }

        if (!screen->DInfo.dri_Font) ok = FALSE;

    } /* if (ok) */

    if (ok)
    {
        /* set default values for pens */
        DEBUG_OPENSCREEN(dprintf("OpenScreen: Set Default Pens\n"));

        CopyMem(ns.Depth == 1 ? GetPrivIBase(IntuitionBase)->DriPens2
                : ns.Depth == 4 ? GetPrivIBase(IntuitionBase)->DriPens4
                : GetPrivIBase(IntuitionBase)->DriPens8,
                screen->Pens,
                sizeof(screen->Pens));

        if (ns.Depth > 1)
        {
            DEBUG_OPENSCREEN(dprintf("OpenScreen: Set NewLook\n"));
            screen->DInfo.dri_Flags |= DRIF_NEWLOOK;
        }


        if (customdripens)
        {
            WORD i;

            DEBUG_OPENSCREEN(dprintf("OpenScreen: Set Custom Pens\n"));

            screen->Pens[DETAILPEN] = screen->Screen.DetailPen;
            screen->Pens[BLOCKPEN] = screen->Screen.BlockPen;

            for(i = 0; (i < NUMDRIPENS) && (customdripens[i] != (UWORD)~0); i++)
            {
                DEBUG_OPENSCREEN(dprintf("OpenScreen: Pen[%ld] %ld\n",i,screen->Pens[i]));
                screen->Pens[i] = customdripens[i];
            }
        }
        else
        {
            /*
             * Let`s do some broken software validation of the pens
             * so we may not run into a black desktop.
             */

            DEBUG_OPENSCREEN(dprintf("OpenScreen: Check Default Pens if the make sense\n"));
            if (screen->Screen.DetailPen == screen->Screen.BlockPen)
            {
                DEBUG_OPENSCREEN(dprintf("OpenScreen: DetailPen==BlockPen..correct\n"));
                screen->Screen.DetailPen = 0;
                screen->Screen.BlockPen = 1;
            }
            else
            if (screen->Screen.BlockPen == 0)
            {
                DEBUG_OPENSCREEN(dprintf("OpenScreen: BlockPen==0..correct\n"));
                screen->Screen.BlockPen = screen->Screen.DetailPen;
                screen->Screen.DetailPen = 0;
            }

            screen->Pens[DETAILPEN] = screen->Screen.DetailPen;
            screen->Pens[BLOCKPEN] = screen->Screen.BlockPen;
        }

        /* Allocate shared/exclusive colors */

        {
            BYTE color_alloced[256];

            WORD i;

            for(i = 0; i < 256; i++)
            {
                color_alloced[i] = FALSE;
            }

            /* Mouse pointer colors */

            color_alloced[17] = TRUE;
            color_alloced[18] = TRUE;
            color_alloced[19] = TRUE;

            /* The Pens in the DrawInfo must be allocated as shared */

            DEBUG_OPENSCREEN(dprintf("OpenScreen: ObtainPen DrawInfo Pens as shared\n"));

            for(i = 0; i < NUMDRIPENS; i++)
            {
                int pen = screen->Pens[i];

                if (!color_alloced[pen])
                {
                    DEBUG_OPENSCREEN(dprintf("OpenScreen: ObtainPen ColorMap 0x%lx Pen %ld\n",
                                             screen->Screen.ViewPort.ColorMap,
                                             pen));

                    ObtainPen(screen->Screen.ViewPort.ColorMap,
                              pen,
                              0,
                              0,
                              0,
                              PENF_NO_SETCOLOR);
                    color_alloced[pen] = TRUE;
                }
            }
            DEBUG_OPENSCREEN(dprintf("OpenScreen: done\n"));

            /* If SA_SharePens is FALSE then allocate the rest of the colors
               in the colormap as exclusive */

            if (!sharepens)
            {
                DEBUG_OPENSCREEN(dprintf("OpenScreen: ObtainPen the remaining Pens as exclusive\n"));

                for(i = 0; i < numcolors; i++)
                {
                    if (!color_alloced[i])
                    {
                        DEBUG_OPENSCREEN(dprintf("OpenScreen: ObtainPen ColorMap 0x%lx Pen %ld\n",
                                                 screen->Screen.ViewPort.ColorMap,
                                                 i));
                        ObtainPen(screen->Screen.ViewPort.ColorMap,
                                  i,
                                  0,
                                  0,
                                  0,
                                  PENF_EXCLUSIVE | PENF_NO_SETCOLOR);
                    }
                }

            } /* if (!sharepens) */

        }
    }

    if (ok)
    {
        ULONG realdepth;

        screen->DInfo.dri_Screen = &screen->Screen; //useful sometimes ;)

        realdepth = GetBitMapAttr( screen->Screen.RastPort.BitMap, BMA_DEPTH );
        if (realdepth > 8)
        {
            screen->DInfo.dri_Flags |= DRIF_DIRECTCOLOR;
        }
        else
        {
            screen->DInfo.dri_Flags &= ~DRIF_DIRECTCOLOR;
        }
#ifdef SKINS
        if (!(screen->DInfo.dri_Colors = AllocMem(4 * DRIPEN_NUMDRIPENS,MEMF_PUBLIC)))
            ok = FALSE;

        if (ok)
        {
            CopyMem(&defaultdricolors,screen->DInfo.dri_Colors,sizeof (defaultdricolors));
            memset(((UBYTE *) screen->DInfo.dri_Colors) + sizeof(defaultdricolors), 0, 4 * DRIPEN_NUMDRIPENS - sizeof(defaultdricolors));
        }
#endif
        if (ok)
        {
            if ((screen->DInfo.dri_Customize = AllocMem(sizeof (struct IntuitionCustomize),MEMF_PUBLIC|MEMF_CLEAR)))
            {
                struct IntuitionCustomize *ic;
                ic = screen->DInfo.dri_Customize;
#ifdef SKINS
                screen->DInfo.dri_Flags |= DRIF_SKINSSUPPORT;
                /* This initializes CustomizePrefs structure */

                int_SkinAction(SKA_LoadSkin,(ULONG*)&screen->DInfo,(struct Screen *)screen,IntuitionBase);
#else
                ok = int_LoadDecorator(NULL, screen, IntuitionBase);
#endif
            }
            else
            {
                ok = FALSE;
            }
        }
    }

#ifdef SKINS
    if (ok)
    {
        struct windowclassprefs *wcprefs;

        wcprefs = (struct windowclassprefs *)int_GetCustomPrefs(TYPE_WINDOWCLASS,&screen->DInfo,IntuitionBase);
        if (wcprefs->flags & WINDOWCLASS_PREFS_1TO1FRAMES)
        {
            screen->Screen.WBorLeft   = screen->Screen.WBorTop;
            screen->Screen.WBorRight  = screen->Screen.WBorTop;
            screen->Screen.WBorBottom = screen->Screen.WBorTop;
        }
#ifdef TITLEHACK
        screen->Screen.WBorTop   += wcprefs->titlebarincrement;
#endif
        int_FreeCustomPrefs(TYPE_WINDOWCLASS,&screen->DInfo,IntuitionBase);
    }

#endif

    if (ok)
    {
        struct TagItem sysi_tags[] =
        {
            {SYSIA_Which    , MENUCHECK     	    },
            {SYSIA_DrawInfo , (IPTR)&screen->DInfo  },
            {SYSIA_UserBuffer, screen->DecorUserBuffer },

            {TAG_DONE                       	    }
        };

        screen->DInfo.dri_CheckMark = NewObjectA(NULL, "sysiclass", sysi_tags);
        DEBUG_OPENSCREEN(dprintf("OpenScreen: CheckMark 0x%lx\n",
                                 screen->DInfo.dri_CheckMark));

        sysi_tags[0].ti_Data = AMIGAKEY;

        screen->DInfo.dri_AmigaKey  = NewObjectA(NULL, "sysiclass", sysi_tags);
        DEBUG_OPENSCREEN(dprintf("OpenScreen: AmigaKey 0x%lx\n",
                                 screen->DInfo.dri_AmigaKey));

        sysi_tags[0].ti_Data = SUBMENUIMAGE;
        screen->DInfo.dri_Customize->submenu  = NewObjectA(NULL, "sysiclass", sysi_tags);
        if (!screen->DInfo.dri_Customize->submenu) ok = FALSE;
#ifdef SKINS
        sysi_tags[0].ti_Data = MENUTOGGLEIMAGE;
        screen->DInfo.dri_Customize->menutoggle  = NewObjectA(NULL, "sysiclass", sysi_tags);
        if !screen->DInfo.dri_Customize->menutoggle) ok = FALSE;
#endif

        if (!screen->DInfo.dri_CheckMark || !screen->DInfo.dri_AmigaKey) ok = FALSE;
    }



    if (ok)
    {
        SetFont(&screen->Screen.RastPort, screen->DInfo.dri_Font);

        AskFont(&screen->Screen.RastPort, (struct TextAttr *) &screen->textattr);

        screen->Screen.Font = (struct TextAttr *) &screen->textattr;

        DEBUG_OPENSCREEN(dprintf("OpenScreen: Font %s/%d\n",
                                 screen->textattr.tta_Name, screen->textattr.tta_YSize));

        screen->Screen.BarVBorder  = 1; /* on the Amiga it is (usually?) 1 */
        screen->Screen.BarHBorder  = 5;
        screen->Screen.MenuVBorder = 2; /* on the Amiga it is (usually?) 2 */
        screen->Screen.MenuHBorder = 4;

        screen->Screen.BarHeight = screen->DInfo.dri_Font->tf_YSize + screen->Screen.BarVBorder * 2;
    }


     if (ok)
     {
#define SDEPTH_HEIGHT (screen->Screen.BarHeight + 1)
#ifdef IA_Screen
#undef IA_Screen
#endif
#define IA_Screen   (IA_Dummy + 0x1f) /* OS v44 includes!*/

            struct TagItem sdepth_tags[] =
            {
                {GA_Image	, 0     	},
                {GA_Top 	, 0             },
#if SQUARE_WIN_GADGETS
		{GA_Width	, SDEPTH_HEIGHT },
#endif
                {GA_Height	, SDEPTH_HEIGHT },
                {GA_SysGadget   , TRUE          },
                {GA_SysGType    , GTYP_SDEPTH   },
                {GA_RelVerify   , TRUE          },
                {TAG_DONE	             	}
            };

            Object *im = 0;

            if (!(screen->Screen.Flags & SCREENQUIET))
            {
                im = CreateStdSysImage(SDEPTHIMAGE, SDEPTH_HEIGHT, &screen->Screen, (APTR) ((struct IntScreen *)screen)->DecorUserBuffer,
		    	    	       (struct DrawInfo *)&screen->DInfo, IntuitionBase);
            }

            sdepth_tags[0].ti_Data = (IPTR)im;

            screen->depthgadget = NewObjectA(NULL, BUTTONGCLASS, sdepth_tags );

            DEBUG_OPENSCREEN(dprintf("OpenScreen: DepthGadget 0x%lx\n",
                                     screen->depthgadget));

            screen->Screen.FirstGadget = (struct Gadget *)screen->depthgadget;
            if (screen->Screen.FirstGadget)
            {
                screen->Screen.FirstGadget->GadgetType |= GTYP_SCRGADGET;

	    #if 0
                struct TagItem gadtags[] =
                {
                    {GA_RelRight, 0 },
                    {TAG_DONE       }
                };
                IPTR width;

                GetAttr(GA_Width, screen->depthgadget, &width);

                gadtags[0].ti_Data = -width + 1;
                SetAttrsA(screen->depthgadget, gadtags);
    	    #endif

            }
            else
            {
                if (im) DisposeObject(im);
            }


#if DEBUG_OpenScreen
        {
            int i;

            for (i = 0; i <= screen->DInfo.dri_NumPens; i++)
            {
                dprintf("OpenScreen: dri_Pen[%ld] = %ld\n", i, screen->DInfo.dri_Pens[i]);
            }
        }
#endif

#ifdef USEWINDOWLOCK
        /* let's wait for user to finish window drag/size actions to avoid
        deadlocks and not break user's input */
        ObtainSemaphore(&GetPrivIBase(IntuitionBase)->WindowLock);
        windowlock = TRUE;
#endif

        int_CalcSkinInfo(&screen->Screen,IntuitionBase);
#ifdef SKINS
        int_InitTitlebarBuffer(screen,IntuitionBase);
#endif

        D(bug("Calling SetRast()\n"));

        DEBUG_OPENSCREEN(dprintf("OpenScreen: Set background color Pen %ld\n",screen->Pens[BACKGROUNDPEN]));
        /* Set screen to background color */
        SetRast(&screen->Screen.RastPort, screen->Pens[BACKGROUNDPEN]);

        D(bug("SetRast() called\n"));

        DEBUG_OPENSCREEN(dprintf("OpenScreen: Creating screen bar\n"));

#ifdef SKINS
        if (workbench)
        {
            if (GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags & ICF_NOWBTITLEBAR) screen->SpecialFlags |= SF_InvisibleBar;
            if (GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags & ICF_DISAPPEARINGTITLEBAR) screen->SpecialFlags |= SF_AppearingBar;
        }
#endif
	if (draggable)
	    screen->SpecialFlags |= SF_Draggable;
	DEBUG_OPENSCREEN(bug("OpenScreen: Special flags: 0x%04X\n", screen->SpecialFlags));

        //jDc: ALL screens MUST have BarLayer!
        CreateScreenBar(&screen->Screen, IntuitionBase);

        DEBUG_OPENSCREEN(bug("OpenScreen: ScreenBar = %p\n", screen->Screen.BarLayer));

        if (!screen->Screen.BarLayer) ok = FALSE;

        /*
        ** jDc: better modify the screen list in sync with inputhandler, this for example allows us to scan the list
        ** without any locks when we are on input.device context
        */
        if (ok)
        {
            struct OpenScreenActionMsg   msg;
            struct List     	     	*list = LockPubScreenList();

            msg.Screen = screen;
            msg.NewScreen = &ns;
            msg.List = list;

            DEBUG_OPENSCREEN(dprintf("OpenScreen: Calling DoSyncAction()\n"));

            DoSyncAction((APTR)int_openscreen,&msg.msg,IntuitionBase);

            DEBUG_OPENSCREEN(dprintf("OpenScreen: DoSyncAction returned\n"));

            UnlockPubScreenList();
        }

    } /* if (ok) */

    if (ok)
    {
        GetAttr(POINTERA_SharedPointer, GetPrivIBase(IntuitionBase)->DefaultPointer, (IPTR *)&screen->Pointer);
        ObtainSharedPointer(screen->Pointer, IntuitionBase);
        DEBUG_OPENSCREEN(dprintf("OpenScreen: Sprite DefaultPtr 0x%lx\n",&screen->Pointer));
    }

#ifdef __MORPHOS__
    if (ok)
    {
        ok = MakeScreen(&screen->Screen) == 0;
        DEBUG_OPENSCREEN(dprintf("OpenScreen: MakeScreen %s\n", ok ? "ok" : "failed"));
    }
#endif
    if (ok)
    {
        ok = RethinkDisplay() == 0;
        DEBUG_OPENSCREEN(dprintf("OpenScreen: RethinkDisplay %s\n", ok ? "ok" : "failed"));
    }

#ifdef SKINS
    if (ok)
    {
        FireMenuMessage(MMCODE_STARTCLOCK,NULL,NULL,IntuitionBase);
    }
#endif

#ifdef USEWINDOWLOCK
    if (windowlock) ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->WindowLock);
#endif

    if (!ok)
    {
        if (li_inited)
        {
            DEBUG_OPENSCREEN(dprintf("OpenScreen: Get ThinLayerInfo\n"));
            ThinLayerInfo(&screen->Screen.LayerInfo);
        }

        if (screen->Screen.ViewPort.ColorMap)
        {
            FreeColorMap(screen->Screen.ViewPort.ColorMap);
        }

        if (screen->Screen.BarLayer)
        {
            DEBUG_OPENSCREEN(dprintf("OpenScreen: KillScreenBar\n"));
            KillScreenBar(&screen->Screen, IntuitionBase);
        }

        DisposeObject(screen->DInfo.dri_Customize->submenu);
#ifdef SKINS
        DisposeObject(screen->DInfo.dri_Customize->menutoggle);
#endif
        if (screen->DInfo.dri_Customize) FreeMem(screen->DInfo.dri_Customize,sizeof (struct IntuitionCustomize));
#ifdef SKINS
        if (screen->DInfo.dri_Colors) FreeMem(screen->DInfo.dri_Colors,4 * DRIPEN_NUMDRIPENS);
#endif
        if (screen->DInfo.dri_AmigaKey)
        {
            DEBUG_OPENSCREEN(dprintf("OpenScreen: Dispose AmigaKey Object\n"));
            DisposeObject(screen->DInfo.dri_AmigaKey);
        }
        if (screen->DInfo.dri_CheckMark)
        {
            DEBUG_OPENSCREEN(dprintf("OpenScreen: Dispose CheckMark Object\n"));
            DisposeObject(screen->DInfo.dri_CheckMark);
        }

        if (screen->DInfo.dri_Font)
        {
            DEBUG_OPENSCREEN(dprintf("OpenScreen: Close Font\n"));
            CloseFont(screen->DInfo.dri_Font);
        }

        if (screen->AllocatedBitmap && !(ns.Type & CUSTOMBITMAP))
        {
            DEBUG_OPENSCREEN(dprintf("OpenScreen: Free BitMap\n"));
            FreeBitMap(screen->AllocatedBitmap);
        }

        if (screen->Screen.ViewPort.RasInfo)
        {
            DEBUG_OPENSCREEN(dprintf("OpenScreen: Free RasInfo\n"));
            FreeMem(screen->Screen.ViewPort.RasInfo, sizeof (struct RasInfo));
        }

        if (rp_inited)
        {
            DEBUG_OPENSCREEN(dprintf("OpenScreen: Trash Rastport\n"));
            DeinitRastPort(&screen->Screen.RastPort);
        }

        if (screen->DecorUserBuffer)
        {
            FreeMem((void *)screen->DecorUserBuffer, screen->DecorUserBufferSize);
        }

        if (screen->Decorator) screen->Decorator->nd_cnt--;

        DEBUG_OPENSCREEN(dprintf("OpenScreen: Free Screen\n"));
        FreeMem (screen, sizeof (struct IntScreen));

        screen = 0;

    } /* if (!ok) */

    DEBUG_OPENSCREEN(dprintf("OpenScreen: return 0x%lx\n", screen));

    FireScreenNotifyMessage((IPTR) screen, SNOTIFY_AFTER_OPENSCREEN, IntuitionBase);

    ReturnPtr ("OpenScreen", struct Screen *, (struct Screen *)screen);

    AROS_LIBFUNC_EXIT

} /* OpenScreen */

static VOID int_openscreen(struct OpenScreenActionMsg *msg,
                            struct IntuitionBase *IntuitionBase)
{
    ULONG   	    	 lock;
    struct IntScreen 	*screen = msg->Screen;
    struct NewScreen 	*ns = msg->NewScreen;
    struct List     	*list = msg->List;
    struct Screen	*oldFirstScreen;

    DEBUG_OPENSCREEN(dprintf("OpenScreen: Checking for pubScrNode (0x%lx)\n",screen->pubScrNode));

    /* If this is a public screen, we link it into the intuition global
       public screen list */
    if (screen->pubScrNode != NULL)
    {
        /* Set the pointer to ourselves */
        GetPrivScreen(screen)->pubScrNode->psn_Screen = &screen->Screen;

        DEBUG_OPENSCREEN(dprintf("OpenScreen: Add Screen to PubList\n"));
        AddTail(list, (struct Node *)GetPrivScreen(screen)->pubScrNode);
    }

    lock = LockIBase(0);

    oldFirstScreen = IntuitionBase->FirstScreen;

    if (ns->Type & SCREENBEHIND)
    {
        struct Screen **ptr = &IntuitionBase->FirstScreen;

        DEBUG_OPENSCREEN(dprintf("OpenScreen: Sort Behind\n"));
        if (!*ptr)
            IntuitionBase->ActiveScreen = &screen->Screen;
        while (*ptr)
            ptr = &(*ptr)->NextScreen;
        *ptr = &screen->Screen;
    }
    else
    {
        screen->Screen.NextScreen = IntuitionBase->FirstScreen;
        IntuitionBase->FirstScreen =
            IntuitionBase->ActiveScreen = &screen->Screen;
        DEBUG_OPENSCREEN(dprintf("OpenScreen: Set as ActiveScreen\n"));
    }

    /* If it's the first screen being opened, activate its monitor */
    if (!oldFirstScreen)
	ActivateMonitor(screen->IMonitorNode, -1, -1, IntuitionBase);

    /* set the default pub screen */
    if (GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags & ICF_DEFPUBSCREEN)
    {
        if ((IntuitionBase->FirstScreen == &screen->Screen) && screen->pubScrNode && (screen->Screen.Flags & (PUBLICSCREEN | WBENCHSCREEN)))
        {
            GetPrivIBase(IntuitionBase)->DefaultPubScreen = &screen->Screen;
        }
    }

    UnlockIBase(lock);

    D(bug("set active screen\n"));

    AddResourceToList(screen, RESOURCE_SCREEN, IntuitionBase);

}
