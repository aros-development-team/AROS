/**********************************************************
** jed.c : An simple, fast and efficient text editor     **
**         Written by T.Pierron and C.Guillaume.         **
**         Started on august 1998.                       **
**-------------------------------------------------------**
** Special requirements: WorkBench 2.0, v36 or above     **
**********************************************************/

#include <intuition/intuition.h>				/* Std types */
#include <libraries/gadtools.h>				/* Menu events */
#include <dos/dos.h>								/* Standard error codes */
#include <exec/memory.h>						/* Memory allocation */
#include "Version.h"
#include "Jed.h"
#include "DiskIO.h"
#include "Events.h"
#include "Utility.h"
#include "Macros.h"
#include "Search.h"
#include "ProtoTypes.h"

#define	DEBUG_STUFF				/* Only activated if DEBUG macro is defined */
#include "Debug.h"

#define  CATCOMP_NUMBERS		/* We will need the string id */
#include "strings.h"

/* Linked list of opened project */
Project  edit = NULL;
ULONG    sigbits=0, sigrcvd, sigport, err_time;
UBYTE    clear=TRUE, BufTxt[256];
pfnSelectFunc move_selection;

/* Static string for version command */
const char Version[]=SVER;

/* Shared libraires we'll need to open */
struct IntuitionBase *IntuitionBase = NULL;
struct GfxBase       *GfxBase       = NULL;
struct Library       *KeymapBase    = NULL;
struct Library       *GadToolsBase  = NULL;
struct Library       *AslBase       = NULL;
struct LocaleBase    *LocaleBase    = NULL;
struct Library       *DiskfontBase  = NULL;
struct UtilityBase   *UtilityBase   = NULL;
struct Library       *IFFParseBase  = NULL;

struct IntuiMessage *msg, msgbuf;     /* Used to collect events */

StartUpArgs args;

#if	DEBUG
ULONG bmem, amem;
#endif

/*** MAIN LOOP ***/
int main(int argc, char *argv[])
{
#if	DEBUG
	bmem = AvailMem( MEMF_PUBLIC );
#endif

	ParseArgs(&args, argc, argv);

	/* Look first if Jano isn't already running */
	if( find_janoed( &args ) ) cleanup(0, RETURN_OK);

	/* Some global initialization */
	init_searchtable();

	/* Optionnal libraries */
	AslBase      = (struct Library *)    OpenLibrary("asl.library",     36);
	LocaleBase   = (struct LocaleBase *) OpenLibrary("locale.library",  38);
	DiskfontBase = (struct Library *)    OpenLibrary("diskfont.library", 0);
	IFFParseBase = (struct Library *)    OpenLibrary("iffparse.library",36);

	if(LocaleBase) InitLocale();    /* Localize the prog */

	/* Open the required ROM libraries */
	if( (IntuitionBase = (struct IntuitionBase *) OpenLibrary("intuition.library", 36)) &&
	    (GfxBase       = (struct GfxBase *)       OpenLibrary("graphics.library",  36)) &&
	    (GadToolsBase  = (struct Library *)       OpenLibrary("gadtools.library",  36)) &&
	    (KeymapBase    = (struct Library *)       OpenLibrary("keymap.library",    36)) &&
		 (UtilityBase   = (struct UtilityBase *)   OpenLibrary("utility.library",   36)) )
	{
		init_macros();
		set_default_prefs(&prefs, IntuitionBase->ActiveScreen);
		
		load_prefs(&prefs, NULL);     /* See if it exists a config file */
		sigport = create_port();

		/* Create whether an empty project or an existing one */
		if( ( edit = create_projects(NULL, args.sa_ArgLst, args.sa_NbArgs) ) )
		{
			/* Open the main interface */
			if(setup() == 0)
			{
				/* Makes edit project visible */
				reshape_panel(edit);
				active_project(edit,TRUE);
				clear_brcorner();

				dispatch_events();

			} else cleanup(ErrMsg(ERR_NOGUI), RETURN_FAIL);
		}
	} else cleanup(ErrMsg(ERR_BADOS), RETURN_FAIL);

	/* Hope that all were well... */
	cleanup(0, RETURN_OK);
	
	return 0;
}

/*** Deallocate ressources properly ***/
void cleanup(UBYTE *msg, int errcode)
{
	CBClose();
	close_port();
	CloseMainWnd(1);
	CleanupLocale();
	free_macros();
	free_diskio_alloc();		/* ASL */
	if(args.sa_Free)  FreeVec(args.sa_ArgLst);
	if(DiskfontBase)  CloseLibrary(DiskfontBase);
	if(LocaleBase)    CloseLibrary((struct Library *)LocaleBase);
	if(AslBase)       CloseLibrary(AslBase);
   if(IFFParseBase)  CloseLibrary(IFFParseBase);
	if(UtilityBase)   CloseLibrary((struct Library *)UtilityBase);
	if(KeymapBase)    CloseLibrary(KeymapBase);
	if(GadToolsBase)  CloseLibrary(GadToolsBase);
	if(GfxBase)       CloseLibrary((struct Library *)GfxBase);
	if(IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
	if(msg)           puts(msg);

#if	DEBUG
	/* Here can be compared good programs with the others :-) */
	amem = AvailMem( MEMF_PUBLIC );
	if(amem < bmem)  printf("Possible memory lost of %d bytes\n", bmem-amem);
#endif
	exit(errcode);
}

/*** Handle events coming from main window: ***/
void dispatch_events()
{
	extern ULONG sigmainwnd, swinsig;
	extern UBYTE record;
	BYTE  scrolldisp=0, state=0, cnt=0, mark=0, quit = 0;

	while( quit == 0 )
	{
		/* Active collect, when pressing arrow gadgets */
		sigrcvd = (state==0 ? Wait(sigbits) : sigmainwnd);

/*		if(sigrcvd & SIGBREAKF_CTRL_C) break;

		else */ if(sigrcvd & sigport) { handle_port(); continue; }

		else if(sigrcvd & swinsig) { handle_search(); continue; }

		/* Collect messages posted to the window port */
		while( ( msg = (struct IntuiMessage *) GetMsg(Wnd->UserPort) ) )
		{
			/* Copy the entire message into the buffer */
			CopyMemQuick(msg, &msgbuf, sizeof(msgbuf));
			ReplyMsg( (struct Message *) msg );

			switch( msgbuf.Class )
			{
				case IDCMP_CLOSEWINDOW: handle_menu(112); break;
				case IDCMP_RAWKEY:
					handle_kbd(edit);
					if(record) {
						if(record == 1) reg_act_com(MAC_ACT_SHORTCUT, msgbuf.Code, msgbuf.Qualifier);
						else record &= 0x7f;
					}
					break;
				case IDCMP_INTUITICKS:
					/* An error message which needs to be removed? */
					if(err_time == 0) err_time = msgbuf.Seconds;
					if(err_time && msgbuf.Seconds-err_time>4) StopError(Wnd);
					break;
				case IDCMP_MOUSEBUTTONS:
					/* Click somewhere in the text */
					switch( msgbuf.Code )
					{
						case SELECTDOWN:
							/* Click over the project bar ? */
							if(msgbuf.MouseY < gui.top)
							{
								edit = select_panel(edit, msgbuf.MouseX);
								break;
							}

							click(edit, msgbuf.MouseX, msgbuf.MouseY, FALSE);

							/* Shift-click to use columnar selection */
							if( ( move_selection = SwitchSelect(edit, msgbuf.Qualifier & SHIFTKEYS ? 1:0, 1) ) )
								mark=TRUE;
							break;
						case SELECTUP:
							if(mark) unclick(edit);
							mark=FALSE; scrolldisp=0; break;
					}
					break;
				case IDCMP_NEWSIZE:
					new_size(EDIT_ALL);
					break;
				case IDCMP_GADGETDOWN:       /* Left scroll bar */
					if(msgbuf.IAddress == (APTR) &Prop->down) state=1;
					if(msgbuf.IAddress == (APTR) &Prop->up)   state=2;
					break;
				case IDCMP_GADGETUP:        /* Arrows or prop gadget */
					state=0;
					if(msgbuf.IAddress == (APTR) Prop)
						scroll_disp(edit, FALSE), scrolldisp=0;
					break;
				case IDCMP_MOUSEMOVE:
					if(mark) scrolldisp=2;
					else
						if(Prop->scroller.Flags & GFLG_SELECTED) scrolldisp=1;
					break;
				case IDCMP_MENUPICK:
				{	struct MenuItem * Item;
					ULONG             MenuId;

					/* Multi-selection of menu entries */
					while(msgbuf.Code != MENUNULL)
						if( (Item = ItemAddress( Menu, msgbuf.Code )) )
						{
							MenuId = (ULONG)GTMENUITEM_USERDATA( Item );
							handle_menu( MenuId );

							if(record) reg_act_com(MAC_ACT_COM_MENU, MenuId, msgbuf.Qualifier);
							else record &= 0x7f;

							msgbuf.Code = Item->NextSelect;
						}
				}
			}
		}
		/* Reduces the number of IDCMP mousemove messages to process */
		if(scrolldisp==1) scroll_disp(edit, FALSE), scrolldisp=0;
		if(scrolldisp==2) { scrolldisp=0; goto moveit; }

		/* User may want to auto-scroll the display using arrow gadgets */
		if(state && (mark || (((struct Gadget *)Prop)[state].Flags & GFLG_SELECTED))) {
			/* Slow down animation: */
			WaitTOF(); cnt++;
			if(cnt>1) {
				cnt=0;
				if(autoscroll(edit,state==1 ? 1:-1)==0) state=0;
				else if(mark) {
					LONG x , y; moveit:
					/* Adjust mouse position */
					x = (msgbuf.MouseX-gui.left) / XSIZE;
					y = (msgbuf.MouseY-gui.top) / YSIZE;
					if(x < 0) x =  0; if(x >= gui.nbcol)  x = gui.nbcol-1;
					if(y < 0) y = -1; if(y >  gui.nbline) y = gui.nbline;
					edit->nbrwc = (x += edit->left_pos);
					y += (LONG)edit->top_line;
					if( x != edit->ccp.xc || y != edit->ccp.yc )
						/* Move the selected stream */
						if( !(state = move_selection(edit,x,y)) )
							set_cursor_line(edit, y, edit->top_line),
							inv_curs(edit,TRUE);
				}
			}
		}	/* endif: arrow gadget pressed or autoscroll */
	}
}

/*** Refresh display, according to new window size ***/
void new_size(UBYTE Flags)
{
	inv_curs(edit, FALSE);
	adjust_win(Wnd,NbProject>1);   /* Adjust internal variables */
	SetABPenDrMd(RP, pen.fg, pen.bg, JAM2);
	clear_brcorner();
	prop_adj(edit);
	edit->left_pos = curs_visible(edit, edit->top_line);
	edit->xcurs    = (edit->nbrc - edit->left_pos)*XSIZE + gui.left;
	if(Flags & EDIT_GUI)  reshape_panel(edit);
	if(Flags & EDIT_AREA) redraw_content(edit,edit->show,gui.topcurs,gui.nbline);
	inv_curs(edit,TRUE);
}

/*** Scroll display according to right prop gadget ***/
void scroll_disp(Project p, BOOL adjust)
{
	ULONG pos = ((struct PropInfo *)((struct Gadget*)Prop)->SpecialInfo)->VertPot *
	            (p->max_lines - gui.nbline) / MAXPOT;

	if(p->max_lines>gui.nbline && pos!=p->top_line)
	{
		if(p->ccp.select)
			/* If selection mode is on, don't move cursor */
			p->ycurs-=(pos-p->top_line)*YSIZE,
			scroll_xy(p, p->left_pos, pos, adjust);
		else
			/* Be sure cursor is always in the edit area */
			inv_curs(p,FALSE),
			scroll_xy(p, curs_visible(p,pos), pos, adjust),
			inv_curs(p,TRUE);
	}
}

/*** Be sure that cursor is always visible ***/
LONG curs_visible(Project p, LONG newtop)
{
	if(p->nbl < newtop) {
		/* The cursor is above the top line */
		p->ycurs = gui.topcurs;
		p->nbl   = newtop;
		goto adj_edited;
	} else if(p->nbl >= newtop+gui.nbline) {
		register LONG nb;
		register LINE *ln;

		/* The cursor is below the bottom line */
		p->nbl   = newtop+gui.nbline-1;
		p->ycurs = gui.botcurs;

		adj_edited:
		for(ln=p->the_line, nb=p->nbl; nb--; ln=ln->next);
		if(ln) p->edited=ln;
		draw_info(p);

	} else
		/* Is between the top and bottom line */
		p->ycurs = (p->nbl-newtop) * YSIZE+ gui.topcurs;

	/* Jump too far to the right? */
	newtop=p->left_pos + gui.nbcol - 1;
	if(p->nbrc>newtop) p->nbrwc=newtop;
	/* Too far on the left */
	if((newtop = p->nbrc<p->left_pos)) p->nbrwc=p->left_pos;

	/* Adjust cursor pos */
	newtop = adjust_rc(p->edited, p->nbrwc, &p->nbc, newtop);
	if(p->nbrc != newtop)
		p->nbrc  = newtop, draw_info(p);

	p->xcurs = (p->nbrc-p->left_pos)*XSIZE + gui.left;
	return center_horiz(p);
}

/*** Center the display horizontally to show the cursor ***/
LONG center_horiz(Project p)
{
	/* Return the new left position */
	if(p->nbrc < p->left_pos)
		if(p->nbrc > gui.xstep) return (LONG)(p->nbrc - gui.xstep);
		else return 0;
	else
		if(p->nbrc >= p->left_pos+gui.nbcol)
			return (LONG)(p->nbrc + gui.xstep - gui.nbcol + 1);
		else return (LONG)p->left_pos;
}

/*** Center the display vertically to show the cursor ***/
LONG center_vert(Project p)
{
	/* Return the new top position */
	if(p->nbl < p->top_line || p->nbl >= p->top_line+gui.nbline)
	{
		LONG newtop=p->nbl - (gui.nbline>>1) + 4;
		return newtop<0 ? 0:newtop;
	}
	return (LONG)p->top_line;
}

/*** Low-level text rendering at current rastport position ***/
void write_text(Project p, LINE *ln)
{
	static   UBYTE ts;
	register UBYTE *str,*buf;
	register LONG nb, nbc, size = ln->size;
	static   LONG stsel, ensel;

	/* Look if line is partially selected */
	if(ln->flags)
		stsel = (ln->flags & FIRSTSEL ? p->ccp.startsel:0),
		ensel = (ln->flags & LASTSEL  ? p->ccp.endsel:0x7FFFFFFF);
	else stsel=ensel=0x7FFFFFFF;

	/* Find the first char to display */
	for(str=ln->stream, nb=p->left_pos, nbc=0; nbc<nb; str++, size--)
		if(*str=='\t') nbc+=(ts=tabstop(nbc));
		else nbc++;

	nbc-=nb; stsel-=nb; ensel-=nb;
	if(nbc>=0 && size>=0)
	{
		/* Tricky case: line begins with an overlapping tabstop */
		if(nbc>0) {
			memset(BufTxt,' ',nbc);
			if(stsel<nbc) stsel=(stsel<=nbc-ts ? 0:nbc);
			if(ensel<nbc) ensel=nbc;
		}

		/* Copy the string to a temp buffer */
		for(nb=size, buf=BufTxt+nbc; nbc<gui.nbcol && nb; str++, nb--)
			if(*str=='\t') {
				register UBYTE ts = tabstop(nbc+p->left_pos);
				memset(buf,' ',ts);
				nbc+=ts; buf+=ts;
				if(stsel<nbc && stsel>nbc-ts) stsel=nbc;
				if(ensel<nbc && ensel>nbc-ts) ensel=nbc;
			} else *buf++ = *str, nbc++;

		/* Overlapping tabulation ? */
		if(nbc>=gui.nbcol) nbc=gui.nbcol;
		else *buf=' ',nbc++;

		/* Optimize rendering of unselected line */
		if(ensel==stsel || stsel>nbc || ensel<0) Text(RP,BufTxt,nbc);
		else {
			buf=BufTxt; nb=stsel;
			if(nb > 0) Text(RP,BufTxt,nb); else nb=0;
			if(ensel>nbc) ensel=nbc;
			SetABPenDrMd(RP,pen.fgfill,pen.bgfill,JAM2);
			Text(RP,buf+nb,ensel-nb);
			SetABPenDrMd(RP,pen.fg,pen.bg,JAM2);
			if(ensel!=nbc) Text(RP,buf+ensel,nbc-ensel);
		}
	}
	if(clear && gui.right>RP->cp_x) {
		/* Clear the end of line, like ClearEOL(rp), but without overlapping borders :-( */
		SetAPen(RP,pen.bg); nb=RP->cp_y-BASEL;
		RectFill(RP,RP->cp_x,nb,gui.right,nb+YSIZE-1);
		SetAPen(RP,pen.fg);
	}
}

/*** Delta horizontal scroll, with boundary check ***/
void scroll_xdelta(Project p, LONG x)
{
	if(x<0)
	{
		x=-x;
		if(x>p->left_pos) x=0; else x=p->left_pos-x;
	}	else x+=p->left_pos;
	/* Refresh only if different pos */
	if(x!=p->left_pos)
	{
		if(!p->ccp.select) inv_curs(p,FALSE);
		scroll_xy(p, x, p->top_line, FALSE);
		/* Is cursor always visible? */
		x=p->xcurs;
		curs_visible(p, p->top_line);
		if(x!=p->xcurs && p->ccp.select)
			move_selection(p, p->nbrwc, p->nbl);

		SetAPen(RP,pen.fg); inv_curs(p,TRUE);
	}
}

/*** Delta vertical scroll, with boundary check ***/
void scroll_ydelta(Project p, LONG y)
{
	LONG pos=p->top_line+y;
	/* Clamp values to the boundary */

	if(pos<0) pos=0;
#if 0
	if(pos>(LONG)p->max_lines-(LONG)gui.nbline) pos=p->max_lines-gui.nbline;
#else
	if(pos>p->max_lines-1) pos=p->max_lines-1;
#endif

	if(pos!=p->top_line)
	{
		if(!p->ccp.select) inv_curs(p,FALSE);
		scroll_xy(p, curs_visible(p,pos), pos, TRUE);
		if(p->ccp.select) move_selection(p, p->nbrwc, p->nbl);
		inv_curs(p,TRUE);
	}
}

/*** Like previous, but with required simplifications ***/
BOOL autoscroll(Project p, WORD y)
{
	LONG pos=p->top_line+y;
	/* Clamp values to the boundary */
#if 1
	if(pos>(LONG)p->max_lines-(LONG)gui.nbline) pos=p->max_lines-gui.nbline;
#else
	if(pos>p->max_lines-1) pos=p->max_lines-1;
#endif
	if(pos<0) pos=0;

	if(pos!=p->top_line)
	{
		if(!p->ccp.select) inv_curs(p,FALSE);
		scroll_xy(p, p->left_pos, pos, TRUE);
		if(!p->ccp.select)
			curs_visible(p, p->top_line),
			inv_curs(p,TRUE);

		return TRUE;
	}	else return FALSE;
}

/*** Redraw part of a display ***/
void redraw_content(Project p, LINE *ln, WORD startpos, WORD nb)
{
	for(SetAPen(RP,pen.fg); nb-- && ln; startpos += YSIZE,ln = ln->next)
		Move(RP, gui.left, startpos),
		write_text(p, ln);

	/* Empty lines visible? */
	if(nb >= 0)
		SetAPen(RP,pen.bg),
		RectFill(RP,gui.left, startpos-BASEL, gui.right, gui.bottom),
		SetAPen(RP,pen.fg);
}

/*** Scroll to a determinate absolute position ***/
void scroll_xy(Project p, LONG xp, LONG yp, BYTE adj)
{
	LONG skipy = yp-p->top_line,
	     skipx = xp-p->left_pos, svg;

	if(skipy<0) skipy = -skipy;
	if(skipx<0) skipx = -skipx;
	/* Can the process be optimized? */
	if(skipy < gui.nbline && skipx < gui.nbcol)
	{
		/* Yes, don't redraw whole display */
		WORD ystart,xstart; LINE *disp = p->show;

		xstart = skipx * XSIZE;
		ystart = skipy * YSIZE;

		/* 1. Scroll the display */
		ScrollRaster(RP, xp<p->left_pos? -xstart:xstart, yp<p->top_line? -ystart:ystart,
		             gui.left, gui.top, gui.rcurs-1, gui.bottom);

		/* Only performs changes if required */
		if(skipy)
		{
			/* 2. Update internal variables if vertical scroll */
			if(yp < p->top_line)
			{
				/* Scroll display down */
				register LONG nb=skipy;
				for(; nb--; disp=disp->prev);
				p->top_line -= skipy;
				ystart       = gui.topcurs;
				p->show      = disp;
			} else {
				/* Scroll display up */
				register LONG nb=skipy;
				p->top_line += skipy;
				ystart       = gui.botcurs - ystart + YSIZE;
				for(; nb--; disp=disp->next);
				p->show      = disp;

				for(nb=gui.nbline - skipy; disp && nb--; disp=disp->next);
			}
		}

		/* Same fight */
		if(skipx)
		{
			/* 3. Update variables if horizontal scroll */
			svg = gui.nbcol;
			gui.nbcol = skipx;

			if(xp < p->left_pos)
			{
				/* Scroll display right */
				p->xcurs += xstart;
				clear = FALSE;
				p->left_pos = xp;
				redraw_content(p, p->show, gui.topcurs, gui.nbline);
				clear = TRUE;
			} else {
				/* Scroll display left */
				register LONG left=gui.left;
				p->xcurs -= xstart;
				gui.left = gui.rcurs-xstart;
				p->left_pos += svg;
				redraw_content(p, p->show, gui.topcurs, gui.nbline);
				gui.left = left;
				p->left_pos = xp;
			}
			gui.nbcol = svg;
		}
		/* 4. Redraw display (needs previous changes before) */
		if(skipy) redraw_content(p, disp, ystart, skipy);

	} else {
		/* We've jump too far, whole redraw */
		register LONG nb = yp-p->top_line;
		register LINE *disp = p->show;
		p->top_line += nb;
		if(nb < 0) for(; nb++; disp=disp->prev);
		else       for(; nb--; disp=disp->next);
		p->show = disp;
		p->left_pos = xp;
		p->xcurs  = (p->nbrc-p->left_pos) * XSIZE + gui.left;
		redraw_content(p, disp, gui.topcurs, gui.nbline);
	}
	/* Adjust position of the prop gadget */
	if(adj) prop_adj(p);
}

/*** Scroll lines up, at a specified position (used for deleting) ***/
void scroll_up(Project p, LINE *ln, WORD ystart, LONG leftpos)
{
	register WORD nbl;
	/* If some of redrawn lines are still visible */
	if( leftpos == p->left_pos )
		/* Optimize lines to refresh */
		nbl = p->top_line + gui.nbline - p->nbl - 1,
		ystart += YSIZE;
	else
		/* Redraw whole content */
		nbl = gui.nbline, ystart = gui.topcurs, ln=p->show,
		p->left_pos = leftpos;

	redraw_content(p, ln, ystart, nbl);
	/* Adjust cursor position */
	p->xcurs = (p->nbrc-p->left_pos) * XSIZE + gui.left;
}
