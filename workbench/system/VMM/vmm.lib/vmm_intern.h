/*
 * Copyright (C) 2025, The AROS Development Team
 * All right reserved.
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#ifndef NONVOLATILE_INTERN_H
#define NONVOLATILE_INTERN_H

#include <exec/libraries.h>
#include <dos/bptr.h>

struct VMMBase {
    struct Library  VM_LIB;
    BPTR      VM_LIB_SEGLIST;
    BPTR      VM_HNDL_SEGLIST;
    APTR      VM_DOSLIB;
    APTR      VM_PORT;
    struct MsgPort *VM_STARTERPORT;
    APTR      VM_SEMA;
    APTR      VM_MEMHEADER;
    struct VMMsg *VM_INIT_MSG;
};

#endif /* VMM_INTERN_H */
