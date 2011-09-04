/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef AROS_ALTSTACK_H
#define AROS_ALTSTACK_H

/******************************************************************************

    MODUL
	$Id$

    DESCRIPTION
        In this file an AROS extension is given for an alternative stack.
        This stack can be used in stub function without interfering will
        function argument passing.

    SEE ALSO
        aros_init_altstack(), aros_set_altstack(), aros_get_altstack(),
        aros_push_altstack(), aros_pop_altstack()

******************************************************************************/

#include <aros/system.h>
#include <aros/macros.h>
#include <exec/tasks.h>

#define AROS_ALTSTACK_ID ((IPTR)AROS_MAKE_ID('A','S','T','K'))

__BEGIN_DECLS

void aros_init_altstack(struct Task *);
IPTR aros_set_altstack(struct Task *, IPTR);
IPTR aros_get_altstack(struct Task *);
void aros_push_altstack(struct Task *, IPTR);
IPTR aros_pop_altstack(struct Task *);

__END_DECLS

#ifndef AROS_ALTSTACK_DEBUG

/* Remark: inline altstack implementation has to be kept in sync
   with non inline implementation in arossupport/altstack.c
*/
static inline void __aros_init_altstack_inline(struct Task *t)
{
    IPTR *stackarray = (IPTR *)t->tc_SPLower;

    stackarray[0] = (IPTR)&stackarray[2];
    stackarray[1] = AROS_ALTSTACK_ID;
}

static inline IPTR __aros_set_altstack_inline(struct Task *t, IPTR value)
{
    IPTR *stacktop = *((IPTR **)t->tc_SPLower);
    IPTR ret = *stacktop;

    *stacktop = value;

    return ret;
}

static inline IPTR __aros_get_altstack_inline(struct Task *t)
{
    return **((IPTR **)t->tc_SPLower);
}

static inline void __aros_push_altstack_inline(struct Task *t, IPTR value)
{
    IPTR **stackptr = (IPTR **)t->tc_SPLower;
    
    *(++(*stackptr)) = value;
}

static inline IPTR __aros_pop_altstack_inline(struct Task *t)
{
    IPTR **stackptr = (IPTR **)t->tc_SPLower;
    
    return *((*stackptr)--);
}

#ifndef __AROS_ALTSTACK_NODEFINE__
#define aros_init_altstack(t) __aros_init_altstack_inline(t)
#define aros_set_altstack(t,v) __aros_set_altstack_inline(t,v)
#define aros_get_altstack(t) __aros_get_altstack_inline(t)
#define aros_push_altstack(t,v) __aros_push_altstack_inline(t,v)
#define aros_altstacl_pop(t) __aros_pop_altstack(t)
#endif /* !__AROS_ALTSTACK_NODFINE__ */

#endif /* !AROS_ALTSTACK_DEBUG */

#endif /* AROS_ALTSTACK_H */
