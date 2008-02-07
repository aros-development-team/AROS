#ifndef KERNEL_INTERN_H_
#define KERNEL_INTERN_H_

#include <inttypes.h>
#include <exec/lists.h>
#include <utility/tagitem.h>
#include <asm/amcc440.h>
#include <stdio.h>

#define KERNEL_PHYS_BASE        0x00800000
#define KERNEL_VIRT_BASE        0xff800000

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
