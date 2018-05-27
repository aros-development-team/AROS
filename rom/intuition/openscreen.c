/*
    Copyright © 1995-2018, The AROS Development Team. All rights reserved.
    Copyright © 2001-2013, The MorphOS Development Team. All Rights Reserved.
    $Id$

    Open a new screen.
*/

/*
 * Things added by AROS, which needs to be kept when merging with newer MorphOS releases:
 *
 * 1. Explicit library bases
 * 2. FireScreenNotifyMessage() calls
 * 3. AddResourceToList() call
 * 4. Decoration calls (see intuition_customize.h)
 * 5. Hardcoded initial color table is not used and removed.
 *    AmigaOS-compatible palette is used instead.
 * 6. Other placed marked by 'AROS:' in comments.
 * 7. Check #ifdef's. Some of them were rearranged or completely deleted.
 *    We reuse MorphOS skin code where appropriate.
 */

#include <aros/config.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <intuition/screens.h>
#include <intuition/intuition.h>
#include <intuition/imageclass.h>
#include <intuition/gadgetclass.h>
#include <intuition/monitorclass.h>
#include <graphics/modeid.h>
#include <graphics/videocontrol.h>
#include <graphics/displayinfo.h>
#include <graphics/rpattr.h>
#include <prefs/screenmode.h>
#include <proto/input.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#ifdef __MORPHOS__
#include <proto/cybergraphics.h>
#include <cybergraphx/cybergraphics.h>
#else
#include <hidd/gfx.h>
#endif
#include "intuition_intern.h"
#include "intuition_customize.h"
#include "intuition_extend.h"
#include "inputhandler.h"
#include "inputhandler_support.h"
#include "inputhandler_actions.h"
#include "menus.h"
#include "monitorclass_intern.h"
#include "monitorclass_private.h"

// disabled as it causes compatibility issues
//#define USE8BITHACK

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
    struct IntuiActionMsg msg;
    struct IntScreen *Screen;
    struct NewScreen *NewScreen;
    struct List *List;
    BOOL Success;
};

VOID int_openscreen(struct OpenScreenActionMsg *msg,struct IntuitionBase *IntuitionBase);

#ifdef SKINS
extern const ULONG defaultdricolors[DRIPEN_NUMDRIPENS];
#endif

#if DEBUG
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
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
        The function relies on private data being passed in
        DimensionInfo.reserved[0] by graphics.library/GetDisplayInfoData().
        Keep this in sync when modifying the code.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct GfxBase        *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct LayersBase     *LayersBase = GetPrivIBase(IntuitionBase)->LayersBase;
    struct Library        *UtilityBase = GetPrivIBase(IntuitionBase)->UtilityBase;
    struct NewScreen       ns;
    struct TagItem        *tag, *tagList;
    struct IntScreen      *screen;
    int                    success;
    struct Hook           *layer_info_hook = NULL;
    struct ColorSpec      *colors = NULL;
    ULONG                 *errorPtr;   /* Store error at user specified location */
    UWORD                 *customdripens = NULL;
    ULONG                 *colors32 = NULL;
    WORD                   sysfont = -1;
    BOOL                   ok = TRUE, rp_inited = FALSE, li_inited = FALSE, sharepens = FALSE;
#ifdef USEWINDOWLOCK
    BOOL                   windowlock = FALSE;
    BOOL                   dowindowlock = TRUE;
#endif
    struct Rectangle      *dclip = NULL;
    LONG                   overscan = OSCAN_TEXT;
    DisplayInfoHandle      displayinfo = NULL;
    struct DisplayInfo     dispinfo;
    struct DimensionInfo   dimensions;
#ifdef __MORPHOS__
    struct MonitorInfo     monitor;
#endif
    ULONG                  allocbitmapflags = BMF_DISPLAYABLE;
    char                  *skinname = 0;
#ifdef SKINS
    ULONG                  vesafallback = 0;
    ULONG                 *modecontrol = 0;
    BOOL                   compositing = FALSE;
#endif
    IPTR                   vctl = 0;
    char                  *monitorname = 0;
    BOOL                   exactname = FALSE;
    BOOL                   showpointer = TRUE;
    BOOL                   gammacontrol = FALSE;
    UBYTE                 *gammared = NULL, *gammablue = NULL, *gammagreen = NULL;
    BOOL                   support3d = FALSE;
    BOOL                   adaptsize = FALSE;
    ULONG                  displaywidth = 0;
    ULONG                  displayheight = 0;
    //ULONG                  lock;
    WORD                   numcolors = 0;
    UWORD                  spritebase;
    BOOL                   workbench = FALSE;
    ULONG                  requesteddepth = 1;
    BOOL                   draggable = TRUE;
    ULONG                  compflags = COMPF_ABOVE; // Default to AmigaOS like behaviour.
    struct Hook            *compalphahook = NULL;

    struct TagItem   modetags[] =
    {
        { BIDTAG_Depth        , 0UL   },
        { BIDTAG_DesiredWidth , 0UL   },
        { BIDTAG_DesiredHeight, 0UL   },
        { BIDTAG_NominalWidth , 0UL   },
        { BIDTAG_NominalHeight, 0UL   },
        { BIDTAG_DIPFMustHave , 0UL   },
        { TAG_DONE                    }
    };

    ULONG modeid = INVALID_ID;

    ASSERT_VALID_PTR_ROMOK(newScreen);


#define COPY(x)     screen->Screen.x = ns.x
#define SetError(x) if (errorPtr != NULL) *errorPtr = x;

    D(bug("OpenScreen (%p = { Left=%d Top=%d Width=%d Height=%d Depth=%d })\n"
          , newScreen
          , newScreen->LeftEdge
          , newScreen->TopEdge
          , newScreen->Width
          , newScreen->Height
          , newScreen->Depth
         ));

    FireScreenNotifyMessage((IPTR) newScreen, SNOTIFY_BEFORE_OPENSCREEN, IntuitionBase);
    ns = *newScreen;

    if (newScreen->Type & NS_EXTENDED)
    {
        tagList = ((struct ExtNewScreen *)newScreen)->Extension;
    } else {
        tagList = NULL;
    }

    DEBUG_OPENSCREEN(dprintf("OpenScreen: Left %d Top %d Width %d Height %d Depth %d Tags 0x%lx\n",
                 ns.LeftEdge, ns.TopEdge, ns.Width, ns.Height, ns.Depth, tagList));

#ifdef __MORPHOS__
    if (!CyberGfxBase)
    {
        struct Library *lib = ComplainOpenLibrary(COMPLAIN_CYBERGFX,FALSE,IntuitionBase);
        Forbid();
        if (!CyberGfxBase)
        {
            CyberGfxBase = lib;
            Permit();
        }
        else
        {
            Permit();
            CloseLibrary(lib);
        }
    }
    if (!CyberGfxBase)
    { 
        FireScreenNotifyMessage(0, SNOTIFY_AFTER_OPENSCREEN, IntuitionBase);
        return NULL;
    }

    if (!LocaleBase)
    {
        struct Library *lib    = ComplainOpenLibrary(COMPLAIN_LOCALE,FALSE,IntuitionBase);
        Forbid();
        if (!LocaleBase)
        {
            LocaleBase = lib;
            Permit();
            OpenintuitionCatalog(IntuitionBase);
        }
        else
        {
            Permit();
            CloseLibrary(lib);
        }
    }

    if (!CGXSystemBase)
    {
        struct Library *lib    = ComplainOpenLibrary(COMPLAIN_CGXSYSTEM,FALSE,IntuitionBase);
        Forbid();
        if (!CGXSystemBase)
        {
            CGXSystemBase = lib;
            Permit();
        }
        else
        {
            Permit();
            CloseLibrary(lib);
        }
    }
#endif

    screen = NewObjectA(IBase->screenclass,0,0);

    DEBUG_OPENSCREEN(dprintf("OpenScreen: screen 0x%lx\n", screen));

    /* Do this really early to be able to report errors */
    errorPtr = (ULONG *)GetTagData((Tag)SA_ErrorCode, (IPTR)NULL, (struct TagItem *)tagList);

    DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_ErrorCode 0x%lx\n",errorPtr));

    if (screen == NULL)
    {
        SetError(OSERR_NOMEM);
        ok = FALSE;
    }

    if (ok && tagList)
    {
        char *pubname = NULL;
        
        modeid = GetTagData(SA_DisplayID,INVALID_ID, tagList);
#ifndef __AROS__
        /*
         * AROS: MorphOS private tag temporarily disabled. Used by OpenWorkbench().
         * Information from MorphOS intuition.notes file:
         *
         * Added SA_OpenWorkbench to fix the OpenScreen handling for this special case, fixes
         * the problem with autoscroll promoting to a bigger res than the one selected in prefs
         *
         * CHECKME: May be we also need this ?
         */
        workbench = GetTagData(SA_OpenWorkbench,FALSE,tagList);

        if (workbench)
        {
            compositing = TRUE;
        }
#endif

        DEBUG_OPENSCREEN(dprintf("OpenScreen: modeid from taglist: %lx\n",modeid));

        if (GetTagData(SA_LikeWorkbench, FALSE, tagList))
        {
            DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_LikeWorkbench\n"));

#ifndef SKINS
            /*
             * AROS: Use oldstyle ScreenModePrefs structure.
             * CHECKME: Can we merge better ?
             */
            if (!GetPrivIBase(IntuitionBase)->ScreenModePrefs)
                SetDisplayDefaults(IntuitionBase);

            ns.Width = GetPrivIBase(IntuitionBase)->ScreenModePrefs->smp_Width;
            ns.Height = GetPrivIBase(IntuitionBase)->ScreenModePrefs->smp_Height;
            ns.Depth = GetPrivIBase(IntuitionBase)->ScreenModePrefs->smp_Depth;
            modeid = GetPrivIBase(IntuitionBase)->ScreenModePrefs->smp_DisplayID;

            if (GetPrivIBase(IntuitionBase)->ScreenModePrefs->smp_Control & SMF_AUTOSCROLL)
            {
                /* need to mark autoscroll */
                ns.Type |= AUTOSCROLL;
            }
#else
            ns.Width = IBase->AmbientScreenMode.DisplayWidth;
            ns.Height = IBase->AmbientScreenMode.DisplayHeight;
            ns.Depth = IBase->AmbientScreenMode.DisplayDepth;
            if (IBase->AmbientScreenMode.AutoScroll) ns.Type |= AUTOSCROLL;
            displaywidth = IBase->AmbientScreenMode.ModeWidth;
            displayheight = IBase->AmbientScreenMode.ModeHeight;

            /* jDc: do NOT copy modeid of wb as that will not allow overloading depth param properly */

            /* unless it's OpenWorkbench! */

            if (workbench)
            {
                modeid = INVALID_ID; // we'll use the stored ID only in the worst case!
                compositing = IBase->AmbientScreenMode.Compositing;
            }

            /* let's try to limit the new screen to the same monitor ambient would use */
            /* but only if modeid is a weak one */
            if ((modeid == INVALID_ID) || (ModeNotAvailable(modeid)) || !FindDisplayInfo(modeid))
            {
                if (IBase->AmbientScreenMode.ModeMonitor[0])
                {
                    monitorname = IBase->AmbientScreenMode.ModeMonitor;
                }
                else
                {
                    struct IMonitorNode *node = FindMonitorNode(IBase->AmbientScreenMode.ModeID,IntuitionBase);
                    
                    if (node)
                    {
                        monitorname = node->MonitorName;
                    }
                }
            }
#endif

            sysfont = 1;
            sharepens = TRUE; /* not sure */
        }

        if ((pubname = (char*)GetTagData(SA_PubName, 0, tagList)))
        {
            /* Name of this public screen. */
            struct PubScreenNode *oldpsn;
            struct List *list;

            list = LockPubScreenList();

            if ((strcmp(pubname, "Workbench") == 0) || workbench)
            {
                if (IBase->WorkBench)
                {
                    ok = FALSE;
                }

                workbench = TRUE;
            }
            else
            {
                /* jDc: LockPubScreen is not safe here as it does not lock
                screens in private state so we could end up with two
                screens with the same name */
                oldpsn = (struct PubScreenNode *)FindName(list,(UBYTE*)pubname);

                if (oldpsn)
                {
                    SetError(OSERR_PUBNOTUNIQUE);
                    ok = FALSE;
                }
            }

            UnlockPubScreenList();

            if (ok)
            {
                screen->pubScrNode = AllocMem(sizeof(struct PubScreenNode), MEMF_CLEAR);

                DEBUG_OPENSCREEN(dprintf("OpenScreen: pubScrNode 0x%lx\n",screen->pubScrNode));

                if (screen->pubScrNode == NULL)
                {
                    SetError(OSERR_NOMEM);
                    ok = FALSE;
                }

                if (ok && (screen->pubScrNode->psn_Node.ln_Name = AllocVec(MAXPUBSCREENNAME + 1,MEMF_ANY)))
                {
                    UBYTE sigbit;

                    if ((ns.Type & SCREENTYPE) == CUSTOMSCREEN)
                    {
                        ns.Type &= ~SCREENTYPE;
                        ns.Type |= PUBLICSCREEN;
                    }

                    /* Always open public screens in private mode. */
                    screen->pubScrNode->psn_Flags |= PSNF_PRIVATE;
                    strcpy(screen->pubScrNode->psn_Node.ln_Name, pubname);

                    /* Task that should be signalled when the public screen loses
                       its last visitor window. */

                    screen->pubScrNode->psn_SigTask    = (struct Task *)GetTagData(SA_PubTask, 0, tagList);

                    /* Signal bit number to use when signalling public screen
                       signal task. */
            
                    sigbit = GetTagData(SA_PubSig,(ULONG)-1,tagList);

                    DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_PubSig 0x%lx\n",sigbit));

                    if (sigbit != (UBYTE)-1)
                    {
                        if (!screen->pubScrNode->psn_SigTask) screen->pubScrNode->psn_SigTask = FindTask(NULL);
                        screen->pubScrNode->psn_SigBit = sigbit;
                    }
                    else
                    {
                        /* do not signal with -1 ! */
                        screen->pubScrNode->psn_SigTask = NULL;
                    }
                }
                else
                {
                    SetError(OSERR_NOMEM);
                    FreeMem(screen->pubScrNode, sizeof(struct PubScreenNode));
                    screen->pubScrNode = NULL;
                    ok = FALSE;
                }
            }

        }

        #ifdef SKINS
        if (modeid != INVALID_ID)
        {
            /* modes can be on other monitors */
            monitorname = NULL;
        }
        #endif

        while((tag = NextTagItem ((struct TagItem **)&tagList)))
        {
#if 1
            DEBUG_OPENSCREEN(dprintf("OpenScreen: Tag 0x%08lx Data 0x%08lx\n",
                         tag->ti_Tag, tag->ti_Data));
#endif
            switch(tag->ti_Tag)
            {
            case SA_CompositingFlags:
                compflags = tag->ti_Data;
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_CompositingFlags 0x%p\n", compflags));
                break;
            case SA_AlphaPreCompositingHook:
                compalphahook = (struct Hook *)tag->ti_Data;
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_AlphaPreCompositingHook 0x%p\n", compalphahook));
                break;
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
                if ((tag->ti_Data != ~0) && (tag->ti_Data > 32767))
                {
                    ok = FALSE;
                }
                ns.Width     = tag->ti_Data;
                break;
            case SA_Height:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_Height %ld\n",tag->ti_Data));
                if ((tag->ti_Data != ~0) && (tag->ti_Data > 32767))
                {
                    ok = FALSE;
                }
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
                ns.Type    &= ~SCREENTYPE;
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
                    DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_BitMap==NULL specified, custom BitMap use disabled\n"));
                }
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

            case SA_SharePens:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_SharePens 0x%lx\n",tag->ti_Data));
                sharepens = tag->ti_Data ? TRUE : FALSE;
                if (tag->ti_Data)
                {
                    ns.Type |= PENSHARED;
                } else {
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

            case SA_DisplayID:
            case SA_LikeWorkbench:
            case SA_ErrorCode:
            case SA_PubName:
            case SA_PubSig:
            case SA_PubTask:
                /*
                 * handled elsewhere
                 */
                break;

            case SA_AutoScroll:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_AutoScroll 0x%lx\n",tag->ti_Data));
                if (tag->ti_Data)
                {
                    ns.Type |= AUTOSCROLL;
                }
                else
                {
                    ns.Type &= ~AUTOSCROLL;
                }
                break;

            case SA_FullPalette:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_FullPalette 0x%lx\n",tag->ti_Data));
                break;
            case SA_ColorMapEntries:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_ColorMapEntries 0x%lx\n",tag->ti_Data));
                numcolors = tag->ti_Data; /* AROS: Added support for this tag */
                break;
            case SA_Parent:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_Parent 0x%lx\n",tag->ti_Data));
                break;
            case SA_Draggable:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_Draggable 0x%lx\n",tag->ti_Data));
                draggable = tag->ti_Data; /* AROS: Added support for this tag */
                break;
            case SA_Exclusive:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_Exclusive 0x%lx\n",tag->ti_Data));
                break;
            case SA_VideoControl:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_VideoControl 0x%lx\n",tag->ti_Data));
                vctl = tag->ti_Data; /* AROS: Added support for this tag */
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

            case SA_MonitorName:
                monitorname = (char*)tag->ti_Data;
                break;
#ifndef __AROS__ /* AROS: Disable MorphOS-specific extensions and private stuff */
            case SA_SkinName: /* TODO: Complete support for named skins and enable this tag. Value needed. */
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_SkinName (%s)\n",(char *)tag->ti_Data));
                skinname = (char *)tag->ti_Data;
                break;

            case SA_VesaFallback:
                vesafallback = tag->ti_Data;
                break;
            
            case SA_ModeEditControl:
                modecontrol = (ULONG*)tag->ti_Data;
                break;

            case SA_WindowLock: /* TODO: We also can support this private tag. In which situations we don't need locking ? */
                dowindowlock = tag->ti_Data ? TRUE : FALSE;
                break;

            case SA_CompositingLayers:
                compositing = tag->ti_Data ? TRUE : FALSE;
                break;
#endif
            case SA_ShowPointer:
                showpointer = tag->ti_Data ? TRUE : FALSE;
                break;

            case SA_GammaControl:
                gammacontrol = tag->ti_Data ? TRUE : FALSE;
                break;

            case SA_GammaRed:
                gammared = (UBYTE*)tag->ti_Data;
                break;

            case SA_GammaBlue:
                gammablue = (UBYTE*)tag->ti_Data;
                break;

            case SA_GammaGreen:
                gammagreen = (UBYTE*)tag->ti_Data;
                break;

            case SA_3DSupport:
                support3d = tag->ti_Data ? TRUE : FALSE;
                break;

            case SA_AdaptSize:
                adaptsize = tag->ti_Data ? TRUE : FALSE;
                break;

            case SA_DisplayWidth:
                displaywidth = (ULONG)tag->ti_Data;
                break;

            case SA_DisplayHeight:
                displayheight = (ULONG)tag->ti_Data;
                break;

            case SA_ExactMatchMonitorName:
                exactname = tag->ti_Data ? TRUE : FALSE;
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

    DEBUG_OPENSCREEN(dprintf("OpenScreen: Left %d Top %d Width %d Height %d Depth %d Tags 0x%lx\n",
                 ns.LeftEdge, ns.TopEdge, ns.Width, ns.Height, ns.Depth, tagList));

    requesteddepth = ns.Depth;

    /* Fail if bogus size is requested - Piru/jDc */
    if ((ns.Width < -1) || (ns.Height < -1))
    {
        DEBUG_OPENSCREEN(dprintf("!!! OpenScreen(): Width and/or Height negative !!!\n");)
        ok = FALSE;
    }

    /* First Init the RastPort then get the BitPlanes!! */

#ifdef __MORPHOS__
    #include "workaround.openscreen.c"
#endif
    /* AROS: Use 3D support code */
    if (support3d)
    {
        struct IMonitorNode *node = NULL;
        ULONG depth = (ULONG)-1;

#ifndef __AROS__
        /*AROS: We don't need these flags because we keep ModeID together with the BitMap */
        screen->Support3D = TRUE;
        allocbitmapflags |= BMF_3DTARGET;
#endif
        /* check for SA_DisplayID or SA_MonitorName, this can also return other monitor
        within the same class of monitors (ex radeon7k instead of 9k) */
        if (monitorname || (modeid != INVALID_ID))
        {
            DEBUG_OPENSCREEN(dprintf("OpenScreen: 3d hunt modeid 0x%lx monname %s\n",modeid,monitorname ? monitorname : "none"));
            node = FindBestMonitorNode(NULL,monitorname,modeid,IntuitionBase);
            if (node) depth = FindBest3dDepth(ns.Depth,node,IntuitionBase);
        }

        /* no suitable depth? find a monitor with best 3d capabilities */
        if ((depth == (ULONG)-1) && !exactname)
        {
            DEBUG_OPENSCREEN(dprintf("OpenScreen: found no suitable depth. monnode 0x%lx\n",node));
            node = FindBest3dMonitor(NULL,IntuitionBase);
            if (node) depth = FindBest3dDepth(ns.Depth,node,IntuitionBase);
        }

        /* found a usable depth? find a modeid with that depth within the monitor we found */
        if (depth != (ULONG)-1)
        {
            DEBUG_OPENSCREEN(dprintf("OpenScreen: found suitable depth %d. monnode 0x%lx\n",depth,node));
            ns.Depth = depth;

            modeid = FindBestModeID(node->MonitorName,ns.Depth,(displaywidth) ? displaywidth : ns.Width,(displayheight) ? displayheight : ns.Height,IntuitionBase);

            if (modeid == INVALID_ID)
            {
                DEBUG_OPENSCREEN(dprintf("OpenScreen: no 3d available, fail (1)!\n"));
                ok = FALSE;
            }
        } else {
            /* whoops, no 3d support at all, fail! */
            DEBUG_OPENSCREEN(dprintf("OpenScreen: no 3d available, fail! (2)\n"));
            ok = FALSE;
        }

    }
    else if (monitorname)
    {
        ULONG newmodeid = INVALID_ID;
        newmodeid = FindBestModeID(monitorname,ns.Depth,(displaywidth) ? displaywidth : ns.Width,(displayheight) ? displayheight : ns.Height,IntuitionBase);
        if (newmodeid != INVALID_ID) modeid = newmodeid;
        /* monitor not available and exactmatch requested ? */
        if (exactname)
        {
            if (newmodeid == INVALID_ID)
            {
                dprintf("uhuh, no modeid!\n");
                ok = FALSE;
            }
            else
            {
                // make sure this is the same monitor!
                struct IMonitorNode *node = FindMonitorNode(newmodeid,IntuitionBase);
                if (!node || strcmp(node->MonitorName,monitorname)) ok = FALSE;
            }
        }
        DEBUG_OPENSCREEN(dprintf("OpenScreen: monname %s modeid %lx\n",monitorname,modeid));
    }

#ifdef __MORPHOS__
    /* Note: CGX patched ModeNotAvailable() alone doesn't return reliable results due to some fallback code for old apps. - Piru */
    if ((modeid == INVALID_ID) || ModeNotAvailable(modeid) || !FindDisplayInfo(modeid))
    {
        struct TagItem bestmodetags[] =
        {
            {CYBRBIDTG_Depth,0},
            {CYBRBIDTG_NominalWidth,0},
            {CYBRBIDTG_NominalHeight,0},
            {TAG_DONE}
        };

        if((modeid & 0xF00F0000) == 0x40020000)
        {
            modeid &= 0x40020000;
        }

        bestmodetags[0].ti_Data = (ns.Depth < 8) ? 8 : ns.Depth; /* jDc: helps finding a correct mode ;)*/
        bestmodetags[1].ti_Data = (displaywidth) ? displaywidth : ns.Width;
        bestmodetags[2].ti_Data = (displayheight) ? displayheight : ns.Height;

        DEBUG_OPENSCREEN(dprintf("ns.Width %ld ns.Height %ld ns.Depth %ld !!\n",
            (LONG) ns.Width, (LONG) ns.Height, (LONG) ns.Depth);)

        modeid = BestCModeIDTagList(bestmodetags);

        DEBUG_OPENSCREEN(dprintf("BestCModeIDTagList returned %ld\n",modeid);)
    }
#endif

#ifdef SKINS
    if ((workbench) && (IBase->EmergencyBoot.monitorname))
    {
        struct IMonitorNode *node;
        ULONG depthid;
        ULONG newmodeid;

        node = FindBestMonitorNode(GetMonitorClass(IBase->EmergencyBoot.monitorname,IntuitionBase),IBase->EmergencyBoot.monitorname,INVALID_ID,IntuitionBase);

        newmodeid = FakeWorkbenchMode(node,(ULONG*)&depthid,IBase->EmergencyBoot.baseresolution,IntuitionBase);

        DEBUG_OPENSCREEN(dprintf("OpenScreen: Emergency Boot! %lx\n",modeid));

        if (newmodeid != INVALID_ID)
        {
            modeid = newmodeid;
            ns.Width = IBase->EmergencyBoot.baseresolution;

            switch (ns.Width)
            {
                case 800:
                    ns.Height = 600;
                    break;

                case 1024:
                    ns.Height = 768;
                    break;

                default:
                    ns.Width = 640;
                    ns.Height = 480;
            }

            screen->VESAMode = ns.Width;
            screen->VESAModeDepthID = depthid;
        }
    }

    if (workbench && (((struct IIHData *)IBase->InputHandler->is_Data)->ActQualifier & IEQUALIFIER_CONTROL))
    {
        vesafallback = 1024;
    }

    if (vesafallback)
    {
        struct IMonitorNode *node = NULL;
        ULONG depthid;
        ULONG newmodeid;

        if (monitorname) node = FindBestMonitorNode(GetMonitorClass(monitorname,IntuitionBase),monitorname,INVALID_ID,IntuitionBase);

        newmodeid = FakeWorkbenchMode(node,(ULONG*)&depthid,vesafallback,IntuitionBase);

        DEBUG_OPENSCREEN(dprintf("OpenScreen: vesa fallback %lx\n",modeid));

        if (newmodeid != INVALID_ID)
        {
            modeid = newmodeid;
            ns.Width = vesafallback;

            switch (ns.Width)
            {
                case 800:
                    ns.Height = 600;
                    break;

                case 1024:
                    ns.Height = 768;
                    break;

                default:
                    ns.Width = 640;
                    ns.Height = 480;
            }

            screen->VESAMode = ns.Width;
            screen->VESAModeDepthID = depthid;
        }
    }

    if (modecontrol)
    {
        struct IMonitorNode *inode = FindMonitorNode(modeid,IntuitionBase);
        modeid = FakeScreenMode(inode,ns.Depth,IntuitionBase);

        DEBUG_OPENSCREEN(dprintf("OpenScreen: fake modeid %lx\n",modeid));

        if (modeid != INVALID_ID)
        {
            switch (ns.Depth)
            {
                case 24: screen->VESAModeDepthID = ID_MD24; break;
                case 16: screen->VESAModeDepthID = ID_MD16; break;
                case 15: screen->VESAModeDepthID = ID_MD15; break;
                default: screen->VESAModeDepthID = ID_MD08; break;
            }

            screen->ModeControl = modecontrol;

        } else {
            DEBUG_OPENSCREEN(dprintf("OpenScreen: failed to init testmode!\n"));
            ok = FALSE;
        }
    }

#endif

    /*
     * AROS: Got OpenScreenTags modeid without monitor, promote to chipset default
     * (if present).
     * We really need proper mode promotion support.
     */
    if ((modeid & MONITOR_ID_MASK) == 0)
    {
        if (GfxBase->DisplayFlags & PAL)
            modeid |= PAL_MONITOR_ID;
        else if (GfxBase->DisplayFlags & NTSC)
            modeid |= NTSC_MONITOR_ID;
     }

    if (INVALID_ID == modeid)
    {
#ifndef __AROS__
        WORD bestwidth,bestheight;
#endif
        modetags[0].ti_Data = ns.Depth;
        modetags[1].ti_Data = ns.Width;
        modetags[2].ti_Data = ns.Height;
        modetags[3].ti_Data = ns.Width;
        modetags[4].ti_Data = ns.Height;

#ifdef __AROS__
        /* 
         * AROS: We don't have FindBestWidthAndHeight().
         * TODO: Can we merge better here ?
         */
        if (ns.Width == STDSCREENWIDTH || ns.Width == 0 || adaptsize) {modetags[1].ti_Tag = TAG_IGNORE; modetags[3].ti_Tag = TAG_IGNORE;}
        if (ns.Height == STDSCREENWIDTH || ns.Height == 0 || adaptsize) {modetags[2].ti_Tag = TAG_IGNORE; modetags[4].ti_Tag = TAG_IGNORE;}
#else
        FindBestWidthAndHeight(&bestwidth,&bestheight,ns.Depth,IntuitionBase);

        if (ns.Width == STDSCREENWIDTH || ns.Width == 0 || adaptsize) {modetags[1].ti_Data = bestwidth; modetags[3].ti_Data = bestwidth;}
        if (ns.Height == STDSCREENWIDTH || ns.Height == 0 || adaptsize) {modetags[2].ti_Data = bestheight;modetags[4].ti_Data = bestheight;}

        DEBUG_OPENSCREEN(dprintf("Attempt BestModeIDA(). Calculated best width %d height %d. Request: %d/%d.\n",bestwidth,bestheight,modetags[1].ti_Data,modetags[2].ti_Data));
#endif
        /* AROS: Support old-style HAM or EHB request */
        if (newScreen->ViewModes & (HAM | EXTRA_HALFBRITE))
        {
            if (newScreen->ViewModes & HAM)         modetags[5].ti_Data |= DIPF_IS_HAM;
            if (newScreen->ViewModes & EXTRA_HALFBRITE) modetags[5].ti_Data |= DIPF_IS_EXTRAHALFBRITE;
        }
        else
        {
            modetags[5].ti_Tag = TAG_IGNORE;
        }

        modeid = BestModeIDA(modetags);

        #ifdef SKINS
        if (modeid == INVALID_ID && workbench)
        {
            ULONG depthid;
            ULONG newmodeid = FakeWorkbenchMode(NULL,(ULONG*)&depthid,640,IntuitionBase);
            /* create an ID */

            DEBUG_OPENSCREEN(dprintf("OpenScreen: Vesa Fallback!\n"));

            if (newmodeid != INVALID_ID)
            {
                modeid = newmodeid;
                ns.Width = 640;
                ns.Height = 480;

                screen->VESAMode = 640;
                screen->VESAModeDepthID = depthid;
            }
        }
        #endif

        if (INVALID_ID == modeid)
        {
            // wtf? no monitors??
            D(bug("!!! OpenScreen(): Could not find any valid modeids. No graphics card? !!!\n"));
            SetError(OSERR_UNKNOWNMODE); /* AROS: Added error core setting, fixed MorphOS bug */
            ok = FALSE;
        }
    }

    DEBUG_OPENSCREEN(dprintf("OpenScreen: ModeID 0x%08lx\n", modeid));

    InitRastPort(&screen->Screen.RastPort);
    rp_inited = TRUE;
    success = FALSE;

    if (ok && (displayinfo = FindDisplayInfo(modeid)) != NULL &&
        GetDisplayInfoData(displayinfo, (APTR)&dimensions, sizeof(dimensions), DTAG_DIMS, modeid)
#ifdef __AROS__ /* AROS: We don't need MonitorSpec, however we get DisplayInfo early to get the compositors bm hooks. */
        && GetDisplayInfoData(displayinfo, (APTR)&dispinfo, sizeof(dispinfo), DTAG_DISP, modeid)
#else 
        && GetDisplayInfoData(displayinfo, (APTR)&monitor, sizeof(monitor), DTAG_MNTR, modeid)
#endif
       )
    {
        success = TRUE;
#ifdef __AROS__ /* AROS: Get HIDD composition flags */
        screen->SpecialFlags = ((compflags & (dimensions.reserved[0] >> 16)) | (dimensions.reserved[0] & 0xFFFF)) << 8;
        screen->preAlphaCompHook = compalphahook;

        if (draggable) screen->SpecialFlags |= SF_Draggable;
#else
        screen->Monitor = monitor.Mspc;
#endif
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

        if (ns.Width == STDSCREENWIDTH || ns.Width == 0 || adaptsize)
            ns.Width = dclip->MaxX - dclip->MinX + 1;
        /* AROS: Added raster size limit support */
        else if (ns.Width < dimensions.MinRasterWidth)
            ns.Width = dimensions.MinRasterWidth;
        else if (ns.Width > dimensions.MaxRasterWidth)
            ns.Width = dimensions.MaxRasterWidth;

        if (ns.Height == STDSCREENHEIGHT || ns.Height == 0 || adaptsize)
            ns.Height = dclip->MaxY - dclip->MinY + 1;
        /* AROS: Added raster size limit support */
        else if (ns.Height < dimensions.MinRasterHeight)
            ns.Height = dimensions.MinRasterHeight;
        else if (ns.Height > dimensions.MaxRasterHeight)
            ns.Height = dimensions.MaxRasterHeight;

#ifndef __AROS__
        DEBUG_OPENSCREEN(dprintf("OpenScreen: Monitor 0x%lx Width %ld Height %ld\n",
                     screen->Monitor, ns.Width, ns.Height));
#endif

        if (ns.Type & CUSTOMBITMAP)
        {
            struct BitMap *custombm;
            custombm = ns.CustomBitMap;
#ifdef __AROS__ /* AROS: We don't have CGX in kickstart */
            /* FIXME: m68k Compositor needs to report that it can composit planar bitmaps */
            BOOL (*__IsCompositable) (struct BitMap *, DisplayInfoHandle, struct GfxBase *) = (APTR)dispinfo.reserved[0];
            BOOL (*__MakeDisplayable) (struct BitMap *, DisplayInfoHandle, struct GfxBase *) = (APTR)dispinfo.reserved[1];

            if (dispinfo.reserved[0])
            {
                if (__IsCompositable(custombm, displayinfo, GfxBase))
                {
                    DEBUG_OPENSCREEN(dprintf("OpenScreen: Marking CustomBitMap 0x%lx as compositable\n", custombm));
                    __MakeDisplayable(custombm, displayinfo, GfxBase);
                }
                else
                {
                    custombm = NULL;
                }
            }
            else if ((!IS_HIDD_BM(custombm)) || (modeid != HIDD_BM_HIDDMODE(custombm)))
            {
                custombm = NULL;
            }
#else
            if (IsCyberModeID(modeid) && custombm)
            {
                int pixfmt = GetCyberIDAttr(CYBRIDATTR_PIXFMT,modeid);
                                                               
                if(GetCyberMapAttr(custombm,CYBRMATTR_PIXFMT) != pixfmt)
                {
                    // incompatible formats !
                    custombm = NULL;
                }
            }
#endif
            if(custombm != NULL)
            {
               screen->Screen.RastPort.BitMap = custombm;
               ns.Depth       =   GetBitMapAttr(ns.CustomBitMap,BMA_DEPTH);
               DEBUG_OPENSCREEN(dprintf("OpenScreen: CustomBitMap Depth %ld\n",
                                        ns.Depth));
            } else {
                ns.CustomBitMap = NULL;
                ns.Type &= ~CUSTOMBITMAP;
            }
        } else {
            screen->Screen.RastPort.BitMap = NULL;            
        }

        if ((screen->IMonitorNode = FindMonitorNode(modeid,IntuitionBase)))
        {
            DEBUG_OPENSCREEN(dprintf("OpenScreen: MonitorNode 0x%lx\n",
                        screen->IMonitorNode));
        } else {
            DEBUG_OPENSCREEN(dprintf("OpenScreen: No MonitorNode\n"));
            ok = FALSE;
        }

        if(ok && (screen->Screen.RastPort.BitMap == NULL))
        {
#ifdef __AROS__ /* AROS: BitMap needs ModeID */
            ULONG Depth = (dimensions.MaxDepth > 8) ? dimensions.MaxDepth : ns.Depth;
            struct TagItem bmtags[] =
            {
                {BMATags_DisplayID, modeid},
                {TAG_DONE         , 0     }
            };

            screen->Screen.RastPort.BitMap = AllocBitMap(ns.Width, ns.Height, Depth,
                                                         allocbitmapflags | BMF_CHECKVALUE,
                                                         (struct BitMap *)bmtags);
#else
            struct BitMap *root = NULL;
            ULONG pixfmt;
            ULONG Depth;
            int retrycnt=1;
            ULONG stdwidth = dimensions.Nominal.MaxX - dimensions.Nominal.MinX + 1;
            ULONG stdheight = dimensions.Nominal.MaxY - dimensions.Nominal.MinY + 1;

            #ifdef USE8BITHACK
            Depth = (dimensions.MaxDepth > 8) ? dimensions.MaxDepth : 8;
            #else
            Depth = (dimensions.MaxDepth > 8) ? dimensions.MaxDepth : ns.Depth;
            #endif

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

            DoMethod((Object*)screen->IMonitorNode,MM_GetRootBitMap,pixfmt,(ULONG)&root);

            DEBUG_OPENSCREEN(dprintf("OpenScreen: root BitMap 0x%lx\n",root));

            do
            {
                allocbitmapflags &= (BMF_DISPLAYABLE | BMF_INTERLEAVED | BMF_3DTARGET);

                if(root == NULL)
                {
                    allocbitmapflags |= (BMF_SPECIALFMT|/*BMF_CLEAR|*/BMF_DISPLAYABLE|BMF_MINPLANES);
                    allocbitmapflags |= SHIFT_PIXFMT(pixfmt);
                }
                else
                {
                    allocbitmapflags |= (/*BMF_CLEAR|*/BMF_DISPLAYABLE|BMF_MINPLANES);
                }
        
                if((screen->Screen.RastPort.BitMap = AllocBitMap((ns.Width > stdwidth) ? ns.Width : stdwidth,
                                 (ns.Height > stdheight) ? ns.Height : stdheight,
                                 Depth,
                                 allocbitmapflags,
                                 root)) == NULL)
                {
                    root = NULL;
                }
            }
            while((screen->Screen.RastPort.BitMap == NULL) && (retrycnt--));
#endif
            screen->AllocatedBitMap = screen->Screen.RastPort.BitMap;

            DEBUG_OPENSCREEN(dprintf("OpenScreen: allocated BitMap 0x%lx\n",screen->AllocatedBitMap));

            if (screen->Screen.RastPort.BitMap)
            {
                UpdateScreenBitMap(&screen->Screen, IntuitionBase);
#ifndef __AROS__
                /*
                 * AROS: This seems to be not needed
                 * TODO: Check if this is really true, test case needed
                 */
                if (Depth > 8)
                {
                    if (stdwidth > ns.Width)
                    {
                        struct RastPort rport;

                        InitRastPort(&rport);
                        rport.BitMap = screen->AllocatedBitMap;
                        SetRPAttrs(&rport,RPTAG_PenMode,FALSE,RPTAG_FgColor,0x0,TAG_DONE);

                        RectFill(&rport,ns.Width,0,stdwidth - 1,ns.Height - 1);
                    }

                    if (stdheight > ns.Height)
                    {
                        struct RastPort rport;

                        InitRastPort(&rport);
                        rport.BitMap = screen->AllocatedBitMap;
                        SetRPAttrs(&rport,RPTAG_PenMode,FALSE,RPTAG_FgColor,0x0,TAG_DONE);

                        RectFill(&rport,0,ns.Height,ns.Width - 1,stdheight - 1);
                    }
                }
#endif
            }
        }

        DEBUG_OPENSCREEN(dprintf("OpenScreen: BitMap 0x%lx\n",
                     screen->Screen.RastPort.BitMap));
    }
    else
    {
        DEBUG_OPENSCREEN(dprintf("OpenScreen: no displayinfo\n"));
        SetError(OSERR_UNKNOWNMODE); /* AROS: Added error code setting, fixed MorphOS bug */
    }

    D(bug("got BitMap\n"));

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
    } else {
        /* Store pointer to BitMap, so we can get hold of it
           from withing LoadRGBxx() functions
        */
        D(bug("got allocated stuff\n"));
        screen->Screen.ViewPort.RasInfo->BitMap = screen->Screen.RastPort.BitMap;
    }

    if (ok)
    {
        /* Read depth from the BitMap to avoid AttachPalExtra/ObtainPen getting
         * confused if cgx decided to allocate a higher depth BitMap than what
         * was asked.
         */
        ns.Depth = GetBitMapAttr(screen->Screen.RastPort.BitMap,BMA_DEPTH);

        if (!numcolors) /* AROS: Added support for SA_ColorMapEntries */
        {
#ifdef USE8BITHACK
            numcolors = 256;
#else
            numcolors = (ns.Depth <= 8) ? (1L << ns.Depth) : 256;
#endif
        }

        /* Get a color map structure. Sufficient colors?? */

        DEBUG_OPENSCREEN(dprintf("OpenScreen: Colormap Entries %ld\n",
                     numcolors));
        if ((screen->Screen.ViewPort.ColorMap = GetColorMap(numcolors < 32 ? 32 : numcolors)) != NULL)
        {
            if (0 == AttachPalExtra(screen->Screen.ViewPort.ColorMap,
                        &screen->Screen.ViewPort))
            {
#if 0
                int i=0;
                char    *alloclist;
                UWORD    *refcnt;

                refcnt    =(UWORD*) screen->Screen.ViewPort.ColorMap->PalExtra->pe_RefCnt;
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

    {
        ULONG pixfmt = -1;
        APTR handle = LockBitMapTags(screen->Screen.RastPort.BitMap,
            LBMI_PIXFMT,(ULONG)&pixfmt,TAG_DONE);

        if (handle)
        {
            UnLockBitMap(handle);
            screen->PixelFormat = pixfmt;
        }
        else
        {
            screen->PixelFormat = -1;
        }
    }
#endif
    screen->GammaControl.UseGammaControl = gammacontrol;
    screen->GammaControl.GammaTableR = gammared;
    screen->GammaControl.GammaTableG = gammagreen;
    screen->GammaControl.GammaTableB = gammablue;
#ifndef __AROS__
    /*
     * AROS: This fragment seems to delay-load default gamma table for the monitor.
     * TODO: May be use this approach ?
     */
    if (screen->IMonitorNode && !screen->IMonitorNode->GammaLoaded)
    {
        int_SkinAction(SKA_LoadGamma,(ULONG *)screen->IMonitorNode,NULL,IntuitionBase);
        screen->IMonitorNode->GammaLoaded = TRUE;
    }

    // no hotkey by default
    screen->RecallHotkey.ia_Key = 0xFF;
#endif
    screen->ModeID = modeid;

    if (ok)
    {
        struct ViewPortExtra *vpe = (struct ViewPortExtra *)GfxNew(VIEWPORT_EXTRA_TYPE);

        DEBUG_OPENSCREEN(dprintf("OpenScreen: ViewPortExtra 0x%lx\n", vpe));

        ok = FALSE;

        if (vpe)
        {
            struct TagItem tags[6];

            memcpy(&vpe->DisplayClip, dclip,sizeof(struct Rectangle));
#ifdef __AROS__
            /*
             * AROS: Set ViewPort offset and get size from dclip.
             * CHECKME: MorphOS source code always sets ViewPort's DWidth and DHeight to
             * nominal size, while ViewPortExtra's DisplayClip is still set to OverScan
             * rectangle. Whose approach is correct ?
             */
            screen->Screen.ViewPort.DxOffset = ns.LeftEdge;
            screen->Screen.ViewPort.DyOffset = ns.TopEdge;
            screen->Screen.ViewPort.DWidth   = dclip->MaxX - dclip->MinX + 1;
            screen->Screen.ViewPort.DHeight  = dclip->MaxY - dclip->MinY + 1;
#else
            /* using vpe data for vesamode/modecontrol is not a good idea since the mode
            is changed on the fly and the actual data might not be what we'll use */

            if (screen->VESAMode)
            {
                screen->Screen.ViewPort.DWidth = screen->VESAMode;

                switch (screen->VESAMode)
                {
                    case 800:
                        screen->Screen.ViewPort.DHeight = 600;
                        break;

                    case 1024:
                        screen->Screen.ViewPort.DHeight = 768;
                        break;

                    default:
                        screen->Screen.ViewPort.DWidth = 640;
                        screen->Screen.ViewPort.DHeight = 480;
                }

                screen->VESAMode = ns.Width;
            } else if (screen->ModeControl) {
                screen->Screen.ViewPort.DWidth = ns.Width;
                screen->Screen.ViewPort.DHeight = ns.Height;
            } else {
                ULONG stdwidth = dimensions.Nominal.MaxX - dimensions.Nominal.MinX + 1;
                ULONG stdheight = dimensions.Nominal.MaxY - dimensions.Nominal.MinY + 1;

                screen->Screen.ViewPort.DWidth = min(ns.Width,stdwidth);
                screen->Screen.ViewPort.DHeight = min(ns.Height,stdheight);
            }
#endif
            tags[0].ti_Tag  = VTAG_ATTACH_CM_SET;
            tags[0].ti_Data = (IPTR)&screen->Screen.ViewPort;
            tags[1].ti_Tag  = VTAG_VIEWPORTEXTRA_SET;
            tags[1].ti_Data = (IPTR)vpe;
            tags[2].ti_Tag  = VTAG_NORMAL_DISP_SET;
            tags[2].ti_Data = (IPTR)displayinfo;
            tags[3].ti_Tag  = VTAG_VPMODEID_SET;
            tags[3].ti_Data = modeid;
            tags[4].ti_Tag  = VTAG_SPEVEN_BASE_SET; /* AROS: Added sprite base and user-supplied tags */
            tags[5].ti_Tag  = VTAG_NEXTBUF_CM; /* if vctl is 0, this will terminate the list */
            tags[5].ti_Data = vctl;

            /*
             * Originally we could always use palette entries 16-19 for
             * sprites, even if the screen has less than 32 colors. AROS may
             * run on hardware that does not allow this (e.g. VGA cards).
             * In this case we have to shift down sprite colors. Currently
             * we use 4 colors before last 4 colors. For example on VGA cards
             * with only 16 colors we use colors 9 - 12. Remember that last 4 colors
             * of the screen are always used by Intuition.
             * Remember that the first color of the sprite is always transparent. So actually
             * we use 3, not 4 colors.
             * Yes, sprites may look not as expected on screens with low color depth, but at
             * least they will be seen. It's better than nothing.
             *
             * Note that because our base color number doesn't always divide by 16, we use MSB to store
             * the remainder (offset in the color bank). Yes, it's a bit hacky, but i have no better idea
             * at the moment.
             *
             * FIXME: this mapping scheme assumes that we always have at least 16 colors.
             * For current display modes supported by AROS it's always true, but in future
             * we may support more display modes (for example monochrome ones), and we
             * should take into account that on screens with less than 11 colors this simply
             * won't work
             *
             * TODO: I think we should have SpriteBase attribute for the BitMap which
             * defaults to acceptable value. We should just get its default value here.
             * The same attribute would be set by VideoControl() and MakeVPort() in order
             * to actually apply the value.
             */
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
        int k;
        UWORD *q;

        DEBUG_OPENSCREEN(dprintf("OpenScreen: Set first 4 colors\n"));

        p = IBase->Colors;
        for (k = 0; k < 4 && k < numcolors; ++k)
        {
            DEBUG_OPENSCREEN(dprintf("OpenScreen: SetRGB32 Viewport 0x%lx Index %ld R 0x%lx G 0x%lx B 0x%lx\n",
                         screen->Screen.ViewPort,
                         k, p[k].red, p[k].green, p[k].blue));
            SetRGB32(&screen->Screen.ViewPort, k, p[k].red, p[k].green, p[k].blue);
        }

        if (ns.Depth >= 3)
        {
            /*
             * AROS: Use screen depth instead of 'numcolors' in order to calculate 
             * numbers of last 4 colors of the screen.
             * This is necessary because we can have 8- or 16- color screen whose
             * ColorMap will still have 32 colors for AmigaOS(tm) compatibility
             * reasons.
             */
            ULONG lastcol = ((ns.Depth > 8) ? 256 : (1 << ns.Depth)) - 4;

            DEBUG_OPENSCREEN(dprintf("OpenScreen: Set last 4 colors\n"));

            for (k = 0; k < 4; ++k)
            {
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SetRGB32 Viewport 0x%lx Index %ld R 0x%lx G 0x%lx B 0x%lx\n",
                             screen->Screen.ViewPort,
                             k + lastcol, p[k+4].red, p[k+4].green, p[k+4].blue));

                if (k + lastcol < numcolors)
                {
                    ObtainPen(screen->Screen.ViewPort.ColorMap,
                              k + lastcol,
                              p[k+4].red,
                              p[k+4].green,
                              p[k+4].blue,
                              PEN_EXCLUSIVE);
                }
                else
                {
                    SetRGB32(&screen->Screen.ViewPort, k + lastcol, p[k+4].red, p[k+4].green, p[k+4].blue);
                }
            }
        }

        /*
         * AROS: AmigaOS(tm) 3.1-compatible mouse colors handling
         * 1. Allocate pens for the mouse pointer only on LUT screens.
         *    On hi- and truecolor screens sprite colors come from colormap attached
         *    to the sprite BitMap itself. See pointerclass::New() for details.
         * 2. Use 32-bit pointer colors instead of KS1.3 ActivePreferences->color17
         */
        if (ns.Depth < 9)
        {
            DEBUG_OPENSCREEN(dprintf("OpenScreen: Obtain Mousepointer colors\n"));

            for (k = 1; k < 4; ++k, ++q)
            {
                DEBUG_OPENSCREEN(dprintf("OpenScreen: ColorMap 0x%P Pen %d R 0x%08X G 0x08X B 0x%08X\n",
                                         screen->Screen.ViewPort.ColorMap,
                                         k + spritebase,
                                         p[k+7].red, p[k+7].green, p[k+7].blue));
                if (k + spritebase < numcolors)
                {
                    ObtainPen(screen->Screen.ViewPort.ColorMap,
                              k + spritebase,
                              p[k+7].red, p[k+7].green, p[k+7].blue,
                              PEN_EXCLUSIVE);
                }
                else
                {
                    /* Can't be allocated, but can still be set. */
                    SetRGB32(&screen->Screen.ViewPort,
                             k + spritebase,
                             p[k+7].red, p[k+7].green, p[k+7].blue);
                }
            }
        }

        if (colors)  /* if SA_Colors tag exists */
        {
            DEBUG_OPENSCREEN(dprintf("OpenScreen: set SA_Colors 0x%lx\n",colors));
            for(; colors->ColorIndex != (WORD)~0; colors++)
            {
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SetRGB4 Viewport 0x%lx Index %ld R 0x%lx G 0x%lx B 0x%lx\n",
                             screen->Screen.ViewPort,
                             colors,
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

        COPY(LeftEdge);
        COPY(TopEdge);
        COPY(Width);
        COPY(Height);
        COPY(DetailPen);
        COPY(BlockPen);
        COPY(Font);
        COPY(DefaultTitle);

        //intui68k filters this
        screen->Screen.Flags = (ns.Type & ~NS_EXTENDED);

        /* Temporary hack */
        if (ns.Width >= 500 || ns.Height >= 300)
            screen->Screen.Flags |= SCREENHIRES;

        /*
           Copy the data from the rastport's BitMap
           to the screen's BitMap structure
        */
        UpdateScreenBitMap(&screen->Screen, IntuitionBase);

#ifdef __MORPHOS__
        screen->Screen.WBorTop    = 2;
        screen->Screen.WBorLeft   = 4;
        screen->Screen.WBorRight  = 4;
        screen->Screen.WBorBottom = 2;
#else
        screen->Screen.WBorTop    = 6;  /* Amiga default is 2 */
        screen->Screen.WBorLeft   = 4;
        screen->Screen.WBorRight  = 4;
        screen->Screen.WBorBottom = 4;  /* Amiga default is 2 */
#endif

        screen->Screen.Title = ns.DefaultTitle;

        DEBUG_OPENSCREEN(dprintf("OpenScreen: init layers\n"));
#ifdef __AROS__ /* AROS: We have no compositing layers */
        InitLayers(&screen->Screen.LayerInfo);
#else
        if (((struct Library *)LayersBase)->lib_Version >= 52 && compositing &&
            screen->IMonitorNode->Compositing)
        {
            struct TagItem layerinfotags[] =
            {
                {SA_Width,screen->Screen.Width},
                {SA_Height,screen->Screen.Height},
                {SA_PixelFormat,screen->PixelFormat},
                {MA_MemorySize,screen->IMonitorNode->BoardMemory},
                {LT_IgnoreMemoryChecks, IBase->LayerSettings.IgnoreMemoryChecks},
                {SA_CompositingLayers, TRUE},
                {TAG_DONE}
            };
            ULONG iscompositing = FALSE;

            // make SA_CompositingLayers return TRUE (layers will ask)
            screen->Compositing = TRUE;
            InitScreenLayerInfo(&screen->Screen,layerinfotags);

            // validate if we're compositing
            LayersControlTags(&screen->Screen.LayerInfo,
                LT_GetCompositing,(ULONG)&iscompositing,

                // also set current layers settings
                LT_AllowDoubleBuffered, IBase->LayerSettings.AllowDoubleBuffered,
                LT_AllowTripleBuffered, IBase->LayerSettings.AllowTripleBuffered,
                LT_IgnoreMemoryChecks, IBase->LayerSettings.IgnoreMemoryChecks,
                LT_LoadBalancing, IBase->LayerSettings.LoadBalancing,

                TAG_DONE);

            screen->Compositing = iscompositing ? TRUE : FALSE;
        }
        else
        {
            struct TagItem layerinfotags[] =
            {
                {SA_CompositingLayers, FALSE},
                {TAG_DONE}
            };
            screen->Compositing = FALSE;
            InitScreenLayerInfo(&screen->Screen,layerinfotags);
        }
#endif
        li_inited = TRUE;

        if (NULL != layer_info_hook)
        {
            DEBUG_OPENSCREEN(dprintf("OpenScreen: install layerinfohook\n"));
            InstallLayerInfoHook(&screen->Screen.LayerInfo, layer_info_hook);
        }
        D(bug("layers inited screen\n"));

        /* AROS: We have already obtained resolution data from DisplayInfo */
#ifdef __AROS__
        screen->DInfo.dri_Version = DRI_VERSION;
#else
        GetDisplayInfoData(NULL, (APTR)&dispinfo, sizeof(dispinfo), DTAG_DISP, modeid);
        screen->DInfo.dri_Version = MOS_DRI_VERSION;
#endif
        screen->DInfo.dri_NumPens = NUMDRIPENS;
        screen->DInfo.dri_Pens = screen->Pens;
        /* dri_Depth is 8 on hi/true color screens like in AmigaOS with picasso96/cybergraphx */
        screen->DInfo.dri_Depth = (ns.Depth <= 8) ? ns.Depth : 8;
        /* AROS: Get resolution from DisplayInfo */
        screen->DInfo.dri_Resolution.X = dispinfo.Resolution.x;
        screen->DInfo.dri_Resolution.Y = dispinfo.Resolution.y;
        screen->DInfo.dri_Flags = 0;


        /* SA_SysFont overrides SA_Font! */

        DEBUG_OPENSCREEN(dprintf("OpenScreen: SysFont = %d, ns.Font = %p\n", sysfont, ns.Font));

        if (sysfont == 0)
        {
            /* Is handled below */
            DEBUG_OPENSCREEN(dprintf("OpenScreen: skip SysFont for now\n"));
        } else if (sysfont == 1) {

            screen->DInfo.dri_Font = SafeReopenFont(IntuitionBase, &IBase->ScreenFont);
            
            if (screen->DInfo.dri_Font)
            {
                screen->SysFont = TRUE;
            }
            DEBUG_OPENSCREEN(dprintf("OpenScreen: Set ScreenFont\n"));

        } else if (ns.Font) {
            screen->DInfo.dri_Font = OpenFont(ns.Font);
            DEBUG_OPENSCREEN(dprintf("OpenScreen: custom font 0x%lx\n",screen->DInfo.dri_Font));
        }

        if (!screen->DInfo.dri_Font)
        {
            /* GfxBase->DefaultFont is *not* always topaz 8. It
               can be set with the Font prefs program!! */

            screen->DInfo.dri_Font = SafeReopenFont(IntuitionBase, &GfxBase->DefaultFont);
        }

        if (!screen->DInfo.dri_Font) ok = FALSE;

    } /* if (ok) */

    if (ok)
    {
        ULONG *dripens = NULL;

        switch (ns.Depth)
        {
            case 1:
                dripens = (ULONG*)&IBase->DriPens2;
                break;

            case 2:
                dripens = (ULONG*)&IBase->DriPens4;
                break;

            default:
                if (colors || colors32)
                {
                    /* jDc: apps are used to use non-std colors for pen # >=4
                    dripens4 will be a good fallback for this */
                    dripens = (ULONG*)&IBase->DriPens4;
                } else {
                    dripens = (ULONG*)&IBase->DriPens8;
                }
                break;
        }

        /* set default values for pens */
        DEBUG_OPENSCREEN(dprintf("OpenScreen: Set Default Pens\n"));
        CopyMem(dripens,screen->Pens,sizeof(screen->Pens));

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
            
            for(i = 0; (i < NUMDRIPENS) && (customdripens[i] != (UWORD)~0) && (screen->Pens[i] != (UWORD)~0); i++)
            {
                DEBUG_OPENSCREEN(dprintf("OpenScreen: Pen[%ld] %ld --> %ld\n",i,screen->Pens[i],customdripens[i]));
                screen->Pens[i] = customdripens[i];
            }
        }

        /*
         * Let's do some broken software validation of the pens
         * so we may not run into a black desktop.
         */

        DEBUG_OPENSCREEN(dprintf("OpenScreen: Check Default Pens if the make sense\n"));
        if (screen->Screen.DetailPen == screen->Screen.BlockPen)
        {
            DEBUG_OPENSCREEN(dprintf("OpenScreen: DetailPen==BlockPen..correct\n"));
            screen->Screen.DetailPen = 0;
            screen->Screen.BlockPen = 1;
        } else if (screen->Screen.BlockPen == 0)
        {
            DEBUG_OPENSCREEN(dprintf("OpenScreen: BlockPen==0..correct\n"));
            screen->Screen.BlockPen = screen->Screen.DetailPen;
            screen->Screen.DetailPen = 0;
        }
            
        screen->Pens[DETAILPEN] = screen->Screen.DetailPen;
        screen->Pens[BLOCKPEN] = screen->Screen.BlockPen;

        /* Allocate shared/exclusive colors */

        {
            BYTE color_alloced[256];
            UWORD mask;
            WORD i;

            /*for(i = 0; i < 256; i++)
            {
                color_alloced[i] = FALSE;
            }*/
            memclr(&color_alloced,256);

            /* Mouse pointer colors */

            color_alloced[17] = TRUE;
            color_alloced[18] = TRUE;
            color_alloced[19] = TRUE;

            /* Clamp the pen numbers. This makes sure that values such as -4 or -3
               won't result in random memory trashing, but rather index colours from
               the end of the shareable palette. - Piru */
            mask = screen->Screen.ViewPort.ColorMap->PalExtra->pe_SharableColors & 0xff;

            /* The Pens in the DrawInfo must be allocated as shared */

            DEBUG_OPENSCREEN(dprintf("OpenScreen: ObtainPen DrawInfo Pens as shared\n"));
            for(i = 0; (i < NUMDRIPENS) && (screen->Pens[i] != (UWORD)~0); i++)
            {
                int pen = screen->Pens[i] & mask;
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
                ULONG shnumcolors = (requesteddepth <= 8) ? (1L << requesteddepth) : 256;

                /* jDc: hack, colors above requested screen depth are not locked! */

                DEBUG_OPENSCREEN(dprintf("OpenScreen: ObtainPen the remaining Pens as exclusive\n"));
                for(i = 0; i < shnumcolors; i++)
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
        screen->DInfo.dri_Screen = &screen->Screen; //useful sometimes ;)

        screen->realdepth = GetBitMapAttr( screen->Screen.RastPort.BitMap, BMA_DEPTH );

        if (screen->realdepth > 8)
        {
            screen->DInfo.dri_Flags |= DRIF_DIRECTCOLOR;
        } else {
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
            DEBUG_OPENSCREEN(dprintf("OpenScreen: allocating dri_Customize\n"));

            if ((screen->DInfo.dri_Customize = AllocMem(sizeof (struct IntuitionCustomize),MEMF_PUBLIC|MEMF_CLEAR)))
            {
#ifdef SKINS
                struct IntuitionCustomize *ic;
                ic = screen->DInfo.dri_Customize;
                screen->DInfo.dri_Flags |= DRIF_SKINSSUPPORT;

                /* This initializes CustomizePrefs structure */

                if (skinname) strcpy(ic->skinname,skinname);

                InitSemaphore(&ic->InitLock);

                DEBUG_OPENSCREEN(dprintf("OpenScreen: load skin %s for screen %lx\n",ic->skinname,screen));
                int_SkinAction(SKA_LoadSkin,(ULONG*)&screen->DInfo,(struct Screen *)screen,IntuitionBase);
#else
                /* AROS: Use less error-prone skinname handling */
                ok = int_LoadDecorator(skinname, screen, IntuitionBase);
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
        screen->Screen.WBorTop   += wcprefs->titlebarincrement;
        int_FreeCustomPrefs(TYPE_WINDOWCLASS,&screen->DInfo,IntuitionBase);
    }

#endif

    if (ok)
    {
        struct TagItem sysi_tags[] =
        {
            {SYSIA_Which,      MENUCHECK          },
            {SYSIA_DrawInfo,   (IPTR)&screen->DInfo   },
            {TAG_DONE                 }
        };

        screen->DInfo.dri_CheckMark = NewObjectA(NULL, "sysiclass", sysi_tags);
        DEBUG_OPENSCREEN(dprintf("OpenScreen: CheckMark 0x%lx\n",
                     screen->DInfo.dri_CheckMark));

        sysi_tags[0].ti_Data = AMIGAKEY;

        screen->DInfo.dri_AmigaKey  = NewObjectA(NULL, "sysiclass", sysi_tags);
        DEBUG_OPENSCREEN(dprintf("OpenScreen: AmigaKey 0x%lx\n",
                     screen->DInfo.dri_AmigaKey));

        /* AROS: We also use submenu image */
        sysi_tags[0].ti_Data = SUBMENUIMAGE;
        screen->DInfo.dri_Customize->submenu  = NewObjectA(NULL, "sysiclass", sysi_tags);
        if (!screen->DInfo.dri_Customize->submenu) ok =FALSE;
#ifdef SKINS
        sysi_tags[0].ti_Data = MENUTOGGLEIMAGE;
        screen->DInfo.dri_Customize->menutoggle  = NewObjectA(NULL, "sysiclass", sysi_tags);
        if (!screen->DInfo.dri_Customize->menutoggle) ok = FALSE;
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

        screen->Screen.BarVBorder  = 1;
        screen->Screen.BarHBorder  = 5;
        screen->Screen.MenuVBorder = 2;
        screen->Screen.MenuHBorder = 4;

        screen->Screen.BarHeight   = screen->DInfo.dri_Font->tf_YSize + screen->Screen.WBorTop-2 +
                         screen->Screen.BarVBorder * 2; /* real layer will be 1 pixel higher! */

        /*
         * AROS: Get size information from decorator.
         * This has to be done before we create gadgets because here we may adjust BarHeight.
         */
        ok = int_InitDecorator(&screen->Screen);
        if (ok)
        {
#define SDEPTH_HEIGHT (screen->Screen.BarHeight + 1)
#ifdef IA_Screen
#undef IA_Screen
#endif
#define IA_Screen    (IA_Dummy + 0x1f) /* OS v44 includes!*/

            struct TagItem sdepth_tags[] =
            {
                {GA_Image,     0            },
                {GA_Top,       0            },
                {GA_Height,    SDEPTH_HEIGHT},
                {GA_SysGadget, TRUE         },
                {GA_SysGType,  GTYP_SDEPTH  },
                {GA_RelVerify, TRUE         },
                {TAG_DONE,     0UL          }
            };

            struct TagItem image_tags[] =
            {
                {IA_Left         , 0                   },
                //{IA_Width        , SDEPTH_WIDTH + 1    },
                {IA_Height       , SDEPTH_HEIGHT       },
                {SYSIA_Which     , SDEPTHIMAGE         },
                {SYSIA_DrawInfo  , (IPTR)&screen->DInfo},
                {SYSIA_Size      , screen->Screen.Flags & SCREENHIRES ?
                                   SYSISIZE_MEDRES : SYSISIZE_LOWRES},
                {TAG_DONE                              }
            };

            struct Object *im = 0;

            if (!(screen->Screen.Flags & SCREENQUIET))
            {
                im = NewObjectA(NULL, SYSICLASS, image_tags);
            }

            sdepth_tags[0].ti_Data = (IPTR)im;

            screen->depthgadget = NewObjectA(IBase->windowsysiclass, NULL, sdepth_tags );

            DEBUG_OPENSCREEN(dprintf("OpenScreen: DepthGadget 0x%lx\n",
                         screen->depthgadget));

            screen->Screen.FirstGadget = (struct Gadget *)screen->depthgadget;
            if (screen->Screen.FirstGadget)
            {
                struct TagItem gadtags[] =
                {
                        {GA_RelRight, 0},
                        {TAG_DONE  }
                };
                IPTR width; /* AROS: Changed from int to IPTR, 64-bit fix */

                GetAttr(GA_Width, screen->depthgadget, &width);

                gadtags[0].ti_Data = -width + 1;
                SetAttrsA(screen->depthgadget, gadtags);
                screen->Screen.FirstGadget->GadgetType |= GTYP_SCRGADGET;
            } else {
                if (im) DisposeObject(im);
            }

#if DEBUG
            {
                int i;
                for (i=0;i<=screen->DInfo.dri_NumPens;i++)
                {
                    DEBUG_OPENSCREEN(dprintf("OpenScreen: dri_Pen[%ld] = %ld\n",i,screen->DInfo.dri_Pens[i]));
                }
            }
#endif

#ifdef USEWINDOWLOCK
            /* let's wait for user to finish window drag/size actions to avoid
            deadlocks and not break user's input */
            if (dowindowlock && (!(FindTask(NULL) == ((struct IIHData *)IBase->InputHandler->is_Data)->InputDeviceTask)))
            {
                LOCKWINDOW;
                windowlock = TRUE;
                screen->WindowLock = TRUE;
            }
#endif

            int_CalcSkinInfo(&screen->Screen,IntuitionBase);
#ifdef SKINS
            int_InitTitlebarBuffer(&screen->Screen,IntuitionBase);
#endif
            D(bug("calling SetRast()\n"));

            DEBUG_OPENSCREEN(dprintf("OpenScreen: Set background color Pen %ld\n",screen->Pens[BACKGROUNDPEN]));
            /* Set screen to background color */
            SetRast(&screen->Screen.RastPort, screen->Pens[BACKGROUNDPEN]);

            D(bug("SetRast() called\n"));

            DEBUG_OPENSCREEN(dprintf("OpenScreen: Creating screen bar\n"));

#ifdef SKINS
            if (workbench)
            {
#if 1
                if (IBase->IControlPrefs.ic_Flags & (ICF_DISAPPEARINGTITLEBAR | ICF_NOWBTITLEBAR)) screen->SpecialFlags |= SF_AppearingBar;
#else
                if (IBase->IControlPrefs.ic_Flags & ICF_NOWBTITLEBAR) screen->SpecialFlags |= SF_InvisibleBar;
                if (IBase->IControlPrefs.ic_Flags & ICF_DISAPPEARINGTITLEBAR) screen->SpecialFlags |= SF_AppearingBar;
#endif
            }
#endif

            //jDc: ALL screens MUST have BarLayer!
            CreateScreenBar(&screen->Screen, IntuitionBase);

            if (!screen->Screen.BarLayer) ok = FALSE;
        }

        if (ok)
        {
            if ((ns.Type & SCREENTYPE) == CUSTOMSCREEN)
            {
                screen->ShowPointer = showpointer;
            } else {
                screen->ShowPointer = TRUE;
            }
#ifdef SKINS
            GetAttr(POINTERA_SharedPointer, screen->IMonitorNode->Pointers[POINTERTYPE_INVISIBLE], (IPTR *)&screen->Pointer);
#else
            GetAttr(POINTERA_SharedPointer, IBase->DefaultPointer, (IPTR *)&screen->Pointer);
#endif
            ObtainSharedPointer(screen->Pointer, IntuitionBase);
            DEBUG_OPENSCREEN(dprintf("OpenScreen: Sprite DefaultPtr 0x%lx\n",&screen->Pointer));
        }

        /*
        ** jDc: better modify the screen list in sync with inputhandler, this for example allows us to scan the list
        ** without any locks when we are on input.device context
        */
        if (ok)
        {
            struct PubScreenNode *ps = NULL;
            struct List *list = LockPubScreenList();

            /* Additional dupe check, just to be sure */
            if (!workbench && screen->pubScrNode && (ps = (struct PubScreenNode *)FindName(list,screen->pubScrNode->psn_Node.ln_Name)))
            {
                ok = FALSE;
            }

            if (ok)
            {
                struct OpenScreenActionMsg msg;

                msg.Screen = screen;
                msg.NewScreen = &ns;
                msg.List = list;

                DoSyncAction((APTR)int_openscreen,&msg.msg,IntuitionBase);

                ok = msg.Success;
            }

            UnlockPubScreenList();
        }

    } /* if (ok) */

#ifdef USEWINDOWLOCK
    if (windowlock) UNLOCKWINDOW;
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
#ifdef __MORPHOS__
            struct TagItem tags[2];

            tags[0].ti_Tag = VTAG_ATTACH_CM_GET;
            tags[0].ti_Data = 0;
            tags[1].ti_Tag = VTAG_END_CM;

            if (VideoControl(screen->Screen.ViewPort.ColorMap, tags) == 0 &&
                tags[0].ti_Data)
            {
                GfxFree((APTR)tags[0].ti_Data);
            }
#endif

            FreeColorMap(screen->Screen.ViewPort.ColorMap);
        }

        if (screen->Screen.BarLayer)
        {
            DEBUG_OPENSCREEN(dprintf("OpenScreen: KillScreenBar\n"));
            KillScreenBar(&screen->Screen, IntuitionBase);
        }

        if (screen->DInfo.dri_Customize)
        {
            /* AROS: submenu image handling moved out of #ifdef */
            DisposeObject(screen->DInfo.dri_Customize->submenu);
#ifdef SKINS
            DisposeObject(screen->DInfo.dri_Customize->menutoggle);
#endif
            FreeMem(screen->DInfo.dri_Customize,sizeof (struct IntuitionCustomize));
        }
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

        if (screen->AllocatedBitMap && !(ns.Type & CUSTOMBITMAP))
        {
            DEBUG_OPENSCREEN(dprintf("OpenScreen: Free BitMap\n"));
            FreeBitMap(screen->AllocatedBitMap);
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

        DEBUG_OPENSCREEN(dprintf("OpenScreen: Free Screen\n"));
        DisposeObject((APTR)screen);
        screen = 0;

    } /* if (!ok) */

    DEBUG_OPENSCREEN(dprintf("OpenScreen: return 0x%lx\n", screen));

    FireScreenNotifyMessage((IPTR) screen, SNOTIFY_AFTER_OPENSCREEN, IntuitionBase);

    ReturnPtr ("OpenScreen", struct Screen *, (struct Screen *)screen);

    AROS_LIBFUNC_EXIT

} /* OpenScreen */

VOID int_openscreen(struct OpenScreenActionMsg *msg,
                            struct IntuitionBase *IntuitionBase)
{
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct IntScreen *screen = msg->Screen;
    struct NewScreen *ns = msg->NewScreen;
    struct List *list = msg->List;
    struct Screen *oldFirstScreen;
    struct RethinkDisplayActionMsg rmsg;
    ULONG lock = 0;

    DEBUG_OPENSCREEN(dprintf("OpenScreen: Checking for pubScrNode (0x%lx)\n",screen->pubScrNode));

    /* If this is a public screen, we link it into the intuition global
       public screen list */
    if (screen->pubScrNode != NULL)
    {
        /* Set the pointer to ourselves */
        IS(screen)->pubScrNode->psn_Screen = &screen->Screen;

        DEBUG_OPENSCREEN(dprintf("OpenScreen: Add Screen to PubList\n"));
        AddTail(list, (struct Node *)IS(screen)->pubScrNode);
    }

    if (!ILOCKCHECK(((struct IntuiActionMsg *)msg))) lock = LockIBase((IPTR)NULL);

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

    /* AROS: If it's the first screen being opened, activate its monitor */
    if (!oldFirstScreen)
	ActivateMonitor(screen->IMonitorNode, -1, -1, IntuitionBase);

    /* set the default pub screen */
    if (IBase->IControlPrefs.ic_Flags & ICF_DEFPUBSCREEN)
    {
        if ((IntuitionBase->FirstScreen == &screen->Screen) && screen->pubScrNode && (screen->Screen.Flags & (PUBLICSCREEN | WBENCHSCREEN)))
        {
            IBase->DefaultPubScreen = &screen->Screen;
        }
    }

    msg->Success = MakeVPort(&IntuitionBase->ViewLord, &screen->Screen.ViewPort) ? FALSE : TRUE;

    if (msg->Success)
    {
        /* AROS: Put offsets validated by MakeVPort() back into screen structure */
        screen->Screen.LeftEdge = screen->Screen.ViewPort.DxOffset;
        screen->Screen.TopEdge = screen->Screen.ViewPort.DyOffset;

        rmsg.lock = FALSE;
        int_RethinkDisplay(&rmsg,IntuitionBase);
    }

#ifdef SKINS
    if (!(ns->Type & SCREENBEHIND))
    {
        // workaround: do not send the event for SCREENBEHIND cause it'd reset the DPMS counter
        QueryBlankerEvent(BLANKEREVENT_SCREENDEPTH,(ULONG)screen->IMonitorNode,IntuitionBase);
    }
#endif

    if (!ILOCKCHECK(((struct IntuiActionMsg *)msg))) UnlockIBase(lock);

    D(bug("set active screen\n"));

    AddResourceToList(screen, RESOURCE_SCREEN, IntuitionBase);
}


