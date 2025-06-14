#ifndef _AROS_TYPES_MBSTATE_T_H
#define _AROS_TYPES_MBSTATE_T_H

/*
    Copyright © 2010-2025, The AROS Development Team. All rights reserved.
    $Id$
*/

typedef struct {
    int __state;   // internal use
    // or platform-specific state fields
} mbstate_t;

#endif /* _AROS_TYPES_MBSTATE_T_H */
