#ifndef _LOCALE_H
#define _LOCALE_H

/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>

#define CATCOMP_ARRAY
#include "clock_strings.h"

/*** Prototypes *************************************************************/
void InitLocale( STRPTR catname, ULONG version );
void CleanupLocale( void );
STRPTR MSG( ULONG id );

#endif /* _LOCALE_H */
