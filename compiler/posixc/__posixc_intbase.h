/*
    Copyright © 2012-2017, The AROS Development Team. All rights reserved.
    $Id$

    This file defines the private part of PosixCBase.
    This should only be used internally in posixc.library code so
    changes can be made to this structure without breaking backwards
    compatibility.
*/
#ifndef __POSIXC_INTBASE_H
#define __POSIXC_INTBASE_H

#include <libraries/posixc.h>
#include <exec/lists.h>
#include <dos/dos.h>
#include <devices/timer.h>

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <pwd.h>

/* Some private structs */
struct random_state;
struct __env_item;
struct _fdesc;
struct vfork_data;

struct PosixCIntBase
{
    struct PosixCBase PosixCBase;

    /* common */
    APTR internalpool;
    int32_t flags;

    struct Device *timerBase;
    struct timerequest timerReq;
    struct MsgPort timerPort;

    /* random.c */
    struct random_state *rs;

    /* getpwuid.c */
    struct passwd pwd;

    /* getpass.c */
    char passbuffer[PASS_MAX];
#if (0)
    /* sigprocmask.c */
    sigset_t   sigmask;
    sigset_t   signals;
#endif

    /* __posixc_environ.c; don't use this field outside that file */
    char ***environptr;

    /* __env.c */
    struct __env_item *env_list;

    /* __exec.c */
    BPTR exec_seglist;
    char *exec_args;
    char *exec_taskname;
    APTR exec_pool;
    char **exec_tmparray;
    BPTR exec_oldin, exec_oldout, exec_olderr;
    struct StdCBase *exec_oldstdcbase;

    /* __fdesc.c */
    int fd_slots;
    struct _fdesc **fd_array;

    /* __upath.c */
    char *upathbuf;  /* Buffer that holds intermediate converted paths */
    int doupath;   /* BOOL - does the conversion need to be done?  */
    int parent_does_upath; /* BOOL - parent does upath conversion */

    /* __vfork.c */
    struct vfork_data *vfork_data;

    /* chdir.c/fchdir.c */
    int cd_changed;
    BPTR cd_lock;

    /* flock.c */
    struct MinList _file_locks, *file_locks;

    /* umask */
    mode_t umask;

    /* __stdio.c */
    struct MinList stdio_files;

    /* setuid.c/getuid.c */
    uid_t uid; /* Real user id of process */
    uid_t euid; /* Effective user id of process */
    /* set(e)gid.c/get(e)gid.c */
    gid_t gid; /* Real group id of process */
    gid_t egid; /* Effective group id of process */
};

/* flags; values of flags are power of two so they can be ORed together */

/* When a program is started with the exec functions and from vfork,
   this is indicated in the flags of the library.
   This way the child can use the parent posixc.library during its
   initialization phase */
#define EXEC_PARENT 0x00000001
#define VFORK_PARENT 0x00000002

/* This flag is set by vfork() to correctly report child process ID during
   execution of child code, even though that it's actually executed by parent
   process until execve() is called. */
#define PRETEND_CHILD 0x00000004

#endif //__POSIXC_INTBASE_H
