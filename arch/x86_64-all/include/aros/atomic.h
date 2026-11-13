/* inherit i386 operations */
#include <aros/i386/atomic.h>

#define __AROS_ATOMIC_INC_Q(var) \
    __asm__ __volatile__ ("lock; incq %0" : "=m" ((var)) : "m" ((var)) : "memory", "cc")

#define __AROS_ATOMIC_DEC_Q(var) \
    __asm__ __volatile__ ("lock; decq %0" : "=m" ((var)) : "m" ((var)) : "memory", "cc")

#define __AROS_ATOMIC_AND_Q(var, mask) \
    __asm__ __volatile__ ("lock; andq %0,%1" : : "r" ((UQUAD)(mask)), "m" ((var)) : "memory", "cc")

#define __AROS_ATOMIC_OR_Q(var, mask) \
    __asm__ __volatile__ ("lock; orq %0,%1" : : "r" ((UQUAD)(mask)), "m" ((var)) : "memory", "cc")
