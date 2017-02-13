#ifndef _AROS_TYPES_SPINLOCK_S_H_
#define _AROS_TYPES_SPINLOCK_S_H_

#include <aros/cpu.h>

typedef struct {
    union
    {
        volatile struct {
            unsigned int        readcount : 24;
            unsigned int        _pad2 : 3;
            unsigned int        write : 1;
            unsigned int        _pad1 : 4;
        } slock;
        volatile unsigned long  lock;
    };
} __attribute__((__aligned__(128))) spinlock_t;

#define SPINLOCK_UNLOCKED               0
#define SPINLOCKB_WRITE                 27
#define SPINLOCKF_WRITE                 (1 << SPINLOCKB_WRITE)

#define SPINLOCK_INIT_UNLOCKED          { SPINLOCK_UNLOCKED }
#define SPINLOCK_INIT_WRITE_LOCKED      { SPINLOCKF_WRITE }
#define SPINLOCK_INIT_READ_LOCKED(n)    { n }

#define SPINLOCK_MODE_READ              0
#define SPINLOCK_MODE_WRITE             1

#endif /* ! _AROS_TYPES_SPINLOCK_S_H_ */
