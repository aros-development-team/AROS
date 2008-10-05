#ifndef __VFORK_H
#define __VFORK_H

/*
    Copyright © 2008, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <dos/bptr.h>
#include <exec/exec.h>
#include <setjmp.h>
#include <aros/startup.h>

/* Define index of a stack register value in jmp_buf.regs */

#if defined __i386__
#   define STACK_INDEX (_JMPLEN-1)
#elif defined __x86_64__
#   define STACK_INDEX (_JMPLEN-1)
#elif defined __powerpc__
#   define STACK_INDEX 1
#else
#   error unsupported CPU type
#endif

#define VFORK_MAGIC 0x666

#define GETUDATA ((struct vfork_data*) ((struct Task*) FindTask(NULL))->tc_UserData)

struct vfork_data
{
    int magic;
    jmp_buf vfork_jump;
    
    struct Task *parent;
    jmp_buf startup_jmp_buf;

    APTR old_UserData;
    ULONG child_id;
    BYTE parent_signal;
    void *parent_acpd_fd_array;
    int parent_acpd_numslots;
    BPTR parent_curdir;

    struct Task *child;
    struct aros_startup child_startup;
    struct Library *aroscbase;
    int child_executed;
    int child_errno;
    BYTE child_signal;

    ULONG exec_stacksize;
    BPTR exec_seglist;
    char *exec_arguments;
    char *exec_taskname;
    LONG exec_returncode;
};

#endif /* __VFORK_H */
