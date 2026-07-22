/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    __atomic_is_lock_free() for toolchains whose runtime does not
    provide it. compiler-rt's builtins library implements every other
    __atomic_* helper, but deliberately omits this one (gcc toolchains
    get it from gcc's libatomic).

    The result is informational: it reports whether atomic operations
    on an object of the given size compile to lock-free instructions
    on this target. Alignment is assumed natural, as with a NULL ptr
    argument to gcc libatomic.
*/

#include <stddef.h>
#include <stdbool.h>

/* clang refuses a direct definition of its __atomic_* builtins, so -
 * as compiler-rt does - define under a different name and emit the
 * real symbol via an asm label.
 */
bool aros_atomic_is_lock_free(size_t size, const volatile void *ptr)
    __asm__("__atomic_is_lock_free");

bool aros_atomic_is_lock_free(size_t size, const volatile void *ptr)
{
    (void)ptr;

    switch (size)
    {
    case 1: return __atomic_always_lock_free(1, 0);
    case 2: return __atomic_always_lock_free(2, 0);
    case 4: return __atomic_always_lock_free(4, 0);
    case 8: return __atomic_always_lock_free(8, 0);
#if defined(__SIZEOF_INT128__)
    case 16: return __atomic_always_lock_free(16, 0);
#endif
    default:
        return false;
    }
}
