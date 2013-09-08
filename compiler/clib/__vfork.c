/*
    Copyright © 2008-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/dos.h>
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

#include "__arosc_privdata.h"
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
        POSIX functions. Due to different memory and process model fork()
        is not implemented at the moment in the C library. vfork() is provided
        with some extenden functionality. In POSIX standard the only guaranteed
        functionality for vfork() is to have a exec*() function or _exit() called
        right after the vfork() in the child.
        Extra functionality for vfork():
        - The child has it's own memory heap, memory allocation/deallocation
          is allowed and heap will be removed when calling _exit() or will be used
          for the code started by the exec*() functions.
        - The child will have a copy of the file descriptors as specified by POSIX
          standard for the fork() function. File I/O is possible in child, also
          file manipulation with dup() etc.

        Difference with fork():
        - The virtual memory heap is not duplicated as in POSIX but the memory
          is shared between parent and child. AROS lives in one big single memory
          region so changes to memory in child are also seen by parent.

        Behaviour for other resources not described in this doc may not be relied
        on for future.

    INPUTS
	-

    RESULT
	-1: error, no child is started; errno will be set.
        0: Running in child
        >0: Running in parent, pid of child is return value.

    NOTES
        Current implementation of vfork() will only really start running things
        in parallel on an exec*() call. After vfork() child code will run until
        _exit() or exec*(). With _exit() child will exit and parent continue;
        with exec*() child will be detached and parent will continue.

    EXAMPLE

    BUGS

    SEE ALSO
	execl(), execve(), execlp(), execv(), execvp()

    INTERNALS

******************************************************************************/

/* The following functions are used to update the childs and parents privdata
   for the parent pretending to be running as child and for the child to take over. It is called
   in the following sequence:
   parent_enterpretendchild() is called in vfork so the parent pretends to be running as child
   child_takeover() is called by child if exec*() so it can continue from the parent state
   parent_leavepretendchild() is called by parent to switch back to be running as parent
*/
static void parent_enterpretendchild(struct vfork_data *udata);
static void child_takeover(struct vfork_data *udata);
static void parent_leavepretendchild(struct vfork_data *udata);

static __attribute__((noinline)) void __vfork_exit_controlled_stack(struct vfork_data *udata);

LONG launcher()
{
    D(bug("launcher: Entered child launcher, ThisTask=%p\n", SysBase->ThisTask));

    struct Task *this = FindTask(NULL);
    struct vfork_data *udata = this->tc_UserData;
    BYTE child_signal;
    struct aroscbase *aroscbase = NULL;
    jmp_buf exec_exitjmp; /* jmp_buf for when calliing __exec_do */
    int exec_error; /* errno for when calling __exec_do */
    LONG ret = 0;

    /* Allocate signal for parent->child communication */
    child_signal = udata->child_signal = AllocSignal(-1);
    D(bug("launcher: Allocated child signal: %d\n", udata->child_signal));
    if (udata->child_signal == -1)
    {
        /* Lie */
        udata->child_errno = ENOMEM;
        Signal(udata->parent, 1 << udata->parent_signal);
        return -1;
    }

    aroscbase = (struct aroscbase *) OpenLibrary((STRPTR) "arosc.library", 0);
    if (aroscbase)
        aroscbase->StdCBase = (struct StdCBase *)OpenLibrary((STRPTR)"stdc.library", 0);
    if (!aroscbase || !aroscbase->StdCBase)
    {
        if (aroscbase)
            CloseLibrary((struct Library *)aroscbase);
        FreeSignal(child_signal);
        udata->child_errno = ENOMEM;
        Signal(udata->parent, 1 << udata->parent_signal);
        return -1;
    }
    D(bug("launcher: Opened aroscbase: %x, StdCBase: %x\n",
          aroscbase, aroscbase->StdCBase
      )
    );
    /* Temporary run with parent PosixCBase;
       this code will be removed when vfork() is moved
    */
    aroscbase->PosixCBase = pbase->PosixCBase;

    udata->child_aroscbase = aroscbase;

    if (setjmp(exec_exitjmp) == 0)
    {
        /* Setup complete, signal parent */
        D(bug("launcher: Signaling parent that we finished setup\n"));
        Signal(udata->parent, 1 << udata->parent_signal);

        D(bug("launcher: Child waiting for exec or exit\n"));
        Wait(1 << udata->child_signal);

        if (udata->child_executed)
        {
            APTR exec_id;

            D(bug("launcher: child executed\n"));

            child_takeover(udata);

            /* Filenames passed from parent obey parent's acb_doupath */

            aroscbase->acb_doupath = udata->parent_aroscbase->acb_doupath;
            D(bug("launcher: acb_doupath == %d for __exec_prepare()\n", aroscbase->acb_doupath));

            exec_id = udata->exec_id = __exec_prepare(udata->exec_filename, 0, udata->exec_argv, udata->exec_envp);

            /* Reset handling of upath */
            aroscbase->acb_doupath = 0;
            udata->child_errno = errno;

            D(bug("launcher: informing parent that we have run __exec_prepare\n"));
            /* Inform parent that we have run __exec_prepare */
            Signal(udata->parent, 1 << udata->parent_signal);

            /* Wait 'till __exec_do() is called on parent process */
            D(bug("launcher: Waiting parent to get the result\n"));
            Wait(1 << udata->child_signal);

            D(bug("launcher: informing parent that we won't use udata anymore\n"));
            /* Inform parent that we won't use udata anymore */
            Signal(udata->parent, 1 << udata->parent_signal);

            D(bug("launcher: waiting for parent to be after _exit()\n"));
            Wait(1 << udata->child_signal);

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
            Signal(udata->parent, 1 << udata->parent_signal);
        }
    }

    D(bug("launcher: freeing child_signal\n"));
    FreeSignal(child_signal);

    D(bug("launcher: closing aroscbase\n"));
    CloseLibrary((struct Library *)aroscbase);

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
    struct aroscbase *aroscbase = __aros_getbase_aroscbase();
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

    D(bug("__vfork: Parent: initial jmp_buf %p\n", env));
    D(bug("__vfork: Parent: ip: %p, stack: %p, alt: 0x%p\n", env->retaddr, env->regs[SP], env->regs[ALT]));
    D(bug("__vfork: Parent: Current altstack 0x%p\n", *((void **) this->tc_SPLower)));
    D(hexdump(env, 0, sizeof(jmp_buf) + sizeof(void *) * 4));

    udata->parent = this;
    udata->prev = aroscbase->acb_vfork_data;

    D(bug("__vfork: Parent: Saved old parent's vfork_data: %p\n", udata->prev));
    udata->parent_aroscbase = aroscbase;

    D(bug("__vfork: Parent: Allocating parent signal\n"));
    /* Allocate signal for child->parent communication */
    udata->parent_signal = AllocSignal(-1);
    if (udata->parent_signal == -1)
    {
        /* Couldn't allocate the signal, return -1 */
        FreeMem(udata, sizeof(struct vfork_data));
        errno = ENOMEM;
        vfork_longjmp(env, -1);
    }

    aroscbase->acb_flags |= VFORK_PARENT;

    D(bug("__vfork: Parent: Creating child\n"));
    udata->child = (struct Task*) CreateNewProc(tags);

    if (udata->child == NULL)
    {
        /* Something went wrong, return -1 */
        FreeMem(udata, sizeof(struct vfork_data));
        errno = ENOMEM; /* Most likely */
        vfork_longjmp(env, -1);
    }
    D(bug("__vfork: Parent: Child created %p, waiting to finish setup\n", udata->child));

    /* Wait for child to finish setup */
    Wait(1 << udata->parent_signal);

    if (udata->child_errno)
    {
        /* An error occured during child setup */
        errno = udata->child_errno;
        vfork_longjmp(env, -1);
    }

    aroscbase->acb_flags &= ~VFORK_PARENT;

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
        aroscbase = __aros_getbase_aroscbase();
        udata = aroscbase->acb_vfork_data;

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
            Signal(udata->child, 1 << udata->child_signal);
        }

        D(bug("__vfork: Parent: Waiting for child to finish using udata, me=%p, signal %d\n", FindTask(NULL),
                udata->parent_signal));
        /* Wait for child to finish using udata */
        Wait(1 << udata->parent_signal);

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
 * the jmp_buf is alligned(16) and as such is represeted on the stack as a pointer to stack
 * region instead of offset from stack base.
 *
 * The exit block of __vfork function reprents a code that underwent a number of longjumps. The
 * stack there is not guaranteed to be preserved, thus the on-stack pointer representing dummy
 * and evn were also damaged. Extracting the code below allows to control when the variables
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

static void parent_enterpretendchild(struct vfork_data *udata)
{
    struct aroscbase *aroscbase = __aros_getbase_aroscbase();
    D(bug("parent_enterpretendchild(%x): entered\n", udata));

    aroscbase->acb_vfork_data = udata;

    /* Remember and switch malloc mempool */
    udata->parent_mempool = aroscbase->acb_mempool;
    aroscbase->acb_mempool = udata->child_aroscbase->acb_mempool;

    /* Remember and switch env var list */
    // Temporary disabled, needs to be enabled in
    // posixc.library/__vfork.c later on
    //udata->parent_env_list = aroscbase->acb_env_list;
    //aroscbase->acb_env_list = udata->child_aroscbase->acb_env_list;

    /* Remember and switch fd descriptor table */
    udata->parent_internalpool = aroscbase->acb_internalpool;
    aroscbase->acb_internalpool = udata->child_aroscbase->acb_internalpool;
    __getfdarray((APTR *) &udata->parent_fd_array, &udata->parent_numslots);
    __setfdarraybase(udata->child_aroscbase);

    /* Remember and switch chdir fields */
    udata->parent_cd_changed = aroscbase->acb_cd_changed;
    aroscbase->acb_cd_changed = udata->child_aroscbase->acb_cd_changed;
    udata->parent_cd_lock = aroscbase->acb_cd_lock;
    aroscbase->acb_cd_lock = udata->child_aroscbase->acb_cd_lock;
    udata->parent_curdir = CurrentDir(((struct Process *) udata->child)->pr_CurrentDir);

    /* Pretend to be running as the child created by vfork */
    udata->parent_flags = aroscbase->acb_flags;
    aroscbase->acb_flags |= PRETEND_CHILD;

    D(bug("parent_enterpretendchild: leaving\n"));
}

static void child_takeover(struct vfork_data *udata)
{
    struct aroscbase *aroscbase = __aros_getbase_aroscbase();
    D(bug("child_takeover(%x): entered\n", udata));

    /* Set current dir to parent's current dir */
    aroscbase->acb_cd_changed = udata->parent_aroscbase->acb_cd_changed;
    aroscbase->acb_cd_lock = udata->parent_aroscbase->acb_cd_lock;
    CurrentDir(((struct Process *) udata->parent)->pr_CurrentDir);

    D(bug("child_takeover(): leaving\n"));
}

static void parent_leavepretendchild(struct vfork_data *udata)
{
    struct aroscbase *aroscbase = __aros_getbase_aroscbase();
    D(bug("parent_leavepretendchild(%x): entered\n", udata));

    /* Restore parent's malloc mempool */
    aroscbase->acb_mempool = udata->parent_mempool;

    /* Restore parent's env var list */
    // Temporary disabled, needs to be enabled in
    // posixc.library/__vfork.c later on
    //aroscbase->acb_env_list = udata->parent_env_list;

    /* Restore parent's old fd_array */
    aroscbase->acb_internalpool = udata->parent_internalpool;
    __setfdarray(udata->parent_fd_array, udata->parent_numslots);

    /* Switch to currentdir from before vfork() call */
    aroscbase->acb_cd_changed = udata->parent_cd_changed;
    aroscbase->acb_cd_lock = udata->parent_cd_lock;
    CurrentDir(udata->parent_curdir);

    /* Switch to previous vfork_data */
    aroscbase->acb_vfork_data = udata->prev;
    if (aroscbase->acb_vfork_data == NULL)
    {
        aroscbase->acb_flags = udata->parent_flags;
        aroscbase->acb_flags &= ~PRETEND_CHILD;
    }

    D(bug("parent_leavepretendchild: leaving\n"));
}
