#ifndef _SUPPORT_H_
#define _SUPPORT_H_

/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id: support.h 20839 2004-02-08 01:45:21Z dlecorfec $
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Prototypes *************************************************************/
STRPTR GetENV(CONST_STRPTR name);
BOOL   SetENV(CONST_STRPTR name, CONST_STRPTR value);
VOID   ShowError(Object *application, Object *window, CONST_STRPTR message, BOOL useIOError);

#endif /* _SUPPORT_H_ */
