#ifndef _LOCALE_H
#define _LOCALE_H

/*
    Copyright © 2003-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>

#define CATCOMP_NUMBERS
#include "strings.h"

/*** Prototypes *************************************************************/
/* Main *********************************************************************/
CONST_STRPTR MSG(ULONG id);

/* Setup ********************************************************************/
VOID Locale_Initialize(VOID);
VOID Locale_Deinitialize(VOID);

#endif /* _LOCALE_H */
