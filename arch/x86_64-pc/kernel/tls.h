#ifndef ASM_TLS_H
#define ASM_TLS_H

typedef struct tls
{
    struct ExecBase     *SysBase;
    void *              *KernelBase;    /* Base of kernel.resource */
} tls_t;

#define TLS_OFFSET(name) ((char *)&(((tls_t *)0)->name)-(char *)0)

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
