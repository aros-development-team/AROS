/*
 * This file is a W.I.P. global KernelBase removal.
 * Please use this inlined function to access global KernelBase in all new code.
 * When done, global KernelBase variable will be not accessible any more.
 * This file can be overriden on a per-architecture basis, to store KernelBase in,
 * for example, CPU system register.
 */
static inline struct KernelBase *getKernelBase(void)
{
    return KernelBase;
}
