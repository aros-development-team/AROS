#ifndef __VFORK_H
#define __VFORK_H

/*
    Copyright © 2008-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <dos/bptr.h>
#include <exec/exec.h>
#include <setjmp.h>
#include <sys/types.h>
#include <aros/startup.h>

#include "__fdesc.h"

struct PosixCIntBase;

struct vfork_data
{
    struct vfork_data *prev;
    jmp_buf vfork_jmp;                      /* jmp to place where vfork was called */

    struct Task *parent;
    int *parent_olderrorptr;
    jmp_buf parent_oldexitjmp;
    jmp_buf parent_newexitjmp;
    BYTE parent_signal;
    BYTE parent_state;
    struct PosixCIntBase *parent_posixcbase;
    struct StdCBase *parent_stdcbase;
    int parent_cd_changed;
    BPTR parent_cd_lock;
    BPTR parent_curdir;
    struct __env_item *parent_env_list;
    APTR parent_internalpool;
    int parent_numslots;
    fdesc **parent_fd_array;
    int parent_flags;
    char *parent_upathbuf;

    struct Task *child;
    int child_executed;
    int child_error, child_errno;
    BYTE child_signal;
    BYTE child_state;
    struct PosixCIntBase *child_posixcbase;

    const char *exec_filename;
    char *const *exec_argv;
    char *const *exec_envp;
    APTR exec_id;
};

pid_t __vfork(jmp_buf env);
void vfork_longjmp (jmp_buf env, int val);

#define PARENT_STATE_EXIT_CALLED            (1L << 0)
#define PARENT_STATE_EXEC_CALLED            (1L << 1)
#define PARENT_STATE_EXEC_DO_FINISHED       (1L << 2)
#define PARENT_STATE_STOPPED_PRETENDING     (1L << 3)

#define CHILD_STATE_SETUP_FAILED            (1L << 0)
#define CHILD_STATE_SETUP_FINISHED          (1L << 1)
#define CHILD_STATE_EXEC_PREPARE_FINISHED   (1L << 2)
#define CHILD_STATE_UDATA_NOT_USED          (1L << 3)

#define DSTATE(x)   //x

#define PRINTSTATE  \
    DSTATE(bug("[PrintState]: Parent = %d, Child = %d, Line %d\n", udata->parent_state, udata->child_state, __LINE__));

#define SETPARENTSTATE(a) \
    udata->parent_state = a

#define ASSERTPARENTSTATE(a) \
    DSTATE({                                \
        if (!(udata->parent_state & (a)))   \
            bug("Parent state assertion failed, was %d, expected %d\n", udata->parent_state, (a));\
    })

#define SETCHILDSTATE(a) \
    udata->child_state = a

#define ASSERTCHILDSTATE(a) \
    DSTATE({                                \
        if (!(udata->child_state & (a)))    \
            bug("Child state assertion failed, was %d, expected %d\n", udata->parent_state, (a));\
    })

#endif /* __VFORK_H */
