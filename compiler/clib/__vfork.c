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

LONG launcher()
{
    D(bug("Entered child launcher\n"));

    struct Task *this = FindTask(NULL);
    struct vfork_data *udata;
    udata = this->tc_UserData;
    struct arosc_privdata *ppriv = GetIntETask(udata->parent)->iet_acpd;
    struct arosc_privdata *cpriv;;
    int i;

    /* Allocate signal for parent->child communication */
    udata->child_signal = AllocSignal(-1);
    Signal(udata->parent, udata->parent_signal);

    udata->aroscbase = OpenLibrary("arosc.library", 0);
    if (udata->aroscbase == NULL)
    {
	/* Most likely there's not enough memory */
	udata->child_errno = ENOMEM;
	FreeSignal(udata->child_signal);
	Signal(udata->parent, udata->parent_signal);
	return -1;
    }

    cpriv = GetIntETask(this)->iet_acpd;

    /* store child's mempool */
    udata->child_mempool = cpriv->acpd_startup_mempool;
    /* Switch to parent's mempool to allow freeing of fdesc structures 
       allocated by parent during file closing. We can't copy them because 
       they contain reference counters for open files. */
    cpriv->acpd_startup_mempool = ppriv->acpd_startup_mempool;

    /* store child's fd_array */
    udata->child_acpd_numslots = cpriv->acpd_numslots;
    udata->child_acpd_fd_array = cpriv->acpd_fd_array;

    /* Child inherits all file descriptors of the parent, allocate enough
       memory and copy them */
    cpriv->acpd_fd_array = malloc((ppriv->acpd_numslots)*sizeof(fdesc *));
    if(cpriv->acpd_fd_array == NULL)
    {
	FreeSignal(udata->child_signal);
	/* Restore overwritten mempool and fd_array before closing library */
	cpriv->acpd_startup_mempool = udata->child_mempool;
	cpriv->acpd_fd_array = udata->child_acpd_fd_array;
	CloseLibrary(udata->aroscbase);
	udata->child_errno = ENOMEM;
	Signal(udata->parent, udata->parent_signal);
	return -1;
    }
    cpriv->acpd_numslots = ppriv->acpd_numslots;

    CopyMem(
	ppriv->acpd_fd_array, 
	cpriv->acpd_fd_array, 
	(ppriv->acpd_numslots)*sizeof(fdesc *)
    );

    /* Reinit stdio files to make them use parent's mempool */
    __init_stdio();

    /* "Open" all copied descriptors */
    for(i = 0; i < ppriv->acpd_numslots; i++)
    {
	if(ppriv->acpd_fd_array[i])
	{
	    ppriv->acpd_fd_array[i]->opencount++;
	}
    }

    /* Store child's stack to restore it later */
    udata->child = this;
    udata->child_SPLower = this->tc_SPLower;
    udata->child_SPUpper = this->tc_SPUpper;
    udata->child_SPReg = AROS_GET_SP;
    this->tc_UserData = udata;

    GetIntETask(this)->iet_startup = &udata->child_startup;

    cpriv->acpd_parent_does_upath = ppriv->acpd_parent_does_upath;
    cpriv->acpd_doupath = ppriv->acpd_doupath;
  
    D(bug("Waiting for parent to set up his temporary stack\n"));
    Wait(udata->child_signal);
    D(bug("Parent ready, stealing his stack\n"));

    /* Steal parent's stack */
    Forbid();
    this->tc_SPLower = udata->parent_SPLower;
    this->tc_SPUpper = udata->parent_SPUpper;
    /* We have to switch stack before setjmp to assure correct %esp value after
       longjmp during child exit */
    AROS_GET_SP = udata->parent_SPReg;
    Permit();
    
    /* Now we can't use local variables anymore */
    
    D(bug("Setting jmp_buf at %p\n", &__aros_startup_jmp_buf));
    if(setjmp(__aros_startup_jmp_buf))
    {
	D(bug("Child exited\n"));

	D(bug("Restoring stack to %p < %p < %p\n", GETUDATA->child_SPLower, GETUDATA->child_SPReg, GETUDATA->child_SPUpper));
	/* Restoring child stack */
	Forbid();
	GETUDATA->child->tc_SPLower = GETUDATA->child_SPLower;
	GETUDATA->child->tc_SPUpper = GETUDATA->child_SPUpper;
	AROS_GET_SP = GETUDATA->child_SPReg;
	Permit();

	fflush(NULL);

	/* Close all opened files in child */
	for(i = 0; i < ((struct arosc_privdata *) GetIntETask(GETUDATA->child)->iet_acpd)->acpd_numslots; i++)
	{
	    if(((struct arosc_privdata *) GetIntETask(GETUDATA->child)->iet_acpd)->acpd_fd_array[i])
	    {
		close(i);
	    }
	}

	/* Switch back to child's fd_array and mempool */
	((struct arosc_privdata *) GetIntETask(GETUDATA->child)->iet_acpd)->acpd_numslots = GETUDATA->child_acpd_numslots;
	free(((struct arosc_privdata *) GetIntETask(GETUDATA->child)->iet_acpd)->acpd_fd_array);
	((struct arosc_privdata *) GetIntETask(GETUDATA->child)->iet_acpd)->acpd_fd_array =  GETUDATA->child_acpd_fd_array;
	((struct arosc_privdata *) GetIntETask(GETUDATA->child)->iet_acpd)->acpd_startup_mempool = GETUDATA->child_mempool;

	D(bug("Closing aroscbase\n"));
	CloseLibrary(GETUDATA->aroscbase);

	/* If child called execve and just exited, it has already signaled 
	   parent, so skip that step */
	if(!GETUDATA->child_executed)
	{
	    D(bug("Calling daddy\n"));
	    Signal(GETUDATA->parent, GETUDATA->parent_signal);
	    FreeSignal(GETUDATA->child_signal);
	}
	else
	{
	    Wait(GETUDATA->child_signal);
	    FreeSignal(GETUDATA->child_signal);
	    /* Parent won't need udata anymore, we can safely free it */
	    FreeMem(udata, sizeof(struct vfork_data));   
	}
	D(bug("Returning\n"));
	return 0;
    }

    D(bug("Jumping to jmp_buf %p\n", &GETUDATA->vfork_jump));
    vfork_longjmp(GETUDATA->vfork_jump, 0);
    return 0; /* not reached */
}

void FreeAndJump(struct vfork_data *udata)
{
    jmp_buf jump;
    ULONG child_id = udata->child_id;
    CopyMem(&udata->vfork_jump, &jump, sizeof(jmp_buf));

    if(!udata->child_executed)
    {
	/* If child process didn't call execve() then it's no longer alive and we
	   can safely free udata. Otherwise it's freed during child exit */
	FreeMem(udata, sizeof(struct vfork_data));
    }
    else
    {
	/* Otherwise inform child that we don't need udata anymore */
	Signal(udata->child, udata->child_signal);
    }
    D(bug("ip: %p, stack: %p\n", jump[0].retaddr, jump[0].regs[_JMPLEN - 1]));
    longjmp(jump, child_id);
}

pid_t __vfork(jmp_buf env)
{
    struct Task *this = FindTask(NULL);
    struct vfork_data *udata = AllocMem(sizeof(struct vfork_data), MEMF_ANY | MEMF_CLEAR);
    if(udata == NULL)
    {
	errno = ENOMEM;
	longjmp(udata->vfork_jump, -1);	
    }
    D(bug("allocated udata %p\n", udata));
    udata->magic = VFORK_MAGIC;

    struct TagItem tags[] =
    {
	{ NP_Entry,       (IPTR) launcher  },
	{ NP_Input,       (IPTR) NULL      }, /* 1 */
	{ NP_Output,      (IPTR) NULL      }, /* 2 */
	{ NP_Error,       (IPTR) NULL      }, /* 3 */
	{ NP_CloseInput,  (IPTR) FALSE     },
	{ NP_CloseOutput, (IPTR) FALSE     },
	{ NP_CloseError,  (IPTR) FALSE     },
        { NP_Cli,         (IPTR) TRUE      },
        { NP_Name,        (IPTR) "vfork()" },
        { NP_UserData,    (IPTR) udata     },
        { TAG_DONE,       0                }
    };

    BPTR in, out, err;
    in = Input();
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
    
    /* Store parent's stack to restore it later */
    udata->parent_SPLower = udata->parent->tc_SPLower;
    udata->parent_SPUpper = udata->parent->tc_SPUpper;
    udata->parent_SPReg = AROS_GET_SP;
    bcopy(env, &udata->vfork_jump, sizeof(jmp_buf));
    D(bug("Set jmp_buf %p\n", &udata->vfork_jump));
   
    /* Allocate signal for child->parent communication */
    udata->parent_signal = AllocSignal(-1);
    if(udata->parent_signal == -1)
    {
	/* Couldn't allocate the signal, return -1 */
	FreeMem(udata, sizeof(struct vfork_data));
	longjmp(udata->vfork_jump, -1);    
    }
    
    D(bug("Creating child\n"));
    udata->child = (struct Task*) CreateNewProc(tags);
    udata->child_id = GetETaskID(udata->child);
    D(bug("Got unique child id: %d\n", udata->child_id));

    if(udata->child == NULL)
    {
	/* Something went wrong, return -1 */
	FreeMem(udata, sizeof(struct vfork_data));
	errno = ENOMEM; /* Most likely */
	longjmp(env, -1);
    }
    D(bug("Child created %p\n", udata->child));

    /* Wait until children allocates a signal for communication */
    Wait(udata->parent_signal);

    if(udata->child_signal == -1)
    {
	/* Child couldn't allocate the signal */
	FreeSignal(udata->parent_signal);
	FreeMem(udata, sizeof(struct vfork_data));
	longjmp(env, -1);    
    }
    
    /* Switch temporarily to mini stack, child process will use the old 
       stack soon */
    Forbid();
    udata->parent->tc_SPLower = (APTR) udata->ministack;
    udata->parent->tc_SPUpper = (APTR) ((char*) udata->ministack + sizeof(udata->ministack));
    /* Compiler needs some space to store function arguments.
       We can't get the specific amount, but this should be
       enough... */
    AROS_GET_SP = udata->parent->tc_SPUpper - AROS_ALIGN(sizeof(udata->ministack)/2);
    Permit();
    
    /* Now we can't use local variables allocated on the parent's stack */
    D(bug("Temporary stack set up, child can steal ours\n"));
    Signal(GETUDATA->child, GETUDATA->child_signal);
    D(bug("Child signaled\n"));

    D(bug("Waiting for child to die or execve\n"));
    Wait(GETUDATA->parent_signal);
    D(bug("Child died or execved, returning\n"));

    if(GETUDATA->child_errno)
    {
	/* An error occured during child setup */
	errno = udata->child_errno;
	udata->child_id = -1;
    }

    D(bug("Restoring stack to %p < %p < %p\n", GETUDATA->parent_SPLower, GETUDATA->parent_SPReg, GETUDATA->parent_SPUpper));
    /* Getting back our stack */
    Forbid();
    GETUDATA->parent->tc_SPLower = GETUDATA->parent_SPLower;
    GETUDATA->parent->tc_SPUpper = GETUDATA->parent_SPUpper;
    AROS_GET_SP = GETUDATA->parent_SPReg;
    Permit();

    /* Stack is restored, we can use local variables again */

    /* Restore udata variable in case child clobbered it */
    udata = GETUDATA;

    /* Restore parent's UserData */
    udata->parent->tc_UserData = udata->old_UserData;
    D(bug("Restored old parent's UserData: %p\n", udata->old_UserData));

    FreeSignal(udata->parent_signal);
    D(bug("ip: %p, stack: %p\n", udata->vfork_jump[0].retaddr, udata->vfork_jump[0].regs[_JMPLEN - 1]));
    FreeAndJump(udata);
    return (pid_t) udata->child_id; // not reached
}
