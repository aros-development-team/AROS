/***************************************************
**                                                **
**      $VER: Macros.c 1.0 (15.8.2001)            **
**      Record sequence of keystrokes             **
**                                                **
**      © T.Pierron, C.Guilaume. Free software    **
**      under terms of GNU public license.        **
**                                                **
***************************************************/

#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <exec/memory.h>
#include "Project.h"
#include "Macros.h"
#include "Gui.h"
#include "Utility.h"
#include "Cursor.h"
#include "Jed.h"
#include "Events.h"
#include "ProtoTypes.h"

#define	CATCOMP_NUMBERS
#include "strings.h"

extern STRPTR  InfoTmpl;
extern UBYTE   record;
extern Project edit;
static STRPTR  OldInfo = NULL;

UBYTE NewTmpl[20], TmplLen, TmplWid;

/** Manage a set of macros **/
Macro MainMacro = NULL, MacCur = NULL, LastChunk;

UBYTE SzOp[] = {
	sizeof( *(ActChar)0L ),
	sizeof( *(ActMenu)0L ),
	sizeof( *(ActShortcut)0L )
};

/*** Setup some variables ***/
void init_macros( void )
{
	TmplLen = strlen(
	strcpy(NewTmpl, ErrMsg(WARN_REC)));
	strcat(NewTmpl, "  "); TmplLen += 2;
	strcat(NewTmpl, InfoTmpl);
}

/*** Free memory used by a macro ***/
void free_macro( Macro m )
{
	Macro next;
	for(; m ; next = m->next, FreeVec(m), m = next);
}
void free_macros( void )
{
	free_macro(MainMacro);
	free_macro(MacCur);
}

/*** Start recording keystrokes ***/
void start_macro( void )
{
	if(OldInfo == NULL)
	{
		/* Show a special msg in info template to show reccording state */
		free_macro(MacCur);
		OldInfo  = InfoTmpl;
		InfoTmpl = NewTmpl;
		TmplWid  = TextLength(prefs.backdrop ? &Scr->RastPort : &RPT, NewTmpl, TmplLen);
		record   = 0x81;
		MacCur   = LastChunk = NULL;
		gui.xinfo -= TmplWid;
		ThrowError(Wnd, ErrMsg(WARN_RECORD));
		draw_info( edit );
	}
}

void stop_macro( void )
{
	if(OldInfo != NULL)
	{
		InfoTmpl = OldInfo; OldInfo = NULL;
		record   = FALSE;
		gui.xinfo += TmplWid;
		/* Do not save something if nothing has been recorded */
		if(MacCur != NULL) {
			free_macro( MainMacro ); MainMacro = MacCur; MacCur = NULL;
			ThrowError(Wnd, ErrMsg(WARN_RECORDED));
		}
		else UpdateTitle(Wnd, edit);
	}
}

/** Alloc a new chunk for recording keystroke **/
void *new_action( UWORD size )
{
	if(LastChunk == NULL || LastChunk->usage + size > SZ_MACRO)
	{
		Macro new;
		/* Alloc a new chunk, not enough room */
		if(NULL == (new = (Macro) AllocVec(sizeof(*new), MEMF_CLEAR)))
			return NULL;

		if(LastChunk) LastChunk->next = new;
		else MacCur = new;  LastChunk = new;
		new->usage = size;
		return new->data;
	}
	else /* Still some place in current chunk */
	{
		STRPTR new = LastChunk->data + LastChunk->usage;
		LastChunk->usage += size;
		return new;
	}
}

/** Register actions **/
void reg_act_addchar( UBYTE code )
{
	ActChar new;
	if( ( new = new_action( sizeof(*new) ) ) )
		new->Type = MAC_ACT_ADD_CHAR, new->Char = code;
}
void reg_act_com( UBYTE type, UWORD code, UWORD qual )
{
	ActMenu new;
	if( ( new = new_action( sizeof(*new) ) ) )
		new->Type = type, new->Code = code, new->Qual = qual;
}

/** Play current macro **/
void play_macro( int nb_times )
{
	extern struct IntuiMessage msgbuf;
	WORD selmask, txtmask;
	Macro m;

	if(MainMacro == NULL) return;

	/* Disconnect display, just to be clean, not for performance */
	inv_curs(edit, FALSE);
	RP->Mask = 0; RPT.Mask = 0;
	selmask  = gui.selmask; gui.selmask = 0;
	txtmask  = gui.txtmask; gui.txtmask = 0;
	record   = 2;

	while( nb_times -- )
	{
		for(m = MainMacro; m; m = m->next)
		{
			STRPTR op, eoc;
			for(eoc = (op = m->data) + m->usage; op < eoc; op += SzOp[*op])
				switch( *op )
				{
					case MAC_ACT_ADD_CHAR:
						if( add_char(&edit->undo, edit->edited, edit->nbc, ((ActChar)op)->Char) )
						{
							REDRAW_CURLINE(edit);
							curs_right(edit, FALSE);
						}	break;
					case MAC_ACT_COM_MENU:
						msgbuf.Qualifier = ((ActMenu)op)->Qual;
						handle_menu( ((ActMenu)op)->Code ); break;
					case MAC_ACT_SHORTCUT:
						msgbuf.Qualifier = ((ActShortcut)op)->Qual;
						msgbuf.Code      = ((ActShortcut)op)->Code;
						handle_kbd(edit); break;
				}
		}
	}
	/* Now we can show changes */
	gui.selmask = selmask;
	gui.txtmask = txtmask;
	RP->Mask = (edit->ccp.select ? selmask : txtmask);
	RPT.Mask = 0xff;
	record   = 0;
	redraw_content(edit, edit->show, gui.topcurs, gui.nbline);
	prop_adj( edit );
	inv_curs(edit,TRUE);
}

void repeat_macro( Project p )
{
	LONG nb;

	if( MainMacro && get_number(p, GetMenuText(404), &nb) )
		play_macro( nb );
}

#if	0
/** Select other slot **/
void new_slot( BYTE dir )
{
	Macro *m = MacTable + CurInd - 1, *last = m;

	for(;;) {
		if(dir < 0) m--; else m++;
		if(m <  MacTable)         m = MacTable + MAX_MAC;
		if(m >= MacTable+MAX_MAC) m = MacTable;
		if(last == m || *m != NULL) break;
	}
	CurInd = m - MacTable + 1;

	ThrowError(Wnd, my_SPrintf(*m ? ErrMsg(WARN_SLOT) : ErrMsg(WARN_EMPTYSLOT), &CurInd));
}
#endif

