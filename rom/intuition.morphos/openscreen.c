/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
 
    Open a new screen.
*/

#include <exec/memory.h>
#include <utility/tagitem.h>
#include <intuition/screens.h>
#include <intuition/intuition.h>
#include <intuition/imageclass.h>
#include <intuition/gadgetclass.h>
#include <graphics/modeid.h>
#include <graphics/videocontrol.h>
#include <graphics/displayinfo.h>
#include <prefs/screenmode.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <string.h>
#ifdef __MORPHOS__
#include <proto/cybergraphics.h>
#include <cybergraphx/cybergraphics.h>
#endif
#include "intuition_intern.h"
#include "intuition_customize.h"
#include "intuition_extend.h"
#include "inputhandler_support.h"
#include "inputhandler_actions.h"
#include "menus.h"

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

#ifdef __MORPHOS__
extern const ULONG defaultdricolors[DRIPEN_NUMDRIPENS];
#else
//FIXME: NUMDRIPENS == DRIPEN_NUMDRIPENS ???
extern const ULONG defaultdricolors[NUMDRIPENS];
#endif

#if DEBUG
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif


#if 0
#define DEBUG_OBTAINPEN(x)      x;

/******* graphics.library/ObtainPen **********************************************
*
*   NAME
*   ObtainPen -- Obtain a free palette entry for use by your program. (V39)
*
*
*   SYNOPSIS
*   n = ObtainPen( cm, n, r, g, b, flags)
*   d0         a0 d0  d1 d2 d3  d4
*
*   LONG ObtainPen(struct ColorMap *,ULONG,ULONG,ULONG,ULONG,ULONG);
*
*   FUNCTION
*   Attempt to allocate an entry in the colormap for use by the application.
*   If successful, you should ReleasePen() this entry after you have finished
*   with it.
*
*   Applications needing exclusive use of a color register (say for color
*   cycling) will typically call this function with n=-1. Applications needing
*   only the shared use of a color will typically use ObtainBestPenA() instead.
*   Other uses of this function are rare.
*
*   INPUTS
*   cm  =  A pointer to a color map created by GetColorMap().
*   n   =  The index of the desired entry, or -1 if any one is acceptable
*   rgb =  The RGB values (32 bit left justified fractions) to set the new
*          palette entry to.
*   flags= PEN_EXCLUSIVE - tells the system that you want exclusive
*          (non-shared) use of this pen value. Default is shared access.
*
*          PEN_NO_SETCOLOR - tells the system to not change the rgb values
*          for the selected pen. Really only makes sense for exclusive pens.
*
*
*   RESULTS
*
*   n   =  The allocated pen. -1 will be returned if there is no pen available
*          for you.
*
*   BUGS
*
*   NOTES
*   When you allocate a palette entry in non-exclusive mode, you
*   should not change it (via SetRGB32), because other programs on the
*   same screen may be using it. With PEN_EXCLUSIVE mode, you can
*   change the returned entry at will.
*
*   To avoid visual artifacts, you should not free up a palette
*   entry until you are sure that your application is not displaying
*   any pixels in that color at the time you free it. Otherwise, another
*   task could allocate and set that color index, thus changing the colors
*   of your pixels.
*
*   Generally, for shared access, you should use ObtainBestPenA()
*   instead, since it will not allocate a new color if there is one
*   "close enough" to the one you want already.
*   If there is no Palextra attached to the colormap, then this
*   routine will always fail.
*
*   SEE ALSO
*   GetColorMap() ReleasePen() AttachPalExtra() ObtainBestPenA()
*
*********************************************************************************/

LONG MyObtainPen(struct ColorMap *cm,
                 ULONG  n,
                 ULONG  r,
                 ULONG  g,
                 ULONG  b,
                 ULONG  flags,
                 struct IntuitionBase *IntuitionBase)
{
    // struct Library *SysBase = (struct Library *)GfxBase->ExecBase;
    struct PaletteExtra *pe=cm->PalExtra;
    long   retvalue=-1;

    DEBUG_OBTAINPEN(dprintf("LIB_ObtainPen(cm 0x%lx n %ld r 0x%08lx g 0x%08lx b 0x%08lx flags %ld)\n",cm,n,r,g,b,flags);)

    DEBUG_OBTAINPEN(dprintf("LIB_ObtainSemaphore: NestCount 0x%lx,QueueCount 0x%lx,Owner %s\n",
                            pe->pe_Semaphore.ss_NestCount,
                            pe->pe_Semaphore.ss_QueueCount,
                            ((ULONG) pe->pe_Semaphore.ss_Owner > 1) ? pe->pe_Semaphore.ss_Owner->tc_Node.ln_Name : "Shared\n"));

    ObtainSemaphore(&pe->pe_Semaphore);

    DEBUG_OBTAINPEN(dprintf("LIB_ObtainPen: obtained the semaphore\n"));

    DEBUG_OBTAINPEN(dprintf("LIB_ObtainPen: pe_NFree 0x%lx\n",
                            pe->pe_NFree));

    if (pe->pe_NFree != 0)
    {
        UWORD   *refcnt;
        UBYTE   *alloclist;
        long    alloc;

        refcnt = (UWORD *)pe->pe_RefCnt;
        alloclist = (char *)pe->pe_AllocList;

        DEBUG_OBTAINPEN(dprintf("LIB_ObtainPen: pe_RefCnt 0x%lx\n",
                                pe->pe_RefCnt));

        DEBUG_OBTAINPEN(dprintf("LIB_ObtainPen: pe_AllocList 0x%lx\n",
                                pe->pe_AllocList));
        if (n != -1L)
        {
            // if not allocatable, fail
            DEBUG_OBTAINPEN(dprintf("LIB_ObtainPen: pe_SharableColors 0x%lx\n",
                                    pe->pe_SharableColors));
            if (pe->pe_SharableColors >= n)
            {
                if (refcnt[n] == 0)
                {
                    refcnt[n]++;
                    retvalue = n;

                    // decrement avail count
                    pe->pe_NFree--;
                    DEBUG_OBTAINPEN(dprintf("LIB_ObtainPen: pe_NFree 0x%lx\n",
                                            pe->pe_NFree));

                    if (pe->pe_NFree != 0)
                    {
                        UWORD j;


                        j = pe->pe_FirstFree;

                        DEBUG_OBTAINPEN(dprintf("LIB_ObtainPen: pe_FirstFree 0x%lx\n",
                                                pe->pe_FirstFree));

                        if (j == n)
                        {
                            // we don't need to walk through the list
                            // first = n -> next
                            pe->pe_FirstFree = alloclist[n];

                            DEBUG_OBTAINPEN(dprintf("LIB_ObtainPen: pe_FirstFree = alloclist[%ld] 0x%lx\n",
                                                    n,
                                                    alloclist[n]));

                        }
                        else
                        {
                            while(alloclist[j] != n)
                            {
                                DEBUG_OBTAINPEN(dprintf("LIB_ObtainPen: alloclist[%ld] %ld != %ld\n",
                                                        j,alloclist[j],n));
                                j = alloclist[j];
                            }
                            // j -> next = n -> next
                            alloclist[j] = alloclist[n];
                        }
                    }

                    // check exclusive

                    if ((flags & PENF_EXCLUSIVE) == 0)
                    {
                        // not exclusive, so add it to shared list
                        pe->pe_NShared++;
                        alloclist[n]=pe->pe_FirstShared;
                        pe->pe_FirstShared=n;
                    }
                }
                else
                {
                    DEBUG_OBTAINPEN(dprintf("(index already allocated)");)
                }
            }
            else
            {
                DEBUG_OBTAINPEN(dprintf("(not allocatable,sharable=%ld)",pe->pe_SharableColors);)
            }
        }
        else
        {
            // decrement avail count
            DEBUG_OBTAINPEN(dprintf("LIB_ObtainPen: decrement avail cnt\n"));
            pe->pe_NFree--;
            alloc = pe->pe_FirstFree;
            refcnt[alloc]++;
            // FirstFree = alloc->next
            pe->pe_FirstFree = alloclist[alloc];
            retvalue = alloc;

            // check exclusive

            if ((flags & PENF_EXCLUSIVE) == 0)
            {
                DEBUG_OBTAINPEN(dprintf("LIB_ObtainPen: add shared list\n"));
                // not exclusive, so add it to shared list
                pe->pe_NShared++;
                alloclist[alloc]=pe->pe_FirstShared;
                pe->pe_FirstShared=alloc;
            }
        }

        if (retvalue != -1L)
        {
            long    i = retvalue;

            if ((flags & PENF_NO_SETCOLOR) == 0)
            {
                DEBUG_OBTAINPEN(dprintf("LIB_ObtainPen: set color\n"));
                SetRGB32(pe->pe_ViewPort,i,r,g,b);
            }
        }
    }

    DEBUG_OBTAINPEN(dprintf("LIB_ObtainPen: release it\n"));
    ReleaseSemaphore(&pe->pe_Semaphore);

    DEBUG_OBTAINPEN(dprintf("LIB_ObtainPen(cm 0x%lx n %ld r 0x%08lx g 0x%08lx b 0x%08lx flags %ld) -> retvalue %lx\n",cm,n,r,g,b,flags,retvalue);)

    return(retvalue);
}

#undef ObtainPen

#define ObtainPen(cm,n,r,g,b,flags) MyObtainPen(cm,n,r,g,b,flags,IntuitionBase)
#endif

/* Default colors for the new screen */

const ULONG coltab[] =
{
    (16L << 16) + 0,    /* 16 colors, loaded at index 0 */

    /* X11 color names  */
    0xB3B3B3B3, 0xB3B3B3B3, 0xB3B3B3B3, /* Grey70   */
    0x00000000, 0x00000000, 0x00000000, /* Black    */
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, /* White    */
    0x66666666, 0x88888888, 0xBBBBBBBB, /* AMIGA Blue   */

    0x00000000, 0x00000000, 0xFFFFFFFF, /* Blue     */
    0x00000000, 0xFFFFFFFF, 0x00000000, /* Green    */
    0xFFFFFFFF, 0x00000000, 0x00000000, /* Red      */
    0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, /* Cyan     */

    0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, /* Magenta  */
    0xEEEEEEEE, 0x82828282, 0xEEEEEEEE, /* Violet   */
    0xA5A5A5A5, 0x2A2A2A2A, 0x2A2A2A2A, /* Brown    */
    0xFFFFFFFF, 0xE4E4E4E4, 0xC4C4C4C4, /* Bisque   */

    0xE6E6E6E6, 0xE6E6E6E6, 0xFAFAFAFA, /* Lavender */
    0x00000000, 0x00000000, 0x80808080, /* Navy     */
    0xF0F0F0F0, 0xE6E6E6E6, 0x8C8C8C8C, /* Khaki    */
    0xA0A0A0A0, 0x52525252, 0x2D2D2D2D, /* Sienna   */
    0L      /* Termination */
};

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
 
    HISTORY
    29-10-95    digulla automatically created from
                intuition_lib.fd and clib/intuition_protos.h
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct NewScreen 	     ns;
    struct TagItem   	    *tag, *tagList;
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
    struct MonitorInfo       monitor;
#ifdef __MORPHOS__
    ULONG                    allocbitmapflags = BMF_DISPLAYABLE;
#else
    BOOL            	     frontbm_set = FALSE;
    struct BitMap    	    *old_front_bm = NULL;
#endif
    //ULONG                  lock;
    WORD                     numcolors;
    BOOL                     workbench = FALSE;
    struct TagItem   	     modetags[] =
    {
        { BIDTAG_Depth        	, 0UL   },
        { BIDTAG_DesiredWidth   , 0UL   },
        { BIDTAG_DesiredHeight  , 0UL   },
        { TAG_DONE          	    	}
    };
    ULONG   	    	     modeid = INVALID_ID;

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

#ifndef __MORPHOS__
    if (IntuitionBase->FirstScreen)
        old_front_bm = IntuitionBase->FirstScreen->RastPort.BitMap;
#endif

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
    if (!CyberGfxBase) CyberGfxBase = OpenLibrary("cybergraphics.library",0);
    if (!CyberGfxBase) return NULL;

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
    errorPtr = (ULONG *)GetTagData((Tag)SA_ErrorCode, (ULONG)NULL, (struct TagItem *)tagList);

    DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_ErrorCode 0x%lx\n",errorPtr));

    if (screen == NULL)
    {
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

        while((tag = NextTagItem (&tagList)))
        {
#if 1
            DEBUG_OPENSCREEN(dprintf("OpenScreen: Tag 0x%08lx Data 0x%08lx\n",
                                     tag->ti_Tag, tag->ti_Data));
#endif
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

                    if (strcmp((char *)tag->ti_Data, "Workbench") == 0)
                    {
#warning This would still not be safe, if a normal app tried to open its own screen with SA_PubName=Workbench
                        if (GetPrivIBase(IntuitionBase)->WorkBench)
                        {
                            UnlockPubScreenList();
                            return NULL;
                        }
                        workbench = TRUE;
                    }
                    else
                    {
                        old = LockPubScreen((STRPTR)tag->ti_Data);

                        if (old != NULL)
                        {
                            UnlockPubScreen(NULL, old);
                            SetError(OSERR_PUBNOTUNIQUE);
                            UnlockPubScreenList();

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
                    return NULL;

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
                    return NULL;

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
#ifdef __MORPHOS__
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_Interleaved 0x%lx\n",tag->ti_Data));
                if (tag->ti_Data)
                {
                    allocbitmapflags |= BMF_INTERLEAVED;
                }
                else
                {
                    allocbitmapflags &= ~BMF_INTERLEAVED;
                }
#endif
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
                break;

            case SA_Parent:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_Parent 0x%lx\n",tag->ti_Data));
                break;

            case SA_Draggable:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_Draggable 0x%lx\n",tag->ti_Data));
                break;

            case SA_Exclusive:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_Exclusive 0x%lx\n",tag->ti_Data));
                break;

            case SA_VideoControl:
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SA_VideoControl 0x%lx\n",tag->ti_Data));
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

#warning TODO: Missing SA_ Tags
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

    /* First Init the RastPort then get the BitPlanes!! */

    modetags[0].ti_Data = ns.Depth;
    modetags[1].ti_Data = ns.Width;
    modetags[2].ti_Data = ns.Height;

#ifdef __MORPHOS__
    // if default HIRES_KEY or HIRESLACE_KEY is passed, make sure we find a replacement
    // cybergraphx mode
    //
    if (INVALID_ID != modeid)
    {
        if (FindDisplayInfo(modeid) == NULL)
        {
            switch(modeid)
            {
            case HIRES_KEY:
            case HIRESLACE_KEY:
                {
                    struct TagItem   bestmodetags[] =
                    {
                        { CYBRBIDTG_Depth     	    , 0UL   },
                        { CYBRBIDTG_NominalWidth    , 0UL   },
                        { CYBRBIDTG_NominalHeight   , 0UL   },
                        { TAG_DONE  	    	    	    }
                    };

                    bestmodetags[0].ti_Data = 8;
                    bestmodetags[1].ti_Data = ns.Width;
                    bestmodetags[2].ti_Data = ns.Height;

                    DEBUG_OPENSCREEN(dprintf("resetting native mode id !!\n");)
                    DEBUG_OPENSCREEN(dprintf("ns.Width %ld ns.Height %ld ns.Depth %ld !!\n",
                            (LONG) ns.Width, (LONG) ns.Height, (LONG) ns.Depth);)


                    modeid = BestCModeIDTagList(bestmodetags);

                    DEBUG_OPENSCREEN(dprintf("BestCModeIDTagList returned %ld\n",modeid);)
                }
                break;
		
            default:
                break;
            }

        }
    }
#endif

    if (INVALID_ID == modeid)
    {
        modeid = BestModeIDA(modetags);
        if (INVALID_ID == modeid)
        {
            DEBUG_OPENSCREEN(dprintf("!!! OpenScreen(): Could not find valid modeid !!!\n");)
            return NULL;
        }
    }

    DEBUG_OPENSCREEN(dprintf("OpenScreen: ModeID 0x%08lx\n", modeid));

#ifdef __MORPHOS__

    InitRastPort(&screen->Screen.RastPort);
    rp_inited = TRUE;
    success = FALSE;
    numcolors = 0;

    if ((displayinfo = FindDisplayInfo(modeid)) != NULL &&
        GetDisplayInfoData(displayinfo, &dimensions, sizeof(dimensions), DTAG_DIMS, modeid) &&
        GetDisplayInfoData(displayinfo, &monitor, sizeof(monitor), DTAG_MNTR, modeid))
    {
        success = TRUE;

        screen->Monitor = monitor.Mspc;

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

        if (ns.Height == STDSCREENHEIGHT)
            ns.Height = dclip->MaxY - dclip->MinY + 1;

        DEBUG_OPENSCREEN(dprintf("OpenScreen: Monitor 0x%lx Width %ld Height %ld\n",
                                 screen->Monitor, ns.Width, ns.Height));

        if (ns.Type & CUSTOMBITMAP)
        {
            struct BitMap *custombm;

            custombm = ns.CustomBitMap;

            if (IsCyberModeID(modeid) && custombm)
            {
                int pixfmt = GetCyberIDAttr(CYBRIDATTR_PIXFMT,modeid);
                                                               
                if(GetCyberMapAttr(custombm,CYBRMATTR_PIXFMT) != pixfmt)
                {
                        // incompatible formats !
                        custombm = NULL;
                }
            }

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
        else
        {
            screen->Screen.RastPort.BitMap = NULL;                        
        }


        if(screen->Screen.RastPort.BitMap == NULL)
        {
            ULONG pixfmt;
            ULONG Depth;

            Depth = (dimensions.MaxDepth > 8) ? dimensions.MaxDepth : ns.Depth;

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

            screen->AllocatedBitmap = screen->Screen.RastPort.BitMap;

            memcpy(&screen->Screen.BitMap,screen->Screen.RastPort.BitMap,sizeof(struct BitMap));
        }

        DEBUG_OPENSCREEN(dprintf("OpenScreen: BitMap 0x%lx\n",
                                 screen->Screen.RastPort.BitMap));
    }
    else
    {
        DEBUG_OPENSCREEN(dprintf("OpenScreen: no displayinfo\n"));
    }
#else
    if ((displayinfo = FindDisplayInfo(modeid)) != NULL &&
        GetDisplayInfoData(displayinfo, &dimensions, sizeof(dimensions), DTAG_DIMS, modeid) &&
        GetDisplayInfoData(displayinfo, &monitor, sizeof(monitor), DTAG_MNTR, modeid))
    {
        screen->Monitor = monitor.Mspc;

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

        if (ns.Height == STDSCREENHEIGHT)
            ns.Height = dclip->MaxY - dclip->MinY + 1;

    }
    
    if ((success = InitRastPort (&screen->Screen.RastPort)))
    {
        rp_inited = TRUE;
    }

    screen->Screen.RastPort.BitMap = screen->AllocatedBitmap = AllocScreenBitMap(modeid);

#endif
    D(bug("got bitmap\n"));

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
        /* Store pointer to bitmap, so we can get hold of it
           from withing LoadRGBxx() functions
        */
        D(bug("got allocated stuff\n"));
        screen->Screen.ViewPort.RasInfo->BitMap = screen->Screen.RastPort.BitMap;

    }

    if (ok)
    {
        /* Read depth from the bitmap to avoid AttachPalExtra/ObtainPen getting
         * confused if cgx decided to allocate a higher depth bitmap than what
         * was asked.
         */
        ns.Depth = GetBitMapAttr(screen->Screen.RastPort.BitMap,BMA_DEPTH);

        numcolors = (ns.Depth <= 8) ? (1L << ns.Depth) : 256;

        /* Get a color map structure. Sufficient colors?? */

        DEBUG_OPENSCREEN(dprintf("OpenScreen: Colormap Entries %ld\n",
                                 numcolors));

        if ((screen->Screen.ViewPort.ColorMap = GetColorMap(numcolors < 32 ? 32 : numcolors)) != NULL)
        {

#ifndef __MORPHOS__ /* Use VideoControl for MorphOS */
            screen->Screen.ViewPort.ColorMap->VPModeID = modeid;
#endif

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

    if (ok)
    {
        struct ViewPortExtra *vpe = GfxNew(VIEWPORT_EXTRA_TYPE);

        DEBUG_OPENSCREEN(dprintf("OpenScreen: ViewPortExtra 0x%lx\n", vpe));

        ok = FALSE;

        if (vpe)
        {
            struct TagItem tags[6];

            memcpy(&vpe->DisplayClip, dclip,sizeof(struct Rectangle));

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
            tags[4].ti_Tag  = VTAG_END_CM;
            tags[5].ti_Tag  = TAG_END;

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

        // added 2002-03-10, pre iprefs openscreen has all first 4 colours black (cyfm)
        // removed 2002-26-12, I now set the pens on start (zapek)
        #if 0
        if ((p = GetPrivIBase(IntuitionBase)->Colors))
        {
            int Sum;

            DEBUG_OPENSCREEN(dprintf("OpenScreen: Intuition Color32 Table 0x%lx\n",p);)

            Sum = p[0].red + p[0].green + p[0].blue;
            Sum += p[1].red + p[1].green + p[1].blue;
            Sum += p[2].red + p[2].green + p[2].blue;
            Sum += p[3].red + p[3].green + p[3].blue;
            if (Sum==0)
            {
                int i;

                DEBUG_OPENSCREEN(dprintf("OpenScreen: All 4 first colours black, reset !!\n");)

                for (i = 0; i < COLORTABLEENTRIES; i++)
                {
                    DEBUG_OPENSCREEN(dprintf("OpenScreen: Current Color32[%ld] R 0x%lx G 0x%lx B 0x%lx\n",
                                             i,
                                             p[i].red, p[i].green, p[i].blue));
                }
                p[0].red   = 0xAAAAAAAA;
                p[0].green = 0xAAAAAAAA;
                p[0].blue  = 0xAAAAAAAA;

                p[1].red   = 0x00000000;
                p[1].green = 0x00000000;
                p[1].blue  = 0x00000000;

                p[2].red   = 0xFFFFFFFF;
                p[2].green = 0xFFFFFFFF;
                p[2].blue  = 0xFFFFFFFF;

                p[3].red   = 0x55555555;
                p[3].green = 0x77777777;
                p[3].blue  = 0xAAAAAAAA;

            }
        }
        #endif
    }

#endif

    if (ok)
    {
        struct Color32 *p;
        int 	    	k;
        UWORD 	       *q;

        DEBUG_OPENSCREEN(dprintf("OpenScreen: Load Colors\n"));

        D(bug("Loading colors\n"));

        /* First load default colors for the screen */
        LoadRGB32(&screen->Screen.ViewPort, (ULONG *)coltab);

        DEBUG_OPENSCREEN(dprintf("OpenScreen: Set first 4 colors\n"));

#warning "FIXME: backport: #if'ed out"
#if 0
        p = GetPrivIBase(IntuitionBase)->Colors;
        for (k = 0; k < 4 && k < numcolors; ++k)
        {
            DEBUG_OPENSCREEN(dprintf("OpenScreen: SetRGB32 Viewport 0x%lx Index %ld R 0x%lx G 0x%lx B 0x%lx\n",
                                     screen->Screen.ViewPort,
                                     k, p[k].red, p[k].green, p[k].blue));
            SetRGB32(&screen->Screen.ViewPort, k, p[k].red, p[k].green, p[k].blue);
        }

        if (ns.Depth >= 3)
        {
            DEBUG_OPENSCREEN(dprintf("OpenScreen: Set last 4 colors\n"));

    	    #warning Isn`t that only set for FullPalette ?
            for (k = 0; k < 4; ++k)
            {
                DEBUG_OPENSCREEN(dprintf("OpenScreen: SetRGB32 Viewport 0x%lx Index %ld R 0x%lx G 0x%lx B 0x%lx\n",
                                         screen->Screen.ViewPort,
                                         numcolors - k - 1, p[k+4].red, p[k+4].green, p[k+4].blue));

    	    #if 1
                ObtainPen(screen->Screen.ViewPort.ColorMap,
                          numcolors - 4 + k,
                          p[k+4].red,
                          p[k+4].green,
                          p[k+4].blue,
                          PEN_EXCLUSIVE);
    	    #else
                SetRGB32(&screen->Screen.ViewPort, numcolors - k - 1, p[k+4].red, p[k+4].green, p[k+4].blue);
    	    #endif
            }
        }
#endif

        DEBUG_OPENSCREEN(dprintf("OpenScreen: Obtain Mousepointer colors\n"));

        /* Allocate pens for the mouse pointer */
        q = &GetPrivIBase(IntuitionBase)->ActivePreferences->color17;
        for (k = 0; k < 3; ++k, ++q)
        {
            DEBUG_OPENSCREEN(dprintf("OpenScreen: ColorMap 0x%lx Pen %ld R 0x%lx G 0x%lx B 0x%lx\n",
                                     screen->Screen.ViewPort.ColorMap,
                                     k + 17,
                                     (*q >> 8) * 0x11111111,
                                     ((*q >> 4) & 0xf) * 0x11111111,
                                     (*q & 0xf) * 0x11111111));
            if (k + 17 < numcolors)
            {
                ObtainPen(screen->Screen.ViewPort.ColorMap,
                          k + 17,
                          (*q >> 8) * 0x11111111,
                          ((*q >> 4) & 0xf) * 0x11111111,
                          (*q & 0xf) * 0x11111111,
                          PEN_EXCLUSIVE);
            }
            else
            {
                /* Can't be allocated, but can still be set. */
                SetRGB32(&screen->Screen.ViewPort,
                         k + 17,
                         (*q >> 8) * 0x11111111,
                         ((*q >> 4) & 0xf) * 0x11111111,
                         (*q & 0xf) * 0x11111111);
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

#ifndef __MORPHOS__
        /* Mark the bitmap of the screen as an AROS-displayed BitMap */
        screen->Screen.RastPort.BitMap->Flags |= BMF_AROS_HIDD;
#endif

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

#if 0
        /* Root layer now installed automatically by first call
           to CreateLayerTagList */

#ifdef CreateLayerTagList
        {
            struct TagItem tags[4] =
            {
                {LA_Visible , FALSE 	    },
                {LA_Priority, ROOTPRIORITY  },
                {TAG_DONE   	    	    }
            };

            screen->rootLayer =
                CreateLayerTagList(&screen->Screen.LayerInfo,
                                   screen->Screen.RastPort.BitMap,
                                   screen->Screen.LeftEdge,
                                   screen->Screen.TopEdge,
                                   screen->Screen.LeftEdge + screen->Screen.Width - 1,
                                   screen->Screen.TopEdge + screen->Screen.Height - 1,
                                   0,
                                   tags);

            DEBUG_OPENSCREEN(dprintf("OpenScreen: rootLayer 0x%lx\n",screen->rootLayer));
        }
#endif
#endif

        if (NULL != layer_info_hook)
        {
            DEBUG_OPENSCREEN(dprintf("OpenScreen: instal layerinfohook\n"));
            InstallLayerInfoHook(&screen->Screen.LayerInfo, layer_info_hook);
        }
        D(bug("layers intited screen\n"));

        screen->DInfo.dri_Version = DRI_VERSION;
        screen->DInfo.dri_NumPens = NUMDRIPENS;
        screen->DInfo.dri_Pens = screen->Pens;
        /* dri_Depth is 8 on hi/true color screens like in AmigaOS with picasso96/cybergraphx */
        screen->DInfo.dri_Depth = (ns.Depth <= 8) ? ns.Depth : 8;
#warning These are probably monitor dependent
        screen->DInfo.dri_Resolution.X = 44;
        screen->DInfo.dri_Resolution.Y = 44;
        screen->DInfo.dri_Flags = 0;


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

#ifndef __MORPHOS__
    if (ok)
    {
        if (!SetFrontBitMap(screen->Screen.RastPort.BitMap, TRUE))
            ok = FALSE;
        else
            frontbm_set = TRUE;
    }
#endif

#ifdef SKINS
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

        if (!(screen->DInfo.dri_Colors = AllocMem(4 * DRIPEN_NUMDRIPENS,MEMF_PUBLIC)))
            ok = FALSE;

        if (ok)
        {
            CopyMem(&defaultdricolors,screen->DInfo.dri_Colors,sizeof (defaultdricolors));
            memset(((UBYTE *) screen->DInfo.dri_Colors) + sizeof(defaultdricolors), 0, 4 * DRIPEN_NUMDRIPENS - sizeof(defaultdricolors));
        }

        if (ok)
        {
            if ((screen->DInfo.dri_Customize = AllocMem(sizeof (struct IntuitionCustomize),MEMF_PUBLIC|MEMF_CLEAR)))
            {
                struct IntuitionCustomize *ic;
                ic = screen->DInfo.dri_Customize;
                screen->DInfo.dri_Flags |= DRIF_SKINSSUPPORT;
                /* This initializes CustomizePrefs structure */

                int_SkinAction(SKA_LoadSkin,(ULONG*)&screen->DInfo,(struct Screen *)screen,IntuitionBase);
            }
            else
            {
                ok = FALSE;
            }
        }
    }

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
            {TAG_DONE                       	    }
        };

        screen->DInfo.dri_CheckMark = NewObjectA(NULL, "sysiclass", sysi_tags);
        DEBUG_OPENSCREEN(dprintf("OpenScreen: CheckMark 0x%lx\n",
                                 screen->DInfo.dri_CheckMark));

        sysi_tags[0].ti_Data = AMIGAKEY;

        screen->DInfo.dri_AmigaKey  = NewObjectA(NULL, "sysiclass", sysi_tags);
        DEBUG_OPENSCREEN(dprintf("OpenScreen: AmigaKey 0x%lx\n",
                                 screen->DInfo.dri_AmigaKey));
#ifdef SKINS
        sysi_tags[0].ti_Data = SUBMENUIMAGE;
        screen->DInfo.dri_Customize->submenu  = NewObjectA(NULL, "sysiclass", sysi_tags);
        sysi_tags[0].ti_Data = MENUTOGGLEIMAGE;
        screen->DInfo.dri_Customize->menutoggle  = NewObjectA(NULL, "sysiclass", sysi_tags);
        if (!screen->DInfo.dri_Customize->submenu ||
            !screen->DInfo.dri_Customize->menutoggle) ok = FALSE;
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

#ifdef __MORPHOS__
        screen->Screen.BarVBorder  = 1;
        screen->Screen.BarHBorder  = 5;
        screen->Screen.MenuVBorder = 2;
        screen->Screen.MenuHBorder = 4;
#else
        screen->Screen.BarVBorder  = 1; /* on the Amiga it is (usually?) 1 */
        screen->Screen.BarHBorder  = 5;
        screen->Screen.MenuVBorder = 2; /* on the Amiga it is (usually?) 2 */
        screen->Screen.MenuHBorder = 4;
#endif
#ifdef TITLEHACK
        screen->Screen.BarHeight   = screen->DInfo.dri_Font->tf_YSize + screen->Screen.WBorTop-2 +
                                     screen->Screen.BarVBorder * 2; /* real layer will be 1 pixel higher! */
#else
        screen->Screen.BarHeight   = screen->DInfo.dri_Font->tf_YSize +
                                     screen->Screen.BarVBorder * 2; /* real layer will be 1 pixel higher! */
#endif

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

            struct TagItem image_tags[] =
            {
            #if SQUARE_WIN_GADGETS
		{IA_Left    	, -1	    	    	    	    	},
                {IA_Width   	, SDEPTH_HEIGHT + 1  	    	    	},
	    #else
		{IA_Left    	, 0 	    	    	    	    	},
	    #endif
                {IA_Height   	, SDEPTH_HEIGHT     	    	    	},
                {SYSIA_Which    , SDEPTHIMAGE       	    	    	},
                {SYSIA_DrawInfo , (IPTR)&screen->DInfo      	    	},
                {SYSIA_Size 	, screen->Screen.Flags & SCREENHIRES ?
                    	    	  SYSISIZE_MEDRES : SYSISIZE_LOWRES 	},
                {TAG_DONE                   	    	    	    	}
            };

            struct Object *im = 0;

            if (!(screen->Screen.Flags & SCREENQUIET))
            {
                im = NewObjectA(NULL, SYSICLASS, image_tags);
            }

            sdepth_tags[0].ti_Data = (IPTR)im;

            screen->depthgadget = NewObjectA(NULL, BUTTONGCLASS, sdepth_tags );

            DEBUG_OPENSCREEN(dprintf("OpenScreen: DepthGadget 0x%lx\n",
                                     screen->depthgadget));

            screen->Screen.FirstGadget = (struct Gadget *)screen->depthgadget;
            if (screen->Screen.FirstGadget)
            {
                struct TagItem gadtags[] =
                {
                    {GA_RelRight, 0 },
                    {TAG_DONE       }
                };
                IPTR width;

                GetAttr(GA_Width, screen->depthgadget, &width);
    	    #if SQUARE_WIN_GADGETS
		width--;
	    #endif
 	     
                gadtags[0].ti_Data = -width + 1;
                SetAttrsA(screen->depthgadget, gadtags);
                screen->Screen.FirstGadget->GadgetType |= GTYP_SCRGADGET;
            }
            else
            {
                if (im) DisposeObject(im);
            }

        }


#if 1
        {
            int i;
	    
            for (i = 0;i <= screen->DInfo.dri_NumPens; i++)
            {
                DEBUG_OPENSCREEN(dprintf("OpenScreen: dri_Pen[%ld] = %ld\n",i,screen->DInfo.dri_Pens[i]));
            }
        }
#endif

#ifdef USEWINDOWLOCK
        /* let's wait for user to finish window drag/size actions to avoid
        deadlocks and not break user's input */
        ObtainSemaphore(&GetPrivIBase(IntuitionBase)->WindowLock);
        windowlock = TRUE;
#endif

#ifdef SKINS
        int_CalcSkinInfo(&screen->Screen,IntuitionBase);
        int_InitTitlebarBuffer(screen,IntuitionBase);
#endif
        D(bug("callling SetRast()\n"));

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

        //jDc: ALL screens MUST have BarLayer!
        CreateScreenBar(&screen->Screen, IntuitionBase);

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

            DoSyncAction((APTR)int_openscreen,&msg.msg,IntuitionBase);

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

    if (ok)
    {
        ok = RethinkDisplay() == 0;
        DEBUG_OPENSCREEN(dprintf("OpenScreen: RethinkDisplay %s\n", ok ? "ok" : "failed"));
    }
#endif

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
#ifndef __MORPHOS__
        if (frontbm_set)
        {
            if (NULL != old_front_bm)
                SetFrontBitMap(old_front_bm, FALSE);
        }
#endif

        if (li_inited)
        {
            DEBUG_OPENSCREEN(dprintf("OpenScreen: Get ThinLayerInfo\n"));
            ThinLayerInfo(&screen->Screen.LayerInfo);
        }

        if (screen->Screen.ViewPort.ColorMap)
        {
#ifdef __MORPHOS__
            struct TagItem tags[2];

            tags[0].ti_Tag  = VTAG_ATTACH_CM_GET;
            tags[0].ti_Data = 0;
            tags[1].ti_Tag  = VTAG_END_CM;

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

#ifdef SKINS
        DisposeObject(screen->DInfo.dri_Customize->submenu);
        DisposeObject(screen->DInfo.dri_Customize->menutoggle);
        if (screen->DInfo.dri_Customize) FreeMem(screen->DInfo.dri_Customize,sizeof (struct IntuitionCustomize));
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

        DEBUG_OPENSCREEN(dprintf("OpenScreen: Free Screen\n"));
        FreeMem (screen, sizeof (struct IntScreen));

        screen = 0;

    } /* if (!ok) */

    DEBUG_OPENSCREEN(dprintf("OpenScreen: return 0x%lx\n", screen));

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

    lock = LockIBase((ULONG)NULL);
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

}


