#ifndef EXEC_TASKS_H
#define EXEC_TASKS_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Task structure and constants
    Lang: english
*/

#ifndef EXEC_NODES_H
#   include <exec/nodes.h>
#endif
#ifndef EXEC_LISTS_H
#   include <exec/lists.h>
#endif
#ifndef EXEC_PORTS_H
#   include <exec/ports.h>
#endif
#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

struct ETask;

/* You must use Exec functions to modify task structure fields. */
struct Task
{
    struct Node tc_Node;
    UBYTE	tc_Flags;
    UBYTE	tc_State;
    BYTE	tc_IDNestCnt;	/* Interrupt disabled nesting */
    BYTE	tc_TDNestCnt;	/* Task disabled nesting */
    ULONG	tc_SigAlloc;	/* Allocated signals */
    ULONG	tc_SigWait;	/* Signals we are waiting for */
    ULONG	tc_SigRecvd;	/* Received signals */
    ULONG	tc_SigExcept;	/* Signals we will take exceptions for */
    union
    {
	struct
	{
	    UWORD tc_ETrapAlloc;   /* Allocated traps */
	    UWORD tc_ETrapAble;    /* Enabled traps */
	} tc_ETrap;
	struct ETask *tc_ETask;	   /* Valid if TF_ETASK is set */
    }		tc_UnionETask;
    APTR	tc_ExceptData;	/* Exception data */
    APTR	tc_ExceptCode;	/* Exception code */
    APTR	tc_TrapData;	/* Trap data */
    APTR	tc_TrapCode;	/* Trap code */
    APTR	tc_SPReg;	/* Stack pointer */
    APTR	tc_SPLower;	/* Stack lower bound */
    APTR	tc_SPUpper;	/* Stack upper bound */
    VOID     (* tc_Switch)();   /* Task loses CPU */
    VOID     (* tc_Launch)();   /* Task gets CPU */
    struct List tc_MemEntry;	/* Allocated memory. Freed by RemTask(). */
    APTR	tc_UserData;	/* For use by the task; no restrictions! */
};

#define tc_TrapAlloc	    tc_UnionETask.tc_ETrap.tc_ETrapAlloc
#define tc_TrapAble	    tc_UnionETask.tc_ETrap.tc_ETrapAble

/* Macros */
#define GetTrapAlloc(t) \
	((((struct Task *)t)->tc_Flags & TF_ETASK) ? \
	    ((struct ETask *)(((struct Task *)t)->tc_UnionETask.tc_ETask))-> \
		et_TrapAlloc : \
	    ((struct Task *)t)->tc_UnionETask.tc_ETrap.tc_ETrapAlloc)
#define GetTrapAble(t) \
	((((struct Task *)t)->tc_Flags & TF_ETASK) ? \
	    ((struct ETask *)(((struct Task *)t)->tc_UnionETask.tc_ETask))-> \
		et_TrapAble : \
	    ((struct Task *)t)->tc_UnionETask.tc_ETrap.tc_ETrapAble)
#define GetETask(t) \
	((((struct Task *)t)->tc_Flags & TF_ETASK) ? \
	    ((struct ETask *)(((struct Task *)t)->tc_UnionETask.tc_ETask)) \
	    : NULL \
	)
#define GetETaskID(t) \
	(   (((struct Task *)(t))->tc_Flags & TF_ETASK) \
	    ? (((struct ETask *) \
		(((struct Task *)(t))->tc_UnionETask.tc_ETask))->et_UniqueID) \
	    : 0UL \
	)


/* Stack swap structure as passed to StackSwap() */
struct StackSwapStruct
{
    APTR  stk_Lower;   /* Lowest byte of stack */
    APTR  stk_Upper;   /* Upper end of stack (size + Lowest) */
    APTR  stk_Pointer; /* Stack pointer at switch point */
};

struct StackSwapArgs
{
    IPTR Args[8];          /* The C function arguments */
};

/* tc_Flags Bits */
#define TB_PROCTIME	0
#define TB_ETASK	3
#define TB_STACKCHK	4
#define TB_EXCEPT	5
#define TB_SWITCH	6
#define TB_LAUNCH	7

#define TF_PROCTIME	(1L<<0)
#define TF_ETASK	(1L<<3)
#define TF_STACKCHK	(1L<<4)
#define TF_EXCEPT	(1L<<5)
#define TF_SWITCH	(1L<<6)
#define TF_LAUNCH	(1L<<7)

/* Task States (tc_State) */
#define TS_INVALID	0
#define TS_ADDED	1
#define TS_RUN		2
#define TS_READY	3
#define TS_WAIT		4
#define TS_EXCEPT	5
#define TS_REMOVED	6

/* Predefined Signals */
#define SIGB_ABORT	0
#define SIGB_CHILD	1
#define SIGB_BLIT	4	/* Note: same as SIGB_SINGLE */
#define SIGB_SINGLE	4	/* Note: same as SIGB_BLIT */
#define SIGB_INTUITION	5
#define SIGB_NET	7
#define SIGB_DOS	8

#define SIGF_ABORT	(1L<<0)
#define SIGF_CHILD	(1L<<1)
#define SIGF_BLIT	(1L<<4)
#define SIGF_SINGLE	(1L<<4)
#define SIGF_INTUITION	(1L<<5)
#define SIGF_NET	(1L<<7)
#define SIGF_DOS	(1L<<8)

/* Define this when compiling MorphOS-compatible source */
#ifdef AROS_MORPHOS_COMPATIBLE

#define tc_ETask tc_UnionETask.tc_ETask

struct ETask
{
    struct Message  Message;
    struct Task    *Parent;	  /* Pointer to task */
    ULONG	    UniqueID;
    struct MinList  Children;     /* List of children */
    UWORD	    TrapAlloc;
    UWORD	    TrapAble;
    ULONG	    Result1;	  /* First result */
    APTR	    Result2;	  /* Result data pointer (AllocVec) */
    struct MsgPort  MsgPort;
    void           *MemPool;
    void	   *Reserved[2];
    void	   *RegFrame;
    IPTR	   *TaskStorage;

    /* Internal fields follow */
};

#else

/* Extended Task structure */
struct ETask
{
    struct Message  et_Message;
    APTR	    et_Parent;	     /* Pointer to parent task		*/
    ULONG	    et_UniqueID;
    struct MinList  et_Children;     /* List of children		*/
    UWORD	    et_TrapAlloc;
    UWORD	    et_TrapAble;
    ULONG	    et_Result1;	    /* First result			*/
    APTR	    et_Result2;	    /* Result data pointer (AllocVec)	*/
    struct MsgPort  et_TaskMsgPort;
    void	   *et_MemPool;	    /* Task's private memory pool	*/
    void	   *et_Reserved[2]; /* PowerPC stack in MorphOS		*/
    void	   *et_RegFrame;
    IPTR           *et_TaskStorage; /* AROS-specific			*/
    /* Internal fields follow */
};

#endif

/* Return codes from new child functions */
#define CHILD_NOTNEW   1 /* Function not called from a new style task */
#define CHILD_NOTFOUND 2 /* Child not found */
#define CHILD_EXITED   3 /* Child has exited */
#define CHILD_ACTIVE   4 /* Child is currently active and running */

/* Tags for NewCreateTaskA() and NewAddTask() */

#define TASKTAG_Dummy		(TAG_USER + 0x100000)
#define TASKTAG_ERROR           (TASKTAG_Dummy + 0)
#define TASKTAG_CODETYPE        (TASKTAG_Dummy + 1)
#define TASKTAG_PC              (TASKTAG_Dummy + 2)
#define TASKTAG_FINALPC         (TASKTAG_Dummy + 3)
#define TASKTAG_STACKSIZE       (TASKTAG_Dummy + 4)
#define TASKTAG_NAME            (TASKTAG_Dummy + 6)
#define TASKTAG_USERDATA        (TASKTAG_Dummy + 7)
#define TASKTAG_PRI             (TASKTAG_Dummy + 8)
#define TASKTAG_POOLPUDDLE      (TASKTAG_Dummy + 9)
#define TASKTAG_POOLTHRESH      (TASKTAG_Dummy + 10)
#define TASKTAG_ARG1		(TASKTAG_Dummy + 16)
#define TASKTAG_ARG2		(TASKTAG_Dummy + 17)
#define TASKTAG_ARG3		(TASKTAG_Dummy + 18)
#define TASKTAG_ARG4		(TASKTAG_Dummy + 19)
#define TASKTAG_ARG5		(TASKTAG_Dummy + 20)
#define TASKTAG_ARG6		(TASKTAG_Dummy + 21)
#define TASKTAG_ARG7		(TASKTAG_Dummy + 22)
#define TASKTAG_ARG8		(TASKTAG_Dummy + 23)
#define TASKTAG_STARTUPMSG      (TASKTAG_Dummy + 24)
#define TASKTAG_TASKMSGPORT     (TASKTAG_Dummy + 25)
#define TASKTAG_FLAGS           (TASKTAG_Dummy + 26)
#define TASKTAG_TCBEXTRASIZE    (TASKTAG_Dummy + 28)

#define TASKERROR_OK       0
#define TASKERROR_NOMEMORY 1

/* Actions for ShutdownA() */

#define SD_ACTION_POWEROFF   0
#define SD_ACTION_COLDREBOOT 1

#endif /* EXEC_TASKS_H */
