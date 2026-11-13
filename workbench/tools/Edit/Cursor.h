/**************************************************************
**** Cursor.h : Cursor movement related functions          ****
**** Free software under GNU license, started on 15/2/2000 ****
**** © T.Pierron, C.Guillaume.                             ****
**************************************************************/

#ifndef CURSOR_H
#define CURSOR_H

#ifndef	PROJECT_H
#include	"Project.h"
#endif

void inv_curs      (Project, BYTE state);		/* Show/Hide cursor */
void join_strip    (Project);						/* Join & strip \s on next line */
void move_to_line  (Project, ULONG, char);	/* Move to a specified line number */
void curs_down     (Project);						/* Move cursor one line down */
void curs_up       (Project);						/* Move one line up */
void curs_left     (Project, BYTE word);		/* Move cursor one character left */
void curs_right    (Project, BYTE word);		/* Move one character right */
void jump_horiz    (Project, BYTE dir);		/* Jump page by page horizontally */
void jump_vert     (Project, BYTE dir);		/* Jump page by page vertically */
void horiz_pos     (Project, ULONG);			/* Jump to an absolute pos in line */
void pg_updown     (Project, BYTE dir);		/* Jump one page without moving curs */
void split_curline (Project);						/* Handle return key */
void join_strip    (Project);						/* Ctrl + J */
void amiga_k       (Project);						/* Remove entire line */
void back_space    (Project, BYTE word);		/* BS key */
void cut_line      (Project, BYTE mode);		/* Remove part (end or beg.) of line */
void del           (Project, BYTE word);		/* Del key */

void set_cursor_line (Project p, LONG  pos, ULONG top);
void set_top_line    (Project p, ULONG top, ULONG left);
void move_cursor     (Project p, LONG  dx,  LONG  dy);

/** Indirect cursor movement **/
LONG  forward_word  (LINE *ln, ULONG pos);
LONG  backward_word (LINE *ln, ULONG pos);
ULONG x2pos         (LINE *, ULONG);
ULONG adjust_rc     (LINE *, ULONG, ULONG *, UBYTE);
ULONG find_nbc      (LINE *, ULONG);

LONG adjust_leftpos(Project p, WORD step);
void click(Project p, WORD x, WORD y, BYTE update);
void unclick(Project p);

/** Parameter for horiz_pos **/
#define	MAXPOS				0x7fff

/** Values for char parameter of move_to_line() **/
#define	LINE_AS_IS			0				/* Display line to jump to as is */
#define	LINE_DIRTY			1				/* The line we want to jump is modified */
#define	LINES_DIRTY			2				/* Lines following the one we want to jump are modified */

/** Selection **/
typedef BYTE (*pfnSelectFunc)(Project, LONG, LONG);

extern pfnSelectFunc move_selection;
extern pfnSelectFunc SwitchSelect          (Project, BYTE mode, BYTE force);
extern BYTE          move_column_selection (Project, LONG, LONG);
extern BYTE          move_stream_selection (Project, LONG, LONG);

#endif
