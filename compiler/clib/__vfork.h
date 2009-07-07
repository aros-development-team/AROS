#ifndef __VFORK_H
#define __VFORK_H

/*
    Copyright © 2008-2009, The AROS Development Team. All rights reserved.
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

#define GETUDATA ((struct vfork_data*) __get_arosc_privdata()->acpd_vfork_data)

struct vfork_data
{
    struct vfork_data *prev;
    jmp_buf vfork_jump;
    
    struct Task *parent;
    jmp_buf startup_jmp_buf;

    APTR old_UserData;
    ULONG child_id;
    BYTE parent_signal;
    APTR parent_acpd_fd_mempool;
    void *parent_acpd_fd_array;
    int parent_acpd_numslots;
    BPTR parent_curdir;
    struct arosc_privdata *ppriv;
    int old_acpd_flags;

    struct Task *child;
    struct aros_startup child_startup;
    struct arosc_privdata *cpriv;
    struct Library *aroscbase;
    int child_executed;
    int child_errno;
    BYTE child_signal;

    const char *exec_filename;
    int exec_searchpath;
    char *const *exec_argv;
    char *const *exec_envp;
    APTR exec_id;
};

void vfork_longjmp (jmp_buf env, int val);

#endif /* __VFORK_H */
