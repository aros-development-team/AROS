/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef VCMBOX_PRIVATE_H_
#define VCMBOX_PRIVATE_H_

#include <exec/nodes.h>
#include <exec/semaphores.h>
#include <inttypes.h>

struct VCMBoxBase {
    struct Node		        vcmb_Node;
    struct SignalSemaphore      vcmb_Sem;
};

#endif /* VCMBOX_PRIVATE_H_ */
