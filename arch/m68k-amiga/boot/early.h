/*
 * Copyright (C) 2011, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#ifndef EARLY_H
#define EARLY_H

#include <exec/memory.h>
#include <exec/alerts.h>

#define RGB(r,g,b)	((((r) & 0xf) << 8) | (((g) & 0xf) << 4) | (((b) & 0xf) << 0))
#define RGB_MASK	RGB(15, 15, 15)

#define CODE_ROM_CHECK	RGB( 4,  4, 4)
#define CODE_RAM_CHECK	RGB( 9,  9, 9)
#define CODE_EXEC_CHECK	RGB( 1,  1, 1)
#define CODE_ALLOC_FAIL	(RGB( 0, 12, 0) | AT_DeadEnd)
#define CODE_TRAP_FAIL	(RGB(12, 12, 0) | AT_DeadEnd)
#define CODE_EXEC_FAIL	(RGB( 0, 12,12) | AT_DeadEnd)

void Early_ScreenCode(ULONG code);
void Early_Alert(ULONG alert);
void __attribute__((interrupt)) Early_TrapHandler(void);
void __attribute__((interrupt)) Early_Exception(void);

APTR Early_AllocAbs(struct MemHeader *mh, APTR location, IPTR byteSize);

/* Must match with AROSBootstrap.c! */
#define ABS_BOOT_MAGIC 0x4d363802
struct BootStruct
{
    ULONG magic;
    struct ExecBase *RealBase;
    struct ExecBase *RealBase2;
    struct List *mlist;
    struct TagItem *kerneltags;
    struct Resident **reslist;
    struct ExecBase *FakeBase;
    APTR bootcode;
    APTR ss_address;
    LONG ss_size;
    APTR magicfastmem;
    LONG magicfastmemsize;
};
struct BootStruct *GetBootStruct(struct ExecBase *eb);

#endif /* EARLY_H */
