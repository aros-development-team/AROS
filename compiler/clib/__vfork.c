/*
    Copyright © 2008, The AROS Development Team. All rights reserved.
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
#include <stdlib.h>
#include <stdio.h>

#include "etask.h"
#include "__arosc_privdata.h"
#include "__open.h"
#include "__vfork.h"

#define DEBUG 0

#include <aros/debug.h>
#include <aros/startup.h>

BPTR DupFHFromfd(int fd, ULONG mode);
void vfork_longjmp (jmp_buf env, int val);
int __init_stdio(void);
LONG exec_command(BPTR seglist, char *taskname, char *args, ULONG stacksize);

LONG launcher()
{
    D(bug("Entered child launcher\n"));

    struct Task *this = FindTask(NULL);
    struct vfork_data *udata = this->tc_UserData;
    BYTE child_signal;
    LONG ret = 0;

    GetIntETask(this)->iet_startup = GetETask(this)->et_Result2 = AllocVec(sizeof(struct aros_startup), MEMF_ANY | MEMF_CLEAR);

    /* Allocate signal for parent->child communication */
    child_signal = udata->child_signal = AllocSignal(-1);
    D(bug("Allocated child signal: %d\n", udata->child_signal));
    if(udata->child_signal == -1)
    {
	/* Lie */
	udata->child_errno = ENOMEM;
	Signal(udata->parent, 1 << udata->parent_signal);
	return -1;
    }

    /* Setup complete, signal parent */
    D(bug("Signaling parent that we finished setup\n"));
    Signal(udata->parent, 1 << udata->parent_signal);

    D(bug("Child waiting for execve or exit\n"));
    Wait(1 << udata->child_signal);

    if(udata->child_executed)
    {
	D(bug("child executed\n"));
	BPTR seglist = udata->exec_seglist;
	char *taskname = udata->exec_taskname;
	char *arguments = udata->exec_arguments;
	ULONG stacksize = udata->exec_stacksize;

	D(bug("informing parent that we won't use udata anymore\n"));
	/* Inform parent that we won't use udata anymore */
	Signal(udata->parent, 1 << udata->parent_signal);
	
	D(bug("executing command\n"));
	/* Run executed command */
	ret = exec_command(
	    seglist, 
	    taskname, 
	    arguments, 
	    stacksize
	);
	
	D(bug("freeing taskname and arguments\n"));
	UnLoadSeg(seglist);
	FreeVec(taskname);
	FreeVec(arguments);
	__aros_startup_error = ret;
	D(bug("exec_command returned %d\n", ret));
    }
    D(bug("freeing child_signal\n"));
    FreeSignal(child_signal);
    return 0;
}

void FreeAndJump(struct vfork_data *udata)
{
    jmp_buf env;
    ULONG child_id = udata->child_id;
    bcopy(&udata->vfork_jump, env, sizeof(jmp_buf));
    D(bug("Restoring old parent's UserData: %p\n", udata->old_UserData));
    udata->parent->tc_UserData = udata->old_UserData;
    D(bug("freeing udata\n"));
    FreeMem(udata, sizeof(struct vfork_data));
    longjmp(env, child_id);
}

pid_t __vfork(jmp_buf env)
{
    int i;
    struct Task *this = FindTask(NULL);
    struct vfork_data *udata = AllocMem(sizeof(struct vfork_data), MEMF_ANY | MEMF_CLEAR);
    if(udata == NULL)
    {
	errno = ENOMEM;
	longjmp(env, -1);	
    }
    D(bug("allocated udata %p\n", udata));
    udata->magic = VFORK_MAGIC;
    bcopy(env, &udata->vfork_jump, sizeof(jmp_buf));

    struct TagItem tags[] =
    {
	{ NP_Entry,         (IPTR) launcher  },
	{ NP_Input,         (IPTR) NULL      }, /* 1 */
	{ NP_Output,        (IPTR) NULL      }, /* 2 */
	{ NP_Error,         (IPTR) NULL      }, /* 3 */
	{ NP_CloseInput,    (IPTR) FALSE     },
	{ NP_CloseOutput,   (IPTR) FALSE     },
	{ NP_CloseError,    (IPTR) FALSE     },
        { NP_Cli,           (IPTR) TRUE      },
        { NP_Name,          (IPTR) "vfork()" },
        { NP_UserData,      (IPTR) udata     },
        { NP_NotifyOnDeath, (IPTR) TRUE      },
        { TAG_DONE,         0                }
    };

    BPTR in, out, err;
    in  = Input();
    out = Output();
    err = Error();
    D(bug("in = %p - out = %p - err = %p\n", BADDR(in), BADDR(out), BADDR(err)));

    if (in)  tags[1].ti_Data = (IPTR)in;
    else     tags[1].ti_Tag  = TAG_IGNORE;
    if (out) tags[2].ti_Data = (IPTR)out;
    else     tags[2].ti_Tag  = TAG_IGNORE;
    if (err) tags[3].ti_Data = (IPTR)err;
    else     tags[3].ti_Tag  = TAG_IGNORE;

    udata->parent = this;
    /* Store parent's UserData to restore it later */
    udata->old_UserData = udata->parent->tc_UserData;
    D(bug("Saved old parent's UserData: %p\n", udata->old_UserData));
    udata->parent->tc_UserData = udata;
    
    D(bug("backuping startup buffer\n"));
    /* Backup startup buffer */
    CopyMem(&__aros_startup_jmp_buf, &udata->startup_jmp_buf, sizeof(jmp_buf));

    D(bug("backuping current directory\n"));
    udata->parent_curdir = CurrentDir(NULL);

    CurrentDir(DupLock(udata->parent_curdir));
    
    D(bug("backuping descriptor table\n"));
    /* Backup parent fd descriptor table */
    struct arosc_privdata *ppriv = GetIntETask(this)->iet_acpd;

    udata->parent_acpd_numslots = ppriv->acpd_numslots;
    udata->parent_acpd_fd_array = ppriv->acpd_fd_array;

    fdesc **acpd_fd_array = calloc((ppriv->acpd_numslots), sizeof(fdesc *));
    if(acpd_fd_array == NULL)
    {
	/* Couldn't allocate fd array, return -1 */
	FreeMem(udata, sizeof(struct vfork_data));
	errno = ENOMEM;
	longjmp(udata->vfork_jump, -1);    	    
    }

    D(bug("opening descriptors\n"));
    /* Copy and "Open" all parent descriptors */
    for(i = 0; i < ppriv->acpd_numslots; i++)
    {
	if(ppriv->acpd_fd_array[i])
	{
	    acpd_fd_array[i] = malloc(sizeof(fdesc));
	    if(!acpd_fd_array[i])
	    {
		FreeMem(udata, sizeof(struct vfork_data));
		errno = ENOMEM;
		longjmp(udata->vfork_jump, -1);		
	    }
	    acpd_fd_array[i]->fcb = ppriv->acpd_fd_array[i]->fcb;
	    acpd_fd_array[i]->fcb->opencount++;
	}
    }

    ppriv->acpd_fd_array = acpd_fd_array;
    
    D(bug("Allocating parent signal\n"));
    /* Allocate signal for child->parent communication */
    udata->parent_signal = AllocSignal(-1);
    if(udata->parent_signal == -1)
    {
	/* Couldn't allocate the signal, return -1 */
	FreeMem(udata, sizeof(struct vfork_data));
	errno = ENOMEM;
	longjmp(udata->vfork_jump, -1);    
    }
    
    D(bug("Creating child\n"));
    udata->child = (struct Task*) CreateNewProc(tags);

    if(udata->child == NULL)
    {
	/* Something went wrong, return -1 */
	FreeMem(udata, sizeof(struct vfork_data));
	errno = ENOMEM; /* Most likely */
	longjmp(env, -1);
    }
    D(bug("Child created %p, waiting to finish setup\n", udata->child));
    udata->child_id = GetETaskID(udata->child);
    D(bug("Got unique child id: %d\n", udata->child_id));

    /* Wait for child to finish setup */
    Wait(1 << udata->parent_signal);
    
    if(udata->child_errno)
    {
	/* An error occured during child setup */
	errno = udata->child_errno;
	longjmp(env, -1);
    }
    
    D(bug("Setting jmp_buf at %p in %p\n", __aros_startup, &__aros_startup_jmp_buf));
    if(setjmp(__aros_startup_jmp_buf))
    {
	D(bug("child exited\n or executed\n"));

	if(!GETUDATA->child_executed)
	{
	    D(bug("not executed\n"));
	    ((struct aros_startup*) GetIntETask(GETUDATA->child)->iet_startup)->as_startup_error = __aros_startup_error;
	    D(bug("Signaling child\n"));
	    Signal(GETUDATA->child, 1 << GETUDATA->child_signal);
	}
	else
	{
	    D(bug("Waiting for child to finish using udata\n"));
	    /* Wait for child to finish using GETUDATA */
	    Wait(1 << GETUDATA->parent_signal);
	}

	D(bug("fflushing\n"));
	fflush(NULL);

	D(bug("Restoring current directory\n"));
	UnLock(CurrentDir(GETUDATA->parent_curdir));
	
	D(bug("Closing opened files\n"));
	/* Close all opened files in "child" */
	int i;
	for(i = 0; i < ((struct arosc_privdata *) GetIntETask(GETUDATA->parent)->iet_acpd)->acpd_numslots; i++)
	{
	    if(((struct arosc_privdata *) GetIntETask(GETUDATA->parent)->iet_acpd)->acpd_fd_array[i])
	    {
		close(i);
	    }
	}

	D(bug("restoring old fd_array\n"));
	/* Restore parent's old fd_array */
	((struct arosc_privdata *) GetIntETask(GETUDATA->parent)->iet_acpd)->acpd_numslots = GETUDATA->parent_acpd_numslots;
	free(((struct arosc_privdata *) GetIntETask(GETUDATA->parent)->iet_acpd)->acpd_fd_array);
	((struct arosc_privdata *) GetIntETask(GETUDATA->parent)->iet_acpd)->acpd_fd_array =  GETUDATA->parent_acpd_fd_array;

	D(bug("restoring startup buffer\n"));
	/* Restore parent startup buffer */
	CopyMem(&GETUDATA->startup_jmp_buf, &__aros_startup_jmp_buf, sizeof(jmp_buf));

	D(bug("freeing parent signal\n"));
	FreeSignal(GETUDATA->parent_signal);

	FreeAndJump(GETUDATA);
	return (pid_t) 1; /* not reached */
    }

    D(bug("Jumping to jmp_buf %p\n", &udata->vfork_jump));
    D(bug("ip: %p, stack: %p\n", udata->vfork_jump[0].retaddr, udata->vfork_jump[0].regs[_JMPLEN - 1]));
    vfork_longjmp(udata->vfork_jump, 0);
    return (pid_t) 0; /* not reached */
}
