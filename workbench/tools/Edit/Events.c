/**********************************************************
** Events.c : Process events coming from main window and **
** public port. Written by T.Pierron and C.Guillaume.    **
** Free software under terms of GNU license. 12 nov 2000 **
**********************************************************/

#include <intuition/intuition.h>            /* Std types */
#include <devices/inputevent.h>             /* For raw keymap conversion */
#include <dos/dos.h>
#include "Jed.h"
#include "Utility.h"
#include "Events.h"
#include "IPC_Prefs.h"
#include "DiskIO.h"
#include "Macros.h"
#include "Gui.h"
#include "Edit.h"
#include "Search.h"
#include "ProtoTypes.h"

#define  CATCOMP_NUMBERS   /* String ID for ErrMsg() */
#include "strings.h"

static struct InputEvent   ie = {0,IECLASS_RAWKEY}; /* Keyboard translation map */
extern struct IntuiMessage msgbuf;
extern Project edit;

UBYTE record = 0;

/*** Process keyboard events ***/
void handle_kbd(Project p)
{
	static UBYTE buffer[8], shift;

	/* Look is rawkey can be processed, thus doesn't translate it */
	if(msgbuf.Code > 0x7E) { record |= 0x80; return; }

	/* Look if keypad should be processed as a PC one */
	if( (*buffer = (msgbuf.Qualifier & IEQUALIFIER_NUMERICPAD && msgbuf.Code >= N0_KEY &&
	               msgbuf.Code <= N9_KEY && (prefs.xtend || msgbuf.Qualifier & CTRLKEYS)))
	    && !prefs.xtend )
		/* Clear CONTROL qualifier, if no PC keypad emulation */
		msgbuf.Qualifier &= ~CTRLKEYS;

	shift = (msgbuf.Qualifier & SHIFTKEYS ? 1 : 0);

	switch( msgbuf.Code )
	{
		case N0_KEY:
			if( *buffer ) {
				/* Switch with replacement cursor */
				p->cursmode = !p->cursmode;
				inv_curs(p, FALSE); inv_curs(p, TRUE);
				return;
			}	break;
		case N1_KEY: if( *buffer ) { horiz_pos(p,MAXPOS); return; } break;
		case N3_KEY: if( *buffer ) { pg_updown(p, 1);     return; } break;
		case N7_KEY: if( *buffer ) { horiz_pos(p, 0);     return; } break;
		case N9_KEY: if( *buffer ) { pg_updown(p,-1);     return; } break;
		case SPACE_KEY:
			/* Amiga space indent line */
			if( msgbuf.Qualifier & AMIGAKEYS ) {
				indent_by(p, ' ', shift ? -1:1); return;
			}	break;
		case TAB_KEY:
			if( msgbuf.Qualifier & AMIGAKEYS ) {
				indent_by(p, '\t',shift ? -1:1); return;
			}	break;
		case BS_KEY:
			if( p->ccp.select )
				del_block( p );
			else if( msgbuf.Qualifier & AMIGAKEYS )
				amiga_k(p);
			else if( shift )
				cut_line(p,0);
			else
				back_space(p, (msgbuf.Qualifier & ALTKEYS) != 0);
			return;
		case ESC_KEY:
			/* Pressing ESC key while text is selected, unmark all **
			** otherwise we want to add the escape character       */
			if(p->ccp.select==0) break;
			unmark_all(p,TRUE); return;
		case NPERIOD_KEY: if( *buffer == 0 ) break;
		case DEL_KEY:
			if( p->ccp.select )
				del_block( p );
			else if( shift )
				cut_line(p,1);
			else
				del(p, (msgbuf.Qualifier & ALTKEYS) != 0);
			return;
		case N8_KEY: if( *buffer == 0 ) break;
		case UP_KEY:
			if( msgbuf.Qualifier & CTRLKEYS )
				move_to_line(p,0,LINE_AS_IS);
			else if( shift )
				if( msgbuf.Qualifier & ALTKEYS )
					scroll_ydelta(p,-1);
				else
					jump_vert(p,-1);
			else
				curs_up(p);
			return;
		case N2_KEY: if( *buffer == 0 ) break;
		case DOWN_KEY:
			if( msgbuf.Qualifier & CTRLKEYS )
				move_to_line(p,p->max_lines-1,LINE_AS_IS);
			else if( shift )
				if( msgbuf.Qualifier & ALTKEYS )
					scroll_ydelta(p, 1);
				else
					jump_vert(p, 1);
			else
				curs_down(p);
			return;
		case N6_KEY: if( *buffer == 0 ) break;
		case RIGHT_KEY:			/* Used for various things! */
			if( msgbuf.Qualifier & CTRLKEYS )
				if( shift )
					edit = select_panel(edit, NEXT_PROJECT);
				else
					horiz_pos(p,MAXPOS);
			else if( shift )
				if( msgbuf.Qualifier & ALTKEYS )
					scroll_xdelta(p, gui.xstep);
				else
					jump_horiz(p, 1);
			else
				curs_right(p, (msgbuf.Qualifier & ALTKEYS) != 0);
			return;
		case N4_KEY: if( *buffer == 0 ) break;
		case LEFT_KEY:
			if( msgbuf.Qualifier & CTRLKEYS )
				if( shift )
					edit = select_panel(edit, PREV_PROJECT);
				else
					horiz_pos(p,0);
			else if( shift )
				if( msgbuf.Qualifier & ALTKEYS )
					scroll_xdelta(p, -gui.xstep);
				else
					jump_horiz(p,-1);
			else
				curs_left(p, (msgbuf.Qualifier & ALTKEYS) != 0);
			return;
		case RETURN_KEY:
		case NENTER_KEY:
			if( msgbuf.Qualifier & CTRLKEYS )
			{
				STRPTR  path  = GetIncludeFile(p, p->edited);
				Project new   = NULL;

				if(NULL != path && (shift == 0 || warn_modif(edit)))
					new = load_and_activate(edit, path, shift);
				if(NULL != new)
					edit = new;
			}
			else split_curline( p ); return;
#ifdef __AROS__
		case PGDOWN_KEY: pg_updown(p, 1); return;
		case PGUP_KEY:   pg_updown(p, -1); return;

		case HOME_KEY:
			if( msgbuf.Qualifier & CTRLKEYS ) move_to_line(p,0,LINE_AS_IS);
			else                              horiz_pos(p,0);
			return;
		case END_KEY:
			if( msgbuf.Qualifier & CTRLKEYS ) move_to_line(p,p->max_lines-1,LINE_AS_IS);
			else                              horiz_pos(p,MAXPOS);
			return;
			
		case RAWKEY_NM_WHEEL_UP:
			scroll_ydelta(p, -3);
			return;
			
		case RAWKEY_NM_WHEEL_DOWN:
			scroll_ydelta(p, 3);
			return;
#endif

#if	DEBUG
		case F1_KEY:
			printf("mask = 0x%02x\n", RP->Mask); return;
		case F2_KEY:
			show_modifs(&p->undo); return;
		case F3_KEY:
			show_modifs(&p->redo); return;
#endif
	}

	/* Translate key (with dead one) using keymap library */
	ie.ie_Code = msgbuf.Code;
	ie.ie_EventAddress = *((APTR *)msgbuf.IAddress);

	/* Look if CTRL qualifier is used */
	if( msgbuf.Qualifier & (CTRLKEYS|AMIGAKEYS) )
	{
		/* Discard qualifiers, if CTRL+<a-z> is pressed a control char will **
		** be returned, which does not reflect the key we want to process.  */
		ie.ie_Qualifier = msgbuf.Qualifier & ~(CTRLKEYS | AMIGAKEYS | IEQUALIFIER_CAPSLOCK);

		/* If it's not a known shortcut, insert control char instead */
		if(MapRawKey(&ie, buffer, 8, NULL) > 0)
		{
			if( msgbuf.Qualifier & AMIGAKEYS )
			{
				/* This is too annoying to insert into menus: Amiga+2~9 enable **
				** to quickly change tabstop of current project (not in prefs) */
				if( '2' <= *buffer && *buffer <= '9' )
				{
					WORD tabstop = *buffer - '0';
					if(tabstop != p->tabsize)
						p->tabsize = tabstop, inv_curs(p, FALSE),
						active_project(p, FALSE);
					return;
				}
			}
			/* CTRL + `1' ~ `0' => activate project nb. x */
			else if( '0' <= *buffer && *buffer <= '9' )
			{
				edit = select_panel(edit,*buffer == '0' ? -10 : '0' - *buffer);
				return;
			}
			else switch(*buffer)
			{
				case '\\':change_case(p, 0); return;
				case '/': change_case(p, 1); return;
				case 'j': join_strip(p);     return;
				case 'n': FindPattern(p, 1); return;
				case 'p': FindPattern(p,-1); return;
				case 'q': handle_menu(111);  return;
				case 'Q': handle_menu(113);  return;
				case 'r': ReplacePattern(p); return;
				case 'R': ReplaceAllPat(p);  return;
				case 's': FindWord(p, 1);    return;
				case 'S': FindWord(p,-1);    return;
				case 'z': last_modif(&p->undo, 0); return;
				case '[': handle_menu(401);  return;
				case ']': handle_menu(402);  return;
			}
		}
	}

	/* Make sure this time qualifiers are taken into account */
	ie.ie_Qualifier = msgbuf.Qualifier;

	/* Map RAWKEY to ANSI (dead keys return 0) */
	if(MapRawKey(&ie, buffer, 8, NULL) > 0)
	{
/*		register UBYTE code = *buffer; */

		/* Inserting one char is the most common operation **
		** and therefore needs to be highly optimized:     */
		if( add_char(&p->undo, p->edited, p->nbc, *buffer) )
		{
			REDRAW_CURLINE(p);
			curs_right(p, FALSE);
		}	else ThrowError(Wnd, ErrMsg(ERR_NOMEM));

		if(record) reg_act_addchar( *buffer ), record |= 0x80;
	}
}

/*** Handle menu related events ***/
void handle_menu( LONG MenuID )
{
	static UBYTE  shift;

	shift = (msgbuf.Qualifier & SHIFTKEYS ? 1 : 0);

	switch( MenuID )
	{
		case 101:	/* New file */
		{	Project new;
			BusyWindow(Wnd);
			if( ( new = new_project(edit, &prefs) ) )
			{
				/* Compute panel tabs size */
				inv_curs(edit, FALSE);
				reshape_panel(new);
				active_project(edit=new, TRUE);

			}	else ThrowError(Wnd, ErrMsg(ERR_NOMEM));
			WakeUp(Wnd);
		}	break;
		case 102:	/* Split open */
		{	STRPTR path;
			/* Ask a new file name, using the same working directory as current document */
			if(NULL != (path = ask_load(Wnd, (AskArgs *)&edit->path, FALSE, GetMenuText(102))))
			{
				Project new;
				/* Use current project if it is empty and unmodified */
				if( (new = load_and_activate(edit, path, (edit->path == NULL && (edit->state & MODIFIED) == 0) ? 2 : 3) ) )
					edit = new;
			}
		}	break;
		case 103:	/* Open (in current panel) */
			if( warn_modif(edit) )
			{
				if( shift == 0 )
				{
					STRPTR path;
					if(NULL != (path = ask_load(Wnd, (AskArgs *)&edit->path, TRUE, GetMenuText(103))))
						load_and_activate(edit, path, 1);
				}
				else reload_project( edit );
			}	break;
		case 105:	/* Save */
			if(0 == shift) goto case_sav;

		case 106: save_project (edit,TRUE,TRUE);  break; /* Save as */
		case_sav: save_project (edit,TRUE,FALSE); break; /* Save one file */
		case 107: save_projects(edit,FALSE);      break; /* Save changes */
		case 109: show_info(edit);                break; /* Information */

		case 113:	/* Save if necessary, then quit */
			if((edit->state & MODIFIED) && save_project(edit, TRUE, FALSE) == 0)
				break;
		case 111:	/* Close project */
		{	Project new = edit->prev;
			if(new == NULL) new = edit->next;
			/* Check if there were modifications */
			if( close_project(edit) )
			{
				/* If there is another opened project, shows it */
				inv_curs(edit,FALSE); FreeVec(edit);
				if( new != NULL )
					reshape_panel(new),
					active_project(edit = new, FALSE);
				else
					/* Otherwise quits */
					cleanup(0,0);
			}
		}	break;
		case 112:    /* Quit */
			/* Modified project not yet saved */
			inv_curs(edit, FALSE);
			if(NULL != (edit = (shift ? save_projects(edit, TRUE) : close_projects())))
				reshape_panel(edit),
				active_project(edit, FALSE);
			else
				cleanup(0,0);
			break;
		case 201:	/* Cut */
		case 202:	/* Copy to clipboard */
			if( edit->ccp.select == 0 ) break;

			if( CBWriteFTXT(edit->ccp.yc > edit->ccp.yp ? edit->ccp.line : edit->ccp.cline, &edit->ccp) ) {
				if( MenuID == 202 ) unmark_all(edit,TRUE);
				else del_block( edit );
			}
			break;
		case 203:	/* Paste from clipboard */
		{	LONG buf[3];
			/* Read chars */
			if( !CBReadCHRS(&edit->undo, edit->edited, edit->nbc, buf) )
				/* CBReadCHRS will show the right error */
				break;

			/* Just one line concerned? */
			edit->max_lines += buf[2];
			if( buf[1] == 0 ) REDRAW_CURLINE(edit)

			/* Move cursor to the end of pasted text? */
			if( shift == 0 ) move_cursor(edit,buf[0],buf[1]);
			if( buf[1]>0 ) redraw_content(edit,edit->show,gui.topcurs,gui.nbline);
			if( edit->ccp.select ) move_selection(edit, edit->nbrc, edit->nbl);
			inv_curs(edit,TRUE); prop_adj(edit);
		}	break;
		case 204:	/* Mark text */
			if(shift) MenuID=205;
		case 205:	/* Mark columnar */
			move_selection = SwitchSelect(edit,MenuID-204,0);
			break;
		case 206:  mark_all(edit);          break; /* Select all */
		case 207:  amiga_k(edit);           break; /* Del line */
		case 2071: indent_by(edit,'\t', 1); break; /* Indent */
		case 2072: indent_by(edit,'\t',-1); break; /* Unindent */
		case 2073: change_case(edit,0);     break; /* Upper */
		case 2074: change_case(edit,1);     break; /* Lower */
		case 2075: change_case(edit,2);     break; /* Toggle */
		case 208:  insert_file(edit);       break; /* Insert file */
		case 209:
			if(shift == 0) {
				rollback(&edit->undo); break;        /* Undo */
			}
		case 210:  rollback(&edit->redo);   break; /* Redo */
		case 301:  setup_winsearch(edit,0); break; /* Search */
		case 302:  setup_winsearch(edit,1); break; /* Replace */
		case 3031: FindPattern(edit, 1);    break; /* Find next */
		case 3032: FindPattern(edit,-1);    break; /* Find prev */
		case 3033: ReplacePattern(edit);    break; /* Replace next */
		case 304:  pg_updown(edit,-1);      break; /* PgUp */
		case 305:  pg_updown(edit, 1);      break; /* PgDown */
		case 306:  goto_line(edit);         break; /* Goto line */
		case 307:  match_bracket(edit);     break; /* Match bracket */
		case 308:  last_modif(&edit->undo,0); break; /* Last modif */
		case 309:  horiz_pos(edit,0);       break; /* Home */
		case 310:  horiz_pos(edit,MAXPOS);  break; /* End */
		case 401:  start_macro();           break; /* Record */
		case 402:  stop_macro();            break; /* Stop recording */
		case 403:
			if(shift == 0) { play_macro(1);  break; } /* Play current macro */
		case 404:  repeat_macro(edit);      break; /* Repeat one or more times */
		case 501:  ask_new_screen();        break; /* Change screen mode */
		case 502:  ask_new_font();          break; /* Change text font */
		case 503:  setup_winpref();         break; /* General prefs */
		case 505:  save_prefs(&prefs);      break; /* Save prefs */

		case 504:  ask_prefs(edit,0,GetMenuText(504)); break; /* Load prefs */
		case 506:  ask_prefs(edit,1,GetMenuText(506)); break; /* Save prefs as */
	}
}


/** Public port of Jano **/
static struct MsgPort *port = NULL, *reply;
static struct JPacket *cmd  = NULL;

UBYTE *PortName = JANO_PORT;

/** Look if jano is already running **/
char find_janoed( StartUpArgs *args )
{
	ULONG sigwait;
	if( (reply = (struct MsgPort *) FindPort(PortName)) )
	{
		PortName = NULL;		/* Private port */
		if( ( sigwait = create_port() ) )
		{
			/* Send to JanoEditor that someone tries to start it again */
			cmd->class    = CMD_NEWEDIT;
			cmd->msg.args = args;
			PutMsg(reply, (struct Message *)cmd);
			/* cmd packet is associated with "port", thus reply will be done here */
			Wait( sigwait | SIGBREAKF_CTRL_C );
			/* Unqueue message */
			GetMsg( port );
		}
		/* Cleanup will be done later */
		return 1;
	} else return 0;
}

/** Setup public port of the editor **/
ULONG create_port( void )
{
	/* Create a port and  */
	if( ( port = (struct MsgPort *) CreateMsgPort() ) )
	{
		/* Set this port public */
		port->mp_Node.ln_Name = PortName;
		port->mp_Node.ln_Pri  = 0;
		AddPort( port );

		/* Create a message that can be sent to the editor */
		if( ( cmd = (struct JPacket *) CreateIORequest(port, (long) sizeof (*cmd)) ) )
			return (ULONG)(1 << port->mp_SigBit);

		DeletePort(port);
	}
	return 0;
}

/** Send a command to preference tool **/
char send_pref(PREFS *prefs, ULONG class)
{
	/* The port can be shutted down at any time! */
	if( ( reply = (struct MsgPort *) FindPort(JANOPREFS_PORT)) )
	{
		cmd->class = class;
		CopyMem(prefs, &cmd->msg.prefs, sizeof(*prefs));

		PutMsg(reply, (struct Message *)cmd);
		Wait( 1 << port->mp_SigBit | SIGBREAKF_CTRL_C );
		GetMsg( port );
		return 1;
	} else return 0;
}

/** Shutdown port **/
void close_port( void )
{
	if( cmd )  DeleteExtIO((struct IORequest *)cmd);
	if( port ) RemPort(port), DeleteMsgPort(port);
}

/*** Handle messages posted to public port of Jano ***/
void handle_port( void )
{
	struct JPacket *msg; char update = 0;
	extern PREFS tmpprefs;
	while( ( msg = (struct JPacket *) GetMsg(port) ) )
	{
		switch( msg->class )
		{
			case CMD_NEWEDIT:
				/* Look if there are projects to load */
				if(msg->msg.args->sa_NbArgs > 0)
				{
					inv_curs(edit, FALSE);
					edit = create_projects(edit, msg->msg.args->sa_ArgLst, msg->msg.args->sa_NbArgs);
					reshape_panel(edit);
					active_project(edit,TRUE);
					clear_brcorner();
				}
			case CMD_SHOW:
				WindowToFront( Wnd );
				ScreenToFront( Scr );
				ActivateWindow( Wnd );
				break;
			case CMD_KILL: cleanup(0,0); break;
			case CMD_PREF:
				/* Send a copy of preference struct */
				prefs.current = Scr;
				CopyMem(&prefs, &msg->msg.prefs, sizeof(prefs));
				break;
			case CMD_SAVPREF: update = 2; goto save;
			case CMD_NEWPREF: update = 1;
				/* Preference tool has sent a new config! */
				save: CopyMem(&msg->msg.prefs, &tmpprefs, sizeof(tmpprefs)); break;
		}
		ReplyMsg((struct Message *)msg);
	}
	/* Preferences have changed? */
	if(update == 2) save_prefs(&tmpprefs);
	if(update >= 1) update_prefs(edit);
}
