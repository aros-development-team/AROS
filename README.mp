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

