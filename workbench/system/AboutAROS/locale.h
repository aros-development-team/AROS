#ifndef _LOCALE_H
#define _LOCALE_H

/*
    Copyright © 2003-2004, The AROS Development Team. All rights reserved.
    This file is part of the About program, which is distributed under
    the terms of version 2 of the GNU General Public License.
    
    $Id$
*/

#include <exec/types.h>

#define CATCOMP_NUMBERS
#include "strings.h"

/*** Message aliases ********************************************************/
#if AROS_BUILD_TYPE == AROS_BUILD_TYPE_PERSONAL
#   define MSG_BUILD_TYPE MSG_PERSONAL_BUILD
#elif AROS_BUILD_TYPE == AROS_BUILD_TYPE_NIGHTLY
#   define MSG_BUILD_TYPE MSG_NIGHTLY_BUILD
#elif AROS_BUILD_TYPE == AROS_BUILD_TYPE_SNAPSHOT
#   define MSG_BUILD_TYPE MSG_SNAPSHOT
#elif AROS_BUILD_TYPE == AROS_BUILD_TYPE_MILESTONE
#   define MSG_BUILD_TYPE MSG_MILESTONE
#elif AROS_BUILD_TYPE == AROS_BUILD_TYPE_RELEASE
#   define MSG_BUILD_TYPE MSG_RELEASE
#else
#   error Unknown AROS_BUILD_TYPE
#endif

/*** Prototypes *************************************************************/
/* Main *********************************************************************/
STRPTR  _(ULONG ID);            /* Get a message, as a STRPTR */
#define __(id) ((IPTR) _(id))   /* Get a message, as an IPTR */

/* Setup ********************************************************************/
BOOL Locale_Initialize(void);
void Locale_Deinitialize(void);

#endif /* _LOCALE_H */
