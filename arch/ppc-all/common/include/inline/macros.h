#ifndef INLINE_MACROS_H
#define INLINE_MACROS_H

/*
    Copyright C 2003, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Inline macros for function calls
    Lang: English
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef AROS_ABI_H
#include <aros/abi.h>
#endif

/*
    These are general inlines used to call library's functions. Macros are
    defined as follows:

    LPx - function that take X arguments.

    Modifiers (variations possible):
    NR - no return (void).
    A4, A5 - "a4" or "a5" is used as one of the arguments.
    UB - base will be given explicitly by user.
    FP - one of the parameters has type "pointer to function"
*/

#define LP0(offs, rt, name, bt, bn)					\
({									\
    rt _##name##_re;							\
    {									\
	(bt)__A6 = (bn);						\
	_##name##_re = ((rt (*)())__AROS_GETJUMPVEC(__A6,(offs)/6))();	\
	(rt)__D0 = _##name##_re;					\
    }									\
    _##name##_re;							\
})

#define LP0NR(offs, name, bt, bn)					\
({									\
    {									\
	(bt)__A6 = (bn);						\
	((rt (*)())__AROS_GETJUMPVEC(__A6,(offs)/6))();			\
    }									\
})



#endif /* INLINE_MACROS_H */

