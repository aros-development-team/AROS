/**********************************************************
**                                                       **
**      $VER: Project.h v1.3 (16.2.2000)                 **
**      Data-types for handling multi-project window     **
**                                                       **
**      © T.Pierron, C.Guilaume. Free software under     **
**      terms of GNU public license.                     **
**                                                       **
**********************************************************/

#ifndef	PROJECT_H
#define	PROJECT_H

typedef	struct _project *				Project;

#ifndef	MEMORY_H
#include "Memory.h"
#endif
#ifndef	PREFS_H
#include "Prefs.h"
#endif
#ifndef	CLIPLOC_H
#include "ClipLoc.h"
#endif

#include "UndoRedo.h"

typedef	struct _project				/* MAIN PROJECT STRUCTURE */
{
	struct _project *next,*prev;		/* Linked list of opened project */
	struct  cutcopypast ccp;			/* The lines selected */
	STRPTR  buffer;						/* Main file buffer */
	ULONG   date[3];						/* Last modification time */
	char    tabsize;						/* Local tabulation size */
	char    cursmode;						/* Cursor movement mode */
	char    syntax;						/* Local syntax mode */
	unsigned char    eol;							/* End of line type, see below */

	UWORD   xcurs, ycurs;				/* Screen position of cursor */
	ULONG   nbc,nbrc, nbl;				/* Nb. char where cursor is and real column */
	ULONG   nbrwc;							/* Nb. of real wanted column */

	WORD    pleft, pwidth;				/* Panel tab information */
	WORD    labwid, labsize;			/* Label width in pixels and size in bytes */
	WORD    modwid;			/* Modification-flag width in pixels */

	STRPTR  path, name;					/* Access path */
	LONG    protection;					/* protection flags */
	UBYTE   state;							/* See below */
	LINE   *show,*the_line;				/* First shown & first line */
	LINE   *edited;						/* Line being edited */
	ULONG   top_line;						/* Number of first line displayed */
	ULONG   left_pos;						/* Number of first real column displayed */
	ULONG   max_lines;					/* Number of total lines */

   JBUF    undo, redo;					/* Journalized buffers */
   STRPTR  savepoint;					/* Last modification saved */

}	PROJECT;

/** State of a project **/
#define	MODIFIED				1			/* Don't close it without asking user */
#define	PAGINATED			2			/* Remain on disk (reduce disk usage) */
#define	DONT_FLUSH			4			/* Do not flush redo log */
#define	DUPLICATE			8			/* Do not free edit buffer */

/** String added to project when modified **/
#define	STR_MODIF			"+"

/** End of line type **/
#define	AMIGA_EOL			0
#define	MACOS_EOL			1
#define	MSDOS_EOL			2

Project close_projects    ( void );								/* Try to close all projects */
Project new_project       (Project, PREFS *);				/* Alloc a new empty project */
Project save_projects     (Project active,  char close);	/* Save all modified projects */
Project select_panel      (Project current, WORD x);		/* Search for panel under position x */
Project create_projects   (Project, APTR, ULONG);			/* Create a list of projects */
Project load_and_activate (Project, STRPTR name, BYTE);	/* Load and create a project */
Project load_and_activate_fr(Project, APTR, BYTE);
WORD    load_in_project   (Project, STRPTR file);			/* Try to a load a file */
void    reload_project    (Project);							/* Load project and flush changes */
char    active_project    (Project, char);					/* Makes specified project, the active one */
char    close_project     (Project);							/* Try to close a project */
char    save_project      (Project, char refresh, char);	/* Save one projet, asking for filename */
void    set_project_name  (Project, STRPTR path);			/* Set path of project */
char    print_project     (Project active);	/* Print one project */
void    change_project    (Project, LINE *);					/* Change content of a project (no redraw) */
void    reshape_panel     (Project);							/* Resize item's project bar */
void    set_modif_mark    (Project);							/* Set and show modification flag */
void    unset_modif_mark  (Project, char showmodif);		/* Unset modification flag */
void    update_panel_name (Project);							/* Change name of ectiv project */
void    insert_file       (Project);

/** Special values for select_panel **/
#define	NEXT_PROJECT				 0x7FFF
#define	PREV_PROJECT				-0x7FFF

/** Read only! **/
extern UBYTE NbProject;

#define	REDRAW_CURLINE(prj)		\
	{ Move(RP,gui.left,prj->ycurs); write_text(prj, prj->edited); }

#endif
