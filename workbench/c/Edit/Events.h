/******************************************************
** Events.h : Standard datatypes and prototypes      **
** public port. Written by T.Pierron and C.Guillaume.**
** Free software under terms of GNU license.         **
******************************************************/


#ifndef	EVENTS_H
#define	EVENTS_H

#ifndef	RAWKEY_H
#include	"Rawkey.h"
#endif

#ifndef UTILITY_H
#include	"Utility.h"
#endif

/** Jano public port management **/
ULONG create_port ( void );					/* Create Jano public port */
void  close_port  ( void );					/* Shutdown port */

/** Message-type processing **/
void  handle_port ( void );
void  handle_menu ( LONG MenuID );
void  handle_kbd  ( Project );

char send_pref(PREFS *prefs, ULONG class);
char find_janoed( StartUpArgs *args );

#endif
