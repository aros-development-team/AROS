
#include <exec/types.h>
#include <exec/io.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/sghooks.h>
#include <intuition/imageclass.h>
#include <libraries/dos.h>
#include <libraries/gadtools.h>
#include <libraries/reqtools.h>
#include <proto/exec.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/reqtools.h>
#include <string.h>

#include "filereq.h"


#include "rtlocale.h"

/****************************************************************************************/

#ifndef __AROS__

#ifdef __SASC
#pragma libcall ReqToolsBase rtLockPrefs a8 00
#pragma libcall ReqToolsBase rtUnlockPrefs ae 00
#else
#warning You might have to fix rtLockPrefs/rtUnlockPrefs for your compiler
#endif

#endif

/****************************************************************************************/

#define ThisProcess()	( ( struct Process * ) FindTask( NULL ) )

/****************************************************************************************/

struct PWCallBackArgs
{
    char 	*buffer;
    ULONG 	lastchecksum, verify, retcode;
};

/****************************************************************************************/

extern ULONG ASM LoopReqHandler (ASM_REGPARAM(a1, struct rtHandlerInfo *,));
extern void REGARGS SetWinTitleFlash (struct Window *, char *);
extern void ShortDelay (void);
extern ULONG ASM myTextLength (ASM_REGPARAM(a1, char *,),
	    	    	       ASM_REGPARAM(a0, struct TextAttr *,),
			       ASM_REGPARAM(a3, UBYTE *,),
			       ASM_REGPARAM(a2, struct Image *,),
			       ASM_REGPARAM(d7, ULONG,));

extern struct ReqToolsBase *ReqToolsBase;
extern struct DosLibrary *DOSBase;
extern struct IntuitionBase *IntuitionBase;
extern struct Library *GadToolsBase;
extern struct GfxBase *GfxBase;

/****************************************************************************************/

struct FmtBuff
{
    long numlines, bufflen;
};

#define DOFMT_COUNTNEWLINES		0
#define DOFMT_COUNTBARS			1

/****************************************************************************************/


extern APTR ASM DofmtCount (ASM_REGPARAM(a0, char *,),
    	    	    	    ASM_REGPARAM(a1, APTR,),
			    ASM_REGPARAM(a3, struct FmtBuff *,),
			    ASM_REGPARAM(d0, int,));
extern APTR STDARGS DofmtArgs (char *, char *,...);


extern void ASM FillBarTable (ASM_REGPARAM(a1, char **,), ASM_REGPARAM(a0, char *,));
extern void ASM FillNewLineTable (ASM_REGPARAM(a1, char **,), ASM_REGPARAM(a0, char *,));

/****************************************************************************************/

typedef struct Req_RealHandlerInfo	Req_GlobData;
#define STRINGGADID			32

struct Req_RealHandlerInfo
{
    ULONG 			(*func)();        /* private */
    ULONG 			WaitMask;
    ULONG 			DoNotWait;

    /* PRIVATE */
    char 			**gadstrbuff, **buff, *stringbuff, *textfmt;
    struct PWCallBackArgs 	arg;
    struct Gadget 		*strgad, *yesgad, *nogad, *retgad, *selgad;
    struct TextAttr 		boldattr;
    struct KeyButtonInfo 	buttoninfo;
    struct StringInfo 		*strinfo;
    struct Screen 		*scr, *frontscr;
    struct Window 		*prwin;
    struct Window 		*reqwin;
    struct DrawInfo 		*drinfo;
    struct rtReqInfo 		*reqinfo;
    struct Hook 		*imsghook, backfillhook;
    struct TextFont 		*reqfont;
    struct Catalog 		*catalog;
    struct Image 		headimg;
    struct NewWindow 		newreqwin;
    struct FmtBuff 		bodyfmt, gadfmtbuff;
    UBYTE 			minmaxstr[30];
    int 			idcmp, mode, min, max, checksum, pubscr, reqflags;
    int 			retnum, waitpointer, allowempty, lockwindow, shareidcmp, noscreenpop;
    int 			textht, texttop, strgadht, strgadtop, width, nowinbackfill;
    int 			numlines, len, fontht, minmax, minmaxlen, minmaxtop, minmaxleft;
    int 			fkeys;
    APTR 			winlock, visinfo;
    ULONG 			*value, *lenptr;
};

/****************************************************************************************/

static UWORD pattern[] = { 0xAAAA,0x5555 };

static ULONG REGARGS ReqExit (Req_GlobData *, int);
static struct Image * REGARGS CreateRectImage
			(Req_GlobData *, struct Image *, int, int, int, int, int, int);
static ULONG ASM SAVEDS myReqHandler (REGPARAM(a1, Req_GlobData *,),
			    	      REGPARAM(d0, ULONG,),
				      REGPARAM(a0, struct TagItem *,));

/****************************************************************************************/

#define GETSTRINGLONG_FLAGS	(GSREQF_CENTERTEXT|GSREQF_HIGHLIGHTTEXT)
#define EZREQ_FLAGS		(EZREQF_NORETURNKEY|EZREQF_LAMIGAQUAL|EZREQF_CENTERTEXT)


/****************************************************************************************/

ULONG ASM SAVEDS GetString (
	REGPARAM(a1, UBYTE *, stringbuff),		/* str in case of rtEZRequestA */
	REGPARAM(d0, LONG, maxlen),			/* args in case of rtEZRequestA */
	REGPARAM(a2, char *, title),			/* gadfmt in case of rtEZRequestA */
	REGPARAM(d1, ULONG, checksum),
	REGPARAM(d2, ULONG *, value),
	REGPARAM(d3, LONG, mode),
	REGPARAM(d4, struct rtReqInfo *, reqinfo),
	REGPARAM(a0, struct TagItem *, taglist))
{
/* #define CLEARSIZE	(28+sizeof(struct NewWindow)+sizeof(struct IntuiText)+\
				 sizeof(struct NewGadget)+2*sizeof(struct FmtBuff)) */
				 
    /* keep these vars together and just BEFORE NewWindow struct! */
    /*-------------------------------------------------*/
    int 		reqpos = REQPOS_DEFAULT;
    char 		*pubname = NULL;
    ULONG 		underscore = 0;
    struct TextAttr 	*fontattr = NULL;
    struct Locale 	*locale = NULL;
    struct IntuiText 	itxt, *bodyitxt = NULL;
    struct NewGadget 	ng;
    struct Image 	*img;
    /**/
    /* KEEP MIN AND MAX IN THIS ORDER !!!! */
    int 		max = MAXINT, min = MININT;
    /**/
    Req_GlobData	*glob;
    struct Gadget 	*gad;
    struct TagItem 	*tag;
    const struct TagItem *tstate;
    /* for rtEZRequestA */
    char 		*gadfmt = title;
    char 		*ptr;
    int 		val, spacing, reqhandler = FALSE, nogadfmt, gadlen = 0;
    int 		height, top, showdef = TRUE;
    int 		scrwidth, scrheight, i, j, npos, nlen, nogadgets, retnum;
    int 		invisible, scrfontht, gadlines = 0;
    int 		leftoff, rightoff;
    ULONG 		*gadlenptr = NULL, *gadposptr = NULL, idcmpflags;
    APTR 		gadfmtargs = NULL, textfmtargs = NULL, args;

    memset (&itxt, 0, sizeof (struct IntuiText));
    memset (&ng, 0, sizeof (struct NewGadget));

    if (!(glob = AllocVec (sizeof (Req_GlobData), MEMF_PUBLIC | MEMF_CLEAR)))
	return (FALSE);
	    
    glob->mode = mode;
    glob->checksum = checksum;
    glob->value = value;
    glob->stringbuff = stringbuff;
    glob->fkeys = rtLockPrefs()->Flags & RTPRF_FKEYS;
    rtUnlockPrefs();

    nogadfmt = (mode != IS_EZREQUEST);
    invisible = (mode <= ENTER_PASSWORD);

    if (mode == IS_EZREQUEST) title = NULL;
    if (mode == ENTER_STRING) glob->width = 350; else glob->width = 180;
    retnum = 1;
    if ((glob->reqinfo = reqinfo))
    {
	if (reqinfo->Width) glob->width = reqinfo->Width;
	if (reqinfo->ReqTitle) title = reqinfo->ReqTitle;
	if (reqinfo->ReqPos != REQPOS_DEFAULT) reqpos = reqinfo->ReqPos;
	
	glob->newreqwin.LeftEdge = reqinfo->LeftOffset;
	glob->newreqwin.TopEdge = reqinfo->TopOffset;
	glob->reqflags = reqinfo->Flags;
	glob->waitpointer = reqinfo->WaitPointer;
	glob->lockwindow = reqinfo->LockWindow;
	glob->shareidcmp = reqinfo->ShareIDCMP;
	glob->imsghook = reqinfo->IntuiMsgFunc;
    }
	    
    /* parse tags */
    tstate = taglist;
    while ((tag = NextTagItem (&tstate)))
    {
	IPTR tagdata = tag->ti_Data;
	if (tag->ti_Tag > RT_TagBase)
	{
	    switch (tag->ti_Tag)
	    {
		case RT_Window:			glob->prwin = (struct Window *)tagdata; break;
		case RT_IDCMPFlags:		glob->idcmp = tagdata; break;
		case RT_ReqPos:			reqpos = tagdata; break;
		case RT_LeftOffset:		glob->newreqwin.LeftEdge = tagdata; break;
		case RT_TopOffset:		glob->newreqwin.TopEdge = tagdata; break;
		case RT_PubScrName:		pubname = (char *)tagdata; break;
		case RT_Screen:			glob->scr = (struct Screen *)tagdata; break;
		case RT_ReqHandler:		*(APTR *)tagdata = glob;
										reqhandler = TRUE;
										break;
		case RT_WaitPointer:		glob->waitpointer = tagdata; break;
		case RT_Underscore:		underscore = tagdata; break;
		case RT_ShareIDCMP:		glob->shareidcmp = tagdata; break;
		case RT_LockWindow:		glob->lockwindow = tagdata; break;
		case RT_ScreenToFront:		glob->noscreenpop = !tagdata; break;
		case RT_TextAttr:			fontattr = (struct TextAttr *)tagdata; break;
		case RT_IntuiMsgFunc:		glob->imsghook = (struct Hook *)tagdata; break;
		case RT_Locale:			locale = (struct Locale *)tagdata; break;
		case RTEZ_ReqTitle:		if (mode == IS_EZREQUEST) title = (char *)tagdata;
										break;
		/* RTGS_Flags == RTGL_Flags == RTEZ_Flags */
		case RTEZ_Flags:		glob->reqflags = tagdata; break;
		case RTEZ_DefaultResponse: 	retnum = tagdata; break;
		case RTGL_Min:			min = tagdata; glob->minmax = TRUE; break;
		case RTGL_Max:			max = tagdata; glob->minmax = TRUE; break;
		/* RTGS_Width == RTGL_Width */
		case RTGL_Width:		if (mode == ENTER_NUMBER || mode == ENTER_STRING)
											glob->width = tagdata;
										break;
		case RTGL_ShowDefault:	showdef = tagdata; break;
		/* RTGS_GadFmt == RTGL_GadFmt */
		case RTGL_GadFmt:		nogadfmt = FALSE;
						gadfmt = (char *)tagdata;
						break;
		/* RTGS_GadFmtArgs == RTGL_GadFmtArgs */
		case RTGL_GadFmtArgs:   	gadfmtargs = (APTR)tagdata; break;
		/* RTGS_Invisible == RTGL_Invisible */
		case RTGL_Invisible:		invisible = tagdata; break;
		/* RTGS_BackFill == RTGL_BackFill */
		case RTGL_BackFill:		if (mode == ENTER_NUMBER || mode == ENTER_STRING)
						    glob->nowinbackfill = !tagdata;
						    break;
		/* RTGS_TextFmt == RTGL_TextFmt */
		case RTGL_TextFmt:		if (mode == ENTER_NUMBER || mode == ENTER_STRING)
						    glob->textfmt = (char *)tagdata;
						    break;
		/* RTGS_TextFmtArgs == RTGL_TextFmtArgs */
		case RTGL_TextFmtArgs:		textfmtargs = (APTR)tagdata; break;
		case RTGS_AllowEmpty:		glob->allowempty = tagdata; break;
		
	    } /* switch (tag->ti_Tag) */
	    
	} /* if (tag->ti_Tag > RT_TagBase)*/
	
    } /* while ((tag = NextTagItem (&tstate))) */

    glob->catalog = RT_OpenCatalog (locale);

    /* ignore RTGL_Min and RTGL_Max if not rtNewGetLongA() */
    if (mode != ENTER_NUMBER) glob->minmax = FALSE;
    retnum++;
    glob->newreqwin.Flags = WFLG_DEPTHGADGET|WFLG_DRAGBAR|WFLG_ACTIVATE
			    |WFLG_SIMPLE_REFRESH|WFLG_RMBTRAP;

    idcmpflags = glob->idcmp | IDCMP_REFRESHWINDOW|IDCMP_GADGETUP|IDCMP_RAWKEY;
    if (mode != IS_EZREQUEST) idcmpflags |= IDCMP_MOUSEBUTTONS|IDCMP_ACTIVEWINDOW;

    if (!glob->prwin || !glob->prwin->UserPort
		     || (glob->prwin->UserPort->mp_SigTask != ThisProcess()))
	glob->shareidcmp = FALSE;

    if (!(glob->scr = GetReqScreen (&glob->newreqwin, &glob->prwin, glob->scr, pubname)))
	return (ReqExit (glob, FALSE));
	
    spacing = rtGetVScreenSize (glob->scr, (ULONG *)&scrwidth, (ULONG *)&scrheight);

    if (fontattr)
    {
	if (!(glob->reqfont = OpenFont (fontattr))) fontattr = NULL;
    }
    
    if (!fontattr) fontattr = glob->scr->Font;

    if (!(glob->visinfo = GetVisualInfoA (glob->scr, NULL))
	|| !(glob->drinfo = GetScreenDrawInfo (glob->scr)))
	return (ReqExit (glob, FALSE));

    itxt.ITextFont = fontattr;
    glob->boldattr = *fontattr;
    glob->boldattr.ta_Style |= FSF_BOLD;
    glob->fontht = fontattr->ta_YSize;
    scrfontht = glob->scr->Font->ta_YSize;
    leftoff = glob->scr->WBorLeft + 4;
    rightoff = glob->scr->WBorRight + 4;

    /* Calculate the width, height and position of the requester window. We try
	    to position the window as close to the mouse as possible (default). */

    if (mode != IS_EZREQUEST)
    {
	if (nogadfmt)
	{
	    underscore = '_';
	    gadfmt = GetStr (glob->catalog, MSG_OK_BAR_CANCEL);
	    if (mode <= ENTER_PASSWORD)
	    {
		gadfmt = GetStr (glob->catalog, MSG_LAST_BAR_CANCEL);
		if (!stringbuff[0] || mode == CHECK_PASSWORD)
		    while (*gadfmt && (*gadfmt++ != '|'));
	    }
	}
	glob->reqflags &= GETSTRINGLONG_FLAGS;
	glob->reqflags |= EZREQF_NORETURNKEY;
    }
    else
    {
	glob->reqflags &= EZREQ_FLAGS;
	glob->textfmt = stringbuff;
	textfmtargs = (APTR)maxlen;
    }

    if (glob->textfmt)
    {
	/* Calculate size of buffer needed to expand format string, also
		calculates number of lines in format string.
		(APTR)maxlen points to the arguments! */

	DofmtCount (glob->textfmt, textfmtargs, &glob->bodyfmt, DOFMT_COUNTNEWLINES);
	glob->numlines = glob->bodyfmt.numlines;
	
	if (!(glob->buff = (char **)AllocVec (glob->bodyfmt.bufflen
			   + (8 + (int)sizeof (struct IntuiText)) * glob->numlines, MEMF_PUBLIC)))
	    return (ReqExit (glob, FALSE));

	/* expand format string and fill in table of pointers to each line */
	glob->lenptr = (ULONG *)&glob->buff[glob->numlines];
	bodyitxt = (struct IntuiText *)&glob->lenptr[glob->numlines];
	ptr = (char *)&bodyitxt[glob->numlines];
	args = Dofmt (ptr, glob->textfmt, textfmtargs);

	if (mode == IS_EZREQUEST) gadfmtargs = args;
	FillNewLineTable (glob->buff, ptr);

	/* Calculate width on screen of each line, remember largest */
	for (i = 0, glob->len = 0; i < glob->numlines; i++)
	{
	    itxt.IText = (UBYTE *)glob->buff[i];
	    j = glob->lenptr[i] = IntuiTextLength (&itxt);
	    if (j > glob->len) glob->len = j;
	}
	glob->width = glob->len + 70;
    }
    
    nogadgets = (gadfmt == NULL);

    if (!nogadgets)
    {
	DofmtCount (gadfmt, gadfmtargs, &glob->gadfmtbuff, DOFMT_COUNTBARS);
	gadlines = glob->gadfmtbuff.numlines;
	glob->gadfmtbuff.bufflen += 12 * gadlines;
	if (!(glob->gadstrbuff = (char **)AllocVec (glob->gadfmtbuff.bufflen, MEMF_PUBLIC)))
	    return (ReqExit (glob, FALSE));
		
	gadlenptr = (ULONG *)&glob->gadstrbuff[gadlines];
	gadposptr = (ULONG *)&gadlenptr[gadlines];
	ptr = (char *)&gadposptr[gadlines];
	Dofmt (ptr, gadfmt, gadfmtargs);
	FillBarTable (glob->gadstrbuff, ptr);

	for (i = 0; i < gadlines; i++)
	{
	    UBYTE underscorechar = (UBYTE)underscore;
	    
	    gadlenptr[i] = myTextLength (glob->gadstrbuff[i], fontattr, &underscorechar, NULL, 0) + 24;
	    gadlen += gadlenptr[i];
	}
	
    } /* if (!nogadgets) */
    
    /* else gadlines = 0; is always NULL (cleared at beginning) */

    if (!title)
    {
	if (gadlines >= 2) title = GetStr (glob->catalog, MSG_REQUEST);
	else title = GetStr (glob->catalog, MSG_INFORMATION);
    }
    
    glob->newreqwin.Title = (UBYTE *)title;

    top = (glob->scr->WBorTop + scrfontht + 1) + spacing;
    val = glob->fontht + 6;

    
    if (mode != IS_EZREQUEST)
    {
#if 1
	/* AROS FIX: calc. was wrong because scr->WBorTop not taken into account. */
	
	height = glob->scr->WBorTop + 13 + glob->fontht * 2 + scrfontht + spacing * 3 + glob->scr->WBorBottom;
	
#else
	height = 15 + glob->fontht * 2 + scrfontht + spacing * 3 + glob->scr->WBorBottom;
#endif

	if (glob->textfmt)
	{
	    glob->texttop = top;
	    glob->textht = (glob->fontht + 1) * glob->numlines + (glob->nowinbackfill ? 0 : 15);
	    height += spacing + glob->textht;
	    top += spacing + glob->textht;
	}
	
	if (glob->minmax)
	{
	    height += glob->fontht + spacing + 4;
	    if (min == 0x80000000)
		    DofmtArgs (glob->minmaxstr, GetStr (glob->catalog, MSG_MAX_FMT), max);
	    else DofmtArgs (glob->minmaxstr, (max != 0x7FFFFFFF) ?
						     GetStr (glob->catalog, MSG_MIN_MAX_FMT) :
						     GetStr (glob->catalog, MSG_MIN_FMT),
						     min, max);
	    itxt.IText = glob->minmaxstr;
	    glob->minmaxlen = IntuiTextLength (&itxt) + 8;
	    if (glob->minmaxlen + 16 > glob->width) glob->width = glob->minmaxlen + 16;
	}
	
	if (glob->width < 180) glob->width = 180;
	
	glob->strgadtop = top;
	glob->strgadht = val;
	
    } /* if (mode != IS_EZREQUEST) */
    else
    {
	glob->texttop = top;
	glob->textht = (glob->fontht + 1) * glob->numlines + 15;
#if 1
	/* AROS FIX: Did not take scr->WBorTop into account */
	height = spacing * 2 + scrfontht + glob->textht + 1 + glob->scr->WBorTop + glob->scr->WBorBottom;
#else
	height = spacing * 2 + scrfontht + glob->textht + 3 + glob->scr->WBorBottom;
#endif
	if (!nogadgets) height += spacing + val;
    }

    i = gadlen + gadlines * 16;
    if (i > glob->width) glob->width = i;

    if (glob->width > scrwidth) glob->width = scrwidth;
    if (height > scrheight) height = scrheight;

    /* Create gadgets */
    gad = (struct Gadget *)CreateContext (&glob->buttoninfo.glist);
    ng.ng_VisualInfo = glob->visinfo;
    ng.ng_TextAttr = fontattr;
    ng.ng_TopEdge = height - spacing - val - glob->scr->WBorBottom;
    ng.ng_Height = val;
    ng.ng_GadgetID = 1;

    if (!nogadgets)
    {
	nlen = gadlenptr[gadlines-1];

	if (gadlines > 1)
	{
	    npos = glob->width - (nlen + rightoff);
	    rtSpread (gadposptr, gadlenptr, gadlen,
		      leftoff, glob->width - rightoff, gadlines);
	}
	else
	{
	    gadposptr[0] = npos = (glob->width - nlen) / 2;
	    retnum = 1;
	}

	for (i = 0; i < gadlines; i++)
	{
	    ng.ng_GadgetID++;
	    if (i == gadlines - 1) ng.ng_GadgetID = 1;
	    
	    ng.ng_LeftEdge = gadposptr[i];
	    ng.ng_Width = gadlenptr[i];
	    ng.ng_GadgetText = glob->gadstrbuff[i];
	    ng.ng_TextAttr = fontattr;
	    
	    if ((val = (ng.ng_GadgetID == retnum
		&& !(glob->reqflags & EZREQF_NORETURNKEY))))
		ng.ng_TextAttr = &glob->boldattr;
		
	    gad = my_CreateButtonGadget (gad, underscore, &ng);
	    
	    if (val) glob->retgad = gad;
	    if (!i) glob->yesgad = gad;
	}
	glob->nogad = gad;
	
    } /* if (!nogadgets) */
    else
    {
	/* glob->nogad = NULL; */
	npos = glob->width / 2;
	nlen = 0;
    }

    if (!glob->nowinbackfill && glob->texttop)
    {
	ng.ng_LeftEdge = leftoff;
	ng.ng_TopEdge = glob->texttop;
	ng.ng_Width = glob->width - (leftoff + rightoff);
	ng.ng_Height = glob->textht;
	ng.ng_GadgetText = NULL;
	
	gad = myCreateGadget (TEXT_KIND, gad, &ng, GTTX_Border, TRUE, TAG_END);
    }

    glob->newreqwin.Width = glob->width;
    glob->newreqwin.Height = height;
    reqpos = CheckReqPos (reqpos, RTPREF_OTHERREQ, &glob->newreqwin);
    
    if (reqpos == REQPOS_POINTER)
    {
	glob->newreqwin.LeftEdge = -npos - nlen / 2;
	glob->newreqwin.TopEdge = -height + glob->fontht / 2 + 5 + spacing;
    }
    
    rtSetReqPosition (reqpos, &glob->newreqwin, glob->scr, glob->prwin);

    ng.ng_Height = glob->fontht + 6;

    if (mode != IS_EZREQUEST)
    {
	glob->minmaxtop = ng.ng_TopEdge = height - 2 * (glob->fontht + spacing) - 10 - glob->scr->WBorBottom;
	ng.ng_GadgetText = NULL;
	
	if (glob->minmax)
	{
	    ng.ng_LeftEdge = glob->minmaxleft = (glob->width - glob->minmaxlen) / 2;
	    ng.ng_Width = glob->minmaxlen;
	    ng.ng_Height -= 2;

	    gad = myCreateGadget (TEXT_KIND, gad, &ng,
			    GTTX_Text, (IPTR) glob->minmaxstr, GTTX_Border, TRUE, TAG_END);
	    ng.ng_Height += 2;
	}
	
	ng.ng_LeftEdge = leftoff;
	ng.ng_Width = glob->width - (leftoff + rightoff);
	ng.ng_TopEdge = glob->strgadtop;
	ng.ng_GadgetID = STRINGGADID;
	
	if (mode <= ENTER_PASSWORD)
	{
	    stringbuff = NULL;
	    maxlen = 16;
	}

	if (mode < ENTER_NUMBER)
	    gad = my_CreateStringGadget (gad, &ng, maxlen, stringbuff);
	else
	    gad = my_CreateIntegerGadget (gad, &ng, 16, *value, GACT_STRINGCENTER);
	    
	glob->strgad = gad;
	if (gad)
	{
	    glob->strinfo = (struct StringInfo *)glob->strgad->SpecialInfo;
	    glob->arg.buffer = glob->strinfo->Buffer;
	    
	    if (mode == ENTER_NUMBER && !showdef) *glob->arg.buffer = 0;
	    if (invisible) *(ULONG *)(glob->strinfo->Extension->Pens) = 0;
	}
	
	/* we do this here because there seems to be a bug in GO! :-( */
	gad = glob->strgad;
	
    } /* if (mode != IS_EZREQUEST) */

    img = &glob->headimg;
    if (!glob->nowinbackfill)
    {
	val = glob->scr->WBorTop + glob->scr->Font->ta_YSize + 1;
	img = CreateRectImage (glob, img, glob->scr->WBorLeft, val,
			  glob->width - glob->scr->WBorLeft - glob->scr->WBorRight,
			  height - val - glob->scr->WBorBottom,
			  SHINEPEN, BACKGROUNDPEN);
			  
	if (glob->minmax)
		img = CreateRectImage (glob, img, glob->minmaxleft, ++glob->minmaxtop,
				  glob->minmaxlen, glob->fontht + 2,
				  BACKGROUNDPEN, BACKGROUNDPEN);
	if (glob->texttop)
		img = CreateRectImage (glob, img, leftoff + 1, glob->texttop + 1,
				  glob->width - (leftoff + rightoff + 2),
				  glob->textht - 2, BACKGROUNDPEN, BACKGROUNDPEN);
				  
	if (glob->strgadtop)
		img = CreateRectImage (glob, img, leftoff + 2, glob->strgadtop,
				  glob->width - (leftoff + rightoff + 4), glob->strgadht,
				  BACKGROUNDPEN, BACKGROUNDPEN);
    }

    if (glob->textfmt)
    {
	/* build list of intuitexts (line by line) */
	itxt.FrontPen = glob->drinfo->dri_Pens[
			(glob->reqflags & GSREQF_HIGHLIGHTTEXT) ? HIGHLIGHTTEXTPEN : TEXTPEN];
	itxt.TopEdge = glob->texttop + (glob->nowinbackfill ? 0 : 8);
	val = (glob->width - glob->len) / 2;
	for (i = 0, j = 0; i < glob->numlines; i++, j += glob->fontht + 1)
	{
	    bodyitxt[i] = itxt;
	    if (glob->reqflags & EZREQF_CENTERTEXT)
		    val = (glob->width - glob->lenptr[i]) / 2;
	    if (val < 35) val = 35;
	    bodyitxt[i].LeftEdge = val;
	    bodyitxt[i].TopEdge += j;
	    bodyitxt[i].IText = glob->buff[i];

	    if (i) bodyitxt[i-1].NextText = &bodyitxt[i];
	}
	
    } /* if (glob->textfmt) */

    if (glob->headimg.NextImage || glob->textfmt)
    {
	ng.ng_LeftEdge = ng.ng_TopEdge = ng.ng_Width = ng.ng_Height = 0;
	ng.ng_GadgetText = NULL;

	gad = myCreateGadget (GENERIC_KIND, gad, &ng, TAG_END);
	if (gad)
	{
	    gad->GadgetType |= GTYP_BOOLGADGET;
#ifdef __AROS__
#warning A workaround here for AROS, because this would overpaint many gadgets
	    /* This seems to rely somehow on how GadTools refreshes gadgets
	       (the order etc.).
	       
	       This gadget is at the end of the gadget list and in GadgetRender
	       contains a linked list of fillrectclass images for the requester
	       background and textbox backgrounds. */
	       
	    gad->Flags |= GFLG_GADGHNONE;
#else
	    gad->Flags |= GFLG_GADGIMAGE|GFLG_GADGHNONE;
	    gad->GadgetRender = (APTR)glob->headimg.NextImage;
#endif
	    gad->GadgetText = &bodyitxt[0];

	}
    }

    if (!gad || (!glob->nowinbackfill && !img)) return (ReqExit (glob, FALSE));

    glob->newreqwin.IDCMPFlags = glob->shareidcmp ? 0 : idcmpflags;


    /* Now open the message window. */
    if (!(glob->reqwin = OpenWindowBF (&glob->newreqwin,
    				       &glob->backfillhook,
				       glob->drinfo->dri_Pens,
				       NULL,
				       NULL,
				       !glob->nowinbackfill)))
	return (ReqExit (glob, FALSE));

    if (glob->shareidcmp)
    {
	glob->reqwin->UserPort = glob->prwin->UserPort;
	ModifyIDCMP (glob->reqwin, idcmpflags);
    }

    AddGList (glob->reqwin, glob->buttoninfo.glist, -1, -1, NULL);
    RefreshGadgets (glob->buttoninfo.glist, glob->reqwin, NULL);
    GT_RefreshWindow (glob->reqwin, NULL);
    glob->winlock = DoLockWindow (glob->prwin, glob->lockwindow, NULL, TRUE);
    DoWaitPointer (glob->prwin, glob->waitpointer, TRUE);

    glob->frontscr = IntuitionBase->FirstScreen;
    DoScreenToFront (glob->scr, glob->noscreenpop, TRUE);

    glob->buttoninfo.win = glob->reqwin;
    glob->min = min;
    glob->max = max;
    glob->pubscr = (glob->newreqwin.Type == PUBLICSCREEN);

    /* fill in RealHandlerInfo */
    glob->func = (ULONG (*)())myReqHandler;
    glob->WaitMask = (1 << glob->reqwin->UserPort->mp_SigBit);
    glob->DoNotWait = TRUE;

    if (reqhandler) return (CALL_HANDLER);
    
    return (LoopReqHandler ((struct rtHandlerInfo *)glob));
}

/****************************************************************************************/

static struct Image * REGARGS CreateRectImage (Req_GlobData *glob,
	struct Image *previmg, int x, int y, int w, int h, int pen, int bgpen)
{
    struct Image *img;

    if (!previmg) return (NULL);
    
    img = NewObject (NULL, "fillrectclass",
		     IA_Left, x, IA_Top, y,
		     IA_Width, w, IA_Height, h,
		     IA_FGPen, glob->drinfo->dri_Pens[pen],
		     IA_BGPen, glob->drinfo->dri_Pens[bgpen],
		     IA_Mode, JAM2,
		     (pen == SHINEPEN) ? IA_APattern : TAG_END, (IPTR) pattern,
		     IA_APatSize, 1,
      		     TAG_END);
      
    previmg->NextImage = img;
    
    return (img);
}

/****************************************************************************************/

#define RETURN_KEY	13
#define ESC_KEY		27
#define SHIFT_KEY	0x60
#define F1_KEY		0x50
#define F10_KEY		0x59

#define QUALS_CONSIDERED	( IEQUALIFIER_LCOMMAND | IEQUALIFIER_RCOMMAND | IEQUALIFIER_LSHIFT | \
				IEQUALIFIER_RSHIFT | IEQUALIFIER_LALT | IEQUALIFIER_RALT | \
				IEQUALIFIER_CONTROL )

/****************************************************************************************/

static ULONG ASM SAVEDS myReqHandler (
	REGPARAM(a1, Req_GlobData *, glob),
	REGPARAM(d0, ULONG, sigs),
	REGPARAM(a0, struct TagItem *, taglist))
{
    struct Gadget 	*tmpgad, *selgad;
    struct TagItem 	*tag;
    const struct TagItem *tstate = taglist;
    struct IntuiMessage *msg;
    ULONG 		class, tagdata;
    UWORD 		code, qual;
    int 		gadid, val, leftamiga, doactgad, copystr;
    char 		*str, key;

    /* uncomment if sigs is no longer ignored */
    //if (glob->DoNotWait) sigs = 0;

    doactgad = (glob->mode != IS_EZREQUEST) && !(glob->buttoninfo.lastcode);

    /* parse tags */
    while ((tag = NextTagItem (&tstate)))
    {
	tagdata = tag->ti_Data;
	if (tag->ti_Tag > RT_TagBase)
	{
	    switch (tag->ti_Tag)
	    {
		case RTRH_EndRequest:
			glob->arg.retcode = tagdata;
			return (ReqExit (glob, tagdata));
	    }
	}
    }

    while ((msg = GetWin_GT_Msg (glob->reqwin, glob->imsghook, glob->reqinfo)))
    {
	class = msg->Class;
	code = msg->Code;
	qual = msg->Qualifier;

	gadid = 0;
	if (class == IDCMP_RAWKEY && glob->nogad)
	{
	    /* Convert key to ASCII and check if a gadget pops up */
	    if (!(gadid = CheckGadgetKey (code, qual, &key, &glob->buttoninfo)))
	    {
		if (!glob->buttoninfo.lastcode && !(qual & IEQUALIFIER_REPEAT))
		{
		    leftamiga = (qual & IEQUALIFIER_LCOMMAND);
		    selgad = NULL;
		    
		    if (key == RETURN_KEY) selgad = glob->retgad;
		    if (key == ESC_KEY) selgad = glob->nogad;
		    
		    if (!(glob->reqflags & EZREQF_LAMIGAQUAL) || leftamiga)
		    {
			switch (key)
			{
			    case 'V': if (!leftamiga) break;
			    case 'Y': selgad = glob->yesgad; break;
			    case 'B': if (!leftamiga) break;
			    case 'N': case 'R': selgad = glob->nogad; break;
			}
		    }
		    
		    if ( ( glob->fkeys ) &&
			 !(qual & QUALS_CONSIDERED) &&
			 (code >= F1_KEY) && (code <= F10_KEY) &&
			 (code - F1_KEY < glob->gadfmtbuff.numlines))
		    {
			LONG	i = code - F1_KEY;

			selgad = glob->yesgad;

			while( i && selgad )
			{
			    selgad = selgad->NextGadget;
			    --i;
			}
		    }

		    if (selgad) my_DownGadget (selgad, code, &glob->buttoninfo);
		    
		} /* if (!glob->buttoninfo.lastcode && !(qual & IEQUALIFIER_REPEAT)) */
		
	    } /* if (!(gadid = CheckGadgetKey (code, qual, &key, &glob->buttoninfo))) */
	    
	} /* if (class == IDCMP_RAWKEY && glob->nogad) */
		
	tmpgad = (struct Gadget *)msg->IAddress;
	Reply_GT_Msg (msg);
	
	if (class == IDCMP_REFRESHWINDOW)
	{
	    GT_BeginRefresh (glob->reqwin);
	    GT_EndRefresh (glob->reqwin, TRUE);
	    continue;
	}
	
	if (class == IDCMP_GADGETUP || gadid)
	{
	    if (!gadid) gadid = tmpgad->GadgetID;
	    
	    if (gadid < STRINGGADID)
	    {
		if (gadid == 1 || glob->mode == IS_EZREQUEST
		    || glob->mode == ENTER_PASSWORD)
		{
		    glob->arg.retcode = gadid - 1;
		    return (ReqExit (glob, FALSE));
		}
	    }
	    
	    if (glob->mode == IS_EZREQUEST) continue;
	    
	    if (glob->mode > ENTER_PASSWORD)
	    {
		if (gadid == STRINGGADID)
		{
		    if (code == 1)
		    {
			doactgad = FALSE;
			continue;
		    }

		    my_SelectGadget ((*glob->arg.buffer || glob->allowempty)
				    ? glob->yesgad : glob->nogad, glob->reqwin);
		    ShortDelay();
		}
			
		if (glob->mode == ENTER_STRING)
		{
		    copystr = (glob->arg.buffer[0] != 0);
		    glob->arg.retcode = copystr;
		    if (glob->allowempty) copystr = glob->arg.retcode = TRUE;
		    if (gadid > 2 && gadid < STRINGGADID) glob->arg.retcode = gadid - 1;
		    return (ReqExit (glob, copystr));
		}
		
		/* glob->mode == ENTER_NUMBER */
		if (glob->arg.buffer[0])
		{
		    val = glob->strinfo->LongInt;
		    str = NULL;
		    if (val < glob->min) str = GetStr (glob->catalog, MSG_TOO_SMALL);
		    else if (val > glob->max) str = GetStr (glob->catalog, MSG_TOO_BIG);
		    if (str)
		    {
			if (gadid == STRINGGADID)
				my_SelectGadget (glob->yesgad, glob->reqwin);
			SetWinTitleFlash (glob->reqwin, str);
			continue;
		    }
		    *glob->value = val;
		}
			
		glob->arg.retcode = (glob->arg.buffer[0] != 0);
		if (gadid > 2 && gadid < STRINGGADID) glob->arg.retcode = gadid - 1;
		
		return (ReqExit (glob, FALSE));
		
	    } /* if (glob->mode > ENTER_PASSWORD) */
		    
	    if (gadid == STRINGGADID)
	    {
		if (code == 1)
		{
		    doactgad = FALSE;
		    continue;
		}
	    }
		    
	    if (glob->arg.buffer[0])
	    {
		if ((str = ((PWCALLBACKFUNPTR)
			   glob->value)(glob->mode, glob->checksum, &glob->arg)))
		{
		    /* Check if return was 'Please verify', MAJOR HACK !! */
		    if (*str != 'P') DisplayBeep (glob->scr);
		    SetWindowTitles (glob->reqwin, str, (char *)~0);
		    my_SetStringGadget (glob->reqwin, glob->strgad, "");
		}
		else return (ReqExit (glob, glob->arg.retcode));
	    }
	    
	} /* if (glob->mode > ENTER_PASSWORD) */
	else if (class & glob->idcmp)
	{
	    glob->arg.retcode = glob->idcmp;
	    return (ReqExit (glob, FALSE));
	}
	
    } /* while ((msg = GetWin_GT_Msg (glob->reqwin, glob->imsghook, glob->reqinfo))) */

    if (doactgad)
    {
        ActivateGadget (glob->strgad, glob->reqwin, NULL);
    }
    
    glob->DoNotWait = FALSE;
    
    return (CALL_HANDLER);
}

/****************************************************************************************/

static ULONG REGARGS ReqExit (Req_GlobData *glob, int cpystr)
{
    ULONG 		ret = glob->arg.retcode;
    struct Image 	*img, *img2;

    if (cpystr && glob->mode <= ENTER_STRING)
	    strcpy (glob->stringbuff, glob->arg.buffer);
	    
    DoScreenToFront (glob->frontscr, glob->noscreenpop, FALSE);
    
    if (glob->reqwin)
    {
	DoLockWindow (glob->prwin, glob->lockwindow, glob->winlock, FALSE);
	DoWaitPointer (glob->prwin, glob->waitpointer, FALSE);
	DoCloseWindow (glob->reqwin, glob->shareidcmp);
    }
    
    RT_CloseCatalog (glob->catalog);
    my_FreeGadgets (glob->buttoninfo.glist);
    img = glob->headimg.NextImage;
    
    while (img)
    {
	img2 = img->NextImage;
	DisposeObject (img);
	img = img2;
    }
    
    FreeVec (glob->buff);
    FreeVec (glob->gadstrbuff);
    
    if (glob->drinfo) FreeScreenDrawInfo (glob->scr, glob->drinfo);
    
    FreeVisualInfo (glob->visinfo);
    
    if (glob->pubscr) UnlockPubScreen (NULL, glob->scr);
    if (glob->reqfont) CloseFont (glob->reqfont);
    
    FreeVec (glob);
    
    return (ret);
}

/****************************************************************************************/
