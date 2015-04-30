#ifndef _AROS_TYPES_SPINLOCK_S_H_
#define _AROS_TYPES_SPINLOCK_S_H_

#include <aros/cpu.h>

typedef struct {
    volatile unsigned long lock;
} spinlock_t;

#endif /* ! _AROS_TYPES_SPINLOCK_S_H_ */
