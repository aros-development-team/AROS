#ifndef _AROS_TYPES_SPINLOCK_S_H_
#define _AROS_TYPES_SPINLOCK_S_H_

#include <aros/cpu.h>
#include <exec/types.h>

/* AROS_SPINLOCK_ALIGN is defined by <aros/cpu.h> (per-arch). Arches
 * whose allocators can hand out suitably aligned memory may set it to
 * __attribute__((__aligned__(N))) for cache-line isolation between
 * locks; the default is empty so spinlock_t gets natural alignment.
 * AROS_PLATFORM_SMP unconditionally embeds spinlock_t as padding in
 * struct MsgPort and SemaphoreRequest, so any container -- including
 * library bases like IconBase -- inherits the alignment. AllocMem
 * delivers only AROS_WORSTALIGN and OOP instance data even less, so a
 * declared alignment above that is a promise the compiler will act on
 * (Clang emits udf-trap checks on ARM, gcc emits aligned SSE stores on
 * x86) but memory does not keep. Keep it empty unless the arch's
 * allocator honours the alignment at runtime.
 *
 * AROS_SPINLOCK_ISOLATION (bytes, also from <aros/cpu.h>) provides the
 * false-sharing mitigation that alignment was meant for in a way that
 * cannot be miscompiled: the lock is padded to that size, keeping other
 * (hot) data off the lock's cache lines by distance rather than by
 * placement. Set it to the arch's cache-line isolation size (e.g. 128
 * on x86 per Intel guidance).
 */
#ifndef AROS_SPINLOCK_ALIGN
#define AROS_SPINLOCK_ALIGN
#endif

typedef struct {
    union
    {
        volatile struct {
            unsigned int        readcount : 24;
            unsigned int        _pad2 : 3;
            unsigned int        write : 1;
            unsigned int        _pad1 : 3;
            unsigned int        updating : 1;
        } slock;
        volatile unsigned char  block[4];
        volatile unsigned int   lock;
    };
    // The field s_Owner is set either to task owning the lock,
    // or NULL if the lock is free/read mode or was acquired in interrupt/supervisor mode
    void * s_Owner;
#if defined(AROS_SPINLOCK_ISOLATION)
    unsigned char s_Isolation[AROS_SPINLOCK_ISOLATION - (2 * sizeof(void *))];
#endif
} AROS_SPINLOCK_ALIGN spinlock_t;

#define SPINLOCK_UNLOCKED               0
#define SPINLOCKB_WRITE                 27
#define SPINLOCKB_UPDATING              31
#define SPINLOCKF_WRITE                 (1 << SPINLOCKB_WRITE)
#define SPINLOCKF_UPDATING              (1 << SPINLOCKB_UPDATING)

#define SPINLOCK_INIT_UNLOCKED          { SPINLOCK_UNLOCKED }
#define SPINLOCK_INIT_WRITE_LOCKED      { SPINLOCKF_WRITE }
#define SPINLOCK_INIT_READ_LOCKED(n)    { n }

#define SPINLOCK_MODE_READ              0
#define SPINLOCK_MODE_WRITE             1

#endif /* ! _AROS_TYPES_SPINLOCK_S_H_ */
