/**************************************************************
**** Cursor.c : cursor movement managment.                 ****
**** Free software under GNU license, started on 20.3.2000 ****
**** © T.Pierron, C.Guillaume.                             ****
**************************************************************/

#include <exec/types.h>
#include <graphics/rastport.h>
#include "Jed.h"
#include "ClipLoc.h"
#include "Utility.h"
#include "ProtoTypes.h"

#define  CATCOMP_NUMBERS		/* Strings id for err. msg */
#include "strings.h"


/*** Convert num chars to real column number: ***/
ULONG x2pos(LINE *ln, ULONG nb)
{
	register ULONG nbrc=0;
	register UBYTE *str;
	for(str=ln->stream; nb; nb--, str++)
		nbrc += (*str=='\t' ? tabstop(nbrc) : 1);

	return nbrc;
}

/*** Adjust column to an existing one: ***/
ULONG adjust_rc(LINE *ln, ULONG rc, ULONG *c, UBYTE atleast)
{
	register ULONG nbrc, nbc;
	register UBYTE *str;
	for(str=ln->stream, nbrc=nbc=0; nbc<ln->size; str++, nbc++)
	{
		register ULONG tmp = (*str=='\t' ? tabstop(nbrc) : 1);
		/* If atleast is TRUE be sure that cursor will be at least at column rc: */
		if(atleast)	if(nbrc<rc) nbrc+=tmp; else break;
		else			if(nbrc+tmp<=rc) nbrc+=tmp; else break;
	}
	*c=nbc; return nbrc;
}

/*** Returns the characer-index corresponding to the screen column ***/
ULONG find_nbc(LINE *ln, ULONG nbrc)
{
	register  LONG rc, nbc;
	register UBYTE *str;
	for(str=ln->stream, rc=0, nbc=ln->size; nbc && rc<nbrc; str++, nbc--)
		rc += (*str=='\t' ? tabstop(rc) : 1);
	return ln->size-nbc;
}

/*** Adjust leftpos according to cursor one: ***/
LONG adjust_leftpos(Project p, WORD step)
{
	LONG x;
	if(step>=0) x=p->nbrc+step-gui.nbcol+1;
	else x=p->nbrc+step;
	return (x<=0 ? 0:x);
}

/*** Search for next available word in the line ***/
LONG forward_word(LINE *ln, ULONG pos)
{
	UBYTE type;
	STRPTR str;
	/* Search next */
	for(str=ln->stream+pos,type=TypeChar[*str]; pos<ln->size && type==TypeChar[*str]; pos++,str++);
	/* Skip white spaces */
	while( pos<ln->size && TypeChar[*str]==SPACE ) str++;
	return str - ln->stream;
}

/*** Search for previous word in a line ***/
LONG backward_word(LINE *ln, ULONG pos)
{
	UBYTE type;
	STRPTR str;
	/* Skip white spaces */
	for(str=ln->stream + pos; pos>0 && TypeChar[*str]==SPACE; str--,pos--);
	/* Search previous */
	for(type=TypeChar[*str]; ; pos--,str--)
		if( type != TypeChar[*str] ) { str ++; break; }
		else if( pos == 0 )          {         break; }
	return str - ln->stream;
}

/*** Display cursor of project p ***/
void inv_curs(Project p, BYTE state)
{
	if(p->ccp.select)
	{
		/* Just draw a simple vert bart for selection */
		WORD y = p->ycurs - BASEL;
		RectFill(RP,p->xcurs, y, p->xcurs+1, y+YSIZE-1);
	}	else {
		UBYTE thechar, oldmask = RP->Mask;
		/* Fully highlight character */
		RP->Mask = gui.selmask;
		if(state) SetABPenDrMd(RP,pen.fgfill,pen.bgfill,JAM2);

		if(state != 0 && p->cursmode) {
			/* Draw the replacement cursor */
			SetAPen(RP, pen.bgfill);
			RectFill(RP,p->xcurs, p->ycurs, p->xcurs+XSIZE-1, p->ycurs-BASEL+YSIZE-1);
		} else {
			/* Normal cursor */
			thechar = (p->nbc < p->edited->size ? p->edited->stream[p->nbc] : ' ');
			if(thechar == '\t') thechar = ' ';
			Move(RP,p->xcurs,p->ycurs); Text(RP,&thechar,1);
		}
		SetABPenDrMd(RP,pen.fg,pen.bg,JAM2);
		RP->Mask = oldmask;
	}
}

/*** Set cursor to a specified position in file ***/
void set_cursor_line(Project p, LONG pos, ULONG top)
{
	LINE *ln;
	/* Adjust all related variables */
	if(pos < 0) pos = 0;
	if(pos >= p->max_lines) pos = p->max_lines-1;
	if(pos < p->nbl) for(ln=p->edited; p->nbl!=pos; ln=ln->prev, p->nbl--);
	else             for(ln=p->edited; p->nbl!=pos; ln=ln->next, p->nbl++);

	/* Cursor position */
	p->nbrc   = adjust_rc(p->edited=ln, p->nbrwc, &p->nbc, FALSE);
	p->xcurs  = (p->nbrc-p->left_pos)*XSIZE + gui.left;
	p->ycurs  = (p->nbl-top)*YSIZE + gui.topcurs;
	draw_info( p );
}

/*** Set top-left visible corner ***/
void set_top_line(Project p, ULONG top, ULONG left)
{
	LINE *ln; LONG nb = top-p->top_line;
	if(nb < 0) for(ln=p->show; nb++; ln=ln->prev);
	else       for(ln=p->show; nb--; ln=ln->next);
	p->show     = ln;
	p->top_line = top;
	p->left_pos = left;
	p->xcurs    = (p->nbrc-p->left_pos) * XSIZE + gui.left;
	redraw_content(p, ln, gui.topcurs, gui.nbline);
	prop_adj( p );
}

/*** Move cursor incrementally (paste) ***/
void move_cursor(Project p, LONG dx, LONG dy)
{
	LINE *ln; LONG nb=dy;
	inv_curs(p, FALSE);
	if(nb < 0) for(ln=p->edited; nb++; ln=ln->prev);
	else       for(ln=p->edited; nb--; ln=ln->next);
	p->edited = ln;
	p->nbl   += dy;
	p->nbrwc  = p->nbrc = x2pos(ln,p->nbc = dx);

	dx = center_horiz(p);
	/* Don't scroll if top_line changed! (whole redraw instead) */
	if(dx != p->left_pos && dy == 0)
		scroll_xy(p, dx, p->top_line, TRUE);
	else {
		nb = center_vert(p) - p->top_line;
		for(p->top_line += nb,ln=p->show; nb--; ln=ln->next);
		p->show = ln; p->left_pos = dx;
	}
	p->ycurs = (p->nbl-p->top_line)*YSIZE + gui.topcurs;
	p->xcurs = (p->nbrc-p->left_pos)*XSIZE + gui.left;
	draw_info( p );
}

/*** Move cursor to absolute position ***/
void move_to_line(Project p, ULONG nbline, char dirtiness)
{
	ULONG newtop;

	if(!p->ccp.select) inv_curs(p,FALSE);
	/* Get the new top line */
	{	register ULONG old_nbl = p->nbl;
		p->nbl = nbline;
		newtop = center_vert(p);
		p->nbl = old_nbl;
	}	set_cursor_line(p, nbline, newtop);

	if(dirtiness == LINE_AS_IS)
		scroll_xy(p, center_horiz(p), newtop, TRUE);
	else
	{
		/* Some lines are modified: redraw in one pass */
		if( newtop == p->top_line ) {
			REDRAW_CURLINE(p);
			if( dirtiness == LINES_DIRTY )
				scroll_up(p, p->edited->next, p->ycurs, center_horiz(p)),
				prop_adj(p);
			else if( p->left_pos != (newtop = center_horiz(p)) )
				scroll_xy(p, newtop, p->top_line, FALSE);
		}
		else if( dirtiness == LINE_DIRTY )
			scroll_xy(p, center_horiz(p), newtop, TRUE);
		else
			set_top_line(p, newtop, center_horiz(p));
	}
	/* Move cursor or selection */
	if(p->ccp.select) move_selection(p, p->nbrwc, p->nbl);
	inv_curs(p,TRUE);
	draw_info( p );
}

/*** Move cursor up ***/
void curs_up(Project p)
{
	LINE *ln;
	if( ( ln = p->edited->prev) )
	{
		LONG newx; BYTE scroll=0;

		inv_curs(p, FALSE);
		/* Is the cursor on top of display? */
		if(p->ycurs > gui.topcurs) p->ycurs -= YSIZE;
		else scroll=1;

		p->nbl--; p->edited=ln;
		p->nbrc  = adjust_rc(ln, p->nbrwc, &p->nbc, FALSE);
		p->xcurs = (p->nbrc-p->left_pos)*XSIZE + gui.left;

		/* If cursor exits edit area due to horizontal **
		** adjustment, scroll the display accordingly: */
		if((newx=center_horiz(p))!=p->left_pos || scroll)
			scroll_xy(p, newx, p->top_line-scroll, scroll);

		/* Update selection */
		if(p->ccp.select) move_selection(p, p->nbrc, p->nbl);
		inv_curs(p, TRUE);
		draw_info( p );
	}
}

/*** Move cursor down ***/
void curs_down(Project p)
{
	LINE *ln;
	if( ( ln = p->edited->next) )
	{
		LONG newx; BYTE scroll=0;

		inv_curs(p, FALSE);
		/* Is the cursor at the bottom of the display? */
		if( p->ycurs < gui.botcurs) p->ycurs += YSIZE;
		else scroll=1;

		p->nbl++; p->edited=ln;
		p->nbrc = adjust_rc(ln, p->nbrwc, &p->nbc, FALSE);
		p->xcurs = (p->nbrc-p->left_pos)*XSIZE + gui.left;

		/* Minimise calls to scroll_xy */
		if((newx=center_horiz(p))!=p->left_pos || scroll)
			scroll_xy(p, newx, p->top_line+scroll, scroll);

		/* Update selection */
		if(p->ccp.select) move_selection(p, p->nbrc, p->nbl);
		inv_curs(p, TRUE);
		draw_info( p );
	}
}

/*** Move cursor to the left ***/
void curs_left(Project p, BYTE word)
{
	register LINE *prev;
	if(p->nbc != 0)
	{
		inv_curs(p,FALSE);
		if(word) p->nbc = backward_word(p->edited, p->nbc-1); else p->nbc--;
		p->nbrwc = p->nbrc = x2pos(p->edited, p->nbc);
		p->xcurs = (p->nbrc-p->left_pos) * XSIZE + gui.left;

		/* Is it gone outside edit area? */
		if(p->nbrc<p->left_pos)
			scroll_xy(p, adjust_leftpos(p, -gui.xstep), p->top_line, FALSE);

		if(p->ccp.select) move_selection(p, p->nbrc, p->nbl);
		inv_curs(p,TRUE);
		draw_info( p );

	}	else if( ( prev = p->edited->prev ) ) {
		/* jump up */
		p->nbrwc = x2pos(prev, prev->size);
		curs_up(p);
	}
}

/*** One pos right ***/
void curs_right(Project p, BYTE word)
{
	if(p->nbc < p->edited->size)
	{
		inv_curs(p,FALSE);
		if(word) p->nbc = forward_word(p->edited, p->nbc); else p->nbc++;
		p->nbrwc = p->nbrc = x2pos(p->edited, p->nbc);
		p->xcurs = (p->nbrc-p->left_pos) * XSIZE + gui.left;

		/* Move the cursor */
		/* Is it gone outside edit area? */
		if(p->nbrc>=p->left_pos+gui.nbcol)
			scroll_xy(p, adjust_leftpos(p, gui.xstep), p->top_line, FALSE);

		if(p->ccp.select) move_selection(p, p->nbrc, p->nbl);
		inv_curs(p,TRUE);
		draw_info( p );
			
	}	else if(p->edited->next) {
		/* jump down to next line */
		p->nbrwc = 0;
		curs_down(p);
	}
}

/*** Jump the cursor far left or far right ***/
void jump_horiz(Project p, BYTE dir)
{
	if( dir >= 0 )
	{
		/* If cursor is already at rightmost position, scroll display */
		if( p->nbrwc == p->left_pos+gui.nbcol-1 )
		{
			p->nbrwc += gui.nbcol-1;
		}
		else p->nbrwc = p->left_pos+gui.nbcol-1;
	} else {
		/* Check if cursor is already at leftmost position */
		if( p->nbrwc == p->left_pos )
		{
			p->nbrwc = p->left_pos - (gui.nbcol-1);
			if( (LONG)p->nbrwc < 0 ) p->nbrwc = 0;
		}
		else p->nbrwc = p->left_pos;
	}

	inv_curs(p,FALSE);
	p->nbrc  = adjust_rc(p->edited, p->nbrwc, &p->nbc, dir<0);
	p->xcurs = (p->nbrc-p->left_pos)*XSIZE + gui.left;
	{
		LONG newleft = center_horiz( p );
		if( newleft != p->left_pos )
			scroll_xy(p, newleft, p->top_line, 0);
	}
	/* Move selection or cursor? */
	if(p->ccp.select) move_selection(p, p->nbrwc, p->nbl);
	inv_curs(p,TRUE);
	draw_info( p );
}

/*** Jump cursor to an absolute position in line (ie: beginning or end) ***/
void horiz_pos( Project p, ULONG newpos )
{
	if( newpos != p->nbrwc )
	{
		inv_curs(p, FALSE);
		p->nbrc = adjust_rc(p->edited, p->nbrwc = newpos, &p->nbc, FALSE);
		scroll_xy(p, center_horiz(p), p->top_line, FALSE);
		p->xcurs = (p->nbrc-p->left_pos)*XSIZE + gui.left;
		if(p->ccp.select) move_selection(p, p->nbrwc, p->nbl);
		inv_curs(p, TRUE);
		draw_info( p );
	}
}

/*** Jump the cursor at bottom or top of display ***/
void jump_vert(Project p, BYTE dir)
{
	LONG newline, newtop=p->top_line;
	if( dir>=0 )
	{
		/* Move cursor to bottom of display or scroll one page if it's already here */
		newline=newtop+gui.nbline-1;
		if( p->nbl==newline )
			newline+=gui.nbline,
			newtop+=gui.nbline;

		/* Want to jump after the last line? */
		if(newline>=p->max_lines) {
			newline=p->max_lines-1;
			newtop=(p->top_line + gui.nbline > p->max_lines ? p->top_line : newline-gui.nbline+1);
			if(newtop<0) newtop=0;
		}
	}	else {
		/* Same fight with reverse direction */
		newline=newtop;
		if( p->nbl==newline ) {
			newtop-=gui.nbline;
			if(newtop<0) newtop=0;
			newline=newtop;
		}
	}
	/* Adjust display according to cursor position */
	if(newline != p->nbl) {
		inv_curs(p,FALSE);
		set_cursor_line(p, newline, newtop);

		/* Set the selection flags before to scroll display */
		if(p->ccp.select) move_selection(p, p->nbrwc, p->nbl);
		scroll_xy(p,center_horiz(p), newtop, TRUE);
		inv_curs(p,TRUE);
		draw_info( p );
	}
}

/*** Move cursor down/up one page ***/
void pg_updown(Project p, BYTE dir)
{
	LONG newtop = p->top_line, newcrs;

	if(dir>0)
		/* One page down */
		if(newtop+gui.nbline >= p->max_lines) newcrs = p->max_lines-1;
		else {
			newtop += gui.nbline;
			if(newtop+gui.nbline >= p->max_lines)
				newtop = p->max_lines - gui.nbline;
			newcrs = newtop - p->top_line + p->nbl;
		}
	else
		/* One page up */
		if(newtop == 0) newcrs=0;
		else {
			newtop -= gui.nbline;
			if(newtop<0) newtop=0;
			newcrs = newtop - p->top_line + p->nbl;
		}
	if(newcrs != p->nbl) inv_curs(p,FALSE),set_cursor_line(p,newcrs,newtop);
	scroll_xy(p,center_horiz(p),newtop,TRUE);
	if(p->ccp.select) move_selection(p, p->nbrc, p->nbl);
	inv_curs(p,TRUE);
	draw_info( p );
}

/*** Split line ***/
void split_curline( Project p )
{
	if( split_line(&p->undo, p->edited, p->nbc, &p->nbrwc, prefs.auto_indent) )
	{
		register LINE *ln = p->edited;
		/* Redraw line where cursor is */
		Move(RP,gui.left,p->ycurs);
		write_text(p, ln);
		/* A line has been added */
		p->max_lines++; p->nbrwc = x2pos(ln,p->nbrwc);
		/* Scroll manually the display and redraw new line */
		if(p->ycurs < gui.botcurs)
		{
			ScrollRaster(RP, 0, -YSIZE, gui.left,p->ycurs+YSIZE-BASEL,gui.right,gui.bottom);
			Move(RP,gui.left,p->ycurs+YSIZE); ln = ln->next;
			write_text(p, ln);
			prop_adj(p);
		}
		/* Go down one line */
		curs_down(p);
	}	else ThrowError(Wnd, ErrMsg(ERR_NOMEM));
}

/*** Join two lines and strip spaces on the next ***/
void join_strip( Project p )
{
	LINE *ln;
	if((ln = p->edited->next) != NULL)
	{
		STRPTR data; ULONG i;
		inv_curs(p, FALSE);
		p->nbc = p->edited->size;
		for(i=0, data=ln->stream; TypeChar[*data] == SPACE && i<ln->size; i++, data++);

		reg_group_by(&p->undo);
		if(i != ln->size)
		{
			/* Do not add a blank if there is already one */
			if( p->nbc > 0 && TypeChar[ p->edited->stream[ p->nbc-1 ] ] != SPACE )
				add_char(&p->undo, p->edited, p->nbc, ' ');
			if( insert_str(&p->undo, p->edited, p->edited->size, data, ln->size-i) == 0 )
				ThrowError(Wnd, ErrMsg(ERR_NOMEM));
		}
		/* ln can't be the first */
		del_line(&p->undo, NULL, ln); p->max_lines--;
		reg_group_by(&p->undo);
		prop_adj(p);
		/* Refresh screen */
		p->nbrc = p->nbrwc = x2pos(p->edited, p->nbc);
		REDRAW_CURLINE( p );
		draw_info( p );
		scroll_up(p, p->edited->next, p->ycurs, center_horiz(p));
		inv_curs(p,TRUE);
	}
}

/*** Remove an entire line ***/
void amiga_k(Project p)
{
	LINE *del = p->edited;

	if(p->ccp.select) return;

	/* In there a next line, move cursor to */
	if( del->next ) {
		p->edited=del->next; p->max_lines--;
		if(p->show==del) p->show=del->next;
		prop_adj(p);
	}
	/* Adjust cursor position */
	inv_curs(p,FALSE);
	del_line(&p->undo, &p->the_line, del);
	p->nbrc = adjust_rc(p->edited, p->nbrwc, &p->nbc, FALSE);
	REDRAW_CURLINE(p);
	scroll_up(p, p->edited->next, p->ycurs, center_horiz(p));
	inv_curs(p,TRUE);
}

/*** Remove the char before the cursor ***/
void back_space(Project p, BYTE word)
{
	if(p->nbc!=0)
	{
		ULONG nbc = word ? backward_word(p->edited,p->nbc-1) : p->nbc-1;
		rem_chars(&p->undo, p->edited, nbc, p->nbc-1);

		/* Set cursor position and redraws it */
		inv_curs(p,FALSE);
		REDRAW_CURLINE(p);
		p->nbrwc = p->nbrc = x2pos(p->edited, p->nbc=nbc);
		p->xcurs = (p->nbrc-p->left_pos)*XSIZE + gui.left;
		if( (nbc = center_horiz(p)) != p->left_pos )
			scroll_xy(p, nbc, p->top_line, FALSE);
		inv_curs(p,TRUE);
		draw_info( p );
	}	else if(p->edited->prev != NULL) {

		p->edited = p->edited->prev;
		p->nbc    = p->edited->size;
		/* Join previous and current line */
		if( join_lines(&p->undo, p->edited, p->edited->next) )
		{
			/* Move cursor to the end of previous line */
			p->nbrwc = p->nbrc = x2pos(p->edited, p->nbc);

			p->max_lines--; p->nbl--;
			/* Require to scroll the display? */
			inv_curs(p, FALSE);
			if(p->ycurs>gui.topcurs)
				scroll_up(p, p->edited->next, p->ycurs-=YSIZE, center_horiz(p));
			else
				p->top_line--, p->show = p->edited,
				p->xcurs = (p->nbrc-p->left_pos)*XSIZE + gui.left;

			SetAPen(RP, pen.fg);
			REDRAW_CURLINE(p);
			/* Redraw the cursor: */
			inv_curs(p,TRUE);
			draw_info(p);
			prop_adj(p);
		}	else ThrowError(Wnd, ErrMsg(ERR_NOMEM));
	}
}

/*** Remove part of a line ***/
void cut_line(Project p, BYTE mode)
{
	/* Is there something to do? */
	if( (mode==0 && p->nbc==0) || (mode==1 && p->nbc==p->edited->size)) return;

	if( rem_chars(&p->undo, p->edited, mode ? p->nbc : 0, (mode ? p->edited->size : p->nbc)-1) )
	{
		/* Recompute cursor position */
		if(mode==0)
		{
			inv_curs(p,FALSE);
			p->nbc=p->nbrc=p->nbrwc=0;
			if(p->left_pos!=0) scroll_xy(p, 0, p->top_line, FALSE);
			p->xcurs=gui.left;
			draw_info(p);
		}
		REDRAW_CURLINE(p);
		inv_curs(p,TRUE);
	}	else ThrowError(Wnd, ErrMsg(ERR_NOMEM));
}

/*** Remove the char under the cursor ***/
void del(Project p, BYTE word)
{
	if(p->nbc < p->edited->size)
	{
		ULONG nbc = word ? forward_word(p->edited,p->nbc)-1 : p->nbc;
		rem_chars(&p->undo, p->edited, p->nbc, nbc);
		REDRAW_CURLINE(p);
		inv_curs(p, TRUE);
	}	else if(p->edited->next) {

		/* Join current and next line: */
		if( join_lines(&p->undo, p->edited, p->edited->next) )
		{
			REDRAW_CURLINE(p);
			/* Is it needed to scroll display? */
			if(p->ycurs < gui.botcurs)
				scroll_up(p,p->edited->next,p->ycurs,p->left_pos);

			/* Redraw the cursor: */
			inv_curs(p,TRUE);
			p->max_lines--;
			prop_adj(p);
		}	else ThrowError(Wnd, ErrMsg(ERR_NOMEM));
	}
}

/*** Move the cursor according to mouse click ***/
void click(Project p, WORD x, WORD y, BYTE update)
{
	WORD xp = (x-gui.left) / XSIZE,
	     yp = (y-gui.top) / YSIZE;
	LINE *ln;

	if(!p->ccp.select) inv_curs(p, FALSE);

	p->nbl = p->top_line + yp;
	for(ln=p->show; ln->next && yp; yp--,ln=ln->next);
	/* There was no lines, where we've clicked */
	p->nbl -= yp;
	p->edited = ln;
	p->nbrwc  = p->nbrc = xp+p->left_pos;

	xp = curs_visible(p,p->top_line);
	if(xp!=p->left_pos)
		scroll_xy(p, xp, p->top_line, FALSE);
	draw_info( p );
	RP->Mask = gui.selmask;

	/* Set starting selection point */
	if( update )
	{
		ln->flags = FIRSTSEL | LASTSEL;
		p->ccp.xc = p->ccp.xp = p->ccp.startsel = p->ccp.endsel = p->nbrc;
		p->ccp.yc = p->ccp.yp = p->nbl;
		p->ccp.cline = p->ccp.line = ln;
	}
}

/*** User release mouse button ***/
void unclick(Project p)
{
	/* Is there something selected ? */
	if( p->ccp.yp!=p->ccp.yc || p->ccp.startsel!=p->ccp.endsel )
		/* Yes ! */
		p->edited = p->ccp.cline, p->nbrwc=p->ccp.xc,
		set_cursor_line(p, p->nbl = p->ccp.yc, p->top_line);
	else
		p->edited->flags = 0, p->ccp.select = 0,
		inv_curs(p, TRUE), RP->Mask = gui.txtmask;
}
