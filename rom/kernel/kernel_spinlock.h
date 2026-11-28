#ifndef KERNEL_SPINLOCK_H_
#define KERNEL_SPINLOCK_H_
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Atomic steps of the kernel spinlock, generic (empty) version.

    KrnSpinLock(), KrnSpinTryLock() and KrnSpinUnLock() have one body for
    every port, in rom/kernel. A port with real spinlocks overrides this
    file in its kernel directory, defines KRN_SPINLOCK_ATOMICS and provides,
    on the lock word laid out in <aros/types/spinlock_s.h>:

        BOOL krnSpinTryWrite(spinlock_t *lock)
            One attempt to change the word from SPINLOCK_UNLOCKED to
            SPINLOCKF_WRITE. TRUE if this caller made the change.

        BOOL krnSpinTryRead(spinlock_t *lock)
            Add one reader unless SPINLOCKF_WRITE or SPINLOCKF_UPDATING is
            set. It fails only for those bits, never because another reader
            changed the count at the same time.

        void krnSpinReleaseWrite(spinlock_t *lock)
            Store SPINLOCK_UNLOCKED, after everything written inside the
            lock, and wake any CPU parked in krnSpinPause().

        void krnSpinReleaseRead(spinlock_t *lock)
            Remove one reader, unless there is none; wake as above when the
            last one leaves.

        void krnSpinPause(void)
            What a CPU does between two attempts.

        struct Task *krnSpinThisTask(void)
            The task running on this CPU.

    Nothing else about the lock may differ between ports: the owner, the
    fail hook and the priority handling are in the shared code.

    Without KRN_SPINLOCK_ATOMICS the functions do nothing, which is what a
    port with one CPU and no real spinlocks wants.
*/

#include <aros/types/spinlock_s.h>

#endif /* KERNEL_SPINLOCK_H_ */
