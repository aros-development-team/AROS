/**********************************************************
**                                                       **
**      $VER: Search.h 1.0 (5 aug 2000)                  **
**      Prototypes for searching through buffer          **
**                                                       **
**      © T.Pierron, C.Guilaume. Free software under     **
**      terms of GNU public license.                     **
**                                                       **
**********************************************************/

#ifndef	SEARCH_H
#define	SEARCH_H

/* Search through buffer for next (dir > 0) or previous (dir < 0) occurency **
** of registered pattern (or the one displayed in search window). Move the  **
** cursor to the line, and scroll display if necessary.                     */
void FindPattern( Project, BYTE dir );

/* Replace the pattern if word just under cursor correspond to the registe- **
** red pattern. If not search for next occurency, but replace nothing       */
void ReplacePattern( Project );

/* Replace all pattern of the file, starting from top. Cursor won't be moved */
void ReplaceAllPat( Project );

/* Search for next (dir > 0) or previous (dir > 0) occurency of word under **
** cursor. Use global word separator, and registered it as a new pattern   */
void FindWord( Project, BYTE dir );

/* Open search window, and init search string gadget with selected text if **
** any. If window were already opened, it is activated and pop to front    */
BYTE setup_winsearch( Project, UBYTE replace);

/* Ask user for a number of line into a new simple window and jump cursor to */
void goto_line( Project );

/* Move cursor and display to matching bracket under cursor */
void match_bracket( Project );

/* Close and free ressource associated to search window, registering (really **
** == TRUE) or not (really == FALSE), patterns in the edit areas             */
void close_searchwnd( BYTE really );

/* Init case insensitive table for searching faster than Stricmp() */
void init_searchtable( void );

/* Asynchronous events handler for search window */
void handle_search( void );

/* Messages for search/replace window */
extern STRPTR JanoMessages[];
#define	SWinTxt     (JanoMessages + (MSG_SEARCHWINDOW - ERR_BADOS))

#endif

