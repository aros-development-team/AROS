#ifndef KERNEL_INTERN_H_
#define KERNEL_INTERN_H_

#include <inttypes.h>
#include <exec/lists.h>
#include <exec/execbase.h>
#include <utility/tagitem.h>
#include <asm/amcc440.h>
#include <stdio.h>

#define KERNEL_PHYS_BASE        0x00800000
#define KERNEL_VIRT_BASE        0xff800000

#define STACK_SIZE 4096

struct KernelBase {
    struct Node         kb_Node;
    void *              kb_MemPool;
    struct List         kb_Exceptions[16];
    struct List         kb_Interrupts[16];
};

struct KernelBSS {
    void *addr;
    uint32_t len;
};

intptr_t krnGetTagData(Tag tagValue, intptr_t defaultVal, const struct TagItem *tagList);
struct TagItem *krnFindTagItem(Tag tagValue, const struct TagItem *tagList);
struct TagItem *krnNextTagItem(const struct TagItem **tagListPtr);

static void __attribute__((used)) kernel_cstart(struct TagItem *msg);
void core_LeaveInterrupt(regs_t *regs) __attribute__((noreturn));
void core_Switch(regs_t *regs) __attribute__((noreturn));
void core_Schedule(regs_t *regs) __attribute__((noreturn));
void core_Dispatch(regs_t *regs) __attribute__((noreturn));
void core_ExitInterrupt(regs_t *regs) __attribute__((noreturn)); 
void core_Cause(struct ExecBase *SysBase);
void mmu_init(struct TagItem *tags);
void intr_init();

void __attribute__((noreturn)) syscall_handler(regs_t *ctx, uint8_t exception, void *self);

void __puts(char *str);
static char tab[512];
#ifdef _rkprintf
#undef _rkprintf
#endif
#define _rkprintf(x...) do { snprintf(tab, 510, x); __puts(tab); } while(0)

#endif /*KERNEL_INTERN_H_*/
