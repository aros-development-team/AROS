#ifndef _STDC_STDBOOL_H_
#define _STDC_STDBOOL_H_

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Boolean type and values.

    C99 defines a new basic type _Bool, which is a boolean type.
    However we need to check whether it exists or not.
*/

#ifndef __cplusplus

/* People are allowed to define their own versions of these */
#undef bool
#undef true
#undef false
#undef __bool_true_false_are_defined

/* These are the C99 definitions */
#define bool				    _Bool
#define	true				    1
#define false				    0
#define __bool_true_false_are_defined       1

/* This is to make _Bool a real type if this isn't C99 or GCC v3+ */
#if __STDC_VERSION__ < 199901L && (!defined __GNUC__ || __GNUC__ < 3)
typedef int	_Bool;
#endif

#else /* __cplusplus */

#define _Bool	bool
#define bool	bool
#define false	false
#define true	true

#endif /* __cplusplus */


#endif /* _STDC_STDBOOL_H_ */
