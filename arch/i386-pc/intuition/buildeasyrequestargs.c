/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: Intuition function BuildEasyRequestArgs()
    Lang: english
*/

#include "intuition_intern.h"
#include <stdio.h>
#include <stdarg.h>
#include <strings.h>
#include <clib/macros.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/boopsi.h>
#include <exec/memory.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <intuition/screens.h>
#include <graphics/rastport.h>
#include <graphics/gfxmacros.h>
#include <utility/tagitem.h>

char *strchr(const char*,int);

#define HSPACE 4
#define HBORDER 4
#define VSPACE 2
#define VBORDER 2


struct reqdims
{
    UWORD width;       /* width of the requester */
    UWORD height;      /* height of the requester */
    UWORD fontheight;  /* height of the default font */
    int   gadgets;     /* number of gadgets */
    UWORD gadgetwidth; /* width of a gadget */
};


STRPTR *buildeasyreq_makelabels(struct reqdims *dims,STRPTR labeltext);
STRPTR buildeasyreq_formattext(STRPTR textformat, APTR args);
BOOL buildeasyreq_calculatedims(struct reqdims *dims,
				struct Screen *scr,
				STRPTR formattedtext,
				STRPTR *gadgetlabels,
				struct IntuitionBase *IntuitionBase);
struct Gadget *buildeasyreq_makegadgets(struct reqdims *dims,
					STRPTR *gadgetlabels,
					struct Screen *scr,
					struct IntuitionBase *IntuitionBase);
void buildeasyreq_draw(struct reqdims *dims, STRPTR text,
		       struct Window *win, struct Screen *scr,
		       struct Gadget *gadgets,
		       struct IntuitionBase *IntuitionBase);

int charsinstring(STRPTR string, char c);

/*****************************************************************************

    NAME */
#include <proto/intuition.h>
#include <exec/types.h>
#include <intuition/intuition.h>

	AROS_LH4(struct Window *, BuildEasyRequestArgs,

/*  SYNOPSIS */
	AROS_LHA(struct Window     *, RefWindow, A0),
	AROS_LHA(struct EasyStruct *, easyStruct, A1),
	AROS_LHA(ULONG              , IDCMP, D0),
	AROS_LHA(APTR               , Args, A3),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 99, Intuition)

/*  FUNCTION
	Opens a requester, which provides one or more choices. The control is
	returned to the application after the requester was opened. It is
	handled by subsequent calls to SysReqHandler() and closed by calling
	FreeSysRequest().

    INPUTS
	RefWindow - A reference window. If NULL, the requester opens on
		    the default public screen.
	easyStruct - The EasyStruct structure (<intuition/intuition.h>),
		     which describes the requester.
	IDCMP - IDCMP flags, which should satisfy the requester, too. This is
		useful for requesters, which want to listen to disk changes,
		etc. Note that this is not a pointer to the flags as in
		EasyRequestArgs().
	Args - The arguments for easyStruct->es_TextFormat.

    RESULT
	Returns a pointer to the requester. Use this pointer only for calls
	to SysReqHandler() and FreeSysRequest().

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	EasyRequestArgs(), SysReqHandler(), FreeSysRequest()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct Screen *scr = NULL, *lockedscr = NULL;
    struct Window *req;
    struct Gadget *gadgets;
    STRPTR reqtitle;
    STRPTR formattedtext;
    STRPTR *gadgetlabels;
    struct reqdims dims;
    struct EasyRequestUserData *requserdata;

    if (!easyStruct)
	return FALSE;

    /* get requester title */
    reqtitle = easyStruct->es_Title;
    if ((!reqtitle) && (RefWindow))
	reqtitle = RefWindow->Title;

    /* get screen and screendrawinfo */
    if (RefWindow)
	scr = RefWindow->WScreen;
    if (!scr)
    {
	scr = LockPubScreen(NULL);
	if (!scr)
	    return FALSE;
	lockedscr = scr;
    }

    /* create everything */
    gadgetlabels = buildeasyreq_makelabels(&dims,
					   easyStruct->es_GadgetFormat);
    if (gadgetlabels)
    {
	formattedtext = buildeasyreq_formattext(easyStruct->es_TextFormat,
						Args);
	if (formattedtext)
	{
	    if (buildeasyreq_calculatedims(&dims, scr,
					   formattedtext, gadgetlabels,
					   IntuitionBase))
	    {
		gadgets = buildeasyreq_makegadgets(&dims, gadgetlabels, scr,
						    IntuitionBase);
		if (gadgets)
		{
		    requserdata = AllocVec(sizeof(struct EasyRequestUserData),
					   MEMF_ANY);
		    if (requserdata)
		    {
			req = OpenWindowTags(NULL,
					     WA_Width, dims.width,
					     WA_Height, dims.height,
					     WA_IDCMP, IDCMP_GADGETUP | IDCMP,
					     WA_Gadgets, (IPTR)gadgets,
					     WA_Title, (IPTR)reqtitle,
					     WA_CustomScreen, (IPTR)scr,
					     WA_Flags, WFLG_DRAGBAR |
						       WFLG_DEPTHGADGET |
						       WFLG_ACTIVATE |
						       WFLG_RMBTRAP |
						       WFLG_SIMPLE_REFRESH,
					     TAG_DONE);
			if (req)
			{
			    req->UserData = (BYTE *)requserdata;
			    requserdata->IDCMP = IDCMP;
			    requserdata->GadgetLabels = gadgetlabels;
			    requserdata->Gadgets = gadgets;
			    requserdata->NumGadgets = dims.gadgets;
			    buildeasyreq_draw(&dims, formattedtext,
					      req, scr, gadgets,
					      IntuitionBase);
			    FreeVec(formattedtext);
			    return req;
			}

			/* opening requester failed -> free everything */
			FreeVec(requserdata);
		    }
		    easyrequest_freegadgets(gadgets,IntuitionBase);
		}
	    }
	    FreeVec(formattedtext);
	}
	easyrequest_freelabels(gadgetlabels,IntuitionBase);
    }
    UnlockPubScreen(NULL, lockedscr);

    return NULL;
    AROS_LIBFUNC_EXIT
} /* BuildEasyRequestArgs */

#undef SysBase
#define SysBase	(*(APTR*)4L)

UWORD BgPattern[2]  = { 0xAAAA, 0x5555 };

/* draw the contents of the requester */
void buildeasyreq_draw(struct reqdims *dims, STRPTR text,
		       struct Window *req, struct Screen *scr,
		       struct Gadget *gadgets,
		       struct IntuitionBase *IntuitionBase)
{
    struct DrawInfo *dri;
    struct Image *frame;
    int currentline;

    dri = GetScreenDrawInfo(scr);
    if (!dri)
	return;

    /* draw background pattern */
    SetABPenDrMd(req->RPort,
		 dri->dri_Pens[SHINEPEN], dri->dri_Pens[BACKGROUNDPEN],
		 JAM1);
    SetAfPt(req->RPort, BgPattern, 1);
    RectFill(req->RPort, scr->WBorLeft,
			 scr->WBorTop + dims->fontheight - 1,
			 req->Width - scr->WBorRight - 1,
			 req->Height - scr->WBorBottom - 1);
    SetAfPt(req->RPort, NULL, 0);

    /* draw textframe */
    frame = (struct Image *)NewObject(NULL, FRAMEICLASS,
	IA_Left, scr->WBorLeft + HSPACE,
	IA_Top, scr->WBorTop + dims->fontheight - 1 + VSPACE,
	IA_Width, req->Width - scr->WBorLeft - scr->WBorRight - HSPACE * 2,
	IA_Height, req->Height - scr->WBorTop - scr->WBorBottom -
		   dims->fontheight * 2 - VSPACE * 3 - VBORDER * 2 + 1,
	IA_Recessed, TRUE,
	IA_EdgesOnly, FALSE,
	TAG_DONE);
    if (frame)
    {
	DrawImageState(req->RPort, frame, 0, 0, IDS_NORMAL, dri);
	DisposeObject((Object *)frame);
    }

    /* draw text */
    SetABPenDrMd(req->RPort,
		 dri->dri_Pens[TEXTPEN], dri->dri_Pens[BACKGROUNDPEN], JAM1);
    for (currentline = 1; text[0] != '\0'; currentline++)
    {
	STRPTR strend;
	int length;

	strend = strchr(text, '\n');
	if (strend)
	    length = strend - text;
	else
	    length = strlen(text);
	Move(req->RPort,
	     scr->WBorLeft + HSPACE + HBORDER,
	     scr->WBorTop + dims->fontheight * currentline - 1 +
	       VSPACE + VBORDER + req->RPort->Font->tf_Baseline);
	Text(req->RPort, text, length);
	text += length;
	if (text[0] == '\n')
	    text++;
    }

    /* draw gadgets */
    RefreshGList(gadgets, req, NULL, -1L);

    FreeScreenDrawInfo(scr, dri);
}



/* create an array of gadgetlabels */
STRPTR *buildeasyreq_makelabels(struct reqdims *dims, STRPTR labeltext)
{
    STRPTR *gadgetlabels;
    STRPTR label;
    int currentgadget;
    int len;

    /* make room for pointer-array */
    dims->gadgets = charsinstring(labeltext, '|') + 1;
    gadgetlabels = AllocVec((dims->gadgets + 1) * sizeof(STRPTR), MEMF_ANY);
    if (!gadgetlabels)
	return NULL;
    gadgetlabels[dims->gadgets] = NULL;

    /* copy label-string */
    len = strlen(labeltext) + 1;
    label = AllocVec(len, MEMF_ANY);
    if (!label)
    {
	FreeVec(gadgetlabels);
	return NULL;
    }
    CopyMem(labeltext, label, len);

    /* set up the pointers and insert null-bytes */
    for (currentgadget = 0; currentgadget < dims->gadgets; currentgadget++)
    {
	gadgetlabels[currentgadget] = label;
	if (currentgadget != (dims->gadgets - 1))
	{
	    while (label[0] != '|')
		label++;
	    label[0] = '\0';
	    label++;
	}
    }

    return gadgetlabels;
}



/* format the supplied text string by using the supplied args */
STRPTR buildeasyreq_formattext(STRPTR textformat, APTR args)
{
    int len;
    STRPTR buffer;

    len = strlen(textformat) + 256;
    for (;;)
    {
	buffer = AllocVec(len, MEMF_ANY);
	if (!buffer)
	    return NULL;
	if (vsnprintf(buffer, len, textformat, args) < len)
	    return buffer;
	FreeVec(buffer);
	len += 256;
    }
}



/* calculate dimensions of the requester */
BOOL buildeasyreq_calculatedims(struct reqdims *dims,
				struct Screen *scr,
				STRPTR formattedtext,
				STRPTR *gadgetlabels,
				struct IntuitionBase *IntuitionBase)
{
    STRPTR textline;
    int textlines, line; /* number of lines in es_TextFormat */
    int currentgadget = 0;
    UWORD textboxwidth = 0, gadgetswidth; /* width of upper/lower part */

    /* calculate height of requester */
    dims->fontheight = scr->RastPort.Font->tf_YSize;
    textlines = charsinstring(formattedtext, '\n') + 1;
    dims->height = (textlines + 2) * dims->fontheight +
		   scr->WBorTop + scr->WBorBottom - 1 +
		   VSPACE * 3 + VBORDER * 4;
    if (dims->height > scr->Height)
	return FALSE;

    /* calculate width of text-box */
    textline = formattedtext;
    for (line = 0; line<textlines; line++)
    {
	int linelen; /* length of current text line */
	UWORD linewidth; /* width (pixel) of current text line */

	if (line == (textlines - 1))
	    linelen = strlen(textline);
	else
	{
	    linelen = 0;
	    while (textline[linelen] != '\n')
		linelen++;
	}
	linewidth = TextLength(&scr->RastPort, textline, linelen);
	if (linewidth > textboxwidth)
	    textboxwidth = linewidth;
	textline = textline + linelen + 1;
    }
    textboxwidth += HBORDER * 2;

    /* calculate width of gadgets */
    dims->gadgetwidth = 0;
    while (gadgetlabels[currentgadget])
    {
	UWORD gadgetwidth; /* width of current gadget */

	gadgetwidth = TextLength(&scr->RastPort, gadgetlabels[currentgadget],
				 strlen(gadgetlabels[currentgadget]));
	if (gadgetwidth > dims->gadgetwidth)
	    dims->gadgetwidth = gadgetwidth;
	currentgadget++;
    }
    dims->gadgetwidth += HBORDER * 2 + HSPACE;
    gadgetswidth = (dims->gadgetwidth + HSPACE) * dims->gadgets - HSPACE;

    /* calculate width of requester */
    dims->width = MAX(textboxwidth, gadgetswidth) + HSPACE * 2;
    if (dims->width > scr->Width)
	return FALSE;

    return TRUE;
}



/* make all the gadgets */
struct Gadget *buildeasyreq_makegadgets(struct reqdims *dims,
					STRPTR *gadgetlabels,
					struct Screen *scr,
					struct IntuitionBase *IntuitionBase)
{
    struct Gadget *gadgetlist, *thisgadget = NULL;
    struct Image *gadgetframe;
    int currentgadget;
    UWORD xoffset, restwidth;

    if (gadgetlabels[0] == NULL)
	return NULL;

    gadgetframe = (struct Image *)NewObject(NULL, FRAMEICLASS,
					    IA_FrameType, FRAME_BUTTON,
					    IA_EdgesOnly, TRUE,
					    TAG_DONE);
    if (!gadgetframe)
	return NULL;

    restwidth = dims->width - scr->WBorLeft - scr->WBorRight - HSPACE * 2;
    if (dims->gadgets == 1)
	xoffset = scr->WBorLeft + HSPACE + (restwidth - dims->gadgetwidth) / 2;
    else
    {
	xoffset = scr->WBorLeft + HSPACE;
	restwidth -= dims->gadgets * dims->gadgetwidth;
    }

    gadgetlist = NULL;

    for (currentgadget = 0; gadgetlabels[currentgadget]; currentgadget++)
    {
	IPTR gadgetid;

	if (currentgadget == (dims->gadgets - 1))
	    gadgetid = 0;
	else
	    gadgetid = currentgadget + 1;

	thisgadget = NewObject(NULL, FRBUTTONCLASS,
		GA_ID,		gadgetid,
		GA_Previous,	thisgadget,
		GA_Left,	xoffset,
		GA_Width,	dims->gadgetwidth,
		GA_Top, 	dims->height -
				scr->WBorBottom - dims->fontheight
				- VSPACE - VBORDER * 2,
		GA_Height,	dims->fontheight + VBORDER * 2,
		GA_Text,	(IPTR)gadgetlabels[currentgadget],
		GA_Image,	(IPTR)gadgetframe,
		GA_RelVerify,	TRUE,
		TAG_DONE
	);

	if (currentgadget == 0)
	    gadgetlist = thisgadget;

	if (!thisgadget)
	{
	    easyrequest_freegadgets(gadgetlist,IntuitionBase);
	    return NULL;
	}
	
	if ((currentgadget + 1) != dims->gadgets)
	{
	    xoffset += dims->gadgetwidth +
		       restwidth / (dims->gadgets - currentgadget - 1);
	    restwidth -= restwidth / (dims->gadgets - currentgadget - 1);
	}
    }

    return gadgetlist;
}

/***** Support Functions *****/

/* count the occurences of a specified character in a string */
int charsinstring(STRPTR string, char c)
{
    int count = 0;

    while (string[0])
    {
	if (string[0] == c)
	    count++;
	string++;
    }
    return count;
}
