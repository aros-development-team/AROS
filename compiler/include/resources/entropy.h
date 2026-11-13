/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Public definitions for entropy.resource.
*/

#ifndef RESOURCES_ENTROPY_H
#define RESOURCES_ENTROPY_H

#include <exec/types.h>

#define ENTROPYNAME     "entropy.resource"

/* entropy.resource version */
#define ENTROPY_VERSION 1
#define ENTROPY_REVISION 0

/*
 * Source-quality flags, as reported by GetEntropyInfo().  They describe, in
 * architecture-neutral terms, which classes of entropy source the running
 * resource is able to draw on.  The software collector (EIF_SOFTWARE) is
 * always available; EIF_HARDWARE is set when an architecture back-end has
 * found a dedicated CPU/board entropy source (the identity of that source -
 * e.g. a particular x86 instruction - is an implementation detail and is not
 * exposed here).
 */
#define EIB_SOFTWARE    0       /* generic software collector (always set)  */
#define EIB_HARDWARE    1       /* some CPU/board hardware source is present */

#define EIF_SOFTWARE    (1L << EIB_SOFTWARE)
#define EIF_HARDWARE    (1L << EIB_HARDWARE)

#endif /* RESOURCES_ENTROPY_H */
