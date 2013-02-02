#ifndef _LOCALE_H
#define _LOCALE_H

/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id: locale.h 41348 2011-09-14 20:21:00Z sonic $
*/

#include <exec/types.h>

#define CATCOMP_NUMBERS
#include "strings.h"

/*** Prototypes *************************************************************/
/* Main *********************************************************************/
STRPTR  _(ULONG ID);            /* Get a message, as a STRPTR */
#define __(id) ((IPTR) _(id))   /* Get a message, as an IPTR */

/* Setup ********************************************************************/
BOOL Locale_Initialize(void);
void Locale_Deinitialize(void);

#endif /* _LOCALE_H */
