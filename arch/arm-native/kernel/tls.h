#ifndef ASM_TLS_H
#define ASM_TLS_H

typedef struct tls
{
    struct ExecBase     *SysBase;
    void *              *KernelBase;    /* Base of kernel.resource */
    struct Task         *ThisTask;      /* Currently running task on this core */
} tls_t;

#define TLS_OFFSET(name) ((char *)&(((tls_t *)0)->name)-(char *)0)

////

#define TLS_GET(name) \
    ({ \
        tls_t *__tls; \
        asm volatile("mrc p15, 0, %0, c13, c0, 3":"=r"(__tls)); \
        typeof(__tls -> name) __ret = (__tls -> name); \
        __ret;  \
    })

#define TLS_SET(name, val) \
    do { \
        tls_t *__tls; \
        asm volatile("mrc p15, 0, %0, c13, c0, 3":"=r"(__tls)); \
        (__tls -> name) = val; \
    } while(0);

#endif
