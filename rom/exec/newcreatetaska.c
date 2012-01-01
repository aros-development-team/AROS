/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Create a new task, improved version
    Lang: english
*/

#include <proto/arossupport.h>
#include <exec/memory.h>
#include <exec/execbase.h>

#include <string.h>

#include "exec_debug.h"
#include "exec_util.h"

struct newMemList
{
    struct Node	    nml_Node;
    UWORD 	    nml_NumEntries;
    struct MemEntry nml_ME[4];
};

static const struct newMemList MemTemplate =
{
    { 0, },
    2,
    {
	{{MEMF_CLEAR|MEMF_PUBLIC}, sizeof(struct Task)	 }, /* Task descriptor itself */
	{{MEMF_CLEAR		}, AROS_STACKSIZE     	 }, /* Task's stack	      */
	{{MEMF_CLEAR|MEMF_PUBLIC}, 0		      	 }, /* Task name	      */
	{{MEMF_PUBLIC		}, sizeof(struct MsgPort)}  /* Task's MsgPort	      */
    }
};

/*****************************************************************************

    NAME */
#include <exec/tasks.h>
#include <proto/exec.h>

	AROS_LH1(struct Task *, NewCreateTaskA,

/*  SYNOPSIS */
	AROS_LHA(struct TagItem *, tags, A0),

/*  LOCATION */
        struct ExecBase *, SysBase, 153, Exec)

/*  FUNCTION
	Create a new task.

    INPUTS
	tags - TagList which may contain the following tags:

	  TASKTAG_ERROR        (ULONG *) - a pointer to an optional location for secondary
				      	   return code. The code itself will be set to
					   TASKERROR_OK on success or TASKERROR_NOMEMORY on
					   failure.
	  TASKTAG_PC           (APTR)    - Start address of the task's code.
	  TAGKTAG_FINALPC      (APTR)    - Address of the finalization routine. Defaults to
				      	   SysBase->TaskExitCode.
          TASKTAG_STACKSIZE    (ULONG)   - Size of task's stack. Defaults to CPU-dependent
					   value.
	  TASKTAG_NAME	       (STRPTR)  - A pointer to task name. The name will be copied.
	  TASKTAG_USERDATA     (APTR)    - Anything. Will be written into tc_UserData.
	  TASKTAG_PRI          (BYTE)    - Task's priority. Defaults to 0.
	  TASKTAG_ARG1 ...
	  TASKTAG_ARG8	       (IPTR)    - Arguments (up to 8) which will be passed to task's
					   entry function. The arguments are supplied in
					   C-standard way.
	  TASKTAG_FLAGS        (ULONG)   - Initial value for tc_Flags.
	  TASKTAG_TASKMSGPORT  (struct MsgPort **)
	  				 - Create a message port for the task and place its
	  				   address into the location specified by ti_Data.
	  TASKTAG_TCBEXTRASIZE (ULONG)   - Value which will be added to sizeof(struct Task)
					   in order to determine final size of task structure.
					   Can be used for appending user data to task structure.

    RESULT
	A pointer to the new task or NULL on failure.

    NOTES

    EXAMPLE

    BUGS
	Value of TASKTAG_FLAGS is actually ignored.
	There are some more tags which are currently not implemented.

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Task * newtask;
    struct newMemList nml = MemTemplate;
    struct MemList  * ml;
    struct TagItem *tag, *tstate = tags;
    struct MsgPort **msgPortPtr = NULL;
    ULONG *errPtr    = NULL;
    APTR   initpc    = NULL;
    APTR   finalpc   = SysBase->TaskExitCode;
    char  *taskname  = NULL;
    APTR   userdata  = NULL;
    ULONG  pri	     = 0;
    ULONG  flags     = 0;

    while ((tag = LibNextTagItem(&tstate)))
    {
	switch (tag->ti_Tag)
	{
	case TASKTAG_ERROR:
	    errPtr = (ULONG *)tag->ti_Data;
	    break;

	case TASKTAG_PC:
	    initpc = (APTR)tag->ti_Data;
	    break;

	case TASKTAG_FINALPC:
	    finalpc = (APTR)tag->ti_Data;
	    break;

	case TASKTAG_STACKSIZE:
	    nml.nml_ME[1].me_Length = AROS_ALIGN(tag->ti_Data);
	    break;

	case TASKTAG_NAME:
	    taskname = (char *)tag->ti_Data;
	    nml.nml_ME[2].me_Length = strlen(taskname) + 1;
	    if (nml.nml_NumEntries < 3)
	    	nml.nml_NumEntries = 3;
	    break;

	case TASKTAG_USERDATA:
	    userdata = (APTR)tag->ti_Data;
	    break;

	case TASKTAG_PRI:
	    pri = tag->ti_Data;
	    break;

	case TASKTAG_TASKMSGPORT:
	    msgPortPtr = (struct MsgPort **)tag->ti_Data;
	    if (nml.nml_NumEntries < 4)
	    	nml.nml_NumEntries = 4;
	    break;

	case TASKTAG_FLAGS:
	    flags = tag->ti_Data;
	    break;

	case TASKTAG_TCBEXTRASIZE:
	    nml.nml_ME[0].me_Length += tag->ti_Data;
	    break;
 	}
    }

    DADDTASK("NewCreateTaskA: name %s", taskname ? taskname : "<NULL>");

    ml = NewAllocEntry((struct MemList *)&nml, NULL);
    if (ml)
    {
    	struct Task *task2 = NULL;
	APTR name = ml->ml_ME[2].me_Addr;

	if (taskname)
	    strcpy(name, taskname);

	newtask = ml->ml_ME[0].me_Addr;

	newtask->tc_Node.ln_Type = NT_TASK;
	newtask->tc_Node.ln_Pri  = pri;
	newtask->tc_Node.ln_Name = name;

	if (nml.nml_NumEntries == 4)
	{
	    /*
	     * The caller asked us to create an MsgPort for the task.
	     * We can't reuse ETask's one because it's used for notifications about child tasks.
	     * We don't check against msgPortPtr == NULL because in future we can have MorphOS-compatible
	     * GetTaskAttr().
	     */
	    struct MsgPort *mp = ml->ml_ME[3].me_Addr;
	    LONG sig;

	    /*
	     * Allocate a signal for the port.
	     * Set tc_SigAlloc early for AllocTaskSignal() to work.
	     */
	    newtask->tc_SigAlloc = SysBase->TaskSigAlloc;
	    sig = AllocTaskSignal(newtask, -1, SysBase);

	    if (sig == -1)
	    {
		DADDTASK("NewCreateTaskA: Failed to allocate a signal for MsgPort");
	    	goto fail;
	    }

	    InitMsgPort(mp);
	    mp->mp_SigBit  = sig;
	    mp->mp_SigTask = newtask;

	    /* Return port address */
	    if (msgPortPtr)
	    	*msgPortPtr = mp;
	}

	/* FIXME: NewAddTask() will reset flags to 0 */
	newtask->tc_Flags    = flags;
	newtask->tc_UserData = userdata;

	/*
	 * On some architectures (PPC) stack frames must be preallocated.
	 * SP_OFFSET is subtracted in order to take care of this.
	 */
	newtask->tc_SPReg   = ml->ml_ME[1].me_Addr + nml.nml_ME[1].me_Length - SP_OFFSET;
	newtask->tc_SPLower = ml->ml_ME[1].me_Addr;
	newtask->tc_SPUpper = newtask->tc_SPReg;

	NEWLIST(&newtask->tc_MemEntry);
	AddHead(&newtask->tc_MemEntry, &ml->ml_Node);

	/*
	 * TASKTAG_ARGx will be processed by PrepareContext()
	 * TODO: process initpc and finalpc there too after ABI v1 transition
	 */
	task2 = NewAddTask (newtask, initpc, finalpc, tags);

fail:	if (!task2)
	{
	    FreeEntry (ml);
	    newtask = NULL;
	}
    }
    else
	newtask=NULL;

    /* Set secondary error code if requested */
    if (errPtr)
	*errPtr = newtask ? TASKERROR_OK : TASKERROR_NOMEMORY;

    return newtask;

    AROS_LIBFUNC_EXIT
}
