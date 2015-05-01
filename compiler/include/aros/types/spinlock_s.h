#ifndef _AROS_TYPES_SPINLOCK_S_H_
#define _AROS_TYPES_SPINLOCK_S_H_

#include <aros/cpu.h>

typedef struct {
    volatile unsigned long lock;
} spinlock_t;

#define SPINLOCK_MODE_READ  0
#define SPINLOCK_MODE_WRITE 1

#endif /* ! _AROS_TYPES_SPINLOCK_S_H_ */
