#ifndef CORE_H_
#define CORE_H_

#include <inttypes.h>
#include <exec/execbase.h>
#include <exec/tasks.h>

typedef struct tls {
    struct Task         *ThisTask;
    struct ExecBase     *SysBase;
} tls_t;

#define TLS_GET(name) \
    ({ \
        tls_t *__tls = (tls_t *)0; \
        typeof(__tls -> name) __ret; \
        switch (sizeof(__tls -> name)) { \
case 2: asm volatile("movw %%gs:%P1,%0":"=r"(__ret):"n"(&__tls -> name):"memory"); break; \
case 4: asm volatile("movl %%gs:%P1,%0":"=r"(__ret):"n"(&__tls -> name):"memory"); break; \
case 8: asm volatile("movq %%gs:%P1,%0":"=r"(__ret):"n"(&__tls -> name):"memory"); break; \
        } \
        __ret;  \
    })

#define TLS_SET(name, val) \
    do { \
        tls_t *__tls = (tls_t *)0; \
        typeof(__tls -> name) __set = val; \
        switch (sizeof(__tls -> name)) { \
case 2: asm volatile("movw %0,%%gs:%P1"::"r"(__set),"n"(&__tls -> name):"memory"); break; \
case 4: asm volatile("movl %0,%%gs:%P1"::"r"(__set),"n"(&__tls -> name):"memory"); break; \
case 8: asm volatile("movq %0,%%gs:%P1"::"r"(__set),"n"(&__tls -> name):"memory"); break; \
        } \
    } while(0);

extern void core_SetupIDT();
extern void core_SetupGDT();
extern void core_DefaultIRETQ();

typedef struct regs {
    uint64_t    ds;
    uint64_t    r15;
    uint64_t    r14;
    uint64_t    r13;
    uint64_t    r12;
    uint64_t    r11;
    uint64_t    r10;
    uint64_t    r9;
    uint64_t    r8;
    uint64_t    rcx;
    uint64_t    rdx;
    uint64_t    rsi;
    uint64_t    rdi;
    uint64_t    rbx;
    uint64_t    rbp;
    uint64_t    rax;
    uint64_t    irq_number;
    uint64_t    error_code;
    uint64_t    return_rip;
    uint64_t    return_cs;
    uint64_t    return_rflags;
    uint64_t    return_rsp;
    uint64_t    return_ss;
} regs_t;

#endif /*CORE_H_*/
