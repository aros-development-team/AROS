/******************************************************
** Events.h : Standard datatypes and prototypes      **
** public port. Written by T.Pierron and C.Guillaume.**
** Free software under terms of GNU license.         **
******************************************************/


#ifndef	SEARCH_H
#define	SEARCH_H

void FindPattern( Project p, BYTE dir );
void ReplacePattern( Project p );
void ReplaceAllPat( Project p );
void FindWord( Project p, BYTE dir );
BYTE setup_winsearch(Project p, UBYTE replace);
void goto_line( Project p, STRPTR title );
void match_bracket( Project p );
void close_searchwnd(BYTE);
void init_searchtable( void );
void handle_search();

#endif
