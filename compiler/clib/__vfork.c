/*
    Copyright © 2008-2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/exec.h>
#include <exec/tasks.h>
#include <dos/dos.h>
#include <dos/filesystem.h>
#include <aros/cpu.h>

#include <sys/types.h>
#include <unistd.h>
#include <setjmp.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "etask.h"
#include "__arosc_privdata.h"
#include "__open.h"
#include "__vfork.h"
#include "__exec.h"

#define DEBUG 0

#include <aros/debug.h>
#include <aros/startup.h>

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

LONG launcher()
{
    D(bug("launcher: Entered child launcher\n"));

    struct Task *this = FindTask(NULL);
    struct vfork_data *udata = this->tc_UserData;
    BYTE child_signal;
    struct Library *aroscbase = NULL;

    GetIntETask(this)->iet_startup = GetETask(this)->et_Result2 = AllocVec(sizeof(struct arosc_startup), MEMF_ANY | MEMF_CLEAR);

    /* Allocate signal for parent->child communication */
    child_signal = udata->child_signal = AllocSignal(-1);
    D(bug("launcher: Allocated child signal: %d\n", udata->child_signal));
    if(udata->child_signal == -1)
    {
	/* Lie */
	udata->child_errno = ENOMEM;
	Signal(udata->parent, 1 << udata->parent_signal);
	return -1;
    }

    if(__register_init_fdarray(udata->ppriv->acpd_fd_array, udata->ppriv->acpd_numslots))
        aroscbase = OpenLibrary((STRPTR) "arosc.library", 0);
    if(!aroscbase)
    {
	FreeSignal(child_signal);
	udata->child_errno = ENOMEM;
	Signal(udata->parent, 1 << udata->parent_signal);
	return -1;	
    }
    
    udata->cpriv = __get_arosc_privdata();
    udata->cpriv->acpd_parent_does_upath = udata->ppriv->acpd_doupath;
    udata->cpriv->acpd_flags |= DO_NOT_CLONE_ENV_VARS;

    if(setjmp(__aros_startup_jmp_buf) == 0)
    {
        /* Setup complete, signal parent */
        D(bug("launcher: Signaling parent that we finished setup\n"));
        Signal(udata->parent, 1 << udata->parent_signal);

        D(bug("launcher: Child waiting for exec or exit\n"));
        Wait(1 << udata->child_signal);

        if(udata->child_executed)
        {
            APTR exec_id;
            
            D(bug("launcher: child executed\n"));

            child_takeover(udata);

            /* Filenames passed from parent obey parent's __doupath */
            __doupath = udata->cpriv->acpd_parent_does_upath;
            D(bug("launcher: __doupath == %d for __exec_prepare()\n", __doupath));
            
            exec_id = udata->exec_id = __exec_prepare(
                udata->exec_filename,
                udata->exec_searchpath,
                udata->exec_argv,
                udata->exec_envp
            );
            
            udata->child_errno = errno;

            /* Clear __doupath again, command will set it if wanted */
            __doupath = 0;
            
            D(bug("launcher: informing parent that we have run __exec_prepare\n"));
            /* Inform parent that we have run __exec_prepare */
            Signal(udata->parent, 1 << udata->parent_signal);
            
            /* Wait 'till __exec_do() is called on parent process */
            D(bug("launcher: Waiting parent to get the result\n"));
            Wait(1 << udata->child_signal);

            D(bug("launcher: informing parent that we won't use udata anymore\n"));
            /* Inform parent that we won't use udata anymore */
            Signal(udata->parent, 1 << udata->parent_signal);

            if (exec_id)
            {
                D(bug("launcher: executing command\n"));
                __exec_do(exec_id);
                
                assert(0); /* Should not be reached */
            }
            else
            {
                D(bug("launcher: exit because execve returned with an error\n"));
                _exit(0);
            }
        }
        else
        {
            D(bug("launcher: informing parent that we won't use udata anymore\n"));
            /* Inform parent that we won't use udata anymore */
            Signal(udata->parent, 1 << udata->parent_signal);
        }
    }
    else
    {
        D(bug("launcher: freeing child_signal\n"));
        FreeSignal(child_signal);
        CloseLibrary(aroscbase);
    }
    
    return 0;
}

pid_t __vfork(jmp_buf env)
{
    struct Task *this = FindTask(NULL);
    struct vfork_data *udata = AllocMem(sizeof(struct vfork_data), MEMF_ANY | MEMF_CLEAR);
    if(udata == NULL)
    {
	errno = ENOMEM;
	longjmp(env, -1);	
    }
    D(bug("__vfork: allocated udata %p\n", udata));
    bcopy(env, &udata->vfork_jump, sizeof(jmp_buf));

    struct TagItem tags[] =
    {
	{ NP_Entry,         (IPTR) launcher  },
	{ NP_CloseInput,    (IPTR) FALSE     },
	{ NP_CloseOutput,   (IPTR) FALSE     },
	{ NP_CloseError,    (IPTR) FALSE     },
        { NP_Cli,           (IPTR) TRUE      },
        { NP_Name,          (IPTR) "vfork()" },
        { NP_UserData,      (IPTR) udata     },
        { NP_NotifyOnDeath, (IPTR) TRUE      },
        { TAG_DONE,         0                }
    };

    udata->parent = this;
    
    struct arosc_privdata *ppriv = __get_arosc_privdata();
    udata->ppriv = ppriv;
    
    /* Store parent's vfork_data to restore it later */
    udata->prev = __get_arosc_privdata()->acpd_vfork_data;
    D(bug("__vfork: Saved old parent's vfork_data: %p\n", udata->prev));
                                        
    D(bug("__vfork: backuping startup buffer\n"));
    /* Backup startup buffer */
    CopyMem(&__aros_startup_jmp_buf, &udata->startup_jmp_buf, sizeof(jmp_buf));

    D(bug("__vfork: Allocating parent signal\n"));
    /* Allocate signal for child->parent communication */
    udata->parent_signal = AllocSignal(-1);
    if(udata->parent_signal == -1)
    {
	/* Couldn't allocate the signal, return -1 */
	FreeMem(udata, sizeof(struct vfork_data));
	errno = ENOMEM;
	longjmp(udata->vfork_jump, -1);    
    }
    
    D(bug("__vfork: Creating child\n"));
    udata->child = (struct Task*) CreateNewProc(tags);

    if(udata->child == NULL)
    {
	/* Something went wrong, return -1 */
	FreeMem(udata, sizeof(struct vfork_data));
	errno = ENOMEM; /* Most likely */
	longjmp(env, -1);
    }
    D(bug("__vfork: Child created %p, waiting to finish setup\n", udata->child));
    udata->child_id = GetETaskID(udata->child);
    D(bug("__vfork: Got unique child id: %d\n", udata->child_id));

    /* Wait for child to finish setup */
    Wait(1 << udata->parent_signal);
    
    if(udata->child_errno)
    {
	/* An error occured during child setup */
	errno = udata->child_errno;
	longjmp(env, -1);
    }
    
    D(bug("__vfork: Setting jmp_buf at %p in %p\n", __aros_startup, &__aros_startup_jmp_buf));
    if(setjmp(__aros_startup_jmp_buf))
    {
	D(bug("__vfork: child exited\n or executed\n"));

        /* Reinitialize variables as they may have been overwritten during setjmp */
        ppriv = __get_arosc_privdata();
        udata = ppriv->acpd_vfork_data;

	if(!udata->child_executed)
	{
	    D(bug("__vfork: not executed\n"));
	    ((struct arosc_startup*) GetIntETask(udata->child)->iet_startup)->as_startup_error = __aros_startup_error;
	    D(bug("__vfork: Signaling child\n"));
	    Signal(udata->child, 1 << udata->child_signal);
	}

	D(bug("__vfork: Waiting for child to finish using udata\n"));
	/* Wait for child to finish using udata */
	Wait(1 << udata->parent_signal);

	D(bug("__vfork: fflushing\n"));
	fflush(NULL);

	D(bug("__vfork: restoring startup buffer\n"));
	/* Restore parent startup buffer */
	CopyMem(&udata->startup_jmp_buf, &__aros_startup_jmp_buf, sizeof(jmp_buf));

	D(bug("__vfork: freeing parent signal\n"));
	FreeSignal(udata->parent_signal);

        errno = udata->child_errno;

        D(bug("__vfork: Remembering jmp_buf\n"));
        jmp_buf env;
        bcopy(&udata->vfork_jump, env, sizeof(jmp_buf));

        parent_leavepretendchild(udata);

        D(bug("__vfork: freeing udata\n"));
        FreeMem(udata, sizeof(struct vfork_data));

        D(bug("__vfork: Child(%d) jumping to jmp_buf %p\n", udata->child_id, &env));
        D(bug("__vfork: ip: %p, stack: %p\n", env->retaddr, env->regs[_JMPLEN - 1]));
        vfork_longjmp(env, udata->child_id);
	assert(0); /* not reached */
        return (pid_t) 1;
    }

    parent_enterpretendchild(udata);

    D(bug("__vfork: Jumping to jmp_buf %p\n", &udata->vfork_jump));
    D(bug("__vfork: ip: %p, stack: %p\n", udata->vfork_jump[0].retaddr, udata->vfork_jump[0].regs[_JMPLEN - 1]));
    vfork_longjmp(udata->vfork_jump, 0);
    assert(0); /* not reached */
    return (pid_t) 0;
}


static void parent_enterpretendchild(struct vfork_data *udata)
{
    D(bug("parent_enterpretendchild(%x): entered\n", udata));

    udata->ppriv->acpd_vfork_data = udata;

    /* Remember and switch fd descriptor table */
    udata->parent_acpd_fd_mempool = udata->ppriv->acpd_fd_mempool;
    udata->parent_acpd_numslots = udata->ppriv->acpd_numslots;
    udata->parent_acpd_fd_array = udata->ppriv->acpd_fd_array;
    udata->ppriv->acpd_fd_mempool = udata->cpriv->acpd_fd_mempool;
    udata->ppriv->acpd_numslots = udata->cpriv->acpd_numslots;
    udata->ppriv->acpd_fd_array = udata->cpriv->acpd_fd_array;

    /* Remember and switch chdir fields */
    udata->parent_cd_changed = __cd_changed;
    udata->parent_cd_lock = __cd_lock;
    __cd_changed = udata->cpriv->acpd_cd_changed;
    __cd_lock = udata->cpriv->acpd_cd_lock;
    udata->parent_curdir = CurrentDir(((struct Process *)udata->child)->pr_CurrentDir);
    
    /* Pretend to be running as the child created by vfork */
    udata->ppriv->acpd_flags |= PRETEND_CHILD;

    D(bug("parent_enterpretendchild: leaving\n"));
}

static void child_takeover(struct vfork_data *udata)
{
    D(bug("child_takeover(%x): entered\n", udata));
            
    /* Set current dir to parent's current dir */
    __cd_changed = udata->ppriv->acpd_cd_changed;
    __cd_lock = udata->ppriv->acpd_cd_lock;
    CurrentDir(((struct Process *)udata->parent)->pr_CurrentDir);

    D(bug("child_takeover(): leaving\n"));
}

static void parent_leavepretendchild(struct vfork_data *udata)
{
    D(bug("parent_leavepretendchild(%x): entered\n", udata));

    /* Restore parent's old fd_array */
    udata->ppriv->acpd_fd_mempool = udata->parent_acpd_fd_mempool;
    udata->ppriv->acpd_numslots = udata->parent_acpd_numslots;
    udata->ppriv->acpd_fd_array =  udata->parent_acpd_fd_array;

    /* Switch to currentdir from before vfork() call */
    __cd_changed = udata->parent_cd_changed;
    __cd_lock = udata->parent_cd_lock;
    CurrentDir(udata->parent_curdir);

    /* Switch to previous vfork_data */
    udata->ppriv->acpd_vfork_data = udata->prev;
    if(udata->prev == NULL)
        udata->ppriv->acpd_flags &= ~PRETEND_CHILD;

    D(bug("parent_leavepretendchild: leaving\n"));
}

