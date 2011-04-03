#ifndef WBSTARTUP_H
#define WBSTARTUP_H 1

/*************************************************************************/

#include <workbench/startup.h>

/*************************************************************************/

extern struct WBStartup *wbmessage;

/*
** Prototypes
*/

void WBMessage_Get( void );
void WBMessage_Reply( void );

/*************************************************************************/

#endif /* WBSTARTUP_H */

