/*
 * AROS Mesa compatibility macros
 *
 * Force-included via -include for Mesa sources that use
 * POSIX functions not available on AROS.
 */
#ifndef _AROS_MESA_COMPAT_H
#define _AROS_MESA_COMPAT_H

#include <stdio.h>
#include <stdlib.h>

/* open_memstream: AROS doesn't implement it.
 * Both callers in radeonsi check for NULL return and handle gracefully.
 */
static inline FILE *open_memstream(char **ptr, size_t *sizeloc)
{
    (void)ptr;
    (void)sizeloc;
    return NULL;
}

#endif /* _AROS_MESA_COMPAT_H */
