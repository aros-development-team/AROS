#ifndef KERNEL_INTERN_H_
#define KERNEL_INTERN_H_

#include <inttypes.h>
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

#endif /*KERNEL_INTERN_H_*/
