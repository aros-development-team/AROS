#ifndef KERNEL_INTERN_H_
#define KERNEL_INTERN_H_

#include <inttypes.h>
#include <exec/nodes.h>
#include <exec/lists.h>
#include <aros/kernel.h>
#include <utility/tagitem.h>
#include <asm/cpu.h>

#include <stdio.h>

struct KernelBase {
    struct Node         kb_Node;
    void *              kb_MemPool;
    struct List         kb_Intr[256];
    uint16_t            kb_XTPIC_Mask;
};

#define KBL_INTERNAL    0
#define KBL_XTPIC       1
#define KBL_APIC        2

struct IntrNode {
    struct MinNode      in_Node;
    void                (*in_Handler)(void *, void *);
    void                *in_HandlerData;
    void                *in_HandlerData2;
};

#define STACK_SIZE 8192

#define SC_CAUSE        0
#define SC_DISPATCH     1
#define SC_SWITCH       2
#define SC_SCHEDULE     3

int exec_main(struct TagItem *msg, void *entry);
void core_LeaveInterrupt(regs_t *regs) __attribute__((noreturn));
void core_Switch(regs_t *regs) __attribute__((noreturn));
void core_Schedule(regs_t *regs) __attribute__((noreturn));
void core_Dispatch(regs_t *regs) __attribute__((noreturn));
void core_ExitInterrupt(regs_t *regs) __attribute__((noreturn)); 
void core_IRQHandle(regs_t regs);
void core_Cause(struct ExecBase *SysBase);
void core_SetupIDT();
void core_SetupGDT();
void core_DefaultIRETQ();

struct TagItem *krnNextTagItem(const struct TagItem **tagListPtr);
struct TagItem *krnFindTagItem(Tag tagValue, const struct TagItem *tagList);
IPTR krnGetTagData(Tag tagValue, intptr_t defaultVal, const struct TagItem *tagList);

void scr_RawPutChars(char *, int);
void clr();
static char tab[512];
#ifdef rkprintf
#undef rkprintf
#endif
#define rkprintf(x...) scr_RawPutChars(tab, snprintf(tab, 510, x))

#endif /*KERNEL_INTERN_H_*/
