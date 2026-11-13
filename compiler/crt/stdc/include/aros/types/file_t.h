#ifndef _AROS_TYPES_FILE_T_H
#define _AROS_TYPES_FILE_T_H

/*
    Copyright © 2026, The AROS Development Team. All rights reserved.
    $Id$

    FILE definition for AROS

    Both <stdio.h> and <wchar.h> have to declare the FILE type (C99 7.19.1
    and 7.24.1), but <wchar.h> must not drag in the rest of <stdio.h> - so
    the type lives here on its own.
*/

#ifndef _AROS_TYPES_FILE_S_H
struct __sFILE;
#endif
typedef struct __sFILE FILE;

#endif /* _AROS_TYPES_FILE_T_H */
