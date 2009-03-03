#ifndef _SUPPORT_H_
#define _SUPPORT_H_

/*
    Copyright � 2005-2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Prototypes *************************************************************/
STRPTR GetENV(CONST_STRPTR name);
BOOL   SetENV(CONST_STRPTR name, CONST_STRPTR value);
VOID   ShowError(Object *application, Object *window, CONST_STRPTR message, BOOL useIOError);
ULONG   FormatSize(STRPTR buffer, ULONG blocks, ULONG totalblocks, ULONG bytesperblock, BOOL showPercentage);

#endif /* _SUPPORT_H_ */
