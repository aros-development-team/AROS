/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/memory.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <intuition/imageclass.h>
#include <intuition/gadgetclass.h>

#include <graphics/gfx.h>
#include <utility/hooks.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/alib.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "misc.h"

/*******************************************************************************/

#define USE_SIMPLEREFRESH 0

#define DEFAULT_TABSIZE 8

#define INNER_SPACING_X 2
#define INNER_SPACING_Y 2

#define WINDOWWIDTH	390
#define WINDOWHEIGHT	228

#define MAX_TEXTLINELEN 4096


enum
{
    GAD_UPARROW,
    GAD_DOWNARROW,
    GAD_LEFTARROW,
    GAD_RIGHTARROW,
    GAD_VERTSCROLL,
    GAD_HORIZSCROLL,
    NUM_GADGETS
};

enum
{
    IMG_UPARROW,
    IMG_DOWNARROW,
    IMG_LEFTARROW,
    IMG_RIGHTARROW,
    IMG_SIZE,
    NUM_IMAGES
};

/*******************************************************************************/

struct LineNode
{
    char *text;
    UWORD stringlen;
    UWORD textlen;
    BOOL invert;
};

/*******************************************************************************/

extern struct IntuitionBase *IntuitionBase;
extern struct GfxBase *GfxBase;

extern struct Screen *scr;
struct DrawInfo *dri;

char *wintitle;

struct Window *morewin;
struct RastPort *morerp;

struct Gadget *scrollergad[NUM_GADGETS], *firstgadget;
struct Image *img[NUM_GADGETS];

struct LineNode *linearray;

UBYTE *filebuffer;

static UWORD tabsize = DEFAULT_TABSIZE;

WORD fontwidth, fontheight, borderleft, bordertop;
WORD shinepen, shadowpen, bgpen, textpen;
WORD borderright, borderbottom, visiblex, visibley;
WORD fontbaseline, textstartx, textstarty, textendx, textendy;
WORD textwidth, textheight, viewstartx, viewstarty;
WORD winwidth, winheight;

LONG filelen, num_lines, max_textlen;

/*******************************************************************************/

void Cleanup()
{
    WORD i;

    if (morewin)
    {
	for(i = 0; i < NUM_GADGETS;i++)
	{
	    if (scrollergad[i]) RemoveGadget(morewin,scrollergad[i]);
	}

	CloseWindow(morewin);
    }

    for(i = 0; i < NUM_GADGETS;i++)
    {
    	if (scrollergad[i]) DisposeObject(scrollergad[i]);
    }
    for(i = 0; i < NUM_IMAGES;i++)
    {
	if (img[i]) DisposeObject(img[i]);
    }

    if (dri) FreeScreenDrawInfo(scr,dri);
    if (scr) UnlockPubScreen(0,scr);

    if (linearray) FreeVec(linearray);
    if (filebuffer) FreeVec(filebuffer);

}

UWORD CalcTextLen(char *text)
{
    char c;
    UWORD rc = 0;

    while((c = *text++))
    {
	if (c == '\t')
	{
	    rc += tabsize;
	} else {
	    rc++;
	}
}

    if (rc > MAX_TEXTLINELEN) rc = MAX_TEXTLINELEN;

    return rc;
}

void MakeLineArray(void)
{
    struct LineNode *linepos;
    UBYTE *filepos = filebuffer;
    LONG flen = filelen, act_line;

    num_lines = 1;

    while(flen--)
    {
	if (*filepos++ == '\n') num_lines++;
    }

    linearray = AllocVec(num_lines * sizeof(struct LineNode),MEMF_PUBLIC | MEMF_CLEAR);
    if (!linearray) Cleanup();

    linepos = linearray;
    filepos = filebuffer;

    act_line = 1;
    while(act_line <= num_lines)
    {
	linepos->text = (char *)filepos;

	while ((*filepos != '\n') && (*filepos != '\0'))
	{
	    linepos->stringlen++;filepos++;
	}

	*filepos++ = '\0';

	linepos->textlen = CalcTextLen(linepos->text);
	if (linepos->textlen > max_textlen) max_textlen = linepos->textlen;

	linepos++;act_line++;
    }
}

void GetVisual(void)
{
    if (!(scr = LockPubScreen(0)))
    {
	Cleanup();
    }

    if (!(dri = GetScreenDrawInfo(scr)))
    {
	Cleanup();
    }

    shinepen = dri->dri_Pens[SHINEPEN];
    shadowpen = dri->dri_Pens[SHADOWPEN];
    textpen = dri->dri_Pens[TEXTPEN];
    bgpen = dri->dri_Pens[BACKGROUNDPEN];
}

void MakeGadgets(void)
{
    static WORD img2which[] =
    {
   	UPIMAGE,
   	DOWNIMAGE,
   	LEFTIMAGE,
   	RIGHTIMAGE,
   	SIZEIMAGE
    };
   
    IPTR imagew[NUM_IMAGES],imageh[NUM_IMAGES];
    WORD v_offset,h_offset, btop, i;

    for(i = 0;i < NUM_IMAGES;i++)
    {
	img[i] = NewObject(0,SYSICLASS,SYSIA_DrawInfo,(IPTR)dri,
				       SYSIA_Which,img2which[i],
				       TAG_DONE);

	if (!img[i]) Cleanup();

	GetAttr(IA_Width,(Object *)img[i],&imagew[i]);
	GetAttr(IA_Height,(Object *)img[i],&imageh[i]);
    }

    btop = scr->WBorTop + dri->dri_Font->tf_YSize + 1;

    v_offset = imagew[IMG_DOWNARROW] / 4;
    h_offset = imageh[IMG_LEFTARROW] / 4;

    firstgadget = scrollergad[GAD_UPARROW] = NewObject(0,BUTTONGCLASS,
	    GA_Image,(IPTR)img[IMG_UPARROW],
	    GA_RelRight,-imagew[IMG_UPARROW] + 1,
	    GA_RelBottom,-imageh[IMG_DOWNARROW] - imageh[IMG_UPARROW] - imageh[IMG_SIZE] + 1,
	    GA_ID,GAD_UPARROW,
	    GA_RightBorder,TRUE,
	    GA_Immediate,TRUE,
	    TAG_DONE);

    scrollergad[GAD_DOWNARROW] = NewObject(0,BUTTONGCLASS,
	    GA_Image,(IPTR)img[IMG_DOWNARROW],
	    GA_RelRight,-imagew[IMG_UPARROW] + 1,
	    GA_RelBottom,-imageh[IMG_UPARROW] - imageh[IMG_SIZE] + 1,
	    GA_ID,GAD_DOWNARROW,
	    GA_RightBorder,TRUE,
	    GA_Previous,(IPTR)scrollergad[GAD_UPARROW],
	    GA_Immediate,TRUE,
	    TAG_DONE);

    scrollergad[GAD_VERTSCROLL] = NewObject(0,PROPGCLASS,
	    GA_Top,btop + 1,
	    GA_RelRight,-imagew[IMG_DOWNARROW] + v_offset + 1,
	    GA_Width,imagew[IMG_DOWNARROW] - v_offset * 2,
	    GA_RelHeight,-imageh[IMG_DOWNARROW] - imageh[IMG_UPARROW] - imageh[IMG_SIZE] - btop -2,
	    GA_ID,GAD_VERTSCROLL,
	    GA_Previous,(IPTR)scrollergad[GAD_DOWNARROW],
	    GA_RightBorder,TRUE,
	    GA_RelVerify,TRUE,
	    GA_Immediate,TRUE,
	    PGA_NewLook,TRUE,
	    PGA_Borderless,TRUE,
	    PGA_Total,100,
	    PGA_Visible,100,
	    PGA_Freedom,FREEVERT,
	    TAG_DONE);

    scrollergad[GAD_RIGHTARROW] = NewObject(0,BUTTONGCLASS,
	    GA_Image,(IPTR)img[IMG_RIGHTARROW],
	    GA_RelRight,-imagew[IMG_SIZE] - imagew[IMG_RIGHTARROW] + 1,
	    GA_RelBottom,-imageh[IMG_RIGHTARROW] + 1,
	    GA_ID,GAD_RIGHTARROW,
	    GA_BottomBorder,TRUE,
	    GA_Previous,(IPTR)scrollergad[GAD_VERTSCROLL],
	    GA_Immediate,TRUE,
	    TAG_DONE);

    scrollergad[GAD_LEFTARROW] = NewObject(0,BUTTONGCLASS,
	    GA_Image,(IPTR)img[IMG_LEFTARROW],
	    GA_RelRight,-imagew[IMG_SIZE] - imagew[IMG_RIGHTARROW] - imagew[IMG_LEFTARROW] + 1,
	    GA_RelBottom,-imageh[IMG_RIGHTARROW] + 1,
	    GA_ID,GAD_LEFTARROW,
	    GA_BottomBorder,TRUE,
	    GA_Previous,(IPTR)scrollergad[GAD_RIGHTARROW],
	    GA_Immediate,TRUE,
	    TAG_DONE);

    scrollergad[GAD_HORIZSCROLL] = NewObject(0,PROPGCLASS,
	    GA_Left,scr->WBorLeft,
	    GA_RelBottom,-imageh[IMG_LEFTARROW] + h_offset + 1,
	    GA_RelWidth,-imagew[IMG_LEFTARROW] - imagew[IMG_RIGHTARROW] - imagew[IMG_SIZE] - scr->WBorRight - 2,
	    GA_Height,imageh[IMG_LEFTARROW] - (h_offset * 2),
	    GA_ID,GAD_HORIZSCROLL,
	    GA_Previous,(IPTR)scrollergad[GAD_LEFTARROW],
	    GA_BottomBorder,TRUE,
	    GA_RelVerify,TRUE,
	    GA_Immediate,TRUE,
	    PGA_NewLook,TRUE,
	    PGA_Borderless,TRUE,
	    PGA_Total,100,
	    PGA_Visible,100,
	    PGA_Freedom,FREEHORIZ,
	    TAG_DONE);

    for(i = 0;i < NUM_GADGETS;i++)
    {
	if (!scrollergad[i]) Cleanup();
    }
}

void CalcVisible(void)
{
    visiblex = (morewin->Width - borderleft - borderright -
	       INNER_SPACING_X * 2) / fontwidth;

    visibley = (morewin->Height - bordertop - borderbottom -
	       INNER_SPACING_Y * 2) / fontheight;

    if (visiblex < 1) visiblex = 1;
    if (visibley < 1) visibley = 1;

    textendx = textstartx + visiblex * fontwidth - 1;
    textendy = textstarty + visibley * fontheight - 1;

    textwidth = textendx - textstartx + 1;
    textheight = textendy - textstarty + 1;	
}

void DrawTextLine(WORD viewline, WORD columns, BOOL clearright)
{
    static char tempstring[MAX_TEXTLINELEN + 1];
    static char c, *stringpos, *text;
    LONG realline = viewline + viewstarty;
    WORD textlen, i = 0, t, x;
    BOOL inverted;

    /* column < 0 means draw only first -column chars
    **
    ** column > 0 means draw only last column chars
    **
    ** column = 0 means draw whole line
    */

    if (columns != 0)
    {
	clearright = FALSE; /* because already cleared by ScrollRaster */
    }

    if ((viewline >= 0) && (viewline < visibley) && 
	(realline >= 0) && (realline < num_lines))
    {
	inverted = linearray[realline].invert;

	text = linearray[realline].text;
	textlen = linearray[realline].textlen;

	stringpos = tempstring;

	while((c = *text++) && (i < textlen))
	{
	    if (c == '\t')
	    {
		for(t = 0; (t < tabsize) && (i < textlen);t++)
		{
		    *stringpos++ = ' ';
		    i++;
		}
	    } else {
		*stringpos++ = c;
		i++;
	    }
	} /* while((c = *text++) && (i < textlen)) */

	stringpos = tempstring + viewstartx;
	i -= viewstartx;

	if (i < 0)
	{
	    i = 0;
	} else {
	    if (i > visiblex) i = visiblex;

	    x = textstartx;
	    if (columns < 0)
	    {
		if (i > (-columns)) i = (-columns);
	    } else if (columns > 0) {
		x = textstartx + (visiblex - columns) * fontwidth;
		stringpos += (visiblex - columns);
		i -= (visiblex - columns);
	    }

	    if (i > 0)
	    {
		SetAPen(morerp, textpen);
		SetBPen(morerp, inverted ? shinepen : bgpen);

		Move(morerp,x,
		        textstarty + (viewline * fontheight) + fontbaseline);

		Text(morerp,stringpos,i); 
	    }
	}

    } /* if ((realline >= 0) && (realline < num_lines)) */

    if ((i < visiblex) && clearright)
    {
	SetAPen(morerp, bgpen);
	RectFill(morerp, textstartx + (i * fontwidth),
		     textstarty + (viewline * fontheight),
		     textendx,
		     textstarty + (viewline * fontheight) + fontheight - 1);
    }
}

void DrawAllText(void)
{
    WORD y;

    for(y = 0;y < visibley;y++)
    {
	DrawTextLine(y,0,TRUE);
    }
}

static void MakeWin(char *title)
{
    wintitle = StrDup( title );

    if (!(morewin = OpenWindowTags(0,WA_PubScreen,(IPTR)scr,
				 WA_Left,(scr->Width - WINDOWWIDTH)/2,
				 WA_Top,(scr->Height - (WINDOWHEIGHT-10))/2+1,
				 WA_Width,WINDOWWIDTH,
				 WA_Height,WINDOWHEIGHT,
				 WA_Title,(IPTR)wintitle,
			         (USE_SIMPLEREFRESH != 0) ?
				 WA_SimpleRefresh         :
                                 TAG_IGNORE,TRUE,
				 WA_CloseGadget,TRUE,
				 WA_DepthGadget,TRUE,
				 WA_DragBar,TRUE,
				 WA_SizeGadget,TRUE,
				 WA_SizeBBottom,TRUE,
				 WA_SizeBRight,TRUE,
				 WA_Activate,TRUE,
				 WA_Gadgets,(IPTR)firstgadget,
				 WA_MinWidth,100,
				 WA_MinHeight,100,
				 WA_MaxWidth,scr->Width,
				 WA_MaxHeight,scr->Height,
				 WA_ReportMouse,TRUE,
				 WA_IDCMP,IDCMP_CLOSEWINDOW   | IDCMP_NEWSIZE   |
					  IDCMP_GADGETDOWN    | IDCMP_GADGETUP  |
					  IDCMP_MOUSEMOVE     | IDCMP_VANILLAKEY| 
               ((USE_SIMPLEREFRESH != 0) * IDCMP_REFRESHWINDOW) | IDCMP_RAWKEY,
				 TAG_DONE)))
    {
	Cleanup();
    }


    winwidth = morewin->Width;
    winheight = morewin->Height;

    morerp = morewin->RPort;

    SetDrMd(morerp,JAM2);

    fontwidth = morerp->TxWidth;
    fontheight = morerp->TxHeight;
    fontbaseline = morerp->TxBaseline;

    borderleft = morewin->BorderLeft;
    bordertop = morewin->BorderTop;
    borderright = morewin->BorderRight;
    borderbottom = morewin->BorderBottom;

    textstartx = borderleft + INNER_SPACING_X;
    textstarty = bordertop + INNER_SPACING_Y;

    CalcVisible();

    SetGadgetAttrs(scrollergad[GAD_HORIZSCROLL],
		   morewin,
		   0,
		   PGA_Top,0,
		   PGA_Total,max_textlen,
		   PGA_Visible,visiblex,
		   TAG_DONE);

    SetGadgetAttrs(scrollergad[GAD_VERTSCROLL],
		   morewin,
		   0,
		   PGA_Top,0,
		   PGA_Total,num_lines,
		   PGA_Visible,visibley,
		   TAG_DONE);

    DrawAllText();
}

void NewWinSize(void)
{
    WORD new_winwidth, new_winheight;

    new_winwidth = morewin->Width;
    new_winheight = morewin->Height;

    CalcVisible();

    if ((viewstartx + visiblex) > max_textlen)
    {
	viewstartx = max_textlen - visiblex;
	if (viewstartx < 0) viewstartx = 0;
    }

    if ((viewstarty + visibley) > num_lines)
    {
	viewstarty = num_lines - visibley;
	if (viewstarty < 0) viewstarty = 0;
    }

    SetGadgetAttrs(scrollergad[GAD_HORIZSCROLL],
		   morewin,
		   0,
		   PGA_Top,viewstartx,
		   PGA_Visible,visiblex,
		   TAG_DONE);

    SetGadgetAttrs(scrollergad[GAD_VERTSCROLL],
		   morewin,
		   0,
		   PGA_Top,viewstarty,
		   PGA_Visible,visibley,
		   TAG_DONE);

    if (new_winwidth < winwidth)
    {
	SetAPen(morerp, bgpen);
	RectFill(morerp,textendx + 1,
		    bordertop + INNER_SPACING_Y,
		    new_winwidth - borderright - 1,
		    new_winheight - borderbottom - 1);
    }

    if (new_winheight < winheight)
    {
	SetAPen(morerp, bgpen);
	RectFill(morerp,borderleft + INNER_SPACING_X,
		    textendy + 1,
		    new_winwidth - borderright - 1,
		    new_winheight - borderbottom - 1);
    }

    if ((new_winwidth > winwidth) ||
	(new_winheight > winheight))
    {
	DrawAllText();
    }

    winwidth  = new_winwidth;
    winheight = new_winheight;
}

#ifdef USE_SIMPLEREFRESH

void RefreshAll(void)
{
    DrawAllText();
}

void HandleRefresh(void)
{
    BeginRefresh(morewin);
    RefreshAll();
    EndRefresh(morewin,TRUE);
}

#endif

void ScrollTo(WORD gadid, LONG top, BOOL refreshprop)
{
    LONG y, dx, dy;

    SetBPen(morerp,bgpen);

    switch(gadid)
    {
	case GAD_VERTSCROLL:
	    if (top + visibley > num_lines)
	    {
		top = num_lines - visibley;
	    }
	    if (top < 0) top = 0;

	    if (top != viewstarty )
	    {
		dy = top - viewstarty;
		viewstarty = top;

		if (refreshprop)
		{
		    SetGadgetAttrs(scrollergad[gadid],
				   morewin,
				   0,
				   PGA_Top,viewstarty,
				   TAG_DONE);
		}

		if (abs(dy) >= visibley)
		{
		    DrawAllText();
		} else {
		    if (dy > 0)
		    {

		    #ifdef USE_SIMPLEREFRESH
			ScrollRaster(morerp,
				     0,
				     fontheight * dy,
				     textstartx,
				     textstarty,
				     textendx,
				     textendy);

			if (morerp->Layer->Flags & LAYERREFRESH)
			{
			    HandleRefresh();
			}
		    #else
			ClipBlit(morerp,textstartx,
				    textstarty + dy * fontheight,
				 morerp,textstartx,
				    textstarty,
				    textwidth,
				    textheight - dy * fontheight,
				 192);
		    #endif

			for (y = visibley - dy;y < visibley;y++)
			{
			    DrawTextLine(y,0,TRUE);
			}
		    } else {
			dy = -dy;

		    #ifdef USE_SIMPLEREFRESH
			ScrollRaster(morerp,
				     0,
				     -fontheight * dy,
				     textstartx,
				     textstarty,
				     textendx,
				     textendy);

			if (morerp->Layer->Flags & LAYERREFRESH)
			{
			    HandleRefresh();
			}

		    #else
			ClipBlit(morerp,textstartx,
				    textstarty,
				 morerp,textstartx,
				    textstarty + dy * fontheight,
				    textwidth,
				    textheight - dy * fontheight,
				 192);
		    #endif
			for (y = 0;y < dy;y++)
			{
			    DrawTextLine(y,0,TRUE);
			}
		    }
		}

	    } /* if (top != viewstarty ) */
	    break;

	case GAD_HORIZSCROLL:
	    if (top + visiblex > max_textlen)
	    {
		top = max_textlen - visiblex;
	    }
	    if (top < 0) top = 0;

	    if (top != viewstartx )
	    {
		dx = top - viewstartx;
		viewstartx = top;

		if (refreshprop)
		{
		    SetGadgetAttrs(scrollergad[gadid],
				   morewin,
				   0,
				   PGA_Top,viewstartx,
				   TAG_DONE);
		}

		if (abs(dx) >= visiblex)
		{
		    DrawAllText();
		} else {
		    if (dx > 0)
		    {

		    #ifdef USE_SIMPLEREFRESH
			ScrollRaster(morerp,
				     fontwidth * dx,
				     0,
				     textstartx,
				     textstarty,
				     textendx,
				     textendy);

			if (morerp->Layer->Flags & LAYERREFRESH)
			{
			    HandleRefresh();
			}
		    #else
			ClipBlit(morerp,textstartx + dx * fontwidth,
				    textstarty,
			         morerp,textstartx,
				    textstarty,
				    textwidth - dx * fontwidth,
				    textheight,
				 192);
		    #endif
			for (y = 0;y < visibley;y++)
			{
			    DrawTextLine(y,dx,TRUE);
			}

		    } else {
			dx = -dx;

		    #ifdef USE_SIMPLEREFRESH
			ScrollRaster(morerp,
				     -fontwidth * dx,
				     0,
				     textstartx,
				     textstarty,
				     textendx,
				     textendy);

			if (morerp->Layer->Flags & LAYERREFRESH)
			{
			    HandleRefresh();
			}

		    #else
			ClipBlit(morerp,textstartx,
				    textstarty,
			         morerp,textstartx + dx * fontwidth,
				    textstarty,
				    textwidth - dx * fontwidth,
				    textheight,
			         192);
		    #endif
			for (y = 0;y < visibley;y++)
			{
				DrawTextLine(y,-dx,TRUE);
			}
		    }
		}

	    } /* if (top != viewstartx ) */
	    break;

    } /* switch(gadid) */
	
}

void HandleScrollGadget(WORD gadid)
{
    struct IntuiMessage *msg;
    IPTR top;
    BOOL ok=FALSE;

    while (!ok)
    {
	    WaitPort(morewin->UserPort);
	    while ((msg = (struct IntuiMessage *)GetMsg(morewin->UserPort)))
	    {
		    switch (msg->Class)
		    {
			    case IDCMP_GADGETUP:
				    ok=TRUE;
				    /* fall through */

			    case IDCMP_MOUSEMOVE:
				    GetAttr(PGA_Top,(Object *)scrollergad[gadid],&top);
				    ScrollTo(gadid,top,FALSE);
				    break;

#ifdef USE_SIMPLEREFRESH
			    case IDCMP_REFRESHWINDOW:
				    HandleRefresh();
				    break;
#endif

		    } /* switch (msg->Class) */
		    ReplyMsg((struct Message *)msg);

	    } /* while ((msg = (struct IntuiMessage *)GetMsg(morewin->UserPort))) */

    } /* while (!ok) */
}

void HandleArrowGadget(WORD gadid)
{
    struct IntuiMessage *msg;

    BOOL ok = FALSE;

    while (!ok)
    {
	while ((msg = (struct IntuiMessage *)GetMsg(morewin->UserPort)))
	{
#ifdef USE_SIMPLEREFRESH
	    if (msg->Class == IDCMP_REFRESHWINDOW)
	    {
		HandleRefresh();
	    }
#endif
	    ReplyMsg((struct Message *)msg);
	}

	if (scrollergad[gadid]->Flags & GFLG_SELECTED)
	{
	    switch (gadid)
	    {
		case GAD_UPARROW:
		    ScrollTo(GAD_VERTSCROLL,viewstarty - 1,TRUE);
		    break;

		case GAD_DOWNARROW:
		    ScrollTo(GAD_VERTSCROLL,viewstarty + 1,TRUE);
		    break;

		case GAD_LEFTARROW:
		    ScrollTo(GAD_HORIZSCROLL,viewstartx - 1,TRUE);
		    break;

		case GAD_RIGHTARROW:
		    ScrollTo(GAD_HORIZSCROLL,viewstartx + 1,TRUE);
		    break;

	    } /* switch (gid) */

	} /* if (scrollergad[gadid]->Flags & GFLG_SELECTED) */

	if (!(scrollergad[gadid]->Activation & GACT_ACTIVEGADGET)) ok=TRUE;

    } /* while (!ok) */	
}



BOOL HandleWin(void)
{
    struct IntuiMessage *msg;
    WORD gadid;
    BOOL pagescroll,maxscroll,quitme = FALSE;

    while ((msg = (struct IntuiMessage *)GetMsg(morewin->UserPort)))
    {
	switch(msg->Class)
	{
	    case IDCMP_CLOSEWINDOW:
		quitme = TRUE;
		break;

	    case IDCMP_NEWSIZE:
		NewWinSize();
		break;

#ifdef USE_SIMPLEREFRESH
	    case IDCMP_REFRESHWINDOW:
		HandleRefresh();
		break;
#endif
	    case IDCMP_GADGETDOWN:
		gadid = ((struct Gadget *)msg->IAddress)->GadgetID;

		switch(gadid)
		{
		    case GAD_HORIZSCROLL:
		    case GAD_VERTSCROLL:
			HandleScrollGadget(gadid);
			break;

		    case GAD_UPARROW:
		    case GAD_DOWNARROW:
		    case GAD_LEFTARROW:
		    case GAD_RIGHTARROW:
			HandleArrowGadget(gadid);
			break;
		};
		break;

	    case IDCMP_VANILLAKEY:
		switch(toupper(msg->Code))
		{
		    case 27:
			quitme = TRUE;
			break;

		    case ' ':
			ScrollTo(GAD_VERTSCROLL,viewstarty + visibley - 1,TRUE);
			break;

		    case 8:
			ScrollTo(GAD_VERTSCROLL,viewstarty - (visibley - 1),TRUE);
			break;

		    case 13:
			ScrollTo(GAD_VERTSCROLL,viewstarty + 1,TRUE);
			break;

		    case 'T':
			ScrollTo(GAD_VERTSCROLL,0,TRUE);
			break;

		    case 'B':
			ScrollTo(GAD_VERTSCROLL,num_lines,TRUE);
			break;

		} /* switch(toupper(msg->Code)) */
		break;

	    case IDCMP_RAWKEY:
		pagescroll = (0 != (msg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)));
		maxscroll  = (0 != (msg->Qualifier & (IEQUALIFIER_LALT | IEQUALIFIER_RALT | IEQUALIFIER_CONTROL)));

		switch(msg->Code)
		{
		    case CURSORUP:
			ScrollTo(GAD_VERTSCROLL,
				 maxscroll ? 0 : viewstarty - (pagescroll ? visibley - 1 : 1),
				 TRUE);
			break;

		    case CURSORDOWN:
			ScrollTo(GAD_VERTSCROLL,
				 maxscroll ? num_lines : viewstarty + (pagescroll ? visibley - 1 : 1),
				 TRUE);
			break;

		    case CURSORLEFT:
			ScrollTo(GAD_HORIZSCROLL,
				 maxscroll ? 0 : viewstartx - (pagescroll ? visiblex - 1 : 1),
				 TRUE);
			break;

		    case CURSORRIGHT:
			ScrollTo(GAD_HORIZSCROLL,
				 maxscroll ? max_textlen : viewstartx + (pagescroll ? visiblex - 1 : 1),
				 TRUE);
			break;
		}
		break;		
					
	} /* switch(msg->Class) */

	ReplyMsg((struct Message *)msg);

    } /* while ((msg = (struct IntuiMessage *)GetMsg(morewin->UserPort))) */

    return quitme;
}

void HandleAll(void)
{
    BOOL quitme = FALSE;

    ScreenToFront(morewin->WScreen);

    while(!quitme)
    {
	WaitPort(morewin->UserPort);

	quitme = HandleWin();

    } /* while(!quitme) */
}

void moremain(char *title, char *text)
{
    filebuffer = StrDup( text );
    filelen = strlen( filebuffer );
    MakeLineArray();
    GetVisual();
    MakeGadgets();
    MakeWin( title );
    HandleAll();
    Cleanup();
}

void morenmain(char *title, int number, char **texts)
{
    filebuffer = collatestrings( number, texts );
    if( filebuffer == NULL )
    {
      return;
    }
    filelen = strlen( filebuffer );
    MakeLineArray();
    GetVisual();
    MakeGadgets();
    MakeWin( title );
    HandleAll();
    Cleanup();
}

