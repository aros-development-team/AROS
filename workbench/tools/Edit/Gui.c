/**************************************************************
**** gui.c: main interface related procedures.             ****
**** Free software under GNU license, written on 3/1/2000  ****
**** © T.Pierron, C.Guillaume.                             ****
**************************************************************/

#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <intuition/imageclass.h>
#include <intuition/gadgetclass.h>
#include <libraries/gadtools.h>
#include <dos/dos.h>
#include <exec/memory.h>
#include "Jed.h"
#include "Prefs.h"
#include "IPC_Prefs.h"
#include "Version.h"
#include "Rawkey.h"
#include "Events.h"
#include "Search.h"
#include "ProtoTypes.h"

#define  CATCOMP_NUMBERS
#define  CATCOMP_STRINGS
#include "strings.h"

struct Screen    *Scr  = NULL;
void             *Vi   = NULL;
struct Window    *Wnd  = NULL;
struct Menu      *Menu = NULL;
struct Scroll    *Prop = NULL;
struct DrawInfo  *di   = NULL;
struct RastPort  *RP, RPT;
ULONG  sigmainwnd=0;
UBYTE  boopsigad;					/* Type of right prop gadget of window */

/* Colors pens */
struct pens pen = {0,1,3,2,2,1,2,1,3,2,3,0};
UWORD  fginfo, bginfo;			/* Information window */

#define	TMPLINF					"9999/99999"

struct NewMenu newmenu[] =
{
	{NM_TITLE, MSG_PROJECTTITLE_STR, NULL, 0, 0L, NULL},
	{	NM_ITEM, MSG_NEWEMPTYFILE_STR,	"N",     0, 0L, (APTR)101},
	{	NM_ITEM, MSG_OPENNEWFILE_STR, 	"O",     0, 0L, (APTR)102},
	{	NM_ITEM, MSG_LOADINPRJ_STR,		"L",     0, 0L, (APTR)103},
	{	NM_ITEM, (STRPTR)NM_BARLABEL,		NULL,    0, 0L, NULL},
	{	NM_ITEM, MSG_SAVEFILE_STR,			"W",     0, 0L, (APTR)105},
	{	NM_ITEM, MSG_SAVEFILEAS_STR,		NULL,    0, 0L, (APTR)106},
	{	NM_ITEM, MSG_SAVECHANGES_STR,		"F",     0, 0L, (APTR)107},
	{	NM_ITEM, (STRPTR)NM_BARLABEL,		NULL,    0, 0L, NULL},
	{	NM_ITEM, MSG_PRINTFILE_STR,		"P",     0, 0L, (APTR)108},
	{	NM_ITEM, (STRPTR)NM_BARLABEL,		NULL,    0, 0L, NULL},
	{	NM_ITEM, MSG_INFORMATION_STR,		"?",     0, 0L, (APTR)109},
	{	NM_ITEM, (STRPTR)NM_BARLABEL,		NULL,    0, 0L, NULL},
	{  NM_ITEM, MSG_CLOSE_STR,				"Ctrl Q",NM_COMMANDSTRING, 0L, (APTR)111},
	{	NM_ITEM, MSG_QUIT_STR,				"Q",     0, 0L, (APTR)112},

	{NM_TITLE, MSG_EDITTITLE_STR, NULL, 0, 0L, NULL},
	{	NM_ITEM, MSG_CUT_STR,				"X",  0, 0L, (APTR)201},
	{	NM_ITEM, MSG_COPY_STR,				"C",  0, 0L, (APTR)202},
	{	NM_ITEM, MSG_PASTE_STR,				"V",  0, 0L, (APTR)203},
	{	NM_ITEM, (STRPTR)NM_BARLABEL,		NULL, 0, 0L, NULL},
	{	NM_ITEM, MSG_MARK_STR,				"B",  0, 0L, (APTR)204},
	{	NM_ITEM, MSG_MARKCOLUMN_STR,		NULL, 0, 0L, (APTR)205},
	{	NM_ITEM, MSG_SELECTALL_STR,		"A",  0, 0L, (APTR)206},
	{	NM_ITEM, (STRPTR)NM_BARLABEL,		NULL, 0, 0L, NULL},
	{	NM_ITEM, MSG_DELLINE_STR,			"K",  0, 0L, (APTR)207},
	{	NM_ITEM, MSG_SUBTOOLS_STR,			NULL, 0, 0L, NULL},
	{		NM_SUB, MSG_INDENT_STR,			"Amiga Tab",NM_COMMANDSTRING, 0L, (APTR)2071},
	{		NM_SUB, MSG_UNINDENT_STR,		NULL, 0, 0L, (APTR)2072},
	{		NM_SUB, NM_BARLABEL,				NULL, 0, 0L, NULL},
	{		NM_SUB, MSG_UPPERCASE_STR,		"Ctrl \\",  NM_COMMANDSTRING, 0L, (APTR)2073},
	{		NM_SUB, MSG_LOWERCASE_STR,		"Ctrl /",   NM_COMMANDSTRING, 0L, (APTR)2074},
	{		NM_SUB, MSG_TOGGLECASE_STR,	"G",  0, 0L, (APTR)2075},
	{	NM_ITEM, MSG_INSERTFILE_STR,		"I",  0, 0L, (APTR)208},
	{	NM_ITEM, (STRPTR)NM_BARLABEL,		NULL, 0, 0L, NULL},
	{	NM_ITEM, MSG_UNDO_STR,				"U",  0, 0L, (APTR)209},
	{	NM_ITEM, MSG_REDO_STR,				NULL, 0, 0L, (APTR)210},

	{NM_TITLE, MSG_SEARCHTITLE_STR, NULL, 0, 0L, NULL},
	{	NM_ITEM, MSG_FIND_STR,				"S",      0, 0L, (APTR)301},
	{	NM_ITEM, MSG_REPLACE_STR,			"R",      0, 0L, (APTR)302},
	{	NM_ITEM, MSG_SUBFIND_STR,			NULL,     0, 0L, NULL},
	{		NM_SUB, MSG_NEXTFIND_STR,		"Ctrl N", NM_COMMANDSTRING, 0L, (APTR)3031},
	{		NM_SUB, MSG_PREVFIND_STR,		"Ctrl P", NM_COMMANDSTRING, 0L, (APTR)3032},
	{		NM_SUB, MSG_NEXTREPLACE_STR,	"Ctrl R", NM_COMMANDSTRING, 0L, (APTR)3033},
	{	NM_ITEM, (STRPTR)NM_BARLABEL,		NULL,     0, 0L, NULL},
	{	NM_ITEM, MSG_PREVPAGE_STR,			"Pg Up",  NM_COMMANDSTRING, 0L, (APTR)304},
	{	NM_ITEM, MSG_NEXTPAGE_STR,			"Pg Dn",  NM_COMMANDSTRING, 0L, (APTR)305},
	{	NM_ITEM, MSG_GOTOLINE_STR,			"J",      0, 0L, (APTR)306},
	{	NM_ITEM, MSG_FINDBRACKET_STR,		"[",      0, 0L, (APTR)307},
	{	NM_ITEM, MSG_LASTMODIF_STR,		"Ctrl Z", NM_COMMANDSTRING, 0L, (APTR)308},
	{	NM_ITEM, (STRPTR)NM_BARLABEL,		NULL,     0, 0L, NULL},
	{	NM_ITEM, MSG_BEGOFLINE_STR,		"Home",   NM_COMMANDSTRING, 0L, (APTR)309},
	{	NM_ITEM, MSG_ENDOFLINE_STR,		"End",    NM_COMMANDSTRING, 0L, (APTR)310},

	{NM_TITLE, MSG_MACROTITLE_STR, NULL, 0, 0L, NULL},
	{	NM_ITEM, MSG_STARTRECORD_STR,		"Ctrl [", NM_COMMANDSTRING, 0L, (APTR)401},
	{	NM_ITEM, MSG_STOPRECORD_STR,		"Ctrl ]", NM_COMMANDSTRING, 0L, (APTR)402},
	{	NM_ITEM, (STRPTR)NM_BARLABEL,		NULL,     0, 0L, NULL},
	{	NM_ITEM, MSG_PLAYMACRO_STR,		"M",      0, 0L, (APTR)403},
	{	NM_ITEM, MSG_REPEATMACRO_STR,		NULL,     0, 0L, (APTR)404},

	{NM_TITLE, MSG_ENVTITLE_STR,			NULL, 0, 0L, NULL},
	{	NM_ITEM, MSG_SCRMODE_STR,			"D",  0, 0L, (APTR)501},
	{	NM_ITEM, MSG_FONTS_STR,				NULL, 0, 0L, (APTR)502},
	{	NM_ITEM, MSG_PREFS_STR,				"T",  0, 0L, (APTR)503},
	{	NM_ITEM, (STRPTR)NM_BARLABEL,		NULL, 0, 0L, NULL},
	{	NM_ITEM, MSG_LOADPREFS_STR,		NULL, 0, 0L, (APTR)504},
	{	NM_ITEM, MSG_SAVEPREFS_STR,		NULL, 0, 0L, (APTR)505},
	{	NM_ITEM, MSG_SAVEPREFSAS_STR,		NULL, 0, 0L, (APTR)506},

	{NM_END, 0, 0, 0, 0, 0}
};

/* Draw info pens, for giving screen a 2.0 newlook */
UWORD DriPens = (UWORD)~0;

/* Default screen and window titles */
UBYTE ScrTitle[] = SVERID;
UBYTE WinTitle[] = APPNAME " " VERSION;

/* NewWindow struct, when opening a window */
struct NewWindow Template =
{
	0,0,0,0, 1,1,					/* LE,TE,Wid,Hei, Pens */
	IDCMP_MENUPICK   | IDCMP_MOUSEMOVE | IDCMP_CLOSEWINDOW  | IDCMP_NEWSIZE |
	IDCMP_GADGETDOWN | IDCMP_GADGETUP  | IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY,
	WFLG_SIZEGADGET  | WFLG_SIZEBRIGHT | WFLG_CLOSEGADGET   | WFLG_NEWLOOKMENUS  |
	WFLG_DEPTHGADGET | WFLG_ACTIVATE   | WFLG_REPORTMOUSE   | WFLG_SMART_REFRESH |
	WFLG_DRAGBAR,
	NULL,								/* FirstGadget */
	NULL,								/* CheckMark */
	WinTitle,						/* Title */
	NULL,								/* Screen */
	NULL,								/* BitMap */
	320, 60, 0xffff, 0xffff,	/* Min & Max size */
	CUSTOMSCREEN
};

/* Different values of NewWindow Flags field */
#define	NORMAL_FLAGS	\
	WFLG_SIZEGADGET  | WFLG_SIZEBRIGHT | WFLG_CLOSEGADGET | WFLG_NEWLOOKMENUS  |	\
	WFLG_DEPTHGADGET | WFLG_ACTIVATE   | WFLG_REPORTMOUSE | WFLG_SMART_REFRESH |	\
	WFLG_DRAGBAR
#define	BACKDROP_FLAGS	\
	WFLG_NEWLOOKMENUS | WFLG_ACTIVATE | WFLG_REPORTMOUSE | WFLG_SMART_REFRESH |	\
	WFLG_BACKDROP     | WFLG_DRAGBAR  | WFLG_BORDERLESS
#define	PROP_FLAGS \
	AUTOKNOB | FREEVERT | PROPNEWLOOK | PROPBORDERLESS

/* Template used to quickly fill constant fields */
struct Scroll ScrollBar = {
{	/* PropGadget */
	NULL, 0,0, 0,0,
	GFLG_RELRIGHT|GFLG_RELHEIGHT,
	GACT_RIGHTBORDER|GACT_RELVERIFY|GACT_FOLLOWMOUSE,
	GTYP_PROPGADGET,
	NULL, NULL, NULL, 0, NULL,
	0, NULL },
{	/* Up-gadget */
	NULL, 0,0, 0,0,
	GFLG_RELRIGHT|GFLG_RELBOTTOM|GFLG_GADGHIMAGE|GFLG_GADGIMAGE,
	GACT_RIGHTBORDER|GACT_RELVERIFY|GACT_IMMEDIATE,
	GTYP_BOOLGADGET,
	NULL, NULL, NULL, 0, NULL,
	1, NULL },
{	/* Down-Gadget */
	NULL, 0,0, 0,0,
	GFLG_RELRIGHT|GFLG_RELBOTTOM|GFLG_GADGHIMAGE|GFLG_GADGIMAGE,
	GACT_RIGHTBORDER|GACT_RELVERIFY|GACT_IMMEDIATE,
	GTYP_BOOLGADGET,
	NULL, NULL, NULL, 0, NULL,
	2, NULL },
{	/* PropInfo */
	PROP_FLAGS,
	MAXPOT, MAXPOT,
	MAXBODY, MAXBODY }
	/* Other values may be NULL */
};

/* Menu color information */
ULONG MenuTags[] = {
	GTMN_FrontPen, 0L,
	TAG_DONE
};

/* Information about edit area */
struct gv gui;

void recalc_sigbits(void)
{
	extern ULONG sigbits, swinsig, sigport;
	sigmainwnd = 1 << Wnd->UserPort->mp_SigBit;
	sigbits = SIGBREAKF_CTRL_C | sigmainwnd | swinsig | sigport;
}

/*** Search for text in the newmenu table ***/
CONST_STRPTR GetMenuText(ULONG MenuID)
{
	struct NewMenu *nm;
	for(nm = newmenu; nm->nm_UserData != (APTR)MenuID; nm++);
	return nm->nm_Label;
}

/*** Adjust internal variables according to current window size ***/
void adjust_win(struct Window *wnd, BYTE PrjBar)
{
	UWORD tmp;

	gui.ysize   = prefs.txtfont->tf_YSize;      /* XSIZE */
	gui.xsize   = prefs.txtfont->tf_XSize;      /* YSIZE */
	gui.basel   = prefs.txtfont->tf_Baseline;   /* BASEL */

	gui.oldtop  = wnd->BorderTop+boopsigad;
	gui.top     = (PrjBar ? (gui.oldtop -= boopsigad)+prefs.scrfont->tf_YSize+6 : gui.oldtop);
	gui.topcurs = gui.top+BASEL;
	gui.nbline  = (wnd->Height - wnd->BorderBottom - gui.top) / YSIZE;
	tmp         = gui.nbline * YSIZE;
	gui.bottom  = gui.top + tmp - 1;
	gui.botcurs = gui.topcurs + tmp - YSIZE;
	gui.left    = wnd->BorderLeft;
	tmp         = (boopsigad ? Prop->scroller.Width : wnd->BorderRight);
	gui.nbcol   = (wnd->Width - tmp - gui.left - 2) / XSIZE;
	gui.right   = wnd->Width - tmp - 1;
	gui.rcurs   = gui.left + gui.nbcol * XSIZE;
	gui.xstep   = (gui.nbcol < 8 ? 1 : gui.nbcol >> 3);

	gui.yinfo   = Scr->RastPort.Font->tf_Baseline + 1;
	gui.xinfo   = (boopsigad ? Scr->Width : wnd->Width) - gui.depthwid*3 -
	              TextLength(&Scr->RastPort,TMPLINF,sizeof(TMPLINF)-1);

	/* Scrolling mask */
	gui.selmask = (pen.fg | pen.bg | pen.fgfill | pen.bgfill) ^
	              (pen.fg & pen.bg & pen.fgfill & pen.bgfill);
	gui.txtmask = pen.fg ^ pen.bg;
}

/*** Clear the right column and bottom line where no chars can be written ***/
void clear_brcorner(void)
{
	SetAPen(RP,pen.bg);
	if(boopsigad || gui.top!=gui.oldtop) Move(RP,gui.left,gui.top-1),Draw(RP,gui.right,RP->cp_y);
	RectFill(RP,gui.rcurs,gui.top,gui.right,gui.bottom);
	RectFill(RP,gui.left,gui.bottom+1,gui.right,Wnd->Height-Wnd->BorderBottom-1);
	SetAPen(RP,pen.fg);
}

/*** Adjust position of the prop gadget ***/
void prop_adj(Project p)
{
	ULONG VertBody, VertPot;

	extern UBYTE record;
	if(record == 2) return;

	/* If we have more lines visible than the text actually has (ie. **
	** there are empty lines visible) the body-size represents the   **
	** lines visible according to the actual screen size:            */
	if(p->top_line + gui.nbline > p->max_lines)
		VertPot = MAXPOT,
		VertBody = ((p->max_lines - p->top_line) * MAXBODY) / p->max_lines;
	else
		if(p->max_lines <= gui.nbline)
			/* There less lines than window can show */
			VertPot  = 0,
			VertBody = MAXBODY;
		else
			VertPot = (p->top_line * MAXPOT) / (p->max_lines - gui.nbline),
			/* The body-size is (number of lines for jump-scroll / all other lines) */
			VertBody = (gui.nbline * MAXBODY) / p->max_lines;

	/* Let's set it */
	NewModifyProp((struct Gadget *)Prop,Wnd,NULL,ScrollBar.pinfo.Flags,MAXPOT,VertPot,MAXBODY,VertBody,1);
}

/*** Allocate and attach a prop gadget to the window ***/
struct Scroll *add_prop(struct Window *win)
{
	struct Image  *dummy;
	struct Scroll *pg;

	UWORD	height, size_width, size_height;

	/* If the window is a backdrop'ed one, use a simplified BOOPSI propgadget **
	** because the next propgadget aspect depends on window activated state   */
	if (win->Flags & WFLG_BACKDROP)
	{
		/* Yes this is actually a (struct Gadget *)... */
		if ((pg = (struct Scroll *)NewObject(NULL, "propgclass",
						GA_Top,         0,
						GA_Left,        win->Width - 10,
						GA_Width,       10,
						GA_Height,      win->Height,
						GA_RelVerify,   TRUE,
						GA_FollowMouse, TRUE,
						GA_Immediate,   TRUE,
						PGA_VertPot,    MAXPOT,
						PGA_VertBody,   MAXBODY,
						PGA_Freedom,    FREEVERT,
						PGA_NewLook,    TRUE,		/* Use new-look prop gadget imagery */
						TAG_END)))
		{
			/* To simplify adj_prop() */
			ScrollBar.pinfo.Flags = ((struct PropInfo *)pg->scroller.SpecialInfo)->Flags;
			/* And finally, add it to the window */
			AddGList(win, (struct Gadget *)pg, 0, 1, NULL);
			RefreshGList((struct Gadget *)pg, win, NULL, 1);
		}

		boopsigad = TRUE;
		return pg;
	}
	boopsigad = FALSE;

	/* Get memory */
	if( ( pg = (void *) AllocMem(sizeof(*pg), MEMF_PUBLIC) ) )
	{
		/* Copy default flags/modes/etc. */
		ScrollBar.pinfo.Flags = PROP_FLAGS;
		CopyMem(&ScrollBar, pg, sizeof(*pg));

		di = (void *) GetScreenDrawInfo(win->WScreen);

		/* We need to get size-gadget height, to adjust properly arrows */
		if((dummy = (struct Image *) NewObject(NULL, "sysiclass",
								SYSIA_Which, SIZEIMAGE,
								SYSIA_DrawInfo, (ULONG)di,
								TAG_END) ))
		{
			size_width	= dummy->Width;		/* width of up/down-gadgets */
			size_height = dummy->Height;		/* bottom offset */

			/* We don't need the image anymore */
			DisposeObject(dummy);

			/* Get the boopsi image of the up and down arrow */
			if((pg->upimage = (struct Image *) NewObject(NULL, "sysiclass",
								SYSIA_Which, UPIMAGE,
								SYSIA_DrawInfo, (ULONG)di,
								TAG_END) ))
			{
				pg->up.GadgetRender = pg->up.SelectRender = (APTR)pg->upimage;
				height = pg->upimage->Height;

				if((pg->downimage = (struct Image *) NewObject(NULL, "sysiclass",
								SYSIA_Which, DOWNIMAGE,
								SYSIA_DrawInfo, (ULONG)di,
								TAG_END) ))
				{
					struct Gadget *G = (void *)pg;
					WORD voffset = size_width / 4;
					
					pg->down.GadgetRender = pg->down.SelectRender = (APTR)pg->downimage;

					/* Release drawinfo */
					FreeScreenDrawInfo (win->WScreen, di);

					/* Now init all sizes/positions relative to window's borders */
					G->Height		= -(win->BorderTop + size_height + 2*height + 2);
					G->TopEdge		= win->BorderTop + 1;
					G->Width			= size_width - voffset * 2;
					G->LeftEdge		= -(size_width - voffset - 1); G++;
					pg->up.LeftEdge=
					G->LeftEdge		= -(size_width - 1);
					G->Width			= pg->up.Width  = size_width;
					G->Height		= pg->up.Height = height;
					G->TopEdge		= -(size_height + height - 1);
					pg->up.TopEdge	= G->TopEdge - height;
    
					/* Other fields */
					pg->scroller.GadgetRender = (APTR)&pg->simage;
					pg->scroller.SpecialInfo  = (APTR)&pg->pinfo;

					/* Link gadgets */
					pg->scroller.NextGadget	= &pg->up;
					pg->up.NextGadget			= &pg->down;

					/* And finally, add them to the window */
					AddGList(win, &pg->scroller, 0, 3, NULL);
					RefreshGList(&pg->scroller, win, NULL, 3);

					return pg;
				}
				DisposeObject(pg->upimage);
			}
		}
		FreeMem(pg, sizeof(*pg));
		FreeScreenDrawInfo(win->WScreen, di);
	}
	return NULL;
}

/*** Free ressources allocated for scroller ***/
void free_prop(struct Scroll *pg)
{
	if( pg )
	{
		if( boopsigad ) DisposeObject( pg );
		else {
			/* Free elements */
			DisposeObject(pg->upimage);
			DisposeObject(pg->downimage);

			/* Free struct */
			FreeMem(pg, sizeof(*pg));
		}
	}
}

/*** Open a screen according to a ModeID ***/
struct Screen *SetupScreen( ULONG ModeID )
{
	return (struct Screen *) OpenScreenTags( NULL, SA_Left, 0,
					SA_Top,        0,
					SA_Depth,      prefs.depth,
					SA_Font,       (ULONG)&prefs.attrscr,
					SA_Type,       CUSTOMSCREEN,
					SA_DisplayID,  ModeID,
					SA_Overscan,   OSCAN_TEXT,
					SA_AutoScroll, TRUE,
					SA_Pens,       (ULONG)&DriPens,
					SA_Title,      (ULONG)ScrTitle,
					TAG_DONE );
}

/*** Try to copy a screen ***/
struct Screen *clone_screen(void)
{
	/* Required information is filled when pref. are loaded */
	return (struct Screen *) OpenScreenTags( NULL, SA_Left, 0,
					SA_Top,        0,
					SA_Depth,      prefs.scrd,
					SA_Width,      prefs.scrw,
					SA_Height,     prefs.scrh,
					SA_Font,       (ULONG)&prefs.attrscr,
					SA_Type,       CUSTOMSCREEN,
					SA_DisplayID,  prefs.vmd,
					SA_AutoScroll, TRUE,
					SA_Pens,       (ULONG)&DriPens,
					SA_Title,      (ULONG)ScrTitle,
					TAG_DONE );
}

/*** Be sure a window fits in a screen ***/
void fit_in_screen(struct NewWindow *wnd, struct Screen *scr)
{
	/* Adjust left edge and width of window */
	if(wnd->LeftEdge + wnd->Width > scr->Width)
		wnd->LeftEdge = scr->Width - wnd->Width;

	if(wnd->LeftEdge < 0) wnd->LeftEdge=0, wnd->Width=Scr->Width;
	
	/* Adjust top edge and height */
	if(wnd->TopEdge + wnd->Height > scr->Height)
		wnd->TopEdge = scr->Height - wnd->Height;

	if(wnd->TopEdge < 0) wnd->TopEdge=0, wnd->Height=Scr->Height;
}

/*** Force a window to recover the whole screen ***/
void maximize(struct NewWindow *wnd, struct Screen *scr)
{
	wnd->LeftEdge = 0;
	wnd->TopEdge = scr->BarHeight+1;
	wnd->Height = scr->Height - wnd->TopEdge;
	wnd->Width = scr->Width;
}

/*** Get correct pens number according to user preferences ***/
void load_pens( void )
{
	/* Find out drawing information */
	if( ( di = (void *) GetScreenDrawInfo(Scr) ) )
	{
		struct Image *dummy;
		WORD *offset = (WORD *)&prefs.pen, *dst;

		/* Get a copy of the correct pens for the screen */
		for(dst=(UWORD *)&pen; (char *)dst < (char *)&pen+sizeof(pen); offset++)
			*dst++ = (*offset < 0 ? -(*offset) - 1 : di->dri_Pens[ *offset ]);

		fginfo = (prefs.use_pub ? 0 : di->dri_Pens[FILLTEXTPEN]);
		bginfo = (prefs.use_pub ? 1 : di->dri_Pens[FILLPEN]);
		MenuTags[1] = di->dri_Pens[BARDETAILPEN];

		/* This one is only available on system V39+ */
		if(di->dri_Version>=2 && prefs.backdrop && prefs.use_pub != 0)
			fginfo = MenuTags[1], bginfo = di->dri_Pens[BARBLOCKPEN];

		/* Get screen depth-arrange image width */
		if((dummy = (struct Image *) NewObject(NULL, "sysiclass",
							SYSIA_Which, SDEPTHIMAGE,
							SYSIA_DrawInfo, (ULONG)di,
							TAG_END) ) )
			gui.depthwid = dummy->Width,DisposeObject(dummy);
		else
			gui.depthwid = 15;

		FreeScreenDrawInfo(Scr,di);
	}
}

/*** Init main window returning 0, if all is OK ***/
long setup( void )
{
	/* Setup pos/dim */
	
	if (prefs.width && prefs.height)
	{
	    CopyMem(&prefs.left,&Template.LeftEdge,4*sizeof(WORD));
	}

	/* Setup screen */
	if(Scr == NULL)
		switch( prefs.use_pub )
		{
			case 0: Scr = prefs.parent; break;
			case 1: if((Scr = SetupScreen(prefs.modeid))) break;
			case 2: Scr = clone_screen(); break;
		}
	/* Something goes wrong? */
	if(Scr == NULL) return 1;
	load_pens();

	/* Get the screen's visual information data */
	if(Vi || (Vi = (void *) GetVisualInfoA(Scr,NULL)))
	{
		/* Build the menu-strip and compute menu items size */
		if(Menu || (Menu = (void *) CreateMenusA(newmenu, (struct TagItem *)MenuTags)))
		{
			if( LayoutMenus(Menu, Vi, GTMN_TextAttr, (ULONG)&prefs.attrscr, TAG_DONE) )
			{
				Template.Screen = Scr;
				/* Force to do not use a backdroped window on a pubscreen */
				if( prefs.backdrop && prefs.use_pub != 0 )
					maximize(&Template, Scr), Template.Flags = BACKDROP_FLAGS,
					Template.Title = NULL;
				else
					fit_in_screen(&Template, Scr), Template.Flags = NORMAL_FLAGS,
					Template.Title = WinTitle;

				/* Open our main window */
				if(Wnd || (Wnd = (void *) OpenWindow( &Template )))
				{
					/* Init temporary rastport, for font measurement */
					CopyMem(RP = Wnd->RPort, &RPT, sizeof(RPT));
					SetFont(&RPT, prefs.scrfont);
					SetMenuStrip( Wnd, Menu );
					recalc_sigbits();

					/* Try to attach a prop gadget on right side of the window */
					if(Prop || (Prop = add_prop(Wnd)))
					{
						/* Compute some often-used values */
						adjust_win(Wnd,NbProject>1);
						SetABPenDrMd(RP, pen.fg, pen.bg, JAM2);
						SetFont(RP, prefs.txtfont);
						return 0;
					}	else return 2;
				}
				/* Window fails to open */
				return 3;
			}
			/* Error in menu layout */
			return 4;
		}
		/* Error in menu allocation */
		return 5;
	}
	/* Can't get VisualInfo */
	return 6;
}

/*** Cleanup properly the ressources allocated for GUIs ***/
void CloseMainWnd(BOOL CloseScr)
{
	send_pref(&prefs, CMD_KILL);
	close_searchwnd(FALSE);
	if(Wnd)
	{
		if( Menu ) ClearMenuStrip(Wnd);

		CloseWindow(Wnd); Wnd=NULL;
		if( Prop ) free_prop(Prop); Prop=NULL;
	}
	if(Menu) FreeMenus(Menu), Menu=NULL;
	if(Vi)   FreeVisualInfo(Vi), Vi=NULL;
	/* If a screen has been opened, close it */
	if(CloseScr)
	{
		if(prefs.use_pub && Scr)
			CloseScreen(Scr);
		Scr=NULL;
	}
}
