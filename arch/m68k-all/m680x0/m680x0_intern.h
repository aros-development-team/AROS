/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id: m680x0_intern.h$

    Desc: 68040/060 missing instruction emulation support code
    Lang: english
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef EXEC_LIBRARIES_H
#include <exec/libraries.h>
#endif

struct M680x0Base
{
    struct Library pb_LibNode;
};
