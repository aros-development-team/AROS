#ifndef _LOCALE_H
#define _LOCALE_H

/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    This file is part of the LoadResource program, which is distributed under
    the terms of version 2 of the GNU General Public License.
*/

#include <exec/types.h>

#define CATCOMP_NUMBERS
#include "strings.h"

/*** Prototypes *************************************************************/
/* Main *********************************************************************/
STRPTR  _(ULONG ID);            /* Get a message, as a STRPTR */
#define __(id) ((IPTR) _(id))   /* Get a message, as an IPTR */

#endif /* _LOCALE_H */
