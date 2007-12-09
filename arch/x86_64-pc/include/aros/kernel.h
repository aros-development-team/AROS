#ifndef AROS_KERNEL_H
#define AROS_KERNEL_H

#ifndef BOOTSTRAP

#include <inttypes.h>
#include <utility/tagitem.h>

struct KernelBSS {
    uint64_t    addr;
    uint64_t    len;
};

#else

struct TagItem{
    unsigned long long ti_Tag;
    unsigned long long ti_Data;
};

#define TAG_DONE        0x00000000ULL
#define TAG_USER        0x80000000ULL

#endif

typedef enum {
    SCHED_RR = 1
} KRN_SchedType;

typedef struct tls {
    struct Task         *ThisTask;
    struct ExecBase     *SysBase;
    void *              *KernelBase;    /* Base of kernel.resource */
} tls_t;

#define TLS_OFFSET(name) (((char *)&((tls_t *)0)->name)-(char *)0)

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


#define KRN_Dummy               (TAG_USER + 0x03d00000)
#define KRN_KernelBase          (KRN_Dummy + 1)
#define KRN_KernelLowest        (KRN_Dummy + 2)
#define KRN_KernelHighest       (KRN_Dummy + 3)
#define KRN_KernelBss           (KRN_Dummy + 4)
#define KRN_GDT                 (KRN_Dummy + 5)
#define KRN_IDT                 (KRN_Dummy + 6)
#define KRN_PL4                 (KRN_Dummy + 7)
#define KRN_VBEModeInfo         (KRN_Dummy + 8)
#define KRN_VBEControllerInfo   (KRN_Dummy + 9)
#define KRN_MMAPAddress         (KRN_Dummy + 10)
#define KRN_MMAPLength          (KRN_Dummy + 11)
#define KRN_CmdLine             (KRN_Dummy + 12)
#define KRN_ProtAreaStart       (KRN_Dummy + 13)
#define KRN_ProtAreaEnd         (KRN_Dummy + 14)
#define KRN_VBEMode             (KRN_Dummy + 15)
#define KRN_VBEPaletteWidth     (KRN_Dummy + 16)

/*
 * The target base address of 64-bit kernel
 */

#define KERNEL_TARGET_ADDRESS   0x01000000
//#define KERNEL_HIGH_OFFSET      0x1ffULL
//#define KERNEL_HIGH_OFFSET      0x1ffULL
//#define KERNEL_HIGH_OFFSET      31ULL
//#define KERNEL_OFFSET           0x01000000
#define KERNEL_OFFSET           0
//#define KERNEL_OFFSET           0xfffffffff8000000ULL
//#define KERNEL_OFFSET           0x0000000000000000ULL

#endif /*AROS_  KERNEL_H*/
