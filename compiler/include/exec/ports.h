#ifndef EXEC_PORTS_H
#define EXEC_PORTS_H

/*
    Copyright  1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Message ports and messages
    Lang: english
*/

#ifndef EXEC_NODES_H
#   include "exec/nodes.h"
#endif
#ifndef EXEC_LISTS_H
#   include "exec/lists.h"
#endif

/* MsgPort */
struct MsgPort
{
    struct Node mp_Node;
    UBYTE	mp_Flags;
    UBYTE	mp_SigBit;  /* Signal bit number */
    void      * mp_SigTask; /* Object to be signalled */
    struct List mp_MsgList; /* Linked list of messages */
};

#define mp_SoftInt mp_SigTask	/* Alias */

/* mp_Flags: Port arrival actions (PutMsg) */
#define PF_ACTION	7	/* Mask */

#define PA_SIGNAL	0	/* Signal task in mp_SigTask */
#define PA_SOFTINT	1	/* Signal SoftInt in mp_SoftInt/mp_SigTask */
#define PA_IGNORE	2	/* Ignore arrival */

#define PA_CALL         3       /* Call function in mp_SigTask. This was never
                                   documented on AmigaOS and was never defined
                                   but would work for mp_Flags == 3 */

#define PA_FASTCALL     4       /* AROS extension. Like PA_SOFTINT, calls an
                                   Interrupt in mp_SoftInt, but passes the
                                   message as the third argument without doesn't
                                   add it to the message list and so doesn't
                                   require any locking, task switching or
                                   Disable()/Enable() pairs */

/* Message */
struct Message
{
    struct Node      mn_Node;
    struct MsgPort * mn_ReplyPort;  /* message reply port */
    UWORD	     mn_Length;     /* total message length, in bytes */
				    /* (include the size of the Message
				       structure in the length) */
};

struct MagicMessage
{
    struct Node     mn_Node;
    struct MsgPort * mn_ReplyPort;  /* message reply port */
    UWORD        mn_Length;     /* total message length, in bytes */
                    /* (include the size of the Message
                       structure in the length) */
    ULONG   mn_Magic;       /* can be used to figure out the message sender */
    ULONG   mn_Version;     /* version can be used to extend a message in later versions */
};

/* definition for entry Magic in Messages
   Magic is introduced to prevent Multiple Ports, for example if you´r using
   ScreenNotifications and DecorNotifications you must have two Ports as long
   as you cannot figure out which Messsage ist posted. With Magic this is no
   problem. */

#define   MAGIC_DECORATOR       0x8000001
#define   MAGIC_SCREENNOTIFY    0x8000002

#endif	/* EXEC_PORTS_H */
