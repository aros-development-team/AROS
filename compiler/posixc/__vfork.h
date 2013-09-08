#ifndef __VFORK_H
#define __VFORK_H

/*
    Copyright © 2008-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <dos/bptr.h>
#include <exec/exec.h>
#include <setjmp.h>
#include <sys/types.h>
#include <aros/startup.h>

#include "__fdesc.h"

struct vfork_data
{
    struct vfork_data *prev;
    jmp_buf vfork_jmp;

    struct Task *parent;
    int *parent_olderrorptr;
    jmp_buf parent_oldexitjmp, parent_newexitjmp;
    BYTE parent_signal;
    struct aroscbase *parent_aroscbase;
    APTR parent_mempool;
    int parent_cd_changed;
    BPTR parent_cd_lock;
    BPTR parent_curdir;
    struct __env_item *parent_env_list;
    APTR parent_internalpool;
    int parent_numslots;
    fdesc **parent_fd_array;
    int parent_flags;

    ULONG child_id;
    struct Task *child;
    struct arosc_privdata *cpriv;
    int child_executed;
    int child_error, child_errno;
    BYTE child_signal;
    struct aroscbase *child_aroscbase;
    jmp_buf child_exitjmp;

    const char *exec_filename;
    char *const *exec_argv;
    char *const *exec_envp;
    APTR exec_id;
};

pid_t __vfork(jmp_buf env);
void vfork_longjmp (jmp_buf env, int val);

#endif /* __VFORK_H */
