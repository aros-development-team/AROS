#ifndef CLIB_SCREENNOTIFY_PROTOS_H
#define CLIB_SCREENNOTIFY_PROTOS_H


/*
**	$VER: screennotify_protos.h 1.0 (27.04.2010)
**
**	C prototypes. For use with 32 bit integers only.
**
**	Copyright © 2010 
**	All Rights Reserved
*/

#ifndef  EXEC_TYPES_H
#include <exec/types.h>
#endif

APTR AddCloseScreenClient(struct Screen * screen, struct MsgPort * port, LONG pri);
BOOL RemCloseScreenClient(APTR handle);
APTR AddPubScreenClient(struct MsgPort * port, LONG pri);
BOOL RemPubScreenClient(APTR handle);
APTR AddWorkbenchClient(struct MsgPort * port, LONG pri);
BOOL RemWorkbenchClient(APTR handle);

#endif	/*  CLIB_SCREENNOTIFY_PROTOS_H  */
