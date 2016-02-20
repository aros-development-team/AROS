#ifndef _ALIB_INTERN_H
#define _ALIB_INTERN_H

/*
    Copyright © 1995-2016, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef UTILITY_HOOKS_H
#   include <utility/hooks.h>
#endif
#ifndef AROS_SYSTEM_H
#   include <aros/system.h>
#endif
#ifndef AROS_ASMCALL_H
#   include <aros/asmcall.h>
#endif

#define STRLEN(s) \
({ \
    CONST_STRPTR _s = s; \
    while (*_s++ != '\0'); \
    _s - s - 1; \
})

#endif /* _ALIB_INTERN_H */
