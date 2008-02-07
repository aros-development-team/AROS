#include <aros/kernel.h>
#include <aros/libcall.h>
#include <asm/amcc440.h>
#include <stddef.h>


AROS_LH4(void *, KrnAddIRQHandler,
         AROS_LHA(uint8_t, irq, D0),
         AROS_LHA(void *, handler, A0),
         AROS_LHA(void *, handlerData, A1),
         AROS_LHA(void *, handlerData2, A2),
         struct KernelBase *, KernelBase, 7, Kernel)
{
    AROS_LIBFUNC_INIT

    return NULL;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, KrnRemIRQHandler,
         AROS_LHA(void *, handle, A0),
         struct KernelBase *, KernelBase, 8, Kernel)
{
    AROS_LIBFUNC_INIT


    AROS_LIBFUNC_EXIT
}

AROS_LH0I(void, KrnCli,
         struct KernelBase *, KernelBase, 9, Kernel)
{
    AROS_LIBFUNC_INIT

    AROS_LIBFUNC_EXIT
}

AROS_LH0I(void, KrnSti,
         struct KernelBase *, KernelBase, 10, Kernel)
{
    AROS_LIBFUNC_INIT

    AROS_LIBFUNC_EXIT
}



static void __attribute__((used)) __EXCEPTION_Prolog_template()
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
     * Registers %r0 to %r5 are now saved together with CPU state. Go to the 
     * trampoline code which will care about the rest. Adjust the stack frame pointer now,
     * or else it will be destroyed later by C code.
     */
    asm volatile("addi %r1,%r3,-16");
    
    /*
     * Go to the trampoline code. Use long call within whole 4GB addresspace in order to
     * avoid any trouble in future.
     */
    asm volatile(
                 "lis %r5,__EXCEPTION_Trampoline@ha\n\t"
                 "la %r5,__EXCEPTION_Trampoline@l(%r5)\n\t"
                 "mtctr %r5\n\t"
                 "bctrl\n\t"
                 ".globl __EXCEPTION_HandlerPtr\n"
    "__EXCEPTION_HandlerPtr: .long 0"
            "");
    
    
    asm volatile(".size __EXCEPTION_Prolog, .-__EXCEPTION_Prolog\n\t"
                 ".globl __EXCEPTION_Prolog_size\n\t"
                 ".type  __EXCEPTION_Prolog_size,@object\n"
                 "__EXCEPTION_Prolog_size: .long . - __EXCEPTION_Prolog - 4");
}

static void __attribute__((used)) __EXCEPTION_Trampoline_template()
{
    asm volatile(".globl __EXCEPTION_Trampoline\n\t.type __EXCEPTION_Trampoline,@function\n"
        "__EXCEPTION_Trampoline:            \n\t"
                 "mflr %%r5             \n\t"   /* link register points to address of exception handler */
                 "stw %%r6,%[gpr6](%%r3) \n\t"
                 "stw %%r7,%[gpr7](%%r3) \n\t"
                 "stw %%r8,%[gpr8](%%r3) \n\t"
                 "stw %%r9,%[gpr9](%%r3) \n\t"
                 "stw %%r10,%[gpr10](%%r3) \n\t"
                 "stw %%r11,%[gpr11](%%r3) \n\t"
                 "stw %%r12,%[gpr12](%%r3) \n\t"
                 "stw %%r13,%[gpr13](%%r3) \n\t"
                 "stw %%r14,%[gpr14](%%r3) \n\t"
                 "stw %%r15,%[gpr15](%%r3) \n\t"
                 "stw %%r16,%[gpr16](%%r3) \n\t"
                 "stw %%r17,%[gpr17](%%r3) \n\t"
                 "stw %%r18,%[gpr18](%%r3) \n\t"
                 "stw %%r19,%[gpr19](%%r3) \n\t"
                 "stw %%r20,%[gpr20](%%r3) \n\t"
                 "stw %%r21,%[gpr21](%%r3) \n\t"
                 "stw %%r22,%[gpr22](%%r3) \n\t"
                 "stw %%r23,%[gpr23](%%r3) \n\t"
                 "stw %%r24,%[gpr24](%%r3) \n\t"
                 "stw %%r25,%[gpr25](%%r3) \n\t"
                 "stw %%r26,%[gpr26](%%r3) \n\t"
                 "stw %%r27,%[gpr27](%%r3) \n\t"
                 "stw %%r28,%[gpr28](%%r3) \n\t"
                 "stw %%r29,%[gpr29](%%r3) \n\t"
                 "stw %%r30,%[gpr30](%%r3) \n\t"
                 "stw %%r31,%[gpr31](%%r3) \n\t"
                 
                 "lwz %%r0,0(%%r5)      \n\t"   /* Load the address of exception handler into srr0 */
                 "mtsrr0 %%r0           \n\t"
                 
                 "andi. %%r5,%%r5,0x3f00 \n\t"   /* %r5 shall contain the exception number */
                 
                 "lis %%r9, %[msrval]@h  \n\t"
                 "ori %%r9,%%r9, %[msrval]@l \n\t"
                 "mtsrr1 %%r9"
                 ::
                 [gpr6]"i"(offsetof(regs_t, gpr[6])),
                 [gpr7]"i"(offsetof(regs_t, gpr[7])),
                 [gpr8]"i"(offsetof(regs_t, gpr[8])),
                 [gpr9]"i"(offsetof(regs_t, gpr[9])),
                 [gpr10]"i"(offsetof(regs_t, gpr[10])),
                 [gpr11]"i"(offsetof(regs_t, gpr[11])),
                 [gpr12]"i"(offsetof(regs_t, gpr[12])),
                 [gpr13]"i"(offsetof(regs_t, gpr[13])),
                 [gpr14]"i"(offsetof(regs_t, gpr[14])),
                 [gpr15]"i"(offsetof(regs_t, gpr[15])),
                 [gpr16]"i"(offsetof(regs_t, gpr[16])),
                 [gpr17]"i"(offsetof(regs_t, gpr[17])),
                 [gpr18]"i"(offsetof(regs_t, gpr[18])),
                 [gpr19]"i"(offsetof(regs_t, gpr[19])),
                 [gpr20]"i"(offsetof(regs_t, gpr[20])),
                 [gpr21]"i"(offsetof(regs_t, gpr[21])),
                 [gpr22]"i"(offsetof(regs_t, gpr[22])),
                 [gpr23]"i"(offsetof(regs_t, gpr[23])),
                 [gpr24]"i"(offsetof(regs_t, gpr[24])),
                 [gpr25]"i"(offsetof(regs_t, gpr[25])),
                 [gpr26]"i"(offsetof(regs_t, gpr[26])),
                 [gpr27]"i"(offsetof(regs_t, gpr[27])),
                 [gpr28]"i"(offsetof(regs_t, gpr[28])),
                 [gpr29]"i"(offsetof(regs_t, gpr[29])),
                 [gpr30]"i"(offsetof(regs_t, gpr[30])),
                 [gpr31]"i"(offsetof(regs_t, gpr[31])),

                 [msrval]"i"(MSR_EE | MSR_ME | MSR_DS | MSR_IS)
    );
#warning TODO: Check msrval!!!!!!!

    asm volatile(
                 "sync\n\t"
                 "rfi"
    );
                 
    asm volatile(
                 ".size __EXCEPTION_Trampoline, .-__EXCEPTION_Trampoline\n\t"
                 ".globl __EXCEPTION_Trampoline_size\n\t"
                 ".type  __EXCEPTION_Trampoline_size,@object\n"
                 "__EXCEPTION_Trampoline_size: .long . - __EXCEPTION_Trampoline - 4"
    );

}
