/* lock op is supposed to make these ops atomic and in result SMP safe 
Atomic addition of an immediate value to a memory location. 
*/

#define __AROS_ATOMIC_INC_B(var) \
    __asm__ __volatile__ ("lock; incb %0" : "=m" ((var)) : "m" ((var)) : "memory", "cc")
#define __AROS_ATOMIC_INC_W(var) \
    __asm__ __volatile__ ("lock; incw %0" : "=m" ((var)) : "m" ((var)) : "memory", "cc")
#define __AROS_ATOMIC_INC_L(var) \
    __asm__ __volatile__ ("lock; incl %0" : "=m" ((var)) : "m" ((var)) : "memory", "cc")

#define __AROS_ATOMIC_DEC_B(var) \
    __asm__ __volatile__ ("lock; decb %0" : "=m" ((var)) : "m" ((var)) : "memory", "cc")
#define __AROS_ATOMIC_DEC_W(var) \
    __asm__ __volatile__ ("lock; decw %0" : "=m" ((var)) : "m" ((var)) : "memory", "cc")
#define __AROS_ATOMIC_DEC_L(var) \
    __asm__ __volatile__ ("lock; decl %0" : "=m" ((var)) : "m" ((var)) : "memory", "cc")

#define __AROS_ATOMIC_AND_B(var, mask) \
    __asm__ __volatile__ ("lock; andb %0,%1" : : "r" ((UBYTE)(mask)), "m" ((var)) : "memory", "cc")
#define __AROS_ATOMIC_AND_W(var, mask) \
    __asm__ __volatile__ ("lock; andw %0,%1" : : "r" ((UWORD)(mask)), "m" ((var)) : "memory", "cc")
#define __AROS_ATOMIC_AND_L(var, mask) \
    __asm__ __volatile__ ("lock; andl %0,%1" : : "r" ((ULONG)(mask)), "m" ((var)) : "memory", "cc")

#define __AROS_ATOMIC_OR_B(var, mask) \
    __asm__ __volatile__ ("lock; orb %0,%1" : : "r" ((UBYTE)(mask)), "m" ((var)) : "memory", "cc")
#define __AROS_ATOMIC_OR_W(var, mask) \
    __asm__ __volatile__ ("lock; orw %0,%1" : : "r" ((UWORD)(mask)), "m" ((var)) : "memory", "cc")
#define __AROS_ATOMIC_OR_L(var, mask) \
    __asm__ __volatile__ ("lock; orl %0,%1" : : "r" ((ULONG)(mask)), "m" ((var)) : "memory", "cc")

