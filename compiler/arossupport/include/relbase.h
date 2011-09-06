/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id:$

    Desc: Macros for storing library base on alternative stack
    Lang: english
*/

#ifndef AROS_RELBASE_H
#define AROS_RELBASE_H

#include <aros/system.h>

/* Declare relbase if asked */
#if !defined(AROS_GET_RELBASE) || !defined(AROS_SET_RELBASE)

__BEGIN_DECLS

void *aros_get_relbase(void);
void *aros_set_relbase(void *libbase);
void aros_push_relbase(void *libbase);
void aros_push2_relbase(void *libbase, void *ptr);
void *aros_pop_relbase(void);
void *aros_pop2_relbase(void);

__END_DECLS

/* Uncomment next define to get debug output for relbase calls
 */
//#define AROS_RELBASE_DEBUG 1

/* Also use debug version if we may not define global SysBase */
#if !defined(AROS_RELBASE_DEBUG) && !defined(__NOLIBBASE__) && !defined(__EXEC_NOLIBBASE__)

#include <exec/types.h>
#include <exec/execbase.h>

#include <aros/altstack.h>

extern struct ExecBase *SysBase;

static inline void *__aros_get_relbase_inline(void)
{
    return (void *)aros_get_altstack(SysBase->ThisTask);
}

static inline void *__aros_set_relbase_inline(void *libbase)
{
    return (void *)aros_set_altstack(SysBase->ThisTask, (IPTR)libbase);
}

static inline void __aros_push_relbase_inline(void *libbase)
{
    aros_push_altstack(SysBase->ThisTask, (IPTR)libbase);
}

static inline void *__aros_pop_relbase_inline(void)
{
    return (void *)aros_pop_altstack(SysBase->ThisTask);
}

#define AROS_GET_RELBASE        __aros_get_relbase_inline()
#define AROS_SET_RELBASE(x)     __aros_set_relbase_inline(x)
#define AROS_PUSH_RELBASE(x)    __aros_push_relbase_inline(x)
#define AROS_POP_RELBASE        __aros_pop_relbase_inline()

#else /* AROS_RELBASE_DEBUG || __NOLIBBASE__ || __EXEC_NOLIBBASE__ */

#define AROS_GET_RELBASE	aros_get_relbase()
#define AROS_SET_RELBASE(x)	aros_set_relbase(x)
#define AROS_PUSH_RELBASE(x)    aros_push_relbase(x)
#define AROS_POP_RELBASE        aros_pop_relbase()

#endif  /* !AROS_RELBASE_DEBUG && !__NOLIBBASE__ && !__EXEC_NOLIBBASE__ */

#endif /* !AROS_GET_RELBASE || !AROS_SET_RELBASE */

/* If AROS_GET_LIBBASE/AROS_SET_LIBBASE use relbase by defining them as resp.
 * AROS_GET_RELBASE/AROS_SET_RELBASE
 */
#ifndef AROS_GET_LIBBASE
#define AROS_GET_LIBBASE AROS_GET_RELBASE
#define AROS_SET_LIBBASE(x) AROS_SET_RELBASE(x)
#endif

#endif
