/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_TYPECHECK_H_
#define _LINUX_TYPECHECK_H_

#define typecheck(type, x) ({ type __dummy; typeof(x) __dummy2; (void)(&__dummy == &__dummy2); 1; })
#define typecheck_fn(type, function) ({ typeof(type) __tmp = function; (void)__tmp; })

#endif /* _LINUX_TYPECHECK_H_ */
