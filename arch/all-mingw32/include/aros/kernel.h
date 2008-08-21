#ifndef AROS_KERNEL_H
#define AROS_KERNEL_H

#include <utility/tagitem.h>

typedef enum {
    SCHED_RR = 1
} KRN_SchedType;

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
#define KRN_MEMLower            (KRN_Dummy + 17)
#define KRN_MEMUpper            (KRN_Dummy + 18)
#define KRN__TAGCOUNT           (18)

/* The following structure is private for now */
struct HostInterface
{
    void *(*HostLib_Open)(const char *, char**);
    int (*HostLib_Close)(void *, char **);
    void *(*HostLib_GetPointer)(void *, const char *, char **);
    void (*HostLib_FreeErrorStr)(char *);
    unsigned long (*HostLib_GetInterface)(void *, char **, void **);
    int (*VKPrintF)(const char *, va_list);
};

#endif /*AROS_  KERNEL_H*/
