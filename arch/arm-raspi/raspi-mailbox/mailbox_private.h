/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef MAILBOX_PRIVATE_H_
#define MAILBOX_PRIVATE_H_

#ifndef EXEC_LISTS_H
#   include <exec/lists.h>
#endif

#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif

#ifndef EXEC_SEMAPHORES_H
#   include <exec/semaphores.h>
#endif

struct mailbox_base {
    struct Node		Mailbox_Node;
};


#endif /* MAILBOX_PRIVATE_H_ */
