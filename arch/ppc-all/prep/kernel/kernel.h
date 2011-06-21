#ifndef _KERNEL_H
#define _KERNEL_H

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/nodes.h>

#include "libdefs.h"

struct KernelBase {
    struct Node	    node;
    struct ExecBase *sysBase;
};

#ifdef SysBase
#undef SysBase
#endif
#define SysBase KernelBase->sysBase

#define _IO_BASE    0x80000000
#define _BUS_BASE   0xc0000000

#define VECTOR_COUNT	8

extern UBYTE Kernel_end;
extern void *const LIBFUNCTABLE[VECTOR_COUNT+1];

#endif /* _KERNEL_H */

