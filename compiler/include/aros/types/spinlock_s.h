#ifndef _AROS_TYPES_SPINLOCK_S_H_
#define _AROS_TYPES_SPINLOCK_S_H_

#include <aros/cpu.h>

typedef struct {
    volatile unsigned long lock;
} spinlock_t;

#define SPINLOCK_INIT_UNLOCKED  { 0 }
#define SPINLOCK_INIT_WRITE_LOCKED  { 0x80000000 }
#define SPINLOCK_INIT_READ_LOCKED(n) { n }

#define SPINLOCK_MODE_READ  0
#define SPINLOCK_MODE_WRITE 1

#endif /* ! _AROS_TYPES_SPINLOCK_S_H_ */
