/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/****************************************************************************************/

#include <exec/memory.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <intuition/imageclass.h>
#include <intuition/gadgetclass.h>
#include <libraries/locale.h>
#include <libraries/gadtools.h>
#include <devices/rawkeycodes.h>

#include <graphics/gfx.h>
#include <utility/hooks.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <proto/alib.h>

#include "global.h"
#include "req.h"

#define CATCOMP_NUMBERS
#include "strings.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define DEBUG 0
#include <aros/debug.h>

/****************************************************************************************/

/* #define USE_WRITEMASK */
#define USE_SIMPLEREFRESH 0

#define DEFAULT_TABSIZE 8

#define INNER_SPACING_X 2
#define INNER_SPACING_Y 2

#define MAX_TEXTLINELEN 4096

#define ARG_TEMPLATE "FILE"

enum
{
    ARG_FILE, 
    NUM_ARGS
};

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

/****************************************************************************************/

struct LineNode
{
    char *text;
    UWORD stringlen;
    UWORD textlen;
    BOOL invert;
};

/****************************************************************************************/

struct IntuitionBase 	*IntuitionBase;
struct GfxBase 		*GfxBase;
struct Library 		*GadToolsBase;
struct Screen 		*scr;
struct DrawInfo 	*dri;
APTR			vi;
struct Menu		*menus;
struct Window		*win;

ULONG 			gotomask, findmask;
UBYTE			filenamebuffer[300];

/****************************************************************************************/

static struct RastPort 	*rp;
static struct RDArgs 	*MyArgs;

static struct Gadget 	*gad[NUM_GADGETS], *firstgadget, *activearrowgad;
static struct Image 	*img[NUM_GADGETS];

static struct LineNode 	*linearray;

static UBYTE 		*filebuffer;

static char 		*filename, s[256], *searchtext;

static BPTR 		fh;

static UWORD 		tabsize = DEFAULT_TABSIZE;

static WORD 		fontwidth, fontheight, borderleft, bordertop;
static WORD 		shinepen, shadowpen, bgpen, textpen;
static WORD 		borderright, borderbottom, visiblex, visibley;
static WORD 		fontbaseline, textstartx, textstarty, textendx, textendy;
static WORD 		textwidth, textheight, viewstartx, viewstarty;
static WORD 		winwidth, winheight;

static ULONG		winmask;
static LONG 		filelen, num_lines, max_textlen;
static LONG 		search_startline, found_line = -1;
static WORD 	    	arrowticker;

static BOOL		in_main_loop;

static IPTR 		Args[NUM_ARGS];

/*********************************************************************************************/

WORD ShowMessage(CONST_STRPTR title, CONST_STRPTR text, CONST_STRPTR gadtext)
{
    struct EasyStruct es;
    
    es.es_StructSize   = sizeof(es);
    es.es_Flags        = 0;
    es.es_Title        = title;
    es.es_TextFormat   = text;
    es.es_GadgetFormat = gadtext;
   
    return EasyRequestArgs(win, &es, NULL, NULL);  
}

/****************************************************************************************/

VOID Cleanup(CONST_STRPTR msg)
{
    WORD rc, i;

    if (msg)
    {
        if (IntuitionBase && !((struct Process *)FindTask(NULL))->pr_CLI)
	{
	    ShowMessage("More", msg, MSG(MSG_OK));
	} else {
	    printf("More: %s\n", msg);
	}
	rc = RETURN_WARN;
    } else {
	rc = RETURN_OK;
    }

    CleanupRequesters();

    KillMenus();
    
    if (win)
    {
	for(i = 0; i < NUM_GADGETS;i++)
	{
	    if (gad[i]) RemoveGadget(win, gad[i]);
	}

	CloseWindow(win);
    }

    for(i = 0; i < NUM_GADGETS;i++)
    {
    	if (gad[i]) DisposeObject(gad[i]);
    }
    for(i = 0; i < NUM_IMAGES;i++)
    {
	if (img[i]) DisposeObject(img[i]);
    }

    if (vi) FreeVisualInfo(vi);
    if (dri) FreeScreenDrawInfo(scr, dri);
    if (scr) UnlockPubScreen(0, scr);

    if (linearray) FreeVec(linearray);
    if (filebuffer) FreeVec(filebuffer);

    if (fh) Close(fh);

    if (GadToolsBase) CloseLibrary(GadToolsBase);
    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);

    if (MyArgs) FreeArgs(MyArgs);
    CleanupLocale();
    
    exit(rc);
}

/****************************************************************************************/

static void DosError(void)
{
    Fault(IoErr(), 0, s, 255);
    if (in_main_loop)
    {
        ShowMessage("More", s, MSG(MSG_OK));
    } else {
        Cleanup(s);
    }
}

/****************************************************************************************/

static void GetArguments(void)
{
    if (!(MyArgs = ReadArgs(ARG_TEMPLATE, Args, 0)))
    {
	DosError();
    }

    filename = (char *)Args[ARG_FILE];
    if (!filename) filename = GetFile();
    if (!filename) Cleanup(NULL);
    
    strncpy(filenamebuffer, filename, 299);
    
}

/****************************************************************************************/

static void OpenLibs(void)
{
    if (!(IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 39)))
    {
        sprintf(s, MSG(MSG_CANT_OPEN_LIB), "intuition.library", 39);	
	Cleanup(s);
    }

    if (!(GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 39)))
    {
        sprintf(s, MSG(MSG_CANT_OPEN_LIB), "graphics.library", 39);	
	Cleanup(s);
    }	

    if (!(GadToolsBase = OpenLibrary("gadtools.library", 39)))
    {
        sprintf(s, MSG(MSG_CANT_OPEN_LIB), "gadtools.library", 39);	
	Cleanup(s);
    }	
}

/****************************************************************************************/

static UWORD CalcTextLen(char *text)
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

/****************************************************************************************/

static BOOL OpenFile(void)
{
    struct LineNode 	*new_linearray;
    UBYTE 		*new_filebuffer;
    LONG  		new_filelen;
    LONG		new_num_lines;

    struct LineNode 	*linepos;
    UBYTE 		*filepos;
    LONG 		flen, act_line;

    if (!(fh = Open(filename, MODE_OLDFILE)))
    {
	DosError();
	return FALSE;
    }

    Seek(fh, 0, OFFSET_END);
    new_filelen = Seek(fh, 0, OFFSET_BEGINNING);

    if (new_filelen < 0)
    {
        Close(fh); fh = 0;
	DosError();
	return FALSE;
    }

    if (!(new_filebuffer = AllocVec(new_filelen + 1, MEMF_PUBLIC)))
    {
        Close(fh); fh = 0;
	if (in_main_loop)
	{
	    ShowMessage("More", MSG(MSG_NO_MEM), MSG(MSG_OK));
	    return FALSE;
	}
	Cleanup(MSG(MSG_NO_MEM));
    }

    new_filebuffer[new_filelen] = '\0';

    if (Read(fh, new_filebuffer, new_filelen) != new_filelen)
    {
        FreeVec(new_filebuffer);
        Close(fh); fh = 0;
	DosError();
	return FALSE;
    }

    Close(fh);fh = 0;
    
    filepos = new_filebuffer;
    flen = new_filelen;

    new_num_lines = 1;

    while(flen--)
    {
	if (*filepos++ == '\n') new_num_lines++;
    }

    new_linearray = AllocVec(new_num_lines * sizeof(struct LineNode), MEMF_PUBLIC | MEMF_CLEAR);
    if (!new_linearray)
    {
        FreeVec(new_filebuffer);
	if (in_main_loop)
	{
	    ShowMessage("More", MSG(MSG_NO_MEM), MSG(MSG_OK));
	    return FALSE;
	}
        Cleanup(MSG(MSG_NO_MEM));
    }
    
    linepos = new_linearray;
    filepos = new_filebuffer;

    act_line = 1;
    max_textlen = 0;
    while(act_line <= new_num_lines)
    {
	linepos->text = (char *)filepos;

	while ((*filepos != '\n') && (*filepos != '\0'))
	{
	    linepos->stringlen++; filepos++;
	}

	*filepos++ = '\0';

	linepos->textlen = CalcTextLen(linepos->text);
	if (linepos->textlen > max_textlen) max_textlen = linepos->textlen;

	linepos++; act_line++;
    }

    if (filebuffer) FreeVec(filebuffer);
    if (linearray) FreeVec(linearray);
    
    filebuffer = new_filebuffer;
    filelen    = new_filelen;
    linearray  = new_linearray;
    num_lines  = new_num_lines;

    return TRUE;
}

/****************************************************************************************/

static void GetVisual(void)
{
    if (!(scr = LockPubScreen(0)))
    {
	Cleanup(MSG(MSG_CANT_LOCK_SCR));
    }

    if (!(dri = GetScreenDrawInfo(scr)))
    {
	Cleanup(MSG(MSG_CANT_GET_DRI));
    }

    if (!(vi = GetVisualInfoA(scr, 0)))
    {
        Cleanup(MSG(MSG_CANT_GET_VI));
    }
    
    shinepen = dri->dri_Pens[SHINEPEN];
    shadowpen = dri->dri_Pens[SHADOWPEN];
    textpen = dri->dri_Pens[TEXTPEN];
    bgpen = dri->dri_Pens[BACKGROUNDPEN];
}

/****************************************************************************************/

static void MakeGadgets(void)
{
    static WORD img2which[] =
    {
   	UPIMAGE, 
   	DOWNIMAGE, 
   	LEFTIMAGE, 
   	RIGHTIMAGE, 
   	SIZEIMAGE
    };
   
    IPTR imagew[NUM_IMAGES], imageh[NUM_IMAGES];
    WORD v_offset, h_offset, btop, i;

    for(i = 0;i < NUM_IMAGES;i++)
    {
	img[i] = NewObject(0, SYSICLASS, SYSIA_DrawInfo	, (Tag) dri, 
				         SYSIA_Which	, (Tag) img2which[i], 
				         TAG_DONE);

	if (!img[i]) Cleanup(MSG(MSG_CANT_CREATE_SYSIMAGE));

	GetAttr(IA_Width, (Object *)img[i], &imagew[i]);
	GetAttr(IA_Height, (Object *)img[i], &imageh[i]);
    }

    btop = scr->WBorTop + dri->dri_Font->tf_YSize + 1;

    v_offset = imagew[IMG_DOWNARROW] / 4;
    h_offset = imageh[IMG_LEFTARROW] / 4;

    firstgadget = 
    gad[GAD_UPARROW] = NewObject(0, BUTTONGCLASS, GA_Image	, (Tag)img[IMG_UPARROW]							, 
						  GA_RelRight	, -imagew[IMG_UPARROW] + 1						, 
						  GA_RelBottom	, -imageh[IMG_DOWNARROW] - imageh[IMG_UPARROW] - imageh[IMG_SIZE] + 1	, 
						  GA_ID		, GAD_UPARROW								, 
						  GA_RightBorder, TRUE									, 
						  GA_Immediate	, TRUE									,
						  GA_RelVerify	, TRUE	    	    	    	    	    	    	    	    	, 
						  TAG_DONE);

    gad[GAD_DOWNARROW] = NewObject(0, BUTTONGCLASS, GA_Image		, (Tag)img[IMG_DOWNARROW]				, 
						    GA_RelRight		, -imagew[IMG_UPARROW] + 1			, 
						    GA_RelBottom	, -imageh[IMG_UPARROW] - imageh[IMG_SIZE] + 1	, 
						    GA_ID		, GAD_DOWNARROW					, 
						    GA_RightBorder	, TRUE						, 
						    GA_Previous		, (Tag)gad[GAD_UPARROW]				, 
						    GA_Immediate	, TRUE						,
						    GA_RelVerify    	, TRUE	    	    	    	    	    	, 
						    TAG_DONE);

    gad[GAD_VERTSCROLL] = NewObject(0, PROPGCLASS, GA_Top		, btop + 1									, 
						   GA_RelRight		, -imagew[IMG_DOWNARROW] + v_offset + 1						, 
						   GA_Width		, imagew[IMG_DOWNARROW] - v_offset * 2						, 
						   GA_RelHeight		, -imageh[IMG_DOWNARROW] - imageh[IMG_UPARROW] - imageh[IMG_SIZE] - btop -2	, 
						   GA_ID		, GAD_VERTSCROLL								, 
						   GA_Previous		, (Tag)gad[GAD_DOWNARROW]								, 
						   GA_RightBorder	, TRUE										, 
						   GA_RelVerify		, TRUE										, 
						   GA_Immediate		, TRUE										, 
						   PGA_NewLook		, TRUE										, 
						   PGA_Borderless	, TRUE										, 
						   PGA_Total		, 100										, 
						   PGA_Visible		, 100										, 
						   PGA_Freedom		, FREEVERT									, 
						   TAG_DONE);

    gad[GAD_RIGHTARROW] = NewObject(0, BUTTONGCLASS, GA_Image		, (Tag)img[IMG_RIGHTARROW]				, 
						     GA_RelRight	, -imagew[IMG_SIZE] - imagew[IMG_RIGHTARROW] + 1, 
						     GA_RelBottom	, -imageh[IMG_RIGHTARROW] + 1			, 
						     GA_ID		, GAD_RIGHTARROW				, 
						     GA_BottomBorder	, TRUE						, 
						     GA_Previous	, (Tag)gad[GAD_VERTSCROLL]				, 
						     GA_Immediate	, TRUE						, 
						     GA_RelVerify   	, TRUE	    	    	    	    	    	,
						     TAG_DONE);

    gad[GAD_LEFTARROW] = NewObject(0, BUTTONGCLASS, GA_Image		, (Tag)img[IMG_LEFTARROW]							, 
						    GA_RelRight		, -imagew[IMG_SIZE] - imagew[IMG_RIGHTARROW] - imagew[IMG_LEFTARROW] + 1, 
						    GA_RelBottom	, -imageh[IMG_RIGHTARROW] + 1						, 
						    GA_ID		, GAD_LEFTARROW								, 
						    GA_BottomBorder	, TRUE									, 
						    GA_Previous		, (Tag)gad[GAD_RIGHTARROW]							, 
						    GA_Immediate	, TRUE									, 
						    GA_RelVerify    	, TRUE	    	    	    	    	    	    	    	    	,
						    TAG_DONE);

    gad[GAD_HORIZSCROLL] = NewObject(0, PROPGCLASS, GA_Left		, scr->WBorLeft, 
						    GA_RelBottom	, -imageh[IMG_LEFTARROW] + h_offset + 1							 , 
						    GA_RelWidth		, -imagew[IMG_LEFTARROW] - imagew[IMG_RIGHTARROW] - imagew[IMG_SIZE] - scr->WBorRight - 2, 
						    GA_Height		, imageh[IMG_LEFTARROW] - (h_offset * 2)						 , 
						    GA_ID		, GAD_HORIZSCROLL									 , 
						    GA_Previous		, (Tag)gad[GAD_LEFTARROW]									 , 
						    GA_BottomBorder	, TRUE											 , 
						    GA_RelVerify	, TRUE											 , 
						    GA_Immediate	, TRUE											 , 
						    PGA_NewLook		, TRUE											 , 
						    PGA_Borderless	, TRUE											 , 
						    PGA_Total		, 100											 , 
						    PGA_Visible		, 100											 , 
						    PGA_Freedom		, FREEHORIZ										 , 
						    TAG_DONE);

    for(i = 0;i < NUM_GADGETS;i++)
    {
	if (!gad[i]) Cleanup(MSG(MSG_CANT_CREATE_GADGET));
    }
}

/****************************************************************************************/

static void CalcVisible(void)
{
    visiblex = (win->Width - borderleft - borderright -
	       INNER_SPACING_X * 2) / fontwidth;

    visibley = (win->Height - bordertop - borderbottom -
	       INNER_SPACING_Y * 2) / fontheight;

    if (visiblex < 1) visiblex = 1;
    if (visibley < 1) visibley = 1;

    textendx = textstartx + visiblex * fontwidth - 1;
    textendy = textstarty + visibley * fontheight - 1;

    textwidth = textendx - textstartx + 1;
    textheight = textendy - textstarty + 1;	
}

/****************************************************************************************/

static void MySetAPen(struct RastPort *rp, LONG reg)
{
    static LONG oldreg = -1;

    if (reg != oldreg)
    {
	oldreg = reg;
	SetAPen(rp, reg);
    }
}

/****************************************************************************************/

static void MySetBPen(struct RastPort *rp, LONG reg)
{
    static LONG oldreg = -1;

    if (reg != oldreg)
    {
	oldreg = reg;
	SetBPen(rp, reg);
    }
}

/****************************************************************************************/

static void DrawTextLine(WORD viewline, WORD columns, BOOL clearright)
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
		MySetAPen(rp, textpen);
		MySetBPen(rp, inverted ? shinepen : bgpen);

		Move(rp, x, 
		        textstarty + (viewline * fontheight) + fontbaseline);

		Text(rp, stringpos, i); 
	    }
	}

    } /* if ((realline >= 0) && (realline < num_lines)) */

    if ((i < visiblex) && clearright)
    {
	MySetAPen(rp, bgpen);
	RectFill(rp, textstartx + (i * fontwidth), 
		     textstarty + (viewline * fontheight), 
		     textendx, 
		     textstarty + (viewline * fontheight) + fontheight - 1);
    }
}

/****************************************************************************************/

static void DrawAllText(void)
{
    WORD y;

    for(y = 0;y < visibley;y++)
    {
	DrawTextLine(y, 0, TRUE);
    }
}

/****************************************************************************************/

static void SetWinTitle(void)
{
    static UBYTE wintitle[256];
    BPTR 	 lock;

    strcpy(s, filename);
    if ((lock = Lock(filename, SHARED_LOCK)))
    {
	NameFromLock(lock, s, 255);
	UnLock(lock);
    }
    sprintf(wintitle, MSG(MSG_WIN_TITLE), s, num_lines, filelen);

    SetWindowTitles(win, wintitle, (UBYTE *)~0L);
}

/****************************************************************************************/

static void MakeWin(void)
{	
    if (!(win = OpenWindowTags(NULL, WA_PubScreen	, (IPTR)scr	        , 
				  WA_Left		, 0			, 
				  WA_Top		, scr->BarHeight + 1	, 
				  WA_Width		, 600			, 
				  WA_Height		, 300			, 
				  WA_AutoAdjust		, TRUE			,
			          USE_SIMPLEREFRESH ? 
                                  WA_SimpleRefresh  :
                                  TAG_IGNORE            , TRUE			, 
				  WA_CloseGadget	, TRUE			, 
				  WA_DepthGadget	, TRUE			, 
				  WA_DragBar		, TRUE			, 
				  WA_SizeGadget		, TRUE			, 
				  WA_SizeBBottom	, TRUE			, 
				  WA_SizeBRight		, TRUE			, 
				  WA_Activate		, TRUE			, 
				  WA_Gadgets		, (IPTR)firstgadget	, 
				  WA_MinWidth		, 100			, 
				  WA_MinHeight		, 100			, 
				  WA_MaxWidth		, scr->Width		, 
				  WA_MaxHeight		, scr->Height		, 
				  WA_ReportMouse	, TRUE			, 
				  WA_NewLookMenus	, TRUE			,
				  WA_IDCMP		, IDCMP_CLOSEWINDOW   |
				  			  IDCMP_NEWSIZE       |
					    		  IDCMP_GADGETDOWN    | 
							  IDCMP_GADGETUP      |
					    		  IDCMP_MOUSEMOVE     |
							  IDCMP_VANILLAKEY    |
							  IDCMP_INTUITICKS    |
                               (USE_SIMPLEREFRESH != 0) * IDCMP_REFRESHWINDOW |
					    		  IDCMP_RAWKEY        |
							  IDCMP_MENUPICK	, 
				  TAG_DONE)))
    {
	Cleanup(MSG(MSG_CANT_CREATE_WIN));
    }
    
    SetWinTitle();
    
    winmask = 1L << win->UserPort->mp_SigBit;

    winwidth = win->Width;
    winheight = win->Height;

    rp = win->RPort;

    SetDrMd(rp, JAM2);

#ifdef USE_WRITEMASK
    SetWriteMask(rp, 0x3);
#endif

    fontwidth = rp->TxWidth;
    fontheight = rp->TxHeight;
    fontbaseline = rp->TxBaseline;

    borderleft = win->BorderLeft;
    bordertop = win->BorderTop;
    borderright = win->BorderRight;
    borderbottom = win->BorderBottom;

    textstartx = borderleft + INNER_SPACING_X;
    textstarty = bordertop + INNER_SPACING_Y;

    CalcVisible();

    SetGadgetAttrs(gad[GAD_HORIZSCROLL], win, 0, PGA_Top	, 0		, 
						 PGA_Total	, max_textlen	, 
						 PGA_Visible	, visiblex	, 
						 TAG_DONE);

    SetGadgetAttrs(gad[GAD_VERTSCROLL], win, 0, PGA_Top		, 0		, 
						PGA_Total	, num_lines	, 
						PGA_Visible	, visibley	, 
						TAG_DONE);

    DrawAllText();
    
    SetMenuStrip(win, menus);
}

/****************************************************************************************/

static void NewWinSize(void)
{
    WORD new_winwidth, new_winheight;

    new_winwidth = win->Width;
    new_winheight = win->Height;

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

    SetGadgetAttrs(gad[GAD_HORIZSCROLL], win, 0, PGA_Top	, viewstartx	, 
						 PGA_Visible	, visiblex	, 
						 TAG_DONE);

    SetGadgetAttrs(gad[GAD_VERTSCROLL], win, 0, PGA_Top		, viewstarty	, 
						PGA_Visible	, visibley	, 
						TAG_DONE);

    if (new_winwidth < winwidth)
    {
	MySetAPen(rp, bgpen);
	RectFill(rp, textendx + 1, 
		     bordertop + INNER_SPACING_Y, 
		     new_winwidth - borderright - 1, 
		     new_winheight - borderbottom - 1);
    }

    if (new_winheight < winheight)
    {
	MySetAPen(rp, bgpen);
	RectFill(rp, borderleft + INNER_SPACING_X, 
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

/****************************************************************************************/

#ifdef USE_SIMPLEREFRESH

/****************************************************************************************/

static void RefreshAll(void)
{
    DrawAllText();
}

/****************************************************************************************/

static void HandleRefresh(void)
{
    BeginRefresh(win);
    RefreshAll();
    EndRefresh(win, TRUE);
}

/****************************************************************************************/

#endif /* USE_SIMPLEREFRESH */

/****************************************************************************************/

static void ScrollTo(WORD gadid, LONG top, BOOL refreshprop)
{
    LONG y, dx, dy;

    MySetBPen(rp, bgpen);

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
		    SetGadgetAttrs(gad[gadid], win, 0, PGA_Top	, viewstarty, 
						       TAG_DONE);
		}

		if (abs(dy) >= visibley)
		{
		    DrawAllText();
		} else {
		    if (dy > 0)
		    {

		    #ifdef USE_SIMPLEREFRESH
			ScrollRaster(rp, 
				     0, 
				     fontheight * dy, 
				     textstartx, 
				     textstarty, 
				     textendx, 
				     textendy);

			if (rp->Layer->Flags & LAYERREFRESH)
			{
			    HandleRefresh();
			}
		    #else
			ClipBlit(rp, textstartx, 
				     textstarty + dy * fontheight, 
				 rp, textstartx, 
				     textstarty, 
				     textwidth, 
				     textheight - dy * fontheight, 
				 192);
		    #endif

			for (y = visibley - dy;y < visibley;y++)
			{
			    DrawTextLine(y, 0, TRUE);
			}
		    } else {
			dy = -dy;

		    #ifdef USE_SIMPLEREFRESH
			ScrollRaster(rp, 
				     0, 
				     -fontheight * dy, 
				     textstartx, 
				     textstarty, 
				     textendx, 
				     textendy);

			if (rp->Layer->Flags & LAYERREFRESH)
			{
			    HandleRefresh();
			}

		    #else
			ClipBlit(rp, textstartx, 
				     textstarty, 
				 rp, textstartx, 
				     textstarty + dy * fontheight, 
				     textwidth, 
				     textheight - dy * fontheight, 
				 192);
		    #endif
			for (y = 0;y < dy;y++)
			{
			    DrawTextLine(y, 0, TRUE);
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
		    SetGadgetAttrs(gad[gadid], win, 0, PGA_Top	, viewstartx, 
				   		       TAG_DONE);
		}

		if (abs(dx) >= visiblex)
		{
		    DrawAllText();
		} else {
		    if (dx > 0)
		    {

		    #ifdef USE_SIMPLEREFRESH
			ScrollRaster(rp, 
				     fontwidth * dx, 
				     0, 
				     textstartx, 
				     textstarty, 
				     textendx, 
				     textendy);

			if (rp->Layer->Flags & LAYERREFRESH)
			{
			    HandleRefresh();
			}
		    #else
			ClipBlit(rp, textstartx + dx * fontwidth, 
				     textstarty, 
			         rp, textstartx, 
				     textstarty, 
				     textwidth - dx * fontwidth, 
				     textheight, 
				 192);
		    #endif
			for (y = 0;y < visibley;y++)
			{
			    DrawTextLine(y, dx, TRUE);
			}

		    } else {
			dx = -dx;

		    #ifdef USE_SIMPLEREFRESH
			ScrollRaster(rp, 
				     -fontwidth * dx, 
				     0, 
				     textstartx, 
				     textstarty, 
				     textendx, 
				     textendy);

			if (rp->Layer->Flags & LAYERREFRESH)
			{
			    HandleRefresh();
			}

		    #else
			ClipBlit(rp, textstartx, 
				     textstarty, 
			         rp, textstartx + dx * fontwidth, 
				     textstarty, 
				     textwidth - dx * fontwidth, 
				     textheight, 
			         192);
		    #endif
			for (y = 0;y < visibley;y++)
			{
			    DrawTextLine(y, -dx, TRUE);
			}
		    }
		}

	    } /* if (top != viewstartx ) */
	    break;

    } /* switch(gadid) */
	
}

/****************************************************************************************/

static void HandleScrollGadget(WORD gadid)
{
    struct IntuiMessage *msg;
    IPTR top;
    BOOL ok=FALSE;

    while (!ok)
    {
	WaitPort(win->UserPort);
	while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort)))
	{
	    switch (msg->Class)
	    {
		case IDCMP_GADGETUP:
		    ok=TRUE;
		    /* fall through */

		case IDCMP_MOUSEMOVE:
		    GetAttr(PGA_Top, (Object *)gad[gadid], &top);
		    ScrollTo(gadid, top, FALSE);
		    break;

#ifdef USE_SIMPLEREFRESH
		case IDCMP_REFRESHWINDOW:
		    HandleRefresh();
		    break;
#endif

	    } /* switch (msg->Class) */
	    ReplyMsg((struct Message *)msg);

	} /* while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort))) */

    } /* while (!ok) */
}

/****************************************************************************************/

static BOOL FindString(struct LineNode *ln, char *search, LONG searchlen)
{
    char *text = ln->text;
    LONG textlen = ln->stringlen;
    LONG i;
    BOOL rc = FALSE;

    textlen -= searchlen;

    while(textlen >= 0)
    {
	for(i = 0;i < searchlen;i++)
	{
	    if (toupper(text[i]) != toupper(search[i])) break;
	}

	if (i == searchlen)
	{
	    rc = TRUE;
	    break;
	}
	text++;textlen--;
    }

    return rc;
}

/****************************************************************************************/

static void DoSearch(WORD kind)
{
    LONG line, searchlen;
    BOOL done = FALSE;

    if (!searchtext) return;

    searchlen = strlen(searchtext);
    if (searchlen == 0) return;

    if (kind == SEARCH_NEW)
    {
	search_startline = 0;
	kind = SEARCH_NEXT;
    }

    line = search_startline;

    while(!done)
    {
	if (FindString(&linearray[line], searchtext, searchlen))
	{
	    done = TRUE;
	    if (found_line >= 0)
	    {
		linearray[found_line].invert = FALSE;
		DrawTextLine(found_line - viewstarty, 0, TRUE);
	    }

	    ScrollTo(GAD_VERTSCROLL, line - visibley / 2, TRUE);

	    found_line = line;
	    linearray[found_line].invert = TRUE;
	    DrawTextLine(found_line - viewstarty, 0, TRUE);
	}

	if (kind == SEARCH_NEXT)
	{
	    line++;
	    if (line < num_lines)
	    {
		search_startline = line;
	    } else {
		done = TRUE;
	    }
	} else {
	    line--;
	    if (line >= 0)
	    {
		 search_startline = line;
	    } else {
		 done = TRUE;
	    }
	}

    } /* while(!done) */

}

/****************************************************************************************/

static BOOL HandleWin(void)
{
    struct IntuiMessage *msg;
    struct MenuItem	*item;
    WORD 		gadid, delta;
    UWORD		men, code;
    UBYTE		key;
    BOOL 		pagescroll, maxscroll, quitme = FALSE;

    while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort)))
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
    	    	arrowticker = 3;
		
		switch(gadid)
		{
		    case GAD_HORIZSCROLL:
		    case GAD_VERTSCROLL:
			HandleScrollGadget(gadid);
			break;

		    case GAD_UPARROW:
		    	activearrowgad = (struct Gadget *)msg->IAddress;
			ScrollTo(GAD_VERTSCROLL, viewstarty - 1, TRUE);
			break;
			
		    case GAD_DOWNARROW:
		    	activearrowgad = (struct Gadget *)msg->IAddress;
			ScrollTo(GAD_VERTSCROLL, viewstarty + 1, TRUE);
			break;
			
		    case GAD_LEFTARROW:
		    	activearrowgad = (struct Gadget *)msg->IAddress;
			ScrollTo(GAD_HORIZSCROLL, viewstartx - 1, TRUE);
			break;
			
		    case GAD_RIGHTARROW:
		    	activearrowgad = (struct Gadget *)msg->IAddress;
			ScrollTo(GAD_HORIZSCROLL, viewstartx + 1, TRUE);
			break;
		};
		break;

    	    case IDCMP_INTUITICKS:
	    	if (activearrowgad)
		{
		    if (arrowticker)
		    {
		    	arrowticker--;
		    }
		    else if (activearrowgad->Flags & GFLG_SELECTED)
    	    	    {
		    	switch(activearrowgad->GadgetID)
			{
			    case GAD_UPARROW:
			    	ScrollTo(GAD_VERTSCROLL, viewstarty - 1, TRUE);
				break;
				
			    case GAD_DOWNARROW:
			    	ScrollTo(GAD_VERTSCROLL, viewstarty + 1, TRUE);
				break;
				
			    case GAD_LEFTARROW:
			    	ScrollTo(GAD_HORIZSCROLL, viewstartx - 1, TRUE);
				break;
				
			    case GAD_RIGHTARROW:
			    	ScrollTo(GAD_HORIZSCROLL, viewstartx + 1, TRUE);
				break;
			}
		    }
		}
		break;
		
	    case IDCMP_GADGETUP:
	    	activearrowgad = NULL;
	    	break;
		
	    case IDCMP_VANILLAKEY:
	    	key = toupper(msg->Code);
		if (key == 27)
		{
		    quitme = TRUE;
		}
		else if (key == ' ')
		{
		    ScrollTo(GAD_VERTSCROLL, viewstarty + visibley - 1, TRUE);
		}
		else if (key == 8)
		{
		    ScrollTo(GAD_VERTSCROLL, viewstarty - (visibley - 1), TRUE);
		}
		else if (key == 13)
		{
		    ScrollTo(GAD_VERTSCROLL, viewstarty + 1, TRUE);
		}
		else if (strchr(MSG(MSG_SHORTCUT_TOP), key))
		{
		    ScrollTo(GAD_VERTSCROLL, 0, TRUE);
		}
		else if (strchr(MSG(MSG_SHORTCUT_BOTTOM), key))
		{
		    ScrollTo(GAD_VERTSCROLL, num_lines, TRUE);
		}
		else if (strchr(MSG(MSG_SHORTCUT_JUMP), key))
		{
		    Make_Goto_Requester();
		}
		else if (strchr(MSG(MSG_SHORTCUT_FIND), key))
		{
		    Make_Find_Requester();
		}
		else if (strchr(MSG(MSG_SHORTCUT_NEXT), key))
		{
		    DoSearch(SEARCH_NEXT);
		}
		else if (strchr(MSG(MSG_SHORTCUT_NEXT), key))
		{
		    DoSearch(SEARCH_PREV);
		}
		break;

	    case IDCMP_RAWKEY:
		pagescroll = (0 != (msg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)));
		maxscroll  = (0 != (msg->Qualifier & (IEQUALIFIER_LALT | IEQUALIFIER_RALT | IEQUALIFIER_CONTROL)));

    	    	code = msg->Code; delta = 1;
		
		switch(code)
    	    	{
		    case RAWKEY_NM_WHEEL_UP:
		    	code = CURSORUP;
			delta = 3;
			break;
			
		    case RAWKEY_NM_WHEEL_DOWN:
		    	code = CURSORDOWN;
			delta = 3;
			break;
			
		    case RAWKEY_NM_WHEEL_LEFT:
		    	code = CURSORLEFT;
			delta = 3;
			break;
			
		    case RAWKEY_NM_WHEEL_RIGHT:
		    	code = CURSORRIGHT;
			delta = 3;
			break;
		}
		
		switch(code)
		{
		    case CURSORUP:
			ScrollTo(GAD_VERTSCROLL, 
				 maxscroll ? 0 : viewstarty - (pagescroll ? visibley - 1 : delta), 
				 TRUE);
			break;

		    case CURSORDOWN:
			ScrollTo(GAD_VERTSCROLL, 
				 maxscroll ? num_lines : viewstarty + (pagescroll ? visibley - 1 : delta), 
				 TRUE);
			break;

		    case CURSORLEFT:
			ScrollTo(GAD_HORIZSCROLL, 
				 maxscroll ? 0 : viewstartx - (pagescroll ? visiblex - 1 : delta), 
				 TRUE);
			break;

		    case CURSORRIGHT:
			ScrollTo(GAD_HORIZSCROLL, 
				 maxscroll ? max_textlen : viewstartx + (pagescroll ? visiblex - 1 : delta), 
				 TRUE);
			break;
			
		    case RAWKEY_HOME:
		        ScrollTo(GAD_VERTSCROLL, 0, TRUE);
			break;
			
		    case RAWKEY_END:
		        ScrollTo(GAD_VERTSCROLL, num_lines, TRUE);
			break;
			
		    case RAWKEY_PAGEUP:
		        ScrollTo(GAD_VERTSCROLL, viewstarty - (visibley - 1), TRUE);
			break;
			
		    case RAWKEY_PAGEDOWN:
		        ScrollTo(GAD_VERTSCROLL, viewstarty + (visibley - 1), TRUE);
			break;
			
		} /* switch(msg->Code) */
		break;		

	    case IDCMP_MENUPICK:
		men = msg->Code;		
		while(men != MENUNULL)
		{
		    if ((item = ItemAddress(menus, men)))
		    {
			switch((ULONG)GTMENUITEM_USERDATA(item))
			{
			    case MSG_MEN_PROJECT_ABOUT:
				break;

			    case MSG_MEN_PROJECT_QUIT:
				quitme = TRUE;
				break;

			    case MSG_MEN_PROJECT_OPEN:
			    	if ((filename = GetFile()))
				{
				    if (OpenFile())
				    {
					strncpy(filenamebuffer, filename, 299);
				        SetWinTitle();
					MySetAPen(rp, bgpen);
					RectFill(rp, textstartx, textstarty, textendx, textendy);
					
					CalcVisible();
					
					viewstartx = viewstarty = 0;
					
					SetGadgetAttrs(gad[GAD_HORIZSCROLL], win, 0, PGA_Top	, viewstartx	, 
										     PGA_Visible, visiblex	,
										     PGA_Total	, max_textlen	,
										     TAG_DONE);

					SetGadgetAttrs(gad[GAD_VERTSCROLL], win, 0, PGA_Top	, viewstarty	, 
										    PGA_Visible	, visibley	, 
										    PGA_Total	, num_lines	,
										    TAG_DONE);
					DrawAllText();   					
					
				    } /* if (OpenFile()) */
				    
				} /* if ((filename = GetFile())) */
				
				break;

			    case MSG_MEN_NAVIGATION_FIND:
			    	Make_Find_Requester();
				break;
				
			    case MSG_MEN_NAVIGATION_FIND_NEXT:
			        DoSearch(SEARCH_NEXT);
				break;
				
			    case MSG_MEN_NAVIGATION_FIND_PREV:
			        DoSearch(SEARCH_PREV);
				break;
			
			    case MSG_MEN_NAVIGATION_JUMP:
			        Make_Goto_Requester();
				break;

			} /* switch(GTMENUITEM_USERDATA(item)) */

		        men = item->NextSelect;
		    } else {
		        men = MENUNULL;
		    }

		} /* while(men != MENUNULL) */
	        break;	
		    				
	} /* switch(msg->Class) */

	ReplyMsg((struct Message *)msg);

    } /* while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort))) */

    return quitme;
}

/****************************************************************************************/

static void HandleAll(void)
{
    ULONG sigs;
    LONG  line;
    WORD  search_kind;
    BOOL  quitme = FALSE;

    in_main_loop = TRUE;
    
    ScreenToFront(win->WScreen);

    while(!quitme)
    {
	sigs = Wait(winmask | gotomask | findmask);

	if (sigs & winmask) quitme = HandleWin();

	if (sigs & gotomask)
	{
	    if (Handle_Goto_Requester(&line))
	    {
		ScrollTo(GAD_VERTSCROLL, line - 1, TRUE);
	    }
	}

	if (sigs & findmask)
	{
	    if ((search_kind = Handle_Find_Requester(&searchtext)))
	    {
		DoSearch(search_kind);
	    }
	}

    } /* while(!quitme) */
}

/****************************************************************************************/

int main(void)
{
    InitLocale("System/Utilities/More.catalog", 1);
    InitMenus();
    GetArguments();
    OpenLibs();
    OpenFile();
    GetVisual();
    MakeGadgets();
    MakeMenus();
    MakeWin();
    HandleAll();
    Cleanup(0);
    return 0;
}

/****************************************************************************************/
