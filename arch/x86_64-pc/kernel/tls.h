#ifndef ASM_TLS_H
#define ASM_TLS_H

/*
    Copyright (C) 2017-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/config.h>

#if defined(__AROSEXEC_SMP__)
#include <kernel_scheduler.h>
#endif

#include "apic.h"

/*
 * The per-CPU block the GS base points at.
 *
 * On SMP the scheduler's per-CPU state is a member here rather than a
 * pointer to a separate allocation, so that every read and write of it is
 * a single %gs-relative instruction. An interrupt can only be taken
 * between instructions, so such an access always lands on the data of the
 * CPU the caller is running on. Through a pointer it took two accesses,
 * and a task moved to another CPU in between answered with the state of
 * the CPU it had left.
 */
typedef struct tls
{
    struct ExecBase             *SysBase;
    void                        *KernelBase;    /* Base of kernel.resource              */
#if defined(__AROSEXEC_SMP__)
    struct X86SchedulerPrivate  Sched;          /* This CPU's scheduler state           */
#endif
    apicid_t                    CPUNumber;
} tls_t;

#define TLSSF_Quantum   (1 << 0)
#define TLSSF_Switch    (1 << 1)
#define TLSSF_Dispatch  (1 << 2)

#define TLS_OFFSET(name) ((char *)&(((tls_t *)0)->name)-(char *)0)

#define TLS_GET(name) \
    ({ \
        tls_t *__tls = (tls_t *)0; \
        typeof(__tls -> name) __ret; \
        switch (sizeof(__tls -> name)) { \
case 1: asm volatile("movb %%gs:%P1,%0":"=q"(__ret):"n"(TLS_OFFSET(name)):"memory"); break; \
case 2: asm volatile("movw %%gs:%P1,%0":"=r"(__ret):"n"(TLS_OFFSET(name)):"memory"); break; \
case 4: asm volatile("movl %%gs:%P1,%0":"=r"(__ret):"n"(TLS_OFFSET(name)):"memory"); break; \
case 8: asm volatile("movq %%gs:%P1,%0":"=r"(__ret):"n"(TLS_OFFSET(name)):"memory"); break; \
        } \
        __ret;  \
    })

#define TLS_SET(name, val) \
    do { \
        tls_t *__tls = (tls_t *)0; \
        typeof(__tls -> name) __set = val; \
        switch (sizeof(__tls -> name)) { \
case 1: asm volatile("movb %0,%%gs:%P1"::"q"(__set),"n"(TLS_OFFSET(name)):"memory"); break; \
case 2: asm volatile("movw %0,%%gs:%P1"::"r"(__set),"n"(TLS_OFFSET(name)):"memory"); break; \
case 4: asm volatile("movl %0,%%gs:%P1"::"r"(__set),"n"(TLS_OFFSET(name)):"memory"); break; \
case 8: asm volatile("movq %0,%%gs:%P1"::"r"(__set),"n"(TLS_OFFSET(name)):"memory"); break; \
        } \
    } while(0);

/*
 * Read-modify-write of one per-CPU field, also in a single instruction.
 *
 * The unlocked forms are for fields only their own CPU ever touches: the
 * instruction cannot be split by an interrupt, which is all that is needed
 * when there is no other writer. The locked forms are for fields another
 * CPU writes as well - the heartbeat and the VBlank server set the
 * schedule flags of the CPU whose quantum has run out.
 */
#define TLS_INC_B(name) \
    asm volatile("incb %%gs:%P0"::"n"(TLS_OFFSET(name)):"memory","cc")
#define TLS_DEC_B(name) \
    asm volatile("decb %%gs:%P0"::"n"(TLS_OFFSET(name)):"memory","cc")
#define TLS_LOCK_OR_L(name, val) \
    asm volatile("lock orl %0,%%gs:%P1"::"ir"(val),"n"(TLS_OFFSET(name)):"memory","cc")
#define TLS_LOCK_AND_L(name, val) \
    asm volatile("lock andl %0,%%gs:%P1"::"ir"(val),"n"(TLS_OFFSET(name)):"memory","cc")

#endif
