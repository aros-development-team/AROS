/******************************************************
**  Edit.c : Implementation of 'Edit' menu commands  **
**  (mark/cut/[un]indent/[lower|upper|toggle] case)  **
**  © T.Pierron, C.Guillaume.                        **
******************************************************/

#include <exec/types.h>
#include <graphics/rastport.h>
#include "Jed.h"
#include "ClipLoc.h"
#include "Utility.h"
#include "Edit.h"
#include "ProtoTypes.h"

#define  CATCOMP_NUMBERS		/* Strings id for err. msg */
#include "strings.h"

/*** Copy selected text up to `Max' chars into buffer ***/
LONG copy_mark_to_buf(Project p, UBYTE *Buf, LONG Max)
{
	LINE *ln; long s,e, nbc;
	ln = (p->ccp.yc < p->ccp.yp ? p->ccp.cline : p->ccp.line);

	for(nbc=0; Max && ln && ln->flags; ln=ln->next)
	{
		s = (ln->flags & FIRSTSEL ? find_nbc(ln, p->ccp.startsel) : 0);
		e = (ln->flags & LASTSEL  ? find_nbc(ln, p->ccp.endsel)   : ln->size);
		if(s>=e) continue;
		e-=s; if(e > Max) e=Max;
		CopyMem(ln->stream+s, Buf, e);
		Max-=e; Buf+=e; nbc+=e;
	}
	*Buf=0;
	return nbc;
}

/*** Mark: stream selection ***/
BYTE move_stream_selection(Project p, LONG xn, LONG yn)
{
	LONG yline;		/* Nb. of line marked */
	LINE *ln;		/* Running pointer */
	BYTE ret;		/* Autoscroll ? */
	WORD y;			/* VPos of item sel */

	/* Vertical autoscroll ? */
	ret = (yn<p->top_line ? 2 : (yn>=(LONG)(p->top_line+gui.nbline) ? 1 : 0));

	if(yn<0) yn=0; if(yn>=p->max_lines) yn=p->max_lines-1,ret=0;
	yline=p->ccp.yc; ln=p->ccp.cline;
	/* top_line may changed */
	y=(yline-p->top_line)*YSIZE+gui.topcurs;

	/* If new selection point is situated after the prece- **
	** eding one (ie:looking afterward in linked list)     */
	if( yn>yline || (yn==yline && xn>p->ccp.xp) )
		for(; yline<yn; yline++, ln=ln->next, y+=YSIZE)
		{
			if(ln->flags && yline<=p->ccp.yp)
				if(yline == p->ccp.yp )
					p->ccp.startsel = (p->ccp.select != WORD_TYPE ? p->ccp.xp :
					                   x2pos(ln, backward_word(ln, find_nbc(ln,p->ccp.xp)))),
					ln->flags       = (p->ccp.select == LINE_TYPE ? WHOLESEL : FIRSTSEL);
				else ln->flags=0;
			else ln->flags=WHOLESEL;
			if(gui.topcurs <= y && y <= gui.botcurs)
				Move(RP, gui.left, y),write_text(p,ln);
		}
	else
		/* New selection point is before the previous */
		for(; yline>yn; yline--, ln=ln->prev, y-=YSIZE)
		{
			if(ln->flags && yline>=p->ccp.yp)
				if(yline == p->ccp.yp)
					p->ccp.endsel = (p->ccp.select != WORD_TYPE ? p->ccp.xp :
					                 x2pos(ln, forward_word(ln, find_nbc(ln,p->ccp.xp)))),
					ln->flags     = (p->ccp.select == LINE_TYPE ? WHOLESEL : LASTSEL);
				else ln->flags=0;
			else ln->flags=WHOLESEL;
			if(gui.topcurs <= y && y <= gui.botcurs)
				Move(RP, gui.left, y),write_text(p,ln);
		}

	/* Current point will be the previous */
	p->ccp.cline=ln; p->ccp.yc=yn; p->ccp.xc=xn;

	/* Update last-selected line */
	{	register UBYTE Set = 0;
		if(p->ccp.select != LINE_TYPE)
			if(yn == p->ccp.yp)
			{
				ln->flags = FIRSTSEL | LASTSEL;
				if(xn < p->ccp.xp) p->ccp.startsel=xn, p->ccp.endsel=p->ccp.xp;
				else p->ccp.startsel=p->ccp.xp, p->ccp.endsel=xn;
				Set = 3;
			} else
				if( yn > p->ccp.yp )
					ln->flags=LASTSEL, p->ccp.endsel=xn, Set = 2;
				else
					ln->flags=FIRSTSEL,p->ccp.startsel=xn, Set = 1;
		else ln->flags = WHOLESEL;

		/* Adjust selection for word selection */
		if(p->ccp.select == WORD_TYPE) {
			if(Set & 1) p->ccp.startsel = x2pos(ln, backward_word(ln, find_nbc(ln,p->ccp.startsel)));
			if(Set & 2) p->ccp.endsel   = x2pos(ln, forward_word (ln, find_nbc(ln,p->ccp.endsel)));
		}
	}
	/* Last line can overlap edit area */
	if(gui.topcurs<=y && y<=gui.botcurs)
		Move(RP, gui.left, y),write_text(p,ln);

	return ret;
}

/*** Mark: block selection ***/
BYTE move_column_selection(Project p, LONG xn, LONG yn)
{
	static WORD left, nbcol;
	static LONG left_pos;
	extern BYTE	clear;
	LONG yline;				/* Nb. of line marked */
	LINE *ln;				/* Running pointer */
	BYTE ret;				/* Autoscroll ? */
	WORD y;					/* VPos of item sel */
	BYTE rdw = 0;

	/** Vertical autoscroll ? **/
	ret = (yn<p->top_line ? 2 : (yn>=(LONG)(p->top_line+gui.nbline) ? 1 : 0));
	if(p->nbrc<p->ccp.startsel && gui.topcurs <= p->ycurs && p->ycurs <= gui.botcurs)
		p->ccp.select=0,inv_curs(p,0),p->ccp.select=COLUMN_TYPE;
	clear=FALSE;

	/** Reduce refreshed area **/
	left_pos=p->left_pos; left=gui.left; nbcol=gui.nbcol;

	gui.left += XSIZE * ( (
	p->left_pos = MIN(MIN(p->ccp.xp,p->ccp.xc), xn)) - left_pos);
	yline = MAX(MAX(p->ccp.xp,p->ccp.xc), xn);
	gui.nbcol = yline - p->left_pos + tabstop(yline);
	if(p->left_pos+gui.nbcol > left_pos+nbcol)
		gui.nbcol = left_pos+nbcol-p->left_pos;

	if(yn<0) yn=0; if(yn>=p->max_lines) yn=p->max_lines-1,ret=0;
	yline=p->ccp.yc; ln=p->ccp.cline;
	/* top_line may changed */
	y=(yline-p->top_line)*YSIZE+gui.topcurs;

	/** Update start & end selection pos **/
	if( xn < p->ccp.xp )
		p->ccp.endsel = p->ccp.xp, p->ccp.startsel = xn;
	else
		p->ccp.startsel = p->ccp.xp, p->ccp.endsel = xn;

	/** Same fight as before: afterward or backward scan **/
	if( yn >= yline )
	{
		/* Search is afterward */
		for(; yline<yn; y+=YSIZE, yline++, ln=ln->next)
		{
			if(yline < p->ccp.yp) ln->flags=0;
			else ln->flags=FIRSTSEL | LASTSEL;
			if(gui.topcurs <= y && y <= gui.botcurs)
				Move(RP,gui.left,y),write_text(p,ln);
		}
		/* If user has changed vertical position of selection an **
		** update of part of selected buffer will be required:   */
		if(xn != p->ccp.xc) {
			if(p->ccp.yp < p->ccp.yc) rdw=6;
			else if(p->ccp.yp > yn) rdw=1;
		}
	} else {
		/* Scan is backward */
		for(; yline>yn; y-=YSIZE, yline--, ln=ln->prev)
		{
			if(yline > p->ccp.yp) ln->flags=0;
			else ln->flags=FIRSTSEL | LASTSEL;
			if(gui.topcurs <= y && y <= gui.botcurs)
				Move(RP,gui.left,y),write_text(p,ln);
		}
		/* If user has changed vertical position of selection an **
		** update of part of selected buffer will be required:   */
		if(xn != p->ccp.xc) {
			if(p->ccp.yp > p->ccp.yc) rdw=5;
			else if(p->ccp.yp < yn) rdw=2;
		}
	}

	/** Current point now become the previous **/
	ln->flags = FIRSTSEL | LASTSEL;
	/* Last line can overlap edit area */
	if(gui.topcurs<=y && y<=gui.botcurs)
		Move(RP,gui.left,y),write_text(p,ln);

	/** Update unmodified lines **/
	if(rdw)
	{
		register LINE *ptr; register WORD yc;
		/* Limits number of lines to redraw */
		if(rdw&4)
			ptr=p->ccp.cline, yline=p->ccp.yc,
			yc=(yline-p->top_line)*YSIZE+gui.topcurs;
		else ptr=ln, yc=y;

		/* Reduces number of columns to redraw */
		if(p->ccp.xc < xn)
			p->left_pos=p->ccp.xc, gui.nbcol=xn-p->ccp.xc+1+tabstop(xn);
		else
			p->left_pos=xn, gui.nbcol=p->ccp.xc-xn+1+tabstop(p->ccp.xc);
		gui.left = left + XSIZE*(p->left_pos-left_pos);

		/* Be sure lines won't erase right border of window */
		if(p->left_pos+gui.nbcol > left_pos+nbcol) gui.nbcol = left_pos+nbcol-p->left_pos;

		if(rdw&1) {
			for(; yc<=gui.botcurs && yline<=p->ccp.yp; yline++, yc+=YSIZE, ptr=ptr->next)
				if(yc>=gui.topcurs) Move(RP,gui.left,yc),write_text(p,ptr);
		} else {
			for(; yc>=gui.topcurs && yline>=p->ccp.yp; yline--, yc-=YSIZE, ptr=ptr->prev)
				if(yc<=gui.botcurs) Move(RP,gui.left,yc),write_text(p,ptr);
		}
	}

	p->left_pos=left_pos; gui.left=left; gui.nbcol=nbcol;
	p->ccp.xc=xn; p->ccp.cline=ln; p->ccp.yc=yn;
	clear=TRUE;
	return ret;
}

/*** Switch between different type of selection ***/
pfnSelectFunc SwitchSelect(Project p, BYTE mode, BYTE force)
{
	pfnSelectFunc new = NULL;
	/* If click on same point, switch selection mode */
	if(p->nbrc == p->ccp.xp && p->nbl == p->ccp.yp)
	{
		new = move_stream_selection;
		switch( p->ccp.select )
		{
			case 0:
			case STREAM_TYPE: p->ccp.select=WORD_TYPE; break;
			case WORD_TYPE:   p->ccp.select=LINE_TYPE; break;
			case COLUMN_TYPE:
			case LINE_TYPE:   new=NULL; unmark_all(p,TRUE); break;
		}
		if(new) new(p,p->nbrc,p->nbl);
		inv_curs(p,TRUE);
	}
	else if(p->ccp.select)
	{
		/* We don't `click' over the same point */
		unmark_all(p,TRUE); if(force) goto new_mode;
	}
	else {
		/* Start a new default mode */
		new_mode: inv_curs(p,FALSE);
		if(mode) p->ccp.select=COLUMN_TYPE, new=move_column_selection;
		else     p->ccp.select=STREAM_TYPE, new=move_stream_selection;

		click(p,p->xcurs,p->ycurs,TRUE);
		inv_curs(p,TRUE);
	}
	return new;
}

/*** Cut: remove block of selected text ***/
void del_block(Project p)
{
	ULONG nbdel, nbrem;
	LINE *ln = p->ccp.cline;

	/* Look for first line to remove */
	switch( p->ccp.select )
	{
		case COLUMN_TYPE:
			if(p->ccp.xc > p->ccp.xp) p->nbrwc = p->ccp.xp;
			if(p->ccp.yc > p->ccp.yp) p->nbl   = p->ccp.yp, ln=p->ccp.line;
		default:
			if( p->ccp.yc > p->ccp.yp || (p->ccp.yc==p->ccp.yp && p->ccp.xc>p->ccp.xp) )
				p->nbl = p->ccp.yp, ln = p->ccp.line, p->nbrwc=p->ccp.xp;
			break;
	}

	/* Has the first line shown changed? */
	if(p->nbl < p->top_line) p->top_line=p->nbl;

	/* Delete selected lines */
	for(nbdel=nbrem=0, reg_group_by(&p->undo); ln; nbdel++)
	{
		if( ln->flags != WHOLESEL )
		{
			/* Need to convert real-column into char-index */
			ULONG rcs, rce;
			rcs = find_nbc(ln, p->ccp.startsel);
			rce = find_nbc(ln, p->ccp.endsel);
			switch( ln->flags )
			{
				case FIRSTSEL |
				     LASTSEL:  if(rcs < rce)      rem_chars(&p->undo,ln,rcs,rce-1);      ln->flags=0; break;
				case FIRSTSEL: if(rcs < ln->size) rem_chars(&p->undo,ln,rcs,ln->size-1); ln->flags=0; break;
				case LASTSEL:  if(rce > 0)        rem_chars(&p->undo,ln,0,  rce-1);      ln->flags=0; ln=ln->prev;
				               join_lines(&p->undo,ln,ln->next); nbrem++; break;
				default: goto Ze_end;
			}
			ln=ln->next;
		} else {
			/* Remove the entire line */
			register LINE *next = ln->next; ln->flags = 0;
			if( del_line(&p->undo, &p->the_line, ln) ) nbrem++, nbdel++;
			ln = next;
		}
	}

	Ze_end: /* Makes some visual cleanup */
	reg_group_by(&p->undo);

	/* Prop gadget should be adjusted? */
	p->max_lines -= nbrem;
	p->ccp.select = 0;
	p->ccp.xp     = (ULONG)-1;
	if(nbrem>=1) prop_adj(p);

	/* Adjust cursor's position */
	p->edited = p->the_line;
	nbrem     = p->nbl;
	p->nbl    = 0;
	set_cursor_line(p, nbrem, p->top_line);
	if(p->top_line == p->nbl) p->show=p->edited;

	/* What should be redrawn? */
	if(nbdel>1)
	{
		nbdel = p->nbl-p->top_line;
		redraw_content(p,p->edited,gui.topcurs+nbdel*YSIZE,gui.nbline-nbdel);
	}
	else REDRAW_CURLINE(p);
	RP->Mask = gui.txtmask;

	inv_curs(p,TRUE);
}


/*** Insert a character at the start of line ***/
void indent_by(Project p, UBYTE ch, BYTE method)
{
#	define	AffectedLine(ln, prj)	(ln->size > 0 && ((ln->flags & ~LASTSEL) || prj->ccp.endsel>0))
	if( p->ccp.select )
	{
		register LINE *ln; LONG y;
		/* Look for the first selected line */
		if(p->ccp.yc < p->ccp.yp) y=p->ccp.yc,ln=p->ccp.cline;
		else y=p->ccp.yp,ln=p->ccp.line;
		y = (y - p->top_line) * YSIZE + gui.topcurs;

		/* Every lines must start with the desired character */
		if( method == -1 )
		{
			register LINE *line = ln;
			for(; line && line->flags; line=line->next)
				if(AffectedLine(line, p) && ch != line->stream[0]) return;
		}

		/* Do it! */
		for(reg_group_by(&p->undo); ln && ln->flags; ln=ln->next,y+=YSIZE)
			if( AffectedLine(ln, p) ) {
				/* Force whole line selection */
				ln->flags = WHOLESEL;
				/* Remove or add a character */
					if(method == -1) rem_chars(&p->undo,ln,0,0);
					else             add_char(&p->undo,ln,0,ch);
				/* Redraw */
				if(gui.topcurs<=y && y<=gui.botcurs)
					Move(RP,gui.left,y),write_text(p,ln);
			}
		reg_group_by(&p->undo);
		move_selection = move_stream_selection; 
		if(p->nbc==0) goto redraw; goto refresh;
	}

	/* Should we add or remove the char? */
	if(method == -1) {
		if(p->edited->size > 0 && p->edited->stream[0]==ch) {
			rem_chars(&p->undo,p->edited,0,0); 
			if(p->nbc==0) goto redraw; goto refresh;
		} else return;
	}

	if( add_char(&p->undo, p->edited, 0, ch) )
	{
		refresh:
		inv_curs(p, FALSE);
		p->nbrwc = x2pos(p->edited, p->nbc+=method);
		p->xcurs = gui.left + XSIZE * ((
		p->nbrc  = p->nbrwc) - p->left_pos);
		redraw: REDRAW_CURLINE(p);
		/* Cursor may have quit edit area */
		if(p->nbrc<p->left_pos || p->nbrc>=p->left_pos+gui.nbcol)
			scroll_xy(p, adjust_leftpos(p, gui.xstep), p->top_line, FALSE);

		inv_curs(p,TRUE);

	}	else ThrowError(Wnd, ErrMsg(ERR_NOMEM));
}

/*** Convert a character to small or capital letter ***/
static UBYTE Method = 0;
UBYTE change_casechar(UBYTE ch)
{
	switch(Method)
	{
		case 0: ch=ToUpper(ch); break;
		case 2:
		{	UBYTE temp;
			temp=ch; ch=ToUpper(ch); if(ch!=temp) break;
		}
		case 1: ch=ToLower(ch);
	}
	return ch;
}

/*** Change one char or a whole selected region ***/
void change_case(Project p, UBYTE method)
{
	LINE *ln;
	if( p->ccp.select )
	{
		LONG y;
		/* Look for the first selected line */
		if(p->ccp.yc < p->ccp.yp) y=p->ccp.yc,ln=p->ccp.cline;
		else y=p->ccp.yp,ln=p->ccp.line;
		y = (y - p->top_line) * YSIZE + gui.topcurs;

		Method=method;
		/* Let's change selected buffer */
		for(; ln && ln->flags; ln=ln->next,y+=YSIZE)
		{
			replace_chars(ln,
				(ln->flags & FIRSTSEL ? find_nbc(ln, p->ccp.startsel) : 0),
				(ln->flags & LASTSEL  ? find_nbc(ln, p->ccp.endsel)   : ln->size),
				change_casechar
			);

			if(gui.topcurs<=y && y<=gui.botcurs)
				Move(RP,gui.left,y),write_text(p,ln);
		}
		inv_curs(p,TRUE);
	} else {

		Method=method; ln=p->edited;
		replace_char(ln,p->nbc,change_casechar(ln->stream[p->nbc]));
		curs_right(p,FALSE);
	}
}

/*** Clears all selected lines ***/
void unmark_all(Project p, BYTE wrap)
{
	LINE *ln;
	LONG  y;
	if(p->ccp.yc < p->ccp.yp) ln = p->ccp.cline, y = p->ccp.yc;
	else                      ln = p->ccp.line,  y = p->ccp.yp;

	y = (y - p->top_line) * YSIZE + gui.topcurs;
	/* Unset just lines and redraw */
	for(p->ccp.select = 0; ln && ln->flags; ln=ln->next, y+=YSIZE)
	{
		ln->flags=0;
		if(gui.topcurs<=y && y<=gui.botcurs)
			Move(RP,gui.left,y),write_text(p,ln);
	}

	/* Is the cursor always visible? */
	if( wrap ) {
		if(p->top_line < p->nbl  || p->nbl  >= p->top_line+gui.nbline ||
		   p->left_pos < p->nbrc || p->nbrc >= p->left_pos+gui.nbcol  )
			scroll_xy(p, center_horiz(p), center_vert(p), TRUE),
			set_cursor_line(p, p->nbl, p->top_line);
		inv_curs(p,TRUE);
	}
	p->ccp.xp = (ULONG)-1;
	RP->Mask  = gui.txtmask;
}

/*** Set all lines as selected ***/
void mark_all(Project p)
{
	LONG  nb, y;
	LINE *ln;

	move_selection = move_stream_selection;
	p->ccp.select  = LINE_TYPE;
	p->ccp.cline   = ln = p->the_line;
	p->ccp.yc      = 0;
	p->ccp.yp      = p->max_lines-1;
	RP->Mask       = gui.selmask;

	/* Set lines before those displayed */
	for(nb=p->top_line; nb--; ln->flags=WHOLESEL, ln=ln->next);

	/* Set and refresh displayed lines */
	{	register LINE *last = ln;
		for(y=gui.topcurs; ln && y<=gui.botcurs; last=ln, ln=ln->next, y+=YSIZE)
			ln->flags=WHOLESEL, Move(RP,gui.left,y),write_text(p,ln);

		/* Set lines after those displayed */
		for(; ln; ln->flags=WHOLESEL, last=ln, ln=ln->next);
		p->ccp.line = last;
	}
	inv_curs(p,TRUE);
}
