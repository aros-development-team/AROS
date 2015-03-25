/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef GPIO_PRIVATE_H_
#define GPIO_PRIVATE_H_

#include <exec/nodes.h>
#include <exec/semaphores.h>
#include <inttypes.h>

struct GPIOBase {
    struct Node		        gpio_Node;
    struct SignalSemaphore      gpio_Sem;
    unsigned int		gpio_periiobase;
};

#define ARM_PERIIOBASE GPIOBase->gpio_periiobase
#include <hardware/bcm283x.h>

#endif /* GPIO_PRIVATE_H_ */
