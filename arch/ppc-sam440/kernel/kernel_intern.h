#ifndef KERNEL_INTERN_H_
#define KERNEL_INTERN_H_

#include <inttypes.h>
#include <exec/lists.h>
#include <utility/tagitem.h>
#include <asm/amcc440.h>
#include <stdio.h>

struct KernelBase {
    struct Node         kb_Node;
    void *              kb_MemPool;
    struct List         kb_Intr[256];
    uint16_t            kb_XTPIC_Mask;
};

struct KernelBSS {
    void *addr;
    uint32_t len;
};

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
#define KRN_ARGC                (KRN_Dummy + 17)
#define KRN_ARGV                (KRN_Dummy + 18)

IPTR krnGetTagData(Tag tagValue, intptr_t defaultVal, const struct TagItem *tagList);
struct TagItem *krnFindTagItem(Tag tagValue, const struct TagItem *tagList);
struct TagItem *krnNextTagItem(const struct TagItem **tagListPtr);

void core_LeaveInterrupt(regs_t *regs) __attribute__((noreturn));
void core_Switch(regs_t *regs) __attribute__((noreturn));
void core_Schedule(regs_t *regs) __attribute__((noreturn));
void core_Dispatch(regs_t *regs) __attribute__((noreturn));
void core_ExitInterrupt(regs_t *regs) __attribute__((noreturn)); 

void __puts(char *str);
static char tab[512];
#ifdef rkprintf
#undef rkprintf
#endif
#define rkprintf(x...) do { snprintf(tab, 510, x); __puts(tab); } while(0)

#endif /*KERNEL_INTERN_H_*/
