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
