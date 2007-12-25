#ifndef DOS_BPTR_H
#define DOS_BPTR_H

/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AROS version of BPTRs
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif


#ifndef AROS_BPTR_TYPE
#   ifdef AROS_FAST_BPTR
#       define AROS_BPTR_TYPE   APTR
#   else
#       define AROS_BPTR_TYPE   IPTR
#   endif
#endif
#ifndef AROS_BSTR_TYPE
#   ifdef AROS_FAST_BSTR
#       define AROS_BSTR_TYPE   STRPTR
#   else
#       define AROS_BSTR_TYPE   IPTR
#   endif
#endif

typedef AROS_BPTR_TYPE BPTR;
typedef AROS_BSTR_TYPE BSTR;

/* In the BCPL language memory is addressed in chunks of long (e.g. 32 bit) and also
 * the BPTRs on used by DOS on amigaos used this addressing. This is contrary to byte
 * (e.g. 8 bit) addressing as is common now for most architectures.
 * 
 * AROS can be configured to have BPTRs also use byte addressing by setting the
 * AROS_FAST_BPTR preprocessor symbol in the cpu.h file.
 * For source code compatibility AROS__FAST_BPTR should only be used for non-standard
 * AROS implementations.
 * 
 * MKBADDR and BADDR macros are defined to access BPTRs in an implementation
 * independent way.
*/
#ifdef AROS_FAST_BPTR
#   define MKBADDR(a)               ((BPTR)(a))
#   define BADDR(a)                 ((APTR)a)
#else
#   define MKBADDR(a)               ((BPTR)(((IPTR)a)>>2))
#   define BADDR(a)                 ((APTR)(((IPTR)a)<<2))
#endif

/* BCPL strings used the first byte as the length of the string followed by the string.
 * Strings then also had a maximum length of 255. The normal C convention is to have
 * strings of any length but ended with a byte value of 0
 * 
 * AROS can be configured to have BSTRs implemented as C strings by setting the
 * AROS_FAST_BSTR preprocessor symbol in the cpu.h file.
 * For source code compatibility AROS_FAST_BSTR should only be used for non-standard
 * AROS implementations.
 * 
 * The AROS_BSTR_ADDR, AROS_BSTR_strlen, AROS_BSTR_setstrlen and AROS_BSTR_MEMSIZE4LEN
 * preprocessor macros are provided to work with BSTRs in an implementation independent
 * way.
 */
#ifdef AROS_FAST_BSTR
#   define AROS_BSTR_ADDR(s)        ((STRPTR)BADDR(s))
#   define AROS_BSTR_strlen(s)      (strlen(AROS_BSTR_ADDR(s)))
#   define AROS_BSTR_setstrlen(s,l) (AROS_BSTR_ADDR(s)[l] = 0)
#   define AROS_BSTR_MEMSIZE4LEN(l) ((l)+1)
#else
#   define AROS_BSTR_ADDR(s)        (((STRPTR)BADDR(s))+1)
#   define AROS_BSTR_strlen(s)      (AROS_BSTR_ADDR(s)[-1])
#   define AROS_BSTR_setstrlen(s,l) do { \
    STRPTR _s = AROS_BSTR_ADDR(s); \
    _s[-1] = l; \
    _s[l]=0; \
} while(0)
#   define AROS_BSTR_MEMSIZE4LEN(l) ((l)+2)
#endif
#define AROS_BSTR_getchar(s,l)   (AROS_BSTR_ADDR(s)[l])
#define AROS_BSTR_putchar(s,l,c) (AROS_BSTR_ADDR(s)[l] = c)

#endif /* DOS_BPTR_H */
