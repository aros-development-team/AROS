#ifndef UTILITY_UTILITY_H
#define UTILITY_UTILITY_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Definitions for utility.library
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif

#define UTILITYNAME	"utility.library"

struct UtilityBase
{
    struct Library ub_LibNode;
    UBYTE          ub_Language;
    UBYTE          ub_Reserved;
};

#endif /* UTILITY_UTILITY_H */
