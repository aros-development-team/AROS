/**************************************************************
**** Project.c: handle project related information.        ****
**** Free software under GNU license, started on 16/2/2000 ****
**** © T.Pierron, C.Guillaume.                             ****
**************************************************************/

#include <dos/dos.h>
#include <dos/exall.h>
#include <graphics/rastport.h>
#include <exec/memory.h>
#include <libraries/asl.h>
#include <workbench/startup.h>
#include "Project.h"
#include "Gui.h"
#include "ClipLoc.h"
#include "DiskIO.h"
#include "Utility.h"
#include "Cursor.h"
#include "Jed.h"
#include "ProtoTypes.h"
#include "Print.h"

#define  CATCOMP_NUMBERS			/* We will need the string id */
#include "strings.h"

#define DEBUG 0
#include <aros/debug.h>

static Project first = NULL;		/* Keep track of first created project */
UBYTE NbProject = 0;					/* Number of opened projects */

#ifndef	JANOPREF
/*** Alloc a new project ***/
Project new_project(Project ins, PREFS *prefs)
{
	Project new;
	if( ( new = (void *) AllocVec(sizeof(*new), MEMF_PUBLIC | MEMF_CLEAR) ) )
	{
		/* Always need at least one line */
		if( ( new->show = create_line(NULL,0) ) )
		{
			InsertAfter(ins, new);
			/* Local preferences herited */
			new->tabsize = prefs->tabsize;
			NbProject++;

			/* The line shown/edited */
			if(NbProject == 1) first = new;
			new->show->prev = new->show->next = NULL;
			new->the_line   = new->edited     = new->show;
			new->ccp.xp     = (ULONG)-1;
			new->max_lines  = 1;
			new->undo.prj   = new->redo.prj = (APTR) new;
			new->redo.rbtype = 1;
			new->protection = 0;
			set_project_name(new, NULL);
		}
		else FreeVec(new),new = NULL;
	}
	return new;
}

/*** Change content of a project ***/
void change_project(Project p, LINE *new)
{
	flush_undo_buf( &p->undo );
	flush_undo_buf( &p->redo );
	trash_file(p->the_line);
	if(p->buffer) FreeVec(p->buffer);
	memset(&p->nbc, 0, 4*sizeof(ULONG));
	p->edited    = p->the_line = p->show = new;
	p->top_line  = p->left_pos = 0;
	p->xcurs     = gui.left;
	p->ycurs     = gui.topcurs;
	p->state     = 0;
	p->savepoint = 0;
}

/*** Try to load the file contained in the project ***/
WORD load_in_project( Project p, STRPTR path )
{
	LoadFileArgs args;
	WORD         err;

	args.filename = path;

	if(RETURN_OK == (err = load_file( &args )))
	{
		/* File successfully loaded */
		change_project(p, args.lines);
		p->buffer    = args.buffer;
		p->max_lines = args.nblines;
		p->eol       = args.eol;
		p->protection= args.protection &= ~FIBF_ARCHIVE;
	}
	else ThrowError(Wnd, ErrMsg(err));

	return err;
}

/*** Load and create a new project from a path ***/
Project load_and_activate(Project ins, STRPTR name, BYTE use_prj)
{
	Project new = NULL;
    
	if( ( new = (use_prj ? ins : new_project(ins, &prefs)) ) )
	{
		if( use_prj >= 2 )
		{
			/* Multi-selection: name is actually a (StartUpArgs *) */
			if( ((StartUpArgs *)name)->sa_NbArgs > 0 )
			{
				new = create_projects(ins, ((StartUpArgs *)name)->sa_ArgLst, ((StartUpArgs *)name)->sa_NbArgs);
				if(new != ins ) {
					inv_curs(ins, FALSE);
					if(use_prj == 2) close_project(ins), FreeVec(ins);
					goto refresh;
				}
			}
			return NULL;
		}
		else /* Load a single file into `new' */
		{
			inv_curs(ins, FALSE);
			/* We have a correct path, try to load file */
			if( RETURN_OK == load_in_project(new, name) )
			{
				set_project_name(new, name);
				/* Add a panel tab and show content of file */
				if(use_prj) update_panel_name(ins);
				else        refresh:reshape_panel(new);
				active_project(new, TRUE);
				return new;
			}
		}
		/* Something failed, close project */
		if(use_prj == 0) close_project(new),FreeVec(new);
		FreeVec(name);
	}
	else ThrowError(Wnd, ErrMsg(ERR_NOMEM));
	return NULL;
}

/*** Load and create a new project from a path ***/
Project load_and_activate_fr(Project ins, APTR name, BYTE use_prj)
{
    struct FileRequester *fr = (struct FileRequester *) name;
	Project new = NULL;

	if( ( new = (use_prj ? ins : new_project(ins, &prefs)) ) )
	{
		if( use_prj >= 2 )
		{
			/* Multi-selection */
			if( fr->fr_NumArgs > 0 )
			{
				new = create_projects(ins, fr->fr_ArgList, fr->fr_NumArgs);
				if(new != ins ) {
					inv_curs(ins, FALSE);
					if(use_prj == 2) close_project(ins), FreeVec(ins);
					goto refresh;
				}
			}
			return NULL;
		}
		else /* Load a single file into `new' */
		{
			inv_curs(ins, FALSE);
			/* We have a correct path, try to load file */
			if( RETURN_OK == load_in_project(new, fr->fr_ArgList[0].wa_Name) )
			{
				set_project_name(new, fr->fr_ArgList[0].wa_Name);
				/* Add a panel tab and show content of file */
				if(use_prj) update_panel_name(ins);
				else        refresh:reshape_panel(new);
				active_project(new, TRUE);
				return new;
			}
		}
		/* Something failed, close project */
		if(use_prj == 0) close_project(new),FreeVec(new);
		FreeVec(name);
	}
	else ThrowError(Wnd, ErrMsg(ERR_NOMEM));
	return NULL;
}

/*** Reload project, flush redolog, reduce fragmentation ***/
void reload_project( Project p )
{
	/* Save cursor position */
	ULONG nbl = p->nbl, top = p->top_line;

	/* Flush all buffers and reload file */
	unset_modif_mark(p, TRUE);
	inv_curs(p, FALSE);
	load_in_project(p, p->path);
	
	/* Reset cursor position, if possible */
	p->nbl = nbl; p->top_line = top; top = center_vert( p );
	p->top_line = p->nbl = 0;
	set_top_line(p, top, 0);
	set_cursor_line(p, nbl, p->top_line);
	inv_curs(p, TRUE);
}

/*** Insert a file at current cursor position ***/
void insert_file( Project p )
{
	LoadFileArgs args;
	ULONG        length;
	WORD         err;

	if( ( args.filename = ask_load(Wnd, (AskArgs *)&p->path, TRUE, GetMenuText(208)) ) )
	{
		if( 0 == (err = read_file( &args, &length )) )
		{
			LONG pos[3];
			reg_group_by(&p->undo);
			if( add_string( &p->undo, p->edited, p->nbc, args.buffer, length, pos) )
			{
				/* Just one line concerned? */
				p->max_lines += pos[2];
				if( pos[1] == 0 ) REDRAW_CURLINE(p)

				move_cursor(p, pos[0], pos[1]);
				if( pos[1] > 0 )    redraw_content(p, p->show, gui.topcurs, gui.nbline);
				if( p->ccp.select ) move_selection(p, p->nbrc, p->nbl);
				inv_curs(p,TRUE); prop_adj(p);
			}
			else ThrowDOSError(Wnd, args.filename);

			if( args.buffer ) FreeVec( args.buffer );

			reg_group_by(&p->undo);
		}
		else ThrowError(Wnd, ErrMsg( err ));

		FreeVec( args.filename );
	}
}

/*** Change and set internal project name variables ***/
void set_project_name( Project p, STRPTR path )
{
	if(p->path != NULL) FreeVec(p->path);
	if(path != NULL)
		p->name = (STRPTR) FilePart(
		p->path = path);
	else
		p->path = NULL,
		p->name = ErrMsg(ERR_NONAME);
	p->labsize = strlen(p->name);
}

/*** Print one project ***/
char print_project(Project p)
{
    return print_file(p->the_line, p->eol);
}

/*** Save one project ***/
char save_project(Project p, char refresh, char ask)
{
        char retval;

	/* Ask for a name if file doesn't have one */
	if(p->path == NULL || ask)
	{
		STRPTR newname;
		if(NULL != (newname = ask_save(Wnd, (AskArgs *)&p->path, GetMenuText(105))) &&
		   0    != warn_overwrite( newname ))
			set_project_name(p, newname);
		else
			/* User cancelled or there was an error */
			return 0;
		p->state = 0;
	}

	if( refresh )
	{
		UpdateTitle(Wnd, p), update_panel_name( p );
    }

	retval = save_file(p->path, p->the_line, p->eol, p->protection);

	if (retval)
	{
        unset_modif_mark(p, TRUE);
    }

    return retval;
}

/*** Save all modified projects ***/
Project save_projects(Project active, char close)
{
	Project cur, p; int nb = 0;
	for(p=first; p; )
	{
		/* Auto-save modified project */
		if(p->state & MODIFIED) {
			if(save_project(p, FALSE, FALSE) == 0) break; else nb ++;
			if(p == active && close == 0) UpdateTitle(Wnd, p);
		}

		if(close == TRUE)
		{
			cur = p; close_project(p);
			p = p->next; FreeVec(cur);
			if(active == cur)
                            active = p;
                        nb = 0;
		}
		else p=p->next;
	}
	if(NbProject > 0 && nb > 0) reshape_panel(active);
	return active;
}

/*** Makes project the active one (i.e. visible) ***/
char active_project(Project p, char InitCurs)
{
	/* If file isn't loaded yet, makes it now */
	if(p->state & PAGINATED)
		/* This call is subject to failed! */
		switch( load_in_project(p, p->path) )
		{
			case RETURN_OK: p->state &= ~PAGINATED; break;
			case ERR_NOMEM:
				/* This is too nasty to attempt something: close faulty project */
				close_project(p); FreeVec(p); return 0;
		}

	prop_adj(p);
	init_tabstop(p->tabsize);
	UpdateTitle(Wnd, p);
	SetABPenDrMd(RP,pen.fg,pen.bg,JAM2);
	if(InitCurs)
		p->ycurs = gui.topcurs,
		p->xcurs = gui.left;
	else
		p->left_pos = curs_visible(p, p->top_line),
		p->xcurs    = (p->nbrc-p->left_pos)*XSIZE + gui.left;

	/* This can speedup a lot, text rendering */
	if(p->ccp.select != 0) RP->Mask = gui.selmask;
	redraw_content(p,p->show,gui.topcurs,gui.nbline);
	if(p->ccp.select == 0) RP->Mask = gui.txtmask;
	inv_curs(p,TRUE);
	draw_info(p);
	return 1;
}

/*** Test if a string represent an AmigaDOS pattern ***/
STRPTR IsPat( STRPTR name )
{
	int    len = strlen( name ) * 2 + 2;
	STRPTR tok;
	/* 512 bytes is used for ExAll buffer scan */
	if((tok = (STRPTR) AllocVec(len + 512, MEMF_CLEAR)))
	{
		if( ParsePatternNoCase(name, tok+512, len) > 0 )
        {
			return tok;
        }
        FreeVec( tok );
	}
	return NULL;
}

/*** Create a new project with an incomplete path ***/
UBYTE new_file(Project *ins, STRPTR path)
{
	Project new;
	if(NULL != (new = new_project(*ins, &prefs)))
	{
		UBYTE err;
	   if(RETURN_OK == (err = get_full_path(path, &new->name))) {
		   set_project_name(new, new->name); *ins = new;
			new->state = PAGINATED;
			return 1;
		} else
			ThrowError(Wnd, ErrMsg(err));
		close_project(new);
		FreeVec(new);
	} else
		ThrowError(Wnd, ErrMsg(ERR_NOMEM));
	return 0;
}

/*** Create a set of projects (command line or WBStartup) ***/
Project create_projects(Project ins_after, APTR args, ULONG nb)
{
	Project new;
	STRPTR  pattern;
	APTR    cwd = (APTR) CurrentDir( BNULL );

	if(nb > 0)
	{
		register struct WBArg *arg = (struct WBArg *)args;

		for(new = ins_after; nb--; arg++ )
		{
			CurrentDir(arg->wa_Lock);
			/* Directory scan is required with pattern */
			if( ( pattern = IsPat( (STRPTR) FilePart(arg->wa_Name) ) ) )
			{
				struct ExAllControl *eac;
				struct ExAllData    *ead;
				struct FileLock     *lock;
				char   more;
				((STRPTR)PathPart(arg->wa_Name))[0] = 0;

				if((lock = (void *) Lock(arg->wa_Name, SHARED_LOCK)))
				{
					CurrentDir((BPTR) lock );
					if ((eac = (void *) AllocDosObject(DOS_EXALLCONTROL,NULL)))
					{
						eac->eac_LastKey = 0;
						eac->eac_MatchString = pattern+512;
						do {
							more = ExAll((BPTR)lock, (struct ExAllData *)pattern, 512, ED_TYPE, eac);
							if( eac->eac_Entries == 0 ) continue;
							for(ead = (void *)pattern; ead; ead = ead->ed_Next)
                            {
								if(ead->ed_Type < 0 && !new_file(&new, ead->ed_Name))
									goto stop_now;
                            }
                        } while( more );
						stop_now: FreeDosObject(DOS_EXALLCONTROL, eac);
					}
					UnLock((BPTR) lock );
				}	/* else bad pattern: discard entry */
				FreeVec( pattern );
			}
			/* Given file contains no pattern */
			else if( !new_file(&new, arg->wa_Name) )
				break;
		}
		CurrentDir((BPTR)cwd );
		/* Still have no file to edit (pattern with no match) */
		if(new == NULL) goto newempty;

		/* We have a bunch of projects, try to load at least one */
		while(new != ins_after)
			if(RETURN_OK != load_in_project(new, new->path)) {
				Project prev = new->prev;
				ThrowDOSError(Wnd, new->path);
				close_project(new);
				FreeVec(new); new = prev;
			} else break;
	} else
		newempty: new = new_project(NULL, &prefs);
	CurrentDir((BPTR) cwd );
	return new;
}

/*** set modification flag on this project ***/
void set_modif_mark(Project p)
{
	p->state |= MODIFIED;
	if( p->path )
	{
		update_panel_name( p );
		UpdateTitle(Wnd, p);
	}
}
void unset_modif_mark(Project p, char update)
{
	commit_work( p );
	p->state = 0;
	if(update)
	{
		update_panel_name(p);
		UpdateTitle(Wnd, p);
	}
}

/*** Close one project and ressources allocated ***/
char close_project(Project p)
{
	/* Close it, only if user knows what he does */
	if( warn_modif(p) )
	{
		NbProject--;
		trash_file(p->the_line);
		flush_undo_buf( &p->undo );
		flush_undo_buf( &p->redo );
		if(p->path)   FreeVec(p->path);
		if(p->buffer) FreeVec(p->buffer);
		Destroy(&first, p);
		return TRUE;
	}
	return FALSE;
}

/*** Close all projects, returning the first non-closed or NULL if none ***/
Project close_projects( void )
{
	register Project prj;
	while( first )
	{
		prj = first;
		/* If user doesn't want to close this one */
		if( close_project(first) ) FreeVec(prj);
		else break;
	}
	/* If NULL, then we should quit */
	return first;
}
#endif

/*** PROJECT BAR, GRAPHICAL ROUTINES ***/

/*** Draw one NextStep-like panel item ***/
void draw_panel(Project p, BYTE rclear, BYTE lclear, BYTE activ)
{
	BYTE max = (gui.top-gui.oldtop-1) >> 1, i;
	WORD right = p->pleft + p->pwidth - max - 1;

	WORD bgpan = (activ ? pen.abpan : pen.bgpan);

	/* Bottom line */
	if(rclear == 0) {
		SetAPen(&RPT,pen.shine);
		Move(&RPT, gui.left, gui.top-2);
		Draw(&RPT, p->pleft, RPT.cp_y);
	}
	/* Right shape */
	for(i=0; i<max; i++) {
		Move(&RPT, p->pleft+i, gui.top-2);        SetAPen(&RPT,    bgpan);
		Draw(&RPT, RPT.cp_x,   gui.top-2-(i<<1)); SetAPen(&RPT,pen.shine);
		Draw(&RPT, RPT.cp_x,   RPT.cp_y-1);
		if(rclear) {
			SetAPen(&RPT, pen.panel);
			Move(&RPT, RPT.cp_x, RPT.cp_y-1);
			Draw(&RPT, RPT.cp_x, gui.oldtop);
		}
	}
	/* Upper line */
	Move(&RPT, RPT.cp_x+1, gui.oldtop); SetAPen(&RPT,pen.shine);
	Draw(&RPT, right,      gui.oldtop); SetAPen(&RPT,    bgpan);

	RectFill(&RPT, p->pleft+max, gui.oldtop+1, RPT.cp_x, gui.top-2);
	SetABPenDrMd(&RPT, pen.fgpan, bgpan, JAM2);

	/* Text in the box */
	Move(&RPT,p->pleft+((p->pwidth-(p->labwid+p->modwid))>>1),
		gui.oldtop+prefs.scrfont->tf_Baseline+3 );
	if(!activ && (p->state & PAGINATED)) RPT.AlgoStyle |= FSF_ITALIC;
	Text(&RPT,p->name,p->labsize);
	if(p->state & MODIFIED)
		Text(&RPT,STR_MODIF,1);

	/* Right shape */
	for(i=0,right++; i<max; i++) {
		Move(&RPT, right+i,  gui.top-2);          SetAPen(&RPT,    bgpan);
		Draw(&RPT, RPT.cp_x, gui.top-((max-i)<<1)); SetAPen(&RPT,pen.shade);
		Draw(&RPT, RPT.cp_x, RPT.cp_y-1);
		if(lclear && i) {
			SetAPen(&RPT,pen.panel);
			Move(&RPT, RPT.cp_x, RPT.cp_y-1);
			Draw(&RPT, RPT.cp_x, gui.oldtop);
		}
	}
	/* Bottom line */
	if( !lclear ) {
		SetAPen(&RPT,pen.shine);
		Move(&RPT, RPT.cp_x,  gui.top-2);
		Draw(&RPT, gui.right, RPT.cp_y);
	}
	SetBPen(&RPT, pen.bg);
}

#ifndef	JANOPREF
/*** Search if a tab is select using mouse ***/
Project select_panel(Project current, WORD x)
{
	Project new;
	/* Search for the hi-lighted panel */
	if( x == NEXT_PROJECT )	new=current->next; else
	if( x == PREV_PROJECT ) new=current->prev; else
	if( x < 0 )
		for(new=first; new && ++x; new=new->next);
	else
		for(new=first; new; new=new->next)
			if(new->pleft <= x && x < new->pleft+new->pwidth) break;

	/* Don't change project if it is the same */
	if(new && new != current)
	{
		/* Refresh panel tab first */
		inv_curs(current, FALSE);
		RPT.AlgoStyle = FS_NORMAL; draw_panel(current, FALSE, FALSE, FALSE);
		RPT.AlgoStyle = FSF_BOLD;  draw_panel(new,     FALSE, FALSE, TRUE);

		/* Then content */
		if( active_project(new, FALSE) == 0 )
			reshape_panel(current);
		else return new;
	}
	return current;
}

/*** Title of panel has changed, update it ***/
void update_panel_name( Project active )
{
	/* Assuming that active is the currently selected */
	active->labwid = TextLength(&RPT, active->name,
		active->labsize=strlen(active->name));
	if(active->state & MODIFIED)
		active->labwid +=
			(active->modwid=TextLength(&RPT, STR_MODIF, 1));
	if(NbProject > 1)
	{
		RPT.AlgoStyle = FSF_BOLD;
		draw_panel(active, FALSE, FALSE, TRUE);
	}
}
#endif

/*** Recompute size of each item and show the active project ***/
void reshape_panel(Project active)
{
	Project lst; UBYTE i;
	/* Do not show the project bar if there is only one project */
	if( NbProject > 1 )
	{
		WORD height;
		/** Change top position, if not already **/
#ifndef	JANOPREF
		if(gui.top == gui.oldtop)
			adjust_win(Wnd, 1), clear_brcorner();
#endif

		/** Compute items size **/
		for(lst=first,i=1,height=(gui.top-gui.oldtop-2)>>1; lst; lst=lst->next,i++)
		{
			lst->pleft  = (lst==first ? gui.left : lst->prev->pleft+lst->prev->pwidth-height);
			lst->pwidth = (gui.right+1) * i / NbProject - lst->pleft;
			lst->labwid = TextLength(&RPT, lst->name, lst->labsize);
		}
		/** Redraw the project bar **/
		for(lst=first; lst; lst=lst->next) {
			RPT.AlgoStyle = FS_NORMAL;
			/* Active project should be the last drawn */
			if(lst != active)
				draw_panel(lst, lst->prev==active || !lst->prev, TRUE, FALSE);
		}
		RPT.AlgoStyle = FSF_BOLD;
		draw_panel(active, !active->prev, !active->next, TRUE);
	}
#ifndef	JANOPREF
	else if(gui.oldtop != gui.top) {
		SetAPen(&RPT, pen.bg);
		RectFill(&RPT, gui.left, gui.oldtop, gui.right, gui.top);
		/** Remove project bar, if it lefts only one project **/
		adjust_win(Wnd, 0); clear_brcorner();
	}
#endif
}
