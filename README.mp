WHAT IS IT
----------

Memory Protection (or short MP) means that you want to protect vital
system structures against hostile programs or bugs.

HOW AROS DOES IT
----------------

AROS uses several different strategies to implement MP depending on the
overhead for the resource. If you want more speed, you can disable this
schemes altogether. AROS will then run without MP.

For compatibility reasons, memory allocated with AllocMem() or similar
(with or without the MEMF_PUBLIC flag) the memory may be read by and
written to by every task. If you want memory which other tasks can read
but must not write to, use the new MEMF_SHARED_READ (Tip: if you have put
some effort in making your code use the MEMF_PUBLIC flag for this kind
of memory, just #undef MEMF_PUBLIC and define it anew as MEMF_SHARED).

The new AROS flag MEMF_PRIVATE makes memory inacessible for other tasks.
NOTE: Memory which is currently not allocated by anyone, is protected
like MEMF_PRIVATE.

OTHER WAYS
----------

AROS has two more ways to protect your memory. You can change the
protection for memory allocated by you with

    MP_SetProtection (APTR memory, ULONG size, ULONG protection);

The protection might be

    MPPF_NONE - This is the default if you didn't specify MEMF_SHARED nor
	    MEMF_PRIVATE
    MPPF_SHARED_READ - Other tasks must not write to this memory but may
	    read it
    MPPF_PRIVATE - Other tasks may neither read nor write to this memory.

NOTE: If you call libraries, they may access your memory like you (because
they can be thought of sub-functions) but there are some cases where a
library doesn't do the work themself. A good example is PutMsg() and
ReplyMsg(). This is how messages work on the AmigaOS:

    1. You create a MsgPort
    2. You allocate memory for a message
    3. You fill the message with some useful contents
    4. You specify the MsgPort as the ReplyPort in the message
    5. You send the message to some other task via PutMsg()
    6. The other task reads and/or modifies the message !!!
    7. The other task replies the message via ReplyMsg()
    8. You get a signal that the message came back
    9. You free the message
    10. You delete the MsgPort

Important are the steps 1, 2, 5, 6, and 7. Since ReplyMsg() always
needs to write to your MsgPort, you must allocate it either with
MEMF_PUBLIC or with the call to CreateMsgPort() (recommended since it will
work on future versions of the OS, too). The same applies to the message since
ReplyMsg() must remove the message from the MsgPort you sent it to and
attach it to your MsgPort. So it does NOT matter if the other task
needs only read the message. A message *must* *always* be allocated with
MEMF_PUBLIC.

If you need additional protection, for example if you are debugging the code,
you can use MP_SetProtection() to protect specific parts of the message,
but you should not use this function in the final version of your application
since it uses *lots* of ressources.

MMU Library/Resource
--------------------

MMUInfo * MMU_QueryInfo (void)

    typedef struct
    {
	ULONG PageSize; /* If Pagesize is 0, then no MMU is available */
    } MMUInfo;

MMUContext * MMU_NewContext (void)

    Create a new MMU context. There is only one context for your whole
    task, so if you call this function more than once, you will get
    the same pointer again. But note that you must pair MMU_NewContext()
    and MMU_DeleteContext() calls.

void MMU_DeleteContext (void)

    Delete the MMU's context. Must be called as many times as
    MMU_NewContext() was called.

ULONG MMU_Protect (MMUContext mmuctx, APTR Memory, ULONG Size, ULONG Mode,
	struct Hook * Hit)

    Memory pointer and Size will be rounded to the next multiple of the MMU's
    pagesize. Mode is one of the flags MMUF_READSELF, MMUF_WRITESELF,
    MMUF_EXECSELF, MMUF_NONESELF, MMUF_READOTHER, MMUF_WRITEOTHER,
    MMUF_EXECOTHER, MMUF_NONEOTHER.

    Hit will be called when the memory is accessed and the flag is not set
    (ie. a read to memory by the process itself if MMUF_READSELF is not set
    or any access by someone else if MMUF_NONEOTHER is set).

    The hook will be called as so:

	CallHook (Hit, MMUContext, OffendingTask, Address, Reason)

    OffendingTask is the task structure of the task which did the illegal
    access, Address is the address in memory where the hit happened
    and Reason is one of the MMU flags (and only one). If you want to
    create some memory where other tasks can read and you can write to,
    try this:

	MMU_Protect (mmuctx, mem, size,
	    MMUF_READSELF|MMUF_WRITESELF|MMUF_READOTHER, &hit);

    If the PC ever points to that memory or someone else tries to write into
    that memory, hit will be called.

    The most simple hit might be:

	RemTask (OffendingTask);

    To do something like VMM, use it like this:

	MMU_Protect (mmuctx, 0x80000000, size,
	    MMUF_NONESELF|MMUF_NONEOTHER, &hit);

    Then you will trap all accesses from the address 0x80000000 to
    0x80000000+size-1. Hit would then check that the access was from a task
    which is allowed to use VMM and load the memory from the diskfile.

APTR MMU_AllocMem (MMUContext * MMUCtx, ULONG Size, ULONG Flags, ULONG Mode)

    Is a small utility routine to allocate some memory with the mode
    already. Use MMU_FreeMem() to get rid of the memory again. The
    protection will be cleared then. The function will pool requests with
    the same protection flags.

MMU_FreeMem (MMUContext * MMUCtx, APTR Address, ULONG Size);

    Free memory which was allocated by MMU_AllocMem().

