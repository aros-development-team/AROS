#ifndef KERNEL_INTERN_H_
#define KERNEL_INTERN_H_

#include <inttypes.h>

#ifdef __AROS__
    
#include <exec/nodes.h>
#include <exec/lists.h>
#include <aros/kernel.h>
#include <utility/tagitem.h>
//#include <asm/cpu.h>

//#include <stdio.h>

struct KernelBase {
    struct Node         kb_Node;
};

struct TagItem *krnNextTagItem(const struct TagItem **tagListPtr);
struct TagItem *krnFindTagItem(Tag tagValue, const struct TagItem *tagList);
IPTR krnGetTagData(Tag tagValue, intptr_t defaultVal, const struct TagItem *tagList);

//#define rkprintf(x...) scr_RawPutChars(tab, snprintf(tab, 510, x))

#endif

#ifndef TAG_USER
#define TAG_USER    ((STACKULONG)(1L<<31))
#endif

#define KRN_Dummy               (TAG_USER + 0x03d00000)
#define KRN_KernelBase          (KRN_Dummy + 1)
#define KRN_KernelLowest        (KRN_Dummy + 2)
#define KRN_KernelHighest       (KRN_Dummy + 3)
#define KRN_KernelBss           (KRN_Dummy + 4)
#define KRN_CmdLine             (KRN_Dummy + 5)
#define KRN_MemBase             (KRN_Dummy + 6)
#define KRN_MemSize             (KRN_Dummy + 7)
#define KRN_SysBasePtr          (KRN_Dummy + 8)


struct KernelInterface
{
    /* Kernel functions */
    /* TODO */

    /* Hostlib functions */
    void *(*HostLib_Open)(const char *, char**);
    int (*HostLib_Close)(void *, char **);
    void *(*HostLib_GetPointer)(void *, const char *, char **);
    void (*HostLib_FreeErrorStr)(char *);
    unsigned long (*HostLib_GetInterface)(void *, char **, void **);
};

#endif /*KERNEL_INTERN_H_*/
