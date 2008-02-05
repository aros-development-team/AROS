#include <asm/amcc440.h>
#include <stddef.h>

static void __attribute__((used)) __intr_template()
{
    asm volatile(".globl __EXCEPTION_Prolog\n\t.type __EXCEPTION_Prolog,@function\n"
        "__EXCEPTION_Prolog:            \n\t"
                 "mtsprg1 %%r3          \n\t"   /* save %r3 */
                 "mfcr %%r3             \n\t"   /* copy CR to %r3 */ 
                 "mtsprg2 %%r3          \n\t"   /* save %r3 */

                 "mfsrr1 %%r3           \n\t"   /* srr1 (previous MSR) reg into %r3 */
                 "andi. %%r3,%%r3,%0    \n\t"   /* Was the PR bit set in MSR already? */
                 "beq- 1f               \n\t"   /* Yes, we were in supervisor mode */
                 
                 "mfsprg0 %%r3          \n\t"   /* user mode case: SSP into %r3 */
                 "b 2f                  \n"
        "1:       mr %%r3,%%r1          \n"     /* Supervisor case: use current stack */
        "2:       addi %%r3,%%r3,%1     "       
                 ::"i"(MSR_PR),"i"(-sizeof(regs_t)));
    
    /*
     * Create partial context on the stack. It is impossible to save it fully since the
     * exception handlers have limited size. This code will do as much as possible and then
     * jump to general trampoline... 
     */
    asm volatile("stw %%r0, %[gpr0](%%r3)  \n\t"
                 "stw %%r1, %[gpr1](%%r3)  \n\t"
                 "stw %%r2, %[gpr2](%%r3)  \n\t"
                 "mfsprg1 %%r0             \n\t"
                 "stw %%r4, %[gpr4](%%r3)  \n\t"
                 "stw %%r0, %[gpr3](%%r3)  \n\t"
                 "stw %%r5, %[gpr5](%%r3)  \n\t"
                 "mfsprg2 %%r2             \n\t"
                 "mfsrr0 %%r0              \n\t"
                 "mfsrr1 %%r1              \n\t"
                 "stw %%r2,%[ccr](%%r3)    \n\t"
                 "stw %%r0,%[srr0](%%r3)   \n\t"
                 "stw %%r1,%[srr1](%%r3)   \n\t"
                 "mfctr %%r0               \n\t"
                 "mflr %%r1                \n\t"
                 "mfxer %%r2               \n\t"           
                 "stw %%r0,%[ctr](%%r3)    \n\t"
                 "stw %%r1,%[lr](%%r3)     \n\t"
                 "stw %%r2,%[xer](%%r3)    \n\t"

                 ::
                 [gpr0]"i"(offsetof(regs_t, gpr[0])),
                 [gpr1]"i"(offsetof(regs_t, gpr[1])),
                 [gpr2]"i"(offsetof(regs_t, gpr[2])),
                 [gpr3]"i"(offsetof(regs_t, gpr[3])),
                 [gpr4]"i"(offsetof(regs_t, gpr[4])),
                 [gpr5]"i"(offsetof(regs_t, gpr[5])),
                 
                 [ccr]"i"(offsetof(regs_t, ccr)),
                 [srr0]"i"(offsetof(regs_t, srr0)),
                 [srr1]"i"(offsetof(regs_t, srr1)),
                 [ctr]"i"(offsetof(regs_t, ctr)),
                 [lr]"i"(offsetof(regs_t, lr)),
                 [xer]"i"(offsetof(regs_t, xer))
                 );
    /*
     * Registers %r0 to %r5 are now saved together with CPU state.
     */
    
    
    asm volatile(".size __EXCEPTION_Prolog, .-__EXCEPTION_Prolog\n\t"
                 ".globl __EXCEPTION_Prolog_size\n\t"
                 ".type  __EXCEPTION_Prolog_size,@object\n"
                 "__EXCEPTION_Prolog_size: .long . - __EXCEPTION_Prolog - 4");
}
