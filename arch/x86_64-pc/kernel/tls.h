#ifndef ASM_TLS_H
#define ASM_TLS_H

#include <aros/config.h>

#if defined(__AROSEXEC_SMP__)
#include <kernel_scheduler.h>
#endif

typedef struct tls
{
    struct tls                  *_self;
    struct ExecBase             *SysBase;
    void                        *KernelBase;    /* Base of kernel.resource              */
#if defined(__AROSEXEC_SMP__)
    struct X86SchedulerPrivate  *ScheduleData;
#endif
} tls_t;

#define TLSSF_Quantum   (1 << 0)
#define TLSSF_Switch    (1 << 1)
#define TLSSF_Dispatch  (1 << 2)

#define TLS_OFFSET(name) ((char *)&(((tls_t *)0)->name)-(char *)0)

#define TLS_PTR_GET() \
    ({ \
        tls_t *__tls; \
        asm volatile("movq %%gs:0,%0":"=r"(__tls)::"memory"); \
        __tls;  \
    })

#define TLS_GET(name) \
    ({ \
        tls_t *__tls = (tls_t *)0; \
        typeof(__tls -> name) __ret; \
        switch (sizeof(__tls -> name)) { \
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
case 2: asm volatile("movw %0,%%gs:%P1"::"r"(__set),"n"(TLS_OFFSET(name)):"memory"); break; \
case 4: asm volatile("movl %0,%%gs:%P1"::"r"(__set),"n"(TLS_OFFSET(name)):"memory"); break; \
case 8: asm volatile("movq %0,%%gs:%P1"::"r"(__set),"n"(TLS_OFFSET(name)):"memory"); break; \
        } \
    } while(0);

#endif
