/*
 * atomic.h
 *
 *  Created on: Oct 23, 2010
 *      Author: misc
 */

#ifndef AROS_ARM_ATOMIC_H
#define AROS_ARM_ATOMIC_H

#ifdef __ARM_ARCH_7A__
#include <aros/arm/atomic_v7.h>
#endif

#if defined(__ARM_ARCH_6__) || defined(__ARM_ARCH_6ZK__)
#include <aros/arm/atomic_v6.h>
#endif

#endif /* AROS_ARM_ATOMIC_H */
