
/* INCLUDES */

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <libraries/dos.h>
#include <libraries/dosextens.h>
#include <graphics/gfxmacros.h>
#include <graphics/videocontrol.h>
#include <graphics/view.h>
#include <graphics/gfxbase.h>
#include <libraries/gadtools.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/imageclass.h>
#include <proto/graphics.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/gadtools.h>
#include <proto/utility.h>
#include <clib/macros.h>
#include <string.h>

#include <libraries/reqtools.h>
#include <proto/reqtools.h>

#ifdef __AROS__
#include <aros/asmcall.h>
#endif

#include "filereq.h"

/****************************************************************************************/

/* Use ColorMap.Count to find out number of palette entried on screen */
//#define DO_CM_DEPTH

/****************************************************************************************/

UWORD CHIP waitpointer[] =
{
    0x0000,0x0000,
    0x0400,0x07C0,0x0000,0x07C0,0x0100,0x0380,0x0000,0x07E0,
    0x07C0,0x1FF8,0x1FF0,0x3FEC,0x3FF8,0x7FDE,0x3FF8,0x7FBE,
    0x7FFC,0xFF7F,0x7EFC,0xFFFF,0x7FFC,0xFFFF,0x3FF8,0x7FFE,
    0x3FF8,0x7FFE,0x1FF0,0x3FFC,0x07C0,0x1FF8,0x0000,0x07E0,
    0x0000,0x0000,
};

/****************************************************************************************/

extern struct Library 		*GadToolsBase;
extern struct DosLibrary 	*DOSBase;
extern struct IntuitionBase 	*IntuitionBase;
extern struct GfxBase 		*GfxBase;
extern struct ReqToolsBase 	*ReqToolsBase;
#if defined(__AROS__) || defined(__GNUC__)
extern struct UtilityBase 	*UtilityBase;
#else
extern struct Library 		*UtilityBase;
#endif

/****************************************************************************************/

int ASM SAVEDS GetVScreenSize (
	REGPARAM(a0, struct Screen *, scr),
	REGPARAM(a1, int *, width),
	REGPARAM(a2, int *, height))
{
    struct ViewPortExtra 	*vpe;
    struct Rectangle 		dispclip, *clip;
    ULONG 			getvpetags[3];
    int 			ht;
#ifndef USE_FORBID
    struct Screen		*pubscr;
    ULONG			ilock = 0L;
    BOOL			isfirst;
#endif


    getvpetags[0] = VTAG_VIEWPORTEXTRA_GET;
    getvpetags[1] = (ULONG)&vpe;
    getvpetags[2] = TAG_END;
    
#ifdef USE_FORBID

    Forbid();

#ifdef __AROS__
#warning No VideoControl in AROS, yet
#else
    if (IntuitionBase->FirstScreen == scr &&
	VideoControl (scr->ViewPort.ColorMap, (struct TagItem *)getvpetags) == 0)
    {
	clip = &((struct ViewPortExtra *)getvpetags[1])->DisplayClip;
    }
    else
    {
#endif
	QueryOverscan (GetVPModeID (&scr->ViewPort), &dispclip, OSCAN_TEXT);
	clip = &dispclip;
#ifndef __AROS__
    }
#endif

    Permit();

#else

    if ((pubscr = LockPubScreenByAddr(scr)))
    {
	/* Cool, we got a lock on the screen, we don't need to Forbid() */
	ilock = LockIBase(0);
    }
    else
    {
	/* damn it, not a pubscreen, Forbid() as a last resort */
	Forbid();
    }

    isfirst = (IntuitionBase->FirstScreen == scr);

    if (pubscr)
    {
	UnlockIBase(ilock);
    }

#ifdef __AROS__
#warning No VideoControl in AROS, yet
#else
    if (isfirst &&
	VideoControl (scr->ViewPort.ColorMap, (struct TagItem *)getvpetags) == 0)
    {
	clip = &((struct ViewPortExtra *)getvpetags[1])->DisplayClip;
    }
    else
    {
#endif
	QueryOverscan (GetVPModeID (&scr->ViewPort), &dispclip, OSCAN_TEXT);
	clip = &dispclip;
#ifndef __AROS__
    }
#endif
    if (pubscr)
    {
	UnlockPubScreen(NULL, pubscr);
    }
    else
    {
	Permit();
    }

#endif
    
    *width = clip->MaxX - clip->MinX + 1;
    *height = ht = clip->MaxY - clip->MinY + 1;
    
    if (scr->Width < *width) *width = scr->Width;
    if (scr->Height < *height) *height = scr->Height;
    
    return ((ht >= 400) ? 4 : 2);
}

/****************************************************************************************/

struct Screen *REGARGS LockPubScreenByAddr (struct Screen *scr)
{
    struct List *pubscreenlist;
    struct PubScreenNode *pubscreennode;
    UBYTE pubscreenname[MAXPUBSCREENNAME + 1];

    pubscreenname[0] = '\0';

    pubscreenlist = LockPubScreenList();
    pubscreennode = (struct PubScreenNode *) pubscreenlist->lh_Head;
    while (pubscreennode->psn_Node.ln_Succ)
    {
	if (pubscreennode->psn_Screen == scr)
	{
	    strcpy(pubscreenname, pubscreennode->psn_Node.ln_Name);
	    break;
	}
	pubscreennode = (struct PubScreenNode *) pubscreennode->psn_Node.ln_Succ;
    }
    UnlockPubScreenList();

    /* If we found a matching pubscreen try to lock it (note: can fail) */
    if (pubscreenname[0])
    {
	scr = LockPubScreen(pubscreenname);
    }
    else
    {
	scr = NULL;
    }

    return scr;
}

/****************************************************************************************/


#undef ThisProcess
#define ThisProcess()		( ( APTR ) FindTask( NULL ) )

/****************************************************************************************/

struct Screen *REGARGS GetReqScreen (
	struct NewWindow *nw, struct Window **prwin, struct Screen *scr, char *pubname)
{
    struct Window  *win = *prwin;
    struct Process *proc;

    if (!pubname && !scr && !win)
    {
	proc = ThisProcess();
	if (proc->pr_Task.tc_Node.ln_Type == NT_PROCESS) win = proc->pr_WindowPtr;
    }
    
    nw->Type = CUSTOMSCREEN;
    if (scr && !win)
    {
	struct Screen *pubscr;

	/* Try to lock the screen as a public screen */
	if ((pubscr = LockPubScreenByAddr(scr)))
	{
	    scr = pubscr;
	    nw->Type = PUBLICSCREEN;
	}
	/* FIXME: probably should do something smart if the locking fail.
	   RT_Screen is more than dangerous if you ask me... - Piru */
    }
    else
    {
	if (win && (ULONG)win != ~0) scr = win->WScreen;
	else
	{
	    if (!(scr = LockPubScreen (pubname)))
		if (!(scr = LockPubScreen (NULL))) return (NULL);
		
	    nw->Type = PUBLICSCREEN;
	    win = NULL;
	}
    }
    *prwin = win;
    return (nw->Screen = scr);
}

/****************************************************************************************/

#ifndef DO_CM_DEPTH
static int VpDepth (struct ViewPort *vp)
{
    ULONG 	modeid = GetVPModeID (vp);
    int 	depth;

    depth = vp->RasInfo->BitMap->Depth;
    
    if (modeid & HAM_KEY) depth -= 2;
    if (modeid & EXTRAHALFBRITE_KEY) depth = 5;

    if( depth > 8 )
    {
	depth = 8;
    }

    return (depth);
}
#endif

/****************************************************************************************/

int REGARGS
GetVpCM( struct ViewPort *vp, APTR *cmap)
{

#ifdef DO_CM_DEPTH
    int numcols;

    if( !vp->ColorMap )
    {
	    return( 0 );
    }

    numcols = vp->ColorMap->Count;
#else
    int depth, numcols;

    if( !vp->ColorMap )
    {
	return( 0 );
    }

    depth = VpDepth (vp);
    numcols = (1 << depth);
#endif
    if( GfxBase->LibNode.lib_Version >= 39 )
    {
	if( ( *cmap = AllocVec( ( numcols * 3 + 2 ) * 4, MEMF_PUBLIC | MEMF_CLEAR ) ) )
	{
	    ( ( UWORD * ) ( *cmap ) )[ 0 ] = numcols;
	}
    }
    else
    {
	*cmap = AllocVec( numcols * 2, MEMF_PUBLIC | MEMF_CLEAR );
    }

    if( *cmap )
    {
	RefreshVpCM( vp, *cmap );
	return( depth );
    }
    else
    {
	return( 0 );
    }
}

/****************************************************************************************/

void REGARGS
RefreshVpCM( struct ViewPort *vp, APTR cmap )
{
#ifdef DO_CM_DEPTH
    int numcols, i;

    numcols = vp->ColorMap->Count;
#else
    int i, depth, numcols;

    depth = VpDepth( vp );
    numcols = ( 1 << depth );
#endif
    if( GfxBase->LibNode.lib_Version >= 39 )
    {
	GetRGB32( vp->ColorMap, 0, numcols, ( ( ULONG * ) cmap ) + 1 );
    }
    else
    {
	for( i = 0; i < numcols; i++ )
	{
	    ( ( UWORD * ) cmap )[ i ] = GetRGB4( vp->ColorMap, i );
	}
    }
}

/****************************************************************************************/

void REGARGS LoadCMap (struct ViewPort *vp, APTR cmap)
{
    if (GfxBase->LibNode.lib_Version >= 39) LoadRGB32 (vp, cmap);
#ifdef DO_CM_DEPTH
    else LoadRGB4 (vp, cmap, vp->ColorMap->Count);
#else
    else LoadRGB4 (vp, cmap, (1 << VpDepth (vp)));
#endif
}

/****************************************************************************************/

void REGARGS FreeVpCM (struct ViewPort *vp, APTR cmap, BOOL restore)
{
    if (restore && cmap) LoadCMap (vp, cmap);
    FreeVec (cmap);
}

/****************************************************************************************/

void REGARGS InitNewGadget (struct NewGadget *ng,
		int x, int y, int w, int h, char *s, UWORD id)
{
    ng->ng_LeftEdge = x; ng->ng_TopEdge = y; ng->ng_Width = w; ng->ng_Height = h;
    ng->ng_GadgetText = s; ng->ng_GadgetID = id;
}

/****************************************************************************************/

struct TextFont * REGARGS GetReqFont (struct TextAttr *attr,
		struct TextFont *deffont, int *fontheight, int *fontwidth, int allowprop)
{
    struct TextFont 	*ft;
    int 		forcedef;

    forcedef = (rtLockPrefs()->Flags & RTPRF_DEFAULTFONT);
    rtUnlockPrefs();

    ft = OpenFont (attr);
    if (!ft || forcedef || (!allowprop && (ft->tf_Flags & FPF_PROPORTIONAL)))
    {
	if (ft) CloseFont (ft);
	if (deffont) ft = deffont;
	else ft = GfxBase->DefaultFont;
	
	attr->ta_Name = ft->tf_Message.mn_Node.ln_Name;
	attr->ta_YSize = ft->tf_YSize;
	attr->ta_Style = ft->tf_Style;
	attr->ta_Flags = ft->tf_Flags;
	ft = OpenFont (attr);
    }
    
    if (ft)
    {
	*fontheight = ft->tf_YSize;
	*fontwidth = ft->tf_XSize;
    }
    
    return (ft);
}

/****************************************************************************************/

struct IntuiMessage *REGARGS GetWin_GT_Msg (struct Window *win,
					    struct Hook *hook, APTR req)
{
    struct IntuiMessage *imsg, *reqmsg;

    while ((imsg = (struct IntuiMessage *)GetMsg (win->UserPort)))
	if ((reqmsg = ProcessWin_Msg (win, imsg, hook, req))) return (reqmsg);
	
    return (NULL);
}

/****************************************************************************************/

struct IntuiMessage *REGARGS ProcessWin_Msg (struct Window *win,
					    struct IntuiMessage *imsg, struct Hook *hook, APTR req)
{
    struct IntuiMessage *reqmsg;

    if (imsg->IDCMPWindow == win)
    {
	reqmsg = GT_FilterIMsg (imsg);
	if (reqmsg) return (reqmsg);
	ReplyMsg ((struct Message *)imsg);
    }
    else
    {
	if (hook) CallHookPkt (hook, req, imsg);
	ReplyMsg ((struct Message *)imsg);
    }
    
    return (NULL);
}

/****************************************************************************************/

void REGARGS Reply_GT_Msg (struct IntuiMessage *reqmsg)
{
    ReplyMsg ((struct Message *)GT_PostFilterIMsg (reqmsg));
}

/****************************************************************************************/

void REGARGS DoScreenToFront (struct Screen *scr, int nopop, int popup)
{
    ULONG noscrtofront;

    if (nopop) return;
    
    noscrtofront = (rtLockPrefs()->Flags & RTPRF_NOSCRTOFRONT);
    rtUnlockPrefs();
    
    if (noscrtofront) return;
    if (popup)
	    ScreenToFront (scr);
    else
	    rtScreenToFrontSafely (scr);
}


/****************************************************************************************/

void REGARGS DoCloseWindow (struct Window *win, int idcmpshared)
{
    if (idcmpshared) rtCloseWindowSafely (win);
    else CloseWindow (win);
}

/****************************************************************************************/

struct BackFillMsg
{
    struct Layer *layer;
    struct Rectangle bounds;
    LONG offsetx;
    LONG offsety;
};

/****************************************************************************************/

#ifdef __AROS__
AROS_UFH3(void, WinBackFill,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(struct RastPort *, the_rp, A2),
    AROS_UFHA(struct BackFillMsg *, msg, A1))
{
    AROS_USERFUNC_INIT
#else
void SAVEDS ASM WinBackFill (
	REGPARAM(a0, struct Hook *, hook),
	REGPARAM(a2, struct RastPort *, the_rp),
	REGPARAM(a1, struct BackFillMsg *, msg))
{
#endif
    struct RastPort rp;

    memcpy( &rp, the_rp, sizeof( rp ) );
    rp.Layer = NULL;
    SetAPen (&rp, ((UWORD *)hook->h_Data)[BACKGROUNDPEN]);
    mySetWriteMask (&rp, ~0);
    RectFill (&rp, msg->bounds.MinX, msg->bounds.MinY,
		   msg->bounds.MaxX, msg->bounds.MaxY);

#ifdef __AROS__
    AROS_USERFUNC_EXIT
#endif
}

/****************************************************************************************/

#ifdef __AROS__

AROS_UFH3(void, PatternWinBackFill,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(struct RastPort *, the_rp, A2),
    AROS_UFHA(struct BackFillMsg *, msg, A1))
{
    AROS_USERFUNC_INIT
    
    struct RastPort rp;
    UWORD pattern[] = {0xAAAA,0x5555};
    
    memcpy( &rp, the_rp, sizeof( rp ) );
    rp.Layer = NULL;

    SetAPen (&rp, ((UWORD *)hook->h_Data)[BACKGROUNDPEN]);
    SetBPen (&rp, ((UWORD *)hook->h_Data)[SHINEPEN]);
    SetDrMd (&rp, JAM2);
    
    mySetWriteMask (&rp, ~0);
    
    SetAfPt(&rp, pattern, 1);
    
    RectFill (&rp, msg->bounds.MinX, msg->bounds.MinY,
		   msg->bounds.MaxX, msg->bounds.MaxY);
		   
    SetAfPt(&rp, NULL, 0);
    
    AROS_USERFUNC_EXIT
}

#endif

/****************************************************************************************/

void REGARGS mySetWriteMask (struct RastPort *rp, ULONG mask)
{
    if (GfxBase->LibNode.lib_Version >= 39) SetWriteMask (rp, mask);
    else SetWrMsk (rp, (UBYTE)mask);
}

/****************************************************************************************/

struct Window *REGARGS OpenWindowBF (struct NewWindow *nw,
			struct Hook *hook, UWORD *pens, ULONG *maskptr, WORD *zoom,
			BOOL backfillpattern)
{
    struct RastPort *rp;
    struct Window *win;
    ULONG tags[5], mask;
    UWORD maxpen = 0;
    int i;

#ifdef __AROS__
    if (backfillpattern)
    {
        hook->h_Entry = (ULONG (*)())PatternWinBackFill;
    } else
#endif
    hook->h_Entry = (ULONG (*)())WinBackFill;

    hook->h_Data = (void *)pens;

    tags[0] = WA_BackFill;
    tags[1] = (ULONG)hook;
    tags[2] = WA_Zoom;
    tags[3] = (ULONG)zoom;
    tags[4] = TAG_END;

    if( zoom )
    {
	if (IntuitionBase->LibNode.lib_Version >= 39)
	    zoom[0] = zoom[1] = ~0;
	else
	{
	    zoom[0] = nw->LeftEdge;
	    zoom[1] = nw->TopEdge;
	}
    }
    else
    {
	tags[2] = TAG_IGNORE;
    }

    if ((win = OpenWindowTagList (nw, (struct TagItem *)tags)))
    {
	rp = win->RPort;
	for (i = 0; i <= HIGHLIGHTTEXTPEN; i++)
		if (pens[i] > maxpen) maxpen = pens[i];
		
	mask = 1;
	while (mask < maxpen)
	{
	    mask <<= 1;
	    mask |= 1;
	}
	
	mySetWriteMask (rp, mask);
	if (maskptr) *maskptr = mask;
    }
    
    return (win);

}

/****************************************************************************************/

int CheckReqPos (int reqpos, int reqdefnum, struct NewWindow *newwin)
{
    struct ReqDefaults *reqdefs;

    if (reqpos == REQPOS_DEFAULT)
    {
	reqdefs = &rtLockPrefs()->ReqDefaults[reqdefnum];
	reqpos = reqdefs->ReqPos;
	
	if (reqpos <= REQPOS_CENTERSCR)
	{
	    newwin->LeftEdge = 0;
	    newwin->TopEdge = 0;
	}
	else
	{
	    newwin->LeftEdge = reqdefs->LeftOffset;
	    newwin->TopEdge = reqdefs->TopOffset;
	}
	
	rtUnlockPrefs();
    }
    
    return (reqpos);
}

/****************************************************************************************/

/*********
* Layout *
*********/

/****************************************************************************************/

int REGARGS StrWidth_noloc (struct IntuiText *itxt, UBYTE *str)
{
    char labstr[100], *l;

    if (!str) return (0);
    
    /* Copy string, remove underscore */
    l = labstr;
    while (*str && *str != '_') *l++ = *str++;
    
    if (*str) while ((*l++ = *++str)); else *l = 0;
    
    itxt->IText = labstr;
    
    return (IntuiTextLength (itxt));
}

/****************************************************************************************/

static int ObjectWidth (struct NewGadget *ng, int normalw, int normalh)
{
    if ((GadToolsBase->lib_Version >= 39) && (ng->ng_TextAttr->ta_YSize > normalh))
	return (normalw + (ng->ng_TextAttr->ta_YSize - normalh) * 2);
	
    return (normalw);
}

/****************************************************************************************/

static int ObjectHeight (struct NewGadget *ng, int normalh)
{
    if ((GadToolsBase->lib_Version >= 39) && (ng->ng_TextAttr->ta_YSize > normalh))
	return (ng->ng_TextAttr->ta_YSize);
	
    return (normalh);
}

/****************************************************************************************/

int CheckBoxWidth (struct NewGadget *ng)
{
    return (ObjectWidth (ng, CHECKBOX_WIDTH, CHECKBOX_HEIGHT));
}

/****************************************************************************************/

int CheckBoxHeight (struct NewGadget *ng)
{
    return (ObjectHeight (ng, CHECKBOX_HEIGHT));
}

/****************************************************************************************/

/*
 * SIZEGAD HEIGHT
 */
LONG BottomBorderHeight (struct Screen *scr)
{
    struct DrawInfo 	*dri;
    APTR 		obj;
    LONG 		h = 10;

    if ((dri = GetScreenDrawInfo (scr)))
    {
	if((obj = NewObject (NULL, "sysiclass", SYSIA_DrawInfo, (IPTR) dri,
						/* Must be SYSISIZE_MEDRES! */
						SYSIA_Size, SYSISIZE_MEDRES,
						SYSIA_Which, SIZEIMAGE,
						TAG_DONE)))
	{
	    if (!GetAttr (IA_Height, obj, (ULONG *)&h))
		    h = 10;  // Probably not needed.. Or?
	    DisposeObject( obj );
	}
	FreeScreenDrawInfo (scr, dri);
    }
    return (h);
}

/****************************************************************************************/
