/*
    Copyright © 2008-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/stdc.h>
#include <exec/exec.h>
#include <exec/tasks.h>
#include <dos/dos.h>
#include <libraries/stdc.h>
#include <aros/cpu.h>

#include <sys/types.h>
#include <unistd.h>
#include <setjmp.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "__posixc_intbase.h"
#include "__fdesc.h"
#include "__vfork.h"
#include "__exec.h"

#define DEBUG 0

#include <aros/debug.h>
#include <aros/startup.h>

/*****************************************************************************

    NAME
#include <unistd.h>

	pid_t vfork(

    SYNOPSIS
	void)

    FUNCTION
        Function to create a subprocess of the current process.

        This is there to ease porting of software using the fork()/vfork()
        POSIX functions. Due to a different memory and process model, fork()
        is not implemented at the moment in the C library. vfork() is provided
        with some extended functionality. In the POSIX standard the only
        guaranteed functionality for vfork() is to have an exec*() function or
        exit() called right after the vfork() in the child.

        Extra functionality for vfork():
        - The child has its own memory heap; memory allocation/deallocation
          is allowed and the heap will be removed when calling _exit() or will
          be used for the code started by the exec*() functions.
        - The child will have a copy of the file descriptors as specified by
          the POSIX standard for the fork() function. File I/O is possible in
          the child, as is file manipulation with dup() etc.

        Difference with fork():
        - The virtual memory heap is not duplicated as in POSIX but the memory
          is shared between parent and child. AROS lives in one big single
          memory region so changes to memory in the child are also seen by the
          parent.

        Behaviour for other resources not described in this doc may not be
        relied on for future compatibility.

    INPUTS
	-

    RESULT
	-1: error, no child is started; errno will be set.
        0: Running in child
        >0: Running in parent, pid of child is return value.

    NOTES
        Current implementation of vfork() will only really start running things
        in parallel on an exec*() call. After vfork(), child code will run until
        _exit() or exec*(). With _exit(), the child will exit and the parent
        will continue; with exec*(), the child will be detached and the parent
        will continue.

    EXAMPLE

    BUGS

    SEE ALSO
	execl(), execve(), execlp(), execv(), execvp()

    INTERNALS

******************************************************************************/

/* The following functions are used to update the child's and parent's privdata
   for the parent pretending to be running as child and for the child to take
   over. It is called in the following sequence:
   parent_enterpretendchild() is called in vfork() so the parent pretends to be
   running as child; child_takeover() is called by child if exec*() so it can
   continue from the parent state; parent_leavepretendchild() is called by
   parent to switch back to be running as parent
*/
static void parent_enterpretendchild(struct vfork_data *udata);
static void child_takeover(struct vfork_data *udata);
static void parent_leavepretendchild(struct vfork_data *udata);
static void parent_createchild(struct vfork_data *udata);

static __attribute__((noinline)) void __vfork_exit_controlled_stack(struct vfork_data *udata);

LONG launcher()
{
    D(bug("launcher: Entered child launcher, ThisTask=%p\n", SysBase->ThisTask));

    struct Task *this = FindTask(NULL);
    struct vfork_data *udata = this->tc_UserData;
    BYTE child_signal;
    struct PosixCIntBase *PosixCBase = NULL;
    jmp_buf exec_exitjmp; /* jmp_buf for when calling __exec_do */
    int exec_error; /* errno for when calling __exec_do */
    LONG ret = 0;

    /* Allocate signal for parent->child communication */
    child_signal = udata->child_signal = AllocSignal(-1);
    D(bug("launcher: Allocated child signal: %d\n", udata->child_signal));
    if (udata->child_signal == -1)
    {
        /* Lie */
        udata->child_errno = ENOMEM;
        SETCHILDSTATE(CHILD_STATE_SETUP_FAILED);
        Signal(udata->parent, 1 << udata->parent_signal);
        return -1;
    }

    /* TODO: Can we avoid opening posixc.library for child process? */
    PosixCBase = (struct PosixCIntBase *)OpenLibrary((STRPTR) "posixc.library", 0);
    if (PosixCBase)
    {
        PosixCBase->PosixCBase.StdCBase = (struct StdCBase *)OpenLibrary((STRPTR) "stdc.library", 0);
        if (!PosixCBase->PosixCBase.StdCBase)
        {
            CloseLibrary((struct Library *)PosixCBase);
            PosixCBase = NULL;
        }
    }
    if (!PosixCBase)
    { 
        D(bug("launcher:Failed to open libraries!\n"));
        FreeSignal(child_signal);
        udata->child_errno = ENOMEM;
        SETCHILDSTATE(CHILD_STATE_SETUP_FAILED);
        Signal(udata->parent, 1 << udata->parent_signal);
        return -1;
    }
    D(bug("launcher: Opened PosixCBase: %x, StdCBase: %x\n",
          PosixCBase, PosixCBase->PosixCBase.StdCBase
    ));

    udata->child_posixcbase = PosixCBase;

    if (setjmp(exec_exitjmp) == 0)
    {
        /* Setup complete, signal parent */
        D(bug("launcher: Signaling parent that we finished setup\n"));
        SETCHILDSTATE(CHILD_STATE_SETUP_FINISHED);
        Signal(udata->parent, 1 << udata->parent_signal);

        D(bug("launcher: Child waiting for exec or exit\n"));
        Wait(1 << udata->child_signal);
        ASSERTPARENTSTATE(PARENT_STATE_EXEC_CALLED | PARENT_STATE_EXIT_CALLED);
        PRINTSTATE;

        if (udata->child_executed)
        {
            APTR exec_id;

            D(bug("launcher: child executed\n"));

            child_takeover(udata);

            /* Filenames passed from parent obey parent's doupath */

            PosixCBase->doupath = udata->parent_posixcbase->doupath;
            D(bug("launcher: doupath == %d for __exec_prepare()\n", PosixCBase->doupath));
            
            exec_id = udata->exec_id = __exec_prepare(
                udata->exec_filename,
                0,
                udata->exec_argv,
                udata->exec_envp
            );

            /* Reset handling of upath */
            PosixCBase->doupath = 0;
            udata->child_errno = errno;

            D(bug("launcher: informing parent that we have run __exec_prepare\n"));
            /* Inform parent that we have run __exec_prepare */
            SETCHILDSTATE(CHILD_STATE_EXEC_PREPARE_FINISHED);
            Signal(udata->parent, 1 << udata->parent_signal);

            /* Wait 'till __exec_do() is called on parent process */
            D(bug("launcher: Waiting parent to get the result\n"));
            Wait(1 << udata->child_signal);
            ASSERTPARENTSTATE(PARENT_STATE_EXEC_DO_FINISHED);
            PRINTSTATE;

            D(bug("launcher: informing parent that we won't use udata anymore\n"));
            /* Inform parent that we won't use udata anymore */
            SETCHILDSTATE(CHILD_STATE_UDATA_NOT_USED);
            Signal(udata->parent, 1 << udata->parent_signal);

            D(bug("launcher: waiting for parent to be after _exit()\n"));
            Wait(1 << udata->child_signal);
            ASSERTPARENTSTATE(PARENT_STATE_STOPPED_PRETENDING);
            PRINTSTATE;

            if (exec_id)
            {
                D(bug("launcher: catch _exit()\n"));
                __stdc_program_startup(exec_exitjmp, &exec_error);

                D(bug("launcher: executing command\n"));
                __exec_do(exec_id);

                assert(0); /* Should not be reached */
            }
            else
            {
                D(bug("launcher: __exec_prepare returned with an error\n"));
                ret = -1;
            }
        }
        else /* !udata->child_executed */
        {
            D(bug("launcher: child not executed\n"));

            D(bug("launcher: informing parent that we won't use udata anymore\n"));
            /* Inform parent that we won't use udata anymore */
            SETCHILDSTATE(CHILD_STATE_UDATA_NOT_USED);
            Signal(udata->parent, 1 << udata->parent_signal);
        }
    }
    else
    {
        /* child exited */
        D(bug("launcher: catched _exit(), errno=%d\n", exec_error));
    }

    D(bug("launcher: freeing child_signal\n"));
    FreeSignal(child_signal);

    D(bug("launcher: closing libraries\n"));
    CloseLibrary((struct Library *)PosixCBase->PosixCBase.StdCBase);
    CloseLibrary((struct Library *)PosixCBase);
    
    D(bug("Child Done\n"));

    return ret;
}

/* This can be good for debugging */
#ifdef __arm__
#define SP 8
#define ALT 27
#endif

#ifndef SP
#define SP  _JMPLEN - 1
#define ALT _JMPLEN - 1
#endif

pid_t __vfork(jmp_buf env)
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    struct Task *this = FindTask(NULL);
    struct ETask *etask = NULL;
    struct vfork_data *udata = AllocMem(sizeof(struct vfork_data), MEMF_ANY | MEMF_CLEAR);
    if (udata == NULL)
    {
        errno = ENOMEM;
        vfork_longjmp(env, -1);
    }
    D(bug("__vfork: Parent: allocated udata %p, jmp_buf %p\n", udata, udata->vfork_jmp));
    *udata->vfork_jmp = *env;

    D(bug("__vfork: Parent: initial jmp_buf %p\n", env));
    D(bug("__vfork: Parent: ip: %p, stack: %p, alt: 0x%p\n", env->retaddr, env->regs[SP], env->regs[ALT]));
    D(bug("__vfork: Parent: Current altstack 0x%p\n", *((void **) this->tc_SPLower)));
    D(hexdump(env, 0, sizeof(jmp_buf) + sizeof(void *) * 4));

    udata->parent = this;
    udata->prev = PosixCBase->vfork_data;

    D(bug("__vfork: Parent: Saved old parent's vfork_data: %p\n", udata->prev));
    udata->parent_posixcbase = PosixCBase;

    parent_createchild(udata);

    D(bug("__vfork: Parent: Setting jmp_buf at %p\n", udata->parent_newexitjmp));
    if (setjmp(udata->parent_newexitjmp) == 0)
    {
        udata->parent_olderrorptr = __stdc_set_errorptr(&udata->child_error);
        udata->child_error = *udata->parent_olderrorptr;
        __stdc_set_exitjmp(udata->parent_newexitjmp, udata->parent_oldexitjmp);

        parent_enterpretendchild(udata);

        D(bug("__vfork: Child(%p) jumping to jmp_buf %p\n",
              udata->child, &udata->vfork_jmp
        ));
        D(bug("__vfork: ip: %p, stack: %p alt: %p\n",
              udata->vfork_jmp[0].retaddr, udata->vfork_jmp[0].regs[SP],
              udata->vfork_jmp[0].regs[ALT]
        ));

        vfork_longjmp(udata->vfork_jmp, 0);
        assert(0); /* not reached */
        return (pid_t) 0;
    }
    else /* setjmp() != 0; so child has exited() */
    {
        D(bug("__vfork: Child: child exiting\n or executed\n"));

        /* Stack may have been overwritten when we return here,
         * we jump to here from a function lower in the call chain
         */
        PosixCBase = (struct PosixCIntBase *)__aros_getbase_PosixCBase();
        udata = PosixCBase->vfork_data;

        D(bug("__vfork: Child: acb_vfork_data = %x\n", udata));

        if (!udata->child_executed)
        {
            D(bug("__vfork: Child: not executed\n"));

            /* et_Result is normally set in startup code but no exec was performed
               so we have to mimic the startup code
             */
            etask = GetETask(udata->child);
            if (etask)
                etask->et_Result1 = udata->child_error;

            D(bug("__vfork: Child: Signaling child %p, signal %d\n", udata->child, udata->child_signal));
            SETPARENTSTATE(PARENT_STATE_EXIT_CALLED);
            Signal(udata->child, 1 << udata->child_signal);
        }

        D(bug("__vfork: Parent: Waiting for child to finish using udata, me=%p, signal %d\n", FindTask(NULL),
                udata->parent_signal));
        /* Wait for child to finish using udata */
        Wait(1 << udata->parent_signal);
        ASSERTCHILDSTATE(CHILD_STATE_UDATA_NOT_USED);
        PRINTSTATE;

        D(bug("__vfork: Parent: fflushing\n"));
        fflush(NULL);

        __vfork_exit_controlled_stack(udata);

        assert(0); /* not reached */
        return (pid_t) 1;
    }
}

/*
 * The sole purpose of this function is to enable control over allocation of dummy and env.
 * Previously they were allocated in the ending code of __vfork function. On ARM however
 * this was causing immediate allocation of space at entry to the __vfork function. Moreover
 * the jmp_buf is aligned(16) and as such is represented on the stack as a pointer to stack
 * region instead of offset from stack base.
 *
 * The exit block of __vfork function represents code that underwent a number of longjumps. The
 * stack there is not guaranteed to be preserved, thus the on-stack pointer representing dummy
 * and env were also damaged. Extracting the code below allows to control when the variables
 * are allocated (as long as the function remains not inlined).
 */
static __attribute__((noinline)) void __vfork_exit_controlled_stack(struct vfork_data *udata)
{
    jmp_buf dummy;
    jmp_buf env;

    D(bug("__vfork: Parent: freeing parent signal\n"));
    FreeSignal(udata->parent_signal);

    errno = udata->child_errno;

    /* leavepretendchild will restore old StdCBase and thus also
       old startup jmp_buf.
       This is also the reason this function has to be called before
       signaling child that we are after _exit().
    */
    parent_leavepretendchild(udata);

    if(udata->child_executed)
    {
        D(bug("__vfork: Inform child that we are after _exit()\n"));
        SETPARENTSTATE(PARENT_STATE_STOPPED_PRETENDING);
        Signal(udata->child, 1 << udata->child_signal);
    }

    D(bug("__vfork: Parent: restoring startup buffer\n"));
    /* Restore parent errorptr and startup buffer */
    __stdc_set_errorptr(udata->parent_olderrorptr);
    __stdc_set_exitjmp(udata->parent_oldexitjmp, dummy);

    /* Save some data from udata before udata is being freed */
    *env = *udata->vfork_jmp;

    D(bug("__vfork: Parent: freeing udata\n"));
    FreeMem(udata, sizeof(struct vfork_data));

    D(bug("__vfork: Parent jumping to jmp_buf %p\n", env));
    D(bug("__vfork: ip: %p, stack: %p\n", env->retaddr, env->regs[SP]));
    vfork_longjmp(env, GetETaskID(udata->child));
}

static void parent_createchild(struct vfork_data *udata)
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();

    struct TagItem tags[] =
    {
        { NP_Entry,         (IPTR) launcher },
        { NP_CloseInput,    (IPTR) FALSE },
        { NP_CloseOutput,   (IPTR) FALSE },
        { NP_CloseError,    (IPTR) FALSE },
        { NP_Cli,           (IPTR) TRUE },
        { NP_Name,          (IPTR) "vfork()" },
        { NP_UserData,      (IPTR) udata },
        { NP_NotifyOnDeath, (IPTR) TRUE },
        { TAG_DONE, 0 }
    };

    D(bug("__vfork: Parent: Allocating parent signal\n"));
    /* Allocate signal for child->parent communication */
    udata->parent_signal = AllocSignal(-1);
    if (udata->parent_signal == -1)
    {
        /* Couldn't allocate the signal, return -1 */
        FreeMem(udata, sizeof(struct vfork_data));
        errno = ENOMEM;
        vfork_longjmp(udata->vfork_jmp, -1);
    }

    PosixCBase->flags |= VFORK_PARENT;

    D(bug("__vfork: Parent: Creating child\n"));
    udata->child = (struct Task*) CreateNewProc(tags);

    if (udata->child == NULL)
    {
        D(bug("__vfork: Child could not be created\n"));
        /* Something went wrong, return -1 */
        FreeMem(udata, sizeof(struct vfork_data));
        errno = ENOMEM; /* Most likely */
        vfork_longjmp(udata->vfork_jmp, -1);
    }
    D(bug("__vfork: Parent: Child created %p, waiting to finish setup\n", udata->child));

    /* Wait for child to finish setup */
    Wait(1 << udata->parent_signal);
    ASSERTCHILDSTATE(CHILD_STATE_SETUP_FAILED | CHILD_STATE_SETUP_FINISHED);
    PRINTSTATE;

    if (udata->child_errno)
    {
        /* An error occured during child setup */
        D(bug("__vfork: Child returned an error (%d)\n", udata->child_errno));
        errno = udata->child_errno;
        vfork_longjmp(udata->vfork_jmp, -1);
    }

    PosixCBase->flags &= ~VFORK_PARENT;
}

static void parent_enterpretendchild(struct vfork_data *udata)
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    D(bug("parent_enterpretendchild(%x): entered\n", udata));

    PosixCBase->vfork_data = udata;

    /* Remember and switch StdCBase */
    udata->parent_stdcbase = PosixCBase->PosixCBase.StdCBase;
    PosixCBase->PosixCBase.StdCBase = udata->child_posixcbase->PosixCBase.StdCBase;
    /* _[eE]xit() can also be called with the switched StdCBase so we also
       register the exit jmp_buf in this StdCBase. We don't need to remember
       old as child will overwrite these if it should call __exec_do().
    */
    __stdc_program_startup(udata->parent_newexitjmp, &udata->child_error);

    /* Remember and switch env var list */
    udata->parent_env_list = PosixCBase->env_list;
    PosixCBase->env_list = udata->child_posixcbase->env_list;

    /* Remember and switch fd descriptor table */
    udata->parent_internalpool = PosixCBase->internalpool;
    PosixCBase->internalpool = udata->child_posixcbase->internalpool;
    __getfdarray((APTR *)&udata->parent_fd_array, &udata->parent_numslots);
    __setfdarraybase(udata->child_posixcbase);
    
    /* Remember and switch chdir fields */
    udata->parent_cd_changed = PosixCBase->cd_changed;
    PosixCBase->cd_changed = udata->child_posixcbase->cd_changed;
    udata->parent_cd_lock = PosixCBase->cd_lock;
    PosixCBase->cd_lock = udata->child_posixcbase->cd_lock;
    udata->parent_curdir = CurrentDir(((struct Process *)udata->child)->pr_CurrentDir);

    /* Remember and switch upathbuf */
    udata->parent_upathbuf = PosixCBase->upathbuf;
    PosixCBase->upathbuf = udata->child_posixcbase->upathbuf;

    /* Pretend to be running as the child created by vfork */
    udata->parent_flags = PosixCBase->flags;
    PosixCBase->flags |= PRETEND_CHILD;

    D(bug("parent_enterpretendchild: leaving\n"));
}

static void child_takeover(struct vfork_data *udata)
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    D(bug("child_takeover(%x): entered\n", udata));

    /* Set current dir to parent's current dir */
    PosixCBase->cd_changed = udata->parent_posixcbase->cd_changed;
    PosixCBase->cd_lock = udata->parent_posixcbase->cd_lock;
    CurrentDir(((struct Process *)udata->parent)->pr_CurrentDir);

    D(bug("child_takeover(): leaving\n"));
}

static void parent_leavepretendchild(struct vfork_data *udata)
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    D(bug("parent_leavepretendchild(%x): entered\n", udata));

    /* Restore parent's StdCBase */
    PosixCBase->PosixCBase.StdCBase = udata->parent_stdcbase;

    /* Restore parent's env var list */
    PosixCBase->env_list = udata->parent_env_list;

    /* Restore parent's old fd_array */
    PosixCBase->internalpool = udata->parent_internalpool;
    __setfdarray(udata->parent_fd_array, udata->parent_numslots);

    /* Switch to currentdir from before vfork() call */
    PosixCBase->cd_changed = udata->parent_cd_changed;
    PosixCBase->cd_lock = udata->parent_cd_lock;
    CurrentDir(udata->parent_curdir);

    /* Restore parent's upathbuf */
    PosixCBase->upathbuf = udata->parent_upathbuf;

    /* Switch to previous vfork_data */
    PosixCBase->vfork_data = udata->prev;
    PosixCBase->flags = udata->parent_flags;

    D(bug("parent_leavepretendchild: leaving\n"));
}
