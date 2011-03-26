#ifndef _AROS_TYPES_WINT_T_H
#define _AROS_TYPES_WINT_T_H

/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef __GNUC__
typedef int wint_t;
#else
#define __need_wint_t
#include <stddef.h>
#endif

#endif /* _AROS_TYPES_WINT_T_H */
