#ifndef AROS_AROSBASE_H
#define AROS_AROSBASE_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: aros.library general defines
    Lang: english
*/

#ifndef EXEC_LIBRARIES_H
#include <exec/libraries.h>
#endif

#define AROSLIBNAME "aros.library"

/* Minimum version that supports everything from the current includes. */
/* Will be bumped whenever new functions are added to the library.     */
#define AROSLIBVERSION 41
#define AROSLIBREVISION 1


#endif /* AROS_AROSBASE_H */
