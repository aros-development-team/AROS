#include <stdint.h>
#include <exec/types.h>

#define BNULL (ULONG)(NULL)
typedef uintptr_t IPTR;

typedef void (*VOID_FUNC)(void);

/* Special hack for setting the 'Z' condition code upon exit
 * for m68k architectures.
 */
#define AROS_INTFUNC_INIT inline ULONG _handler(void) {
#define AROS_INTFUNC_EXIT }; register ULONG _res asm ("d0") = _handler();     \
                             asm volatile ("tst.l %0\n" : : "r" (_res)); \
                             return _res; /* gcc only generates movem/unlk/rts */   \
                             AROS_USERFUNC_EXIT }

#ifndef AROS_USERFUNC_INIT
#   define AROS_USERFUNC_INIT {
#endif
#ifndef AROS_USERFUNC_EXIT
#   define AROS_USERFUNC_EXIT }}
#endif


#ifndef __AROS_UFH_PREFIX
#define __AROS_UFH_PREFIX   /* eps */
#endif

#ifndef __AROS_UFHA
#define __AROS_UFHA(type,name,reg)    type name
#endif
#define AROS_UFHA(type,name,reg)    type,name,reg

#define AROS_UFH5(t,n,a1,a2,a3,a4,a5) \
    __AROS_UFH_PREFIX t n (\
    __AROS_UFHA(a1),\
    __AROS_UFHA(a2),\
    __AROS_UFHA(a3),\
    __AROS_UFHA(a4),\
    __AROS_UFHA(a5)\
    ) {

#define AROS_INTH4(n, type, data, intmask, custom, code)   \
        AROS_UFH5(ULONG, n,                                \
          AROS_UFHA(APTR,      __ufi_data, A1),            \
          AROS_UFHA(ULONG,     intmask,    D1),            \
          AROS_UFHA(APTR,      custom,     A0),            \
          AROS_UFHA(VOID_FUNC, code,       A5),            \
          AROS_UFHA(struct ExecBase *, SysBase, A6)        \
        ) { AROS_USERFUNC_INIT                             \
            type __unused data = __ufi_data;

#define AROS_INTH1(n, type, data)                  AROS_INTH4(n, type, data, __ufi_intmask, __ufi_custom, __ufi_code)

